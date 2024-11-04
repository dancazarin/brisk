#pragma once
#include "private.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*hash_init)(hash_state* md);
typedef int (*hash_process)(hash_state* md, const unsigned char* in, unsigned long inlen);
typedef int (*hash_done)(hash_state* md, unsigned char* out);

int PUB_NAME(md5_init)(hash_state* md);
int PUB_NAME(md5_process)(hash_state* md, const unsigned char* in, unsigned long inlen);
int PUB_NAME(md5_done)(hash_state* md, unsigned char* out);

int PUB_NAME(sha1_init)(hash_state* md);
int PUB_NAME(sha1_process)(hash_state* md, const unsigned char* in, unsigned long inlen);
int PUB_NAME(sha1_done)(hash_state* md, unsigned char* out);

int PUB_NAME(sha256_init)(hash_state* md);
int PUB_NAME(sha256_process)(hash_state* md, const unsigned char* in, unsigned long inlen);
int PUB_NAME(sha256_done)(hash_state* md, unsigned char* out);

int PUB_NAME(sha512_init)(hash_state* md);
int PUB_NAME(sha512_process)(hash_state* md, const unsigned char* in, unsigned long inlen);
int PUB_NAME(sha512_done)(hash_state* md, unsigned char* out);

int PUB_NAME(sha3_256_init)(hash_state* md);
int PUB_NAME(sha3_512_init)(hash_state* md);
int PUB_NAME(sha3_process)(hash_state* md, const unsigned char* in, unsigned long inlen);
int PUB_NAME(sha3_done)(hash_state* md, unsigned char* out);

#ifdef __cplusplus
}
#endif
