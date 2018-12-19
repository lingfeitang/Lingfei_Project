/*
 * Copyright (c) 2014-2015 The XiaoMi Tec. All rights reserved.
 */

#include <platform.h>
#include <openssl/rand.h>
#include <sys/types.h>
#include <string.h>
#include <miunlock.h>
#include <boot_verifier.h>
#include <openssl/rsa.h>
#include <scm.h>
#include <miunlock_keystore.h>
#include <pm8x41.h>

#define MILOCK_DEBUG
#ifdef MILOCK_DEBUG
    #define debug(...) dprintf(CRITICAL, __VA_ARGS__)
#else
    #define debug
#endif

#define RANDOMLEN 16
#define RANDOMLEN_E7 12
#define TOKENSIZE 128
#define MAX_PRODUCT_NAME_SIZE 16
#define MAX_CPUID_SIZE 32

#define MI_TOKEN_INFO_HEADER 0x55
#define MI_TOKEN_INFO_VERSION 0x01
#define MI_TOKEN_INFO_TYPE_RANG 0x01
#define MI_TOKEN_INFO_TYPE_CPUID 0x02
#define MI_TOKEN_INFO_TYPE_PRODUCT 0x03

#define MI_VERIFY_INFO_PLATFORM_VENDOR_QCOMM 0x01
#define MI_VERIFY_INFO_PLATFORM_VENDOR_MTK   0x02
#define MI_VERIFY_INFO_PLATFORM_VENDOR_PINECONE  0x03

/*** MUST BE BIG ENDIEN ***/
struct token_header {
    uint8_t head;
    uint8_t version;
    uint8_t platform_vendor_id;
    uint8_t length;
} __attribute__((packed));

/*** MUST BE BIG ENDIEN ***/
struct token_item {
    uint8_t type;
    uint8_t length;
    uint8_t data[0];
} __attribute__((packed));

extern unsigned int cpuid_len;
extern unsigned int cpuid_len_e7;
char Token[TOKENSIZE] = {0};

static int test_cpuid_length;
static unsigned char test_cpuid[MAX_CPUID_SIZE];
static int using_test_cpuid;

unsigned char* platform_get_cpuid();
unsigned char* platform_get_cpuid_e7();
static uint32_t read_der_message_length1(unsigned char* input);
static int Token_decrypt_signature_rsa(int sig_size, unsigned char *signature_ptr,
        int plain_size, unsigned char *plain_text);
static char* base64_encode(const unsigned char * bindata, char * base64, int binlength, int baselength);
static int base64_decode(const char * base64, unsigned char * bindata, int baselength, int binlength);
static unsigned char * base64_decode1(const char * base64, unsigned char * bindata );
void get_product_name(unsigned char *buf);

//for E7
static int Token_decrypt_signature_rsa_e7(int sig_size, unsigned char *signature_ptr, unsigned char *plain_text);
static char* base64_encode_e7( const unsigned char * bindata, char * base64, int binlength );
static int base64_decode_e7( const char * base64, unsigned char * bindata );
static char* base64_encode1_e7( const unsigned char * bindata, char * base64, int binlength );
static unsigned char * base64_decode1_e7( const char * base64, unsigned char * bindata );

/********************************************************
 * ODM MUST IMPLEMENTS THE FOLLOWING CALLBACK FUNCTIONS *
 *******************************************************/
/*
 * Get platform vendor id.
 * Currently supported:
 *  MI_VERIFY_INFO_PLATFORM_VENDOR_QCOMM for all qualcomm projects
 *  MI_VERIFY_INFO_PLATFORM_VENDOR_MTK for all mtk projects
 *  MI_VERIFY_INFO_PLATFORM_VENDOR_PINECONE for all pincone projects
 *
 * @return Platform vendor id.
 */
static char get_platform_vendor_callback()
{
    /* ODM_MAY_MODIFY */
    return MI_VERIFY_INFO_PLATFORM_VENDOR_QCOMM;
}

/*
 * Get real product name from hardware. (for example: mido, rosy, whyred, etc.).
 *
 * @param product Input buffer.
 * @param len Size of input buffer.
 * @return Length of product name(NOT include '\0' if appended). <=0 for failure.
 */
static int get_product_name_callback(unsigned char *product, int len)
{
	unsigned char product_name[64] = {0};
	int product_name_len = 0;

	get_product_name(product_name);
	product_name_len = strlen((char *)product_name);
	if (product_name_len > len) {
		return -1;
	}
	memcpy(product, (void *)product_name, product_name_len);
	dprintf(CRITICAL, "product = %s\n", product);
    return product_name_len;
}

/*
 * Get cpuid
 *
 * @param cpuid Input buffer.
 * @param len Size of input buffer.
 * @return Length of cpuid(NOT include '\0' if appended). <=0 for failure.
 */
static int get_cpuid_callback(unsigned char *cpuid, int len)
{
    unsigned char *id;
    int id_len;

    if (using_test_cpuid) {
        id = test_cpuid;
        id_len = test_cpuid_length;
    } else {
        /* ODM MAY MODIFY */
        id = platform_get_cpuid();
        if (id == NULL || cpuid_len > (unsigned int)len) {
            return -1;
        }
        id_len = cpuid_len;
    }
    memcpy(cpuid, id, id_len);
    return id_len;
}

/************END OF CALLBACK FUNCTIONS *******************/
/*********************************************************/

void print_buf(char *name,unsigned char *buf,int start,int end,int flag)
{
	int i;
	printf("[%s]:",name);
	for(i=start;i<=end;i++){
		if(flag)
			printf(" %d",buf[i]);
		else
			printf("%c",buf[i]);
	}
	printf("\n");
}

int generate_Token()
{
    unsigned char buf[(TOKENSIZE-1)/4*3] = {0}; /* Token include nil('\0'), so -1 is needed*/
    unsigned long buf_end = (unsigned long)(buf + sizeof(buf));
    char b64[TOKENSIZE] = {0};
    struct token_header *header;
    struct token_item *item;
    size_t total_length;

	memset(buf, 0, sizeof(buf));
	memset(b64, 0, sizeof(b64));
	memset(Token, 0, sizeof(Token));

    /* 1. Fill token header */
    header = (struct token_header*) buf;
    if (buf_end - (unsigned long)header < sizeof(*header)) {
        dprintf(CRITICAL, "Token: TOKENSIZE too small\n");
        return -1;
    }
    header->head = MI_TOKEN_INFO_HEADER;
    header->version = MI_TOKEN_INFO_VERSION;
    header->platform_vendor_id = get_platform_vendor_callback();
    header->length = 0;

    /* 2. random */
    item = (struct token_item *) (buf + sizeof(*header));
    if (buf_end - (unsigned long)item < sizeof(*item) + RANDOMLEN) {
        dprintf(CRITICAL, "Token: TOKENSIZE too small: ramdom\n");
        return -1;
    }
    item->type = MI_TOKEN_INFO_TYPE_RANG;
    item->length = RANDOMLEN;
    if (RAND_bytes(item->data, RANDOMLEN) <= 0) {
        dprintf(CRITICAL, "Token: failed to get random\n");
        return -1;
    }
    item = (struct token_item *) ((char*)item + sizeof(*item) + item->length);

    /* 3. product */
    if (buf_end - (unsigned long)item < sizeof(*item) + MAX_PRODUCT_NAME_SIZE) {
        dprintf(CRITICAL, "Token: TOKENSIZE too small: product\n");
        return -1;
    }
    item->type = MI_TOKEN_INFO_TYPE_PRODUCT;
    item->length = get_product_name_callback(item->data, MAX_PRODUCT_NAME_SIZE);
    if (item->length <= 0) {
        dprintf(CRITICAL, "Token: failed to get product name\n");
        return -1;
    }
    item = (struct token_item *) ((char*)item + sizeof(*item) + item->length);

    /* 4. cpuid */
    if (buf_end - (unsigned long)item < sizeof(*item) + MAX_CPUID_SIZE) {
        dprintf(CRITICAL, "Token: TOKENSIZE too small: cpuid\n");
        return -1;
    }
    item->type = MI_TOKEN_INFO_TYPE_CPUID;
    item->length = get_cpuid_callback(item->data, MAX_CPUID_SIZE);
    if (item->length <= 0) {
        dprintf(CRITICAL, "Token: failed to get cpuid\n");
        return -1;
    }
    item = (struct token_item *) ((char*)item + sizeof(*item) + item->length);
    total_length = (size_t) item - (size_t) buf;
    header->length = total_length - sizeof(*header);

    /* 5. base64 data */
    if (base64_encode(buf, b64, total_length, sizeof(b64)) == NULL) {
        dprintf(CRITICAL, "Token: failed to base64\n");
        return -1;
    }
    strncpy(Token, b64, sizeof(Token) - 1);
	print_buf("token",(unsigned char*)Token,0,30,1);
    dprintf(CRITICAL, "Token is %s\n", Token);
    return 0;
}

int generate_Token_e7()
{
    unsigned char buf[TOKENSIZE] = {0};
    char b64[TOKENSIZE] = {0};
    int ret;

	memset(buf,0,TOKENSIZE);
	memset(b64,0,TOKENSIZE);
	memset(Token,0,TOKENSIZE);
    ret = RAND_bytes(buf, RANDOMLEN_E7);
    if (ret < 0)
        return ret;


    unsigned char *id = platform_get_cpuid_e7();
    for (unsigned int i = 0; i < cpuid_len_e7; ++i)
    {
        buf[i + RANDOMLEN_E7] = id[i];
    }
	print_buf("rand+id",buf,0,17,1);
    base64_encode_e7(buf, b64, RANDOMLEN_E7 + cpuid_len_e7);
    strcpy(Token, b64);
	Token[24] = 0;
	print_buf("token",(unsigned char*)Token,0,30,1);
    dprintf(INFO, "Token is %s\n", b64);
    return 0;
}

int Token_verify_compare(char * sig, unsigned sz)
{
    unsigned char plain_text[TOKENSIZE];
    int count;
    int ret;

	print_buf("sig_compare",(unsigned char*)sig,0,30,1);
    memset(plain_text, 0, sizeof(plain_text));
    count = Token_decrypt_signature_rsa(sz, (unsigned char*)sig, sizeof(plain_text), plain_text);
    if (count <= 0) {
        dprintf(CRITICAL, "Token Decryptiton Failed\n");
        return -1;
    }
	print_buf("plain_text1",plain_text,0,30,1);
	print_buf("token1",(unsigned char*)Token,0,30,1);
    if (!strncmp(Token, (char *)plain_text, count)) {
        ret = 0;
        dprintf(INFO, "Token Verification Succesfully\n");
    } else {
        ret = -1;
        dprintf(CRITICAL, "Token Verification Failed\n");
    }

    return ret;
}

int Token_verify_compare_e7(char * sig, unsigned sz)
{
    unsigned char plain_text[TOKENSIZE];
    int ret= -1;

	print_buf("sig_compare",(unsigned char*)sig,0,30,1);
    memset(plain_text, '\0', TOKENSIZE);
    if (!Token_decrypt_signature_rsa_e7(sz, (unsigned char *)sig, plain_text))
    {
        ret = -1;
        dprintf(CRITICAL, "Token Decryptiton Failed\n");
        return ret;
    }
	print_buf("plain_text1",plain_text,0,30,1);
	print_buf("token1",(unsigned char*)Token,0,30,1);
    if (!strcmp(Token, (char *)plain_text))
    {
        ret = 0;
        dprintf(INFO, "Token Verification Succesfully\n");
    }
    else
    {
        ret = -1;
        dprintf(CRITICAL, "Token Verification Failed\n");
    }

    return ret;
}

int Token_verify_SN(char * sig, unsigned sz)
{
    unsigned char buf[TOKENSIZE/4*3];
    unsigned char plain_text[TOKENSIZE];
    unsigned char cpuid[MAX_CPUID_SIZE];
    unsigned char product[MAX_PRODUCT_NAME_SIZE];
    int length, count;
    struct token_header *header;
    struct token_item *item, *item_end;

    memset(plain_text, 0, sizeof(plain_text));
	memset(buf, 0, sizeof(buf));
    memset(cpuid, 0, sizeof(cpuid));
    memset(product, 0, sizeof(product));

    /* decrypt token */
    count = Token_decrypt_signature_rsa(sz, (unsigned char*)sig, sizeof(plain_text), plain_text);
    if (count <= 0) {
        dprintf(CRITICAL, "SN Decryptiton Failed\n");
        return -1;
    }
    count = base64_decode((char*)plain_text, buf, count, sizeof(buf));
    if (count <= 0) {
        dprintf(CRITICAL, "SN base64 decode failed\n");
        return -1;
    }

    /* check token validity */
    if ((size_t) count < sizeof(*header)) {
        dprintf(CRITICAL, "SN token too small\n");
        return -1;
    }
    header = (struct token_header*)buf;
    if (header->length + sizeof(*header) != (size_t)count
            || header->head != MI_TOKEN_INFO_HEADER
            || header->version > MI_TOKEN_INFO_VERSION) {
        dprintf(CRITICAL, "SN token header invalid\n");
        return -1;
    }

    item = (struct token_item*) (buf + sizeof(*header));
    item_end = (struct token_item*) (buf + count);
    while (item < item_end) {
        size_t remind = (size_t) item_end - (size_t) item;
        if (remind < sizeof(*item) || remind < sizeof(*item) + item->length) {
            dprintf(CRITICAL, "SN paritial item\n");
            return -1;
        }
        switch (item->type) {
            case MI_TOKEN_INFO_TYPE_RANG:
                /* ignore random */
                break;
            case MI_TOKEN_INFO_TYPE_CPUID:
                /* check cpuid */
                length = get_cpuid_callback(cpuid, sizeof(cpuid));
                if (length <= 0) {
                    dprintf(CRITICAL, "SN get cpuid failed\n");
                    return -1;
                }
                if (length != item->length || memcmp(cpuid, item->data, length)) {
                    dprintf(CRITICAL, "SN cpuid invalid\n");
                    return -1;
                }
                break;
            case MI_TOKEN_INFO_TYPE_PRODUCT:
                /* check product */
                length = get_product_name_callback(product, sizeof(product));
                if (length <= 0) {
                    dprintf(CRITICAL, "SN get product failed\n");
                    return -1;
                }
                if (length != item->length || memcmp(product, item->data, length)) {
                    dprintf(CRITICAL, "SN product invalid\n");
                    return -1;
                }
                break;
            default:
                dprintf(CRITICAL, "SN unknown type");
                return -1;
                break;
        }
        item = (struct token_item*) ((char*)item + sizeof(*item) + item->length);
    }

    dprintf(INFO, "SN verify ok\n");
    return 0;
}

int Token_verify_SN_e7(char * sig, unsigned sz)
{
    unsigned char buf[TOKENSIZE * 4];
    unsigned char plain_text[TOKENSIZE];
    int length, ret= -1;

    memset(plain_text, '\0', TOKENSIZE);
	memset(buf,0,TOKENSIZE);
    if (!Token_decrypt_signature_rsa_e7(sz, (unsigned char *)sig, plain_text))
    {
        ret = -1;
        dprintf(CRITICAL, "SN Decryptiton Failed\n");
        return ret;
    }
    length = base64_decode_e7((char *)plain_text, buf);
    buf[length] = '\0';
	print_buf("buf_sn",(unsigned char*)buf,0,30,1);
    unsigned char *id = platform_get_cpuid_e7();
	print_buf("id_sn",(unsigned char*)id,0,5,1);
    /*CPU ID(SN) starts with RANDOMLEN*/
    if (!memcmp((char *)id, (char *)(buf + RANDOMLEN_E7),6))
    {
        ret = 0;
        dprintf(INFO, "SN Verification Succesfully\n");
    }
    else
    {
        ret = -1;
        dprintf(CRITICAL, "SN Verification Failed\n");
    }

    return ret;
}

static int Token_decrypt_signature_rsa(int sig_size, unsigned char *signature_ptr,
        int plain_size, unsigned char *plain_text)
{
    KEYSTORE *ks = NULL;
    uint32_t len = 0;
    RSA *rsa = NULL;
    int ret = -1;
    const unsigned char *input = MIUNLOCK_KEYSTORE;

    len = read_der_message_length1((unsigned char *)input);
    if(!len)
    {
        dprintf(CRITICAL, "Token RSA keystore length invalid\n");
        return ret;
    }

    ks = d2i_KEYSTORE(NULL, (const unsigned char **) &input, len);
    rsa = ks->mykeybag->mykey->key_material;

    if (rsa == NULL) {
        dprintf(CRITICAL, "ERROR: Token Verification Error, RSA_KEY is NULL!\n");
        return ret;
    }

    if ((unsigned long)plain_size < sizeof(Token)) {
        dprintf(CRITICAL, "ERROR: Token verification error, decrypt buffer too small\n");
        return ret;
    }

    ret = RSA_public_decrypt(sig_size, signature_ptr, plain_text, rsa, RSA_PKCS1_PADDING);
    if (ret > 0 && (unsigned long) ret >= sizeof(Token)) {
        dprintf(CRITICAL, "ERROR: Token verification error, token buffer too small\n");
        return -1;
    }

    return ret;
}
static int Token_decrypt_signature_rsa_e7(int sig_size, unsigned char *signature_ptr, unsigned char *plain_text)
{
    KEYSTORE *ks = NULL;
    uint32_t len = 0;
    RSA *rsa = NULL;
    int ret = -1;
    const unsigned char *input = MIUNLOCK_KEYSTORE;

    len = read_der_message_length1((unsigned char *)input);
    if(!len)
    {
        dprintf(CRITICAL, "Token RSA keystore length invalid\n");
        return ret;
    }

    ks = d2i_KEYSTORE(NULL, (const unsigned char **) &input, len);
    rsa = ks->mykeybag->mykey->key_material;

    if (rsa == NULL) {
        dprintf(CRITICAL, "ERROR: Token Verification Error, RSA_KEY is NULL!\n");
        return ret;
    }

    ret = RSA_public_decrypt(sig_size, signature_ptr, plain_text, rsa, RSA_PKCS1_PADDING);

    return ret;
}

/*Implement RAND_poll in order to use openssl RAND_bytes API*/
int RAND_poll(void)
{
   bigtime_t l = (bigtime_t)pmi_get_RTC();
   dprintf(CRITICAL, "RTC %llx\n", l);
   RAND_add(&l,sizeof(l),100);

   return 1;
}
/*Base64 encode implemention*/
const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char* base64_encode( const unsigned char * bindata, char * base64,
        int binlength, int baselength)
{
    int i, j;
    unsigned char current;

    if ((binlength+2)/3*4 > baselength-1) {
        return NULL;
    }

    for ( i = 0, j = 0 ; i < binlength ; i += 3 )
    {
        current = (bindata[i] >> 2) ;
        current &= (unsigned char)0x3F;
        base64[j++] = base64char[(int)current];
        current = ( (unsigned char)(bindata[i] << 4 ) ) & ( (unsigned char)0x30 ) ;
        if ( i + 1 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            base64[j++] = '=';
            break;
        }
        current |= ( (unsigned char)(bindata[i+1] >> 4) ) & ( (unsigned char) 0x0F );
        base64[j++] = base64char[(int)current];
        current = ( (unsigned char)(bindata[i+1] << 2) ) & ( (unsigned char)0x3C ) ;
        if ( i + 2 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            break;
        }
        current |= ( (unsigned char)(bindata[i+2] >> 6) ) & ( (unsigned char) 0x03 );
        base64[j++] = base64char[(int)current];
        current = ( (unsigned char)bindata[i+2] ) & ( (unsigned char)0x3F ) ;
        base64[j++] = base64char[(int)current];
    }
    base64[j] = '\0';
    return base64;
}

static char* base64_encode_e7( const unsigned char * bindata, char * base64, int binlength )
{
    int i, j;
    unsigned char current;

    for ( i = 0, j = 0 ; i < binlength ; i += 3 )
    {
        current = (bindata[i] >> 2) ;
        current &= (unsigned char)0x3F;
        base64[j++] = base64char[(int)current];
        current = ( (unsigned char)(bindata[i] << 4 ) ) & ( (unsigned char)0x30 ) ;
        if ( i + 1 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            base64[j++] = '=';
            break;
        }
        current |= ( (unsigned char)(bindata[i+1] >> 4) ) & ( (unsigned char) 0x0F );
        base64[j++] = base64char[(int)current];
        current = ( (unsigned char)(bindata[i+1] << 2) ) & ( (unsigned char)0x3C ) ;
        if ( i + 2 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            break;
        }
        current |= ( (unsigned char)(bindata[i+2] >> 6) ) & ( (unsigned char) 0x03 );
        base64[j++] = base64char[(int)current];
        current = ( (unsigned char)bindata[i+2] ) & ( (unsigned char)0x3F ) ;
        base64[j++] = base64char[(int)current];
    }
    base64[j] = '\0';
    return base64;
}


static int base64_decode(const char * base64, unsigned char * bindata,
        int baselength, int binlength)
{
    int i, j;
    unsigned char k;
    unsigned char temp[4];

    if (baselength / 4 * 3 > binlength) {
        return -1;
    }

    for ( i = 0, j = 0; i<baselength; i += 4 )
    {
        memset( temp, 0xFF, sizeof(temp) );
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i] )
                temp[0]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
       {
            if ( base64char[k] == base64[i+1] )
                temp[1]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i+2] )
                temp[2]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i+3] )
                temp[3]= k;
        }

       bindata[j++] = ((unsigned char)(((unsigned char)(temp[0] << 2))&0xFC)) |
                ((unsigned char)((unsigned char)(temp[1]>>4)&0x03));
        if ( base64[i+2] == '=' )
            break;

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[1] << 4))&0xF0)) |
                ((unsigned char)((unsigned char)(temp[2]>>2)&0x0F));
        if ( base64[i+3] == '=' )
            break;

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[2] << 6))&0xF0)) |
                ((unsigned char)(temp[3]&0x3F));
    }
    return j;
}

static int base64_decode_e7( const char * base64, unsigned char * bindata )
{
    int i, j;
    unsigned char k;
    unsigned char temp[4];
    for ( i = 0, j = 0; base64[i] != '\0' ; i += 4 )
    {
        memset( temp, 0xFF, sizeof(temp) );
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i] )
                temp[0]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
       {
            if ( base64char[k] == base64[i+1] )
                temp[1]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i+2] )
                temp[2]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i+3] )
                temp[3]= k;
        }

       bindata[j++] = ((unsigned char)(((unsigned char)(temp[0] << 2))&0xFC)) |
                ((unsigned char)((unsigned char)(temp[1]>>4)&0x03));
        if ( base64[i+2] == '=' )
            break;

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[1] << 4))&0xF0)) |
                ((unsigned char)((unsigned char)(temp[2]>>2)&0x0F));
        if ( base64[i+3] == '=' )
            break;

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[2] << 6))&0xF0)) |
                ((unsigned char)(temp[3]&0x3F));
    }
    return j;
}

static uint32_t read_der_message_length1(unsigned char* input)
{
    uint32_t len = 0;
    int pos = 0;
    uint8_t len_bytes = 1;

    /* Check if input starts with Sequence id (0X30) */
    if(input[pos] != 0x30)
        return len;
    pos++;

    /* A length of 0xAABBCCDD in DER encoded messages would be sequence of
       following octets 0xAA, 0xBB, 0XCC, 0XDD.

       To read length - read each octet and shift left by 1 octect before
       reading next octet.
    */
    /* check if short or long length form */
    if(input[pos] & 0x80)
    {
        len_bytes = (input[pos] & ~(0x80));
        pos++;
    }
    while(len_bytes)
    {
        /* Shift len by 1 octet */
        len = len << 8;

        /* Read next octet */
        len = len | input[pos];
        pos++; len_bytes--;
    }

    /* Add number of octets representing sequence id and length  */
    len += pos;

    return len;
}

/***************************************************
 *********************TEST*************************/

static int c2b(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'z') {
        return c - 'a' + 10;
    } else { // Ignore error case.
        return c - 'A' + 10;
    }
}

static int hex2bin(const char *hex, unsigned char *buf, size_t sz)
{
    int i;
    int len = strlen(hex);
    if (len % 2 || len / 2 > (int)sz) {
        return 0;
    }

    for (i=0; i<len; i+=2) {
        int hi = c2b(hex[i]);
        int low = c2b(hex[i+1]);
        buf[i/2] = hi << 4 | low;
    }
    return len / 2;
}

/*
 * fastboot oem unlock_test gen [cpuid_hex]
 * fastboot oem unlock_test verify [cpuid_hex] sig_file
 */
void unlock_test(const char *arg, void *data, unsigned sz, void (*cb)(const char *msg))
{
    char buf[128];
    int argc = 0;
    const char *test_type = NULL;
    const char *cpuid = NULL;
    const char *p;

    strncpy(buf, arg, sizeof(buf)-1);
    p = strtok(buf, " \t\v");
    while (p) {
        argc++;
        switch (argc) {
            case 1:
                test_type = p;
                break;
            case 2:
                cpuid = p;
                break;
            default:
                /* ignore others */
                break;
        }
        p = strtok(NULL, " \t\v");
    }

    if (argc < 1) {
        if (cb) {
            cb("Too few arguments");
            return;
        }
    }

    if (cpuid != NULL) {
        test_cpuid_length = hex2bin(cpuid, test_cpuid, sizeof(test_cpuid));
        if (test_cpuid_length) {
            using_test_cpuid = 1;
        }
    }

    if (strcmp(test_type, "gen") == 0) {
        if (generate_Token() < 0) {
            if (cb) cb("generate_Token_failed");
        } else {
            if (cb) cb(Token);
        }
    } else if (strcmp(test_type, "verify") == 0) {
        if (Token_verify_compare(data, sz) == 0) {
            if (cb) cb("compare_ok");
        } else {
            if (cb) cb("compare_fail");
        }
        if (Token_verify_SN(data, sz) == 0) {
            if (cb) cb("sn_ok");
        } else {
            if (cb) cb("sn_fail");
        }
    } else {
        if (cb) cb("Unsupported arguments");
    }

    if (using_test_cpuid) {
        using_test_cpuid = 0;
    }
}
