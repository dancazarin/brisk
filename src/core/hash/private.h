#pragma once
/* LibTomCrypt, modular cryptographic library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef uint32_t ulong32;

#ifdef _MSC_VER
#define CONST64(n) n##ui64
typedef unsigned __int64 ulong64;
#else
#define CONST64(n) n##ULL
typedef unsigned long long ulong64;
#endif

#define LTC_SMALL_CODE 1

struct sha512_state {
    ulong64 length, state[8];
    unsigned long curlen;
    unsigned char buf[128];
};

struct sha256_state {
    ulong64 length;
    ulong32 state[8], curlen;
    unsigned char buf[64];
};

struct sha1_state {
    ulong64 length;
    ulong32 state[5], curlen;
    unsigned char buf[64];
};

struct md5_state {
    ulong64 length;
    ulong32 state[4], curlen;
    unsigned char buf[64];
};

struct sha3_state {
    ulong64 saved; /* the portion of the input message that we didn't consume yet */
    ulong64 s[25];
    unsigned char sb[25 * 8]; /* used for storing `ulong64 s[25]` as little-endian bytes */
    unsigned short
        byte_index; /* 0..7--the next byte after the set one (starts from 0; 0--none are buffered) */
    unsigned short word_index;     /* 0..24--the next word to integrate input (starts from 0) */
    unsigned short capacity_words; /* the double size of the hash output in words (e.g. 16 for Keccak 512) */
    unsigned short xof_flag;
};

typedef union _hash_state {
    struct sha3_state sha3;
    struct sha512_state sha512;
    struct sha256_state sha256;
    struct sha1_state sha1;
    struct md5_state md5;
} hash_state;

#define STORE32L(x, y)                                                                                       \
    do {                                                                                                     \
        (y)[3] = (unsigned char)(((x) >> 24) & 255);                                                         \
        (y)[2] = (unsigned char)(((x) >> 16) & 255);                                                         \
        (y)[1] = (unsigned char)(((x) >> 8) & 255);                                                          \
        (y)[0] = (unsigned char)((x) & 255);                                                                 \
    } while (0)

#define LOAD32L(x, y)                                                                                        \
    do {                                                                                                     \
        x = ((ulong32)((y)[3] & 255) << 24) | ((ulong32)((y)[2] & 255) << 16) |                              \
            ((ulong32)((y)[1] & 255) << 8) | ((ulong32)((y)[0] & 255));                                      \
    } while (0)

#define STORE64L(x, y)                                                                                       \
    do {                                                                                                     \
        (y)[7] = (unsigned char)(((x) >> 56) & 255);                                                         \
        (y)[6] = (unsigned char)(((x) >> 48) & 255);                                                         \
        (y)[5] = (unsigned char)(((x) >> 40) & 255);                                                         \
        (y)[4] = (unsigned char)(((x) >> 32) & 255);                                                         \
        (y)[3] = (unsigned char)(((x) >> 24) & 255);                                                         \
        (y)[2] = (unsigned char)(((x) >> 16) & 255);                                                         \
        (y)[1] = (unsigned char)(((x) >> 8) & 255);                                                          \
        (y)[0] = (unsigned char)((x) & 255);                                                                 \
    } while (0)

#define LOAD64L(x, y)                                                                                        \
    do {                                                                                                     \
        x = (((ulong64)((y)[7] & 255)) << 56) | (((ulong64)((y)[6] & 255)) << 48) |                          \
            (((ulong64)((y)[5] & 255)) << 40) | (((ulong64)((y)[4] & 255)) << 32) |                          \
            (((ulong64)((y)[3] & 255)) << 24) | (((ulong64)((y)[2] & 255)) << 16) |                          \
            (((ulong64)((y)[1] & 255)) << 8) | (((ulong64)((y)[0] & 255)));                                  \
    } while (0)

#define STORE32H(x, y)                                                                                       \
    do {                                                                                                     \
        (y)[0] = (unsigned char)(((x) >> 24) & 255);                                                         \
        (y)[1] = (unsigned char)(((x) >> 16) & 255);                                                         \
        (y)[2] = (unsigned char)(((x) >> 8) & 255);                                                          \
        (y)[3] = (unsigned char)((x) & 255);                                                                 \
    } while (0)

#define LOAD32H(x, y)                                                                                        \
    do {                                                                                                     \
        x = ((ulong32)((y)[0] & 255) << 24) | ((ulong32)((y)[1] & 255) << 16) |                              \
            ((ulong32)((y)[2] & 255) << 8) | ((ulong32)((y)[3] & 255));                                      \
    } while (0)

#define STORE64H(x, y)                                                                                       \
    do {                                                                                                     \
        (y)[0] = (unsigned char)(((x) >> 56) & 255);                                                         \
        (y)[1] = (unsigned char)(((x) >> 48) & 255);                                                         \
        (y)[2] = (unsigned char)(((x) >> 40) & 255);                                                         \
        (y)[3] = (unsigned char)(((x) >> 32) & 255);                                                         \
        (y)[4] = (unsigned char)(((x) >> 24) & 255);                                                         \
        (y)[5] = (unsigned char)(((x) >> 16) & 255);                                                         \
        (y)[6] = (unsigned char)(((x) >> 8) & 255);                                                          \
        (y)[7] = (unsigned char)((x) & 255);                                                                 \
    } while (0)

#define LOAD64H(x, y)                                                                                        \
    do {                                                                                                     \
        x = (((ulong64)((y)[0] & 255)) << 56) | (((ulong64)((y)[1] & 255)) << 48) |                          \
            (((ulong64)((y)[2] & 255)) << 40) | (((ulong64)((y)[3] & 255)) << 32) |                          \
            (((ulong64)((y)[4] & 255)) << 24) | (((ulong64)((y)[5] & 255)) << 16) |                          \
            (((ulong64)((y)[6] & 255)) << 8) | (((ulong64)((y)[7] & 255)));                                  \
    } while (0)

#if defined(_MSC_VER)

/* instrinsic rotate */
#pragma intrinsic(_rotr, _rotl)
#define ROR(x, n) _rotr(x, n)
#define ROL(x, n) _rotl(x, n)
#define RORc(x, n) ROR(x, n)
#define ROLc(x, n) ROL(x, n)
#define ROR64(x, n) _rotr64(x, n)
#define ROL64(x, n) _rotl64(x, n)
#define ROR64c(x, n) ROR64(x, n)
#define ROL64c(x, n) ROL64(x, n)

#else

#if __has_builtin(__builtin_rotateright32) && __has_builtin(__builtin_rotateright64)
#define ROR(x, n) __builtin_rotateright32(x, n)
#define ROL(x, n) __builtin_rotateleft32(x, n)
#define ROLc(x, n) ROL(x, n)
#define RORc(x, n) ROR(x, n)
#define ROR64(x, n) __builtin_rotateright64(x, n)
#define ROL64(x, n) __builtin_rotateleft64(x, n)
#define ROL64c(x, n) ROL64(x, n)
#define ROR64c(x, n) ROR64(x, n)
#else
#define ROR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define ROL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define ROLc(x, n) ROL(x, n)
#define RORc(x, n) ROR(x, n)
#define ROR64(x, n) (((x) >> (n)) | ((x) << (64 - (n))))
#define ROL64(x, n) (((x) << (n)) | ((x) >> (64 - (n))))
#define ROL64c(x, n) ROL64(x, n)
#define ROR64c(x, n) ROR64(x, n)
#endif

#endif

/* error codes [will be expanded in future releases] */
enum {
    CRYPT_OK = 0, /* Result OK */
    CRYPT_ERROR,  /* Generic Error */
    CRYPT_NOP,    /* Not a failure but no operation was performed */

    CRYPT_INVALID_KEYSIZE, /* Invalid key size given */
    CRYPT_INVALID_ROUNDS,  /* Invalid number of rounds */
    CRYPT_FAIL_TESTVECTOR, /* Algorithm failed test vectors */

    CRYPT_BUFFER_OVERFLOW, /* Not enough space for output */
    CRYPT_INVALID_PACKET,  /* Invalid input packet given */

    CRYPT_INVALID_PRNGSIZE, /* Invalid number of bits for a PRNG */
    CRYPT_ERROR_READPRNG,   /* Could not read enough from PRNG */

    CRYPT_INVALID_CIPHER, /* Invalid cipher specified */
    CRYPT_INVALID_HASH,   /* Invalid hash specified */
    CRYPT_INVALID_PRNG,   /* Invalid PRNG specified */

    CRYPT_MEM, /* Out of memory */

    CRYPT_PK_TYPE_MISMATCH, /* Not equivalent types of PK keys */
    CRYPT_PK_NOT_PRIVATE,   /* Requires a private PK key */

    CRYPT_INVALID_ARG,   /* Generic invalid argument */
    CRYPT_FILE_NOTFOUND, /* File Not Found */

    CRYPT_PK_INVALID_TYPE, /* Invalid type of PK key */

    CRYPT_OVERFLOW, /* An overflow of a value was detected/prevented */

    CRYPT_PK_ASN1_ERROR, /* An error occurred while en- or decoding ASN.1 data */

    CRYPT_INPUT_TOO_LONG, /* The input was longer than expected. */

    CRYPT_PK_INVALID_SIZE, /* Invalid size input for PK parameters */

    CRYPT_INVALID_PRIME_SIZE, /* Invalid size of prime requested */
    CRYPT_PK_INVALID_PADDING, /* Invalid padding on input */

    CRYPT_HASH_OVERFLOW,  /* Hash applied to too many bits */
    CRYPT_PW_CTX_MISSING, /* Password context to decrypt key file is missing */
    CRYPT_UNKNOWN_PEM,    /* The PEM header was not recognized */
};

#define LTC_ARGCHK(x)                                                                                        \
    if (!(x))                                                                                                \
        return CRYPT_INVALID_ARG;
#define LTC_ARGCHKVD(x)                                                                                      \
    if (!(x))                                                                                                \
        return;

/* a simple macro for making hash "process" functions */
#define HASH_PROCESS(func_name, compress_name, state_var, block_size)                                        \
    int func_name(hash_state* md, const unsigned char* in, unsigned long inlen) {                            \
        unsigned long n;                                                                                     \
        int err;                                                                                             \
        LTC_ARGCHK(md != NULL);                                                                              \
        LTC_ARGCHK(in != NULL);                                                                              \
        if (md->state_var.curlen > sizeof(md->state_var.buf)) {                                              \
            return CRYPT_INVALID_ARG;                                                                        \
        }                                                                                                    \
        if (((md->state_var.length + inlen * 8) < md->state_var.length) || ((inlen * 8) < inlen)) {          \
            return CRYPT_HASH_OVERFLOW;                                                                      \
        }                                                                                                    \
        while (inlen > 0) {                                                                                  \
            if (md->state_var.curlen == 0 && inlen >= block_size) {                                          \
                if ((err = compress_name(md, in)) != CRYPT_OK) {                                             \
                    return err;                                                                              \
                }                                                                                            \
                md->state_var.length += block_size * 8;                                                      \
                in += block_size;                                                                            \
                inlen -= block_size;                                                                         \
            } else {                                                                                         \
                n = MIN(inlen, (block_size - md->state_var.curlen));                                         \
                XMEMCPY(md->state_var.buf + md->state_var.curlen, in, (size_t)n);                            \
                md->state_var.curlen += n;                                                                   \
                in += n;                                                                                     \
                inlen -= n;                                                                                  \
                if (md->state_var.curlen == block_size) {                                                    \
                    if ((err = compress_name(md, md->state_var.buf)) != CRYPT_OK) {                          \
                        return err;                                                                          \
                    }                                                                                        \
                    md->state_var.length += 8 * block_size;                                                  \
                    md->state_var.curlen = 0;                                                                \
                }                                                                                            \
            }                                                                                                \
        }                                                                                                    \
        return CRYPT_OK;                                                                                     \
    }

#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef LTC_UNUSED_PARAM
#define LTC_UNUSED_PARAM(x) (void)(x)
#endif

#ifndef XMEMSET
#define XMEMSET memset
#endif
#ifndef XMEMCPY
#define XMEMCPY memcpy
#endif
#ifndef XMEMMOVE
#define XMEMMOVE memmove
#endif
#ifndef XMEMCMP
#define XMEMCMP memcmp
#endif

#define PUB_NAME(x) brisk__##x

#if defined(_MSC_VER)
#define LTC_INLINE __inline
#else
#define LTC_INLINE inline
#endif
