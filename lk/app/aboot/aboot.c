/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Copyright (c) 2009-2018, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of The Linux Foundation nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <app.h>
#include <debug.h>
#include <arch/arm.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <kernel/thread.h>
#include <arch/ops.h>

#include <dev/flash.h>
#include <dev/flash-ubi.h>
#include <lib/ptable.h>
#include <dev/keys.h>
#include <dev/fbcon.h>
#include <baseband.h>
#include <target.h>
#include <mmc.h>
#include <partition_parser.h>
#include <ab_partition_parser.h>
#include <verifiedboot.h>
#include <platform.h>
#include <crypto_hash.h>
#include <malloc.h>
#include <boot_stats.h>
#include <sha.h>
#include <platform/iomap.h>
#include <boot_device.h>
#include <boot_verifier.h>
#include <image_verify.h>
 #include <miunlock.h>
#include <decompress.h>
#include <platform/timer.h>
#include <platform/gpio.h>
#include <sys/types.h>
#include <pm8x41.h>
#include <pm8x41_hw.h>
#if USE_RPMB_FOR_DEVINFO
#include <rpmb.h>
#endif

#if ENABLE_WBC
#include <pm_app_smbchg.h>
#endif

#if DEVICE_TREE
#include <libfdt.h>
#include <dev_tree.h>
#endif

#if WDOG_SUPPORT
#include <wdog.h>
#endif

#if PON_VIB_SUPPORT
#include <vibrator.h>
#define VIBRATE_TIME 250
#endif

#include <reboot.h>
#include "image_verify.h"
#include "recovery.h"
#include "bootimg.h"
#include "fastboot.h"
#include "sparse_format.h"
#include "meta_format.h"
#include "mmc.h"
#include "devinfo.h"
#include "board.h"
#include "scm.h"
#include "mdtp.h"
#include "secapp_loader.h"
#include <menu_keys_detect.h>
#include <display_menu.h>
#include "fastboot_test.h"

#ifdef SAMSUNG_RPMB_RESET
#include "mmc_sdhci.h"
#include "SAMSUNG_FW.h"
#endif
extern ddr_result ddrresult;
unsigned pu_reason = 0;
extern  bool target_use_signed_kernel(void);
extern void platform_uninit(void);
extern void target_uninit(void);
extern int get_target_boot_params(const char *cmdline, const char *part,
				  char **buf);

static int display_fastboot(void); //add pit.by feng

void *info_buf;
void write_device_info_mmc(device_info *dev);
void write_device_info_flash(device_info *dev);
static int aboot_save_boot_hash_mmc(uint32_t image_addr, uint32_t image_size);
static int aboot_frp_unlock(char *pname, void *data, unsigned sz);
static inline uint64_t validate_partition_size();
bool pwr_key_is_pressed = false;
static bool is_systemd_present=false;
static void publish_getvar_multislot_vars();
/* fastboot command function pointer */
typedef void (*fastboot_cmd_fn) (const char *, void *, unsigned);
bool get_perm_attr_status();

struct fastboot_cmd_desc {
	char * name;
	fastboot_cmd_fn cb;
};

#define EXPAND(NAME) #NAME
#define TARGET(NAME) EXPAND(NAME)

#define DISPLAY_PANEL_HDMI "hdmi"

#ifdef MEMBASE
#define EMMC_BOOT_IMG_HEADER_ADDR (0xFF000+(MEMBASE))
#else
#define EMMC_BOOT_IMG_HEADER_ADDR 0xFF000
#endif

#ifndef MEMSIZE
#define MEMSIZE 1024*1024
#endif

#define MAX_TAGS_SIZE   1024
#define PLL_CODES_OFFSET 4096
#define WARM_RST                256

/* make 4096 as default size to ensure EFS,EXT4's erasing */
#define DEFAULT_ERASE_SIZE  4096
#define MAX_PANEL_BUF_SIZE 196
#define FOOTER_SIZE 16384

#define DISPLAY_DEFAULT_PREFIX "mdss_mdp"
#define BOOT_DEV_MAX_LEN  64

#define IS_ARM64(ptr) (ptr->magic_64 == KERNEL64_HDR_MAGIC) ? true : false

#define ADD_OF(a, b) (UINT_MAX - b > a) ? (a + b) : UINT_MAX

//Size of the header that is used in case the boot image has
//a uncompressed kernel + appended dtb
#define PATCHED_KERNEL_HEADER_SIZE 20

//String used to determine if the boot image has
//a uncompressed kernel + appended dtb
#define PATCHED_KERNEL_MAGIC "UNCOMPRESSED_IMG"


#define MAX_CRC_LIST_LEN  45
#define CRC_LIST_HEADER    "CRC-LIST"

#define MAX_SPARSECRC_LIST_LEN  3
#define SPARSECRC_LIST_HEADER    "SPARSECRC-LIST"

#if USE_BOOTDEV_CMDLINE
static const char *emmc_cmdline = " androidboot.bootdevice=";
#else
static const char *emmc_cmdline = " androidboot.emmc=true";
#endif
static const char *bootloader_cmdline = " androidboot.bootloader=";
static const char *usb_sn_cmdline = " androidboot.serialno=";
static const char *androidboot_mode = " androidboot.mode=";

static const char *systemd_ffbm_mode = " systemd.unit=ffbm.target";
static const char *alarmboot_cmdline = " androidboot.alarmboot=true";
static const char *loglevel         = " quiet";
static const char *battchg_pause = " androidboot.mode=charger";
static const char *auth_kernel = " androidboot.authorized_kernel=true";
static const char *secondary_gpt_enable = " gpt";
#ifdef MDTP_SUPPORT
static const char *mdtp_activated_flag = " mdtp";
#endif
static const char *ddr_sorting_addr_cmdline = " ddr_sorting=";

static const char *baseband_apq     = " androidboot.baseband=apq";
static const char *baseband_msm     = " androidboot.baseband=msm";
static const char *baseband_csfb    = " androidboot.baseband=csfb";
static const char *baseband_svlte2a = " androidboot.baseband=svlte2a";
static const char *baseband_mdm     = " androidboot.baseband=mdm";
static const char *baseband_mdm2    = " androidboot.baseband=mdm2";
static const char *baseband_sglte   = " androidboot.baseband=sglte";
static const char *baseband_dsda    = " androidboot.baseband=dsda";
static const char *baseband_dsda2   = " androidboot.baseband=dsda2";
static const char *baseband_sglte2  = " androidboot.baseband=sglte2";
static const char *warmboot_cmdline = " qpnp-power-on.warm_boot=1";
static const char *baseband_apq_nowgr   = " androidboot.baseband=baseband_apq_nowgr";
static const char *androidboot_slot_suffix = " androidboot.slot_suffix=";
static const char *skip_ramfs = " skip_initramfs";

#if HIBERNATION_SUPPORT
static const char *resume = " resume=/dev/mmcblk0p";
#endif

#ifdef INIT_BIN_LE
static const char *sys_path_cmdline = " rootwait ro init="INIT_BIN_LE;
#else
static const char *sys_path_cmdline = " rootwait ro init=/init";

#endif

#if VERITY_LE
static const char *verity_dev = " root=/dev/dm-0";
static const char *verity_system_part = " dm=\"system";
static const char *verity_params = " none ro,0 1 android-verity /dev/mmcblk0p";

static const char *sys_path = " root=/dev/mmcblk0p";
#define MAX_DTBO_IDX_STR 64
static const char *android_boot_dtbo_idx = " androidboot.dtbo_idx=";
#else
static const char *sys_path = " root=/dev/mmcblk0p";

#define MAX_DTBO_IDX_STR 64
static const char *android_boot_dtbo_idx = " androidboot.dtbo_idx=";
#endif
static const char *secureboot_cmdline    = " androidboot.secureboot=";
#if VERIFIED_BOOT
static const char *verity_mode = " androidboot.veritymode=";
static const char *verified_state= " androidboot.verifiedbootstate=";
static const char *keymaster_v1= " androidboot.keymaster=1";
//indexed based on enum values, green is 0 by default

struct verified_boot_verity_mode vbvm[] =
{
#if ENABLE_VB_ATTEST
	{false, "eio"},
#else
	{false, "logging"},
#endif
	{true, "enforcing"},
};
struct verified_boot_state_name vbsn[] =
{
	{GREEN, "green"},
	{ORANGE, "orange"},
	{YELLOW,"yellow"},
	{RED,"red" },
};
#endif
/*As per spec delay wait time before shutdown in Red state*/
#define DELAY_WAIT 30000
static unsigned page_size = 0;
static unsigned page_mask = 0;
static unsigned mmc_blocksize = 0;
static unsigned mmc_blocksize_mask = 0;
static char ffbm_mode_string[FFBM_MODE_BUF_SIZE];
static unsigned char bootloader_string[64];
static bool boot_into_ffbm;
static char *target_boot_params = NULL;
static bool boot_reason_alarm;
static bool devinfo_present = true;
bool boot_into_fastboot = false;
static uint32_t dt_size = 0;
static char *vbcmdline;
static bootinfo info = {0};
static void *recovery_dtbo_buf = NULL;
static uint32_t recovery_dtbo_size = 0;

/* Assuming unauthorized kernel image by default */
static int auth_kernel_img = 0;
static device_info device = {DEVICE_MAGIC,0,0,0,0,{0},{0},{0},1,{0},0,{0},{0}};
static bool is_allow_unlock = 0;

static char frp_ptns[2][8] = {"config","frp"};

static const char *critical_flash_allowed_ptn[] = {
	"aboot",
	"rpm",
	"tz",
	"sbl",
	"sdi",
	"sbl1",
	"xbl",
	"hyp",
	"pmic",
	"bootloader",
	"devinfo",
	"partition"};

struct atag_ptbl_entry
{
	char name[16];
	unsigned offset;
	unsigned size;
	unsigned flags;
};

/*
 * Partition info, required to be published
 * for fastboot
 */
struct getvar_partition_info {
	char part_name[MAX_GPT_NAME_SIZE]; /* Partition name */
	char getvar_size[MAX_GET_VAR_NAME_SIZE]; /* fastboot get var name for size */
	char getvar_type[MAX_GET_VAR_NAME_SIZE]; /* fastboot get var name for type */
	char size_response[MAX_RSP_SIZE];        /* fastboot response for size */
	char type_response[MAX_RSP_SIZE];        /* fastboot response for type */
};

/*
 * Update the part_type_known for known paritions types.
 */
#define RAW_STR "raw"
#define EXT_STR "ext4"
#define F2FS_STR "f2fs"

#define FS_SUPERBLOCK_OFFSET    0x400
#define EXT_MAGIC    0xEF53
#define EXT_MAGIC_OFFSET_SB   0x38
#define F2FS_MAGIC   0xF2F52010  // F2FS Magic Number
#define F2FS_MAGIC_OFFSET_SB 0x0

typedef enum fs_signature_type {
	EXT_FS_SIGNATURE = 1,
	EXT_F2FS_SIGNATURE = 2,
	NO_FS = -1
} fs_signature_type;

struct getvar_partition_info part_info[NUM_PARTITIONS];
struct getvar_partition_info part_type_known[] =
{
	{ "system"     , "partition-size:", "partition-type:", "", "ext4" },
	{ "userdata"   , "partition-size:", "partition-type:", "", "ext4" },
	{ "cache"      , "partition-size:", "partition-type:", "", "ext4" },
	{ "recoveryfs" , "partition-size:", "partition-type:", "", "ext4" },
};

char rollback_str[10];
char max_download_size[MAX_RSP_SIZE];
char charger_screen_enabled[MAX_RSP_SIZE];
char sn_buf[13];
char sortingerr_addr[60];
char display_panel_buf[MAX_PANEL_BUF_SIZE];
char panel_display_mode[MAX_RSP_SIZE];
char soc_version_str[MAX_RSP_SIZE];
char block_size_string[MAX_RSP_SIZE];
#if PRODUCT_IOT

/* For IOT we are using custom version */
#define PRODUCT_IOT_VERSION "IOT001"
char bootloader_version_string[MAX_RSP_SIZE];
#endif
char secureboot_buf[13];

#if CHECK_BAT_VOLTAGE
char battery_voltage[MAX_RSP_SIZE];
char battery_soc_ok [MAX_RSP_SIZE];
#endif

char get_variant[MAX_RSP_SIZE];

struct crc_list_value {
	char partition_name[MAX_PTENTRY_NAME];
	uint32_t value;
};

static struct crc_list_value crc_list[MAX_CRC_LIST_LEN];

static bool crc_loaded;

struct sparsecrc_list_value {
	char partition_name[MAX_PTENTRY_NAME];
	uint32_t crcvalue;
	uint32_t sparsecount;
	uint32_t currentsparsecount;
	uint32_t currentcrcvalue;
};
static struct sparsecrc_list_value sparsecrc_list[MAX_SPARSECRC_LIST_LEN];
static bool sparsecrc_loaded;

// Xiaomi added begin
#if PROJECT_SAKURA && (!SAKURA_INDIA)
static int anti_rb_version = 0;
static int tgt_anti_rb_version = 0;
static int update_tgt_anti_rb_version(unsigned char data)
{
	if (data >= '0' && data <= '9') {
		tgt_anti_rb_version = data - '0';
	} else if (data >= 'a' && data <= 'f') {
		tgt_anti_rb_version = data - 'a' + 10;
	} else if (data >= 'A' && data <= 'F') {
		tgt_anti_rb_version = data - 'A' + 10;
	} else {
		return -1;
	}
	return 0;
}
#endif
// Xiaomi added end

static void display_ddr_sorting_result(uint64_t ddr_result, int font)
{
	char buff[50] ="";
	char *str = buff;
	dprintf(CRITICAL, "ddr_sorting_result=%llx\n",ddr_result);
	sprintf(str,"ErrorAdd:0x%llx",ddr_result);
	fbcon_clear();
	while(*str != 0) {
		fbcon_putc_factor(*str++, FBCON_COMMON_MSG, font);
	}
	fbcon_draw_line(FBCON_COMMON_MSG);
}

int parse_sparsecrc_str(char *str, char *str1, char *str2, char *str3)
{
	char *p1;
	char *p2;
	char *p3;
	char *p4;
	int str1_length;
	int str2_length;
	int str3_length;
	int sub_length;

	p1 = strchr(str, ' ');
	p2 = p1;
	while (*p2 == ' ' || *p2 == '\t' || *p2 == '\r') p2 += 1;
	p3 = strchr(p2, ' ');
	p4 = p3;
	while (*p4 == ' ' || *p4 == '\t' || *p4 == '\r') p4 += 1;
	str1_length = p1-str;
	str2_length = p3-p2;
	sub_length = p4-str;
	str3_length = strlen(str) - sub_length;
	if (str1_length > (MAX_PTENTRY_NAME-1) ||
		str2_length > (MAX_PTENTRY_NAME-1) || str3_length > (MAX_PTENTRY_NAME-1))
		return 1;
	strncpy(str1, str, str1_length);
	str1[str1_length] = '\0';
	strncpy(str2, p2, str2_length);
	str2[str2_length] = '\0';
	strncpy(str3, p4, str3_length);
	str3[str3_length] = '\0';
	return 0;
}

int update_sparsecrc_list(char *data)
{
	char *sub_str;
	char str1[MAX_PTENTRY_NAME];
	char str2[MAX_PTENTRY_NAME];
	char str3[MAX_PTENTRY_NAME];
	int  crc_num = 0;
	int err = 0;

	sub_str = strtok((char *)data, "\n");
	if (memcmp(sub_str, SPARSECRC_LIST_HEADER, strlen(SPARSECRC_LIST_HEADER)) != 0) {
		for (int i = 0; i < MAX_SPARSECRC_LIST_LEN; i++)
			sparsecrc_list[i].crcvalue = 0;
		sparsecrc_loaded = 0;
		return 1;
	}
	sub_str = strtok(NULL, "\n");
	while (sub_str) {
		if (crc_num > (MAX_SPARSECRC_LIST_LEN-1)) {
			sparsecrc_loaded = 0;
			return 1;
		}
		if (parse_sparsecrc_str(sub_str, str1, str2, str3) != 0) {
			sparsecrc_loaded = 0;
			return 1;
		}
		strcpy(sparsecrc_list[crc_num].partition_name, str1);
		sparsecrc_list[crc_num].crcvalue = _strtoul(str2, NULL, 16, &err);
		if (err == 1) {
			sparsecrc_loaded = 0;
			return 1;
		}
		sparsecrc_list[crc_num].sparsecount = _strtoul(str3, NULL, 10, &err);
		if (err == 1) {
			sparsecrc_loaded = 0;
			return 1;
		}
		sparsecrc_list[crc_num].currentsparsecount = 0;
		sparsecrc_list[crc_num].currentcrcvalue = 0;
		sub_str = strtok(NULL, "\n");
		crc_num++;
	}
	sparsecrc_loaded = 1;
	return 0;
}

int get_sparsecrc_index(const char *partition_name)
{
	int index = INVALID_PTN;

	for (int i = 0; i < MAX_SPARSECRC_LIST_LEN; i++) {
		if (strcmp(partition_name, sparsecrc_list[i].partition_name) == 0)
			return i;
	}

	return index;
}

int check_sparsecrc(int sparsecrc_index, uint32_t sparsecrc)
{
	int result = 0;

	sparsecrc_list[sparsecrc_index].currentsparsecount ++;
	sparsecrc_list[sparsecrc_index].currentcrcvalue = sparsecrc;
	if (sparsecrc_list[sparsecrc_index].sparsecount == sparsecrc_list[sparsecrc_index].currentsparsecount) {
		dprintf(CRITICAL, "SPARSECRC: 0x%x (crc.list) vs 0x%x\n", sparsecrc_list[sparsecrc_index].crcvalue, sparsecrc_list[sparsecrc_index].currentcrcvalue);
		if (sparsecrc_list[sparsecrc_index].crcvalue != sparsecrc_list[sparsecrc_index].currentcrcvalue) {
			result = 1;
		}
		sparsecrc_list[sparsecrc_index].currentsparsecount = 0;
		sparsecrc_list[sparsecrc_index].currentcrcvalue = 0;
	}

	return result;
}

uint32_t get_init_sparsecrcvalue(int sparsecrc_index)
{
	return sparsecrc_list[sparsecrc_index].currentcrcvalue;
}

int parse_crc_str(char *str, char *str1, char *str2)
{
	char *p1 = strchr(str, ' ');
	char *p2 = strrchr(str, ' ');
	int str1_length;
	int str2_length;
	int sub_length;

	str1_length = p1-str;
	sub_length = p2-str+1;
	str2_length = strlen(str) - sub_length;
	if (str1_length > (MAX_PTENTRY_NAME-1) ||
		str2_length > (MAX_PTENTRY_NAME-1))
		return 1;
	strncpy(str1, str, str1_length);
	str1[str1_length] = '\0';
	strncpy(str2, p2+1, str2_length);
	str2[str2_length] = '\0';
	return 0;
}

int update_crc_list(char *data)
{
	char *sub_str;
	char str1[MAX_PTENTRY_NAME];
	char str2[MAX_PTENTRY_NAME];
	int  crc_num = 0;
	int err = 0;

	sub_str = strtok((char *)data, "\n");
	if (memcmp(sub_str, CRC_LIST_HEADER, strlen(CRC_LIST_HEADER)) != 0) {
		for (int i = 0; i < MAX_CRC_LIST_LEN; i++)
			crc_list[i].value = 0;
		crc_loaded = 0;
		return 1;
	}
	sub_str = strtok(NULL, "\n");
	while (sub_str) {
		if (crc_num > (MAX_CRC_LIST_LEN-1)) {
			crc_loaded = 0;
			return 1;
		}
		if (parse_crc_str(sub_str, str1, str2) != 0) {
			crc_loaded = 0;
			return 1;
		}
		strcpy(crc_list[crc_num].partition_name, str1);
		crc_list[crc_num].value = _strtoul(str2, NULL, 16, &err);
		if (err == 1) {
			crc_loaded = 0;
			return 1;
		}
		sub_str = strtok(NULL, "\n");
		crc_num++;
	}
	crc_loaded = 1;
	return 0;
}

static uint32_t crc32_little_endian_table[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t simple_crc32(uint32_t CrcInitVal, const char *buf, uint32_t size)
{
    uint32_t crc = CrcInitVal ^ ~0U;

    while (size--)
        crc = crc32_little_endian_table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
    return crc ^ ~0U;
}

int check_crc(const char *arg, char *data, unsigned sz)
{
	int length;
	uint32_t value;

	for (int i = 0; i < MAX_CRC_LIST_LEN; i++) {
		if (strcmp(arg, crc_list[i].partition_name) != 0)
			continue;
		length = sz;
		value = simple_crc32(0, data, length);
		dprintf(CRITICAL, "CRC: 0x%x (crc.list) vs 0x%x\n", crc_list[i].value, value);
		if (value == crc_list[i].value)
			return 0;
		else
			return 1;
	}
	return 0;
}

extern int emmc_recovery_init(void);

#if NO_KEYPAD_DRIVER
extern int fastboot_trigger(void);
#endif

static void update_ker_tags_rdisk_addr(boot_img_hdr *hdr, bool is_arm64)
{
	/* overwrite the destination of specified for the project */
#ifdef ABOOT_IGNORE_BOOT_HEADER_ADDRS
	if (is_arm64)
		hdr->kernel_addr = ABOOT_FORCE_KERNEL64_ADDR;
	else
		hdr->kernel_addr = ABOOT_FORCE_KERNEL_ADDR;
	hdr->ramdisk_addr = ABOOT_FORCE_RAMDISK_ADDR;
	hdr->tags_addr = ABOOT_FORCE_TAGS_ADDR;
#endif
}

static void ptentry_to_tag(unsigned **ptr, struct ptentry *ptn)
{
	struct atag_ptbl_entry atag_ptn;

	memcpy(atag_ptn.name, ptn->name, 16);
	atag_ptn.name[15] = '\0';
	atag_ptn.offset = ptn->start;
	atag_ptn.size = ptn->length;
	atag_ptn.flags = ptn->flags;
	memcpy(*ptr, &atag_ptn, sizeof(struct atag_ptbl_entry));
	*ptr += sizeof(struct atag_ptbl_entry) / sizeof(unsigned);
}
#ifdef VERIFIED_BOOT_2
void load_vbmeta_image(void **vbmeta_image_buf, uint32_t *vbmeta_image_sz)
{
	int index = 0;
	char *vbm_img_buf = NULL;
	unsigned long long ptn = 0;
	unsigned long long ptn_size = 0;

	/* Immediately return if dtbo is not supported */
	index = partition_get_index("vbmeta");
	ptn = partition_get_offset(index);
	if(!ptn)
	{
		dprintf(CRITICAL, "ERROR: vbmeta partition not found.\n");
		return;
	}

	ptn_size = partition_get_size(index);
	if (ptn_size > MAX_SUPPORTED_VBMETA_IMG_BUF)
	{
		dprintf(CRITICAL, "ERROR: vbmeta parition size is greater than supported.\n");
		return;
	}

	vbm_img_buf = (char *)memalign(CACHE_LINE, ROUNDUP((uint32_t)ptn_size, CACHE_LINE));
	if (!vbm_img_buf)
	{
		dprintf(CRITICAL, "ERROR: vbmeta unable to locate buffer\n");
		return;
	}

	mmc_set_lun(partition_get_lun(index));
	if (mmc_read(ptn, (uint32_t *)vbm_img_buf, (uint32_t)ptn_size))
	{
		dprintf(CRITICAL, "ERROR: vbmeta read failure\n");
		free(vbm_img_buf);
		return;
	}

	*vbmeta_image_buf = vbm_img_buf;
	*vbmeta_image_sz = (uint32_t)ptn_size;
	return;
}
#endif

#if CHECK_BAT_VOLTAGE
void update_battery_status(void)
{
	snprintf(battery_voltage,MAX_RSP_SIZE, "%d",target_get_battery_voltage());
	snprintf(battery_soc_ok ,MAX_RSP_SIZE, "%s",target_battery_soc_ok()? "yes":"no");
}
#endif

#ifndef FACTORY_VERSION_ENABLE
static int boot_flag = 0;
#endif
unsigned char *update_cmdline(const char * cmdline)
{
	int cmdline_len = 0;
	int have_cmdline = 0;
	unsigned char *cmdline_final = NULL;
	int pause_at_bootup = 0;
	bool warm_boot = false;
	bool gpt_exists = partition_gpt_exists();
	int have_target_boot_params = 0;
	char *boot_dev_buf = NULL;
#ifdef MDTP_SUPPORT
    	bool is_mdtp_activated = 0;
#endif
	int current_active_slot = INVALID;
	int system_ptn_index = -1;
	unsigned int lun = 0;
	char lun_char_base = 'a';
#if VERITY_LE
    int syspath_buflen;
    int dtbo_idx = INVALID_PTN;
    char dtbo_idx_str[MAX_DTBO_IDX_STR] = "\0";
    if (boot_verify_get_state() == ORANGE) {
        syspath_buflen = strlen(sys_path) + sizeof(int) + 2; /*allocate buflen for largest possible string*/
    } else {
        syspath_buflen = strlen(verity_dev)
            + strlen(verity_system_part) + (sizeof(char) * 2) + 2
            + strlen(verity_params) + sizeof(int) + 2;
    }
#else
	int syspath_buflen = strlen(sys_path) + sizeof(int) + 2; /*allocate buflen for largest possible string*/
	char dtbo_idx_str[MAX_DTBO_IDX_STR] = "\0";
	int dtbo_idx = INVALID_PTN;
#endif
	char syspath_buf[syspath_buflen];
#if HIBERNATION_SUPPORT
	int resume_buflen = strlen(resume) + sizeof(int) + 2;
	char resume_buf[resume_buflen];
	int swap_ptn_index = INVALID_PTN;
#endif

#if VERIFIED_BOOT
	uint32_t boot_state = RED;
#endif

#if USE_LE_SYSTEMD
	is_systemd_present=true;
#endif

#if VERIFIED_BOOT
	if (VB_M <= target_get_vb_version())
	{
    		boot_state = boot_verify_get_state();
	}
#endif

#ifdef MDTP_SUPPORT
    mdtp_activated(&is_mdtp_activated);
#endif /* MDTP_SUPPORT */

	if (cmdline && cmdline[0]) {
		cmdline_len = strlen(cmdline);
		have_cmdline = 1;
	}
	if (target_is_emmc_boot()) {
		cmdline_len += strlen(emmc_cmdline);
#if USE_BOOTDEV_CMDLINE
		boot_dev_buf = (char *) malloc(sizeof(char) * BOOT_DEV_MAX_LEN);
		ASSERT(boot_dev_buf);
		platform_boot_dev_cmdline(boot_dev_buf);
		cmdline_len += strlen(boot_dev_buf);
#endif
	}

	get_bootloader_version(bootloader_string);
	cmdline_len += strlen(ddr_sorting_addr_cmdline);
	cmdline_len += strlen(sortingerr_addr);
	cmdline_len += strlen(usb_sn_cmdline);
	cmdline_len += strlen(sn_buf);

	cmdline_len += strlen(secureboot_cmdline);
	cmdline_len += strlen(secureboot_buf);

#if VERIFIED_BOOT
	if (VB_M <= target_get_vb_version())
	{
		cmdline_len += strlen(verified_state) + strlen(vbsn[boot_state].name);
		if ((device.verity_mode != 0 ) && (device.verity_mode != 1))
		{
			dprintf(CRITICAL, "Devinfo paritition possibly corrupted!!!. Please erase devinfo partition to continue booting\n");
			ASSERT(0);
		}
		cmdline_len += strlen(verity_mode) + strlen(vbvm[device.verity_mode].name);
		cmdline_len += strlen(keymaster_v1);
	}
#endif


	if (vbcmdline != NULL) {
		dprintf(DEBUG, "UpdateCmdLine vbcmdline present len %d\n",
						strlen(vbcmdline));
		cmdline_len += strlen(vbcmdline);
	}
	cmdline_len += strlen(bootloader_cmdline);
	cmdline_len += strlen((char *)bootloader_string);

	if (boot_into_recovery && gpt_exists)
		cmdline_len += strlen(secondary_gpt_enable);

#ifdef MDTP_SUPPORT
	if(is_mdtp_activated)
		cmdline_len += strlen(mdtp_activated_flag);
#endif
	if (boot_into_ffbm) {
		cmdline_len += strlen(androidboot_mode);

		if(is_systemd_present)
			cmdline_len += strlen(systemd_ffbm_mode);

		cmdline_len += strlen(ffbm_mode_string);
		/* reduce kernel console messages to speed-up boot */
		cmdline_len += strlen(loglevel);
	} else if (boot_reason_alarm) {
		cmdline_len += strlen(alarmboot_cmdline);
#ifdef FACTORY_VERSION_ENABLE
	} else if ((target_build_variant_user() || device.charger_screen_enabled)
			&& target_pause_for_battery_charge()) {
#else
	} else if (boot_flag) {
#endif
		if(!boot_into_recovery){
			pause_at_bootup = 1;
			cmdline_len += strlen(battchg_pause);
		}
	}

	if(target_use_signed_kernel() && auth_kernel_img) {
		cmdline_len += strlen(auth_kernel);
	}

	if (get_target_boot_params(cmdline, boot_into_recovery ? "recoveryfs" :
								 "system",
						&target_boot_params) == 0) {
		have_target_boot_params = 1;
		cmdline_len += strlen(target_boot_params);
	}

	/* Determine correct androidboot.baseband to use */
	switch(target_baseband())
	{
		case BASEBAND_APQ:
			cmdline_len += strlen(baseband_apq);
			break;

		case BASEBAND_MSM:
			cmdline_len += strlen(baseband_msm);
			break;

		case BASEBAND_CSFB:
			cmdline_len += strlen(baseband_csfb);
			break;

		case BASEBAND_SVLTE2A:
			cmdline_len += strlen(baseband_svlte2a);
			break;

		case BASEBAND_MDM:
			cmdline_len += strlen(baseband_mdm);
			break;

		case BASEBAND_MDM2:
			cmdline_len += strlen(baseband_mdm2);
			break;

		case BASEBAND_SGLTE:
			cmdline_len += strlen(baseband_sglte);
			break;

		case BASEBAND_SGLTE2:
			cmdline_len += strlen(baseband_sglte2);
			break;

		case BASEBAND_DSDA:
			cmdline_len += strlen(baseband_dsda);
			break;

		case BASEBAND_DSDA2:
			cmdline_len += strlen(baseband_dsda2);
			break;
		case BASEBAND_APQ_NOWGR:
			cmdline_len += strlen(baseband_apq_nowgr);
			break;
	}

#if ENABLE_DISPLAY
	if (cmdline) {
		if ((strstr(cmdline, DISPLAY_DEFAULT_PREFIX) == NULL) &&
			target_display_panel_node(display_panel_buf,
			MAX_PANEL_BUF_SIZE) &&
			strlen(display_panel_buf)) {
			cmdline_len += strlen(display_panel_buf);
		}
	}
#endif

	if (target_warm_boot()) {
		warm_boot = true;
		cmdline_len += strlen(warmboot_cmdline);
	}

	if (target_uses_system_as_root() ||
		partition_multislot_is_supported())
	{
		current_active_slot = partition_find_active_slot();
		cmdline_len += (strlen(androidboot_slot_suffix)+
					strlen(SUFFIX_SLOT(current_active_slot)));

		system_ptn_index = partition_get_index("system");
		if (platform_boot_dev_isemmc())
		{
#if VERITY_LE
			/*
			  Condition 4: Verity and A/B both enabled
			  Eventual command line looks like:
			  ... androidboot.slot_suffix=<slot_suffix>  ... rootfstype=ext4 ...
			  ... root=/dev/dm-0  dm="system_<slot_suffix>  none ro,0 1 android-verity /dev/mmcblk0p<NN>"
			*/
            if (boot_verify_get_state() == ORANGE) {
                snprintf(syspath_buf, syspath_buflen, " %s%d",
					sys_path, system_ptn_index + 1);
            } else {
                snprintf(syspath_buf, syspath_buflen, " %s %s%s %s%d\"",
                        verity_dev,
                        verity_system_part, suffix_slot[current_active_slot],
                        verity_params, system_ptn_index + 1);
            }
#else
			/*
			  Condition 5: A/B enabled, but verity disabled
			  Eventual command line looks like:
			  ... androidboot.slot_suffix=<slot_suffix>  ... rootfstype=ext4 ...
			  ... root=/dev/mmcblk0p<NN> ...
			*/
			snprintf(syspath_buf, syspath_buflen, " %s%d",
					sys_path, system_ptn_index + 1);
#endif
		}
		else
		{
			lun = partition_get_lun(system_ptn_index);
			snprintf(syspath_buf, syspath_buflen, " root=/dev/sd%c%d",
					lun_char_base + lun,
					partition_get_index_in_lun("system", lun));
		}

#ifndef VERIFIED_BOOT_2
		cmdline_len += strlen(syspath_buf);
#endif
	}

	if (target_uses_system_as_root() ||
		partition_multislot_is_supported())
	{
		cmdline_len += strlen(sys_path_cmdline);
		if (!boot_into_recovery)
			cmdline_len += strlen(skip_ramfs);
	}

#if HIBERNATION_SUPPORT
	if (platform_boot_dev_isemmc())
	{
		swap_ptn_index = partition_get_index("swap");
		if (swap_ptn_index != INVALID_PTN)
		{
			snprintf(resume_buf, resume_buflen,
				" %s%d", resume,
				(swap_ptn_index + 1));
			cmdline_len += strlen(resume_buf);
		}
		else
		{
			dprintf(INFO, "WARNING: swap partition not found\n");
		}
	}
#endif

#if TARGET_CMDLINE_SUPPORT
	char *target_cmdline_buf = malloc(TARGET_MAX_CMDLNBUF);
	int target_cmd_line_len;
	ASSERT(target_cmdline_buf);
	target_cmd_line_len = target_update_cmdline(target_cmdline_buf);
	cmdline_len += target_cmd_line_len;
#endif

#if !VERITY_LE
	dtbo_idx = get_dtbo_idx ();
	if (dtbo_idx != INVALID_PTN) {
		snprintf(dtbo_idx_str, sizeof(dtbo_idx_str), "%s%d",
			android_boot_dtbo_idx, dtbo_idx);
		cmdline_len += strlen (dtbo_idx_str);
	}
#else
    if (boot_verify_get_state() == ORANGE) {
        dtbo_idx = get_dtbo_idx ();
        if (dtbo_idx != INVALID_PTN) {
            snprintf(dtbo_idx_str, sizeof(dtbo_idx_str), "%s%d",
                    android_boot_dtbo_idx, dtbo_idx);
            cmdline_len += strlen (dtbo_idx_str);
        }
    }
#endif

	if (cmdline_len > 0) {
		const char *src;
		unsigned char *dst;

		cmdline_final = (unsigned char*) malloc((cmdline_len + 4) & (~3));
		ASSERT(cmdline_final != NULL);
		memset((void *)cmdline_final, 0, sizeof(*cmdline_final));
		dst = cmdline_final;

		/* Save start ptr for debug print */
		if (have_cmdline) {
			src = cmdline;
			while ((*dst++ = *src++));
		}
		if (target_is_emmc_boot()) {
			src = emmc_cmdline;
			if (have_cmdline) --dst;
			have_cmdline = 1;
			while ((*dst++ = *src++));
#if USE_BOOTDEV_CMDLINE
			src = boot_dev_buf;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
#endif
		}

#if VERIFIED_BOOT
		if (VB_M <= target_get_vb_version())
		{
			src = verified_state;
			if(have_cmdline) --dst;
			have_cmdline = 1;
			while ((*dst++ = *src++));
			src = vbsn[boot_state].name;
			if(have_cmdline) --dst;
			while ((*dst++ = *src++));

			if ((device.verity_mode != 0 ) && (device.verity_mode != 1))
			{
				dprintf(CRITICAL, "Devinfo paritition possibly corrupted!!!. Please erase devinfo partition to continue booting\n");
				ASSERT(0);
			}
			src = verity_mode;
			if(have_cmdline) --dst;
			while ((*dst++ = *src++));
			src = vbvm[device.verity_mode].name;
			if(have_cmdline) -- dst;
			while ((*dst++ = *src++));
			src = keymaster_v1;
			if(have_cmdline) --dst;
			while ((*dst++ = *src++));
		}
#endif

		if (vbcmdline != NULL) {
			src = vbcmdline;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
		}

		src = ddr_sorting_addr_cmdline;
		if (have_cmdline) --dst;
		have_cmdline = 1;
		while ((*dst++ = *src++));
		src = sortingerr_addr;
		if (have_cmdline) --dst;
		have_cmdline = 1;
		while ((*dst++ = *src++));
		src = usb_sn_cmdline;
		if (have_cmdline) --dst;
		have_cmdline = 1;
		while ((*dst++ = *src++));
		src = sn_buf;
		if (have_cmdline) --dst;
		have_cmdline = 1;
		while ((*dst++ = *src++));

		src = secureboot_cmdline;
		if (have_cmdline) --dst;
		have_cmdline = 1;
		while ((*dst++ = *src++));
		src = secureboot_buf;
		if (have_cmdline) --dst;
		have_cmdline = 1;
		while ((*dst++ = *src++));

		if (warm_boot) {
			if (have_cmdline) --dst;
			src = warmboot_cmdline;
			while ((*dst++ = *src++));
		}
		src = bootloader_cmdline;
		if (have_cmdline) --dst;
			have_cmdline = 1;
		while ((*dst++ = *src++));
			src = (char *)bootloader_string;
		if (have_cmdline) --dst;
			have_cmdline = 1;
		while ((*dst++ = *src++));

		if (boot_into_recovery && gpt_exists) {
			src = secondary_gpt_enable;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
		}
#ifdef MDTP_SUPPORT
		if (is_mdtp_activated) {
			src = mdtp_activated_flag;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
		}
#endif
		if (boot_into_ffbm) {
			src = androidboot_mode;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
			src = ffbm_mode_string;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));

			if(is_systemd_present) {
				src = systemd_ffbm_mode;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
			}

			src = loglevel;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
		} else if (boot_reason_alarm) {
			src = alarmboot_cmdline;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
		} else if (pause_at_bootup) {
			src = battchg_pause;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
		}

		if(target_use_signed_kernel() && auth_kernel_img) {
			src = auth_kernel;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
		}

		switch(target_baseband())
		{
			case BASEBAND_APQ:
				src = baseband_apq;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_MSM:
				src = baseband_msm;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_CSFB:
				src = baseband_csfb;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_SVLTE2A:
				src = baseband_svlte2a;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_MDM:
				src = baseband_mdm;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_MDM2:
				src = baseband_mdm2;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_SGLTE:
				src = baseband_sglte;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_SGLTE2:
				src = baseband_sglte2;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_DSDA:
				src = baseband_dsda;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_DSDA2:
				src = baseband_dsda2;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;
			case BASEBAND_APQ_NOWGR:
				src = baseband_apq_nowgr;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;
		}

		if (strlen(display_panel_buf)) {
			src = display_panel_buf;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
		}

		if (have_target_boot_params) {
			if (have_cmdline) --dst;
			src = target_boot_params;
			while ((*dst++ = *src++));
			free(target_boot_params);
		}

		if (partition_multislot_is_supported() && have_cmdline)
		{
				src = androidboot_slot_suffix;
				--dst;
				while ((*dst++ = *src++));
				--dst;
				src = SUFFIX_SLOT(current_active_slot);
				while ((*dst++ = *src++));
		}


		/*
		 * System-As-Root behaviour, system.img should contain both
		 * system content and ramdisk content, and should be mounted at
		 * root(a/b).
		 * Apending skip_ramfs for non a/b builds which use, system as root.
		 */
		if ((target_uses_system_as_root() ||
			partition_multislot_is_supported()) &&
			have_cmdline)
		{
			if (!boot_into_recovery)
			{
				src = skip_ramfs;
				--dst;
				while ((*dst++ = *src++));
			}

			src = sys_path_cmdline;
			--dst;
			while ((*dst++ = *src++));

#ifndef VERIFIED_BOOT_2
			src = syspath_buf;
			--dst;
			while ((*dst++ = *src++));
#endif
		}

#if HIBERNATION_SUPPORT
		if (swap_ptn_index != INVALID_PTN)
		{
			src = resume_buf;
			--dst;
			while ((*dst++ = *src++));
		}
#endif

#if TARGET_CMDLINE_SUPPORT
		if (target_cmdline_buf && target_cmd_line_len)
		{
			if (have_cmdline) --dst;
			src = target_cmdline_buf;
			while((*dst++ = *src++));
			free(target_cmdline_buf);
		}
#endif

#if !VERITY_LE
		if (dtbo_idx != INVALID_PTN) {
			src = dtbo_idx_str;
			--dst;
			while ((*dst++ = *src++));
		}
#else
        if (boot_verify_get_state() == ORANGE) {
            if (dtbo_idx != INVALID_PTN) {
                src = dtbo_idx_str;
                --dst;
                while ((*dst++ = *src++));
            }
        }
#endif
	}


	if (boot_dev_buf)
		free(boot_dev_buf);

	if (cmdline_final)
		dprintf(INFO, "cmdline: %s\n", cmdline_final);
	else
		dprintf(INFO, "cmdline is NULL\n");
	return cmdline_final;
}

unsigned *atag_core(unsigned *ptr)
{
	/* CORE */
	*ptr++ = 2;
	*ptr++ = 0x54410001;

	return ptr;

}

unsigned *atag_ramdisk(unsigned *ptr, void *ramdisk,
							   unsigned ramdisk_size)
{
	if (ramdisk_size) {
		*ptr++ = 4;
		*ptr++ = 0x54420005;
		*ptr++ = (unsigned)ramdisk;
		*ptr++ = ramdisk_size;
	}

	return ptr;
}

unsigned *atag_ptable(unsigned **ptr_addr)
{
	int i;
	struct ptable *ptable;

	if ((ptable = flash_get_ptable()) && (ptable->count != 0)) {
		*(*ptr_addr)++ = 2 + (ptable->count * (sizeof(struct atag_ptbl_entry) /
							sizeof(unsigned)));
		*(*ptr_addr)++ = 0x4d534d70;
		for (i = 0; i < ptable->count; ++i)
			ptentry_to_tag(ptr_addr, ptable_get(ptable, i));
	}

	return (*ptr_addr);
}

unsigned *atag_cmdline(unsigned *ptr, const char *cmdline)
{
	int cmdline_length = 0;
	int n;
	char *dest;

	cmdline_length = strlen((const char*)cmdline);
	n = (cmdline_length + 4) & (~3);

	*ptr++ = (n / 4) + 2;
	*ptr++ = 0x54410009;
	dest = (char *) ptr;
	while ((*dest++ = *cmdline++));
	ptr += (n / 4);

	return ptr;
}

unsigned *atag_end(unsigned *ptr)
{
	/* END */
	*ptr++ = 0;
	*ptr++ = 0;

	return ptr;
}

void generate_atags(unsigned *ptr, const char *cmdline,
                    void *ramdisk, unsigned ramdisk_size)
{
	unsigned *orig_ptr = ptr;
	ptr = atag_core(ptr);
	ptr = atag_ramdisk(ptr, ramdisk, ramdisk_size);
	ptr = target_atag_mem(ptr);

	/* Skip NAND partition ATAGS for eMMC boot */
	if (!target_is_emmc_boot()){
		ptr = atag_ptable(&ptr);
	}

	/*
	 * Atags size filled till + cmdline size + 1 unsigned for 4-byte boundary + 4 unsigned
	 * for atag identifier in atag_cmdline and atag_end should be with in MAX_TAGS_SIZE bytes
	 */
	if (!cmdline)
		return;

	if (((ptr - orig_ptr) + strlen(cmdline) + 5 * sizeof(unsigned)) <  MAX_TAGS_SIZE) {
		ptr = atag_cmdline(ptr, cmdline);
		ptr = atag_end(ptr);
	}
	else {
		dprintf(CRITICAL,"Crossing ATAGs Max size allowed\n");
		ASSERT(0);
	}
}

typedef void entry_func_ptr(unsigned, unsigned, unsigned*);
void boot_linux(void *kernel, unsigned *tags,
		const char *cmdline, unsigned machtype,
		void *ramdisk, unsigned ramdisk_size)
{
	unsigned char *final_cmdline;
#if DEVICE_TREE
	int ret = 0;
#endif

	void (*entry)(unsigned, unsigned, unsigned*) = (entry_func_ptr*)(PA((addr_t)kernel));
	uint32_t tags_phys = PA((addr_t)tags);
	struct kernel64_hdr *kptr = ((struct kernel64_hdr*)(PA((addr_t)kernel)));

	ramdisk = (void *)PA((addr_t)ramdisk);

	final_cmdline = update_cmdline((const char*)cmdline);

#if DEVICE_TREE
	dprintf(INFO, "Updating device tree: start\n");

	/* Update the Device Tree */
	ret = update_device_tree((void *)tags,(const char *)final_cmdline, ramdisk, ramdisk_size, pu_reason);
	if(ret)
	{
		dprintf(CRITICAL, "ERROR: Updating Device Tree Failed \n");
		ASSERT(0);
	}
	dprintf(INFO, "Updating device tree: done\n");
#else
	/* Generating the Atags */
	generate_atags(tags, final_cmdline, ramdisk, ramdisk_size);
#endif

#if VERIFIED_BOOT
	if (VB_M == target_get_vb_version())
	{
		if (device.verity_mode == 0) {
#if FBCON_DISPLAY_MSG
#if ENABLE_VB_ATTEST
			display_bootverify_menu(DISPLAY_MENU_EIO);
			wait_for_users_action();
			if(!pwr_key_is_pressed)
				shutdown_device();
#else
			display_bootverify_menu(DISPLAY_MENU_LOGGING);
#endif
			wait_for_users_action();
#else
			dprintf(CRITICAL,
				"The dm-verity is not started in enforcing mode.\nWait for 5 seconds before proceeding\n");
			mdelay(5000);
#endif
		}
	}
#endif

#if 0//VERIFIED_BOOT
	/* Write protect the device info */
	if (!boot_into_recovery && target_build_variant_user() && devinfo_present && mmc_write_protect("devinfo", 1))
	{
		dprintf(INFO, "Failed to write protect dev info\n");
		//ASSERT(0);
	}
#endif

	/* Turn off splash screen if enabled */
#if DISPLAY_SPLASH_SCREEN
	target_display_shutdown();
#endif

	/* Perform target specific cleanup */
	target_uninit();
	free_verified_boot_resource(&info);
	if (final_cmdline)
		free(final_cmdline);

	dprintf(INFO, "booting linux @ %p, ramdisk @ %p (%d), tags/device tree @ %p\n",
		entry, ramdisk, ramdisk_size, (void *)tags_phys);

	enter_critical_section();

	/* do any platform specific cleanup before kernel entry */
	platform_uninit();

	arch_disable_cache(UCACHE);

#if ARM_WITH_MMU
	arch_disable_mmu();
#endif
	bs_set_timestamp(BS_KERNEL_ENTRY);

	if (IS_ARM64(kptr))
		/* Jump to a 64bit kernel */
		scm_elexec_call((paddr_t)kernel, tags_phys);
	else
		/* Jump to a 32bit kernel */
		entry(0, machtype, (unsigned*)tags_phys);
}

/* Function to check if the memory address range falls within the aboot
 * boundaries.
 * start: Start of the memory region
 * size: Size of the memory region
 */
int check_aboot_addr_range_overlap(uintptr_t start, uint32_t size)
{
	/* Check for boundary conditions. */
	if ((UINT_MAX - start) < size)
		return -1;

	/* Check for memory overlap. */
	if ((start < MEMBASE) && ((start + size) <= MEMBASE))
		return 0;
	else if (start >= (MEMBASE + MEMSIZE))
		return 0;
	else
		return -1;
}

/* Function to check if the memory address range falls beyond ddr region.
 * start: Start of the memory region
 * size: Size of the memory region
 */
int check_ddr_addr_range_bound(uintptr_t start, uint32_t size)
{
	uintptr_t ddr_pa_start_addr = PA(get_ddr_start());
	uint64_t ddr_size = smem_get_ddr_size();
	uint64_t ddr_pa_end_addr = ddr_pa_start_addr + ddr_size;
	uintptr_t pa_start_addr = PA(start);

	/* Check for boundary conditions. */
	if ((UINT_MAX - start) < size)
		return -1;

	/* Check if memory range is beyond the ddr range. */
	if (pa_start_addr < ddr_pa_start_addr ||
		pa_start_addr >= (ddr_pa_end_addr) ||
		(pa_start_addr + size) > ddr_pa_end_addr)
		return -1;
	else
		return 0;
}

BUF_DMA_ALIGN(buf, BOOT_IMG_MAX_PAGE_SIZE); //Equal to max-supported pagesize

int getimage(void **image_buffer, uint32_t *imgsize,
                    const char *imgname)
{
	uint32_t loadedindex;
	if (image_buffer == NULL || imgsize == NULL ||
	    imgname == NULL) {
		dprintf(CRITICAL, "getimage: invalid parameters\n");
		return -1;
	}
	for (loadedindex = 0; loadedindex < info.num_loaded_images; loadedindex++) {
		if (!strncmp(info.images[loadedindex].name, imgname,
		                  strlen(imgname))) {
			*image_buffer = info.images[loadedindex].image_buffer;
			*imgsize = info.images[loadedindex].imgsize;
			dprintf(SPEW, "getimage(): Loaded image [%s|%d]\n",
						info.images[loadedindex].name,
						info.images[loadedindex].imgsize);
			return 0;
		}
	}
	return -1;
}

static int verify_signed_bootimg(uint32_t bootimg_addr, uint32_t bootimg_size)
{
	int ret;

#if !VERIFIED_BOOT
#if IMAGE_VERIF_ALGO_SHA1
	uint32_t auth_algo = CRYPTO_AUTH_ALG_SHA1;
#else
	uint32_t auth_algo = CRYPTO_AUTH_ALG_SHA256;
#endif
#endif

	/* Assume device is rooted at this time. */
	device.is_tampered = 1;

	dprintf(INFO, "Authenticating boot image (%d): start\n", bootimg_size);

#if VERIFIED_BOOT
	uint32_t bootstate;
	if(boot_into_recovery &&
		(!partition_multislot_is_supported()))
	{
		ret = boot_verify_image((unsigned char *)bootimg_addr,
				bootimg_size, "/recovery", &bootstate);
	}
	else
	{
		ret = boot_verify_image((unsigned char *)bootimg_addr,
				bootimg_size, "/boot", &bootstate);
	}
	boot_verify_print_state();
#else
	ret = image_verify((unsigned char *)bootimg_addr,
					   (unsigned char *)(bootimg_addr + bootimg_size),
					   bootimg_size,
					   auth_algo);
#endif
	dprintf(INFO, "Authenticating boot image: done return value = %d\n", ret);

	if (ret)
	{
		/* Authorized kernel */
		device.is_tampered = 0;
		auth_kernel_img = 1;
	}

#ifdef MDTP_SUPPORT
	{
		/* Verify MDTP lock.
		 * For boot & recovery partitions, use aboot's verification result.
		 */
		mdtp_ext_partition_verification_t ext_partition;
		ext_partition.partition = boot_into_recovery ? MDTP_PARTITION_RECOVERY : MDTP_PARTITION_BOOT;
		ext_partition.integrity_state = device.is_tampered ? MDTP_PARTITION_STATE_INVALID : MDTP_PARTITION_STATE_VALID;
		ext_partition.page_size = 0; /* Not needed since already validated */
		ext_partition.image_addr = 0; /* Not needed since already validated */
		ext_partition.image_size = 0; /* Not needed since already validated */
		ext_partition.sig_avail = FALSE; /* Not needed since already validated */
		mdtp_fwlock_verify_lock(&ext_partition);
	}
#endif /* MDTP_SUPPORT */

#if USE_PCOM_SECBOOT
	set_tamper_flag(device.is_tampered);
#endif

#if VERIFIED_BOOT
	switch(boot_verify_get_state())
	{
		case RED:
#if FBCON_DISPLAY_MSG
			display_bootverify_menu(DISPLAY_MENU_RED);
#if ENABLE_VB_ATTEST
			mdelay(DELAY_WAIT);
			shutdown_device();
#else
			wait_for_users_action();
#endif
#else
			dprintf(CRITICAL,
					"Your device has failed verification and may not work properly.\nWait for 5 seconds before proceeding\n");
			mdelay(5000);
#endif

			break;
		case YELLOW:
#if FBCON_DISPLAY_MSG
			display_bootverify_menu(DISPLAY_MENU_YELLOW);
			wait_for_users_action();
#else
			dprintf(CRITICAL,
					"Your device has loaded a different operating system.\nWait for 5 seconds before proceeding\n");
			mdelay(5000);
#endif
			break;
		default:
			break;
	}
#endif
#if !VERIFIED_BOOT
	if(device.is_tampered)
	{
		write_device_info_mmc(&device);
	#ifdef TZ_TAMPER_FUSE
		set_tamper_fuse_cmd(HLOS_IMG_TAMPER_FUSE);
	#endif
	#ifdef ASSERT_ON_TAMPER
		dprintf(CRITICAL, "Device is tampered. Asserting..\n");
		ASSERT(0);
	#endif
	}
#endif
	return ret;
}

static bool check_format_bit()
{
	bool ret = false;
	int index;
	uint64_t offset;
	struct boot_selection_info *in = NULL;
	char *buf = NULL;

	index = partition_get_index("bootselect");
	if (index == INVALID_PTN)
	{
		dprintf(INFO, "Unable to locate /bootselect partition\n");
		return ret;
	}
	offset = partition_get_offset(index);
	if(!offset)
	{
		dprintf(INFO, "partition /bootselect doesn't exist\n");
		return ret;
	}
	buf = (char *) memalign(CACHE_LINE, ROUNDUP(page_size, CACHE_LINE));
	mmc_set_lun(partition_get_lun(index));
	ASSERT(buf);
	if (mmc_read(offset, (uint32_t *)buf, page_size))
	{
		dprintf(INFO, "mmc read failure /bootselect %d\n", page_size);
		free(buf);
		return ret;
	}
	in = (struct boot_selection_info *) buf;
	if ((in->signature == BOOTSELECT_SIGNATURE) &&
			(in->version == BOOTSELECT_VERSION)) {
		if ((in->state_info & BOOTSELECT_FORMAT) &&
				!(in->state_info & BOOTSELECT_FACTORY))
			ret = true;
	} else {
		dprintf(CRITICAL, "Signature: 0x%08x or version: 0x%08x mismatched of /bootselect\n",
				in->signature, in->version);
		ASSERT(0);
	}
	free(buf);
	return ret;
}

void boot_verifier_init()
{
	uint32_t boot_state;
	/* Check if device unlock */
	if(device.is_unlocked)
	{
		boot_verify_send_event(DEV_UNLOCK);
		boot_verify_print_state();
		#ifdef HQ_BUILD_FLAG
		dprintf(CRITICAL, "Device is unlocked! \n");
		#else
		dprintf(CRITICAL, "Device is unlocked! Skipping verification...\n");
		#endif
		//return;
	}
	else
	{
		boot_verify_send_event(BOOT_INIT);
	}

	/* Initialize keystore */
	boot_state = boot_verify_keystore_init();
	if(boot_state == YELLOW)
	{
		boot_verify_print_state();
		dprintf(CRITICAL, "Keystore verification failed! Continuing anyways...\n");
	}
}

#ifdef SAMSUNG_RPMB_RESET
extern void target_sdc_init_for_cid_program(void);
extern uint32_t mmc_write_for_cid_program(uint64_t data_addr, uint32_t data_len, void *in);
//uint32_t cid_ori[4]={0,};


struct fw_update_info {
	unsigned int manfid;
	char prod_name[8];
	unsigned char *update_arry;
	u32 update_arry_size;
};


#define SAMSUNG_CID_MANFID 0x15
#define SAMSUNG_KMGD6001BM_B421_32GB_3GB_CID_PROD_NAME "GD6BMB"
#define SAMSUNG_KMGX6001BM_B514_32GB_3GB_CID_PROD_NAME "GX6BMB" 
#define SAMSUNG_KMRH60014M_B614_64GB_4GB_CID_PROD_NAME "RH64MB"
#define SAMSUNG_KMRH60014A_B614_64GB_4GB_CID_PROD_NAME "RH64AB"

static struct fw_update_info ffu_table[] = {
	{SAMSUNG_CID_MANFID, SAMSUNG_KMGD6001BM_B421_32GB_3GB_CID_PROD_NAME,
			ss_emmc_fw_binary_GD6BMB, sizeof(ss_emmc_fw_binary_GD6BMB)},
	{SAMSUNG_CID_MANFID, SAMSUNG_KMGX6001BM_B514_32GB_3GB_CID_PROD_NAME,
			ss_emmc_fw_binary_GX6BMB, sizeof(ss_emmc_fw_binary_GX6BMB)},
	{SAMSUNG_CID_MANFID, SAMSUNG_KMRH60014M_B614_64GB_4GB_CID_PROD_NAME,
			ss_emmc_fw_binary_RH64MB, sizeof(ss_emmc_fw_binary_RH64MB)},
	{SAMSUNG_CID_MANFID, SAMSUNG_KMRH60014A_B614_64GB_4GB_CID_PROD_NAME,
			ss_emmc_fw_binary_RH64AB, sizeof(ss_emmc_fw_binary_RH64AB)},
};

uint32_t mmc_Full_Block_Erase_FWpatch(struct mmc_device *dev, struct fw_update_info *fw_info);
uint32_t mmc_rpmb_reset(struct mmc_device *dev)
{
	unsigned int i;
	struct mmc_card  *card =  &dev->card;

	dprintf(CRITICAL, "manfid=%u, product name =%s\n", card->cid.mid, card->cid.pnm);

	for (i = 0; i < sizeof(ffu_table) / sizeof(ffu_table[0]); i++) {
		if (card->cid.mid == ffu_table[i].manfid &&
		    !strncmp((const char *)card->cid.pnm, (const char *)ffu_table[i].prod_name, strlen((const char *)ffu_table[i].prod_name))) {
			return mmc_Full_Block_Erase_FWpatch(dev,ffu_table +i);
		}
	}
	return 0;
}

uint32_t mmc_Full_Block_Erase_FWpatch(struct mmc_device *dev, struct fw_update_info *fw_info)
{

	struct mmc_command cmd = {0};
	struct mmc_cid mmc_cid_old = {0};
	uint32_t mmc_ret = 0;
	uint32_t mmc_status;
	uint32_t i;
	//unsigned char Read_binary[258048] = {0};
	//unsigned char Read_binary_temp[512] = {0};
	unsigned long long size = 0;
	void *buf = NULL;
	uint32_t result = 0;
	uint32_t cid_temp[4] = {0,};
	uint32_t *cid_block;
	uint32_t cid_ori[4]={0,};

#if 0
// [Samsung] Clock Change
	if (sdhci_clk_supply(&dev->host, SDHCI_CLK_400KHZ)) {
		dprintf(CRITICAL, "[SAMSUNG] Error: cannot change frequency\n");
		return 1;
	}
#else
	mmc_ret = mmc_host_init(dev);
	if (mmc_ret) {
		dprintf(CRITICAL, "Error Initializing MMC host : %u\n", mmc_ret);
		return mmc_ret;
	} else {
		dprintf(CRITICAL, "MMC host init success\n");
	}

	/* Initialize and identify cards connected to host */
	mmc_ret = mmc_card_init(dev);
	if (mmc_ret) {
		dprintf(CRITICAL, "Failed detecting MMC/SDC @ slot%d\n",
						  dev->config.slot);
		return mmc_ret;
	} else {
		dprintf(CRITICAL, "mmc_card_init success\n");
	}
#endif

// [Samsung] CID backup
	dprintf(CRITICAL, "[SAMSUNG] -Start- CID backup\n");

	memcpy((struct mmc_cid *)&mmc_cid_old,
		   (struct mmc_cid *)&dev->card.cid, sizeof(struct mmc_cid));

	dprintf(CRITICAL, "[SAMSUNG] Decoded CID fields:\n");
	dprintf(CRITICAL, "[SAMSUNG] Manufacturer ID: %x\n", mmc_cid_old.mid);
	dprintf(CRITICAL, "[SAMSUNG] OEM ID: 0x%x\n", mmc_cid_old.oid);
	dprintf(CRITICAL, "[SAMSUNG] Product Name: %s\n", mmc_cid_old.pnm);
	dprintf(CRITICAL, "[SAMSUNG] Product revision: %d.%d\n", (mmc_cid_old.prv >> 4), (mmc_cid_old.prv & 0xF));
	dprintf(CRITICAL, "[SAMSUNG] Product serial number: %x\n", mmc_cid_old.psn);
	dprintf(CRITICAL, "[SAMSUNG] Manufacturing date: %d %d\n", mmc_cid_old.month, mmc_cid_old.year);
	dprintf(CRITICAL, "[SAMSUNG] -End- CID backup\n");

	dprintf(CRITICAL, "[SAMSUNG]CID cid_ori=0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n",
                           cid_ori[0], cid_ori[1], cid_ori[2],cid_ori[3],mmc_cid_old.year & 0xF,mmc_cid_old.month & 0xF,mmc_cid_old.psn & 0xFFFF);

//	cid_ori[3] = (mmc_cid_old.pnm[0] & 0xFF) | ((mmc_cid_old.oid & 0xFF) << 8) | ((mmc_cid_old.mid & 0xFF) << 24);
//	cid_ori[2] = ((mmc_cid_old.pnm[1] & 0xFF) << 24) | ((mmc_cid_old.pnm[2] & 0xFF) << 16) | ((mmc_cid_old.pnm[3] & 0xFF) << 8) | (mmc_cid_old.pnm[4] & 0xFF);
//	cid_ori[1] = ((mmc_cid_old.psn & 0xFFFF0000) >> 16)  | ((mmc_cid_old.prv & 0xFF) << 16) | ((mmc_cid_old.pnm[5] & 0xFF) << 24);
//	cid_ori[0] = ((mmc_cid_old.month & 0xF) << 8) | ((mmc_cid_old.year  & 0xF) << 12) | ((mmc_cid_old.psn & 0xFFFF) << 16);
//	cid_ori[0] =  0xd4f83300; //0xfd030000

#if 0
	// [Samsung] ----- Reset Card
	cmd.cmd_index = CMD0_GO_IDLE_STATE;
	cmd.argument = 0;
	cmd.cmd_type = SDHCI_CMD_TYPE_NORMAL;
	cmd.resp_type = SDHCI_CMD_RESP_NONE;

	/* send command */
	mmc_ret = sdhci_send_command(&dev->host, &cmd);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] Reset Card _ Step.0\n");
		return mmc_ret;
	}
//#else
	mmc_ret = mmc_get_card_status(&dev->host, &dev->card, &mmc_status);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] Before Go Into Factory Patch Mode _ Check eMMC Status\n");
		return mmc_ret;
	}
	if (MMC_CARD_STATUS(mmc_status) != MMC_TRAN_STATE) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] Before Go Into Factory Patch Mode _ eMMC not in tran state %x, try to STOP CMD.\n", mmc_status);
		mmc_ret = mmc_stop_command(dev);
	}
	
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] Before Go Into Factory Patch Mode _ Failed to stop CMD.\n");
		//return mmc_ret; //Let's ignore this error for now.
	}
	
	mmc_ret = mmc_get_card_status(&dev->host, &dev->card, &mmc_status);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] Before Go Into Factory Patch Mode _ Check eMMC Status\n");
		return mmc_ret;
	}
	if (MMC_CARD_STATUS(mmc_status) != MMC_TRAN_STATE) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] Before Go Into Factory Patch Mode _ eMMC not in tran state %x\n", mmc_status);
		return 1;
	}
#endif

	// [Samsung] ----- Go Into Factory Patch Mode
	dprintf(CRITICAL, "[SAMSUNG] -Start- Go Into Factory Patch Mode\n");

	cmd.cmd_index = 60;
	cmd.argument = 0xEFAC60FC;
	cmd.cmd_type = SDHCI_CMD_TYPE_NORMAL;
	cmd.resp_type = SDHCI_CMD_RESP_R1B;
	cmd.cmd_timeout = 0;

	mmc_ret = sdhci_send_command(&dev->host, &cmd);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] Go Into Factory Patch Mode _ Step.1\n");
		return mmc_ret;
	}

	cmd.cmd_index = 60;
	cmd.argument = 0xCBAD1160;
	cmd.cmd_type = SDHCI_CMD_TYPE_NORMAL;
	cmd.resp_type = SDHCI_CMD_RESP_R1B;

	mmc_ret = sdhci_send_command(&dev->host, &cmd);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] Go Into Factory Patch Mode _ Step.2\n");
		return mmc_ret;
	}
	mmc_ret = mmc_get_card_status(&dev->host, &dev->card, &mmc_status);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] Go Into Factory Patch Mode _ Check eMMC Status\n");
		return mmc_ret;
	}
	if (MMC_CARD_STATUS(mmc_status) != MMC_TRAN_STATE) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] Go Into Factory Patch Mode _ eMMC not in tran state %x\n", mmc_status);
		return 1;
	}
	dprintf(CRITICAL, "[SAMSUNG] -End- Go Into Factory Patch Mode\n");

	// [Samsung] ----- Erase CMD Sequence
	dprintf(CRITICAL, "[SAMSUNG] -Start- Erase CMD Sequence\n");

	cmd.cmd_index = CMD35_ERASE_GROUP_START;
	cmd.argument = 0x00000000;
	cmd.cmd_type = SDHCI_CMD_TYPE_NORMAL;
	cmd.resp_type = SDHCI_CMD_RESP_R1;
	mmc_ret = sdhci_send_command(&dev->host, &cmd);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] Erase CMD Sequence _ CMD35\n");
		return mmc_ret;
	}

	cmd.cmd_index = CMD36_ERASE_GROUP_END;
	cmd.argument = 0x00000000;
	cmd.cmd_type = SDHCI_CMD_TYPE_NORMAL;
	cmd.resp_type = SDHCI_CMD_RESP_R1;
	mmc_ret = sdhci_send_command(&dev->host, &cmd);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] Erase CMD Sequence _ CMD36\n");
		return mmc_ret;
	}

	cmd.cmd_index = CMD38_ERASE;
	cmd.argument = 0x00000000;
	cmd.cmd_type = SDHCI_CMD_TYPE_NORMAL;
	cmd.resp_type = SDHCI_CMD_RESP_R1B;
	cmd.cmd_timeout = ((uint64_t)300 * 1000* 4 * dev->card.ext_csd[MMC_ERASE_TIMEOUT_MULT] * 10);
	mmc_ret = sdhci_send_command(&dev->host, &cmd);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] Erase CMD Sequence _ CMD38\n");
		return mmc_ret;
	}

	dprintf(CRITICAL, "[SAMSUNG] -End- Erase CMD Sequence\n");

	// [Samsung] ----- Write Patch Binary
	do{
		dprintf(CRITICAL, "[SAMSUNG] -Start- Write Patch Binary\n");

		mmc_ret = mmc_get_card_status(&dev->host, &dev->card, &mmc_status);
		if (mmc_ret) {
			dprintf(CRITICAL, "[SAMSUNG] [Fail] Write Patch Binary _ Check eMMC Status\n");
			return mmc_ret;
		}
		if (MMC_CARD_STATUS(mmc_status) != MMC_TRAN_STATE) {
			dprintf(CRITICAL, "[SAMSUNG] [Fail] Write Patch Binary _ eMMC not in tran state %x\n", mmc_status);
			return 1;
		}

		mmc_ret = mmc_write (0x00000000,fw_info->update_arry_size, (void *)fw_info->update_arry);
		if (mmc_ret) {
			dprintf(CRITICAL, "[SAMSUNG] [Fail] Write Patch Binary _ FW Write Fail\n");
			return 1;
		}
		dprintf(CRITICAL, "[SAMSUNG] -End- Write Patch Binary\n");

		// [Samsung] ----- Verify Patch Binary, only verify 3 sectors
		dprintf(CRITICAL, "[SAMSUNG] -Start- Verify Patch Binary\n");

	       size =  504*512;
		buf = memalign(PAGE_SIZE, ROUNDUP(size, PAGE_SIZE));
		if (!buf) {
			dprintf(CRITICAL, "%s: Aloc failed \n",__func__);
			//ret = GENERIC_ERROR;
			//goto err;
			return -1;
		}

		mmc_ret = mmc_read(0, (unsigned int *)buf,size); //Read_binary_temp
		if (mmc_ret) {
			dprintf(CRITICAL, "[SAMSUNG] [Fail] Verify Patch Binary at addr=0_ FW Read Fail\n");
			return 1;
		}

		for(i=0;i < 504*512;i++){
			if(fw_info->update_arry[i]!=*((unsigned char *)buf+i)){
				dprintf(CRITICAL, "[SAMSUNG] [Fail] Verify Patch Binary at addr=0 _ FW Verify Fail,i =%d\n",i);
				result = 1;
				break;
			}else {
				result = 0;
			}
		}

	} while(result == 1);
	dprintf(CRITICAL, "[SAMSUNG] -End- Verify Patch Binary\n");

	// [Samsung] ----- Confirm Patch Verification OK
	dprintf(CRITICAL, "[SAMSUNG] -Start- Confirm Patch Verification OK\n");

	cmd.cmd_index = 28;
	cmd.argument = 0x00000000; //0x000000FA
	cmd.cmd_type = SDHCI_CMD_TYPE_NORMAL;
	cmd.resp_type = SDHCI_CMD_RESP_R1B;

	mmc_ret = sdhci_send_command(&dev->host, &cmd);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] Confirm Patch Verification OK\n");
		return mmc_ret;
	}

	dprintf(CRITICAL, "[SAMSUNG] -End- Confirm Patch Verification OK\n");
	//dprintf(CRITICAL, "[SAMSUNG] -add mdelay 2s\n");
	mdelay(2000);

	// [Samsung] ----- eMMC Reset
	dprintf(CRITICAL, "[SAMSUNG] -Start- eMMC Reset\n");

	target_sdc_init_for_cid_program();

	mmc_ret = mmc_get_card_status(&dev->host, &dev->card, &mmc_status);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] eMMC Reset _ Check eMMC Status\n");
		return mmc_ret;
	}
	if (MMC_CARD_STATUS(mmc_status) != MMC_TRAN_STATE) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] eMMC Reset _ eMMC not in tran state %x\n", mmc_status);
		return 1;
	}

	dprintf(CRITICAL, "[SAMSUNG] eMMC tran state %x  _ CMD26\n", mmc_status);
	dprintf(CRITICAL, "[SAMSUNG] -End- eMMC Reset\n");

	// [Samsung] CID re-write
	dprintf(CRITICAL, "[SAMSUNG] -Start- CID re-write\n");

	cmd.cmd_index = 62;
	cmd.argument = 0xEFAC62EC;
	cmd.cmd_type = SDHCI_CMD_TYPE_NORMAL;
	cmd.resp_type = SDHCI_CMD_RESP_R1B;
	cmd.cmd_timeout = 0;

	mmc_ret = sdhci_send_command(&dev->host, &cmd);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] CID re-write _ Step.1\n");
		return mmc_ret;
	}

	cmd.cmd_index = 62;
	cmd.argument = 0x00CCED82;
	cmd.cmd_type = SDHCI_CMD_TYPE_NORMAL;
	cmd.resp_type = SDHCI_CMD_RESP_R1B;

	mmc_ret = sdhci_send_command(&dev->host, &cmd);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] CID re-write _ Step.2\n");
		return mmc_ret;
	}

	mmc_ret = mmc_get_card_status(&dev->host, &dev->card, &mmc_status);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] CID re-write _ Check eMMC Status\n");
		return mmc_ret;
	}
	if (MMC_CARD_STATUS(mmc_status) != MMC_TRAN_STATE) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] CID re-write _ eMMC not in tran state %x\n", mmc_status);
		return 1;
	}

	cmd.cmd_index = CMD16_SET_BLOCKLEN;
	cmd.argument = 0x00000010;
	cmd.cmd_type = SDHCI_CMD_TYPE_NORMAL;
	cmd.resp_type = SDHCI_CMD_RESP_R1;
	mmc_ret = sdhci_send_command(&dev->host, &cmd);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] CID re-write _ CMD16\n");
		return mmc_ret;
	}

	mmc_ret = mmc_get_card_status(&dev->host, &dev->card, &mmc_status);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] Last step.3 _ Check eMMC Status _ CMD26\n");
		return mmc_ret;
	}
	if (MMC_CARD_STATUS(mmc_status) != MMC_TRAN_STATE) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail]  Last step.3 _ eMMC not in tran state %x  _ CMD26\n", mmc_status);
		return 1;
	}
	dprintf(CRITICAL, "[SAMSUNG] eMMC tran state %x  _ CMD26\n", mmc_status);
	dprintf(CRITICAL, "[SAMSUNG] CID cid_ori=0x%x:0x%x:0x%x:0x%x\n",cid_ori[0], cid_ori[1], cid_ori[2],cid_ori[3]);

	cid_ori[3] = (mmc_cid_old.pnm[0] & 0xFF) | ((mmc_cid_old.oid & 0xFF) << 8) | (0x01 << 16) | ((mmc_cid_old.mid & 0xFF) << 24);
	cid_ori[2] = ((mmc_cid_old.pnm[1] & 0xFF) << 24) | ((mmc_cid_old.pnm[2] & 0xFF) << 16) | ((mmc_cid_old.pnm[3] & 0xFF) << 8) | (mmc_cid_old.pnm[4] & 0xFF);
	cid_ori[1] = ((mmc_cid_old.psn & 0xFFFF0000) >> 16)  | ((mmc_cid_old.prv & 0xFF) << 16) | ((mmc_cid_old.pnm[5] & 0xFF) << 24);
	cid_ori[0] = ((mmc_cid_old.month & 0xF) << 8) | (((mmc_cid_old.year % 1997)  & 0xF) << 12) | ((mmc_cid_old.psn & 0xFFFF) << 16);

	dprintf(CRITICAL, "[SAMSUNG] CID cid_ori=0x%x:0x%x:0x%x:0x%x\n",cid_ori[0], cid_ori[1], cid_ori[2],cid_ori[3]);

	for(i = 0;i < 4; i++)
	cid_temp[i] =((cid_ori[3-i] & 0x000000FF) << 24) |
	               ((cid_ori[3-i] & 0x0000FF00) << 8) |
	               ((cid_ori[3-i] & 0x00FF0000) >> 8) |
	               ((cid_ori[3-i] & 0xFF000000) >> 24) ;

	//150100513331334d42065c19d4cd3300
//	cid_temp[0] = 0x16010030;
//	cid_temp[0] = 0x51000115;
//	cid_temp[1] = 0x4d333133;
//	cid_temp[2] = 0x195c0642;
//	cid_temp[3] = 0x0033cdd4;

	cid_block = (uint32_t *)cid_temp;
	mmc_ret = mmc_write_for_cid_program(0x00000000,16, (void *)cid_block);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] Write Patch Binary _ FW Write Fail\n");
		return 1;
	}

	cmd.cmd_index = 62;
	cmd.argument = 0xDECCEE;
	cmd.cmd_type = SDHCI_CMD_TYPE_NORMAL;
	cmd.resp_type = SDHCI_CMD_RESP_R1B;

	mmc_ret = sdhci_send_command(&dev->host, &cmd);
	if (mmc_ret) {
		dprintf(CRITICAL, "[SAMSUNG] [Fail] Last step.1\n");
		return mmc_ret;
	}
	dprintf(CRITICAL, "[SAMSUNG] -End- CID re-write\n");
	dprintf(CRITICAL, "[SAMSUNG] Full Block Erase Patch Success\n");
	reboot_device(NORMAL_DLOAD);
	dprintf(CRITICAL, "should no get here\n");
	return 0;
}

#endif

/* Function to return recovery appended dtbo buffer info */
void get_recovery_dtbo_info(uint32_t *dtbo_size, void **dtbo_buf)
{
	*dtbo_size = recovery_dtbo_size;
	*dtbo_buf = recovery_dtbo_buf;
	return;
}

int boot_linux_from_mmc(void)
{
	boot_img_hdr *hdr = (void*) buf;
	boot_img_hdr *uhdr;
	unsigned offset = 0;
	int rcode;
	unsigned long long ptn = 0;
	int index = INVALID_PTN;

	unsigned char *image_addr = 0;
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	unsigned imagesize_actual;
	unsigned second_actual = 0;

	unsigned int dtb_size = 0;
	unsigned int out_len = 0;
	unsigned int out_avai_len = 0;
	unsigned char *out_addr = NULL;
	uint32_t dtb_offset = 0;
	unsigned char *kernel_start_addr = NULL;
	unsigned int kernel_size = 0;
	unsigned int patched_kernel_hdr_size = 0;
	uint64_t image_size = 0;
	int rc;
#if VERIFIED_BOOT_2
	int status;
	void *dtbo_image_buf = NULL;
	uint32_t dtbo_image_sz = 0;
	void *vbmeta_image_buf = NULL;
	uint32_t vbmeta_image_sz = 0;
#endif
	char *ptn_name = NULL;
	
#ifdef SAMSUNG_RPMB_RESET
	void *dev;
	uint8_t mmc_ret = 0;
#endif
	
#if DEVICE_TREE
	struct dt_table *table;
	struct dt_entry dt_entry;
	unsigned dt_table_offset;
	uint32_t dt_actual;
	uint32_t dt_hdr_size;
	unsigned char *best_match_dt_addr = NULL;
#endif
	struct kernel64_hdr *kptr = NULL;
	int current_active_slot = INVALID;
	
#ifdef SAMSUNG_RPMB_RESET
	dprintf(CRITICAL,"[SAMSUNG] SAMSUNG_RPMB_RESET\n");
	dev = target_mmc_device();
	mmc_ret = mmc_rpmb_reset((struct mmc_device *)dev);
	if (mmc_ret){
		dprintf(CRITICAL,"[SAMSUNG] Full Block Erase Patch Fail\n");
		return -1;
	}else{
		dprintf(CRITICAL,"[SAMSUNG] Full Block Erase Patch Success\n");
	}
#endif

	if (check_format_bit())
		boot_into_recovery = 1;

	if (!boot_into_recovery) {
		memset(ffbm_mode_string, '\0', sizeof(ffbm_mode_string));
		rcode = get_ffbm(ffbm_mode_string, sizeof(ffbm_mode_string));
		if (rcode <= 0) {
			boot_into_ffbm = false;
			if (rcode < 0)
				dprintf(CRITICAL,"failed to get ffbm cookie");
		} else
			boot_into_ffbm = true;
	} else
		boot_into_ffbm = false;
	uhdr = (boot_img_hdr *)EMMC_BOOT_IMG_HEADER_ADDR;
	if (!memcmp(uhdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
		dprintf(INFO, "Unified boot method!\n");
		hdr = uhdr;
		goto unified_boot;
	}

	/* For a/b recovery image code is on boot partition.
	   If we support multislot, always use boot partition. */
	if (boot_into_recovery &&
		(!partition_multislot_is_supported()))
			ptn_name = "recovery";
	else
			ptn_name = "boot";

	index = partition_get_index(ptn_name);
	ptn = partition_get_offset(index);
	image_size = partition_get_size(index);
	if(ptn == 0 || image_size == 0) {
		dprintf(CRITICAL, "ERROR: No %s partition found\n", ptn_name);
		return -1;
	}

	/* Set Lun for boot & recovery partitions */
	mmc_set_lun(partition_get_lun(index));

	if (mmc_read(ptn + offset, (uint32_t *) buf, page_size)) {
		dprintf(CRITICAL, "ERROR: Cannot read boot image header\n");
                return -1;
	}

	if (memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
		dprintf(CRITICAL, "ERROR: Invalid boot image header\n");
                return ERR_INVALID_BOOT_MAGIC;
	}

	if (hdr->page_size && (hdr->page_size != page_size)) {

		if (hdr->page_size > BOOT_IMG_MAX_PAGE_SIZE) {
			dprintf(CRITICAL, "ERROR: Invalid page size\n");
			return -1;
		}
		page_size = hdr->page_size;
		page_mask = page_size - 1;
	}

	kernel_actual  = ROUND_TO_PAGE(hdr->kernel_size,  page_mask);
	ramdisk_actual = ROUND_TO_PAGE(hdr->ramdisk_size, page_mask);
	second_actual  = ROUND_TO_PAGE(hdr->second_size, page_mask);

	image_addr = (unsigned char *)target_get_scratch_address();
	memcpy(image_addr, (void *)buf, page_size);

	/* ensure commandline is terminated */
        hdr->cmdline[BOOT_ARGS_SIZE-1] = 0;

#if DEVICE_TREE
#ifndef OSVERSION_IN_BOOTIMAGE
	dt_size = hdr->dt_size;
#else
	dprintf(INFO, "BootImage Header: %d\n", hdr->header_version);
#endif

	dt_actual = ROUND_TO_PAGE(dt_size, page_mask);
	if (UINT_MAX < ((uint64_t)kernel_actual + (uint64_t)ramdisk_actual+ (uint64_t)second_actual + (uint64_t)dt_actual + page_size)) {
		dprintf(CRITICAL, "Integer overflow detected in bootimage header fields at %u in %s\n",__LINE__,__FILE__);
		return -1;
	}
	imagesize_actual = (page_size + kernel_actual + ramdisk_actual + second_actual + dt_actual);
#else
	if (UINT_MAX < ((uint64_t)kernel_actual + (uint64_t)ramdisk_actual + (uint64_t)second_actual + page_size)) {
		dprintf(CRITICAL, "Integer overflow detected in bootimage header fields at %u in %s\n",__LINE__,__FILE__);
		return -1;
	}
	imagesize_actual = (page_size + kernel_actual + ramdisk_actual + second_actual);
#endif

#ifdef OSVERSION_IN_BOOTIMAGE
	/* If header version is ONE and booting into recovery,
		dtbo is appended with recovery image.
		Doing following:
			* Validating the recovery offset and size.
			* Extracting recovery dtbo to be used as dtbo.
	*/
	if (boot_into_recovery &&
		hdr->header_version == BOOT_HEADER_VERSION_ONE)
	{
		struct boot_img_hdr_v1 *hdr1 =
			(struct boot_img_hdr_v1 *) (image_addr + sizeof(boot_img_hdr));

		if ((hdr1->header_size !=
				sizeof(struct boot_img_hdr_v1) + sizeof(boot_img_hdr)))
		{
			dprintf(CRITICAL, "Invalid boot image header: %d\n", hdr1->header_size);
			return -1;
		}

		if (UINT_MAX < (hdr1->recovery_dtbo_offset + hdr1->recovery_dtbo_size)) {
			dprintf(CRITICAL,
				"Integer overflow detected in recovery image header fields at %u in %s\n",__LINE__,__FILE__);
			return -1;
		}

		if (hdr1->recovery_dtbo_size > MAX_SUPPORTED_DTBO_IMG_BUF) {
			dprintf(CRITICAL, "Recovery Dtbo Size too big %x, Allowed size %x\n", hdr1->recovery_dtbo_size,
				MAX_SUPPORTED_DTBO_IMG_BUF);
			return -1;
		}

		if (UINT_MAX < ((uint64_t)imagesize_actual + recovery_dtbo_size))
		{
			dprintf(CRITICAL, "Integer overflow detected in recoveryimage header fields at %u in %s\n",__LINE__,__FILE__);
			return -1;
		}

		if (hdr1->recovery_dtbo_offset + recovery_dtbo_size > image_size)
		{
			dprintf(CRITICAL, "Invalid recovery dtbo: Recovery Dtbo Offset=0x%llx,"
				" Recovery Dtbo Size=0x%x, Image Size=0x%llx\n",
				hdr1->recovery_dtbo_offset, recovery_dtbo_size, image_size);
			return -1;
		}

		recovery_dtbo_buf = (void *)(hdr1->recovery_dtbo_offset + image_addr);
		recovery_dtbo_size = hdr1->recovery_dtbo_size;
		imagesize_actual += recovery_dtbo_size;

		dprintf(SPEW, "Header version: %d\n", hdr->header_version);
		dprintf(SPEW, "Recovery Dtbo Size 0x%x\n", recovery_dtbo_size);
		dprintf(SPEW, "Recovery Dtbo Offset 0x%llx\n", hdr1->recovery_dtbo_offset);

	}
#endif

#if VERIFIED_BOOT
	boot_verifier_init();
#endif

	if (check_aboot_addr_range_overlap((uintptr_t) image_addr, imagesize_actual))
	{
		dprintf(CRITICAL, "Boot image buffer address overlaps with aboot addresses.\n");
		return -1;
	}

	/*
	 * Update loading flow of bootimage to support compressed/uncompressed
	 * bootimage on both 64bit and 32bit platform.
	 * 1. Load bootimage from emmc partition onto DDR.
	 * 2. Check if bootimage is gzip format. If yes, decompress compressed kernel
	 * 3. Check kernel header and update kernel load addr for 64bit and 32bit
	 *    platform accordingly.
	 * 4. Sanity Check on kernel_addr and ramdisk_addr and copy data.
	 */
	if (partition_multislot_is_supported())
	{
		current_active_slot = partition_find_active_slot();
		dprintf(INFO, "Loading boot image (%d) active_slot(%s): start\n",
				imagesize_actual, SUFFIX_SLOT(current_active_slot));
	}
	else
	{
		dprintf(INFO, "Loading (%s) image (%d): start\n",
				(!boot_into_recovery ? "boot" : "recovery"),imagesize_actual);
	}
	bs_set_timestamp(BS_KERNEL_LOAD_START);

	if ((target_get_max_flash_size() - page_size) < imagesize_actual)
	{
		dprintf(CRITICAL, "booimage  size is greater than DDR can hold\n");
		return -1;
	}
	offset = page_size;
	/* Read image without signature and header*/
	if (mmc_read(ptn + offset, (void *)(image_addr + offset), imagesize_actual - page_size))
	{
		dprintf(CRITICAL, "ERROR: Cannot read boot image\n");
		return -1;
	}

	if (partition_multislot_is_supported())
	{
		dprintf(INFO, "Loading boot image (%d) active_slot(%s): done\n",
				imagesize_actual, SUFFIX_SLOT(current_active_slot));
	}
	else
	{
		dprintf(INFO, "Loading (%s) image (%d): done\n",
			(!boot_into_recovery ? "boot" : "recovery"),imagesize_actual);

	}
	bs_set_timestamp(BS_KERNEL_LOAD_DONE);

	/* Authenticate Kernel */
	dprintf(INFO, "use_signed_kernel=%d, is_unlocked=%d, is_tampered=%d.\n",
		(int) target_use_signed_kernel(),
		device.is_unlocked,
		device.is_tampered);
#if VERIFIED_BOOT_2
	offset = imagesize_actual;
	if (check_aboot_addr_range_overlap((uintptr_t)image_addr + offset, page_size))
	{
		dprintf(CRITICAL, "Signature read buffer address overlaps with aboot addresses.\n");
		return -1;
	}

	/* Read signature */
	if(mmc_read(ptn + offset, (void *)(image_addr + offset), page_size))
	{
		dprintf(CRITICAL, "ERROR: Cannot read boot image signature\n");
		return -1;
	}

	/* load and validate dtbo partition */
	load_validate_dtbo_image(&dtbo_image_buf, &dtbo_image_sz);

	/* load vbmeta partition */
	load_vbmeta_image(&vbmeta_image_buf, &vbmeta_image_sz);

	memset(&info, 0, sizeof(bootinfo));

	/* Pass loaded boot image passed */
	info.images[IMG_BOOT].image_buffer = image_addr;
	info.images[IMG_BOOT].imgsize = imagesize_actual;
	info.images[IMG_BOOT].name = ptn_name;
	++info.num_loaded_images;

	/* Pass loaded dtbo image */
	if (dtbo_image_buf != NULL) {
		info.images[IMG_DTBO].image_buffer = dtbo_image_buf;
		info.images[IMG_DTBO].imgsize = dtbo_image_sz;
		info.images[IMG_DTBO].name = "dtbo";
		++info.num_loaded_images;
	}

	/* Pass loaded vbmeta image */
	if (vbmeta_image_buf != NULL) {
		info.images[IMG_VBMETA].image_buffer = vbmeta_image_buf;
		info.images[IMG_VBMETA].imgsize = vbmeta_image_sz;
		info.images[IMG_VBMETA].name = "vbmeta";
		++info.num_loaded_images;
	}

	info.header_version = hdr->header_version;
	info.multi_slot_boot = partition_multislot_is_supported();
	info.bootreason_alarm = boot_reason_alarm;
	info.bootinto_recovery = boot_into_recovery;
	status = load_image_and_auth(&info);
	if(status)
		return -1;

	vbcmdline = info.vbcmdline;

	/* Free the buffer allocated to vbmeta post verification */
	free(vbmeta_image_buf);
	--info.num_loaded_images;
#else
	/* Change the condition a little bit to include the test framework support.
	 * We would never reach this point if device is in fastboot mode, even if we did
	 * that means we are in test mode, so execute kernel authentication part for the
	 * tests */

	#ifdef HQ_BUILD_FLAG
		if((target_use_signed_kernel()) || is_test_mode_enabled())
	#else
		if((target_use_signed_kernel() && (!device.is_unlocked)) || is_test_mode_enabled())
        #endif
	{
		offset = imagesize_actual;
		if (check_aboot_addr_range_overlap((uintptr_t)image_addr + offset, page_size))
		{
			dprintf(CRITICAL, "Signature read buffer address overlaps with aboot addresses.\n");
			return -1;
		}

		/* Read signature */
		if(mmc_read(ptn + offset, (void *)(image_addr + offset), page_size))
		{
			dprintf(CRITICAL, "ERROR: Cannot read boot image signature\n");
			return -1;
		}

		if(verify_signed_bootimg((uint32_t)image_addr, imagesize_actual)){
			/* The purpose of our test is done here */
			if(is_test_mode_enabled() && auth_kernel_img)
				return 0;
		} else {
			return -1;
		}
	} else {
		second_actual  = ROUND_TO_PAGE(hdr->second_size,  page_mask);
		#ifdef TZ_SAVE_KERNEL_HASH
		aboot_save_boot_hash_mmc((uint32_t) image_addr, imagesize_actual);
		#endif /* TZ_SAVE_KERNEL_HASH */

#ifdef MDTP_SUPPORT
		{
			/* Verify MDTP lock.
			 * For boot & recovery partitions, MDTP will use boot_verifier APIs,
			 * since verification was skipped in aboot. The signature is not part of the loaded image.
			 */
			mdtp_ext_partition_verification_t ext_partition;
			ext_partition.partition = boot_into_recovery ? MDTP_PARTITION_RECOVERY : MDTP_PARTITION_BOOT;
			ext_partition.integrity_state = MDTP_PARTITION_STATE_UNSET;
			ext_partition.page_size = page_size;
			ext_partition.image_addr = (uint32)image_addr;
			ext_partition.image_size = imagesize_actual;
			ext_partition.sig_avail = FALSE;
			mdtp_fwlock_verify_lock(&ext_partition);
		}
#endif /* MDTP_SUPPORT */
	}
#endif

#if VERIFIED_BOOT
	if((boot_verify_get_state() == ORANGE) && (!boot_into_ffbm))
	{
#if FBCON_DISPLAY_MSG
		display_bootverify_menu(DISPLAY_MENU_ORANGE);
		wait_for_users_action();
#else
		dprintf(CRITICAL,
			"Your device has been unlocked and can't be trusted.\nWait for 5 seconds before proceeding\n");
		mdelay(5000);
#endif
	}
#endif

#if VERIFIED_BOOT
	if (VB_M == target_get_vb_version())
	{
		/* set boot and system versions. */
		set_os_version((unsigned char *)image_addr);
		// send root of trust
		if(!send_rot_command((uint32_t)device.is_unlocked))
			ASSERT(0);
	}
#endif
	/*
	 * Check if the kernel image is a gzip package. If yes, need to decompress it.
	 * If not, continue booting.
	 */
	if (is_gzip_package((unsigned char *)(image_addr + page_size), hdr->kernel_size))
	{
		out_addr = (unsigned char *)(image_addr + imagesize_actual + page_size);
		out_avai_len = target_get_max_flash_size() - imagesize_actual - page_size;
#if VERIFIED_BOOT_2
		if (dtbo_image_sz)
			out_avai_len -= DTBO_IMG_BUF;
#endif
		dprintf(INFO, "decompressing kernel image: start\n");
		rc = decompress((unsigned char *)(image_addr + page_size),
				hdr->kernel_size, out_addr, out_avai_len,
				&dtb_offset, &out_len);
		if (rc)
		{
			dprintf(CRITICAL, "decompressing kernel image failed!!!\n");
			ASSERT(0);
		}

		dprintf(INFO, "decompressing kernel image: done\n");
		kptr = (struct kernel64_hdr *)out_addr;
		kernel_start_addr = out_addr;
		kernel_size = out_len;
	} else {
		dprintf(INFO, "Uncpmpressed kernel in use\n");
		if (!strncmp((char*)(image_addr + page_size),
					PATCHED_KERNEL_MAGIC,
					sizeof(PATCHED_KERNEL_MAGIC) - 1)) {
			dprintf(INFO, "Patched kernel detected\n");
			kptr = (struct kernel64_hdr *)(image_addr + page_size +
					PATCHED_KERNEL_HEADER_SIZE);
			//The size of the kernel is stored at start of kernel image + 16
			//The dtb would start just after the kernel
			dtb_offset = *((uint32_t*)((unsigned char*)
						(image_addr + page_size +
						 sizeof(PATCHED_KERNEL_MAGIC) -
						 1)));
			//The actual kernel starts after the 20 byte header.
			kernel_start_addr = (unsigned char*)(image_addr +
					page_size + PATCHED_KERNEL_HEADER_SIZE);
			kernel_size = hdr->kernel_size;
			patched_kernel_hdr_size = PATCHED_KERNEL_HEADER_SIZE;
		} else {
			dprintf(INFO, "Kernel image not patched..Unable to locate dt offset\n");
			kptr = (struct kernel64_hdr *)(image_addr + page_size);
			kernel_start_addr = (unsigned char *)(image_addr + page_size);
			kernel_size = hdr->kernel_size;
		}
	}

	/*
	 * Update the kernel/ramdisk/tags address if the boot image header
	 * has default values, these default values come from mkbootimg when
	 * the boot image is flashed using fastboot flash:raw
	 */
	update_ker_tags_rdisk_addr(hdr, IS_ARM64(kptr));

	/* Get virtual addresses since the hdr saves physical addresses. */
	hdr->kernel_addr = VA((addr_t)(hdr->kernel_addr));
	hdr->ramdisk_addr = VA((addr_t)(hdr->ramdisk_addr));
	hdr->tags_addr = VA((addr_t)(hdr->tags_addr));

	kernel_size = ROUND_TO_PAGE(kernel_size,  page_mask);
	/* Check if the addresses in the header are valid. */
	if (check_aboot_addr_range_overlap(hdr->kernel_addr, kernel_size) ||
		check_ddr_addr_range_bound(hdr->kernel_addr, kernel_size) ||
		check_aboot_addr_range_overlap(hdr->ramdisk_addr, ramdisk_actual) ||
		check_ddr_addr_range_bound(hdr->ramdisk_addr, ramdisk_actual))
	{
		dprintf(CRITICAL, "kernel/ramdisk addresses are not valid.\n");
		return -1;
	}

#ifndef DEVICE_TREE
	if (check_aboot_addr_range_overlap(hdr->tags_addr, MAX_TAGS_SIZE) ||
		check_ddr_addr_range_bound(hdr->tags_addr, MAX_TAGS_SIZE))
	{
		dprintf(CRITICAL, "Tags addresses are not valid.\n");
		return -1;
	}
#endif

	/* Move kernel, ramdisk and device tree to correct address */
	memmove((void*) hdr->kernel_addr, kernel_start_addr, kernel_size);
	memmove((void*) hdr->ramdisk_addr, (char *)(image_addr + page_size + kernel_actual), hdr->ramdisk_size);

	#if DEVICE_TREE
	if(dt_size) {
		dt_table_offset = ((uint32_t)image_addr + page_size + kernel_actual + ramdisk_actual + second_actual);
		table = (struct dt_table*) dt_table_offset;

		if (dev_tree_validate(table, hdr->page_size, &dt_hdr_size) != 0) {
			dprintf(CRITICAL, "ERROR: Cannot validate Device Tree Table \n");
			return -1;
		}

		/* Its Error if, dt_hdr_size (table->num_entries * dt_entry size + Dev_Tree Header)
		goes beyound hdr->dt_size*/
		if (dt_hdr_size > ROUND_TO_PAGE(dt_size,hdr->page_size)) {
			dprintf(CRITICAL, "ERROR: Invalid Device Tree size \n");
			return -1;
		}

		/* Find index of device tree within device tree table */
		if(dev_tree_get_entry_info(table, &dt_entry) != 0){
			dprintf(CRITICAL, "ERROR: Getting device tree address failed\n");
			return -1;
		}

		if(dt_entry.offset > (UINT_MAX - dt_entry.size)) {
			dprintf(CRITICAL, "ERROR: Device tree contents are Invalid\n");
			return -1;
		}

		/* Ensure we are not overshooting dt_size with the dt_entry selected */
		if ((dt_entry.offset + dt_entry.size) > dt_size) {
			dprintf(CRITICAL, "ERROR: Device tree contents are Invalid\n");
			return -1;
		}

		if (is_gzip_package((unsigned char *)dt_table_offset + dt_entry.offset, dt_entry.size))
		{
			unsigned int compressed_size = 0;
			out_addr += out_len;
			out_avai_len -= out_len;
			dprintf(INFO, "decompressing dtb: start\n");
			rc = decompress((unsigned char *)dt_table_offset + dt_entry.offset,
					dt_entry.size, out_addr, out_avai_len,
					&compressed_size, &dtb_size);
			if (rc)
			{
				dprintf(CRITICAL, "decompressing dtb failed!!!\n");
				ASSERT(0);
			}

			dprintf(INFO, "decompressing dtb: done\n");
			best_match_dt_addr = out_addr;
		} else {
			best_match_dt_addr = (unsigned char *)dt_table_offset + dt_entry.offset;
			dtb_size = dt_entry.size;
		}

		/* Validate and Read device device tree in the tags_addr */
		if (check_aboot_addr_range_overlap(hdr->tags_addr, dtb_size) ||
			check_ddr_addr_range_bound(hdr->tags_addr, dtb_size))
		{
			dprintf(CRITICAL, "Device tree addresses are not valid\n");
			return -1;
		}

		memmove((void *)hdr->tags_addr, (char *)best_match_dt_addr, dtb_size);
	} else {
		/* Validate the tags_addr */
		if (check_aboot_addr_range_overlap(hdr->tags_addr, kernel_actual) ||
			check_ddr_addr_range_bound(hdr->tags_addr, kernel_actual))
		{
			dprintf(CRITICAL, "Device tree addresses are not valid.\n");
			return -1;
		}
		/*
		 * If appended dev tree is found, update the atags with
		 * memory address to the DTB appended location on RAM.
		 * Else update with the atags address in the kernel header
		 */
		void *dtb;
		dtb = dev_tree_appended(
				(void*)(image_addr + page_size +
					patched_kernel_hdr_size),
				hdr->kernel_size, dtb_offset,
				(void *)hdr->tags_addr);
		if (!dtb) {
			dprintf(CRITICAL, "ERROR: Appended Device Tree Blob not found\n");
			return -1;
		}
	}
	#endif

	if (boot_into_recovery && !device.is_unlocked && !device.is_tampered)
		target_load_ssd_keystore();

unified_boot:

	boot_linux((void *)hdr->kernel_addr, (void *)hdr->tags_addr,
		   (const char *)hdr->cmdline, board_machtype(),
		   (void *)hdr->ramdisk_addr, hdr->ramdisk_size);

	return 0;
}

int boot_linux_from_flash(void)
{
	boot_img_hdr *hdr = (void*) buf;
	struct ptentry *ptn;
	struct ptable *ptable;
	unsigned offset = 0;

	unsigned char *image_addr = 0;
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	unsigned imagesize_actual;
	unsigned second_actual = 0;

#if DEVICE_TREE
	struct dt_table *table = NULL;
	struct dt_entry dt_entry;
	unsigned dt_table_offset;
	uint32_t dt_actual;
	uint32_t dt_hdr_size = 0;
	uint32_t dtb_offset = 0;
	unsigned int dtb_size = 0;
	unsigned char *best_match_dt_addr = NULL;
#endif

	if (target_is_emmc_boot()) {
		hdr = (boot_img_hdr *)EMMC_BOOT_IMG_HEADER_ADDR;
		if (memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
			dprintf(CRITICAL, "ERROR: Invalid boot image header\n");
			return -1;
		}
		goto continue_boot;
	}

	ptable = flash_get_ptable();
	if (ptable == NULL) {
		dprintf(CRITICAL, "ERROR: Partition table not found\n");
		return -1;
	}

	if(!boot_into_recovery)
	{
	        ptn = ptable_find(ptable, "boot");

	        if (ptn == NULL) {
		        dprintf(CRITICAL, "ERROR: No boot partition found\n");
		        return -1;
	        }
	}
	else
	{
	        ptn = ptable_find(ptable, "recovery");
	        if (ptn == NULL) {
		        dprintf(CRITICAL, "ERROR: No recovery partition found\n");
		        return -1;
	        }
	}

	/* Read boot.img header from flash */
	if (flash_read(ptn, offset, buf, page_size)) {
		dprintf(CRITICAL, "ERROR: Cannot read boot image header\n");
		return -1;
	}

	if (memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
		dprintf(CRITICAL, "ERROR: Invalid boot image header\n");
		return -1;
	}

	if (hdr->page_size != page_size) {
		dprintf(CRITICAL, "ERROR: Invalid boot image pagesize. Device pagesize: %d, Image pagesize: %d\n",page_size,hdr->page_size);
		return -1;
	}

	image_addr = (unsigned char *)target_get_scratch_address();
	memcpy(image_addr, (void *)buf, page_size);

	/*
	 * Update the kernel/ramdisk/tags address if the boot image header
	 * has default values, these default values come from mkbootimg when
	 * the boot image is flashed using fastboot flash:raw
	 */
	update_ker_tags_rdisk_addr(hdr, false);

	/* Get virtual addresses since the hdr saves physical addresses. */
	hdr->kernel_addr = VA((addr_t)(hdr->kernel_addr));
	hdr->ramdisk_addr = VA((addr_t)(hdr->ramdisk_addr));
	hdr->tags_addr = VA((addr_t)(hdr->tags_addr));

	kernel_actual  = ROUND_TO_PAGE(hdr->kernel_size,  page_mask);
	ramdisk_actual = ROUND_TO_PAGE(hdr->ramdisk_size, page_mask);
	second_actual = ROUND_TO_PAGE(hdr->second_size, page_mask);

	/* ensure commandline is terminated */
	hdr->cmdline[BOOT_ARGS_SIZE-1] = 0;

	/* Check if the addresses in the header are valid. */
	if (check_aboot_addr_range_overlap(hdr->kernel_addr, kernel_actual) ||
		check_ddr_addr_range_bound(hdr->kernel_addr, kernel_actual) ||
		check_aboot_addr_range_overlap(hdr->ramdisk_addr, ramdisk_actual) ||
		check_ddr_addr_range_bound(hdr->ramdisk_addr, ramdisk_actual))
	{
		dprintf(CRITICAL, "kernel/ramdisk addresses are not valid.\n");
		return -1;
	}

#ifndef DEVICE_TREE
	if (UINT_MAX < ((uint64_t)kernel_actual + (uint64_t)ramdisk_actual+ (uint64_t)second_actual + page_size)) {
		dprintf(CRITICAL, "Integer overflow detected in bootimage header fields\n");
		return -1;
	}
	imagesize_actual = (page_size + kernel_actual + ramdisk_actual + second_actual);

	if (check_aboot_addr_range_overlap(hdr->tags_addr, MAX_TAGS_SIZE) ||
		check_ddr_addr_range_bound(hdr->tags_addr, MAX_TAGS_SIZE))
	{
		dprintf(CRITICAL, "Tags addresses are not valid.\n");
		return -1;
	}
#else

#ifndef OSVERSION_IN_BOOTIMAGE
	dt_size = hdr->dt_size;
#endif
	dt_actual = ROUND_TO_PAGE(dt_size, page_mask);
	if (UINT_MAX < ((uint64_t)kernel_actual + (uint64_t)ramdisk_actual+ (uint64_t)second_actual + (uint64_t)dt_actual + page_size)) {
		dprintf(CRITICAL, "Integer overflow detected in bootimage header fields\n");
		return -1;
	}

	imagesize_actual = (page_size + kernel_actual + ramdisk_actual + second_actual + dt_actual);

	if (check_aboot_addr_range_overlap(hdr->tags_addr, dt_size) ||
		check_ddr_addr_range_bound(hdr->tags_addr, dt_size))
	{
		dprintf(CRITICAL, "Device tree addresses are not valid.\n");
		return -1;
	}
#endif

	/* Read full boot.img from flash */
	dprintf(INFO, "Loading (%s) image (%d): start\n",
		(!boot_into_recovery ? "boot" : "recovery"),imagesize_actual);
	bs_set_timestamp(BS_KERNEL_LOAD_START);

	if (UINT_MAX - page_size < imagesize_actual)
	{
		dprintf(CRITICAL,"Integer overflow detected in bootimage header fields %u %s\n", __LINE__,__func__);
		return -1;
	}

	/*Check the availability of RAM before reading boot image + max signature length from flash*/
	if (target_get_max_flash_size() < (imagesize_actual + page_size))
	{
		dprintf(CRITICAL, "bootimage  size is greater than DDR can hold\n");
		return -1;
	}

	offset = page_size;
	/* Read image without signature and header */
	if (flash_read(ptn, offset, (void *)(image_addr + offset), imagesize_actual - page_size))
	{
		dprintf(CRITICAL, "ERROR: Cannot read boot image\n");
			return -1;
	}

	dprintf(INFO, "Loading (%s) image (%d): done\n",
		(!boot_into_recovery ? "boot" : "recovery"), imagesize_actual);
	bs_set_timestamp(BS_KERNEL_LOAD_DONE);

	/* Authenticate Kernel */
	if(target_use_signed_kernel() && (!device.is_unlocked))
	{
		offset = imagesize_actual;

		/* Read signature */
		if (flash_read(ptn, offset, (void *)(image_addr + offset), page_size))
		{
			dprintf(CRITICAL, "ERROR: Cannot read boot image signature\n");
			return -1;
		}

		verify_signed_bootimg((uint32_t)image_addr, imagesize_actual);
	}
	offset = page_size;
	if(hdr->second_size != 0) {
		if (UINT_MAX - offset < second_actual)
		{
			dprintf(CRITICAL, "ERROR: Integer overflow in boot image header %s\t%d\n",__func__,__LINE__);
			return -1;
		}
		offset += second_actual;
		/* Second image loading not implemented. */
		ASSERT(0);
	}
	/* Move kernel and ramdisk to correct address */
	memmove((void*) hdr->kernel_addr, (char*) (image_addr + page_size), hdr->kernel_size);
	memmove((void*) hdr->ramdisk_addr, (char*) (image_addr + page_size + kernel_actual), hdr->ramdisk_size);

#if DEVICE_TREE
	if(dt_size != 0) {

		dt_table_offset = ((uint32_t)image_addr + page_size + kernel_actual + ramdisk_actual + second_actual);

		table = (struct dt_table*) dt_table_offset;

		if (dev_tree_validate(table, hdr->page_size, &dt_hdr_size) != 0) {
			dprintf(CRITICAL, "ERROR: Cannot validate Device Tree Table \n");
			return -1;
		}

		/* Its Error if, dt_hdr_size (table->num_entries * dt_entry size + Dev_Tree Header)
		goes beyound hdr->dt_size*/
		if (dt_hdr_size > ROUND_TO_PAGE(dt_size,hdr->page_size)) {
			dprintf(CRITICAL, "ERROR: Invalid Device Tree size \n");
			return -1;
		}

		/* Find index of device tree within device tree table */
		if(dev_tree_get_entry_info(table, &dt_entry) != 0){
			dprintf(CRITICAL, "ERROR: Getting device tree address failed\n");
			return -1;
		}

		/* Validate and Read device device tree in the "tags_add */
		if (check_aboot_addr_range_overlap(hdr->tags_addr, dt_entry.size) ||
			check_ddr_addr_range_bound(hdr->tags_addr, dt_entry.size))
		{
			dprintf(CRITICAL, "Device tree addresses are not valid.\n");
			return -1;
		}

		if(dt_entry.offset > (UINT_MAX - dt_entry.size)) {
			dprintf(CRITICAL, "ERROR: Device tree contents are Invalid\n");
			return -1;
		}

		/* Ensure we are not overshooting dt_size with the dt_entry selected */
		if ((dt_entry.offset + dt_entry.size) > dt_size) {
			dprintf(CRITICAL, "ERROR: Device tree contents are Invalid\n");
			return -1;
		}

		best_match_dt_addr = (unsigned char *)table + dt_entry.offset;
		dtb_size = dt_entry.size;
		memmove((void *)hdr->tags_addr, (char *)best_match_dt_addr, dtb_size);

	} else {
		/* Validate the tags_addr */
		if (check_aboot_addr_range_overlap(hdr->tags_addr, kernel_actual) ||
	        check_ddr_addr_range_bound(hdr->tags_addr, kernel_actual))
		{
			dprintf(CRITICAL, "Device tree addresses are not valid.\n");
			return -1;
		}
		/*
		 * If appended dev tree is found, update the atags with
		 * memory address to the DTB appended location on RAM.
		 * Else update with the atags address in the kernel header
		 */
		void *dtb = NULL;
		dtb = dev_tree_appended((void*)(image_addr + page_size ),hdr->kernel_size, dtb_offset, (void *)hdr->tags_addr);
		if (!dtb) {
			dprintf(CRITICAL, "ERROR: Appended Device Tree Blob not found\n");
			return -1;
		}
         }
#endif
	if(target_use_signed_kernel() && (!device.is_unlocked))
	{
		/* Make sure everything from scratch address is read before next step!*/
		if(device.is_tampered)
		{
			write_device_info_flash(&device);
		}
#if USE_PCOM_SECBOOT
		set_tamper_flag(device.is_tampered);
#endif
	}
continue_boot:

	/* TODO: create/pass atags to kernel */

	boot_linux((void *)hdr->kernel_addr, (void *)hdr->tags_addr,
		   (const char *)hdr->cmdline, board_machtype(),
		   (void *)hdr->ramdisk_addr, hdr->ramdisk_size);

	return 0;
}

void write_device_info_mmc(device_info *dev)
{
	unsigned long long ptn = 0;
	unsigned long long size;
	int index = INVALID_PTN;
	uint8_t lun = 0;
	uint32_t ret = 0;
	uint32_t device_info_sz = 0;

	if (devinfo_present)
		index = partition_get_index("devinfo");
	else
		index = partition_get_index("aboot");

	ptn = partition_get_offset(index);
	if(ptn == 0)
	{
		return;
	}

	lun = partition_get_lun(index);
	mmc_set_lun(lun);

	size = partition_get_size(index);

	device_info_sz = ROUND_TO_PAGE(sizeof(struct device_info),
							mmc_blocksize_mask);
	if (device_info_sz == UINT_MAX)
	{
		dprintf(CRITICAL, "ERROR: Incorrect blocksize of card\n");
		return;
	}

	if (devinfo_present)
		ret = mmc_write(ptn, device_info_sz, (void *)info_buf);
	else
		ret = mmc_write((ptn + size - device_info_sz), device_info_sz, (void *)info_buf);
	if (ret)
	{
		dprintf(CRITICAL, "ERROR: Cannot write device info\n");
		ASSERT(0);
	}
}

void read_device_info_mmc(struct device_info *info)
{
	unsigned long long ptn = 0;
	unsigned long long size;
	int index = INVALID_PTN;
	uint32_t ret  = 0;
	uint32_t device_info_sz = 0;

	if ((index = partition_get_index("devinfo")) < 0)
	{
		devinfo_present = false;
		index = partition_get_index("aboot");
	}

	ptn = partition_get_offset(index);
	if(ptn == 0)
	{
		return;
	}

	mmc_set_lun(partition_get_lun(index));

	size = partition_get_size(index);

	device_info_sz = ROUND_TO_PAGE(sizeof(struct device_info),
							mmc_blocksize_mask);
	if (device_info_sz == UINT_MAX)
	{
		dprintf(CRITICAL, "ERROR: Incorrect blocksize of card\n");
		return;
	}

	if (devinfo_present)
		ret = mmc_read(ptn, (void *)info_buf, device_info_sz);
	else
		ret = mmc_read((ptn + size - device_info_sz), (void *)info_buf, device_info_sz);
	if (ret)
	{
		dprintf(CRITICAL, "ERROR: Cannot read device info\n");
		ASSERT(0);
	}
}

void write_device_info_flash(device_info *dev)
{
	struct device_info *info = memalign(PAGE_SIZE, ROUNDUP(BOOT_IMG_MAX_PAGE_SIZE, PAGE_SIZE));
	struct ptentry *ptn;
	struct ptable *ptable;
	if(info == NULL)
	{
		dprintf(CRITICAL, "Failed to allocate memory for device info struct\n");
		ASSERT(0);
	}
	info_buf = info;
	ptable = flash_get_ptable();
	if (ptable == NULL)
	{
		dprintf(CRITICAL, "ERROR: Partition table not found\n");
		return;
	}

	ptn = ptable_find(ptable, "devinfo");
	if (ptn == NULL)
	{
		dprintf(CRITICAL, "ERROR: No devinfo partition found\n");
			return;
	}

	memset(info, 0, BOOT_IMG_MAX_PAGE_SIZE);
	memcpy(info, dev, sizeof(device_info));

	if (flash_write(ptn, 0, (void *)info_buf, page_size))
	{
		dprintf(CRITICAL, "ERROR: Cannot write device info\n");
			return;
	}
	free(info);
}

static int read_allow_oem_unlock(device_info *dev)
{
	unsigned offset;
	int index;
	unsigned long long ptn;
	unsigned long long ptn_size;
	unsigned blocksize = mmc_get_device_blocksize();
	STACKBUF_DMA_ALIGN(buf, blocksize);

	index = partition_get_index(frp_ptns[0]);
	if (index == INVALID_PTN)
	{
		index = partition_get_index(frp_ptns[1]);
		if (index == INVALID_PTN)
		{
			dprintf(CRITICAL, "Neither '%s' nor '%s' partition found\n", frp_ptns[0],frp_ptns[1]);
			return -1;
		}
	}

	ptn = partition_get_offset(index);
	ptn_size = partition_get_size(index);
	offset = ptn_size - blocksize;

	/* Set Lun for partition */
	mmc_set_lun(partition_get_lun(index));

	if (mmc_read(ptn + offset, (void *)buf, blocksize))
	{
		dprintf(CRITICAL, "Reading MMC failed\n");
		return -1;
	}

	/*is_allow_unlock is a bool value stored at the LSB of last byte*/
	is_allow_unlock = buf[blocksize-1] & 0x01;

	if (is_allow_unlock)
		dprintf(INFO, "is_allow_unlock is true\n");
	else
		dprintf(CRITICAL, "is_allow_unlock is false\n");

	return 0;
}

static int write_allow_oem_unlock(bool allow_unlock)
{
	unsigned offset;
	int index;
	unsigned long long ptn;
	unsigned long long ptn_size;
	unsigned blocksize = mmc_get_device_blocksize();
	STACKBUF_DMA_ALIGN(buf, blocksize);

	index = partition_get_index(frp_ptns[0]);
	if (index == INVALID_PTN)
	{
		index = partition_get_index(frp_ptns[1]);
		if (index == INVALID_PTN)
		{
			dprintf(CRITICAL, "Neither '%s' nor '%s' partition found\n", frp_ptns[0],frp_ptns[1]);
			return -1;
		}
	}

	ptn = partition_get_offset(index);
	ptn_size = partition_get_size(index);
	offset = ptn_size - blocksize;
	mmc_set_lun(partition_get_lun(index));

	if (mmc_read(ptn + offset, (void *)buf, blocksize))
	{
		dprintf(CRITICAL, "Reading MMC failed\n");
		return -1;
	}

	/*is_allow_unlock is a bool value stored at the LSB of last byte*/
	buf[blocksize-1] = allow_unlock;
	if (mmc_write(ptn + offset, blocksize, buf))
	{
		dprintf(CRITICAL, "Writing MMC failed\n");
		return -1;
	}

	return 0;
}

void read_device_info_flash(device_info *dev)
{
	struct device_info *info = memalign(PAGE_SIZE, ROUNDUP(BOOT_IMG_MAX_PAGE_SIZE, PAGE_SIZE));
	struct ptentry *ptn;
	struct ptable *ptable;
	if(info == NULL)
	{
		dprintf(CRITICAL, "Failed to allocate memory for device info struct\n");
		ASSERT(0);
	}
	info_buf = info;
	ptable = flash_get_ptable();
	if (ptable == NULL)
	{
		dprintf(CRITICAL, "ERROR: Partition table not found\n");
		return;
	}

	ptn = ptable_find(ptable, "devinfo");
	if (ptn == NULL)
	{
		dprintf(CRITICAL, "ERROR: No devinfo partition found\n");
			return;
	}

	if (flash_read(ptn, 0, (void *)info_buf, page_size))
	{
		dprintf(CRITICAL, "ERROR: Cannot write device info\n");
			return;
	}

	if (memcmp(info->magic, DEVICE_MAGIC, DEVICE_MAGIC_SIZE))
	{
		memcpy(info->magic, DEVICE_MAGIC, DEVICE_MAGIC_SIZE);
		info->is_unlocked = 0;
		info->is_tampered = 0;
		write_device_info_flash(info);
	}
	memcpy(dev, info, sizeof(device_info));
	free(info);
}

void write_device_info(device_info *dev)
{
	if(target_is_emmc_boot())
	{
		struct device_info *info = memalign(PAGE_SIZE, ROUNDUP(BOOT_IMG_MAX_PAGE_SIZE, PAGE_SIZE));
		if(info == NULL)
		{
			dprintf(CRITICAL, "Failed to allocate memory for device info struct\n");
			ASSERT(0);
		}
		info_buf = info;
		memcpy(info, dev, sizeof(struct device_info));

#if USE_RPMB_FOR_DEVINFO
		if (VB_M <= target_get_vb_version() &&
			is_secure_boot_enable()) {
				if((write_device_info_rpmb((void*) info, PAGE_SIZE)) < 0)
					ASSERT(0);
		}
		else
			write_device_info_mmc(info);
#else
		dprintf(CRITICAL, "write_device_info_mmc\n");
		write_device_info_mmc(info);
#endif
		free(info);
	}
	else
	{
		write_device_info_flash(dev);
	}
}

int read_rollback_index(uint32_t loc, uint64_t *roll_back_index)
{
        if (!devinfo_present) {
                dprintf(CRITICAL, "DeviceInfo not initalized \n");
                return -EINVAL;
        }
        if (loc >= ARRAY_SIZE(device.rollback_index)) {
                dprintf(CRITICAL, "%s() Loc out of range index: %d, array len: %d\n",
                                __func__, loc, ARRAY_SIZE(device.rollback_index));
                ASSERT(0);
        }

        *roll_back_index = device.rollback_index[loc];
        return 0;
}

int write_rollback_index(uint32_t loc, uint64_t roll_back_index)
{
        if (!devinfo_present) {
                dprintf(CRITICAL, "DeviceInfo not initalized \n");
                return -EINVAL;
        }
        if (loc >= ARRAY_SIZE(device.rollback_index)) {
                dprintf(CRITICAL, "%s() Loc out of range index: %d, array len: %d\n",
                                __func__, loc, ARRAY_SIZE(device.rollback_index));
                ASSERT(0);
        }

        device.rollback_index[loc] = roll_back_index;
        write_device_info(&device);
        return 0;
}

int store_userkey(uint8_t *user_key, uint32_t user_key_size)
{
        if (!devinfo_present) {
                dprintf(CRITICAL, "DeviceInfo not initalized \n");
                return -EINVAL;
        }

        if (user_key_size > ARRAY_SIZE(device.user_public_key)) {
                dprintf(CRITICAL, "StoreUserKey, UserKeySize too large!\n");
                return -ENODEV;
        }

        memcpy(device.user_public_key, user_key, user_key_size);
        device.user_public_key_length = user_key_size;
        write_device_info(&device);
        return 0;
}

int erase_userkey()
{
        if (!devinfo_present) {
                dprintf(CRITICAL, "DeviceInfo not initalized \n");
                return -EINVAL;
        }
        memset(device.user_public_key, 0, ARRAY_SIZE(device.user_public_key));
        device.user_public_key_length = 0;
        write_device_info(&device);
        return 0;
}


int get_userkey(uint8_t **user_key, uint32_t *user_key_size)
{
        if (!devinfo_present) {
                dprintf(CRITICAL, "DeviceInfo not initalized \n");
                return -EINVAL;
        }
        *user_key = device.user_public_key;
        *user_key_size = device.user_public_key_length;
        return 0;
}

void read_device_info(device_info *dev)
{
	if(target_is_emmc_boot())
	{
		struct device_info *info = memalign(PAGE_SIZE, ROUNDUP(BOOT_IMG_MAX_PAGE_SIZE, PAGE_SIZE));
		if(info == NULL)
		{
			dprintf(CRITICAL, "Failed to allocate memory for device info struct\n");
			ASSERT(0);
		}
		info_buf = info;

#if USE_RPMB_FOR_DEVINFO
		if (VB_M <= target_get_vb_version() &&
			is_secure_boot_enable()) {
				if((read_device_info_rpmb((void*) info, PAGE_SIZE)) < 0)
					ASSERT(0);
		}
		else
			read_device_info_mmc(info);
#else
		dprintf(CRITICAL, "read_device_info_mmc\n");
		read_device_info_mmc(info);
#endif

		if (memcmp(info->magic, DEVICE_MAGIC, DEVICE_MAGIC_SIZE))
		{
			memcpy(info->magic, DEVICE_MAGIC, DEVICE_MAGIC_SIZE);
			if (is_secure_boot_enable()) {
				info->is_unlocked = 0;
#if VERIFIED_BOOT || VERIFIED_BOOT_2
				if (VB_M <= target_get_vb_version())
					info->is_unlock_critical = 0;
#endif
			} else {
				info->is_unlocked = 1;
#if VERIFIED_BOOT || VERIFIED_BOOT_2
				if (VB_M <= target_get_vb_version())
					info->is_unlock_critical = 1;
#endif
			}
#if (DEFAULT_UNLOCK)
			info->is_unlocked = 1;
			info->is_unlock_critical = 1;
#endif
			info->is_tampered = 0;
			info->charger_screen_enabled = 0;
#if VERIFIED_BOOT || VERIFIED_BOOT_2
			if (VB_M <= target_get_vb_version())
				info->verity_mode = 1; //enforcing by default
#endif

			write_device_info(info);
		}

#if (!DEFAULT_UNLOCK && VERIFIED_BOOT)
#ifndef PROJECT_DAISY
#ifdef PROJECT_VINCE
		if(!Token_verify_SN_e7(info->sig, sizeof(info->sig)))
#else
		if(!Token_verify_SN(info->sig, sizeof(info->sig)))
#endif
		{
			dprintf(CRITICAL, "Token_verify_SN success\n");
			info->is_unlocked = 1;
			info->is_unlock_critical = 1;
		}
		else
		{
			dprintf(CRITICAL, "Token_verify_SN fail\n");
			info->is_unlocked = 0;
			info->is_unlock_critical = 0;
		}
#endif        
#else//DEFAULT_UNLOCK && VERIFIED_BOOT
#if (DEFAULT_UNLOCK)
			info->is_unlocked = 1;
			info->is_unlock_critical = 1;
	#ifdef HQ_BUILD_FLAG
		info->verity_mode = 1; //enforcing by default
	#endif
#endif//DEFAULT_UNLOCK
#endif//DEFAULT_UNLOCK && VERIFIED_BOOT
		write_device_info(info);

		memcpy(dev, info, sizeof(device_info));
		free(info);
	}
	else
	{
		read_device_info_flash(dev);
	}
}

void reset_device_info()
{
	dprintf(ALWAYS, "reset_device_info called.");
	device.is_tampered = 0;
	write_device_info(&device);
}

void set_device_root()
{
	dprintf(ALWAYS, "set_device_root called.");
	device.is_tampered = 1;
	write_device_info(&device);
}

/* set device unlock value
 * Must check FRP before call this function
 * Need to wipe data when unlock status changed
 * type 0: oem unlock
 * type 1: unlock critical
 * status 0: unlock as false
 * status 1: lock as true
 */
void set_device_unlock_value(int type, bool status)
{
	if (type == UNLOCK)
		device.is_unlocked = status;
#if VERIFIED_BOOT || VERIFIED_BOOT_2
	else if (type == UNLOCK_CRITICAL)
			device.is_unlock_critical = status;
#endif
	write_device_info(&device);
}

static void set_device_unlock(int type, bool status)
{
	int is_unlocked = -1;
	char response[MAX_RSP_SIZE];

	/* check device unlock status if it is as expected */
	if (type == UNLOCK)
		is_unlocked = device.is_unlocked;

#if VERIFIED_BOOT || VERIFIED_BOOT_2
	//if(VB_M <= target_get_vb_version() &&
		if(type == UNLOCK_CRITICAL)

	{
			is_unlocked = device.is_unlock_critical;
	}
#endif
	if (is_unlocked == status) {
		snprintf(response, sizeof(response), "\tDevice already : %s", (status ? "unlocked!" : "locked!"));
		fastboot_info(response);
		fastboot_okay("devcie already lock");
		return;
	}
#if 0
	/* status is true, it means to unlock device */
	if (status && !is_allow_unlock) {
		fastboot_fail("oem unlock is not allowed");
		return;
	}

#if FBCON_DISPLAY_MSG
        dprintf(CRITICAL,"FBCON_DISPLAY_MSG is true");
	display_unlock_menu(type, status);
	fastboot_okay("");
	return;
#else
        dprintf(CRITICAL,"FBCON_DISPLAY_MSG is false");
	if (status && type == UNLOCK) {
		fastboot_fail("Need wipe userdata. Do 'fastboot oem unlock-go'");
		return;
	}
#endif
#endif
	set_device_unlock_value(type, status);
	fastboot_okay("");
//#if PROJECT_SAKURA
#if (PROJECT_SAKURA || PROJECT_DAISY)
    //   #if 0
    if (is_vb_le_enabled()) {
		fastboot_okay("");
		fastboot_okay("is_vb_le_enabled,will not erase userdata");
	} else
	    {
	    if(type == UNLOCK_CRITICAL){
        	/* wipe data */
        	struct recovery_message msg;
        	memset(&msg, 0, sizeof(msg));
        	snprintf(msg.recovery, sizeof(msg.recovery), "recovery\n--wipe_data");
        	write_misc(0, &msg, sizeof(msg));

                fastboot_okay("then erase userdata");
                reboot_device(RECOVERY_MODE);
        	    }
	    }
#endif
}

static bool critical_flash_allowed(const char * entry)
{
	uint32_t i = 0;
	if (entry == NULL)
		return false;

	for (i = 0; i < ARRAY_SIZE(critical_flash_allowed_ptn); i++) {
		if(!strcmp(entry, critical_flash_allowed_ptn[i]))
			return true;
	}
	return false;
}

#if DEVICE_TREE
int copy_dtb(uint8_t *boot_image_start, unsigned int scratch_offset)
{
	uint32 dt_image_offset = 0;
	uint32_t n;
	struct dt_table *table = NULL;
	struct dt_entry dt_entry;
	uint32_t dt_hdr_size = 0;
	unsigned int compressed_size = 0;
	unsigned int dtb_size = 0;
	unsigned int out_avai_len = 0;
	unsigned char *out_addr = NULL;
	unsigned char *best_match_dt_addr = NULL;
	int rc;

	boot_img_hdr *hdr = (boot_img_hdr *) (boot_image_start);

#ifndef OSVERSION_IN_BOOTIMAGE
	dt_size = hdr->dt_size;
#endif

	if(dt_size != 0) {
		/* add kernel offset */
		dt_image_offset += page_size;
		n = ROUND_TO_PAGE(hdr->kernel_size, page_mask);
		dt_image_offset += n;

		/* add ramdisk offset */
		n = ROUND_TO_PAGE(hdr->ramdisk_size, page_mask);
		dt_image_offset += n;

		/* add second offset */
		if(hdr->second_size != 0) {
			n = ROUND_TO_PAGE(hdr->second_size, page_mask);
			dt_image_offset += n;
		}

		/* offset now point to start of dt.img */
		table = (struct dt_table*)(boot_image_start + dt_image_offset);

		if (dev_tree_validate(table, hdr->page_size, &dt_hdr_size) != 0) {
			dprintf(CRITICAL, "ERROR: Cannot validate Device Tree Table \n");
			return -1;
		}

		/* Its Error if, dt_hdr_size (table->num_entries * dt_entry size + Dev_Tree Header)
		goes beyound hdr->dt_size*/
		if (dt_hdr_size > ROUND_TO_PAGE(dt_size,hdr->page_size)) {
			dprintf(CRITICAL, "ERROR: Invalid Device Tree size \n");
			return -1;
		}

		/* Find index of device tree within device tree table */
		if(dev_tree_get_entry_info(table, &dt_entry) != 0){
			dprintf(CRITICAL, "ERROR: Getting device tree address failed\n");
			return -1;
		}

		best_match_dt_addr = (unsigned char *)boot_image_start + dt_image_offset + dt_entry.offset;
		if (is_gzip_package(best_match_dt_addr, dt_entry.size))
		{
			out_addr = (unsigned char *)target_get_scratch_address() + scratch_offset;
			out_avai_len = target_get_max_flash_size() - scratch_offset;
			dprintf(INFO, "decompressing dtb: start\n");
			rc = decompress(best_match_dt_addr,
					dt_entry.size, out_addr, out_avai_len,
					&compressed_size, &dtb_size);
			if (rc)
			{
				dprintf(CRITICAL, "decompressing dtb failed!!!\n");
				ASSERT(0);
			}

			dprintf(INFO, "decompressing dtb: done\n");
			best_match_dt_addr = out_addr;
		} else {
			dtb_size = dt_entry.size;
		}
		/* Validate and Read device device tree in the "tags_add */
		if (check_aboot_addr_range_overlap(hdr->tags_addr, dtb_size) ||
			check_ddr_addr_range_bound(hdr->tags_addr, dtb_size))
		{
			dprintf(CRITICAL, "Device tree addresses are not valid.\n");
			return -1;
		}

		/* Read device device tree in the "tags_add */
		memmove((void*) hdr->tags_addr, (void *)best_match_dt_addr, dtb_size);
	} else
		return -1;

	/* Everything looks fine. Return success. */
	return 0;
}
#endif

void cmd_boot(const char *arg, void *data, unsigned sz)
{
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	unsigned second_actual;
	uint32_t image_actual;
	uint32_t dt_actual = 0;
	boot_img_hdr *hdr = NULL;
	struct kernel64_hdr *kptr = NULL;
	char *ptr = ((char*) data);
	int ret = 0;
	uint8_t dtb_copied = 0;
	unsigned int out_len = 0;
	unsigned int out_avai_len = 0;
	unsigned char *out_addr = NULL;
	uint32_t dtb_offset = 0;
	unsigned char *kernel_start_addr = NULL;
	unsigned int kernel_size = 0;
	unsigned int scratch_offset = 0;
#if !VERIFIED_BOOT_2
	uint32_t sig_actual = 0;
	uint32_t sig_size = 0;
#ifdef MDTP_SUPPORT
        static bool is_mdtp_activated = 0;
#endif /* MDTP_SUPPORT */
#endif

#if FBCON_DISPLAY_MSG
	/* Exit keys' detection thread firstly */
	exit_menu_keys_detection();
#endif

#if VERIFIED_BOOT || VERIFIED_BOOT_2
	if(target_build_variant_user() && !device.is_unlocked)
	{
		fastboot_fail("unlock device to use this command");
		goto boot_failed;
	}
#endif

	if (sz < sizeof(hdr)) {
		fastboot_fail("invalid bootimage header");
		goto boot_failed;
	}

	hdr = (boot_img_hdr *)data;

	/* ensure commandline is terminated */
	hdr->cmdline[BOOT_ARGS_SIZE-1] = 0;

	if(target_is_emmc_boot() && hdr->page_size) {
		page_size = hdr->page_size;
		page_mask = page_size - 1;
	}

	kernel_actual = ROUND_TO_PAGE(hdr->kernel_size, page_mask);
	ramdisk_actual = ROUND_TO_PAGE(hdr->ramdisk_size, page_mask);
	second_actual = ROUND_TO_PAGE(hdr->second_size, page_mask);
#if DEVICE_TREE
#ifndef OSVERSION_IN_BOOTIMAGE
	dt_size = hdr->dt_size;
#endif
	dt_actual = ROUND_TO_PAGE(dt_size, page_mask);
#endif

	image_actual = ADD_OF(page_size, kernel_actual);
	image_actual = ADD_OF(image_actual, ramdisk_actual);
	image_actual = ADD_OF(image_actual, second_actual);
	image_actual = ADD_OF(image_actual, dt_actual);

	/* Checking to prevent oob access in read_der_message_length */
	if (image_actual > sz) {
		fastboot_fail("bootimage header fields are invalid");
		goto boot_failed;
	}
#if VERIFIED_BOOT_2
	memset(&info, 0, sizeof(bootinfo));
	info.images[0].image_buffer = data;
	info.images[0].imgsize = image_actual;
	info.images[0].name = "boot";
	info.num_loaded_images = 1;
	info.multi_slot_boot = partition_multislot_is_supported();
	if (load_image_and_auth(&info))
		goto boot_failed;
	vbcmdline = info.vbcmdline;
#else
	sig_size = sz - image_actual;

	if (target_use_signed_kernel() && (!device.is_unlocked)) {
		unsigned chk;
		/* Calculate the signature length from boot image */
		sig_actual = read_der_message_length(
				(unsigned char*)(data + image_actual), sig_size);
		chk = ADD_OF(image_actual, sig_actual);

		if (chk > sz) {
			fastboot_fail("bootimage header fields are invalid");
			goto boot_failed;
		}
	}

	// Initialize boot state before trying to verify boot.img
#if VERIFIED_BOOT
	boot_verifier_init();
#endif
	/* Handle overflow if the input image size is greater than
	 * boot image buffer can hold
	 */
	if ((target_get_max_flash_size() - page_size) < image_actual)
	{
		fastboot_fail("booimage: size is greater than boot image buffer can hold");
		goto boot_failed;
	}

	/* Verify the boot image
	 * device & page_size are initialized in aboot_init
	 */
	if (target_use_signed_kernel() && (!device.is_unlocked)) {
		/* Pass size excluding signature size, otherwise we would try to
		 * access signature beyond its length
		 */
		verify_signed_bootimg((uint32_t)data, image_actual);
	}
#ifdef MDTP_SUPPORT
	else
	{
		/* fastboot boot is not allowed when MDTP is activated */
		mdtp_ext_partition_verification_t ext_partition;

		if (!is_mdtp_activated) {
			ext_partition.partition = MDTP_PARTITION_NONE;
			mdtp_fwlock_verify_lock(&ext_partition);
		}
	}

	/* If mdtp state cannot be validate, block fastboot boot*/
	if(mdtp_activated(&is_mdtp_activated)){
		dprintf(CRITICAL, "mdtp_activated cannot validate state.\n");
		dprintf(CRITICAL, "Can not proceed with fastboot boot command.\n");
		goto boot_failed;
	}
	if(is_mdtp_activated){
		dprintf(CRITICAL, "fastboot boot command is not available.\n");
		goto boot_failed;
	}
#endif /* MDTP_SUPPORT */
#endif /* VERIFIED_BOOT_2 else */

#if VERIFIED_BOOT
	if (VB_M == target_get_vb_version())
	{
		/* set boot and system versions. */
		set_os_version((unsigned char *)data);
		// send root of trust
		if(!send_rot_command((uint32_t)device.is_unlocked))
			ASSERT(0);
	}
#endif
	/*
	 * Check if the kernel image is a gzip package. If yes, need to decompress it.
	 * If not, continue booting.
	 */
	if (is_gzip_package((unsigned char *)(data + page_size), hdr->kernel_size))
	{
		out_addr = (unsigned char *)target_get_scratch_address();
		out_addr = (unsigned char *)(out_addr + image_actual + page_size);
		out_avai_len = target_get_max_flash_size() - image_actual - page_size;
		dprintf(INFO, "decompressing kernel image: start\n");
		ret = decompress((unsigned char *)(ptr + page_size),
				hdr->kernel_size, out_addr, out_avai_len,
				&dtb_offset, &out_len);
		if (ret)
		{
			dprintf(CRITICAL, "decompressing image failed!!!\n");
			ASSERT(0);
		}

		dprintf(INFO, "decompressing kernel image: done\n");
		kptr = (struct kernel64_hdr *)out_addr;
		kernel_start_addr = out_addr;
		kernel_size = out_len;
	} else {
		kptr = (struct kernel64_hdr*)((char *)data + page_size);
		kernel_start_addr = (unsigned char *)((char *)data + page_size);
		kernel_size = hdr->kernel_size;
	}

	/*
	 * Update the kernel/ramdisk/tags address if the boot image header
	 * has default values, these default values come from mkbootimg when
	 * the boot image is flashed using fastboot flash:raw
	 */
	update_ker_tags_rdisk_addr(hdr, IS_ARM64(kptr));

	/* Get virtual addresses since the hdr saves physical addresses. */
	hdr->kernel_addr = VA(hdr->kernel_addr);
	hdr->ramdisk_addr = VA(hdr->ramdisk_addr);
	hdr->tags_addr = VA(hdr->tags_addr);

	kernel_size  = ROUND_TO_PAGE(kernel_size,  page_mask);
	/* Check if the addresses in the header are valid. */
	if (check_aboot_addr_range_overlap(hdr->kernel_addr, kernel_size) ||
		check_ddr_addr_range_bound(hdr->kernel_addr, kernel_size) ||
		check_aboot_addr_range_overlap(hdr->ramdisk_addr, ramdisk_actual) ||
		check_ddr_addr_range_bound(hdr->ramdisk_addr, ramdisk_actual))
	{
		dprintf(CRITICAL, "kernel/ramdisk addresses are not valid.\n");
		goto boot_failed;
	}

#if DEVICE_TREE
	scratch_offset = image_actual + page_size + out_len;
	/* find correct dtb and copy it to right location */
	ret = copy_dtb(data, scratch_offset);

	dtb_copied = !ret ? 1 : 0;
#else
	if (check_aboot_addr_range_overlap(hdr->tags_addr, MAX_TAGS_SIZE) ||
		check_ddr_addr_range_bound(hdr->tags_addr, MAX_TAGS_SIZE))
	{
		dprintf(CRITICAL, "Tags addresses are not valid.\n");
		goto boot_failed;
	}
#endif

	/* Load ramdisk & kernel */
	memmove((void*) hdr->ramdisk_addr, ptr + page_size + kernel_actual, hdr->ramdisk_size);
	memmove((void*) hdr->kernel_addr, (char*) (kernel_start_addr), kernel_size);

#if DEVICE_TREE
	if (check_aboot_addr_range_overlap(hdr->tags_addr, kernel_actual) ||
		check_ddr_addr_range_bound(hdr->tags_addr, kernel_actual))
	{
		dprintf(CRITICAL, "Tags addresses are not valid.\n");
		goto boot_failed;
	}

	/*
	 * If dtb is not found look for appended DTB in the kernel.
	 * If appended dev tree is found, update the atags with
	 * memory address to the DTB appended location on RAM.
	 * Else update with the atags address in the kernel header
	 */
	if (!dtb_copied) {
		void *dtb;
		dtb = dev_tree_appended((void*)(ptr + page_size),
					hdr->kernel_size, dtb_offset,
					(void *)hdr->tags_addr);
		if (!dtb) {
			fastboot_fail("dtb not found");
			goto boot_failed;
		}
	}
#endif

	fastboot_okay("");
	fastboot_stop();

	boot_linux((void*) hdr->kernel_addr, (void*) hdr->tags_addr,
		   (const char*) hdr->cmdline, board_machtype(),
		   (void*) hdr->ramdisk_addr, hdr->ramdisk_size);

	/* fastboot already stop, it's no need to show fastboot menu */
	return;
boot_failed:
#if FBCON_DISPLAY_MSG
	/* revert to fastboot menu if boot failed */
	display_fastboot_menu();
 #else
         display_fastboot();
#endif
	return;
}

void cmd_erase_nand(const char *arg, void *data, unsigned sz)
{
	struct ptentry *ptn;
	struct ptable *ptable;

	ptable = flash_get_ptable();
	if (ptable == NULL) {
		fastboot_fail("partition table doesn't exist");
		return;
	}

	ptn = ptable_find(ptable, arg);
	if (ptn == NULL) {
		fastboot_fail("unknown partition name");
		return;
	}

	if (!strncmp(arg, "avb_custom_key", strlen("avb_custom_key"))) {
		dprintf(INFO, "erasing avb_custom_key\n");
		if (erase_userkey()) {
			fastboot_fail("Erasing avb_custom_key failed");
		} else {
			fastboot_okay("");
		}
		return;
	}

	if (flash_erase(ptn)) {
		fastboot_fail("failed to erase partition");
		return;
	}
	fastboot_okay("");
}


void cmd_erase_mmc(const char *arg, void *data, unsigned sz)
{
	unsigned long long ptn = 0;
	unsigned long long size = 0;
	int index = INVALID_PTN;
	uint8_t lun = 0;
	char *footer = NULL;

#if VERIFIED_BOOT
	if(!strcmp(arg, KEYSTORE_PTN_NAME))
	{
		if(!device.is_unlocked)
		{
			fastboot_fail("unlock device to erase keystore");
			return;
		}
	}
#endif

	index = partition_get_index(arg);
	ptn = partition_get_offset(index);
	size = partition_get_size(index);

	if (!strncmp(arg, "avb_custom_key", strlen("avb_custom_key"))) {
                dprintf(INFO, "erasing avb_custom_key\n");
                if (erase_userkey()) {
                        fastboot_fail("Erasing avb_custom_key failed");
                } else {
                        fastboot_okay("");
                }
                return;
        }

	if(ptn == 0) {
		fastboot_fail("Partition table doesn't exist\n");
		return;
	}

	lun = partition_get_lun(index);
	mmc_set_lun(lun);

	if (platform_boot_dev_isemmc())
	{
		if (mmc_erase_card(ptn, size)) {
			fastboot_fail("failed to erase partition\n");
			return;
		}
	} else {
		BUF_DMA_ALIGN(out, DEFAULT_ERASE_SIZE);
		size = partition_get_size(index);
		if (size > DEFAULT_ERASE_SIZE)
			size = DEFAULT_ERASE_SIZE;

		/* Simple inefficient version of erase. Just writing
	       0 in first several blocks */
		if (mmc_write(ptn , size, (unsigned int *)out)) {
			fastboot_fail("failed to erase partition");
			return;
		}
		/*Erase FDE metadata at the userdata footer*/
		if(!(strncmp(arg, "userdata", 8)))
		{
			footer = memalign(CACHE_LINE, FOOTER_SIZE);
			memset((void *)footer, 0, FOOTER_SIZE);

			size = partition_get_size(index);

			if (mmc_write((ptn + size) - FOOTER_SIZE , FOOTER_SIZE, (unsigned int *)footer)) {
				fastboot_fail("failed to erase userdata footer");
				free(footer);
				return;
			}
			free(footer);
		}
	}
#if VERIFIED_BOOT || VERIFIED_BOOT_2
	if (VB_M <= target_get_vb_version() &&
		!(strncmp(arg, "userdata", 8)) &&
		send_delete_keys_to_tz())
			ASSERT(0);
#endif
	fastboot_okay("");
}

void cmd_erase(const char *arg, void *data, unsigned sz)
{
#if VERIFIED_BOOT || VERIFIED_BOOT_2
	if (target_build_variant_user())
	{
		if(!device.is_unlocked)
		{
			fastboot_fail("device is locked. Cannot erase");
			return;
		}
	}
#endif
// Xiaomi added begin
#if PROJECT_SAKURA && (!SAKURA_INDIA)
	if (tgt_anti_rb_version < anti_rb_version) {
		dprintf(CRITICAL, "rollback version downgrade\n");
		fastboot_fail("rollback version downgrade");
		return;
	}
#endif
// Xiaomi added end

	if(target_is_emmc_boot())
		cmd_erase_mmc(arg, data, sz);
	else
		cmd_erase_nand(arg, data, sz);
}

/* Get the size from partiton name */
static void get_partition_size(const char *arg, char *response)
{
	uint64_t ptn = 0;
	uint64_t size;
	int index = INVALID_PTN;

	index = partition_get_index(arg);

	if (index == INVALID_PTN)
	{
		dprintf(CRITICAL, "Invalid partition index\n");
		return;
	}

	ptn = partition_get_offset(index);

	if(!ptn)
	{
		dprintf(CRITICAL, "Invalid partition name %s\n", arg);
		return;
	}

	size = partition_get_size(index);

	snprintf(response, MAX_RSP_SIZE, "\t 0x%llx", size);
	return;
}

/* Function to check partition type of a partition*/
static fs_signature_type
check_partition_fs_signature(const char *arg)
{
	fs_signature_type ret = NO_FS;
	int index;
	unsigned long long ptn;
	char *buffer = memalign(CACHE_LINE, mmc_blocksize);
	uint32_t sb_blk_offset = 0;
	char *sb_buffer = buffer;

	if (!sb_buffer)
	{
		dprintf(CRITICAL, "ERROR: Failed to allocate buffer for superblock\n");
		goto out;
	}

	/* Read super block */
	if ((index = partition_get_index(arg)) < 0)
	{
		dprintf(CRITICAL, "ERROR: %s() doesn't exsit\n", arg);
		goto out;
	}
	ptn = partition_get_offset(index);
	mmc_set_lun(partition_get_lun(index));
	sb_blk_offset = (FS_SUPERBLOCK_OFFSET/mmc_blocksize);

	if(mmc_read(ptn + (sb_blk_offset * mmc_blocksize),
				(void *)sb_buffer, mmc_blocksize))
	{
		dprintf(CRITICAL, "ERROR: Failed to read Superblock\n");
		goto out;
	}

	if (sb_blk_offset == 0)
		sb_buffer += FS_SUPERBLOCK_OFFSET;

	if (*((uint16 *)(&sb_buffer[EXT_MAGIC_OFFSET_SB])) == (uint16)EXT_MAGIC)
	{
		dprintf(SPEW, "%s() Found EXT FS\n", arg);
		ret = EXT_FS_SIGNATURE;
	}
	else if (*((uint32 *)(&sb_buffer[F2FS_MAGIC_OFFSET_SB])) == F2FS_MAGIC)
	{
		dprintf(SPEW, "%s() Found F2FS FS\n", arg);
		ret = EXT_F2FS_SIGNATURE;
	}
	else
	{
		dprintf(SPEW, "%s() Reverting to default 0x%x\n",
				arg, *((uint16 *)(&sb_buffer[EXT_MAGIC_OFFSET_SB])));
		ret = NO_FS;
	}

out:
	if(buffer)
		free(buffer);
	return ret;
}

/* Function to get partition type */
static void get_partition_type(const char *arg, char *response)
{
	uint n = 0;
	fs_signature_type fs_signature;

	if (arg == NULL ||
		response == NULL)
	{
		dprintf(CRITICAL, "Invalid input parameter\n");
		return;
	}

	/* By default copy raw to response */
	strlcpy(response, RAW_STR, MAX_RSP_SIZE);

	/* Mark partiton type for known paritions only */
	for (n=0; n < ARRAY_SIZE(part_type_known); n++)
	{
		if (!strncmp(part_type_known[n].part_name, arg, strlen(arg)))
		{
			/* Check partition for FS signature */
			fs_signature = check_partition_fs_signature(arg);
			switch (fs_signature)
			{
				case EXT_FS_SIGNATURE:
					strlcpy(response, EXT_STR, MAX_RSP_SIZE);
					break;
				case EXT_F2FS_SIGNATURE:
					strlcpy(response, F2FS_STR, MAX_RSP_SIZE);
					break;
				case NO_FS:
					strlcpy(response, part_type_known[n].type_response, MAX_RSP_SIZE);
			}
		}
	}
	return;
}

/*
 * Publish the partition type & size info
 * fastboot getvar will publish the required information.
 * fastboot getvar partition_size:<partition_name>: partition size in hex
 * fastboot getvar partition_type:<partition_name>: partition type (ext/fat)
 */
static void publish_getvar_partition_info(struct getvar_partition_info *info, uint8_t num_parts)
{
	uint8_t i;
	static bool published = false;
	struct partition_entry *ptn_entry =
				partition_get_partition_entries();
	memset(info, 0, sizeof(struct getvar_partition_info)* num_parts);

	for (i = 0; i < num_parts; i++) {
		strlcat(info[i].part_name, (const char *)ptn_entry[i].name, MAX_RSP_SIZE);
		strlcat(info[i].getvar_size, "partition-size:", MAX_GET_VAR_NAME_SIZE);
		strlcat(info[i].getvar_type, "partition-type:", MAX_GET_VAR_NAME_SIZE);

		get_partition_type(info[i].part_name, info[i].type_response);
		get_partition_size(info[i].part_name, info[i].size_response);
		if (strlcat(info[i].getvar_size, info[i].part_name, MAX_GET_VAR_NAME_SIZE) >= MAX_GET_VAR_NAME_SIZE)
		{
			dprintf(CRITICAL, "partition size name truncated\n");
			return;
		}
		if (strlcat(info[i].getvar_type, info[i].part_name, MAX_GET_VAR_NAME_SIZE) >= MAX_GET_VAR_NAME_SIZE)
		{
			dprintf(CRITICAL, "partition type name truncated\n");
			return;
		} 

		if (!published)
		{
			/* publish partition size & type info */
			fastboot_publish((const char *) info[i].getvar_size, (const char *) info[i].size_response);
			fastboot_publish((const char *) info[i].getvar_type, (const char *) info[i].type_response);
		}
	}
	if (!published)
		published = true;
}


void cmd_flash_mmc_img(const char *arg, void *data, unsigned sz)
{
	unsigned long long ptn = 0;
	unsigned long long size = 0;
	int index = INVALID_PTN;
	char *token = NULL;
	char *pname = NULL;
	char *sp;
	uint8_t lun = 0;
	bool lun_set = false;
	int current_active_slot = INVALID;

	token = strtok_r((char *)arg, ":", &sp);
	pname = token;
	token = strtok_r(NULL, ":", &sp);
	if(token)
	{
		lun = atoi(token);
		mmc_set_lun(lun);
		lun_set = true;
	}

	if (pname)
	{
		if (!strncmp(pname, "frp-unlock", strlen("frp-unlock")))
		{
			if (!aboot_frp_unlock(pname, data, sz))
			{
				fastboot_info("FRP unlock successful");
				fastboot_okay("");
			}
			else
				fastboot_fail("Secret key is invalid, please update the bootloader with secret key");

			return;
		}

		if (!strcmp(pname, "partition"))
		{
			dprintf(INFO, "Attempt to write partition image.\n");
			if (write_partition(sz, (unsigned char *) data)) {
				fastboot_fail("failed to write partition");
				return;
			}
			/* Re-publish partition table */
			publish_getvar_partition_info(part_info, partition_get_partition_count());

			/* Rescan partition table to ensure we have multislot support*/
			if (partition_scan_for_multislot())
			{
				current_active_slot = partition_find_active_slot();
				dprintf(INFO, "Multislot supported: Slot %s active",
					(SUFFIX_SLOT(current_active_slot)));
			}
			partition_mark_active_slot(current_active_slot);
		}
		else
		{
#if VERIFIED_BOOT
			if(!strcmp(pname, KEYSTORE_PTN_NAME))
			{
				if(!device.is_unlocked)
				{
					fastboot_fail("unlock device to flash keystore");
					return;
				}
				if(!boot_verify_validate_keystore((unsigned char *)data,sz))
				{
					fastboot_fail("image is not a keystore file");
					return;
				}
			}
#endif
			index = partition_get_index(pname);
			ptn = partition_get_offset(index);
			if(ptn == 0) {
				fastboot_fail("partition table doesn't exist");
				return;
			}

			if (!strncmp(pname, "boot", strlen("boot"))
					|| !strcmp(pname, "recovery"))
			{
				if (memcmp((void *)data, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
					fastboot_fail("image is not a boot image");
					return;
				}

				/* Reset multislot_partition attributes in case of flashing boot */
				if (partition_multislot_is_supported())
				{
					partition_reset_attributes(index);
				}
			}

			if(!lun_set)
			{
				lun = partition_get_lun(index);
				mmc_set_lun(lun);
			}

			size = partition_get_size(index);
			if (ROUND_TO_PAGE(sz, mmc_blocksize_mask) > size) {
				fastboot_fail("size too large");
				return;
			}
			else if (mmc_write(ptn , sz, (unsigned int *)data)) {
				fastboot_fail("flash write failure");
				return;
			}
		}
	}
	fastboot_okay("");
	return;
}

void cmd_flash_meta_img(const char *arg, void *data, unsigned sz)
{
	int i, images;
	meta_header_t *meta_header;
	img_header_entry_t *img_header_entry;
	/*End of the image address*/
	uintptr_t data_end;

	if( (UINT_MAX - sz) > (uintptr_t)data )
		data_end  = (uintptr_t)data + sz;
	else
	{
		fastboot_fail("Cannot  flash: image header corrupt");
		return;
	}

	if( data_end < ((uintptr_t)data + sizeof(meta_header_t)))
	{
		fastboot_fail("Cannot  flash: image header corrupt");
		return;
	}

	/* If device is locked:
	 * Forbid to flash image to avoid the device to bypass the image
	 * which with "any" name other than bootloader. Because it maybe
	 * a meta package of all partitions.
	 */
#if VERIFIED_BOOT || VERIFIED_BOOT_2
	if (target_build_variant_user()) {
		if (!device.is_unlocked) {
			fastboot_fail("Device is locked, meta image flashing is not allowed");
			return;
		}

		if (VB_M <= target_get_vb_version() &&
			!device.is_unlock_critical) 
		{
			fastboot_fail("Device is critical locked, Meta image flashing is not allowed");
			return;
		}
	}
#endif

	meta_header = (meta_header_t*) data;
	if( data_end < ((uintptr_t)data + meta_header->img_hdr_sz))
	{
		fastboot_fail("Cannot  flash: image header corrupt");
		return;
	}
	img_header_entry = (img_header_entry_t*) (data+sizeof(meta_header_t));

	images = meta_header->img_hdr_sz / sizeof(img_header_entry_t);

	for (i=0; i<images; i++) {

		if((img_header_entry[i].ptn_name == NULL) ||
			(img_header_entry[i].start_offset == 0) ||
			(img_header_entry[i].size == 0))
			break;
		if ((UINT_MAX - img_header_entry[i].start_offset) < (uintptr_t)data) {
			fastboot_fail("Integer overflow detected in start_offset of img");
			break;
		}
		else if ((UINT_MAX - (img_header_entry[i].start_offset + (uintptr_t)data)) < img_header_entry[i].size) {
			fastboot_fail("Integer overflow detected in size of img");
			break;
		}
		if( data_end < ((uintptr_t)data + img_header_entry[i].start_offset
						+ img_header_entry[i].size) )
		{
			fastboot_fail("Cannot  flash: image size mismatch");
			break;
		}

		cmd_flash_mmc_img(img_header_entry[i].ptn_name,
					(void *) data + img_header_entry[i].start_offset,
					img_header_entry[i].size);
	}

	if (!strncmp(arg, "bootloader", strlen("bootloader")))
	{
		//strlcpy(device.bootloader_version, TARGET(BOARD), MAX_VERSION_LEN);
		//strlcat(device.bootloader_version, "-", MAX_VERSION_LEN);
		//strlcat(device.bootloader_version, meta_header->img_version, MAX_VERSION_LEN);
		strlcpy(device.bootloader_version, (const char *)TARGET(BOOTLOADER_VERSION), MAX_VERSION_LEN);
	}
	else
	{
		strlcpy(device.radio_version, TARGET(BOARD), MAX_VERSION_LEN);
		strlcat(device.radio_version, "-", MAX_VERSION_LEN);
		strlcat(device.radio_version, meta_header->img_version, MAX_VERSION_LEN);
	}

	write_device_info(&device);
	fastboot_okay("");
	return;
}

void cmd_flash_mmc_sparse_img(const char *arg, void *data, unsigned sz)
{
	unsigned int chunk;
	uint64_t chunk_data_sz;
	uint32_t *fill_buf = NULL;
	uint32_t fill_val;
	uint32_t blk_sz_actual = 0;
	sparse_header_t *sparse_header;
	chunk_header_t *chunk_header;
	uint32_t total_blocks = 0;
	unsigned long long ptn = 0;
	unsigned long long size = 0;
	int index = INVALID_PTN;
	uint32_t i;
	uint8_t lun = 0;

	int sparsecrc_index = INVALID_PTN;
	uint32_t sparsecrc = 0;

	/*End of the sparse image address*/
	uintptr_t data_end = (uintptr_t)data + sz;

	index = partition_get_index(arg);
	ptn = partition_get_offset(index);
	if(ptn == 0) {
		fastboot_fail("partition table doesn't exist");
		return;
	}

	size = partition_get_size(index);

	lun = partition_get_lun(index);
	mmc_set_lun(lun);

	if (sz < sizeof(sparse_header_t)) {
		fastboot_fail("size too low");
		return;
	}

	/* Read and skip over sparse image header */
	sparse_header = (sparse_header_t *) data;

	if (!sparse_header->blk_sz || (sparse_header->blk_sz % 4)){
		fastboot_fail("Invalid block size\n");
		return;
	}

	if (((uint64_t)sparse_header->total_blks * (uint64_t)sparse_header->blk_sz) > size) {
		fastboot_fail("size too large");
		return;
	}

	if (sparsecrc_loaded) {
		sparsecrc_index = get_sparsecrc_index(arg);
		sparsecrc = get_init_sparsecrcvalue(sparsecrc_index);
	}

	data += sizeof(sparse_header_t);

	if (data_end < (uintptr_t)data) {
		fastboot_fail("buffer overreads occured due to invalid sparse header");
		return;
	}

	if(sparse_header->file_hdr_sz != sizeof(sparse_header_t))
	{
		fastboot_fail("sparse header size mismatch");
		return;
	}

	dprintf (SPEW, "=== Sparse Image Header ===\n");
	dprintf (SPEW, "magic: 0x%x\n", sparse_header->magic);
	dprintf (SPEW, "major_version: 0x%x\n", sparse_header->major_version);
	dprintf (SPEW, "minor_version: 0x%x\n", sparse_header->minor_version);
	dprintf (SPEW, "file_hdr_sz: %d\n", sparse_header->file_hdr_sz);
	dprintf (SPEW, "chunk_hdr_sz: %d\n", sparse_header->chunk_hdr_sz);
	dprintf (SPEW, "blk_sz: %d\n", sparse_header->blk_sz);
	dprintf (SPEW, "total_blks: %d\n", sparse_header->total_blks);
	dprintf (SPEW, "total_chunks: %d\n", sparse_header->total_chunks);

	/* Start processing chunks */
	for (chunk=0; chunk<sparse_header->total_chunks; chunk++)
	{
		/* Make sure the total image size does not exceed the partition size */
		if(((uint64_t)total_blocks * (uint64_t)sparse_header->blk_sz) >= size) {
			fastboot_fail("size too large");
			return;
		}
		/* Read and skip over chunk header */
		chunk_header = (chunk_header_t *) data;
		data += sizeof(chunk_header_t);

		if (data_end < (uintptr_t)data) {
			fastboot_fail("buffer overreads occured due to invalid sparse header");
			return;
		}

		dprintf (SPEW, "=== Chunk Header ===\n");
		dprintf (SPEW, "chunk_type: 0x%x\n", chunk_header->chunk_type);
		dprintf (SPEW, "chunk_data_sz: 0x%x\n", chunk_header->chunk_sz);
		dprintf (SPEW, "total_size: 0x%x\n", chunk_header->total_sz);

		if(sparse_header->chunk_hdr_sz != sizeof(chunk_header_t))
		{
			fastboot_fail("chunk header size mismatch");
			return;
		}

		chunk_data_sz = (uint64_t)sparse_header->blk_sz * chunk_header->chunk_sz;

		/* Make sure that the chunk size calculated from sparse image does not
		 * exceed partition size
		 */
		if ((uint64_t)total_blocks * (uint64_t)sparse_header->blk_sz + chunk_data_sz > size)
		{
			fastboot_fail("Chunk data size exceeds partition size");
			return;
		}

		switch (chunk_header->chunk_type)
		{
			case CHUNK_TYPE_RAW:
			if((uint64_t)chunk_header->total_sz != ((uint64_t)sparse_header->chunk_hdr_sz +
											chunk_data_sz))
			{
				fastboot_fail("Bogus chunk size for chunk type Raw");
				return;
			}

			if (data_end < (uintptr_t)data + chunk_data_sz) {
				fastboot_fail("buffer overreads occured due to invalid sparse header");
				return;
			}

			/* chunk_header->total_sz is uint32,So chunk_data_sz is now less than 2^32
			   otherwise it will return in the line above
			 */
			if(mmc_write(ptn + ((uint64_t)total_blocks*sparse_header->blk_sz),
						(uint32_t)chunk_data_sz,
						(unsigned int*)data))
			{
				fastboot_fail("flash write failure");
				return;
			}
			if(total_blocks > (UINT_MAX - chunk_header->chunk_sz)) {
				fastboot_fail("Bogus size for RAW chunk type");
				return;
			}

			if (sparsecrc_loaded && sparsecrc_index>=0)
			{
				sparsecrc = simple_crc32(sparsecrc, (const char *)data, chunk_data_sz);
			}

			total_blocks += chunk_header->chunk_sz;
			data += (uint32_t)chunk_data_sz;
			break;

			case CHUNK_TYPE_FILL:
			if(chunk_header->total_sz != (sparse_header->chunk_hdr_sz +
											sizeof(uint32_t)))
			{
				fastboot_fail("Bogus chunk size for chunk type FILL");
				return;
			}

			blk_sz_actual = ROUNDUP(sparse_header->blk_sz, CACHE_LINE);
			/* Integer overflow detected */
			if (blk_sz_actual < sparse_header->blk_sz)
			{
				fastboot_fail("Invalid block size");
				return;
			}

			fill_buf = (uint32_t *)memalign(CACHE_LINE, blk_sz_actual);
			if (!fill_buf)
			{
				fastboot_fail("Malloc failed for: CHUNK_TYPE_FILL");
				return;
			}

			if (data_end < (uintptr_t)data + sizeof(uint32_t)) {
				fastboot_fail("buffer overreads occured due to invalid sparse header");
				free(fill_buf);
				return;
			}
			fill_val = *(uint32_t *)data;
			data = (char *) data + sizeof(uint32_t);

			for (i = 0; i < (sparse_header->blk_sz / sizeof(fill_val)); i++)
			{
				fill_buf[i] = fill_val;
			}

			if(total_blocks > (UINT_MAX - chunk_header->chunk_sz))
			{
				fastboot_fail("bogus size for chunk FILL type");
				free(fill_buf);
				return;
			}

			for (i = 0; i < chunk_header->chunk_sz; i++)
			{
				/* Make sure that the data written to partition does not exceed partition size */
				if ((uint64_t)total_blocks * (uint64_t)sparse_header->blk_sz + sparse_header->blk_sz > size)
				{
					fastboot_fail("Chunk data size for fill type exceeds partition size");
					free(fill_buf);
					return;
				}

				if(mmc_write(ptn + ((uint64_t)total_blocks*sparse_header->blk_sz),
							sparse_header->blk_sz,
							fill_buf))
				{
					fastboot_fail("flash write failure");
					free(fill_buf);
					return;
				}

				if (sparsecrc_loaded && sparsecrc_index>=0)
				{
					sparsecrc = simple_crc32(sparsecrc, (const char *)fill_buf, sparse_header->blk_sz);
				}

				total_blocks++;
			}

			free(fill_buf);
			break;

			case CHUNK_TYPE_DONT_CARE:
			if(total_blocks > (UINT_MAX - chunk_header->chunk_sz)) {
				fastboot_fail("bogus size for chunk DONT CARE type");
				return;
			}
			total_blocks += chunk_header->chunk_sz;
			break;

			case CHUNK_TYPE_CRC:
			if(chunk_header->total_sz != sparse_header->chunk_hdr_sz  + sizeof(uint32_t))
			{
				fastboot_fail("Bogus chunk size for chunk type CRC");
				return;
			}
			if(total_blocks > (UINT_MAX - chunk_header->chunk_sz)) {
				fastboot_fail("bogus size for chunk CRC type");
				return;
			}
			total_blocks += chunk_header->chunk_sz;
			if ((uintptr_t)data > UINT_MAX - chunk_data_sz) {
				fastboot_fail("integer overflow occured");
				return;
			}
			data += (uint32_t)chunk_data_sz;
			if (data_end < (uintptr_t)data) {
				fastboot_fail("buffer overreads occured due to invalid sparse header");
				return;
			}
			break;

			default:
			dprintf(CRITICAL, "Unkown chunk type: %x\n",chunk_header->chunk_type);
			fastboot_fail("Unknown chunk type");
			return;
		}
	}

	dprintf(INFO, "Wrote %d blocks, expected to write %d blocks\n",
					total_blocks, sparse_header->total_blks);

	if(total_blocks != sparse_header->total_blks)
	{
		fastboot_fail("sparse image write failure");
	}

	if (sparsecrc_loaded && sparsecrc_index>=0) {
		// result: 0=>crc success; 1=>crc fail;
		int result = check_sparsecrc(sparsecrc_index, sparsecrc);
		if (result == 1) {
			fastboot_fail("failed to check sparse crc");
			return;
		}
	}

	fastboot_okay("");
	return;
}

void cmd_flash_mmc(const char *arg, void *data, unsigned sz)
{
	sparse_header_t *sparse_header;
	meta_header_t *meta_header;

	if (strcmp(arg, "crclist") == 0) {
		char *tmp = data;
		tmp[sz] = '\0';
		if (update_crc_list(data) == 0) {
			dprintf (SPEW, "Loaded crclist\n");
			fastboot_okay("");
		} else
			fastboot_fail("update crc list failed");
		return;
	} else if (strcmp(arg, "sparsecrclist") == 0) {
		char *tmp = data;
		tmp[sz] = '\0';
		if (update_sparsecrc_list(data) == 0) {
			dprintf (SPEW, "Loaded sparsecrclist\n");
			fastboot_okay("");
		} else
			fastboot_fail("update sparse crc list failed");
		return;
	// Xiaomi added begin
#if PROJECT_SAKURA && (!SAKURA_INDIA)
	} else if (strcmp(arg, "antirbpass") == 0) {
		char *tmp = data;
		if (update_tgt_anti_rb_version(tmp[0]) == 0) {
			dprintf (SPEW, "updated antirbpass\n");
			fastboot_okay("");
		} else {
			fastboot_fail("update antirbpass failed");
		}
		return;
#endif
	// Xiaomi added end
	}

#ifdef SSD_ENABLE
	/* 8 Byte Magic + 2048 Byte xml + Encrypted Data */
	unsigned int *magic_number = (unsigned int *) data;
	int              ret=0;
	uint32           major_version=0;
	uint32           minor_version=0;

	ret = scm_svc_version(&major_version,&minor_version);
	if(!ret)
	{
		if(major_version >= 2)
		{
			if( !strcmp(arg, "ssd") || !strcmp(arg, "tqs") )
			{
				ret = encrypt_scm((uint32 **) &data, &sz);
				if (ret != 0) {
					dprintf(CRITICAL, "ERROR: Encryption Failure\n");
					return;
				}

				/* Protect only for SSD */
				if (!strcmp(arg, "ssd")) {
					ret = scm_protect_keystore((uint32 *) data, sz);
					if (ret != 0) {
						dprintf(CRITICAL, "ERROR: scm_protect_keystore Failed\n");
						return;
					}
				}
			}
			else
			{
				ret = decrypt_scm_v2((uint32 **) &data, &sz);
				if(ret != 0)
				{
					dprintf(CRITICAL,"ERROR: Decryption Failure\n");
					return;
				}
			}
		}
		else
		{
			if (magic_number[0] == DECRYPT_MAGIC_0 &&
			magic_number[1] == DECRYPT_MAGIC_1)
			{
				ret = decrypt_scm((uint32 **) &data, &sz);
				if (ret != 0) {
					dprintf(CRITICAL, "ERROR: Invalid secure image\n");
					return;
				}
			}
			else if (magic_number[0] == ENCRYPT_MAGIC_0 &&
				magic_number[1] == ENCRYPT_MAGIC_1)
			{
				ret = encrypt_scm((uint32 **) &data, &sz);
				if (ret != 0) {
					dprintf(CRITICAL, "ERROR: Encryption Failure\n");
					return;
				}
			}
		}
	}
	else
	{
		dprintf(CRITICAL,"INVALID SVC Version\n");
		return;
	}
#endif /* SSD_ENABLE */

#if VERIFIED_BOOT || VERIFIED_BOOT_2
	if (target_build_variant_user())
	{
		/* if device is locked:
		 * common partition will not allow to be flashed
		 * critical partition will allow to flash image.
		 */
		if(!device.is_unlocked && !critical_flash_allowed(arg)) {
			fastboot_fail("Partition flashing is not allowed");
			return;
		}

		/* if device critical is locked:
		 * common partition will allow to be flashed
		 * critical partition will not allow to flash image.
		 */
		if (VB_M <= target_get_vb_version() &&
			!device.is_unlock_critical &&
			critical_flash_allowed(arg)) {
				fastboot_fail("Critical partition flashing is not allowed");
				return;
		}
	}
#endif
	if (!strncmp(arg, "avb_custom_key", strlen("avb_custom_key"))) {
		dprintf(INFO, "flashing avb_custom_key\n");
		if (store_userkey(data, sz)) {
			fastboot_fail("Flashing avb_custom_key failed");
		} else {
			fastboot_okay("");
		}
		return;
	}
// Xiaomi added begin
#if PROJECT_SAKURA && (!SAKURA_INDIA)
	if (tgt_anti_rb_version < anti_rb_version) {
		dprintf(CRITICAL, "rollback version downgrade\n");
		fastboot_fail("rollback version downgrade");
		return;
	}
#endif
// Xiaomi added end

	if (crc_loaded == 1) {
		if (check_crc(arg, data, sz) != 0) {
			fastboot_fail("check  crc failed");
			return;
		}
		dprintf(CRITICAL, "CRC verification pass\n");
	}


	sparse_header = (sparse_header_t *) data;
	meta_header = (meta_header_t *) data;
	if (sparse_header->magic == SPARSE_HEADER_MAGIC)
		cmd_flash_mmc_sparse_img(arg, data, sz);
	else if (meta_header->magic == META_HEADER_MAGIC)
		cmd_flash_meta_img(arg, data, sz);
	else
		cmd_flash_mmc_img(arg, data, sz);

	if(!strncmp(arg, "aboot", 5))
	{
		dprintf(CRITICAL, "flash aboot set BOOTLOADER_VERSION\n");
		strlcpy(device.bootloader_version, TARGET(BOARD), MAX_VERSION_LEN);
		strlcat(device.bootloader_version, "-", MAX_VERSION_LEN);
		strlcat(device.bootloader_version, TARGET(BOOTLOADER_VERSION), MAX_VERSION_LEN);
		write_device_info(&device);
	}

#if VERIFIED_BOOT
	if (VB_M <= target_get_vb_version() &&
		(!strncmp(arg, "system", 6)) &&
		!device.verity_mode){
			// reset dm_verity mode to enforcing
			device.verity_mode = 1;
			write_device_info(&device);
		}
#endif

	return;
}

void cmd_updatevol(const char *vol_name, void *data, unsigned sz)
{
	struct ptentry *sys_ptn;
	struct ptable *ptable;

	ptable = flash_get_ptable();
	if (ptable == NULL) {
		fastboot_fail("partition table doesn't exist");
		return;
	}

	sys_ptn = ptable_find(ptable, "system");
	if (sys_ptn == NULL) {
		fastboot_fail("system partition not found");
		return;
	}

	sz = ROUND_TO_PAGE(sz, page_mask);
	if (update_ubi_vol(sys_ptn, vol_name, data, sz))
		fastboot_fail("update_ubi_vol failed");
	else
		fastboot_okay("");
}

void cmd_flash_nand(const char *arg, void *data, unsigned sz)
{
	struct ptentry *ptn;
	struct ptable *ptable;
	unsigned extra = 0;
	uint64_t partition_size = 0;
	unsigned bytes_to_round_page = 0;
	unsigned rounded_size = 0;

	if((uintptr_t)data > (UINT_MAX - sz)) {
		fastboot_fail("Cannot flash: image header corrupt");
                return;
        }

	ptable = flash_get_ptable();
	if (ptable == NULL) {
		fastboot_fail("partition table doesn't exist");
		return;
	}

	ptn = ptable_find(ptable, arg);
	if (ptn == NULL) {
		dprintf(INFO, "unknown partition name (%s). Trying updatevol\n",
				arg);
		cmd_updatevol(arg, data, sz);
		return;
	}

	if (!strncmp(arg, "avb_custom_key", strlen("avb_custom_key"))) {
		dprintf(INFO, "flashing avb_custom_key\n");
		if (store_userkey(data, sz)) {
			fastboot_fail("Flashing avb_custom_key failed");
		} else {
			fastboot_okay("");
		}
		return;
	}

	if (!strcmp(ptn->name, "boot") || !strcmp(ptn->name, "recovery")) {
		if((sz > BOOT_MAGIC_SIZE) && (!memcmp((void *)data, BOOT_MAGIC, BOOT_MAGIC_SIZE))) {
			dprintf(INFO, "Verified the BOOT_MAGIC in image header  \n");
		} else {
			fastboot_fail("Image is not a boot image");
			return;
		}
	}

	if (!strcmp(ptn->name, "system")
		|| !strcmp(ptn->name, "userdata")
		|| !strcmp(ptn->name, "persist")
		|| !strcmp(ptn->name, "recoveryfs")
		|| !strcmp(ptn->name, "modem"))
		extra = 1;
	else {
		rounded_size = ROUNDUP(sz, page_size);
		bytes_to_round_page = rounded_size - sz;
		if (bytes_to_round_page) {
			if (((uintptr_t)data + sz ) > (UINT_MAX - bytes_to_round_page)) {
				fastboot_fail("Integer overflow detected");
				return;
			}
			if (((uintptr_t)data + sz + bytes_to_round_page) >
				((uintptr_t)target_get_scratch_address() + target_get_max_flash_size())) {
				fastboot_fail("Buffer size is not aligned to page_size");
				return;
			}
			else {
				memset(data + sz, 0, bytes_to_round_page);
				sz = rounded_size;
			}
		}
	}

	/*Checking partition_size for the possible integer overflow */
	partition_size = validate_partition_size(ptn);

	if (sz > partition_size) {
		fastboot_fail("Image size too large");
		return;
	}

	dprintf(INFO, "writing %d bytes to '%s'\n", sz, ptn->name);
	if ((sz > UBI_EC_HDR_SIZE) &&
		(!memcmp((void *)data, UBI_MAGIC, UBI_MAGIC_SIZE))) {
		if (flash_ubi_img(ptn, data, sz)) {
			fastboot_fail("flash write failure");
			return;
		}
	} else {
		if (flash_write(ptn, extra, data, sz)) {
			fastboot_fail("flash write failure");
			return;
		}
	}
	dprintf(INFO, "partition '%s' updated\n", ptn->name);
	fastboot_okay("");
}


static inline uint64_t validate_partition_size(struct ptentry *ptn)
{
	if (ptn->length && flash_num_pages_per_blk() && page_size) {
		if ((ptn->length < ( UINT_MAX / flash_num_pages_per_blk())) && ((ptn->length * flash_num_pages_per_blk()) < ( UINT_MAX / page_size))) {
			return ptn->length * flash_num_pages_per_blk() * page_size;
		}
        }
	return 0;
}


void cmd_flash(const char *arg, void *data, unsigned sz)
{
	if(target_is_emmc_boot())
		cmd_flash_mmc(arg, data, sz);
	else
		cmd_flash_nand(arg, data, sz);
}

void cmd_continue(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");
	fastboot_stop();

	if (target_is_emmc_boot())
	{
#if FBCON_DISPLAY_MSG
		/* Exit keys' detection thread firstly */
		exit_menu_keys_detection();
#endif
		boot_linux_from_mmc();
	}
	else
	{
		boot_linux_from_flash();
	}
}

void cmd_reboot(const char *arg, void *data, unsigned sz)
{
	dprintf(INFO, "rebooting the device\n");
	fastboot_okay("");
	//reboot_device(0);
	reboot_device(NORMAL_MODE);
}

void cmd_set_active(const char *arg, void *data, unsigned sz)
{
	char *p, *sp = NULL;
	unsigned i,current_active_slot;
	const char *current_slot_suffix;

	if (!partition_multislot_is_supported())
	{
		fastboot_fail("Command not supported");
		return;
	}

	if (arg)
	{
		p = strtok_r((char *)arg, ":", &sp);
		if (p)
		{
			current_active_slot = partition_find_active_slot();

			/* Check if trying to make curent slot active */
			current_slot_suffix = SUFFIX_SLOT(current_active_slot);
			current_slot_suffix = strtok_r((char *)current_slot_suffix,
							(char *)suffix_delimiter, &sp);

			if (current_slot_suffix &&
				!strncmp(p, current_slot_suffix, strlen(current_slot_suffix)))
			{
				fastboot_okay("Slot already set active");
				return;
			}
			else
			{
				for (i = 0; i < AB_SUPPORTED_SLOTS; i++)
				{
					current_slot_suffix = SUFFIX_SLOT(i);
					current_slot_suffix = strtok_r((char *)current_slot_suffix,
									(char *)suffix_delimiter, &sp);
					if (current_slot_suffix &&
						!strncmp(p, current_slot_suffix, strlen(current_slot_suffix)))
					{
						partition_switch_slots(current_active_slot, i);
						publish_getvar_multislot_vars();
						fastboot_okay("");
						return;
					}
				}
			}
		}
	}
	fastboot_fail("Invalid slot suffix.");
	return;
}

void cmd_reboot_bootloader(const char *arg, void *data, unsigned sz)
{
	dprintf(INFO, "rebooting the device\n");
	fastboot_okay("");
	reboot_device(FASTBOOT_MODE);
}

void cmd_oem_enable_charger_screen(const char *arg, void *data, unsigned size)
{
	dprintf(INFO, "Enabling charger screen check\n");
	device.charger_screen_enabled = 1;
	write_device_info(&device);
	fastboot_okay("");
}

void cmd_oem_disable_charger_screen(const char *arg, void *data, unsigned size)
{
	dprintf(INFO, "Disabling charger screen check\n");
	device.charger_screen_enabled = 0;
	write_device_info(&device);
	fastboot_okay("");
}

void cmd_oem_off_mode_charger(const char *arg, void *data, unsigned size)
{
	char *p = NULL;
	const char *delim = " \t\n\r";
	char *sp;

	if (arg) {
		p = strtok_r((char *)arg, delim, &sp);
		if (p) {
			if (!strncmp(p, "0", 1)) {
				device.charger_screen_enabled = 0;
			} else if (!strncmp(p, "1", 1)) {
				device.charger_screen_enabled = 1;
			}
		}
	}

	/* update charger_screen_enabled value for getvar
	 * command
	 */
	snprintf(charger_screen_enabled, MAX_RSP_SIZE, "%d",
		device.charger_screen_enabled);

	write_device_info(&device);
	fastboot_okay("");
}

void cmd_oem_select_display_panel(const char *arg, void *data, unsigned size)
{
	dprintf(INFO, "Selecting display panel %s\n", arg);
	if (arg)
		strlcpy(device.display_panel, arg,
			sizeof(device.display_panel));
	write_device_info(&device);
	fastboot_okay("");
}

/* Only for test. DON'T build into user build */
void cmd_oem_unlock_test(const char *arg, void *data, unsigned sz)
{
    unlock_test(arg, data, sz, fastboot_info);
    fastboot_okay("");
}

void cmd_oem_unlock(const char *arg, void *data, unsigned sz)
{
	#if (PROJECT_DAISY)
	if(!is_allow_unlock){
		dprintf(CRITICAL,"oem unlock is not allowed ,please open the Setting");
		fastboot_fail("oem unlock is not allowed");
		return ;
	}
#endif 

#ifndef PROJECT_DAISY
	#if (VERIFIED_BOOT && !DEFAULT_UNLOCK)
			#ifdef PROJECT_VINCE
			if (Token_verify_compare_e7(data, sz))
			#else
		if (Token_verify_compare(data, sz))
			#endif
		{
			fastboot_fail("Token verification failed, reboot the device");
			reboot_device(FASTBOOT_MODE);
		}
		#endif
#endif

#ifdef HQ_BUILD_FLAG
	device.verity_mode = 1;
#endif

#if !DEFAULT_UNLOCK
	memcpy(device.sig, data, sizeof(device.sig));
	write_device_info(&device);
#endif

set_device_unlock(UNLOCK, TRUE);
set_device_unlock(UNLOCK_CRITICAL, TRUE);

}

void cmd_oem_edl(const char *arg, void *data, unsigned sz)  //add "fastboot oem edl" cmd into  EMERGENCY_DLOAD mode
{
#ifdef FACTORY_VERSION_ENABLE
	dprintf(INFO, "factory sw ,allow edl anyway.");
	reboot_device(EMERGENCY_DLOAD);
#else
	if(!is_device_locked()) {  //device is locked ,no edl
		dprintf(INFO, "device oem is unlock,fastboot oem edl succes!");
		fastboot_okay("");
		mdelay(200);
		reboot_device(EMERGENCY_DLOAD);
	} else {
		dprintf(INFO, "device oem locked, fastboot oem edl fail!");
		fastboot_fail("device is locked! no edl");
	}
#endif
}

void cmd_oem_unlock_go(const char *arg, void *data, unsigned sz)
{
	if(!device.is_unlocked) {
		if(!is_allow_unlock) {
			fastboot_fail("oem unlock is not allowed");
			return;
		}

		set_device_unlock_value(UNLOCK, TRUE);

		/* wipe data */
		struct recovery_message msg;
	        memset(&msg, 0, sizeof(msg));
		snprintf(msg.recovery, sizeof(msg.recovery), "recovery\n--wipe_data");
		write_misc(0, &msg, sizeof(msg));

		fastboot_okay("");
		reboot_device(RECOVERY_MODE);
	}
	fastboot_okay("");
}

static int aboot_frp_unlock(char *pname, void *data, unsigned sz)
{
	int ret=1;
	bool authentication_success=false;

	/*
		Authentication method not  implemented.

		OEM to implement, authentication system which on successful validataion,
		calls write_allow_oem_unlock() with is_allow_unlock.
	*/
#if 0
	authentication_success = oem_specific_auth_mthd();
#endif

	if (authentication_success)
	{
		is_allow_unlock = true;
		write_allow_oem_unlock(is_allow_unlock);
		ret = 0;
	}
	return ret;
}

void cmd_oem_lock(const char *arg, void *data, unsigned sz)
{
#if (VERIFIED_BOOT && !DEFAULT_UNLOCK)
	memset(device.sig, 0, sizeof(device.sig));
#endif
	set_device_unlock(UNLOCK, FALSE);
	set_device_unlock(UNLOCK_CRITICAL, FALSE);
}

extern int write_misc(unsigned offset, void * buf, unsigned size);
void cmd_oem_ddsorting(const char *arg, void *data, unsigned sz){
	int ret = 0;
	char sorting_buff[]="DDRsorting=1";
	ret = write_misc(1, sorting_buff, strlen(sorting_buff));
	if(0 == ret){
		 fastboot_okay("DDR sorting send success");
	}else{
		fastboot_fail("DDR sorting send fail");
	}
}

void cmd_oem_devinfo(const char *arg, void *data, unsigned sz)
{
	char response[MAX_RSP_SIZE];
	snprintf(response, sizeof(response), "\tDevice tampered: %s", (device.is_tampered ? "true" : "false"));
	fastboot_info(response);
	snprintf(response, sizeof(response), "\tDevice unlocked: %s", (device.is_unlocked ? "true" : "false"));
	fastboot_info(response);
#if VERIFIED_BOOT || VERIFIED_BOOT_2
	if (VB_M <= target_get_vb_version())
	{
		snprintf(response, sizeof(response), "\tDevice critical unlocked: %s",
					(device.is_unlock_critical ? "true" : "false"));
		fastboot_info(response);
	}
#endif
	snprintf(response, sizeof(response), "\tCharger screen enabled: %s", (device.charger_screen_enabled ? "true" : "false"));
	fastboot_info(response);
	snprintf(response, sizeof(response), "\tDisplay panel: %s", (device.display_panel));
	fastboot_info(response);
	fastboot_okay("");
}

void cmd_flashing_get_unlock_ability(const char *arg, void *data, unsigned sz)
{
	char response[MAX_RSP_SIZE];
	snprintf(response, sizeof(response), "\tget_unlock_ability: %d", is_allow_unlock);
	fastboot_info(response);
	fastboot_okay("");
}

void cmd_flashing_lock_critical(const char *arg, void *data, unsigned sz)
{
	set_device_unlock(UNLOCK_CRITICAL, FALSE);
}

void cmd_flashing_unlock_critical(const char *arg, void *data, unsigned sz)
{
	set_device_unlock(UNLOCK_CRITICAL, TRUE);
}

void cmd_preflash(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");
}

static uint8_t logo_header[LOGO_IMG_HEADER_SIZE];

int splash_screen_check_header(logo_img_header *header)
{
	if (memcmp(header->magic, LOGO_IMG_MAGIC, 8))
		return -1;
	if (header->width == 0 || header->height == 0)
		return -1;
	return 0;
}

int splash_screen_flash()
{
	struct ptentry *ptn;
	struct ptable *ptable;
	struct logo_img_header *header;
	struct fbcon_config *fb_display = NULL;

	ptable = flash_get_ptable();
	if (ptable == NULL) {
		dprintf(CRITICAL, "ERROR: Partition table not found\n");
		return -1;
	}

	ptn = ptable_find(ptable, "splash");
	if (ptn == NULL) {
		dprintf(CRITICAL, "ERROR: splash Partition not found\n");
		return -1;
	}
	if (flash_read(ptn, 0, (void *)logo_header, LOGO_IMG_HEADER_SIZE)) {
		dprintf(CRITICAL, "ERROR: Cannot read boot image header\n");
		return -1;
	}

	header = (struct logo_img_header *)logo_header;
	if (splash_screen_check_header(header)) {
		dprintf(CRITICAL, "ERROR: Boot image header invalid\n");
		return -1;
	}

	fb_display = fbcon_display();
	if (fb_display) {
		if (header->type && (header->blocks != 0)) { // RLE24 compressed data
			uint8_t *base = (uint8_t *) fb_display->base + LOGO_IMG_OFFSET;

			/* if the logo is full-screen size, remove "fbcon_clear()" */
			if ((header->width != fb_display->width)
						|| (header->height != fb_display->height))
					fbcon_clear();

			if (flash_read(ptn + LOGO_IMG_HEADER_SIZE, 0,
				(uint32_t *)base,
				(header->blocks * 512))) {
				dprintf(CRITICAL, "ERROR: Cannot read splash image from partition\n");
				return -1;
			}
			fbcon_extract_to_screen(header, base);
			return 0;
		}

		if ((header->width > fb_display->width) || (header->height > fb_display->height)) {
			dprintf(CRITICAL, "Logo config greater than fb config. Fall back default logo\n");
			return -1;
		}

		uint8_t *base = (uint8_t *) fb_display->base;
		uint32_t fb_size = ROUNDUP(fb_display->width *
					fb_display->height *
					(fb_display->bpp / 8), 4096);
		uint32_t splash_size = ((((header->width * header->height *
					fb_display->bpp/8) + 511) >> 9) << 9);

		if (splash_size > fb_size) {
			dprintf(CRITICAL, "ERROR: Splash image size invalid\n");
			return -1;
		}

		if (flash_read(ptn + LOGO_IMG_HEADER_SIZE, 0,
			(uint32_t *)base,
			((((header->width * header->height * fb_display->bpp/8) + 511) >> 9) << 9))) {
			fbcon_clear();
			dprintf(CRITICAL, "ERROR: Cannot read splash image from partition\n");
			return -1;
		}
	}

	return 0;
}

int splash_screen_mmc(int offset)
{
	int index = INVALID_PTN;
	unsigned long long ptn = 0;
	struct fbcon_config *fb_display = NULL;
	struct logo_img_header *header;
	uint32_t blocksize, realsize, readsize;
	uint8_t *base;
      dprintf(0," in splash_screen_mmc offset = %d\n",offset);

	index = partition_get_index("splash");
	if (index == 0) {
		dprintf(CRITICAL, "ERROR: splash Partition table not found\n");
		return -1;
	}

	ptn = partition_get_offset(index);
	if (ptn == 0) {
		dprintf(CRITICAL, "ERROR: splash Partition invalid\n");
		return -1;
	}
	ptn += offset; // add for pit .add  by feng

	mmc_set_lun(partition_get_lun(index));

	blocksize = mmc_get_device_blocksize();
	if (blocksize == 0) {
		dprintf(CRITICAL, "ERROR:splash Partition invalid blocksize\n");
		return -1;
	}

	fb_display = fbcon_display();
	if (!fb_display)
	{
		dprintf(CRITICAL, "ERROR: fb config is not allocated\n");
		return -1;
	}

	base = (uint8_t *) fb_display->base;

	if (mmc_read(ptn, (uint32_t *)(base + LOGO_IMG_OFFSET), blocksize)) {
		dprintf(CRITICAL, "ERROR: Cannot read splash image header\n");
		return -1;
	}

	header = (struct logo_img_header *)(base + LOGO_IMG_OFFSET);
	printf("header: %s\n",header->magic);
	if (splash_screen_check_header(header)) {
		dprintf(CRITICAL, "ERROR: Splash image header invalid\n");
		//return -1;
	}

	if (fb_display) {
		if (header->type && (header->blocks != 0)) { /* 1 RLE24 compressed data */
			base += LOGO_IMG_OFFSET;

			realsize =  header->blocks * 512;
			readsize =  ROUNDUP((realsize + LOGO_IMG_HEADER_SIZE), blocksize) - blocksize;

			/* if the logo is not full-screen size, clean screen */
			if ((header->width != fb_display->width)
						|| (header->height != fb_display->height))
				fbcon_clear();

			uint32_t fb_size = ROUNDUP(fb_display->width *
					fb_display->height *
					(fb_display->bpp / 8), 4096);

			if (readsize > fb_size) {
				dprintf(CRITICAL, "ERROR: Splash image size invalid\n");
				return -1;
			}

			if (mmc_read(ptn + blocksize, (uint32_t *)(base + blocksize), readsize)) {
				dprintf(CRITICAL, "ERROR: Cannot read splash image from partition\n");
				return -1;
			}

			fbcon_extract_to_screen(header, (base + LOGO_IMG_HEADER_SIZE));
		} else { /* 2 Raw BGR data */

			if ((header->width > fb_display->width) || (header->height > fb_display->height)) {
				dprintf(CRITICAL, "Logo config greater than fb config. Fall back default logo\n");
				return -1;
			}

			realsize =  header->width * header->height * fb_display->bpp / 8;
			readsize =  ROUNDUP((realsize + LOGO_IMG_HEADER_SIZE), blocksize) - blocksize;

			if (blocksize == LOGO_IMG_HEADER_SIZE) { /* read the content directly */
				if (mmc_read((ptn + LOGO_IMG_HEADER_SIZE), (uint32_t *)base, readsize)) {
					fbcon_clear();
					dprintf(CRITICAL, "ERROR: Cannot read splash image from partition\n");
					return -1;
				}
			} else {
				if (mmc_read(ptn + blocksize ,
						(uint32_t *)(base + LOGO_IMG_OFFSET + blocksize), readsize)) {
					dprintf(CRITICAL, "ERROR: Cannot read splash image from partition\n");
					return -1;
				}
				memmove(base, (base + LOGO_IMG_OFFSET + LOGO_IMG_HEADER_SIZE), realsize);
			}
		}
	}

	return 0;
}

int fetch_image_from_partition()
{

    // add fot pit .by feng
	if (target_is_emmc_boot()) {
		#if PROJECT_DAISY
			if(boot_into_fastboot)
				return splash_screen_mmc(SPLASH_FASTBOOT_IMG_OFFSET);
		#endif
		#if VERIFIED_BOOT
		if(device.is_unlocked)
		{
			#if PROJECT_DAISY
				return splash_screen_mmc(SPLASH_WARNING_IMG_OFFSET);
			#else
				return splash_screen_mmc(SPLASH_UNLOCK_IMG_OFFSET);
			#endif
		} else {
			return splash_screen_mmc(SPLASH_LOGO_IMG_OFFSET);
		}
		#else
			return splash_screen_mmc(SPLASH_LOGO_IMG_OFFSET);
		#endif
	} else {
		return splash_screen_flash();
	}
}

void publish_getvar_multislot_vars()
{
	int i,count;
	static bool published = false;
	static char slot_count[MAX_RSP_SIZE];
	static struct ab_slot_info slot_info[AB_SUPPORTED_SLOTS];
	static char active_slot_suffix[MAX_RSP_SIZE];
	static char has_slot_pname[NUM_PARTITIONS][MAX_GET_VAR_NAME_SIZE];
	static char has_slot_reply[NUM_PARTITIONS][MAX_RSP_SIZE];
	const char *tmp;
	char tmpbuff[MAX_GET_VAR_NAME_SIZE];
	signed active_slt;

	if (!published)
	{
		/* Update slot meta info */
		count = partition_fill_partition_meta(has_slot_pname, has_slot_reply,
							partition_get_partition_count());
		for(i=0; i<count; i++)
		{
			memset(tmpbuff, 0, MAX_GET_VAR_NAME_SIZE);
			snprintf(tmpbuff, MAX_GET_VAR_NAME_SIZE,"has-slot:%s",
								has_slot_pname[i]);
			strlcpy(has_slot_pname[i], tmpbuff, MAX_GET_VAR_NAME_SIZE);
			fastboot_publish(has_slot_pname[i], has_slot_reply[i]);
		}

		for (i=0; i<AB_SUPPORTED_SLOTS; i++)
		{
			tmp = SUFFIX_SLOT(i);
			tmp++; // to remove "_" from slot_suffix.
			snprintf(slot_info[i].slot_is_unbootable, sizeof(slot_info[i].slot_is_unbootable),
										"slot-unbootable:%s", tmp);
			snprintf(slot_info[i].slot_is_active, sizeof(slot_info[i].slot_is_active),
										"slot-active:%s", tmp);
			snprintf(slot_info[i].slot_is_succesful, sizeof(slot_info[i].slot_is_succesful),
										"slot-success:%s", tmp);
			snprintf(slot_info[i].slot_retry_count, sizeof(slot_info[i].slot_retry_count),
										"slot-retry-count:%s", tmp);
			fastboot_publish(slot_info[i].slot_is_unbootable,
							slot_info[i].slot_is_unbootable_rsp);
			fastboot_publish(slot_info[i].slot_is_active,
							slot_info[i].slot_is_active_rsp);
			fastboot_publish(slot_info[i].slot_is_succesful,
							slot_info[i].slot_is_succesful_rsp);
			fastboot_publish(slot_info[i].slot_retry_count,
							slot_info[i].slot_retry_count_rsp);
		}
		fastboot_publish("current-slot", active_slot_suffix);
		snprintf(slot_count, sizeof(slot_count),"%d", AB_SUPPORTED_SLOTS);
		fastboot_publish("slot-count", slot_count);
		published = true;
	}

	active_slt = partition_find_active_slot();
	if (active_slt != INVALID)
	{
		tmp = SUFFIX_SLOT(active_slt);
		tmp++; // to remove "_" from slot_suffix.
		snprintf(active_slot_suffix, sizeof(active_slot_suffix), "%s", tmp);
	}
	else
		strlcpy(active_slot_suffix, "INVALID", sizeof(active_slot_suffix));

	/* Update partition meta information */
	partition_fill_slot_meta(slot_info);
	return;
}

void get_product_name(unsigned char *buf)
{
	snprintf((char*)buf, MAX_RSP_SIZE, "%s",  TARGET(XIAOMI_PRODUCT));
	return;
}

#if PRODUCT_IOT
void get_bootloader_version_iot(unsigned char *buf)
{
	if (buf != NULL)
	{
		strlcpy(buf, TARGET(BOARD), MAX_VERSION_LEN);
		strlcat(buf, "-", MAX_VERSION_LEN);
		strlcat(buf, PRODUCT_IOT_VERSION, MAX_VERSION_LEN);
	}
	return;
}
#endif

void get_bootloader_version(unsigned char *buf)
{
	snprintf((char*)buf, MAX_RSP_SIZE, "%s", (const char *)TARGET(BOOTLOADER_VERSION));
	return;
}

void get_baseband_version(unsigned char *buf)
{
	snprintf((char*)buf, MAX_RSP_SIZE, "%s", device.radio_version);
	return;
}

bool is_device_locked_critical()
{
        return device.is_unlock_critical ? false:true;
}

bool is_device_locked()
{
	return device.is_unlocked ? false:true;
}

bool is_verity_enforcing()
{
        return device.verity_mode ? true:false;
}

/* register commands and variables for fastboot */
void aboot_fastboot_register_commands(void)
{
	int i;
	char hw_platform_buf[MAX_RSP_SIZE];
	const char * bootloader_version_temp = (const char *)TARGET(BOOTLOADER_VERSION);

	struct fastboot_cmd_desc cmd_list[] = {
						/* By default the enabled list is empty. */
						{"", NULL},
						/* move commands enclosed within the below ifndef to here
						 * if they need to be enabled in user build.
						 */
#ifndef DISABLE_FASTBOOT_CMDS
						/* Register the following commands only for non-user builds */
						{"flash:", cmd_flash},
						{"erase:", cmd_erase},
						{"boot", cmd_boot},
						{"continue", cmd_continue},
						{"reboot", cmd_reboot},
						{"reboot-bootloader", cmd_reboot_bootloader},
						{"oem unlock", cmd_oem_unlock},
                        // ONLY for test, DON'T build it.
                        //{"oem unlock_test", cmd_oem_unlock_test},
						//{"oem unlock-go", cmd_oem_unlock_go},
						{"oem lock", cmd_oem_lock},
						//{"flashing unlock", cmd_oem_unlock},
						//{"flashing lock", cmd_oem_lock},
						{"oem ddrsorting", cmd_oem_ddsorting},
						//{"flashing lock_critical", cmd_flashing_lock_critical},
						//{"flashing unlock_critical", cmd_flashing_unlock_critical},
						//{"flashing get_unlock_ability", cmd_flashing_get_unlock_ability},
						{"oem device-info", cmd_oem_devinfo},
						{"preflash", cmd_preflash},
						{"oem enable-charger-screen", cmd_oem_enable_charger_screen},
						{"oem disable-charger-screen", cmd_oem_disable_charger_screen},
						{"oem off-mode-charge", cmd_oem_off_mode_charger},
						{"oem select-display-panel", cmd_oem_select_display_panel},
						{"set_active",cmd_set_active},
						{"oem edl", cmd_oem_edl},  
#if UNITTEST_FW_SUPPORT
						{"oem run-tests", cmd_oem_runtests},
#endif
#endif
						};

	int fastboot_cmds_count = sizeof(cmd_list)/sizeof(cmd_list[0]);
	for (i = 1; i < fastboot_cmds_count; i++)
		fastboot_register(cmd_list[i].name,cmd_list[i].cb);

	/* publish variables and their values */
	fastboot_publish("product",  TARGET(XIAOMI_PRODUCT));
	fastboot_publish("kernel",   "lk");
	fastboot_publish("serialno", sn_buf);
        fastboot_publish("rollback_ver",rollback_str);

	/*publish hw-revision major(upper 16 bits) and minor(lower 16 bits)*/
	snprintf(soc_version_str, MAX_RSP_SIZE, "%x", board_soc_version());
	fastboot_publish("hw-revision", soc_version_str);

	/*
	 * partition info is supported only for emmc partitions
	 * Calling this for NAND prints some error messages which
	 * is harmless but misleading. Avoid calling this for NAND
	 * devices.
	 */
	if (target_is_emmc_boot())
		publish_getvar_partition_info(part_info, partition_get_partition_count());

	if (partition_multislot_is_supported())
		publish_getvar_multislot_vars();

	/* Max download size supported */
	snprintf(max_download_size, MAX_RSP_SIZE, "\t0x%x",
			target_get_max_flash_size());
	fastboot_publish("max-download-size", (const char *) max_download_size);
	/* Is the charger screen check enabled */
	snprintf(charger_screen_enabled, MAX_RSP_SIZE, "%d",
			device.charger_screen_enabled);
	fastboot_publish("charger-screen-enabled",
			(const char *) charger_screen_enabled);
	fastboot_publish("off-mode-charge", (const char *) charger_screen_enabled);
	snprintf(panel_display_mode, MAX_RSP_SIZE, "%s",
			device.display_panel);
	fastboot_publish("display-panel",
			(const char *) panel_display_mode);

        if (target_is_emmc_boot())
        {
		mmc_blocksize = mmc_get_device_blocksize();
        }
        else
        {
		mmc_blocksize = flash_block_size();
        }
	snprintf(block_size_string, MAX_RSP_SIZE, "0x%x", mmc_blocksize);
	fastboot_publish("erase-block-size", (const char *) block_size_string);
	fastboot_publish("logical-block-size", (const char *) block_size_string);
#if PRODUCT_IOT
	get_bootloader_version_iot(&bootloader_version_string);
	fastboot_publish("version-bootloader", (const char *) bootloader_version_string);

	/* Version baseband is n/a for apq iot devices */
	fastboot_publish("version-baseband", "N/A");
#else
	fastboot_publish("version-bootloader", bootloader_version_temp);
	fastboot_publish("version-baseband", (const char *) device.radio_version);
#endif
	fastboot_publish("secure", is_secure_boot_enable()? "yes":"no");
	fastboot_publish("unlocked", device.is_unlocked ? "yes":"no");
	smem_get_hw_platform_name((unsigned char *) hw_platform_buf, sizeof(hw_platform_buf));
	snprintf(get_variant, MAX_RSP_SIZE, "%s %s", hw_platform_buf,
		target_is_emmc_boot()? "eMMC":"UFS");
	fastboot_publish("variant", (const char *) get_variant);
#if CHECK_BAT_VOLTAGE
	update_battery_status();
	fastboot_publish("battery-voltage", (const char *) battery_voltage);
	fastboot_publish("battery-soc-ok", (const char *) battery_soc_ok);
#endif

#if (VERIFIED_BOOT && !DEFAULT_UNLOCK) 
#ifdef PROJECT_VINCE
	if(generate_Token_e7())
#else
	if(generate_Token())
#endif
	{
		dprintf(CRITICAL, "RAND Bytes generation failed");
	}
	else
	{
		fastboot_publish("token", Token);
	}
#endif
}



//add for pit .by feng
static int display_fastboot(void)
{
	int index = INVALID_PTN;
	unsigned long long ptn = 0;
	struct fbcon_config *fb_display = NULL;
	struct logo_img_header *header;
	uint32_t blocksize, realsize, readsize;
	uint8_t *base;

	index = partition_get_index("splash");
	if (index == 0) {
		dprintf(CRITICAL, "ERROR: splash Partition table not found\n");
		return -1;
	}

	ptn = partition_get_offset(index);
	if (ptn == 0) {
		dprintf(CRITICAL, "ERROR: splash Partition invalid\n");
		return -1;
	}

	ptn += SPLASH_FASTBOOT_IMG_OFFSET;

	mmc_set_lun(partition_get_lun(index));

	blocksize = mmc_get_device_blocksize();
	if (blocksize == 0) {
		dprintf(CRITICAL, "ERROR:splash Partition invalid blocksize\n");
		return -1;
	}

	fb_display = fbcon_display();
	if (!fb_display)
	{
		dprintf(CRITICAL, "ERROR: fb config is not allocated\n");
		return -1;
	}

	base = (uint8_t *) fb_display->base;

	if (mmc_read(ptn, (uint32_t *)(base + LOGO_IMG_OFFSET), blocksize)) {
		dprintf(CRITICAL, "ERROR: Cannot read splash image header\n");
		return -1;
	}

	header = (struct logo_img_header *)(base + LOGO_IMG_OFFSET);
	if (splash_screen_check_header(header)) {
		dprintf(CRITICAL, "ERROR: Splash image header invalid\n");
		return -1;
	}

	if (fb_display) {
		if (header->type && (header->blocks != 0)) { /* 1 RLE24 compressed data */
			base += LOGO_IMG_OFFSET;

			realsize =  header->blocks * 512;
			readsize =  ROUNDUP((realsize + LOGO_IMG_HEADER_SIZE), blocksize) - blocksize;

			/* if the logo is not full-screen size, clean screen */
			if ((header->width != fb_display->width)
						|| (header->height != fb_display->height))
				fbcon_clear();

			if (mmc_read(ptn + blocksize, (uint32_t *)(base + blocksize), readsize)) {
				dprintf(CRITICAL, "ERROR: Cannot read splash image from partition\n");
				return -1;
			}

			fbcon_extract_to_screen(header, (base + LOGO_IMG_HEADER_SIZE));
		} else { /* 2 Raw BGR data */

			if ((header->width > fb_display->width) || (header->height > fb_display->height)) {
				dprintf(CRITICAL, "Logo config greater than fb config. Fall back default logo\n");
				return -1;
			}

			realsize =  header->width * header->height * fb_display->bpp / 8;
			readsize =  ROUNDUP((realsize + LOGO_IMG_HEADER_SIZE), blocksize) - blocksize;

			if (blocksize == LOGO_IMG_HEADER_SIZE) { /* read the content directly */
				if (mmc_read((ptn + LOGO_IMG_HEADER_SIZE), (uint32_t *)base, readsize)) {
					fbcon_clear();
					dprintf(CRITICAL, "ERROR: Cannot read splash image from partition\n");
					return -1;
				}
			} else {
				if (mmc_read(ptn + blocksize ,
						(uint32_t *)(base + LOGO_IMG_OFFSET + blocksize), readsize)) {
					dprintf(CRITICAL, "ERROR: Cannot read splash image from partition\n");
					return -1;
				}
				memmove(base, (base + LOGO_IMG_OFFSET + LOGO_IMG_HEADER_SIZE), realsize);
			}
		}
	}

	return 0;
}

extern uint32_t pm8x41_get_pwrkey_is_pressed(); // shut down after warning pit , by feng




void aboot_init(const struct app_descriptor *app)
{
	unsigned reboot_mode = 0;
	int boot_err_type = 0;
	int boot_slot = INVALID;
        unsigned int version_hig = 0;
        unsigned int version_mid = 0;
        unsigned int version_low = 0;
        unsigned int rollback_ver = 0;
        int i = 0;

#if PROJECT_SAKURA // for D1S
	static PCBA_CONFIG pcba_config=PCBA_UNKNOW;
#endif

#if PROJECT_DAISY //for D1S_A1
		static PCBA_CONFIG pcba_config=PCBA_UNKNOW;
#endif 

	/* Initialise wdog to catch early lk crashes */
#if WDOG_SUPPORT
	msm_wdog_init();
#endif

#ifndef FACTORY_VERSION_ENABLE
	boot_flag = target_pause_for_battery_charge();
#endif
	/* Setup page size information for nv storage */
	if (target_is_emmc_boot())
	{
		page_size = mmc_page_size();
		page_mask = page_size - 1;
		mmc_blocksize = mmc_get_device_blocksize();
		mmc_blocksize_mask = mmc_blocksize - 1;
	}
	else
	{
		page_size = flash_page_size();
		page_mask = page_size - 1;
	}
	ASSERT((MEMBASE + MEMSIZE) > MEMBASE);

	read_device_info(&device);
	read_allow_oem_unlock(&device);


#if PROJECT_SAKURA
	pcba_config = get_huaqin_pcba_config();
	printf("pcba_config : %d\n",pcba_config);
if((pcba_config == PCBA_V1_CN) || (pcba_config == PCBA_V2_CN)|| (pcba_config == PCBA_V1_IN)|| (pcba_config == PCBA_V2_IN)){
    printf("pcba is D1S , normal boot\n");
	} else{
		printf("pcba is not D1S , shut down\n");
		shutdown_device();
	}
#endif

#if PROJECT_DAISY
	pcba_config = get_huaqin_pcba_config();
	printf("pcba_config : %d\n",pcba_config);
	if((pcba_config == PCBA_V1_SA) || (pcba_config == PCBA_V1_GLOBAL) ||  (pcba_config == PCBA_V2_GLOBAL) ){
			printf("pcba is D1SA , normal boot\n");
	} else{
		printf("pcba is not D1SA , shut down\n");
		shutdown_device();
	}
#endif


            //read anti rollback version
        version_hig = (*((volatile uint32 *)(0x000a0144)));
        version_mid = (*((volatile uint32 *)(0x000a0140)));
        version_low = (*((volatile uint32 *)(0x000a013c)));

        version_hig = (version_hig & 0xf) << 17;
        version_mid = (version_mid & 0xf) << 14;
         version_low = (version_low & 0xffc0000) >> 18;

         rollback_ver = version_hig | version_mid | version_low;
         while(rollback_ver > 0){
         	i++;
         	rollback_ver = rollback_ver >> 1;
         }
         rollback_ver = i;
	// Xiaomi added begin
#if PROJECT_SAKURA && (!SAKURA_INDIA)
	anti_rb_version = rollback_ver;
#endif
	// Xiaomi added end
         sprintf(rollback_str,"%d",rollback_ver);

	/* Detect multi-slot support */
	if (partition_multislot_is_supported())
	{
		boot_slot = partition_find_active_slot();
		if (boot_slot == INVALID)
		{
			boot_into_fastboot = true;
			dprintf(INFO, "Active Slot: (INVALID)\n");
		}
		else
		{
			/* Setting the state of system to boot active slot */
			partition_mark_active_slot(boot_slot);
			dprintf(INFO, "Active Slot: (%s)\n", SUFFIX_SLOT(boot_slot));
		}
	}

	/* Display splash screen if enabled */
#if DISPLAY_SPLASH_SCREEN
#if NO_ALARM_DISPLAY
	if (!check_alarm_boot()) {
#endif
		dprintf(SPEW, "Display Init: Start\n");
#if DISPLAY_HDMI_PRIMARY
	if (!strlen(device.display_panel))
		strlcpy(device.display_panel, DISPLAY_PANEL_HDMI,
			sizeof(device.display_panel));
#endif
#if ENABLE_WBC
		/* Wait if the display shutdown is in progress */
		while(pm_app_display_shutdown_in_prgs());
		if (!pm_appsbl_display_init_done())
			target_display_init(device.display_panel);
		else
			display_image_on_screen();
#else
    #if PROJECT_DAISY
	dprintf(CRITICAL, "Line %d Display Init Done !\n",__LINE__);
    #else
	target_display_init(device.display_panel);
    #endif 
 #endif
		dprintf(SPEW, "Display Init: Done\n");
#if NO_ALARM_DISPLAY
	}
#endif
#endif

	target_serialno((unsigned char *) sn_buf);
	dprintf(SPEW,"serial number: %s\n",sn_buf);
	if((1 == ddrresult.ddr_button) || (2 == ddrresult.ddr_button)){
		if(1 == ddrresult.addr){
			snprintf((char *)sortingerr_addr, 60, "success");
		}else{
			snprintf((char *)sortingerr_addr, 60, "addr:0x%08x,write:0x%08x,read:0x%08x,type=%d", 
				ddrresult.addr,ddrresult.pattern_w,ddrresult.pattern_r,ddrresult.ddr_button);
		}
	}else{
		snprintf((char *)sortingerr_addr, 60, "undo");
	}	
	dprintf(SPEW,"ddr sorting addr: %s\n",sortingerr_addr);
	sprintf(secureboot_buf, "%s", is_secure_boot_enable() ? "1":"0");

	memset(display_panel_buf, '\0', MAX_PANEL_BUF_SIZE);
	dprintf(CRITICAL,"ddr:sorting_result=0x%08x button=0x%08x\n",ddrresult.addr,ddrresult.ddr_button);
	if(1 == ddrresult.ddr_button){
		if( !(0 == ddrresult.addr) && !(1 == ddrresult.addr)){		
			while(1){
				if(!gpio_status(85)){
					mdelay(3000);
					if(!gpio_status(85)){
						dprintf(CRITICAL,"VolumeUp is on over 3s,Continue...\n");
						break;
					}
				}
				display_ddr_sorting_result(((uint64_t)(ddrresult.addr)<<32) | (uint64_t)(ddrresult.pattern_r) ,8);
				mdelay(2000);
			}
		}
	}
	/*
	 * Check power off reason if user force reset,
	 * if yes phone will do normal boot.
	 */
#ifndef PROJECT_VINCE	 
	if (is_user_force_reset())
		goto normal_boot;
#endif
	/* Check if we should do something other than booting up */
	if (keys_get_state(KEY_VOLUMEUP) && keys_get_state(KEY_VOLUMEDOWN))
	{
#if 0
		dprintf(ALWAYS,"dload mode key sequence detected\n");
		reboot_device(EMERGENCY_DLOAD);
		dprintf(CRITICAL,"Failed to reboot into dload mode\n");

		boot_into_fastboot = true;

#endif
	}
	if (!boot_into_fastboot)
	{
	 	//VolumeUp + Power , boot into recovery . add by feng
		if(keys_get_state(KEY_VOLUMEUP) && !keys_get_state(KEY_VOLUMEDOWN)){
			dprintf(CRITICAL, "volumUp & !volumDown,boot_into_recovery is 1\n");
			boot_into_recovery = 1;
		}
		//VolumeDown + Power , boot into recovery . add by feng
		if(!keys_get_state(KEY_VOLUMEUP) && keys_get_state(KEY_VOLUMEDOWN)){
			dprintf(CRITICAL, "!volumUp & volumDown,boot_into_fastboot is true\n");
			boot_into_fastboot = true;
		}
	}
	#if NO_KEYPAD_DRIVER
	if (fastboot_trigger())
		boot_into_fastboot = true;
	#endif

	pu_reason = pm8x41_get_pon_reason();
	dprintf(INFO, "%s : pon_reason is:0x%x\n", __func__, pu_reason);
		if (pu_reason!=0x21)
	{
		vib_timed_turn_on(VIBRATE_TIME);
	}

	if (!pm8x41_get_is_cold_boot())
		pu_reason |= WARM_RST;

#if USE_PON_REBOOT_REG
	reboot_mode = check_hard_reboot_mode();
#else
	reboot_mode = check_reboot_mode();
#endif
	if (reboot_mode == RECOVERY_MODE)
	{
		boot_into_recovery = 1;
	}
	else if(reboot_mode == FASTBOOT_MODE)
	{
		boot_into_fastboot = true;
	}
	else if(reboot_mode == ALARM_BOOT)
	{
		boot_reason_alarm = true;
	}
#if VERIFIED_BOOT
	else if (VB_M <= target_get_vb_version())
	{
		if (reboot_mode == DM_VERITY_ENFORCING)
		{
			device.verity_mode = 1;
			write_device_info(&device);
		}
#if ENABLE_VB_ATTEST
		else if (reboot_mode == DM_VERITY_EIO)
#else
		else if (reboot_mode == DM_VERITY_LOGGING)
#endif
		{
			device.verity_mode = 1;
			write_device_info(&device);
		}
		else if (reboot_mode == DM_VERITY_KEYSCLEAR)
		{
			if(send_delete_keys_to_tz())
				ASSERT(0);
		}
	}
#endif
#if PROJECT_DAISY
   target_display_init(device.display_panel);
#endif
#ifndef PROJECT_VINCE
normal_boot:
#endif
	if (!boot_into_fastboot)
	{
		if (target_is_emmc_boot())
		{
			if(emmc_recovery_init())
				dprintf(ALWAYS,"error in emmc_recovery_init\n");
			if(target_use_signed_kernel())
			{
				if((device.is_unlocked) || (device.is_tampered))
				{
				#ifdef TZ_TAMPER_FUSE
					set_tamper_fuse_cmd(HLOS_IMG_TAMPER_FUSE);
				#endif
				#if USE_PCOM_SECBOOT
					set_tamper_flag(device.is_tampered);
				#endif
				}
			}
#if PROJECT_DAISY
                    if(device.is_unlocked) //for unlock pit
                    {
                    	mdelay(4800);
                    	splash_screen_mmc(SPLASH_UNLOCK_IMG_OFFSET);
                    }
#else
                    	display_image_on_screen();//wwm 20170308 .by feng//
#endif
                if(boot_linux_from_mmc() < 0){
                	splash_screen_mmc(SPLASH_DESTROY_IMG_OFFSET);
                	while(1) {
                		if (pm8x41_get_pwrkey_is_pressed()) {
                			shutdown_device();
                		}
                	}
                }
retry_boot:
			/* Trying to boot active partition */
			if (partition_multislot_is_supported())
			{
				boot_slot = partition_find_boot_slot();
				partition_mark_active_slot(boot_slot);
				if (boot_slot == INVALID)
					goto fastboot;
			}

			boot_err_type = boot_linux_from_mmc();
			switch (boot_err_type)
			{
			        case ERR_BOOT_VERIFIED_FAIL: // add for warning pit . by feng
                        	splash_screen_mmc(SPLASH_DESTROY_IMG_OFFSET);
                        	while(1) {
                        		if (pm8x41_get_pwrkey_is_pressed()) {
                        			shutdown_device();
                        		}
                        	}
                        	break;
				case ERR_INVALID_PAGE_SIZE:
				case ERR_DT_PARSE:
				case ERR_ABOOT_ADDR_OVERLAP:
				case ERR_INVALID_BOOT_MAGIC:
					if(partition_multislot_is_supported())
					{
						/*
						 * Deactivate current slot, as it failed to
						 * boot, and retry next slot.
						 */
						partition_deactivate_slot(boot_slot);
						goto retry_boot;
					}
					else
						break;
				default:
					break;
				/* going to fastboot menu */
			}
		}
		else
		{
			recovery_init();
	#if USE_PCOM_SECBOOT
		if((device.is_unlocked) || (device.is_tampered))
			set_tamper_flag(device.is_tampered);
	#endif
			boot_linux_from_flash();
		}
		dprintf(CRITICAL, "ERROR: Could not do normal boot. Reverting "
			"to fastboot mode.\n");
	}

fastboot:
	/* We are here means regular boot did not happen. Start fastboot. */

	/* register aboot specific fastboot commands */
	aboot_fastboot_register_commands();

	/* dump partition table for debug info */
	partition_dump();

	/* initialize and start fastboot */
	fastboot_init(target_get_scratch_address(), target_get_max_flash_size());
#if FBCON_DISPLAY_MSG
	display_fastboot_menu();
#else
	dprintf(CRITICAL, "display fastboot logo\n");	
#if PROJECT_DAISY
	dprintf(CRITICAL, "D1S_A1,skip display_fastboot\n");
#else
	display_fastboot();
#endif
#endif
}

uint32_t get_page_size()
{
	return page_size;
}

/*
 * Calculated and save hash (SHA256) for non-signed boot image.
 *
 * @param image_addr - Boot image address
 * @param image_size - Size of the boot image
 *
 * @return int - 0 on success, negative value on failure.
 */
static int aboot_save_boot_hash_mmc(uint32_t image_addr, uint32_t image_size)
{
	unsigned int digest[8];
#if IMAGE_VERIF_ALGO_SHA1
	uint32_t auth_algo = CRYPTO_AUTH_ALG_SHA1;
#else
	uint32_t auth_algo = CRYPTO_AUTH_ALG_SHA256;
#endif

	target_crypto_init_params();
	hash_find((unsigned char *) image_addr, image_size, (unsigned char *)&digest, auth_algo);

	save_kernel_hash_cmd(digest);
	dprintf(INFO, "aboot_save_boot_hash_mmc: imagesize_actual size %d bytes.\n", (int) image_size);

	return 0;
}


APP_START(aboot)
	.init = aboot_init,
APP_END
