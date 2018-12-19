/*Copyright (c) 2014-2015 The XiaoMi Tec. All rights reserved. */

#ifndef MIUNLOCK_H_
#define MIUNLOCK_H_
int generate_Token();
int Token_verify_compare(char * sig, unsigned sz);
int Token_verify_SN(char * sig, unsigned sz);
int generate_Token_e7();
int Token_verify_compare_e7(char * sig, unsigned sz);
int Token_verify_SN_e7(char * sig, unsigned sz);
void unlock_test(const char *arg, void *data, unsigned sz, void (*cb)(const char *msg));
extern char Token[];
#endif // MIUNLOCK_H_
