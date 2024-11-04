/* LibTomCrypt, modular cryptographic library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */
#include "private.h"

#define SHA3_KECCAK_SPONGE_WORDS 25 /* 1600 bits > 200 bytes > 25 x ulong64 */
#define SHA3_KECCAK_ROUNDS 24

static const ulong64 s_keccakf_rndc[24] = {
    CONST64(0x0000000000000001), CONST64(0x0000000000008082), CONST64(0x800000000000808a),
    CONST64(0x8000000080008000), CONST64(0x000000000000808b), CONST64(0x0000000080000001),
    CONST64(0x8000000080008081), CONST64(0x8000000000008009), CONST64(0x000000000000008a),
    CONST64(0x0000000000000088), CONST64(0x0000000080008009), CONST64(0x000000008000000a),
    CONST64(0x000000008000808b), CONST64(0x800000000000008b), CONST64(0x8000000000008089),
    CONST64(0x8000000000008003), CONST64(0x8000000000008002), CONST64(0x8000000000000080),
    CONST64(0x000000000000800a), CONST64(0x800000008000000a), CONST64(0x8000000080008081),
    CONST64(0x8000000000008080), CONST64(0x0000000080000001), CONST64(0x8000000080008008)
};

static const unsigned s_keccakf_rotc[24] = { 1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
                                             27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44 };

static const unsigned s_keccakf_piln[24] = { 10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
                                             15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1 };

static void s_keccakf(ulong64 s[25]) {
    int i, j, round;
    ulong64 t, bc[5];

    for (round = 0; round < SHA3_KECCAK_ROUNDS; round++) {
        /* Theta */
        for (i = 0; i < 5; i++) {
            bc[i] = s[i] ^ s[i + 5] ^ s[i + 10] ^ s[i + 15] ^ s[i + 20];
        }
        for (i = 0; i < 5; i++) {
            t = bc[(i + 4) % 5] ^ ROL64(bc[(i + 1) % 5], 1);
            for (j = 0; j < 25; j += 5) {
                s[j + i] ^= t;
            }
        }
        /* Rho Pi */
        t = s[1];
        for (i = 0; i < 24; i++) {
            j     = s_keccakf_piln[i];
            bc[0] = s[j];
            s[j]  = ROL64(t, s_keccakf_rotc[i]);
            t     = bc[0];
        }
        /* Chi */
        for (j = 0; j < 25; j += 5) {
            for (i = 0; i < 5; i++) {
                bc[i] = s[j + i];
            }
            for (i = 0; i < 5; i++) {
                s[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
            }
        }
        /* Iota */
        s[0] ^= s_keccakf_rndc[round];
    }
}

static LTC_INLINE int ss_done(hash_state* md, unsigned char* hash, ulong64 pad) {
    unsigned i;

    LTC_ARGCHK(md != NULL);
    LTC_ARGCHK(hash != NULL);

    md->sha3.s[md->sha3.word_index] ^= (md->sha3.saved ^ (pad << (md->sha3.byte_index * 8)));
    md->sha3.s[SHA3_KECCAK_SPONGE_WORDS - md->sha3.capacity_words - 1] ^= CONST64(0x8000000000000000);
    s_keccakf(md->sha3.s);

    /* store sha3.s[] as little-endian bytes into sha3.sb */
    for (i = 0; i < SHA3_KECCAK_SPONGE_WORDS; i++) {
        STORE64L(md->sha3.s[i], md->sha3.sb + i * 8);
    }

    XMEMCPY(hash, md->sha3.sb, md->sha3.capacity_words * 4);
    return CRYPT_OK;
}

/* Public Inteface */

int PUB_NAME(sha3_224_init)(hash_state* md) {
    LTC_ARGCHK(md != NULL);
    XMEMSET(&md->sha3, 0, sizeof(md->sha3));
    md->sha3.capacity_words = 2 * 224 / (8 * sizeof(ulong64));
    return CRYPT_OK;
}

int PUB_NAME(sha3_256_init)(hash_state* md) {
    LTC_ARGCHK(md != NULL);
    XMEMSET(&md->sha3, 0, sizeof(md->sha3));
    md->sha3.capacity_words = 2 * 256 / (8 * sizeof(ulong64));
    return CRYPT_OK;
}

int PUB_NAME(sha3_384_init)(hash_state* md) {
    LTC_ARGCHK(md != NULL);
    XMEMSET(&md->sha3, 0, sizeof(md->sha3));
    md->sha3.capacity_words = 2 * 384 / (8 * sizeof(ulong64));
    return CRYPT_OK;
}

int PUB_NAME(sha3_512_init)(hash_state* md) {
    LTC_ARGCHK(md != NULL);
    XMEMSET(&md->sha3, 0, sizeof(md->sha3));
    md->sha3.capacity_words = 2 * 512 / (8 * sizeof(ulong64));
    return CRYPT_OK;
}

int PUB_NAME(sha3_process)(hash_state* md, const unsigned char* in, unsigned long inlen) {
    /* 0...7 -- how much is needed to have a word */
    unsigned old_tail = (8 - md->sha3.byte_index) & 7;

    unsigned long words;
    unsigned tail;
    unsigned long i;

    if (inlen == 0)
        return CRYPT_OK; /* nothing to do */
    LTC_ARGCHK(md != NULL);
    LTC_ARGCHK(in != NULL);

    if (inlen < old_tail) { /* have no complete word or haven't started the word yet */
        while (inlen--)
            md->sha3.saved |= (ulong64)(*(in++)) << ((md->sha3.byte_index++) * 8);
        return CRYPT_OK;
    }

    if (old_tail) { /* will have one word to process */
        inlen -= old_tail;
        while (old_tail--)
            md->sha3.saved |= (ulong64)(*(in++)) << ((md->sha3.byte_index++) * 8);
        /* now ready to add saved to the sponge */
        md->sha3.s[md->sha3.word_index] ^= md->sha3.saved;
        md->sha3.byte_index = 0;
        md->sha3.saved      = 0;
        if (++md->sha3.word_index == (SHA3_KECCAK_SPONGE_WORDS - md->sha3.capacity_words)) {
            s_keccakf(md->sha3.s);
            md->sha3.word_index = 0;
        }
    }

    /* now work in full words directly from input */
    words = inlen / sizeof(ulong64);
    tail  = inlen - words * sizeof(ulong64);

    for (i = 0; i < words; i++, in += sizeof(ulong64)) {
        ulong64 t;
        LOAD64L(t, in);
        md->sha3.s[md->sha3.word_index] ^= t;
        if (++md->sha3.word_index == (SHA3_KECCAK_SPONGE_WORDS - md->sha3.capacity_words)) {
            s_keccakf(md->sha3.s);
            md->sha3.word_index = 0;
        }
    }

    /* finally, save the partial word */
    while (tail--) {
        md->sha3.saved |= (ulong64)(*(in++)) << ((md->sha3.byte_index++) * 8);
    }
    return CRYPT_OK;
}

int PUB_NAME(sha3_done)(hash_state* md, unsigned char* out) {
    return ss_done(md, out, CONST64(0x06));
}
