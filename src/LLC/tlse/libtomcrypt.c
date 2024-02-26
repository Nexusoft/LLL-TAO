#define CRYPT 0x0117
#define LTC_NO_ROLC


#include "libtomcrypt.h"


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/**
   @file crypt.c
   Build strings, Tom St Denis
 */

const char *crypt_build_settings =
    "LibTomCrypt ""1.17"" (Tom St Denis, tomstdenis@gmail.com)\n"
                          "LibTomCrypt is public domain software.\n"
                          "Built on " __DATE__ " at " __TIME__ "\n\n\n"
                                                               "Endianess: "
#if defined(ENDIAN_NEUTRAL)
    "neutral\n"
#elif defined(ENDIAN_LITTLE)
    "little"
 #if defined(ENDIAN_32BITWORD)
    " (32-bit words)\n"
 #else
    " (64-bit words)\n"
 #endif
#elif defined(ENDIAN_BIG)
    "big"
 #if defined(ENDIAN_32BITWORD)
    " (32-bit words)\n"
 #else
    " (64-bit words)\n"
 #endif
#endif
    "Clean stack: "
#if defined(LTC_CLEAN_STACK)
    "enabled\n"
#else
    "disabled\n"
#endif
    "Ciphers built-in:\n"
#if defined(LTC_BLOWFISH)
    "   Blowfish\n"
#endif
#if defined(LTC_RC2)
    "   LTC_RC2\n"
#endif
#if defined(LTC_RC5)
    "   LTC_RC5\n"
#endif
#if defined(LTC_RC6)
    "   LTC_RC6\n"
#endif
#if defined(LTC_SAFERP)
    "   Safer+\n"
#endif
#if defined(LTC_SAFER)
    "   Safer\n"
#endif
#if defined(LTC_RIJNDAEL)
    "   Rijndael\n"
#endif
#if defined(LTC_XTEA)
    "   LTC_XTEA\n"
#endif
#if defined(LTC_TWOFISH)
    "   Twofish "
 #if defined(LTC_TWOFISH_SMALL) && defined(LTC_TWOFISH_TABLES) && defined(LTC_TWOFISH_ALL_TABLES)
    "(small, tables, all_tables)\n"
 #elif defined(LTC_TWOFISH_SMALL) && defined(LTC_TWOFISH_TABLES)
    "(small, tables)\n"
 #elif defined(LTC_TWOFISH_SMALL) && defined(LTC_TWOFISH_ALL_TABLES)
    "(small, all_tables)\n"
 #elif defined(LTC_TWOFISH_TABLES) && defined(LTC_TWOFISH_ALL_TABLES)
    "(tables, all_tables)\n"
 #elif defined(LTC_TWOFISH_SMALL)
    "(small)\n"
 #elif defined(LTC_TWOFISH_TABLES)
    "(tables)\n"
 #elif defined(LTC_TWOFISH_ALL_TABLES)
    "(all_tables)\n"
 #else
    "\n"
 #endif
#endif
#if defined(LTC_DES)
    "   LTC_DES\n"
#endif
#if defined(LTC_CAST5)
    "   LTC_CAST5\n"
#endif
#if defined(LTC_NOEKEON)
    "   Noekeon\n"
#endif
#if defined(LTC_SKIPJACK)
    "   Skipjack\n"
#endif
#if defined(LTC_KHAZAD)
    "   Khazad\n"
#endif
#if defined(LTC_ANUBIS)
    "   Anubis "
#endif
#if defined(LTC_ANUBIS_TWEAK)
    " (tweaked)"
#endif
    "\n"
#if defined(LTC_KSEED)
    "   LTC_KSEED\n"
#endif
#if defined(LTC_KASUMI)
    "   KASUMI\n"
#endif

    "\nHashes built-in:\n"
#if defined(LTC_SHA512)
    "   LTC_SHA-512\n"
#endif
#if defined(LTC_SHA384)
    "   LTC_SHA-384\n"
#endif
#if defined(LTC_SHA256)
    "   LTC_SHA-256\n"
#endif
#if defined(LTC_SHA224)
    "   LTC_SHA-224\n"
#endif
#if defined(LTC_TIGER)
    "   LTC_TIGER\n"
#endif
#if defined(LTC_SHA1)
    "   LTC_SHA1\n"
#endif
#if defined(LTC_MD5)
    "   LTC_MD5\n"
#endif
#if defined(LTC_MD4)
    "   LTC_MD4\n"
#endif
#if defined(LTC_MD2)
    "   LTC_MD2\n"
#endif
#if defined(LTC_RIPEMD128)
    "   LTC_RIPEMD128\n"
#endif
#if defined(LTC_RIPEMD160)
    "   LTC_RIPEMD160\n"
#endif
#if defined(LTC_RIPEMD256)
    "   LTC_RIPEMD256\n"
#endif
#if defined(LTC_RIPEMD320)
    "   LTC_RIPEMD320\n"
#endif
#if defined(LTC_WHIRLPOOL)
    "   LTC_WHIRLPOOL\n"
#endif
#if defined(LTC_CHC_HASH)
    "   LTC_CHC_HASH \n"
#endif

    "\nBlock Chaining Modes:\n"
#if defined(LTC_CFB_MODE)
    "   CFB\n"
#endif
#if defined(LTC_OFB_MODE)
    "   OFB\n"
#endif
#if defined(LTC_ECB_MODE)
    "   ECB\n"
#endif
#if defined(LTC_CBC_MODE)
    "   CBC\n"
#endif
#if defined(LTC_CTR_MODE)
    "   CTR "
#endif
#if defined(LTC_CTR_OLD)
    " (CTR_OLD) "
#endif
    "\n"
#if defined(LRW_MODE)
    "   LRW_MODE"
 #if defined(LRW_TABLES)
    " (LRW_TABLES) "
 #endif
    "\n"
#endif
#if defined(LTC_F8_MODE)
    "   F8 MODE\n"
#endif
#if defined(LTC_XTS_MODE)
    "   LTC_XTS_MODE\n"
#endif

    "\nMACs:\n"
#if defined(LTC_HMAC)
    "   LTC_HMAC\n"
#endif
#if defined(LTC_OMAC)
    "   LTC_OMAC\n"
#endif
#if defined(LTC_PMAC)
    "   PMAC\n"
#endif
#if defined(LTC_PELICAN)
    "   LTC_PELICAN\n"
#endif
#if defined(LTC_XCBC)
    "   XCBC-MAC\n"
#endif
#if defined(LTC_F9_MODE)
    "   F9-MAC\n"
#endif

    "\nENC + AUTH modes:\n"
#if defined(LTC_EAX_MODE)
    "   LTC_EAX_MODE\n"
#endif
#if defined(LTC_OCB_MODE)
    "   LTC_OCB_MODE\n"
#endif
#if defined(LTC_CCM_MODE)
    "   LTC_CCM_MODE\n"
#endif
#if defined(LTC_GCM_MODE)
    "   LTC_GCM_MODE "
#endif
#if defined(LTC_GCM_TABLES)
    " (LTC_GCM_TABLES) "
#endif
    "\n"

    "\nPRNG:\n"
#if defined(LTC_YARROW)
    "   Yarrow\n"
#endif
#if defined(LTC_SPRNG)
    "   LTC_SPRNG\n"
#endif
#if defined(LTC_RC4)
    "   LTC_RC4\n"
#endif
#if defined(LTC_FORTUNA)
    "   Fortuna\n"
#endif
#if defined(LTC_SOBER128)
    "   LTC_SOBER128\n"
#endif

    "\nPK Algs:\n"
#if defined(LTC_MRSA)
    "   RSA \n"
#endif
#if defined(LTC_MECC)
    "   ECC\n"
#endif
#if defined(LTC_MDSA)
    "   DSA\n"
#endif
#if defined(MKAT)
    "   Katja\n"
#endif

    "\nCompiler:\n"
#if defined(WIN32)
    "   WIN32 platform detected.\n"
#endif
#if defined(__CYGWIN__)
    "   CYGWIN Detected.\n"
#endif
#if defined(__DJGPP__)
    "   DJGPP Detected.\n"
#endif
#if defined(_MSC_VER)
    "   MSVC compiler detected.\n"
#endif
#if defined(__GNUC__)
    "   GCC compiler detected.\n"
#endif
#if defined(INTEL_CC)
    "   Intel C Compiler detected.\n"
#endif
#if defined(__x86_64__)
    "   x86-64 detected.\n"
#endif
#if defined(LTC_PPC32)
    "   LTC_PPC32 defined \n"
#endif

    "\nVarious others: "
#if defined(LTC_BASE64)
    " LTC_BASE64 "
#endif
#if defined(MPI)
    " MPI "
#endif
#if defined(TRY_UNRANDOM_FIRST)
    " TRY_UNRANDOM_FIRST "
#endif
#if defined(LTC_TEST)
    " LTC_TEST "
#endif
#if defined(LTC_PKCS_1)
    " LTC_PKCS#1 "
#endif
#if defined(LTC_PKCS_5)
    " LTC_PKCS#5 "
#endif
#if defined(LTC_SMALL_CODE)
    " LTC_SMALL_CODE "
#endif
#if defined(LTC_NO_FILE)
    " LTC_NO_FILE "
#endif
#if defined(LTC_DER)
    " LTC_DER "
#endif
#if defined(LTC_FAST)
    " LTC_FAST "
#endif
#if defined(LTC_NO_FAST)
    " LTC_NO_FAST "
#endif
#if defined(LTC_NO_BSWAP)
    " LTC_NO_BSWAP "
#endif
#if defined(LTC_NO_ASM)
    " LTC_NO_ASM "
#endif
#if defined(LTC_NO_TEST)
    " LTC_NO_TEST "
#endif
#if defined(LTC_NO_TABLES)
    " LTC_NO_TABLES "
#endif
#if defined(LTC_PTHREAD)
    " LTC_PTHREAD "
#endif
#if defined(LTM_LTC_DESC)
    " LTM_DESC "
#endif
#if defined(TFM_LTC_DESC)
    " TFM_DESC "
#endif
#if defined(LTC_MECC_ACCEL)
    " LTC_MECC_ACCEL "
#endif
#if defined(GMP_LTC_DESC)
    " GMP_DESC "
#endif
#if defined(LTC_EASY)
    " (easy) "
#endif
#if defined(LTC_MECC_FP)
    " LTC_MECC_FP "
#endif
#if defined(LTC_ECC_SHAMIR)
    " LTC_ECC_SHAMIR "
#endif
    "\n"
    "\n\n\n"
;


/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt.c,v $ */
/* $Revision: 1.36 $ */
/* $Date: 2007/05/12 14:46:12 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */
#include <signal.h>

/**
   @file crypt_argchk.c
   Perform argument checking, Tom St Denis
 */

#if (ARGTYPE == 0)
void crypt_argchk(char *v, char *s, int d) {
    fprintf(stderr, "LTC_ARGCHK '%s' failure on line %d of file %s\n",
            v, d, s);
    (void)raise(SIGABRT);
}
#endif


/* $Source: /cvs/libtom/libtomcrypt/src/headers/tomcrypt.h,v $ */
/* $Revision: 1.21 $ */
/* $Date: 2006/12/16 19:34:05 $ */

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_argchk.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_cipher_descriptor.c
   Stores the cipher descriptor table, Tom St Denis
 */

struct ltc_cipher_descriptor cipher_descriptor[TAB_SIZE] = {
    { NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

LTC_MUTEX_GLOBAL(ltc_cipher_mutex)


/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_cipher_descriptor.c,v $ */
/* $Revision: 1.13 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_cipher_is_valid.c
   Determine if cipher is valid, Tom St Denis
 */

/*
   Test if a cipher index is valid
   @param idx   The index of the cipher to search for
   @return CRYPT_OK if valid
 */
int cipher_is_valid(int idx) {
    LTC_MUTEX_LOCK(&ltc_cipher_mutex);
    if ((idx < 0) || (idx >= TAB_SIZE) || (cipher_descriptor[idx].name == NULL)) {
        LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
        return CRYPT_INVALID_CIPHER;
    }
    LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
    return CRYPT_OK;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_cipher_is_valid.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_find_cipher.c
   Find a cipher in the descriptor tables, Tom St Denis
 */

/**
   Find a registered cipher by name
   @param name   The name of the cipher to look for
   @return >= 0 if found, -1 if not present
 */
int find_cipher(const char *name) {
    int x;

    LTC_ARGCHK(name != NULL);
    LTC_MUTEX_LOCK(&ltc_cipher_mutex);
    for (x = 0; x < TAB_SIZE; x++) {
        if ((cipher_descriptor[x].name != NULL) && !XSTRCMP(cipher_descriptor[x].name, name)) {
            LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
            return x;
        }
    }
    LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
    return -1;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_find_cipher.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_find_cipher_any.c
   Find a cipher in the descriptor tables, Tom St Denis
 */

/**
   Find a cipher flexibly.  First by name then if not present by block and key size
   @param name        The name of the cipher desired
   @param blocklen    The minimum length of the block cipher desired (octets)
   @param keylen      The minimum length of the key size desired (octets)
   @return >= 0 if found, -1 if not present
 */
int find_cipher_any(const char *name, int blocklen, int keylen) {
    int x;

    LTC_ARGCHK(name != NULL);

    x = find_cipher(name);
    if (x != -1) return x;

    LTC_MUTEX_LOCK(&ltc_cipher_mutex);
    for (x = 0; x < TAB_SIZE; x++) {
        if (cipher_descriptor[x].name == NULL) {
            continue;
        }
        if ((blocklen <= (int)cipher_descriptor[x].block_length) && (keylen <= (int)cipher_descriptor[x].max_key_length)) {
            LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
            return x;
        }
    }
    LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
    return -1;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_find_cipher_any.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_find_cipher_id.c
   Find cipher by ID, Tom St Denis
 */

/**
   Find a cipher by ID number
   @param ID    The ID (not same as index) of the cipher to find
   @return >= 0 if found, -1 if not present
 */
int find_cipher_id(unsigned char ID) {
    int x;

    LTC_MUTEX_LOCK(&ltc_cipher_mutex);
    for (x = 0; x < TAB_SIZE; x++) {
        if (cipher_descriptor[x].ID == ID) {
            x = (cipher_descriptor[x].name == NULL) ? -1 : x;
            LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
            return x;
        }
    }
    LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
    return -1;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_find_cipher_id.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_find_hash.c
   Find a hash, Tom St Denis
 */

/**
   Find a registered hash by name
   @param name   The name of the hash to look for
   @return >= 0 if found, -1 if not present
 */
int find_hash(const char *name) {
    int x;

    LTC_ARGCHK(name != NULL);
    LTC_MUTEX_LOCK(&ltc_hash_mutex);
    for (x = 0; x < TAB_SIZE; x++) {
        if ((hash_descriptor[x].name != NULL) && (XSTRCMP(hash_descriptor[x].name, name) == 0)) {
            LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
            return x;
        }
    }
    LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
    return -1;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_find_hash.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_find_hash_any.c
   Find a hash, Tom St Denis
 */

/**
   Find a hash flexibly.  First by name then if not present by digest size
   @param name        The name of the hash desired
   @param digestlen   The minimum length of the digest size (octets)
   @return >= 0 if found, -1 if not present
 */int find_hash_any(const char *name, int digestlen) {
    int x, y, z;

    LTC_ARGCHK(name != NULL);

    x = find_hash(name);
    if (x != -1) return x;

    LTC_MUTEX_LOCK(&ltc_hash_mutex);
    y = MAXBLOCKSIZE + 1;
    z = -1;
    for (x = 0; x < TAB_SIZE; x++) {
        if (hash_descriptor[x].name == NULL) {
            continue;
        }
        if (((int)hash_descriptor[x].hashsize >= digestlen) && ((int)hash_descriptor[x].hashsize < y)) {
            z = x;
            y = hash_descriptor[x].hashsize;
        }
    }
    LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
    return z;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_find_hash_any.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_find_hash_id.c
   Find hash by ID, Tom St Denis
 */

/**
   Find a hash by ID number
   @param ID    The ID (not same as index) of the hash to find
   @return >= 0 if found, -1 if not present
 */
int find_hash_id(unsigned char ID) {
    int x;

    LTC_MUTEX_LOCK(&ltc_hash_mutex);
    for (x = 0; x < TAB_SIZE; x++) {
        if (hash_descriptor[x].ID == ID) {
            x = (hash_descriptor[x].name == NULL) ? -1 : x;
            LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
            return x;
        }
    }
    LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
    return -1;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_find_hash_id.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_find_hash_oid.c
   Find a hash, Tom St Denis
 */

int find_hash_oid(const unsigned long *ID, unsigned long IDlen) {
    int x;

    LTC_ARGCHK(ID != NULL);
    LTC_MUTEX_LOCK(&ltc_hash_mutex);
    for (x = 0; x < TAB_SIZE; x++) {
        if ((hash_descriptor[x].name != NULL) && (hash_descriptor[x].OIDlen == IDlen) && !XMEMCMP(hash_descriptor[x].OID, ID, sizeof(unsigned long) * IDlen)) {
            LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
            return x;
        }
    }
    LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
    return -1;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_find_hash_oid.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_find_prng.c
   Find a PRNG, Tom St Denis
 */

/**
   Find a registered PRNG by name
   @param name   The name of the PRNG to look for
   @return >= 0 if found, -1 if not present
 */
int find_prng(const char *name) {
    int x;

    LTC_ARGCHK(name != NULL);
    LTC_MUTEX_LOCK(&ltc_prng_mutex);
    for (x = 0; x < TAB_SIZE; x++) {
        if ((prng_descriptor[x].name != NULL) && (XSTRCMP(prng_descriptor[x].name, name) == 0)) {
            LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
            return x;
        }
    }
    LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
    return -1;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_find_prng.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

#include <stdarg.h>

/**
   @file crypt_fsa.c
   LibTomCrypt FULL SPEED AHEAD!, Tom St Denis
 */

/* format is ltc_mp, cipher_desc, [cipher_desc], NULL, hash_desc, [hash_desc], NULL, prng_desc, [prng_desc], NULL */
int crypt_fsa(void *mp, ...) {
    int     err;
    va_list args;
    void    *p;

    va_start(args, mp);
    if (mp != NULL) {
        XMEMCPY(&ltc_mp, mp, sizeof(ltc_mp));
    }

    while ((p = va_arg(args, void *)) != NULL) {
        if ((err = register_cipher(p)) != CRYPT_OK) {
            va_end(args);
            return err;
        }
    }

    while ((p = va_arg(args, void *)) != NULL) {
        if ((err = register_hash(p)) != CRYPT_OK) {
            va_end(args);
            return err;
        }
    }

    while ((p = va_arg(args, void *)) != NULL) {
        if ((err = register_prng(p)) != CRYPT_OK) {
            va_end(args);
            return err;
        }
    }

    va_end(args);
    return CRYPT_OK;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_fsa.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_hash_descriptor.c
   Stores the hash descriptor table, Tom St Denis
 */

struct ltc_hash_descriptor hash_descriptor[TAB_SIZE] = {
    { NULL, 0, 0, 0, { 0 }, 0, NULL, NULL, NULL, NULL, NULL }
};

LTC_MUTEX_GLOBAL(ltc_hash_mutex)


/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_hash_descriptor.c,v $ */
/* $Revision: 1.10 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_hash_is_valid.c
   Determine if hash is valid, Tom St Denis
 */

/*
   Test if a hash index is valid
   @param idx   The index of the hash to search for
   @return CRYPT_OK if valid
 */
int hash_is_valid(int idx) {
    LTC_MUTEX_LOCK(&ltc_hash_mutex);
    if ((idx < 0) || (idx >= TAB_SIZE) || (hash_descriptor[idx].name == NULL)) {
        LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
        return CRYPT_INVALID_HASH;
    }
    LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
    return CRYPT_OK;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_hash_is_valid.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


ltc_math_descriptor ltc_mp;


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_prng_descriptor.c
   Stores the PRNG descriptors, Tom St Denis
 */
struct ltc_prng_descriptor prng_descriptor[TAB_SIZE] = {
    { NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

LTC_MUTEX_GLOBAL(ltc_prng_mutex)


/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_prng_descriptor.c,v $ */
/* $Revision: 1.8 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_prng_is_valid.c
   Determine if PRNG is valid, Tom St Denis
 */

/*
   Test if a PRNG index is valid
   @param idx   The index of the PRNG to search for
   @return CRYPT_OK if valid
 */
int prng_is_valid(int idx) {
    LTC_MUTEX_LOCK(&ltc_prng_mutex);
    if ((idx < 0) || (idx >= TAB_SIZE) || (prng_descriptor[idx].name == NULL)) {
        LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
        return CRYPT_INVALID_PRNG;
    }
    LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
    return CRYPT_OK;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_prng_is_valid.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_register_cipher.c
   Register a cipher, Tom St Denis
 */

/**
   Register a cipher with the descriptor table
   @param cipher   The cipher you wish to register
   @return value >= 0 if successfully added (or already present), -1 if unsuccessful
 */
int register_cipher(const struct ltc_cipher_descriptor *cipher) {
    int x;

    LTC_ARGCHK(cipher != NULL);

    /* is it already registered? */
    LTC_MUTEX_LOCK(&ltc_cipher_mutex);
    for (x = 0; x < TAB_SIZE; x++) {
        if ((cipher_descriptor[x].name != NULL) && (cipher_descriptor[x].ID == cipher->ID)) {
            LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
            return x;
        }
    }

    /* find a blank spot */
    for (x = 0; x < TAB_SIZE; x++) {
        if (cipher_descriptor[x].name == NULL) {
            XMEMCPY(&cipher_descriptor[x], cipher, sizeof(struct ltc_cipher_descriptor));
            LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
            return x;
        }
    }

    /* no spot */
    LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
    return -1;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_register_cipher.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_register_hash.c
   Register a HASH, Tom St Denis
 */

/**
   Register a hash with the descriptor table
   @param hash   The hash you wish to register
   @return value >= 0 if successfully added (or already present), -1 if unsuccessful
 */
int register_hash(const struct ltc_hash_descriptor *hash) {
    int x;

    LTC_ARGCHK(hash != NULL);

    /* is it already registered? */
    LTC_MUTEX_LOCK(&ltc_hash_mutex);
    for (x = 0; x < TAB_SIZE; x++) {
        if (XMEMCMP(&hash_descriptor[x], hash, sizeof(struct ltc_hash_descriptor)) == 0) {
            LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
            return x;
        }
    }

    /* find a blank spot */
    for (x = 0; x < TAB_SIZE; x++) {
        if (hash_descriptor[x].name == NULL) {
            XMEMCPY(&hash_descriptor[x], hash, sizeof(struct ltc_hash_descriptor));
            LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
            return x;
        }
    }

    /* no spot */
    LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
    return -1;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_register_hash.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_register_prng.c
   Register a PRNG, Tom St Denis
 */

/**
   Register a PRNG with the descriptor table
   @param prng   The PRNG you wish to register
   @return value >= 0 if successfully added (or already present), -1 if unsuccessful
 */
int register_prng(const struct ltc_prng_descriptor *prng) {
    int x;

    LTC_ARGCHK(prng != NULL);

    /* is it already registered? */
    LTC_MUTEX_LOCK(&ltc_prng_mutex);
    for (x = 0; x < TAB_SIZE; x++) {
        if (XMEMCMP(&prng_descriptor[x], prng, sizeof(struct ltc_prng_descriptor)) == 0) {
            LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
            return x;
        }
    }

    /* find a blank spot */
    for (x = 0; x < TAB_SIZE; x++) {
        if (prng_descriptor[x].name == NULL) {
            XMEMCPY(&prng_descriptor[x], prng, sizeof(struct ltc_prng_descriptor));
            LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
            return x;
        }
    }

    /* no spot */
    LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
    return -1;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_register_prng.c,v $ */
/* $Revision: 1.8 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_unregister_cipher.c
   Unregister a cipher, Tom St Denis
 */

/**
   Unregister a cipher from the descriptor table
   @param cipher   The cipher descriptor to remove
   @return CRYPT_OK on success
 */
int unregister_cipher(const struct ltc_cipher_descriptor *cipher) {
    int x;

    LTC_ARGCHK(cipher != NULL);

    /* is it already registered? */
    LTC_MUTEX_LOCK(&ltc_cipher_mutex);
    for (x = 0; x < TAB_SIZE; x++) {
        if (XMEMCMP(&cipher_descriptor[x], cipher, sizeof(struct ltc_cipher_descriptor)) == 0) {
            cipher_descriptor[x].name = NULL;
            cipher_descriptor[x].ID   = 255;
            LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
            return CRYPT_OK;
        }
    }
    LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
    return CRYPT_ERROR;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_unregister_cipher.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_unregister_hash.c
   Unregister a hash, Tom St Denis
 */

/**
   Unregister a hash from the descriptor table
   @param hash   The hash descriptor to remove
   @return CRYPT_OK on success
 */
int unregister_hash(const struct ltc_hash_descriptor *hash) {
    int x;

    LTC_ARGCHK(hash != NULL);

    /* is it already registered? */
    LTC_MUTEX_LOCK(&ltc_hash_mutex);
    for (x = 0; x < TAB_SIZE; x++) {
        if (XMEMCMP(&hash_descriptor[x], hash, sizeof(struct ltc_hash_descriptor)) == 0) {
            hash_descriptor[x].name = NULL;
            LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
            return CRYPT_OK;
        }
    }
    LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
    return CRYPT_ERROR;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_unregister_hash.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file crypt_unregister_prng.c
   Unregister a PRNG, Tom St Denis
 */

/**
   Unregister a PRNG from the descriptor table
   @param prng   The PRNG descriptor to remove
   @return CRYPT_OK on success
 */
int unregister_prng(const struct ltc_prng_descriptor *prng) {
    int x;

    LTC_ARGCHK(prng != NULL);

    /* is it already registered? */
    LTC_MUTEX_LOCK(&ltc_prng_mutex);
    for (x = 0; x < TAB_SIZE; x++) {
        if (XMEMCMP(&prng_descriptor[x], prng, sizeof(struct ltc_prng_descriptor)) != 0) {
            prng_descriptor[x].name = NULL;
            LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
            return CRYPT_OK;
        }
    }
    LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
    return CRYPT_ERROR;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_unregister_prng.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_decode_bit_string.c
   ASN.1 DER, encode a BIT STRING, Tom St Denis
 */


#ifdef LTC_DER

/**
   Store a BIT STRING
   @param in      The DER encoded BIT STRING
   @param inlen   The size of the DER BIT STRING
   @param out     [out] The array of bits stored (one per char)
   @param outlen  [in/out] The number of bits stored
   @return CRYPT_OK if successful
 */
int der_decode_bit_string(const unsigned char *in, unsigned long inlen,
                          unsigned char *out, unsigned long *outlen) {
    unsigned long dlen, blen, x, y;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* packet must be at least 4 bytes */
    if (inlen < 4) {
        return CRYPT_INVALID_ARG;
    }

    /* check for 0x03 */
    if ((in[0] & 0x1F) != 0x03) {
        return CRYPT_INVALID_PACKET;
    }

    /* offset in the data */
    x = 1;

    /* get the length of the data */
    if (in[x] & 0x80) {
        /* long format get number of length bytes */
        y = in[x++] & 0x7F;

        /* invalid if 0 or > 2 */
        if ((y == 0) || (y > 2)) {
            return CRYPT_INVALID_PACKET;
        }

        /* read the data len */
        dlen = 0;
        while (y--) {
            dlen = (dlen << 8) | (unsigned long)in[x++];
        }
    } else {
        /* short format */
        dlen = in[x++] & 0x7F;
    }

    /* is the data len too long or too short? */
    if ((dlen == 0) || (dlen + x > inlen)) {
        return CRYPT_INVALID_PACKET;
    }

    /* get padding count */
    blen = ((dlen - 1) << 3) - (in[x++] & 7);

    /* too many bits? */
    if (blen > *outlen) {
        *outlen = blen;
        return CRYPT_BUFFER_OVERFLOW;
    }

    /* decode/store the bits */
    for (y = 0; y < blen; y++) {
        out[y] = (in[x] & (1 << (7 - (y & 7)))) ? 1 : 0;
        if ((y & 7) == 7) {
            ++x;
        }
    }

    /* we done */
    *outlen = blen;
    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/bit/der_decode_bit_string.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_decode_boolean.c
   ASN.1 DER, decode a BOOLEAN, Tom St Denis
 */


#ifdef LTC_DER

/**
   Read a BOOLEAN
   @param in      The destination for the DER encoded BOOLEAN
   @param inlen   The size of the DER BOOLEAN
   @param out     [out]  The boolean to decode
   @return CRYPT_OK if successful
 */
int der_decode_boolean(const unsigned char *in, unsigned long inlen,
                       int *out) {
    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);

    if ((inlen != 3) || (in[0] != 0x01) || (in[1] != 0x01) || ((in[2] != 0x00) && (in[2] != 0xFF))) {
        return CRYPT_INVALID_ARG;
    }

    *out = (in[2] == 0xFF) ? 1 : 0;

    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/boolean/der_decode_boolean.c,v $ */
/* $Revision: 1.2 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_decode_choice.c
   ASN.1 DER, decode a CHOICE, Tom St Denis
 */

#ifdef LTC_DER

/**
   Decode a CHOICE
   @param in       The DER encoded input
   @param inlen    [in/out] The size of the input and resulting size of read type
   @param list     The list of items to decode
   @param outlen   The number of items in the list
   @return CRYPT_OK on success
 */
int der_decode_choice(const unsigned char *in, unsigned long *inlen,
                      ltc_asn1_list *list, unsigned long outlen) {
    unsigned long size, x, z;
    void          *data;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(inlen != NULL);
    LTC_ARGCHK(list != NULL);

    /* get blk size */
    if (*inlen < 2) {
        return CRYPT_INVALID_PACKET;
    }

    /* set all of the "used" flags to zero */
    for (x = 0; x < outlen; x++) {
        list[x].used = 0;
    }

    /* now scan until we have a winner */
    for (x = 0; x < outlen; x++) {
        size = list[x].size;
        data = list[x].data;

        switch (list[x].type) {
            case LTC_ASN1_INTEGER:
                if (der_decode_integer(in, *inlen, data) == CRYPT_OK) {
                    if (der_length_integer(data, &z) == CRYPT_OK) {
                        list[x].used = 1;
                        *inlen       = z;
                        return CRYPT_OK;
                    }
                }
                break;

            case LTC_ASN1_SHORT_INTEGER:
                if (der_decode_short_integer(in, *inlen, data) == CRYPT_OK) {
                    if (der_length_short_integer(size, &z) == CRYPT_OK) {
                        list[x].used = 1;
                        *inlen       = z;
                        return CRYPT_OK;
                    }
                }
                break;

            case LTC_ASN1_BIT_STRING:
                if (der_decode_bit_string(in, *inlen, data, &size) == CRYPT_OK) {
                    if (der_length_bit_string(size, &z) == CRYPT_OK) {
                        list[x].used = 1;
                        list[x].size = size;
                        *inlen       = z;
                        return CRYPT_OK;
                    }
                }
                break;

            case LTC_ASN1_OCTET_STRING:
                if (der_decode_octet_string(in, *inlen, data, &size) == CRYPT_OK) {
                    if (der_length_octet_string(size, &z) == CRYPT_OK) {
                        list[x].used = 1;
                        list[x].size = size;
                        *inlen       = z;
                        return CRYPT_OK;
                    }
                }
                break;

            case LTC_ASN1_NULL:
                if ((*inlen == 2) && (in[x] == 0x05) && (in[x + 1] == 0x00)) {
                    *inlen       = 2;
                    list[x].used = 1;
                    return CRYPT_OK;
                }
                break;

            case LTC_ASN1_OBJECT_IDENTIFIER:
                if (der_decode_object_identifier(in, *inlen, data, &size) == CRYPT_OK) {
                    if (der_length_object_identifier(data, size, &z) == CRYPT_OK) {
                        list[x].used = 1;
                        list[x].size = size;
                        *inlen       = z;
                        return CRYPT_OK;
                    }
                }
                break;

            case LTC_ASN1_IA5_STRING:
                if (der_decode_ia5_string(in, *inlen, data, &size) == CRYPT_OK) {
                    if (der_length_ia5_string(data, size, &z) == CRYPT_OK) {
                        list[x].used = 1;
                        list[x].size = size;
                        *inlen       = z;
                        return CRYPT_OK;
                    }
                }
                break;


            case LTC_ASN1_PRINTABLE_STRING:
                if (der_decode_printable_string(in, *inlen, data, &size) == CRYPT_OK) {
                    if (der_length_printable_string(data, size, &z) == CRYPT_OK) {
                        list[x].used = 1;
                        list[x].size = size;
                        *inlen       = z;
                        return CRYPT_OK;
                    }
                }
                break;

            case LTC_ASN1_UTF8_STRING:
                if (der_decode_utf8_string(in, *inlen, data, &size) == CRYPT_OK) {
                    if (der_length_utf8_string(data, size, &z) == CRYPT_OK) {
                        list[x].used = 1;
                        list[x].size = size;
                        *inlen       = z;
                        return CRYPT_OK;
                    }
                }
                break;

            case LTC_ASN1_UTCTIME:
                z = *inlen;
                if (der_decode_utctime(in, &z, data) == CRYPT_OK) {
                    list[x].used = 1;
                    *inlen       = z;
                    return CRYPT_OK;
                }
                break;

            case LTC_ASN1_SET:
            case LTC_ASN1_SETOF:
            case LTC_ASN1_SEQUENCE:
                if (der_decode_sequence(in, *inlen, data, size) == CRYPT_OK) {
                    if (der_length_sequence(data, size, &z) == CRYPT_OK) {
                        list[x].used = 1;
                        *inlen       = z;
                        return CRYPT_OK;
                    }
                }
                break;

            default:
                return CRYPT_INVALID_ARG;
        }
    }

    return CRYPT_INVALID_PACKET;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/choice/der_decode_choice.c,v $ */
/* $Revision: 1.9 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_decode_ia5_string.c
   ASN.1 DER, encode a IA5 STRING, Tom St Denis
 */


#ifdef LTC_DER

/**
   Store a IA5 STRING
   @param in      The DER encoded IA5 STRING
   @param inlen   The size of the DER IA5 STRING
   @param out     [out] The array of octets stored (one per char)
   @param outlen  [in/out] The number of octets stored
   @return CRYPT_OK if successful
 */
int der_decode_ia5_string(const unsigned char *in, unsigned long inlen,
                          unsigned char *out, unsigned long *outlen) {
    unsigned long x, y, len;
    int           t;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* must have header at least */
    if (inlen < 2) {
        return CRYPT_INVALID_PACKET;
    }

    /* check for 0x16 */
    if ((in[0] & 0x1F) != 0x16) {
        return CRYPT_INVALID_PACKET;
    }
    x = 1;

    /* decode the length */
    if (in[x] & 0x80) {
        /* valid # of bytes in length are 1,2,3 */
        y = in[x] & 0x7F;
        if ((y == 0) || (y > 3) || ((x + y) > inlen)) {
            return CRYPT_INVALID_PACKET;
        }

        /* read the length in */
        len = 0;
        ++x;
        while (y--) {
            len = (len << 8) | in[x++];
        }
    } else {
        len = in[x++] & 0x7F;
    }

    /* is it too long? */
    if (len > *outlen) {
        *outlen = len;
        return CRYPT_BUFFER_OVERFLOW;
    }

    if (len + x > inlen) {
        return CRYPT_INVALID_PACKET;
    }

    /* read the data */
    for (y = 0; y < len; y++) {
        t = der_ia5_value_decode(in[x++]);
        if (t == -1) {
            return CRYPT_INVALID_ARG;
        }
        out[y] = t;
    }

    *outlen = y;

    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/ia5/der_decode_ia5_string.c,v $ */
/* $Revision: 1.4 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_decode_integer.c
   ASN.1 DER, decode an integer, Tom St Denis
 */


#ifdef LTC_DER

/**
   Read a mp_int integer
   @param in       The DER encoded data
   @param inlen    Size of DER encoded data
   @param num      The first mp_int to decode
   @return CRYPT_OK if successful
 */
int der_decode_integer(const unsigned char *in, unsigned long inlen, void *num) {
    unsigned long x, y, z;
    int           err;

    LTC_ARGCHK(num != NULL);
    LTC_ARGCHK(in != NULL);

    /* min DER INTEGER is 0x02 01 00 == 0 */
    if (inlen < (1 + 1 + 1)) {
        return CRYPT_INVALID_PACKET;
    }

    /* ok expect 0x02 when we AND with 0001 1111 [1F] */
    x = 0;
    if ((in[x++] & 0x1F) != 0x02) {
        return CRYPT_INVALID_PACKET;
    }

    /* now decode the len stuff */
    z = in[x++];

    if ((z & 0x80) == 0x00) {
        /* short form */

        /* will it overflow? */
        if (x + z > inlen) {
            return CRYPT_INVALID_PACKET;
        }

        /* no so read it */
        if ((err = mp_read_unsigned_bin(num, (unsigned char *)in + x, z)) != CRYPT_OK) {
            return err;
        }
    } else {
        /* long form */
        z &= 0x7F;

        /* will number of length bytes overflow? (or > 4) */
        if (((x + z) > inlen) || (z > 4) || (z == 0)) {
            return CRYPT_INVALID_PACKET;
        }

        /* now read it in */
        y = 0;
        while (z--) {
            y = ((unsigned long)(in[x++])) | (y << 8);
        }

        /* now will reading y bytes overrun? */
        if ((x + y) > inlen) {
            return CRYPT_INVALID_PACKET;
        }

        /* no so read it */
        if ((err = mp_read_unsigned_bin(num, (unsigned char *)in + x, y)) != CRYPT_OK) {
            return err;
        }
    }

    /* see if it's negative */
    if (in[x] & 0x80) {
        void *tmp;
        if (mp_init(&tmp) != CRYPT_OK) {
            return CRYPT_MEM;
        }

        if ((mp_2expt(tmp, mp_count_bits(num)) != CRYPT_OK) || (mp_sub(num, tmp, num) != CRYPT_OK)) {
            mp_clear(tmp);
            return CRYPT_MEM;
        }
        mp_clear(tmp);
    }

    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/integer/der_decode_integer.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_decode_object_identifier.c
   ASN.1 DER, Decode Object Identifier, Tom St Denis
 */

#ifdef LTC_DER

/**
   Decode OID data and store the array of integers in words
   @param in      The OID DER encoded data
   @param inlen   The length of the OID data
   @param words   [out] The destination of the OID words
   @param outlen  [in/out] The number of OID words
   @return CRYPT_OK if successful
 */
int der_decode_object_identifier(const unsigned char *in, unsigned long inlen,
                                 unsigned long *words, unsigned long *outlen) {
    unsigned long x, y, t, len;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(words != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* header is at least 3 bytes */
    if (inlen < 3) {
        return CRYPT_INVALID_PACKET;
    }

    /* must be room for at least two words */
    if (*outlen < 2) {
        return CRYPT_BUFFER_OVERFLOW;
    }

    /* decode the packet header */
    x = 0;
    if ((in[x++] & 0x1F) != 0x06) {
        return CRYPT_INVALID_PACKET;
    }

    /* get the length */
    if (in[x] < 128) {
        len = in[x++];
    } else {
        if ((in[x] < 0x81) || (in[x] > 0x82)) {
            return CRYPT_INVALID_PACKET;
        }
        y   = in[x++] & 0x7F;
        len = 0;
        while (y--) {
            len = (len << 8) | (unsigned long)in[x++];
        }
    }

    if ((len < 1) || ((len + x) > inlen)) {
        return CRYPT_INVALID_PACKET;
    }

    /* decode words */
    y = 0;
    t = 0;
    while (len--) {
        t = (t << 7) | (in[x] & 0x7F);
        if (!(in[x++] & 0x80)) {
            /* store t */
            if (y >= *outlen) {
                return CRYPT_BUFFER_OVERFLOW;
            }
            if (y == 0) {
                words[0] = t / 40;
                words[1] = t % 40;
                y        = 2;
            } else {
                words[y++] = t;
            }
            t = 0;
        }
    }

    *outlen = y;
    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/object_identifier/der_decode_object_identifier.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_decode_octet_string.c
   ASN.1 DER, encode a OCTET STRING, Tom St Denis
 */


#ifdef LTC_DER

/**
   Store a OCTET STRING
   @param in      The DER encoded OCTET STRING
   @param inlen   The size of the DER OCTET STRING
   @param out     [out] The array of octets stored (one per char)
   @param outlen  [in/out] The number of octets stored
   @return CRYPT_OK if successful
 */
int der_decode_octet_string(const unsigned char *in, unsigned long inlen,
                            unsigned char *out, unsigned long *outlen) {
    unsigned long x, y, len;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* must have header at least */
    if (inlen < 2) {
        return CRYPT_INVALID_PACKET;
    }

    /* check for 0x04 */
    if ((in[0] & 0x1F) != 0x04) {
        return CRYPT_INVALID_PACKET;
    }
    x = 1;

    /* decode the length */
    if (in[x] & 0x80) {
        /* valid # of bytes in length are 1,2,3 */
        y = in[x] & 0x7F;
        if ((y == 0) || (y > 3) || ((x + y) > inlen)) {
            return CRYPT_INVALID_PACKET;
        }

        /* read the length in */
        len = 0;
        ++x;
        while (y--) {
            len = (len << 8) | in[x++];
        }
    } else {
        len = in[x++] & 0x7F;
    }

    /* is it too long? */
    if (len > *outlen) {
        *outlen = len;
        return CRYPT_BUFFER_OVERFLOW;
    }

    if (len + x > inlen) {
        return CRYPT_INVALID_PACKET;
    }

    /* read the data */
    for (y = 0; y < len; y++) {
        out[y] = in[x++];
    }

    *outlen = y;

    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/octet/der_decode_octet_string.c,v $ */
/* $Revision: 1.4 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_decode_printable_string.c
   ASN.1 DER, encode a printable STRING, Tom St Denis
 */


#ifdef LTC_DER

/**
   Store a printable STRING
   @param in      The DER encoded printable STRING
   @param inlen   The size of the DER printable STRING
   @param out     [out] The array of octets stored (one per char)
   @param outlen  [in/out] The number of octets stored
   @return CRYPT_OK if successful
 */
int der_decode_printable_string(const unsigned char *in, unsigned long inlen,
                                unsigned char *out, unsigned long *outlen) {
    unsigned long x, y, len;
    int           t;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* must have header at least */
    if (inlen < 2) {
        return CRYPT_INVALID_PACKET;
    }

    /* check for 0x13 */
    if ((in[0] & 0x1F) != 0x13) {
        return CRYPT_INVALID_PACKET;
    }
    x = 1;

    /* decode the length */
    if (in[x] & 0x80) {
        /* valid # of bytes in length are 1,2,3 */
        y = in[x] & 0x7F;
        if ((y == 0) || (y > 3) || ((x + y) > inlen)) {
            return CRYPT_INVALID_PACKET;
        }

        /* read the length in */
        len = 0;
        ++x;
        while (y--) {
            len = (len << 8) | in[x++];
        }
    } else {
        len = in[x++] & 0x7F;
    }

    /* is it too long? */
    if (len > *outlen) {
        *outlen = len;
        return CRYPT_BUFFER_OVERFLOW;
    }

    if (len + x > inlen) {
        return CRYPT_INVALID_PACKET;
    }

    /* read the data */
    for (y = 0; y < len; y++) {
        t = der_printable_value_decode(in[x++]);
        if (t == -1) {
            return CRYPT_INVALID_ARG;
        }
        out[y] = t;
    }

    *outlen = y;

    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/printable_string/der_decode_printable_string.c,v $ */
/* $Revision: 1.4 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

#include <stdarg.h>


/**
   @file der_decode_sequence_ex.c
   ASN.1 DER, decode a SEQUENCE, Tom St Denis
 */

#ifdef LTC_DER

/**
   Decode a SEQUENCE
   @param in       The DER encoded input
   @param inlen    The size of the input
   @param list     The list of items to decode
   @param outlen   The number of items in the list
   @param ordered  Search an unordered or ordered list
   @return CRYPT_OK on success
 */
int der_decode_sequence_ex(const unsigned char *in, unsigned long inlen,
                           ltc_asn1_list *list, unsigned long outlen, int ordered) {
    int           err, type;
    unsigned long size, x, y, z, i, blksize;
    void          *data;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(list != NULL);

    /* get blk size */
    if (inlen < 2) {
        return CRYPT_INVALID_PACKET;
    }

    /* sequence type? We allow 0x30 SEQUENCE and 0x31 SET since fundamentally they're the same structure */
    x = 0;
    if ((in[x] != 0x30) && (in[x] != 0x31)) {
        return CRYPT_INVALID_PACKET;
    }
    ++x;

    if (in[x] < 128) {
        blksize = in[x++];
    } else if (in[x] & 0x80) {
        if ((in[x] < 0x81) || (in[x] > 0x83)) {
            return CRYPT_INVALID_PACKET;
        }
        y = in[x++] & 0x7F;

        /* would reading the len bytes overrun? */
        if (x + y > inlen) {
            return CRYPT_INVALID_PACKET;
        }

        /* read len */
        blksize = 0;
        while (y--) {
            blksize = (blksize << 8) | (unsigned long)in[x++];
        }
    }

    /* would this blksize overflow? */
    if (x + blksize > inlen) {
        return CRYPT_INVALID_PACKET;
    }

    /* mark all as unused */
    for (i = 0; i < outlen; i++) {
        list[i].used = 0;
    }

    /* ok read data */
    inlen = blksize;
    for (i = 0; i < outlen; i++) {
        z    = 0;
        type = list[i].type;
        size = list[i].size;
        data = list[i].data;
        if (!ordered && (list[i].used == 1)) {
            continue;
        }

        if (type == LTC_ASN1_EOL) {
            break;
        }

        switch (type) {
            case LTC_ASN1_BOOLEAN:
                z = inlen;
                if ((err = der_decode_boolean(in + x, z, ((int *)data))) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                if ((err = der_length_boolean(&z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                break;

            case LTC_ASN1_INTEGER:
                z = inlen;
                if ((err = der_decode_integer(in + x, z, data)) != CRYPT_OK) {
                    if (!ordered) {
                        continue;
                    }
                    goto LBL_ERR;
                }
                if ((err = der_length_integer(data, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                break;

            case LTC_ASN1_SHORT_INTEGER:
                z = inlen;
                if ((err = der_decode_short_integer(in + x, z, data)) != CRYPT_OK) {
                    if (!ordered) {
                        continue;
                    }
                    goto LBL_ERR;
                }
                if ((err = der_length_short_integer(((unsigned long *)data)[0], &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }

                break;

            case LTC_ASN1_BIT_STRING:
                z = inlen;
                if ((err = der_decode_bit_string(in + x, z, data, &size)) != CRYPT_OK) {
                    if (!ordered) {
                        continue;
                    }
                    goto LBL_ERR;
                }
                list[i].size = size;
                if ((err = der_length_bit_string(size, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                break;

            case LTC_ASN1_OCTET_STRING:
                z = inlen;
                if ((err = der_decode_octet_string(in + x, z, data, &size)) != CRYPT_OK) {
                    if (!ordered) {
                        continue;
                    }
                    goto LBL_ERR;
                }
                list[i].size = size;
                if ((err = der_length_octet_string(size, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                break;

            case LTC_ASN1_NULL:
                if ((inlen < 2) || (in[x] != 0x05) || (in[x + 1] != 0x00)) {
                    if (!ordered) {
                        continue;
                    }
                    err = CRYPT_INVALID_PACKET;
                    goto LBL_ERR;
                }
                z = 2;
                break;

            case LTC_ASN1_OBJECT_IDENTIFIER:
                z = inlen;
                if ((err = der_decode_object_identifier(in + x, z, data, &size)) != CRYPT_OK) {
                    if (!ordered) {
                        continue;
                    }
                    goto LBL_ERR;
                }
                list[i].size = size;
                if ((err = der_length_object_identifier(data, size, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                break;

            case LTC_ASN1_IA5_STRING:
                z = inlen;
                if ((err = der_decode_ia5_string(in + x, z, data, &size)) != CRYPT_OK) {
                    if (!ordered) {
                        continue;
                    }
                    goto LBL_ERR;
                }
                list[i].size = size;
                if ((err = der_length_ia5_string(data, size, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                break;


            case LTC_ASN1_PRINTABLE_STRING:
                z = inlen;
                if ((err = der_decode_printable_string(in + x, z, data, &size)) != CRYPT_OK) {
                    if (!ordered) {
                        continue;
                    }
                    goto LBL_ERR;
                }
                list[i].size = size;
                if ((err = der_length_printable_string(data, size, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                break;

            case LTC_ASN1_UTF8_STRING:
                z = inlen;
                if ((err = der_decode_utf8_string(in + x, z, data, &size)) != CRYPT_OK) {
                    if (!ordered) {
                        continue;
                    }
                    goto LBL_ERR;
                }
                list[i].size = size;
                if ((err = der_length_utf8_string(data, size, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                break;

            case LTC_ASN1_UTCTIME:
                z = inlen;
                if ((err = der_decode_utctime(in + x, &z, data)) != CRYPT_OK) {
                    if (!ordered) {
                        continue;
                    }
                    goto LBL_ERR;
                }
                break;

            case LTC_ASN1_SET:
                z = inlen;
                if ((err = der_decode_set(in + x, z, data, size)) != CRYPT_OK) {
                    if (!ordered) {
                        continue;
                    }
                    goto LBL_ERR;
                }
                if ((err = der_length_sequence(data, size, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                break;

            case LTC_ASN1_SETOF:
            case LTC_ASN1_SEQUENCE:
                /* detect if we have the right type */
                if (((type == LTC_ASN1_SETOF) && ((in[x] & 0x3F) != 0x31)) || ((type == LTC_ASN1_SEQUENCE) && ((in[x] & 0x3F) != 0x30))) {
                    err = CRYPT_INVALID_PACKET;
                    goto LBL_ERR;
                }

                z = inlen;
                if ((err = der_decode_sequence(in + x, z, data, size)) != CRYPT_OK) {
                    if (!ordered) {
                        continue;
                    }
                    goto LBL_ERR;
                }
                if ((err = der_length_sequence(data, size, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                break;


            case LTC_ASN1_CHOICE:
                z = inlen;
                if ((err = der_decode_choice(in + x, &z, data, size)) != CRYPT_OK) {
                    if (!ordered) {
                        continue;
                    }
                    goto LBL_ERR;
                }
                break;

            default:
                err = CRYPT_INVALID_ARG;
                goto LBL_ERR;
        }
        x           += z;
        inlen       -= z;
        list[i].used = 1;
        if (!ordered) {
            /* restart the decoder */
            i = -1;
        }
    }

    for (i = 0; i < outlen; i++) {
        if (list[i].used == 0) {
            err = CRYPT_INVALID_PACKET;
            goto LBL_ERR;
        }
    }
    err = CRYPT_OK;

LBL_ERR:
    return err;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/sequence/der_decode_sequence_ex.c,v $ */
/* $Revision: 1.16 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_decode_sequence_flexi.c
   ASN.1 DER, decode an array of ASN.1 types with a flexi parser, Tom St Denis
 */

#ifdef LTC_DER

static unsigned long fetch_length(const unsigned char *in, unsigned long inlen) {
    unsigned long x, y, z;

    y = 0;

    /* skip type and read len */
    if (inlen < 2) {
        return 0xFFFFFFFF;
    }
    ++in;
    ++y;

    /* read len */
    x = *in++;
    ++y;

    /* <128 means literal */
    if (x < 128) {
        return x + y;
    }
    x     &= 0x7F; /* the lower 7 bits are the length of the length */
    inlen -= 2;

    /* len means len of len! */
    if ((x == 0) || (x > 4) || (x > inlen)) {
        return 0xFFFFFFFF;
    }

    y += x;
    z  = 0;
    while (x--) {
        z = (z << 8) | ((unsigned long)*in);
        ++in;
    }
    return z + y;
}

/**
   ASN.1 DER Flexi(ble) decoder will decode arbitrary DER packets and create a linked list of the decoded elements.
   @param in      The input buffer
   @param inlen   [in/out] The length of the input buffer and on output the amount of decoded data
   @param out     [out] A pointer to the linked list
   @return CRYPT_OK on success.
 */
int der_decode_sequence_flexi(const unsigned char *in, unsigned long *inlen, ltc_asn1_list **out) {
    ltc_asn1_list *l;
    unsigned long err, type, len, totlen, x, y;
    void          *realloc_tmp;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(inlen != NULL);
    LTC_ARGCHK(out != NULL);

    l      = NULL;
    totlen = 0;

    /* scan the input and and get lengths and what not */
    while (*inlen) {
        /* read the type byte */
        type = *in;

        /* fetch length */
        len = fetch_length(in, *inlen);
        if (len > *inlen) {
            err = CRYPT_INVALID_PACKET;
            goto error;
        }

        /* alloc new link */
        if (l == NULL) {
            l = XCALLOC(1, sizeof(*l));
            if (l == NULL) {
                err = CRYPT_MEM;
                goto error;
            }
        } else {
            l->next = XCALLOC(1, sizeof(*l));
            if (l->next == NULL) {
                err = CRYPT_MEM;
                goto error;
            }
            l->next->prev = l;
            l             = l->next;
        }

        /* now switch on type */
        switch (type) {
            case 0x01: /* BOOLEAN */
                l->type = LTC_ASN1_BOOLEAN;
                l->size = 1;
                l->data = XCALLOC(1, sizeof(int));

                if ((err = der_decode_boolean(in, *inlen, l->data)) != CRYPT_OK) {
                    goto error;
                }

                if ((err = der_length_boolean(&len)) != CRYPT_OK) {
                    goto error;
                }
                break;

            case 0x02: /* INTEGER */
                /* init field */
                l->type = LTC_ASN1_INTEGER;
                l->size = 1;
                if ((err = mp_init(&l->data)) != CRYPT_OK) {
                    goto error;
                }

                /* decode field */
                if ((err = der_decode_integer(in, *inlen, l->data)) != CRYPT_OK) {
                    goto error;
                }

                /* calc length of object */
                if ((err = der_length_integer(l->data, &len)) != CRYPT_OK) {
                    goto error;
                }
                break;

            case 0x03: /* BIT */
                /* init field */
                l->type = LTC_ASN1_BIT_STRING;
                l->size = len * 8; /* *8 because we store decoded bits one per char and they are encoded 8 per char.  */

                if ((l->data = XCALLOC(1, l->size)) == NULL) {
                    err = CRYPT_MEM;
                    goto error;
                }

                if ((err = der_decode_bit_string(in, *inlen, l->data, &l->size)) != CRYPT_OK) {
                    goto error;
                }

                if ((err = der_length_bit_string(l->size, &len)) != CRYPT_OK) {
                    goto error;
                }
                break;

            case 0x04: /* OCTET */

                /* init field */
                l->type = LTC_ASN1_OCTET_STRING;
                l->size = len;

                if ((l->data = XCALLOC(1, l->size)) == NULL) {
                    err = CRYPT_MEM;
                    goto error;
                }

                if ((err = der_decode_octet_string(in, *inlen, l->data, &l->size)) != CRYPT_OK) {
                    goto error;
                }

                if ((err = der_length_octet_string(l->size, &len)) != CRYPT_OK) {
                    goto error;
                }
                break;

            case 0x05: /* NULL */

                /* valid NULL is 0x05 0x00 */
                if ((in[0] != 0x05) || (in[1] != 0x00)) {
                    err = CRYPT_INVALID_PACKET;
                    goto error;
                }

                /* simple to store ;-) */
                l->type = LTC_ASN1_NULL;
                l->data = NULL;
                l->size = 0;
                len     = 2;

                break;

            case 0x06: /* OID */

                /* init field */
                l->type = LTC_ASN1_OBJECT_IDENTIFIER;
                l->size = len;

                if ((l->data = XCALLOC(len, sizeof(unsigned long))) == NULL) {
                    err = CRYPT_MEM;
                    goto error;
                }

                if ((err = der_decode_object_identifier(in, *inlen, l->data, &l->size)) != CRYPT_OK) {
                    goto error;
                }

                if ((err = der_length_object_identifier(l->data, l->size, &len)) != CRYPT_OK) {
                    goto error;
                }

                /* resize it to save a bunch of mem */
                if ((realloc_tmp = XREALLOC(l->data, l->size * sizeof(unsigned long))) == NULL) {
                    /* out of heap but this is not an error */
                    break;
                }
                l->data = realloc_tmp;
                break;

            case 0x0C: /* UTF8 */

                /* init field */
                l->type = LTC_ASN1_UTF8_STRING;
                l->size = len;

                if ((l->data = XCALLOC(sizeof(wchar_t), l->size)) == NULL) {
                    err = CRYPT_MEM;
                    goto error;
                }

                if ((err = der_decode_utf8_string(in, *inlen, l->data, &l->size)) != CRYPT_OK) {
                    goto error;
                }

                if ((err = der_length_utf8_string(l->data, l->size, &len)) != CRYPT_OK) {
                    goto error;
                }
                break;

            case 0x13: /* PRINTABLE */

                /* init field */
                l->type = LTC_ASN1_PRINTABLE_STRING;
                l->size = len;

                if ((l->data = XCALLOC(1, l->size)) == NULL) {
                    err = CRYPT_MEM;
                    goto error;
                }

                if ((err = der_decode_printable_string(in, *inlen, l->data, &l->size)) != CRYPT_OK) {
                    goto error;
                }

                if ((err = der_length_printable_string(l->data, l->size, &len)) != CRYPT_OK) {
                    goto error;
                }
                break;

            case 0x16: /* IA5 */

                /* init field */
                l->type = LTC_ASN1_IA5_STRING;
                l->size = len;

                if ((l->data = XCALLOC(1, l->size)) == NULL) {
                    err = CRYPT_MEM;
                    goto error;
                }

                if ((err = der_decode_ia5_string(in, *inlen, l->data, &l->size)) != CRYPT_OK) {
                    goto error;
                }

                if ((err = der_length_ia5_string(l->data, l->size, &len)) != CRYPT_OK) {
                    goto error;
                }
                break;

            case 0x17: /* UTC TIME */

                /* init field */
                l->type = LTC_ASN1_UTCTIME;
                l->size = 1;

                if ((l->data = XCALLOC(1, sizeof(ltc_utctime))) == NULL) {
                    err = CRYPT_MEM;
                    goto error;
                }

                len = *inlen;
                if ((err = der_decode_utctime(in, &len, l->data)) != CRYPT_OK) {
                    goto error;
                }

                if ((err = der_length_utctime(l->data, &len)) != CRYPT_OK) {
                    goto error;
                }
                break;

            case 0x30: /* SEQUENCE */
            case 0x31: /* SET */

                /* init field */
                l->type = (type == 0x30) ? LTC_ASN1_SEQUENCE : LTC_ASN1_SET;

                /* we have to decode the SEQUENCE header and get it's length */

                /* move past type */
                ++in;
                --(*inlen);

                /* read length byte */
                x = *in++;
                --(*inlen);

                /* smallest SEQUENCE/SET header */
                y = 2;

                /* now if it's > 127 the next bytes are the length of the length */
                if (x > 128) {
                    x      &= 0x7F;
                    in     += x;
                    *inlen -= x;

                    /* update sequence header len */
                    y += x;
                }

                /* Sequence elements go as child */
                len = len - y;
                if ((err = der_decode_sequence_flexi(in, &len, &(l->child))) != CRYPT_OK) {
                    goto error;
                }

                /* len update */
                totlen += y;

                /* link them up y0 */
                l->child->parent = l;

                break;

            default:
                /* invalid byte ... this is a soft error */
                /* remove link */
                l = l->prev;
                XFREE(l->next);
                l->next = NULL;
                goto outside;
        }

        /* advance pointers */
        totlen += len;
        in     += len;
        *inlen -= len;
    }

outside:

    /* rewind l please */
    while (l->prev != NULL || l->parent != NULL) {
        if (l->parent != NULL) {
            l = l->parent;
        } else {
            l = l->prev;
        }
    }

    /* return */
    *out   = l;
    *inlen = totlen;
    return CRYPT_OK;

error:
    /* free list */
    der_sequence_free(l);

    return err;
}
#endif


/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/sequence/der_decode_sequence_flexi.c,v $ */
/* $Revision: 1.26 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

#include <stdarg.h>


/**
   @file der_decode_sequence_multi.c
   ASN.1 DER, decode a SEQUENCE, Tom St Denis
 */

#ifdef LTC_DER

/**
   Decode a SEQUENCE type using a VA list
   @param in    Input buffer
   @param inlen Length of input in octets
   @remark <...> is of the form <type, size, data> (int, unsigned long, void*)
   @return CRYPT_OK on success
 */
int der_decode_sequence_multi(const unsigned char *in, unsigned long inlen, ...) {
    int           err, type;
    unsigned long size, x;
    void          *data;
    va_list       args;
    ltc_asn1_list *list;

    LTC_ARGCHK(in != NULL);

    /* get size of output that will be required */
    va_start(args, inlen);
    x = 0;
    for ( ; ; ) {
        type = va_arg(args, int);
        size = va_arg(args, unsigned long);
        data = va_arg(args, void *);

        if (type == LTC_ASN1_EOL) {
            break;
        }

        switch (type) {
            case LTC_ASN1_BOOLEAN:
            case LTC_ASN1_INTEGER:
            case LTC_ASN1_SHORT_INTEGER:
            case LTC_ASN1_BIT_STRING:
            case LTC_ASN1_OCTET_STRING:
            case LTC_ASN1_NULL:
            case LTC_ASN1_OBJECT_IDENTIFIER:
            case LTC_ASN1_IA5_STRING:
            case LTC_ASN1_PRINTABLE_STRING:
            case LTC_ASN1_UTF8_STRING:
            case LTC_ASN1_UTCTIME:
            case LTC_ASN1_SET:
            case LTC_ASN1_SETOF:
            case LTC_ASN1_SEQUENCE:
            case LTC_ASN1_CHOICE:
                ++x;
                break;

            default:
                va_end(args);
                return CRYPT_INVALID_ARG;
        }
    }
    va_end(args);

    /* allocate structure for x elements */
    if (x == 0) {
        return CRYPT_NOP;
    }

    list = XCALLOC(sizeof(*list), x);
    if (list == NULL) {
        return CRYPT_MEM;
    }

    /* fill in the structure */
    va_start(args, inlen);
    x = 0;
    for ( ; ; ) {
        type = va_arg(args, int);
        size = va_arg(args, unsigned long);
        data = va_arg(args, void *);

        if (type == LTC_ASN1_EOL) {
            break;
        }

        switch (type) {
            case LTC_ASN1_BOOLEAN:
            case LTC_ASN1_INTEGER:
            case LTC_ASN1_SHORT_INTEGER:
            case LTC_ASN1_BIT_STRING:
            case LTC_ASN1_OCTET_STRING:
            case LTC_ASN1_NULL:
            case LTC_ASN1_OBJECT_IDENTIFIER:
            case LTC_ASN1_IA5_STRING:
            case LTC_ASN1_PRINTABLE_STRING:
            case LTC_ASN1_UTF8_STRING:
            case LTC_ASN1_UTCTIME:
            case LTC_ASN1_SEQUENCE:
            case LTC_ASN1_SET:
            case LTC_ASN1_SETOF:
            case LTC_ASN1_CHOICE:
                list[x].type   = type;
                list[x].size   = size;
                list[x++].data = data;
                break;

            default:
                va_end(args);
                err = CRYPT_INVALID_ARG;
                goto LBL_ERR;
        }
    }
    va_end(args);

    err = der_decode_sequence(in, inlen, list, x);
LBL_ERR:
    XFREE(list);
    return err;
}
#endif


/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/sequence/der_decode_sequence_multi.c,v $ */
/* $Revision: 1.13 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_decode_short_integer.c
   ASN.1 DER, decode an integer, Tom St Denis
 */


#ifdef LTC_DER

/**
   Read a short integer
   @param in       The DER encoded data
   @param inlen    Size of data
   @param num      [out] The integer to decode
   @return CRYPT_OK if successful
 */
int der_decode_short_integer(const unsigned char *in, unsigned long inlen, unsigned long *num) {
    unsigned long len, x, y;

    LTC_ARGCHK(num != NULL);
    LTC_ARGCHK(in != NULL);

    /* check length */
    if (inlen < 2) {
        return CRYPT_INVALID_PACKET;
    }

    /* check header */
    x = 0;
    if ((in[x++] & 0x1F) != 0x02) {
        return CRYPT_INVALID_PACKET;
    }

    /* get the packet len */
    len = in[x++];

    if (x + len > inlen) {
        return CRYPT_INVALID_PACKET;
    }

    /* read number */
    y = 0;
    while (len--) {
        y = (y << 8) | (unsigned long)in[x++];
    }
    *num = y;

    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/short_integer/der_decode_short_integer.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_decode_utctime.c
   ASN.1 DER, decode a  UTCTIME, Tom St Denis
 */

#ifdef LTC_DER

static int char_to_int(unsigned char x) {
    switch (x) {
        case '0':
            return 0;

        case '1':
            return 1;

        case '2':
            return 2;

        case '3':
            return 3;

        case '4':
            return 4;

        case '5':
            return 5;

        case '6':
            return 6;

        case '7':
            return 7;

        case '8':
            return 8;

        case '9':
            return 9;
    }
    return 100;
}

 #define DECODE_V(y, max)                                   \
    y = char_to_int(buf[x]) * 10 + char_to_int(buf[x + 1]); \
    if (y >= max) return CRYPT_INVALID_PACKET;              \
    x += 2;

/**
   Decodes a UTC time structure in DER format (reads all 6 valid encoding formats)
   @param in     Input buffer
   @param inlen  Length of input buffer in octets
   @param out    [out] Destination of UTC time structure
   @return CRYPT_OK   if successful
 */
int der_decode_utctime(const unsigned char *in, unsigned long *inlen,
                       ltc_utctime *out) {
    unsigned char buf[32];
    unsigned long x;
    int           y;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(inlen != NULL);
    LTC_ARGCHK(out != NULL);

    /* check header */
    if ((*inlen < 2UL) || (in[1] >= sizeof(buf)) || ((in[1] + 2UL) > *inlen)) {
        return CRYPT_INVALID_PACKET;
    }

    /* decode the string */
    for (x = 0; x < in[1]; x++) {
        y = der_ia5_value_decode(in[x + 2]);
        if (y == -1) {
            return CRYPT_INVALID_PACKET;
        }
        buf[x] = y;
    }
    *inlen = 2 + x;


    /* possible encodings are
       YYMMDDhhmmZ
       YYMMDDhhmm+hh'mm'
       YYMMDDhhmm-hh'mm'
       YYMMDDhhmmssZ
       YYMMDDhhmmss+hh'mm'
       YYMMDDhhmmss-hh'mm'

       So let's do a trivial decode upto [including] mm
     */

    x = 0;
    DECODE_V(out->YY, 100);
    DECODE_V(out->MM, 13);
    DECODE_V(out->DD, 32);
    DECODE_V(out->hh, 24);
    DECODE_V(out->mm, 60);

    /* clear timezone and seconds info */
    out->off_dir = out->off_hh = out->off_mm = out->ss = 0;

    /* now is it Z, +, - or 0-9 */
    if (buf[x] == 'Z') {
        return CRYPT_OK;
    } else if ((buf[x] == '+') || (buf[x] == '-')) {
        out->off_dir = (buf[x++] == '+') ? 0 : 1;
        DECODE_V(out->off_hh, 24);
        DECODE_V(out->off_mm, 60);
        return CRYPT_OK;
    }

    /* decode seconds */
    DECODE_V(out->ss, 60);

    /* now is it Z, +, - */
    if (buf[x] == 'Z') {
        return CRYPT_OK;
    } else if ((buf[x] == '+') || (buf[x] == '-')) {
        out->off_dir = (buf[x++] == '+') ? 0 : 1;
        DECODE_V(out->off_hh, 24);
        DECODE_V(out->off_mm, 60);
        return CRYPT_OK;
    } else {
        return CRYPT_INVALID_PACKET;
    }
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/utctime/der_decode_utctime.c,v $ */
/* $Revision: 1.9 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_decode_utf8_string.c
   ASN.1 DER, encode a UTF8 STRING, Tom St Denis
 */


#ifdef LTC_DER

/**
   Store a UTF8 STRING
   @param in      The DER encoded UTF8 STRING
   @param inlen   The size of the DER UTF8 STRING
   @param out     [out] The array of utf8s stored (one per char)
   @param outlen  [in/out] The number of utf8s stored
   @return CRYPT_OK if successful
 */
int der_decode_utf8_string(const unsigned char *in, unsigned long inlen,
                           wchar_t *out, unsigned long *outlen) {
    wchar_t       tmp;
    unsigned long x, y, z, len;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* must have header at least */
    if (inlen < 2) {
        return CRYPT_INVALID_PACKET;
    }

    /* check for 0x0C */
    if ((in[0] & 0x1F) != 0x0C) {
        return CRYPT_INVALID_PACKET;
    }
    x = 1;

    /* decode the length */
    if (in[x] & 0x80) {
        /* valid # of bytes in length are 1,2,3 */
        y = in[x] & 0x7F;
        if ((y == 0) || (y > 3) || ((x + y) > inlen)) {
            return CRYPT_INVALID_PACKET;
        }

        /* read the length in */
        len = 0;
        ++x;
        while (y--) {
            len = (len << 8) | in[x++];
        }
    } else {
        len = in[x++] & 0x7F;
    }

    if (len + x > inlen) {
        return CRYPT_INVALID_PACKET;
    }

    /* proceed to decode */
    for (y = 0; x < inlen; ) {
        /* get first byte */
        tmp = in[x++];

        /* count number of bytes */
        for (z = 0; (tmp & 0x80) && (z <= 4); z++, tmp = (tmp << 1) & 0xFF);

        if ((z > 4) || (x + (z - 1) > inlen)) {
            return CRYPT_INVALID_PACKET;
        }

        /* decode, grab upper bits */
        tmp >>= z;

        /* grab remaining bytes */
        if (z > 1) {
            --z;
        }
        while (z-- != 0) {
            if ((in[x] & 0xC0) != 0x80) {
                return CRYPT_INVALID_PACKET;
            }
            tmp = (tmp << 6) | ((wchar_t)in[x++] & 0x3F);
        }

        if (y > *outlen) {
            *outlen = y;
            return CRYPT_BUFFER_OVERFLOW;
        }
        out[y++] = tmp;
    }
    *outlen = y;

    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/utf8/der_decode_utf8_string.c,v $ */
/* $Revision: 1.8 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_encode_bit_string.c
   ASN.1 DER, encode a BIT STRING, Tom St Denis
 */


#ifdef LTC_DER

/**
   Store a BIT STRING
   @param in       The array of bits to store (one per char)
   @param inlen    The number of bits tostore
   @param out      [out] The destination for the DER encoded BIT STRING
   @param outlen   [in/out] The max size and resulting size of the DER BIT STRING
   @return CRYPT_OK if successful
 */
int der_encode_bit_string(const unsigned char *in, unsigned long inlen,
                          unsigned char *out, unsigned long *outlen) {
    unsigned long len, x, y;
    unsigned char buf;
    int           err;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* avoid overflows */
    if ((err = der_length_bit_string(inlen, &len)) != CRYPT_OK) {
        return err;
    }

    if (len > *outlen) {
        *outlen = len;
        return CRYPT_BUFFER_OVERFLOW;
    }

    /* store header (include bit padding count in length) */
    x = 0;
    y = (inlen >> 3) + ((inlen & 7) ? 1 : 0) + 1;

    out[x++] = 0x03;
    if (y < 128) {
        out[x++] = (unsigned char)y;
    } else if (y < 256) {
        out[x++] = 0x81;
        out[x++] = (unsigned char)y;
    } else if (y < 65536) {
        out[x++] = 0x82;
        out[x++] = (unsigned char)((y >> 8) & 255);
        out[x++] = (unsigned char)(y & 255);
    }

    /* store number of zero padding bits */
    out[x++] = (unsigned char)((8 - inlen) & 7);

    /* store the bits in big endian format */
    for (y = buf = 0; y < inlen; y++) {
        buf |= (in[y] ? 1 : 0) << (7 - (y & 7));
        if ((y & 7) == 7) {
            out[x++] = buf;
            buf      = 0;
        }
    }
    /* store last byte */
    if (inlen & 7) {
        out[x++] = buf;
    }
    *outlen = x;
    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/bit/der_encode_bit_string.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_encode_boolean.c
   ASN.1 DER, encode a BOOLEAN, Tom St Denis
 */


#ifdef LTC_DER

/**
   Store a BOOLEAN
   @param in       The boolean to encode
   @param out      [out] The destination for the DER encoded BOOLEAN
   @param outlen   [in/out] The max size and resulting size of the DER BOOLEAN
   @return CRYPT_OK if successful
 */
int der_encode_boolean(int in,
                       unsigned char *out, unsigned long *outlen) {
    LTC_ARGCHK(outlen != NULL);
    LTC_ARGCHK(out != NULL);

    if (*outlen < 3) {
        *outlen = 3;
        return CRYPT_BUFFER_OVERFLOW;
    }

    *outlen = 3;
    out[0]  = 0x01;
    out[1]  = 0x01;
    out[2]  = in ? 0xFF : 0x00;

    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/boolean/der_encode_boolean.c,v $ */
/* $Revision: 1.4 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_encode_ia5_string.c
   ASN.1 DER, encode a IA5 STRING, Tom St Denis
 */

#ifdef LTC_DER

/**
   Store an IA5 STRING
   @param in       The array of IA5 to store (one per char)
   @param inlen    The number of IA5 to store
   @param out      [out] The destination for the DER encoded IA5 STRING
   @param outlen   [in/out] The max size and resulting size of the DER IA5 STRING
   @return CRYPT_OK if successful
 */
int der_encode_ia5_string(const unsigned char *in, unsigned long inlen,
                          unsigned char *out, unsigned long *outlen) {
    unsigned long x, y, len;
    int           err;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* get the size */
    if ((err = der_length_ia5_string(in, inlen, &len)) != CRYPT_OK) {
        return err;
    }

    /* too big? */
    if (len > *outlen) {
        *outlen = len;
        return CRYPT_BUFFER_OVERFLOW;
    }

    /* encode the header+len */
    x        = 0;
    out[x++] = 0x16;
    if (inlen < 128) {
        out[x++] = (unsigned char)inlen;
    } else if (inlen < 256) {
        out[x++] = 0x81;
        out[x++] = (unsigned char)inlen;
    } else if (inlen < 65536UL) {
        out[x++] = 0x82;
        out[x++] = (unsigned char)((inlen >> 8) & 255);
        out[x++] = (unsigned char)(inlen & 255);
    } else if (inlen < 16777216UL) {
        out[x++] = 0x83;
        out[x++] = (unsigned char)((inlen >> 16) & 255);
        out[x++] = (unsigned char)((inlen >> 8) & 255);
        out[x++] = (unsigned char)(inlen & 255);
    } else {
        return CRYPT_INVALID_ARG;
    }

    /* store octets */
    for (y = 0; y < inlen; y++) {
        out[x++] = der_ia5_char_encode(in[y]);
    }

    /* retun length */
    *outlen = x;

    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/ia5/der_encode_ia5_string.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_encode_integer.c
   ASN.1 DER, encode an integer, Tom St Denis
 */


#ifdef LTC_DER

/* Exports a positive bignum as DER format (upto 2^32 bytes in size) */

/**
   Store a mp_int integer
   @param num      The first mp_int to encode
   @param out      [out] The destination for the DER encoded integers
   @param outlen   [in/out] The max size and resulting size of the DER encoded integers
   @return CRYPT_OK if successful
 */
int der_encode_integer(void *num, unsigned char *out, unsigned long *outlen) {
    unsigned long tmplen, y;
    int           err, leading_zero;

    LTC_ARGCHK(num != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* find out how big this will be */
    if ((err = der_length_integer(num, &tmplen)) != CRYPT_OK) {
        return err;
    }

    if (*outlen < tmplen) {
        *outlen = tmplen;
        return CRYPT_BUFFER_OVERFLOW;
    }

    if (mp_cmp_d(num, 0) != LTC_MP_LT) {
        /* we only need a leading zero if the msb of the first byte is one */
        if (((mp_count_bits(num) & 7) == 0) || (mp_iszero(num) == LTC_MP_YES)) {
            leading_zero = 1;
        } else {
            leading_zero = 0;
        }

        /* get length of num in bytes (plus 1 since we force the msbyte to zero) */
        y = mp_unsigned_bin_size(num) + leading_zero;
    } else {
        leading_zero = 0;
        y            = mp_count_bits(num);
        y            = y + (8 - (y & 7));
        y            = y >> 3;
        if (((mp_cnt_lsb(num) + 1) == mp_count_bits(num)) && ((mp_count_bits(num) & 7) == 0)) --y;
    }

    /* now store initial data */
    *out++ = 0x02;
    if (y < 128) {
        /* short form */
        *out++ = (unsigned char)y;
    } else if (y < 256) {
        *out++ = 0x81;
        *out++ = (unsigned char)y;
    } else if (y < 65536UL) {
        *out++ = 0x82;
        *out++ = (unsigned char)((y >> 8) & 255);
        *out++ = (unsigned char)y;
    } else if (y < 16777216UL) {
        *out++ = 0x83;
        *out++ = (unsigned char)((y >> 16) & 255);
        *out++ = (unsigned char)((y >> 8) & 255);
        *out++ = (unsigned char)y;
    } else {
        return CRYPT_INVALID_ARG;
    }

    /* now store msbyte of zero if num is non-zero */
    if (leading_zero) {
        *out++ = 0x00;
    }

    /* if it's not zero store it as big endian */
    if (mp_cmp_d(num, 0) == LTC_MP_GT) {
        /* now store the mpint */
        if ((err = mp_to_unsigned_bin(num, out)) != CRYPT_OK) {
            return err;
        }
    } else if (mp_iszero(num) != LTC_MP_YES) {
        void *tmp;

        /* negative */
        if (mp_init(&tmp) != CRYPT_OK) {
            return CRYPT_MEM;
        }

        /* 2^roundup and subtract */
        y = mp_count_bits(num);
        y = y + (8 - (y & 7));
        if (((mp_cnt_lsb(num) + 1) == mp_count_bits(num)) && ((mp_count_bits(num) & 7) == 0)) y -= 8;
        if ((mp_2expt(tmp, y) != CRYPT_OK) || (mp_add(tmp, num, tmp) != CRYPT_OK)) {
            mp_clear(tmp);
            return CRYPT_MEM;
        }
        if ((err = mp_to_unsigned_bin(tmp, out)) != CRYPT_OK) {
            mp_clear(tmp);
            return err;
        }
        mp_clear(tmp);
    }

    /* we good */
    *outlen = tmplen;
    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/integer/der_encode_integer.c,v $ */
/* $Revision: 1.9 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_encode_object_identifier.c
   ASN.1 DER, Encode Object Identifier, Tom St Denis
 */

#ifdef LTC_DER

/**
   Encode an OID
   @param words   The words to encode  (upto 32-bits each)
   @param nwords  The number of words in the OID
   @param out     [out] Destination of OID data
   @param outlen  [in/out] The max and resulting size of the OID
   @return CRYPT_OK if successful
 */
int der_encode_object_identifier(unsigned long *words, unsigned long nwords,
                                 unsigned char *out, unsigned long *outlen) {
    unsigned long i, x, y, z, t, mask, wordbuf;
    int           err;

    LTC_ARGCHK(words != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* check length */
    if ((err = der_length_object_identifier(words, nwords, &x)) != CRYPT_OK) {
        return err;
    }
    if (x > *outlen) {
        *outlen = x;
        return CRYPT_BUFFER_OVERFLOW;
    }

    /* compute length to store OID data */
    z       = 0;
    wordbuf = words[0] * 40 + words[1];
    for (y = 1; y < nwords; y++) {
        t  = der_object_identifier_bits(wordbuf);
        z += t / 7 + ((t % 7) ? 1 : 0) + (wordbuf == 0 ? 1 : 0);
        if (y < nwords - 1) {
            wordbuf = words[y + 1];
        }
    }

    /* store header + length */
    x        = 0;
    out[x++] = 0x06;
    if (z < 128) {
        out[x++] = (unsigned char)z;
    } else if (z < 256) {
        out[x++] = 0x81;
        out[x++] = (unsigned char)z;
    } else if (z < 65536UL) {
        out[x++] = 0x82;
        out[x++] = (unsigned char)((z >> 8) & 255);
        out[x++] = (unsigned char)(z & 255);
    } else {
        return CRYPT_INVALID_ARG;
    }

    /* store first byte */
    wordbuf = words[0] * 40 + words[1];
    for (i = 1; i < nwords; i++) {
        /* store 7 bit words in little endian */
        t = wordbuf & 0xFFFFFFFF;
        if (t) {
            y    = x;
            mask = 0;
            while (t) {
                out[x++] = (unsigned char)((t & 0x7F) | mask);
                t      >>= 7;
                mask    |= 0x80; /* upper bit is set on all but the last byte */
            }
            /* now swap bytes y...x-1 */
            z = x - 1;
            while (y < z) {
                t      = out[y];
                out[y] = out[z];
                out[z] = (unsigned char)t;
                ++y;
                --z;
            }
        } else {
            /* zero word */
            out[x++] = 0x00;
        }

        if (i < nwords - 1) {
            wordbuf = words[i + 1];
        }
    }

    *outlen = x;
    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/object_identifier/der_encode_object_identifier.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_encode_octet_string.c
   ASN.1 DER, encode a OCTET STRING, Tom St Denis
 */


#ifdef LTC_DER

/**
   Store an OCTET STRING
   @param in       The array of OCTETS to store (one per char)
   @param inlen    The number of OCTETS to store
   @param out      [out] The destination for the DER encoded OCTET STRING
   @param outlen   [in/out] The max size and resulting size of the DER OCTET STRING
   @return CRYPT_OK if successful
 */
int der_encode_octet_string(const unsigned char *in, unsigned long inlen,
                            unsigned char *out, unsigned long *outlen) {
    unsigned long x, y, len;
    int           err;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* get the size */
    if ((err = der_length_octet_string(inlen, &len)) != CRYPT_OK) {
        return err;
    }

    /* too big? */
    if (len > *outlen) {
        *outlen = len;
        return CRYPT_BUFFER_OVERFLOW;
    }

    /* encode the header+len */
    x        = 0;
    out[x++] = 0x04;
    if (inlen < 128) {
        out[x++] = (unsigned char)inlen;
    } else if (inlen < 256) {
        out[x++] = 0x81;
        out[x++] = (unsigned char)inlen;
    } else if (inlen < 65536UL) {
        out[x++] = 0x82;
        out[x++] = (unsigned char)((inlen >> 8) & 255);
        out[x++] = (unsigned char)(inlen & 255);
    } else if (inlen < 16777216UL) {
        out[x++] = 0x83;
        out[x++] = (unsigned char)((inlen >> 16) & 255);
        out[x++] = (unsigned char)((inlen >> 8) & 255);
        out[x++] = (unsigned char)(inlen & 255);
    } else {
        return CRYPT_INVALID_ARG;
    }

    /* store octets */
    for (y = 0; y < inlen; y++) {
        out[x++] = in[y];
    }

    /* retun length */
    *outlen = x;

    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/octet/der_encode_octet_string.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_encode_printable_string.c
   ASN.1 DER, encode a printable STRING, Tom St Denis
 */

#ifdef LTC_DER

/**
   Store an printable STRING
   @param in       The array of printable to store (one per char)
   @param inlen    The number of printable to store
   @param out      [out] The destination for the DER encoded printable STRING
   @param outlen   [in/out] The max size and resulting size of the DER printable STRING
   @return CRYPT_OK if successful
 */
int der_encode_printable_string(const unsigned char *in, unsigned long inlen,
                                unsigned char *out, unsigned long *outlen) {
    unsigned long x, y, len;
    int           err;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* get the size */
    if ((err = der_length_printable_string(in, inlen, &len)) != CRYPT_OK) {
        return err;
    }

    /* too big? */
    if (len > *outlen) {
        *outlen = len;
        return CRYPT_BUFFER_OVERFLOW;
    }

    /* encode the header+len */
    x        = 0;
    out[x++] = 0x13;
    if (inlen < 128) {
        out[x++] = (unsigned char)inlen;
    } else if (inlen < 256) {
        out[x++] = 0x81;
        out[x++] = (unsigned char)inlen;
    } else if (inlen < 65536UL) {
        out[x++] = 0x82;
        out[x++] = (unsigned char)((inlen >> 8) & 255);
        out[x++] = (unsigned char)(inlen & 255);
    } else if (inlen < 16777216UL) {
        out[x++] = 0x83;
        out[x++] = (unsigned char)((inlen >> 16) & 255);
        out[x++] = (unsigned char)((inlen >> 8) & 255);
        out[x++] = (unsigned char)(inlen & 255);
    } else {
        return CRYPT_INVALID_ARG;
    }

    /* store octets */
    for (y = 0; y < inlen; y++) {
        out[x++] = der_printable_char_encode(in[y]);
    }

    /* retun length */
    *outlen = x;

    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/printable_string/der_encode_printable_string.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

#include <stdarg.h>


/**
   @file der_encode_sequence_ex.c
   ASN.1 DER, encode a SEQUENCE, Tom St Denis
 */

#ifdef LTC_DER

/**
   Encode a SEQUENCE
   @param list      The list of items to encode
   @param inlen     The number of items in the list
   @param out       [out] The destination
   @param outlen    [in/out] The size of the output
   @param type_of   LTC_ASN1_SEQUENCE or LTC_ASN1_SET/LTC_ASN1_SETOF
   @return CRYPT_OK on success
 */
int der_encode_sequence_ex(ltc_asn1_list *list, unsigned long inlen,
                           unsigned char *out, unsigned long *outlen, int type_of) {
    int           err, type;
    unsigned long size, x, y, z, i;
    void          *data;

    LTC_ARGCHK(list != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* get size of output that will be required */
    y = 0;
    for (i = 0; i < inlen; i++) {
        type = list[i].type;
        size = list[i].size;
        data = list[i].data;

        if (type == LTC_ASN1_EOL) {
            break;
        }

        switch (type) {
            case LTC_ASN1_BOOLEAN:
                if ((err = der_length_boolean(&x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_INTEGER:
                if ((err = der_length_integer(data, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_SHORT_INTEGER:
                if ((err = der_length_short_integer(*((unsigned long *)data), &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_BIT_STRING:
                if ((err = der_length_bit_string(size, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_OCTET_STRING:
                if ((err = der_length_octet_string(size, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_NULL:
                y += 2;
                break;

            case LTC_ASN1_OBJECT_IDENTIFIER:
                if ((err = der_length_object_identifier(data, size, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_IA5_STRING:
                if ((err = der_length_ia5_string(data, size, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_PRINTABLE_STRING:
                if ((err = der_length_printable_string(data, size, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_UTF8_STRING:
                if ((err = der_length_utf8_string(data, size, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_UTCTIME:
                if ((err = der_length_utctime(data, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_SET:
            case LTC_ASN1_SETOF:
            case LTC_ASN1_SEQUENCE:
                if ((err = der_length_sequence(data, size, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            default:
                err = CRYPT_INVALID_ARG;
                goto LBL_ERR;
        }
    }

    /* calc header size */
    z = y;
    if (y < 128) {
        y += 2;
    } else if (y < 256) {
        /* 0x30 0x81 LL */
        y += 3;
    } else if (y < 65536UL) {
        /* 0x30 0x82 LL LL */
        y += 4;
    } else if (y < 16777216UL) {
        /* 0x30 0x83 LL LL LL */
        y += 5;
    } else {
        err = CRYPT_INVALID_ARG;
        goto LBL_ERR;
    }

    /* too big ? */
    if (*outlen < y) {
        *outlen = y;
        err     = CRYPT_BUFFER_OVERFLOW;
        goto LBL_ERR;
    }

    /* store header */
    x        = 0;
    out[x++] = (type_of == LTC_ASN1_SEQUENCE) ? 0x30 : 0x31;

    if (z < 128) {
        out[x++] = (unsigned char)z;
    } else if (z < 256) {
        out[x++] = 0x81;
        out[x++] = (unsigned char)z;
    } else if (z < 65536UL) {
        out[x++] = 0x82;
        out[x++] = (unsigned char)((z >> 8UL) & 255);
        out[x++] = (unsigned char)(z & 255);
    } else if (z < 16777216UL) {
        out[x++] = 0x83;
        out[x++] = (unsigned char)((z >> 16UL) & 255);
        out[x++] = (unsigned char)((z >> 8UL) & 255);
        out[x++] = (unsigned char)(z & 255);
    }

    /* store data */
    *outlen -= x;
    for (i = 0; i < inlen; i++) {
        type = list[i].type;
        size = list[i].size;
        data = list[i].data;

        if (type == LTC_ASN1_EOL) {
            break;
        }

        switch (type) {
            case LTC_ASN1_BOOLEAN:
                z = *outlen;
                if ((err = der_encode_boolean(*((int *)data), out + x, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                x       += z;
                *outlen -= z;
                break;

            case LTC_ASN1_INTEGER:
                z = *outlen;
                if ((err = der_encode_integer(data, out + x, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                x       += z;
                *outlen -= z;
                break;

            case LTC_ASN1_SHORT_INTEGER:
                z = *outlen;
                if ((err = der_encode_short_integer(*((unsigned long *)data), out + x, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                x       += z;
                *outlen -= z;
                break;

            case LTC_ASN1_BIT_STRING:
                z = *outlen;
                if ((err = der_encode_bit_string(data, size, out + x, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                x       += z;
                *outlen -= z;
                break;

            case LTC_ASN1_OCTET_STRING:
                z = *outlen;
                if ((err = der_encode_octet_string(data, size, out + x, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                x       += z;
                *outlen -= z;
                break;

            case LTC_ASN1_NULL:
                out[x++] = 0x05;
                out[x++] = 0x00;
                *outlen -= 2;
                break;

            case LTC_ASN1_OBJECT_IDENTIFIER:
                z = *outlen;
                if ((err = der_encode_object_identifier(data, size, out + x, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                x       += z;
                *outlen -= z;
                break;

            case LTC_ASN1_IA5_STRING:
                z = *outlen;
                if ((err = der_encode_ia5_string(data, size, out + x, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                x       += z;
                *outlen -= z;
                break;

            case LTC_ASN1_PRINTABLE_STRING:
                z = *outlen;
                if ((err = der_encode_printable_string(data, size, out + x, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                x       += z;
                *outlen -= z;
                break;

            case LTC_ASN1_UTF8_STRING:
                z = *outlen;
                if ((err = der_encode_utf8_string(data, size, out + x, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                x       += z;
                *outlen -= z;
                break;

            case LTC_ASN1_UTCTIME:
                z = *outlen;
                if ((err = der_encode_utctime(data, out + x, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                x       += z;
                *outlen -= z;
                break;

            case LTC_ASN1_SET:
                z = *outlen;
                if ((err = der_encode_set(data, size, out + x, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                x       += z;
                *outlen -= z;
                break;

            case LTC_ASN1_SETOF:
                z = *outlen;
                if ((err = der_encode_setof(data, size, out + x, &z)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                x       += z;
                *outlen -= z;
                break;

            case LTC_ASN1_SEQUENCE:
                z = *outlen;
                if ((err = der_encode_sequence_ex(data, size, out + x, &z, type)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                x       += z;
                *outlen -= z;
                break;

            default:
                err = CRYPT_INVALID_ARG;
                goto LBL_ERR;
        }
    }
    *outlen = x;
    err     = CRYPT_OK;

LBL_ERR:
    return err;
}
#endif


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

#include <stdarg.h>


/**
   @file der_encode_sequence_multi.c
   ASN.1 DER, encode a SEQUENCE, Tom St Denis
 */

#ifdef LTC_DER

/**
   Encode a SEQUENCE type using a VA list
   @param out    [out] Destination for data
   @param outlen [in/out] Length of buffer and resulting length of output
   @remark <...> is of the form <type, size, data> (int, unsigned long, void*)
   @return CRYPT_OK on success
 */
int der_encode_sequence_multi(unsigned char *out, unsigned long *outlen, ...) {
    int           err, type;
    unsigned long size, x;
    void          *data;
    va_list       args;
    ltc_asn1_list *list;

    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* get size of output that will be required */
    va_start(args, outlen);
    x = 0;
    for ( ; ; ) {
        type = va_arg(args, int);
        size = va_arg(args, unsigned long);
        data = va_arg(args, void *);

        if (type == LTC_ASN1_EOL) {
            break;
        }

        switch (type) {
            case LTC_ASN1_BOOLEAN:
            case LTC_ASN1_INTEGER:
            case LTC_ASN1_SHORT_INTEGER:
            case LTC_ASN1_BIT_STRING:
            case LTC_ASN1_OCTET_STRING:
            case LTC_ASN1_NULL:
            case LTC_ASN1_OBJECT_IDENTIFIER:
            case LTC_ASN1_IA5_STRING:
            case LTC_ASN1_PRINTABLE_STRING:
            case LTC_ASN1_UTF8_STRING:
            case LTC_ASN1_UTCTIME:
            case LTC_ASN1_SEQUENCE:
            case LTC_ASN1_SET:
            case LTC_ASN1_SETOF:
                ++x;
                break;

            default:
                va_end(args);
                return CRYPT_INVALID_ARG;
        }
    }
    va_end(args);

    /* allocate structure for x elements */
    if (x == 0) {
        return CRYPT_NOP;
    }

    list = XCALLOC(sizeof(*list), x);
    if (list == NULL) {
        return CRYPT_MEM;
    }

    /* fill in the structure */
    va_start(args, outlen);
    x = 0;
    for ( ; ; ) {
        type = va_arg(args, int);
        size = va_arg(args, unsigned long);
        data = va_arg(args, void *);

        if (type == LTC_ASN1_EOL) {
            break;
        }

        switch (type) {
            case LTC_ASN1_BOOLEAN:
            case LTC_ASN1_INTEGER:
            case LTC_ASN1_SHORT_INTEGER:
            case LTC_ASN1_BIT_STRING:
            case LTC_ASN1_OCTET_STRING:
            case LTC_ASN1_NULL:
            case LTC_ASN1_OBJECT_IDENTIFIER:
            case LTC_ASN1_IA5_STRING:
            case LTC_ASN1_PRINTABLE_STRING:
            case LTC_ASN1_UTF8_STRING:
            case LTC_ASN1_UTCTIME:
            case LTC_ASN1_SEQUENCE:
            case LTC_ASN1_SET:
            case LTC_ASN1_SETOF:
                list[x].type   = type;
                list[x].size   = size;
                list[x++].data = data;
                break;

            default:
                va_end(args);
                err = CRYPT_INVALID_ARG;
                goto LBL_ERR;
        }
    }
    va_end(args);

    err = der_encode_sequence(list, x, out, outlen);
LBL_ERR:
    XFREE(list);
    return err;
}
#endif


/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/sequence/der_encode_sequence_multi.c,v $ */
/* $Revision: 1.12 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_encode_set.c
   ASN.1 DER, Encode a SET, Tom St Denis
 */

#ifdef LTC_DER

/* LTC define to ASN.1 TAG */
static int ltc_to_asn1(int v) {
    switch (v) {
        case LTC_ASN1_BOOLEAN:
            return 0x01;

        case LTC_ASN1_INTEGER:
        case LTC_ASN1_SHORT_INTEGER:
            return 0x02;

        case LTC_ASN1_BIT_STRING:
            return 0x03;

        case LTC_ASN1_OCTET_STRING:
            return 0x04;

        case LTC_ASN1_NULL:
            return 0x05;

        case LTC_ASN1_OBJECT_IDENTIFIER:
            return 0x06;

        case LTC_ASN1_UTF8_STRING:
            return 0x0C;

        case LTC_ASN1_PRINTABLE_STRING:
            return 0x13;

        case LTC_ASN1_IA5_STRING:
            return 0x16;

        case LTC_ASN1_UTCTIME:
            return 0x17;

        case LTC_ASN1_SEQUENCE:
            return 0x30;

        case LTC_ASN1_SET:
        case LTC_ASN1_SETOF:
            return 0x31;

        default:
            return -1;
    }
}

static int qsort_helper_set(const void *a, const void *b) {
    ltc_asn1_list *A = (ltc_asn1_list *)a, *B = (ltc_asn1_list *)b;
    int           r;

    r = ltc_to_asn1(A->type) - ltc_to_asn1(B->type);

    /* for QSORT the order is UNDEFINED if they are "equal" which means it is NOT DETERMINISTIC.  So we force it to be :-) */
    if (r == 0) {
        /* their order in the original list now determines the position */
        return A->used - B->used;
    } else {
        return r;
    }
}

/*
   Encode a SET type
   @param list      The list of items to encode
   @param inlen     The number of items in the list
   @param out       [out] The destination
   @param outlen    [in/out] The size of the output
   @return CRYPT_OK on success
 */
int der_encode_set(ltc_asn1_list *list, unsigned long inlen,
                   unsigned char *out, unsigned long *outlen) {
    ltc_asn1_list *copy;
    unsigned long x;
    int           err;

    /* make copy of list */
    copy = XCALLOC(inlen, sizeof(*copy));
    if (copy == NULL) {
        return CRYPT_MEM;
    }

    /* fill in used member with index so we can fully sort it */
    for (x = 0; x < inlen; x++) {
        copy[x]      = list[x];
        copy[x].used = x;
    }

    /* sort it by the "type" field */
    XQSORT(copy, inlen, sizeof(*copy), &qsort_helper_set);

    /* call der_encode_sequence_ex() */
    err = der_encode_sequence_ex(copy, inlen, out, outlen, LTC_ASN1_SET);

    /* free list */
    XFREE(copy);

    return err;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/set/der_encode_set.c,v $ */
/* $Revision: 1.12 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_encode_setof.c
   ASN.1 DER, Encode SET OF, Tom St Denis
 */

#ifdef LTC_DER

struct edge {
    unsigned char *start;
    unsigned long size;
};

static int qsort_helper(const void *a, const void *b) {
    struct edge   *A = (struct edge *)a, *B = (struct edge *)b;
    int           r;
    unsigned long x;

    /* compare min length */
    r = XMEMCMP(A->start, B->start, MIN(A->size, B->size));

    if ((r == 0) && (A->size != B->size)) {
        if (A->size > B->size) {
            for (x = B->size; x < A->size; x++) {
                if (A->start[x]) {
                    return 1;
                }
            }
        } else {
            for (x = A->size; x < B->size; x++) {
                if (B->start[x]) {
                    return -1;
                }
            }
        }
    }

    return r;
}

/**
   Encode a SETOF stucture
   @param list      The list of items to encode
   @param inlen     The number of items in the list
   @param out       [out] The destination
   @param outlen    [in/out] The size of the output
   @return CRYPT_OK on success
 */
int der_encode_setof(ltc_asn1_list *list, unsigned long inlen,
                     unsigned char *out, unsigned long *outlen) {
    unsigned long x, y, z, hdrlen;
    int           err;
    struct edge   *edges;
    unsigned char *ptr, *buf;

    /* check that they're all the same type */
    for (x = 1; x < inlen; x++) {
        if (list[x].type != list[x - 1].type) {
            return CRYPT_INVALID_ARG;
        }
    }

    /* alloc buffer to store copy of output */
    buf = XCALLOC(1, *outlen);
    if (buf == NULL) {
        return CRYPT_MEM;
    }

    /* encode list */
    if ((err = der_encode_sequence_ex(list, inlen, buf, outlen, LTC_ASN1_SETOF)) != CRYPT_OK) {
        XFREE(buf);
        return err;
    }

    /* allocate edges */
    edges = XCALLOC(inlen, sizeof(*edges));
    if (edges == NULL) {
        XFREE(buf);
        return CRYPT_MEM;
    }

    /* skip header */
    ptr = buf + 1;

    /* now skip length data */
    x = *ptr++;
    if (x >= 0x80) {
        ptr += (x & 0x7F);
    }

    /* get the size of the static header */
    hdrlen = ((unsigned long)ptr) - ((unsigned long)buf);


    /* scan for edges */
    x = 0;
    while (ptr < (buf + *outlen)) {
        /* store start */
        edges[x].start = ptr;

        /* skip type */
        z = 1;

        /* parse length */
        y = ptr[z++];
        if (y < 128) {
            edges[x].size = y;
        } else {
            y            &= 0x7F;
            edges[x].size = 0;
            while (y--) {
                edges[x].size = (edges[x].size << 8) | ((unsigned long)ptr[z++]);
            }
        }

        /* skip content */
        edges[x].size += z;
        ptr           += edges[x].size;
        ++x;
    }

    /* sort based on contents (using edges) */
    XQSORT(edges, inlen, sizeof(*edges), &qsort_helper);

    /* copy static header */
    XMEMCPY(out, buf, hdrlen);

    /* copy+sort using edges+indecies to output from buffer */
    for (y = hdrlen, x = 0; x < inlen; x++) {
        XMEMCPY(out + y, edges[x].start, edges[x].size);
        y += edges[x].size;
    }

 #ifdef LTC_CLEAN_STACK
    zeromem(buf, *outlen);
 #endif

    /* free buffers */
    XFREE(edges);
    XFREE(buf);

    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/set/der_encode_setof.c,v $ */
/* $Revision: 1.12 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_encode_short_integer.c
   ASN.1 DER, encode an integer, Tom St Denis
 */


#ifdef LTC_DER

/**
   Store a short integer in the range (0,2^32-1)
   @param num      The integer to encode
   @param out      [out] The destination for the DER encoded integers
   @param outlen   [in/out] The max size and resulting size of the DER encoded integers
   @return CRYPT_OK if successful
 */
int der_encode_short_integer(unsigned long num, unsigned char *out, unsigned long *outlen) {
    unsigned long len, x, y, z;
    int           err;

    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* force to 32 bits */
    num &= 0xFFFFFFFFUL;

    /* find out how big this will be */
    if ((err = der_length_short_integer(num, &len)) != CRYPT_OK) {
        return err;
    }

    if (*outlen < len) {
        *outlen = len;
        return CRYPT_BUFFER_OVERFLOW;
    }

    /* get len of output */
    z = 0;
    y = num;
    while (y) {
        ++z;
        y >>= 8;
    }

    /* handle zero */
    if (z == 0) {
        z = 1;
    }

    /* see if msb is set */
    z += (num & (1UL << ((z << 3) - 1))) ? 1 : 0;

    /* adjust the number so the msB is non-zero */
    for (x = 0; (z <= 4) && (x < (4 - z)); x++) {
        num <<= 8;
    }

    /* store header */
    x        = 0;
    out[x++] = 0x02;
    out[x++] = (unsigned char)z;

    /* if 31st bit is set output a leading zero and decrement count */
    if (z == 5) {
        out[x++] = 0;
        --z;
    }

    /* store values */
    for (y = 0; y < z; y++) {
        out[x++] = (unsigned char)((num >> 24) & 0xFF);
        num    <<= 8;
    }

    /* we good */
    *outlen = x;

    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/short_integer/der_encode_short_integer.c,v $ */
/* $Revision: 1.8 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_encode_utctime.c
   ASN.1 DER, encode a  UTCTIME, Tom St Denis
 */

#ifdef LTC_DER

static const char baseten[] = "0123456789";

 #define STORE_V(y)                                         \
    out[x++] = der_ia5_char_encode(baseten[(y / 10) % 10]); \
    out[x++] = der_ia5_char_encode(baseten[y % 10]);

/**
   Encodes a UTC time structure in DER format
   @param utctime      The UTC time structure to encode
   @param out          The destination of the DER encoding of the UTC time structure
   @param outlen       [in/out] The length of the DER encoding
   @return CRYPT_OK if successful
 */
int der_encode_utctime(ltc_utctime *utctime,
                       unsigned char *out, unsigned long *outlen) {
    unsigned long x, tmplen;
    int           err;

    LTC_ARGCHK(utctime != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    if ((err = der_length_utctime(utctime, &tmplen)) != CRYPT_OK) {
        return err;
    }
    if (tmplen > *outlen) {
        *outlen = tmplen;
        return CRYPT_BUFFER_OVERFLOW;
    }

    /* store header */
    out[0] = 0x17;

    /* store values */
    x = 2;
    STORE_V(utctime->YY);
    STORE_V(utctime->MM);
    STORE_V(utctime->DD);
    STORE_V(utctime->hh);
    STORE_V(utctime->mm);
    STORE_V(utctime->ss);

    if (utctime->off_mm || utctime->off_hh) {
        out[x++] = der_ia5_char_encode(utctime->off_dir ? '-' : '+');
        STORE_V(utctime->off_hh);
        STORE_V(utctime->off_mm);
    } else {
        out[x++] = der_ia5_char_encode('Z');
    }

    /* store length */
    out[1] = (unsigned char)(x - 2);

    /* all good let's return */
    *outlen = x;
    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/utctime/der_encode_utctime.c,v $ */
/* $Revision: 1.10 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_encode_utf8_string.c
   ASN.1 DER, encode a UTF8 STRING, Tom St Denis
 */


#ifdef LTC_DER

/**
   Store an UTF8 STRING
   @param in       The array of UTF8 to store (one per wchar_t)
   @param inlen    The number of UTF8 to store
   @param out      [out] The destination for the DER encoded UTF8 STRING
   @param outlen   [in/out] The max size and resulting size of the DER UTF8 STRING
   @return CRYPT_OK if successful
 */
int der_encode_utf8_string(const wchar_t *in, unsigned long inlen,
                           unsigned char *out, unsigned long *outlen) {
    unsigned long x, y, len;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* get the size */
    for (x = len = 0; x < inlen; x++) {
        if ((in[x] < 0) || (in[x] > 0x1FFFF)) {
            return CRYPT_INVALID_ARG;
        }
        len += der_utf8_charsize(in[x]);
    }

    if (len < 128) {
        y = 2 + len;
    } else if (len < 256) {
        y = 3 + len;
    } else if (len < 65536UL) {
        y = 4 + len;
    } else if (len < 16777216UL) {
        y = 5 + len;
    } else {
        return CRYPT_INVALID_ARG;
    }

    /* too big? */
    if (y > *outlen) {
        *outlen = len;
        return CRYPT_BUFFER_OVERFLOW;
    }

    /* encode the header+len */
    x        = 0;
    out[x++] = 0x0C;
    if (len < 128) {
        out[x++] = (unsigned char)len;
    } else if (len < 256) {
        out[x++] = 0x81;
        out[x++] = (unsigned char)len;
    } else if (len < 65536UL) {
        out[x++] = 0x82;
        out[x++] = (unsigned char)((len >> 8) & 255);
        out[x++] = (unsigned char)(len & 255);
    } else if (len < 16777216UL) {
        out[x++] = 0x83;
        out[x++] = (unsigned char)((len >> 16) & 255);
        out[x++] = (unsigned char)((len >> 8) & 255);
        out[x++] = (unsigned char)(len & 255);
    } else {
        return CRYPT_INVALID_ARG;
    }

    /* store UTF8 */
    for (y = 0; y < inlen; y++) {
        switch (der_utf8_charsize(in[y])) {
            case 1:
                out[x++] = (unsigned char)in[y];
                break;

            case 2:
                out[x++] = 0xC0 | ((in[y] >> 6) & 0x1F);
                out[x++] = 0x80 | (in[y] & 0x3F);
                break;

            case 3:
                out[x++] = 0xE0 | ((in[y] >> 12) & 0x0F);
                out[x++] = 0x80 | ((in[y] >> 6) & 0x3F);
                out[x++] = 0x80 | (in[y] & 0x3F);
                break;

            case 4:
                out[x++] = 0xF0 | ((in[y] >> 18) & 0x07);
                out[x++] = 0x80 | ((in[y] >> 12) & 0x3F);
                out[x++] = 0x80 | ((in[y] >> 6) & 0x3F);
                out[x++] = 0x80 | (in[y] & 0x3F);
                break;
        }
    }

    /* retun length */
    *outlen = x;

    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/utf8/der_encode_utf8_string.c,v $ */
/* $Revision: 1.9 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_length_bit_string.c
   ASN.1 DER, get length of BIT STRING, Tom St Denis
 */

#ifdef LTC_DER

/**
   Gets length of DER encoding of BIT STRING
   @param nbits  The number of bits in the string to encode
   @param outlen [out] The length of the DER encoding for the given string
   @return CRYPT_OK if successful
 */
int der_length_bit_string(unsigned long nbits, unsigned long *outlen) {
    unsigned long nbytes;

    LTC_ARGCHK(outlen != NULL);

    /* get the number of the bytes */
    nbytes = (nbits >> 3) + ((nbits & 7) ? 1 : 0) + 1;

    if (nbytes < 128) {
        /* 03 LL PP DD DD DD ... */
        *outlen = 2 + nbytes;
    } else if (nbytes < 256) {
        /* 03 81 LL PP DD DD DD ... */
        *outlen = 3 + nbytes;
    } else if (nbytes < 65536) {
        /* 03 82 LL LL PP DD DD DD ... */
        *outlen = 4 + nbytes;
    } else {
        return CRYPT_INVALID_ARG;
    }

    return CRYPT_OK;
}
#endif


/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/bit/der_length_bit_string.c,v $ */
/* $Revision: 1.3 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_length_boolean.c
   ASN.1 DER, get length of a BOOLEAN, Tom St Denis
 */

#ifdef LTC_DER

/**
   Gets length of DER encoding of a BOOLEAN
   @param outlen [out] The length of the DER encoding
   @return CRYPT_OK if successful
 */
int der_length_boolean(unsigned long *outlen) {
    LTC_ARGCHK(outlen != NULL);
    *outlen = 3;
    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/boolean/der_length_boolean.c,v $ */
/* $Revision: 1.3 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_length_ia5_string.c
   ASN.1 DER, get length of IA5 STRING, Tom St Denis
 */

#ifdef LTC_DER

static const struct {
    int code, value;
} ia5_table[] = {
    { '\0',   0 },
    { '\a',   7 },
    { '\b',   8 },
    { '\t',   9 },
    { '\n',  10 },
    { '\f',  12 },
    { '\r',  13 },
    { ' ',   32 },
    { '!',   33 },
    { '"',   34 },
    { '#',   35 },
    { '$',   36 },
    { '%',   37 },
    { '&',   38 },
    { '\'',  39 },
    { '(',   40 },
    { ')',   41 },
    { '*',   42 },
    { '+',   43 },
    { ',',   44 },
    { '-',   45 },
    { '.',   46 },
    { '/',   47 },
    { '0',   48 },
    { '1',   49 },
    { '2',   50 },
    { '3',   51 },
    { '4',   52 },
    { '5',   53 },
    { '6',   54 },
    { '7',   55 },
    { '8',   56 },
    { '9',   57 },
    { ':',   58 },
    { ';',   59 },
    { '<',   60 },
    { '=',   61 },
    { '>',   62 },
    { '?',   63 },
    { '@',   64 },
    { 'A',   65 },
    { 'B',   66 },
    { 'C',   67 },
    { 'D',   68 },
    { 'E',   69 },
    { 'F',   70 },
    { 'G',   71 },
    { 'H',   72 },
    { 'I',   73 },
    { 'J',   74 },
    { 'K',   75 },
    { 'L',   76 },
    { 'M',   77 },
    { 'N',   78 },
    { 'O',   79 },
    { 'P',   80 },
    { 'Q',   81 },
    { 'R',   82 },
    { 'S',   83 },
    { 'T',   84 },
    { 'U',   85 },
    { 'V',   86 },
    { 'W',   87 },
    { 'X',   88 },
    { 'Y',   89 },
    { 'Z',   90 },
    { '[',   91 },
    { '\\',  92 },
    { ']',   93 },
    { '^',   94 },
    { '_',   95 },
    { '`',   96 },
    { 'a',   97 },
    { 'b',   98 },
    { 'c',   99 },
    { 'd',  100 },
    { 'e',  101 },
    { 'f',  102 },
    { 'g',  103 },
    { 'h',  104 },
    { 'i',  105 },
    { 'j',  106 },
    { 'k',  107 },
    { 'l',  108 },
    { 'm',  109 },
    { 'n',  110 },
    { 'o',  111 },
    { 'p',  112 },
    { 'q',  113 },
    { 'r',  114 },
    { 's',  115 },
    { 't',  116 },
    { 'u',  117 },
    { 'v',  118 },
    { 'w',  119 },
    { 'x',  120 },
    { 'y',  121 },
    { 'z',  122 },
    { '{',  123 },
    { '|',  124 },
    { '}',  125 },
    { '~',  126 }
};

int der_ia5_char_encode(int c) {
    int x;

    for (x = 0; x < (int)(sizeof(ia5_table) / sizeof(ia5_table[0])); x++) {
        if (ia5_table[x].code == c) {
            return ia5_table[x].value;
        }
    }
    return -1;
}

int der_ia5_value_decode(int v) {
    int x;

    for (x = 0; x < (int)(sizeof(ia5_table) / sizeof(ia5_table[0])); x++) {
        if (ia5_table[x].value == v) {
            return ia5_table[x].code;
        }
    }
    return -1;
}

/**
   Gets length of DER encoding of IA5 STRING
   @param octets   The values you want to encode
   @param noctets  The number of octets in the string to encode
   @param outlen   [out] The length of the DER encoding for the given string
   @return CRYPT_OK if successful
 */
int der_length_ia5_string(const unsigned char *octets, unsigned long noctets, unsigned long *outlen) {
    unsigned long x;

    LTC_ARGCHK(outlen != NULL);
    LTC_ARGCHK(octets != NULL);

    /* scan string for validity */
    for (x = 0; x < noctets; x++) {
        if (der_ia5_char_encode(octets[x]) == -1) {
            return CRYPT_INVALID_ARG;
        }
    }

    if (noctets < 128) {
        /* 16 LL DD DD DD ... */
        *outlen = 2 + noctets;
    } else if (noctets < 256) {
        /* 16 81 LL DD DD DD ... */
        *outlen = 3 + noctets;
    } else if (noctets < 65536UL) {
        /* 16 82 LL LL DD DD DD ... */
        *outlen = 4 + noctets;
    } else if (noctets < 16777216UL) {
        /* 16 83 LL LL LL DD DD DD ... */
        *outlen = 5 + noctets;
    } else {
        return CRYPT_INVALID_ARG;
    }

    return CRYPT_OK;
}
#endif


/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/ia5/der_length_ia5_string.c,v $ */
/* $Revision: 1.3 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_length_integer.c
   ASN.1 DER, get length of encoding, Tom St Denis
 */


#ifdef LTC_DER

/**
   Gets length of DER encoding of num
   @param num    The int to get the size of
   @param outlen [out] The length of the DER encoding for the given integer
   @return CRYPT_OK if successful
 */
int der_length_integer(void *num, unsigned long *outlen) {
    unsigned long z, len;
    int           leading_zero;

    LTC_ARGCHK(num != NULL);
    LTC_ARGCHK(outlen != NULL);

    if (mp_cmp_d(num, 0) != LTC_MP_LT) {
        /* positive */

        /* we only need a leading zero if the msb of the first byte is one */
        if (((mp_count_bits(num) & 7) == 0) || (mp_iszero(num) == LTC_MP_YES)) {
            leading_zero = 1;
        } else {
            leading_zero = 0;
        }

        /* size for bignum */
        z = len = leading_zero + mp_unsigned_bin_size(num);
    } else {
        /* it's negative */
        /* find power of 2 that is a multiple of eight and greater than count bits */
        leading_zero = 0;
        z            = mp_count_bits(num);
        z            = z + (8 - (z & 7));
        if (((mp_cnt_lsb(num) + 1) == mp_count_bits(num)) && ((mp_count_bits(num) & 7) == 0)) --z;
        len = z = z >> 3;
    }

    /* now we need a length */
    if (z < 128) {
        /* short form */
        ++len;
    } else {
        /* long form (relies on z != 0), assumes length bytes < 128 */
        ++len;

        while (z) {
            ++len;
            z >>= 8;
        }
    }

    /* we need a 0x02 to indicate it's INTEGER */
    ++len;

    /* return length */
    *outlen = len;
    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/integer/der_length_integer.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_length_object_identifier.c
   ASN.1 DER, get length of Object Identifier, Tom St Denis
 */

#ifdef LTC_DER

unsigned long der_object_identifier_bits(unsigned long x) {
    unsigned long c;

    x &= 0xFFFFFFFF;
    c  = 0;
    while (x) {
        ++c;
        x >>= 1;
    }
    return c;
}

/**
   Gets length of DER encoding of Object Identifier
   @param nwords   The number of OID words
   @param words    The actual OID words to get the size of
   @param outlen   [out] The length of the DER encoding for the given string
   @return CRYPT_OK if successful
 */
int der_length_object_identifier(unsigned long *words, unsigned long nwords, unsigned long *outlen) {
    unsigned long y, z, t, wordbuf;

    LTC_ARGCHK(words != NULL);
    LTC_ARGCHK(outlen != NULL);


    /* must be >= 2 words */
    if (nwords < 2) {
        return CRYPT_INVALID_ARG;
    }

    /* word1 = 0,1,2,3 and word2 0..39 */
    if ((words[0] > 3) || ((words[0] < 2) && (words[1] > 39))) {
        return CRYPT_INVALID_ARG;
    }

    /* leading word is the first two */
    z       = 0;
    wordbuf = words[0] * 40 + words[1];
    for (y = 1; y < nwords; y++) {
        t  = der_object_identifier_bits(wordbuf);
        z += t / 7 + ((t % 7) ? 1 : 0) + (wordbuf == 0 ? 1 : 0);
        if (y < nwords - 1) {
            /* grab next word */
            wordbuf = words[y + 1];
        }
    }

    /* now depending on the length our length encoding changes */
    if (z < 128) {
        z += 2;
    } else if (z < 256) {
        z += 3;
    } else if (z < 65536UL) {
        z += 4;
    } else {
        return CRYPT_INVALID_ARG;
    }

    *outlen = z;
    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/object_identifier/der_length_object_identifier.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_length_octet_string.c
   ASN.1 DER, get length of OCTET STRING, Tom St Denis
 */

#ifdef LTC_DER

/**
   Gets length of DER encoding of OCTET STRING
   @param noctets  The number of octets in the string to encode
   @param outlen   [out] The length of the DER encoding for the given string
   @return CRYPT_OK if successful
 */
int der_length_octet_string(unsigned long noctets, unsigned long *outlen) {
    LTC_ARGCHK(outlen != NULL);

    if (noctets < 128) {
        /* 04 LL DD DD DD ... */
        *outlen = 2 + noctets;
    } else if (noctets < 256) {
        /* 04 81 LL DD DD DD ... */
        *outlen = 3 + noctets;
    } else if (noctets < 65536UL) {
        /* 04 82 LL LL DD DD DD ... */
        *outlen = 4 + noctets;
    } else if (noctets < 16777216UL) {
        /* 04 83 LL LL LL DD DD DD ... */
        *outlen = 5 + noctets;
    } else {
        return CRYPT_INVALID_ARG;
    }

    return CRYPT_OK;
}
#endif


/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/octet/der_length_octet_string.c,v $ */
/* $Revision: 1.3 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_length_printable_string.c
   ASN.1 DER, get length of Printable STRING, Tom St Denis
 */

#ifdef LTC_DER

static const struct {
    int code, value;
} printable_table[] = {
    { ' ',   32 },
    { '\'',  39 },
    { '(',   40 },
    { ')',   41 },
    { '+',   43 },
    { ',',   44 },
    { '-',   45 },
    { '.',   46 },
    { '/',   47 },
    { '0',   48 },
    { '1',   49 },
    { '2',   50 },
    { '3',   51 },
    { '4',   52 },
    { '5',   53 },
    { '6',   54 },
    { '7',   55 },
    { '8',   56 },
    { '9',   57 },
    { ':',   58 },
    { '=',   61 },
    { '?',   63 },
    { 'A',   65 },
    { 'B',   66 },
    { 'C',   67 },
    { 'D',   68 },
    { 'E',   69 },
    { 'F',   70 },
    { 'G',   71 },
    { 'H',   72 },
    { 'I',   73 },
    { 'J',   74 },
    { 'K',   75 },
    { 'L',   76 },
    { 'M',   77 },
    { 'N',   78 },
    { 'O',   79 },
    { 'P',   80 },
    { 'Q',   81 },
    { 'R',   82 },
    { 'S',   83 },
    { 'T',   84 },
    { 'U',   85 },
    { 'V',   86 },
    { 'W',   87 },
    { 'X',   88 },
    { 'Y',   89 },
    { 'Z',   90 },
    { 'a',   97 },
    { 'b',   98 },
    { 'c',   99 },
    { 'd',  100 },
    { 'e',  101 },
    { 'f',  102 },
    { 'g',  103 },
    { 'h',  104 },
    { 'i',  105 },
    { 'j',  106 },
    { 'k',  107 },
    { 'l',  108 },
    { 'm',  109 },
    { 'n',  110 },
    { 'o',  111 },
    { 'p',  112 },
    { 'q',  113 },
    { 'r',  114 },
    { 's',  115 },
    { 't',  116 },
    { 'u',  117 },
    { 'v',  118 },
    { 'w',  119 },
    { 'x',  120 },
    { 'y',  121 },
    { 'z',  122 },
};

int der_printable_char_encode(int c) {
    int x;

    for (x = 0; x < (int)(sizeof(printable_table) / sizeof(printable_table[0])); x++) {
        if (printable_table[x].code == c) {
            return printable_table[x].value;
        }
    }
    return -1;
}

int der_printable_value_decode(int v) {
    int x;

    for (x = 0; x < (int)(sizeof(printable_table) / sizeof(printable_table[0])); x++) {
        if (printable_table[x].value == v) {
            return printable_table[x].code;
        }
    }
    return -1;
}

/**
   Gets length of DER encoding of Printable STRING
   @param octets   The values you want to encode
   @param noctets  The number of octets in the string to encode
   @param outlen   [out] The length of the DER encoding for the given string
   @return CRYPT_OK if successful
 */
int der_length_printable_string(const unsigned char *octets, unsigned long noctets, unsigned long *outlen) {
    unsigned long x;

    LTC_ARGCHK(outlen != NULL);
    LTC_ARGCHK(octets != NULL);

    /* scan string for validity */
    for (x = 0; x < noctets; x++) {
        if (der_printable_char_encode(octets[x]) == -1) {
            return CRYPT_INVALID_ARG;
        }
    }

    if (noctets < 128) {
        /* 16 LL DD DD DD ... */
        *outlen = 2 + noctets;
    } else if (noctets < 256) {
        /* 16 81 LL DD DD DD ... */
        *outlen = 3 + noctets;
    } else if (noctets < 65536UL) {
        /* 16 82 LL LL DD DD DD ... */
        *outlen = 4 + noctets;
    } else if (noctets < 16777216UL) {
        /* 16 83 LL LL LL DD DD DD ... */
        *outlen = 5 + noctets;
    } else {
        return CRYPT_INVALID_ARG;
    }

    return CRYPT_OK;
}
#endif


/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/printable_string/der_length_printable_string.c,v $ */
/* $Revision: 1.3 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_length_sequence.c
   ASN.1 DER, length a SEQUENCE, Tom St Denis
 */

#ifdef LTC_DER

/**
   Get the length of a DER sequence
   @param list   The sequences of items in the SEQUENCE
   @param inlen  The number of items
   @param outlen [out] The length required in octets to store it
   @return CRYPT_OK on success
 */
int der_length_sequence(ltc_asn1_list *list, unsigned long inlen,
                        unsigned long *outlen) {
    int           err, type;
    unsigned long size, x, y, z, i;
    void          *data;

    LTC_ARGCHK(list != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* get size of output that will be required */
    y = 0;
    for (i = 0; i < inlen; i++) {
        type = list[i].type;
        size = list[i].size;
        data = list[i].data;

        if (type == LTC_ASN1_EOL) {
            break;
        }

        switch (type) {
            case LTC_ASN1_BOOLEAN:
                if ((err = der_length_boolean(&x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_INTEGER:
                if ((err = der_length_integer(data, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_SHORT_INTEGER:
                if ((err = der_length_short_integer(*((unsigned long *)data), &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_BIT_STRING:
                if ((err = der_length_bit_string(size, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_OCTET_STRING:
                if ((err = der_length_octet_string(size, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_NULL:
                y += 2;
                break;

            case LTC_ASN1_OBJECT_IDENTIFIER:
                if ((err = der_length_object_identifier(data, size, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_IA5_STRING:
                if ((err = der_length_ia5_string(data, size, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_PRINTABLE_STRING:
                if ((err = der_length_printable_string(data, size, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_UTCTIME:
                if ((err = der_length_utctime(data, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_UTF8_STRING:
                if ((err = der_length_utf8_string(data, size, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;

            case LTC_ASN1_SET:
            case LTC_ASN1_SETOF:
            case LTC_ASN1_SEQUENCE:
                if ((err = der_length_sequence(data, size, &x)) != CRYPT_OK) {
                    goto LBL_ERR;
                }
                y += x;
                break;


            default:
                err = CRYPT_INVALID_ARG;
                goto LBL_ERR;
        }
    }

    /* calc header size */
    z = y;
    if (y < 128) {
        y += 2;
    } else if (y < 256) {
        /* 0x30 0x81 LL */
        y += 3;
    } else if (y < 65536UL) {
        /* 0x30 0x82 LL LL */
        y += 4;
    } else if (y < 16777216UL) {
        /* 0x30 0x83 LL LL LL */
        y += 5;
    } else {
        err = CRYPT_INVALID_ARG;
        goto LBL_ERR;
    }

    /* store size */
    *outlen = y;
    err     = CRYPT_OK;

LBL_ERR:
    return err;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/sequence/der_length_sequence.c,v $ */
/* $Revision: 1.14 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_length_short_integer.c
   ASN.1 DER, get length of encoding, Tom St Denis
 */


#ifdef LTC_DER

/**
   Gets length of DER encoding of num
   @param num    The integer to get the size of
   @param outlen [out] The length of the DER encoding for the given integer
   @return CRYPT_OK if successful
 */
int der_length_short_integer(unsigned long num, unsigned long *outlen) {
    unsigned long z, y, len;

    LTC_ARGCHK(outlen != NULL);

    /* force to 32 bits */
    num &= 0xFFFFFFFFUL;

    /* get the number of bytes */
    z = 0;
    y = num;
    while (y) {
        ++z;
        y >>= 8;
    }

    /* handle zero */
    if (z == 0) {
        z = 1;
    }

    /* we need a 0x02 to indicate it's INTEGER */
    len = 1;

    /* length byte */
    ++len;

    /* bytes in value */
    len += z;

    /* see if msb is set */
    len += (num & (1UL << ((z << 3) - 1))) ? 1 : 0;

    /* return length */
    *outlen = len;

    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/short_integer/der_length_short_integer.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_length_utctime.c
   ASN.1 DER, get length of UTCTIME, Tom St Denis
 */

#ifdef LTC_DER

/**
   Gets length of DER encoding of UTCTIME
   @param utctime      The UTC time structure to get the size of
   @param outlen [out] The length of the DER encoding
   @return CRYPT_OK if successful
 */
int der_length_utctime(ltc_utctime *utctime, unsigned long *outlen) {
    LTC_ARGCHK(outlen != NULL);
    LTC_ARGCHK(utctime != NULL);

    if ((utctime->off_hh == 0) && (utctime->off_mm == 0)) {
        /* we encode as YYMMDDhhmmssZ */
        *outlen = 2 + 13;
    } else {
        /* we encode as YYMMDDhhmmss{+|-}hh'mm' */
        *outlen = 2 + 17;
    }

    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/utctime/der_length_utctime.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_length_utf8_string.c
   ASN.1 DER, get length of UTF8 STRING, Tom St Denis
 */

#ifdef LTC_DER

/** Return the size in bytes of a UTF-8 character
   @param c   The UTF-8 character to measure
   @return    The size in bytes
 */
unsigned long der_utf8_charsize(const wchar_t c) {
    if (c <= 0x7F) {
        return 1;
    } else if (c <= 0x7FF) {
        return 2;
    } else if (c <= 0xFFFF) {
        return 3;
    } else {
        return 4;
    }
}

/**
   Gets length of DER encoding of UTF8 STRING
   @param in       The characters to measure the length of
   @param noctets  The number of octets in the string to encode
   @param outlen   [out] The length of the DER encoding for the given string
   @return CRYPT_OK if successful
 */
int der_length_utf8_string(const wchar_t *in, unsigned long noctets, unsigned long *outlen) {
    unsigned long x, len;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(outlen != NULL);

    len = 0;
    for (x = 0; x < noctets; x++) {
        if ((in[x] < 0) || (in[x] > 0x10FFFF)) {
            return CRYPT_INVALID_ARG;
        }
        len += der_utf8_charsize(in[x]);
    }

    if (len < 128) {
        /* 0C LL DD DD DD ... */
        *outlen = 2 + len;
    } else if (len < 256) {
        /* 0C 81 LL DD DD DD ... */
        *outlen = 3 + len;
    } else if (len < 65536UL) {
        /* 0C 82 LL LL DD DD DD ... */
        *outlen = 4 + len;
    } else if (len < 16777216UL) {
        /* 0C 83 LL LL LL DD DD DD ... */
        *outlen = 5 + len;
    } else {
        return CRYPT_INVALID_ARG;
    }

    return CRYPT_OK;
}
#endif


/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/utf8/der_length_utf8_string.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file der_sequence_free.c
   ASN.1 DER, free's a structure allocated by der_decode_sequence_flexi(), Tom St Denis
 */

#ifdef LTC_DER

/**
   Free memory allocated by der_decode_sequence_flexi()
   @param in     The list to free
 */
void der_sequence_free(ltc_asn1_list *in) {
    ltc_asn1_list *l;

    /* walk to the start of the chain */
    while (in->prev != NULL || in->parent != NULL) {
        if (in->parent != NULL) {
            in = in->parent;
        } else {
            in = in->prev;
        }
    }

    /* now walk the list and free stuff */
    while (in != NULL) {
        /* is there a child? */
        if (in->child) {
            /* disconnect */
            in->child->parent = NULL;
            der_sequence_free(in->child);
        }

        switch (in->type) {
            case LTC_ASN1_SET:
            case LTC_ASN1_SETOF:
            case LTC_ASN1_SEQUENCE:
                break;

            case LTC_ASN1_INTEGER:
                if (in->data != NULL) {
                    mp_clear(in->data);
                }
                break;

            default:
                if (in->data != NULL) {
                    XFREE(in->data);
                }
        }

        /* move to next and free current */
        l = in->next;
        XFREE(in);
        in = l;
    }
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/sequence/der_sequence_free.c,v $ */
/* $Revision: 1.4 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ecc.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

/* This holds the key settings.  ***MUST*** be organized by size from smallest to largest. */
const ltc_ecc_set_type ltc_ecc_sets[] = {
 #ifdef ECC112
    {
        14,
        "SECP112R1",
        "DB7C2ABF62E35E668076BEAD208B",
        "659EF8BA043916EEDE8911702B22",
        "DB7C2ABF62E35E7628DFAC6561C5",
        "09487239995A5EE76B55F9C2F098",
        "A89CE5AF8724C0A23E0E0FF77500"
    },
 #endif
 #ifdef ECC128
    {
        16,
        "SECP128R1",
        "FFFFFFFDFFFFFFFFFFFFFFFFFFFFFFFF",
        "E87579C11079F43DD824993C2CEE5ED3",
        "FFFFFFFE0000000075A30D1B9038A115",
        "161FF7528B899B2D0C28607CA52C5B86",
        "CF5AC8395BAFEB13C02DA292DDED7A83",
    },
 #endif
 #ifdef ECC160
    {
        20,
        "SECP160R1",
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF7FFFFFFF",
        "1C97BEFC54BD7A8B65ACF89F81D4D4ADC565FA45",
        "0100000000000000000001F4C8F927AED3CA752257",
        "4A96B5688EF573284664698968C38BB913CBFC82",
        "23A628553168947D59DCC912042351377AC5FB32",
    },
 #endif
 #ifdef ECC192
    {
        24,
        "ECC-192",
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFFFFFFFFFF",
        "64210519E59C80E70FA7E9AB72243049FEB8DEECC146B9B1",
        "FFFFFFFFFFFFFFFFFFFFFFFF99DEF836146BC9B1B4D22831",
        "188DA80EB03090F67CBF20EB43A18800F4FF0AFD82FF1012",
        "7192B95FFC8DA78631011ED6B24CDD573F977A11E794811",
    },
 #endif
 #ifdef ECC224
    {
        28,
        "ECC-224",
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000000000000001",
        "B4050A850C04B3ABF54132565044B0B7D7BFD8BA270B39432355FFB4",
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFF16A2E0B8F03E13DD29455C5C2A3D",
        "B70E0CBD6BB4BF7F321390B94A03C1D356C21122343280D6115C1D21",
        "BD376388B5F723FB4C22DFE6CD4375A05A07476444D5819985007E34",
    },
 #endif
 #ifdef ECC256
    {
        32,
        "ECC-256",
        "FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF",
        "5AC635D8AA3A93E7B3EBBD55769886BC651D06B0CC53B0F63BCE3C3E27D2604B",
        "FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551",
        "6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296",
        "4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5",
    },
 #endif
 #ifdef ECC384
    {
        48,
        "ECC-384",
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF0000000000000000FFFFFFFF",
        "B3312FA7E23EE7E4988E056BE3F82D19181D9C6EFE8141120314088F5013875AC656398D8A2ED19D2A85C8EDD3EC2AEF",
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC7634D81F4372DDF581A0DB248B0A77AECEC196ACCC52973",
        "AA87CA22BE8B05378EB1C71EF320AD746E1D3B628BA79B9859F741E082542A385502F25DBF55296C3A545E3872760AB7",
        "3617DE4A96262C6F5D9E98BF9292DC29F8F41DBD289A147CE9DA3113B5F0B8C00A60B1CE1D7E819D7A431D7C90EA0E5F",
    },
 #endif
 #ifdef ECC521
    {
        66,
        "ECC-521",
        "1FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
        "51953EB9618E1C9A1F929A21A0B68540EEA2DA725B99B315F3B8B489918EF109E156193951EC7E937B1652C0BD3BB1BF073573DF883D2C34F1EF451FD46B503F00",
        "1FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFA51868783BF2F966B7FCC0148F709A5D03BB5C9B8899C47AEBB6FB71E91386409",
        "C6858E06B70404E9CD9E3ECB662395B4429C648139053FB521F828AF606B4D3DBAA14B5E77EFE75928FE1DC127A2FFA8DE3348B3C1856A429BF97E7E31C2E5BD66",
        "11839296A789A3BC0045C8A5FB42C7D1BD998F54449579B446817AFBD17273E662C97EE72995EF42640C550B9013FAD0761353C7086A272C24088BE94769FD16650",
    },
 #endif
    {
        0,
        NULL, NULL, NULL, NULL, NULL, NULL
    }
};
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc.c,v $ */
/* $Revision: 1.40 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ecc_ansi_x963_export.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

/** ECC X9.63 (Sec. 4.3.6) uncompressed export
   @param key     Key to export
   @param out     [out] destination of export
   @param outlen  [in/out]  Length of destination and final output size
   Return CRYPT_OK on success
 */
int ecc_ansi_x963_export(ecc_key *key, unsigned char *out, unsigned long *outlen) {
    unsigned char buf[ECC_BUF_SIZE];
    unsigned long numlen;

    LTC_ARGCHK(key != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    if (ltc_ecc_is_valid_idx(key->idx) == 0) {
        return CRYPT_INVALID_ARG;
    }
    numlen = key->dp->size;

    if (*outlen < (1 + 2 * numlen)) {
        *outlen = 1 + 2 * numlen;
        return CRYPT_BUFFER_OVERFLOW;
    }

    /* store byte 0x04 */
    out[0] = 0x04;

    /* pad and store x */
    zeromem(buf, sizeof(buf));
    mp_to_unsigned_bin(key->pubkey.x, buf + (numlen - mp_unsigned_bin_size(key->pubkey.x)));
    XMEMCPY(out + 1, buf, numlen);

    /* pad and store y */
    zeromem(buf, sizeof(buf));
    mp_to_unsigned_bin(key->pubkey.y, buf + (numlen - mp_unsigned_bin_size(key->pubkey.y)));
    XMEMCPY(out + 1 + numlen, buf, numlen);

    *outlen = 1 + 2 * numlen;
    return CRYPT_OK;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_ansi_x963_export.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ecc_ansi_x963_import.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

/** Import an ANSI X9.63 format public key
   @param in      The input data to read
   @param inlen   The length of the input data
   @param key     [out] destination to store imported key \
 */
int ecc_ansi_x963_import(const unsigned char *in, unsigned long inlen, ecc_key *key) {
    return ecc_ansi_x963_import_ex(in, inlen, key, NULL);
}

int ecc_ansi_x963_import_ex(const unsigned char *in, unsigned long inlen, ecc_key *key, ltc_ecc_set_type *dp) {
    int x, err;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(key != NULL);

    /* must be odd */
    if ((inlen & 1) == 0) {
        return CRYPT_INVALID_ARG;
    }

    /* init key */
    if (mp_init_multi(&key->pubkey.x, &key->pubkey.y, &key->pubkey.z, &key->k, NULL) != CRYPT_OK) {
        return CRYPT_MEM;
    }

    /* check for 4, 6 or 7 */
    if ((in[0] != 4) && (in[0] != 6) && (in[0] != 7)) {
        err = CRYPT_INVALID_PACKET;
        goto error;
    }

    /* read data */
    if ((err = mp_read_unsigned_bin(key->pubkey.x, (unsigned char *)in + 1, (inlen - 1) >> 1)) != CRYPT_OK) {
        goto error;
    }

    if ((err = mp_read_unsigned_bin(key->pubkey.y, (unsigned char *)in + 1 + ((inlen - 1) >> 1), (inlen - 1) >> 1)) != CRYPT_OK) {
        goto error;
    }
    if ((err = mp_set(key->pubkey.z, 1)) != CRYPT_OK) {
        goto error;
    }

    if (dp == NULL) {
        /* determine the idx */
        for (x = 0; ltc_ecc_sets[x].size != 0; x++) {
            if ((unsigned)ltc_ecc_sets[x].size >= ((inlen - 1) >> 1)) {
                break;
            }
        }
        if (ltc_ecc_sets[x].size == 0) {
            err = CRYPT_INVALID_PACKET;
            goto error;
        }
        /* set the idx */
        key->idx = x;
        key->dp  = &ltc_ecc_sets[x];
    } else {
        if (((inlen - 1) >> 1) != (unsigned long)dp->size) {
            err = CRYPT_INVALID_PACKET;
            goto error;
        }
        key->idx = -1;
        key->dp  = dp;
    }
    key->type = PK_PUBLIC;

    /* we're done */
    return CRYPT_OK;
error:
    mp_clear_multi(key->pubkey.x, key->pubkey.y, key->pubkey.z, key->k, NULL);
    return err;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_ansi_x963_import.c,v $ */
/* $Revision: 1.11 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ecc_decrypt_key.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

/**
   Decrypt an ECC encrypted key
   @param in       The ciphertext
   @param inlen    The length of the ciphertext (octets)
   @param out      [out] The plaintext
   @param outlen   [in/out] The max size and resulting size of the plaintext
   @param key      The corresponding private ECC key
   @return CRYPT_OK if successful
 */
int ecc_decrypt_key(const unsigned char *in, unsigned long inlen,
                    unsigned char *out, unsigned long *outlen,
                    ecc_key *key) {
    unsigned char *ecc_shared, *skey, *pub_expt;
    unsigned long x, y, hashOID[32];
    int           hash, err;
    ecc_key       pubkey;
    ltc_asn1_list decode[3];

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);
    LTC_ARGCHK(key != NULL);

    /* right key type? */
    if (key->type != PK_PRIVATE) {
        return CRYPT_PK_NOT_PRIVATE;
    }

    /* decode to find out hash */
    LTC_SET_ASN1(decode, 0, LTC_ASN1_OBJECT_IDENTIFIER, hashOID, sizeof(hashOID) / sizeof(hashOID[0]));

    if ((err = der_decode_sequence(in, inlen, decode, 1)) != CRYPT_OK) {
        return err;
    }

    hash = find_hash_oid(hashOID, decode[0].size);
    if (hash_is_valid(hash) != CRYPT_OK) {
        return CRYPT_INVALID_PACKET;
    }

    /* we now have the hash! */

    /* allocate memory */
    pub_expt   = XMALLOC(ECC_BUF_SIZE);
    ecc_shared = XMALLOC(ECC_BUF_SIZE);
    skey       = XMALLOC(MAXBLOCKSIZE);
    if ((pub_expt == NULL) || (ecc_shared == NULL) || (skey == NULL)) {
        if (pub_expt != NULL) {
            XFREE(pub_expt);
        }
        if (ecc_shared != NULL) {
            XFREE(ecc_shared);
        }
        if (skey != NULL) {
            XFREE(skey);
        }
        return CRYPT_MEM;
    }
    LTC_SET_ASN1(decode, 1, LTC_ASN1_OCTET_STRING, pub_expt, ECC_BUF_SIZE);
    LTC_SET_ASN1(decode, 2, LTC_ASN1_OCTET_STRING, skey, MAXBLOCKSIZE);

    /* read the structure in now */
    if ((err = der_decode_sequence(in, inlen, decode, 3)) != CRYPT_OK) {
        goto LBL_ERR;
    }

    /* import ECC key from packet */
    if ((err = ecc_import(decode[1].data, decode[1].size, &pubkey)) != CRYPT_OK) {
        goto LBL_ERR;
    }

    /* make shared key */
    x = ECC_BUF_SIZE;
    if ((err = ecc_shared_secret(key, &pubkey, ecc_shared, &x)) != CRYPT_OK) {
        ecc_free(&pubkey);
        goto LBL_ERR;
    }
    ecc_free(&pubkey);

    y = MIN(ECC_BUF_SIZE, MAXBLOCKSIZE);
    if ((err = hash_memory(hash, ecc_shared, x, ecc_shared, &y)) != CRYPT_OK) {
        goto LBL_ERR;
    }

    /* ensure the hash of the shared secret is at least as big as the encrypt itself */
    if (decode[2].size > y) {
        err = CRYPT_INVALID_PACKET;
        goto LBL_ERR;
    }

    /* avoid buffer overflow */
    if (*outlen < decode[2].size) {
        *outlen = decode[2].size;
        err     = CRYPT_BUFFER_OVERFLOW;
        goto LBL_ERR;
    }

    /* Decrypt the key */
    for (x = 0; x < decode[2].size; x++) {
        out[x] = skey[x] ^ ecc_shared[x];
    }
    *outlen = x;

    err = CRYPT_OK;
LBL_ERR:
 #ifdef LTC_CLEAN_STACK
    zeromem(pub_expt, ECC_BUF_SIZE);
    zeromem(ecc_shared, ECC_BUF_SIZE);
    zeromem(skey, MAXBLOCKSIZE);
 #endif

    XFREE(pub_expt);
    XFREE(ecc_shared);
    XFREE(skey);

    return err;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_decrypt_key.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ecc_encrypt_key.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

/**
   Encrypt a symmetric key with ECC
   @param in         The symmetric key you want to encrypt
   @param inlen      The length of the key to encrypt (octets)
   @param out        [out] The destination for the ciphertext
   @param outlen     [in/out] The max size and resulting size of the ciphertext
   @param prng       An active PRNG state
   @param wprng      The index of the PRNG you wish to use
   @param hash       The index of the hash you want to use
   @param key        The ECC key you want to encrypt to
   @return CRYPT_OK if successful
 */
int ecc_encrypt_key(const unsigned char *in, unsigned long inlen,
                    unsigned char *out, unsigned long *outlen,
                    prng_state *prng, int wprng, int hash,
                    ecc_key *key) {
    unsigned char *pub_expt, *ecc_shared, *skey;
    ecc_key       pubkey;
    unsigned long x, y, pubkeysize;
    int           err;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);
    LTC_ARGCHK(key != NULL);

    /* check that wprng/cipher/hash are not invalid */
    if ((err = prng_is_valid(wprng)) != CRYPT_OK) {
        return err;
    }

    if ((err = hash_is_valid(hash)) != CRYPT_OK) {
        return err;
    }

    if (inlen > hash_descriptor[hash].hashsize) {
        return CRYPT_INVALID_HASH;
    }

    /* make a random key and export the public copy */
    if ((err = ecc_make_key_ex(prng, wprng, &pubkey, key->dp)) != CRYPT_OK) {
        return err;
    }

    pub_expt   = XMALLOC(ECC_BUF_SIZE);
    ecc_shared = XMALLOC(ECC_BUF_SIZE);
    skey       = XMALLOC(MAXBLOCKSIZE);
    if ((pub_expt == NULL) || (ecc_shared == NULL) || (skey == NULL)) {
        if (pub_expt != NULL) {
            XFREE(pub_expt);
        }
        if (ecc_shared != NULL) {
            XFREE(ecc_shared);
        }
        if (skey != NULL) {
            XFREE(skey);
        }
        ecc_free(&pubkey);
        return CRYPT_MEM;
    }

    pubkeysize = ECC_BUF_SIZE;
    if ((err = ecc_export(pub_expt, &pubkeysize, PK_PUBLIC, &pubkey)) != CRYPT_OK) {
        ecc_free(&pubkey);
        goto LBL_ERR;
    }

    /* make random key */
    x = ECC_BUF_SIZE;
    if ((err = ecc_shared_secret(&pubkey, key, ecc_shared, &x)) != CRYPT_OK) {
        ecc_free(&pubkey);
        goto LBL_ERR;
    }
    ecc_free(&pubkey);
    y = MAXBLOCKSIZE;
    if ((err = hash_memory(hash, ecc_shared, x, skey, &y)) != CRYPT_OK) {
        goto LBL_ERR;
    }

    /* Encrypt key */
    for (x = 0; x < inlen; x++) {
        skey[x] ^= in[x];
    }

    err = der_encode_sequence_multi(out, outlen,
                                    LTC_ASN1_OBJECT_IDENTIFIER, hash_descriptor[hash].OIDlen, hash_descriptor[hash].OID,
                                    LTC_ASN1_OCTET_STRING, pubkeysize, pub_expt,
                                    LTC_ASN1_OCTET_STRING, inlen, skey,
                                    LTC_ASN1_EOL, 0UL, NULL);

LBL_ERR:
 #ifdef LTC_CLEAN_STACK
    /* clean up */
    zeromem(pub_expt, ECC_BUF_SIZE);
    zeromem(ecc_shared, ECC_BUF_SIZE);
    zeromem(skey, MAXBLOCKSIZE);
 #endif

    XFREE(skey);
    XFREE(ecc_shared);
    XFREE(pub_expt);

    return err;
}
#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_encrypt_key.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ecc_export.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

/**
   Export an ECC key as a binary packet
   @param out     [out] Destination for the key
   @param outlen  [in/out] Max size and resulting size of the exported key
   @param type    The type of key you want to export (PK_PRIVATE or PK_PUBLIC)
   @param key     The key to export
   @return CRYPT_OK if successful
 */
int ecc_export(unsigned char *out, unsigned long *outlen, int type, ecc_key *key) {
    int           err;
    unsigned char flags[1];
    unsigned long key_size;

    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);
    LTC_ARGCHK(key != NULL);

    /* type valid? */
    if ((key->type != PK_PRIVATE) && (type == PK_PRIVATE)) {
        return CRYPT_PK_TYPE_MISMATCH;
    }

    if (ltc_ecc_is_valid_idx(key->idx) == 0) {
        return CRYPT_INVALID_ARG;
    }

    /* we store the NIST byte size */
    key_size = key->dp->size;

    if (type == PK_PRIVATE) {
        flags[0] = 1;
        err      = der_encode_sequence_multi(out, outlen,
                                             LTC_ASN1_BIT_STRING, 1UL, flags,
                                             LTC_ASN1_SHORT_INTEGER, 1UL, &key_size,
                                             LTC_ASN1_INTEGER, 1UL, key->pubkey.x,
                                             LTC_ASN1_INTEGER, 1UL, key->pubkey.y,
                                             LTC_ASN1_INTEGER, 1UL, key->k,
                                             LTC_ASN1_EOL, 0UL, NULL);
    } else {
        flags[0] = 0;
        err      = der_encode_sequence_multi(out, outlen,
                                             LTC_ASN1_BIT_STRING, 1UL, flags,
                                             LTC_ASN1_SHORT_INTEGER, 1UL, &key_size,
                                             LTC_ASN1_INTEGER, 1UL, key->pubkey.x,
                                             LTC_ASN1_INTEGER, 1UL, key->pubkey.y,
                                             LTC_ASN1_EOL, 0UL, NULL);
    }

    return err;
}
#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_export.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ecc_free.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

/**
   Free an ECC key from memory
   @param key   The key you wish to free
 */
void ecc_free(ecc_key *key) {
    LTC_ARGCHKVD(key != NULL);
    mp_clear_multi(key->pubkey.x, key->pubkey.y, key->pubkey.z, key->k, NULL);
}
#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_free.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ecc_get_size.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

/**
   Get the size of an ECC key
   @param key    The key to get the size of
   @return The size (octets) of the key or INT_MAX on error
 */
int ecc_get_size(ecc_key *key) {
    LTC_ARGCHK(key != NULL);
    if (ltc_ecc_is_valid_idx(key->idx))
        return key->dp->size;
    else
        return INT_MAX; /* large value known to cause it to fail when passed to ecc_make_key() */
}
#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_get_size.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ecc_import.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

static int is_point(ecc_key *key) {
    void *prime, *b, *t1, *t2;
    int  err;

    if ((err = mp_init_multi(&prime, &b, &t1, &t2, NULL)) != CRYPT_OK) {
        return err;
    }

    /* load prime and b */
    if ((err = mp_read_radix(prime, key->dp->prime, 16)) != CRYPT_OK) {
        goto error;
    }
    if ((err = mp_read_radix(b, key->dp->B, 16)) != CRYPT_OK) {
        goto error;
    }

    /* compute y^2 */
    if ((err = mp_sqr(key->pubkey.y, t1)) != CRYPT_OK) {
        goto error;
    }

    /* compute x^3 */
    if ((err = mp_sqr(key->pubkey.x, t2)) != CRYPT_OK) {
        goto error;
    }
    if ((err = mp_mod(t2, prime, t2)) != CRYPT_OK) {
        goto error;
    }
    if ((err = mp_mul(key->pubkey.x, t2, t2)) != CRYPT_OK) {
        goto error;
    }

    /* compute y^2 - x^3 */
    if ((err = mp_sub(t1, t2, t1)) != CRYPT_OK) {
        goto error;
    }

    /* compute y^2 - x^3 + 3x */
    if ((err = mp_add(t1, key->pubkey.x, t1)) != CRYPT_OK) {
        goto error;
    }
    if ((err = mp_add(t1, key->pubkey.x, t1)) != CRYPT_OK) {
        goto error;
    }
    if ((err = mp_add(t1, key->pubkey.x, t1)) != CRYPT_OK) {
        goto error;
    }
    if ((err = mp_mod(t1, prime, t1)) != CRYPT_OK) {
        goto error;
    }
    while (mp_cmp_d(t1, 0) == LTC_MP_LT) {
        if ((err = mp_add(t1, prime, t1)) != CRYPT_OK) {
            goto error;
        }
    }
    while (mp_cmp(t1, prime) != LTC_MP_LT) {
        if ((err = mp_sub(t1, prime, t1)) != CRYPT_OK) {
            goto error;
        }
    }

    /* compare to b */
    if (mp_cmp(t1, b) != LTC_MP_EQ) {
        err = CRYPT_INVALID_PACKET;
    } else {
        err = CRYPT_OK;
    }

error:
    mp_clear_multi(prime, b, t1, t2, NULL);
    return err;
}

/**
   Import an ECC key from a binary packet
   @param in      The packet to import
   @param inlen   The length of the packet
   @param key     [out] The destination of the import
   @return CRYPT_OK if successful, upon error all allocated memory will be freed
 */
int ecc_import(const unsigned char *in, unsigned long inlen, ecc_key *key) {
    return ecc_import_ex(in, inlen, key, NULL);
}

/**
   Import an ECC key from a binary packet, using user supplied domain params rather than one of the NIST ones
   @param in      The packet to import
   @param inlen   The length of the packet
   @param key     [out] The destination of the import
   @param dp      pointer to user supplied params; must be the same as the params used when exporting
   @return CRYPT_OK if successful, upon error all allocated memory will be freed
 */
int ecc_import_ex(const unsigned char *in, unsigned long inlen, ecc_key *key, const ltc_ecc_set_type *dp) {
    unsigned long key_size;
    unsigned char flags[1];
    int           err;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(key != NULL);
    LTC_ARGCHK(ltc_mp.name != NULL);

    /* init key */
    if (mp_init_multi(&key->pubkey.x, &key->pubkey.y, &key->pubkey.z, &key->k, NULL) != CRYPT_OK) {
        return CRYPT_MEM;
    }

    /* find out what type of key it is */
    if ((err = der_decode_sequence_multi(in, inlen,
                                         LTC_ASN1_BIT_STRING, 1UL, &flags,
                                         LTC_ASN1_EOL, 0UL, NULL)) != CRYPT_OK) {
        goto done;
    }


    if (flags[0] == 1) {
        /* private key */
        key->type = PK_PRIVATE;
        if ((err = der_decode_sequence_multi(in, inlen,
                                             LTC_ASN1_BIT_STRING, 1UL, flags,
                                             LTC_ASN1_SHORT_INTEGER, 1UL, &key_size,
                                             LTC_ASN1_INTEGER, 1UL, key->pubkey.x,
                                             LTC_ASN1_INTEGER, 1UL, key->pubkey.y,
                                             LTC_ASN1_INTEGER, 1UL, key->k,
                                             LTC_ASN1_EOL, 0UL, NULL)) != CRYPT_OK) {
            goto done;
        }
    } else {
        /* public key */
        key->type = PK_PUBLIC;
        if ((err = der_decode_sequence_multi(in, inlen,
                                             LTC_ASN1_BIT_STRING, 1UL, flags,
                                             LTC_ASN1_SHORT_INTEGER, 1UL, &key_size,
                                             LTC_ASN1_INTEGER, 1UL, key->pubkey.x,
                                             LTC_ASN1_INTEGER, 1UL, key->pubkey.y,
                                             LTC_ASN1_EOL, 0UL, NULL)) != CRYPT_OK) {
            goto done;
        }
    }

    if (dp == NULL) {
        /* find the idx */
        for (key->idx = 0; ltc_ecc_sets[key->idx].size && (unsigned long)ltc_ecc_sets[key->idx].size != key_size; ++key->idx);
        if (ltc_ecc_sets[key->idx].size == 0) {
            err = CRYPT_INVALID_PACKET;
            goto done;
        }
        key->dp = &ltc_ecc_sets[key->idx];
    } else {
        key->idx = -1;
        key->dp  = dp;
    }
    /* set z */
    if ((err = mp_set(key->pubkey.z, 1)) != CRYPT_OK) {
        goto done;
    }

    /* is it a point on the curve?  */
    if ((err = is_point(key)) != CRYPT_OK) {
        goto done;
    }

    /* we're good */
    return CRYPT_OK;
done:
    mp_clear_multi(key->pubkey.x, key->pubkey.y, key->pubkey.z, key->k, NULL);
    return err;
}
#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_import.c,v $ */
/* $Revision: 1.13 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ecc_make_key.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

/**
   Make a new ECC key
   @param prng         An active PRNG state
   @param wprng        The index of the PRNG you wish to use
   @param keysize      The keysize for the new key (in octets from 20 to 65 bytes)
   @param key          [out] Destination of the newly created key
   @return CRYPT_OK if successful, upon error all allocated memory will be freed
 */
int ecc_make_key(prng_state *prng, int wprng, int keysize, ecc_key *key) {
    int x, err;

    /* find key size */
    for (x = 0; (keysize > ltc_ecc_sets[x].size) && (ltc_ecc_sets[x].size != 0); x++);
    keysize = ltc_ecc_sets[x].size;

    if ((keysize > ECC_MAXSIZE) || (ltc_ecc_sets[x].size == 0)) {
        return CRYPT_INVALID_KEYSIZE;
    }
    err      = ecc_make_key_ex(prng, wprng, key, &ltc_ecc_sets[x]);
    key->idx = x;
    return err;
}

int ecc_make_key_ex(prng_state *prng, int wprng, ecc_key *key, const ltc_ecc_set_type *dp) {
    int           err;
    ecc_point     *base;
    void          *prime, *order;
    unsigned char *buf;
    int           keysize;

    LTC_ARGCHK(key != NULL);
    LTC_ARGCHK(ltc_mp.name != NULL);
    LTC_ARGCHK(dp != NULL);

    /* good prng? */
    if ((err = prng_is_valid(wprng)) != CRYPT_OK) {
        return err;
    }

    key->idx = -1;
    key->dp  = dp;
    keysize  = dp->size;

    /* allocate ram */
    base = NULL;
    buf  = XMALLOC(ECC_MAXSIZE);
    if (buf == NULL) {
        return CRYPT_MEM;
    }

    /* make up random string */
    if (prng_descriptor[wprng].read(buf, (unsigned long)keysize, prng) != (unsigned long)keysize) {
        err = CRYPT_ERROR_READPRNG;
        goto ERR_BUF;
    }

    /* setup the key variables */
    if ((err = mp_init_multi(&key->pubkey.x, &key->pubkey.y, &key->pubkey.z, &key->k, &prime, &order, NULL)) != CRYPT_OK) {
        goto ERR_BUF;
    }
    base = ltc_ecc_new_point();
    if (base == NULL) {
        err = CRYPT_MEM;
        goto errkey;
    }

    /* read in the specs for this key */
    if ((err = mp_read_radix(prime, (char *)key->dp->prime, 16)) != CRYPT_OK) {
        goto errkey;
    }
    if ((err = mp_read_radix(order, (char *)key->dp->order, 16)) != CRYPT_OK) {
        goto errkey;
    }
    if ((err = mp_read_radix(base->x, (char *)key->dp->Gx, 16)) != CRYPT_OK) {
        goto errkey;
    }
    if ((err = mp_read_radix(base->y, (char *)key->dp->Gy, 16)) != CRYPT_OK) {
        goto errkey;
    }
    if ((err = mp_set(base->z, 1)) != CRYPT_OK) {
        goto errkey;
    }
    if ((err = mp_read_unsigned_bin(key->k, (unsigned char *)buf, keysize)) != CRYPT_OK) {
        goto errkey;
    }

    /* the key should be smaller than the order of base point */
    if (mp_cmp(key->k, order) != LTC_MP_LT) {
        if ((err = mp_mod(key->k, order, key->k)) != CRYPT_OK) {
            goto errkey;
        }
    }
    /* make the public key */
    if ((err = ltc_mp.ecc_ptmul(key->k, base, &key->pubkey, prime, 1)) != CRYPT_OK) {
        goto errkey;
    }
    key->type = PK_PRIVATE;

    /* free up ram */
    err = CRYPT_OK;
    goto cleanup;
errkey:
    mp_clear_multi(key->pubkey.x, key->pubkey.y, key->pubkey.z, key->k, NULL);
cleanup:
    ltc_ecc_del_point(base);
    mp_clear_multi(prime, order, NULL);
ERR_BUF:
 #ifdef LTC_CLEAN_STACK
    zeromem(buf, ECC_MAXSIZE);
 #endif
    XFREE(buf);
    return err;
}
#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_make_key.c,v $ */
/* $Revision: 1.13 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ecc_shared_secret.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

/**
   Create an ECC shared secret between two keys
   @param private_key      The private ECC key
   @param public_key       The public key
   @param out              [out] Destination of the shared secret (Conforms to EC-DH from ANSI X9.63)
   @param outlen           [in/out] The max size and resulting size of the shared secret
   @return CRYPT_OK if successful
 */
int ecc_shared_secret(ecc_key *private_key, ecc_key *public_key,
                      unsigned char *out, unsigned long *outlen) {
    unsigned long x;
    ecc_point     *result;
    void          *prime;
    int           err;

    LTC_ARGCHK(private_key != NULL);
    LTC_ARGCHK(public_key != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* type valid? */
    if (private_key->type != PK_PRIVATE) {
        return CRYPT_PK_NOT_PRIVATE;
    }

    if ((ltc_ecc_is_valid_idx(private_key->idx) == 0) || (ltc_ecc_is_valid_idx(public_key->idx) == 0)) {
        return CRYPT_INVALID_ARG;
    }

    if (XSTRCMP(private_key->dp->name, public_key->dp->name) != 0) {
        return CRYPT_PK_TYPE_MISMATCH;
    }

    /* make new point */
    result = ltc_ecc_new_point();
    if (result == NULL) {
        return CRYPT_MEM;
    }

    if ((err = mp_init(&prime)) != CRYPT_OK) {
        ltc_ecc_del_point(result);
        return err;
    }

    if ((err = mp_read_radix(prime, (char *)private_key->dp->prime, 16)) != CRYPT_OK) {
        goto done;
    }
    if ((err = ltc_mp.ecc_ptmul(private_key->k, &public_key->pubkey, result, prime, 1)) != CRYPT_OK) {
        goto done;
    }

    x = (unsigned long)mp_unsigned_bin_size(prime);
    if (*outlen < x) {
        *outlen = x;
        err     = CRYPT_BUFFER_OVERFLOW;
        goto done;
    }
    zeromem(out, x);
    if ((err = mp_to_unsigned_bin(result->x, out + (x - mp_unsigned_bin_size(result->x)))) != CRYPT_OK) {
        goto done;
    }

    err     = CRYPT_OK;
    *outlen = x;
done:
    mp_clear(prime);
    ltc_ecc_del_point(result);
    return err;
}
#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_shared_secret.c,v $ */
/* $Revision: 1.10 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ecc_sign_hash.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

/**
   Sign a message digest
   @param in        The message digest to sign
   @param inlen     The length of the digest
   @param out       [out] The destination for the signature
   @param outlen    [in/out] The max size and resulting size of the signature
   @param prng      An active PRNG state
   @param wprng     The index of the PRNG you wish to use
   @param key       A private ECC key
   @return CRYPT_OK if successful
 */
int ecc_sign_hash(const unsigned char *in, unsigned long inlen,
                  unsigned char *out, unsigned long *outlen,
                  prng_state *prng, int wprng, ecc_key *key) {
    ecc_key pubkey;
    void    *r, *s, *e, *p;
    int     err;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);
    LTC_ARGCHK(key != NULL);

    /* is this a private key? */
    if (key->type != PK_PRIVATE) {
        return CRYPT_PK_NOT_PRIVATE;
    }

    /* is the IDX valid ?  */
    if (ltc_ecc_is_valid_idx(key->idx) != 1) {
        return CRYPT_PK_INVALID_TYPE;
    }

    if ((err = prng_is_valid(wprng)) != CRYPT_OK) {
        return err;
    }

    /* get the hash and load it as a bignum into 'e' */
    /* init the bignums */
    if ((err = mp_init_multi(&r, &s, &p, &e, NULL)) != CRYPT_OK) {
        return err;
    }
    if ((err = mp_read_radix(p, (char *)key->dp->order, 16)) != CRYPT_OK) {
        goto errnokey;
    }
    if ((err = mp_read_unsigned_bin(e, (unsigned char *)in, (int)inlen)) != CRYPT_OK) {
        goto errnokey;
    }

    /* make up a key and export the public copy */
    for ( ; ; ) {
        if ((err = ecc_make_key_ex(prng, wprng, &pubkey, key->dp)) != CRYPT_OK) {
            goto errnokey;
        }

        /* find r = x1 mod n */
        if ((err = mp_mod(pubkey.pubkey.x, p, r)) != CRYPT_OK) {
            goto error;
        }

        if (mp_iszero(r) == LTC_MP_YES) {
            ecc_free(&pubkey);
        } else {
            /* find s = (e + xr)/k */
            if ((err = mp_invmod(pubkey.k, p, pubkey.k)) != CRYPT_OK) {
                goto error;
            }                                                                                /* k = 1/k */
            if ((err = mp_mulmod(key->k, r, p, s)) != CRYPT_OK) {
                goto error;
            }                                                                                /* s = xr */
            if ((err = mp_add(e, s, s)) != CRYPT_OK) {
                goto error;
            }                                                                                /* s = e +  xr */
            if ((err = mp_mod(s, p, s)) != CRYPT_OK) {
                goto error;
            }                                                                                /* s = e +  xr */
            if ((err = mp_mulmod(s, pubkey.k, p, s)) != CRYPT_OK) {
                goto error;
            }                                                                                /* s = (e + xr)/k */
            ecc_free(&pubkey);
            if (mp_iszero(s) == LTC_MP_NO) {
                break;
            }
        }
    }

    /* store as SEQUENCE { r, s -- integer } */
    err = der_encode_sequence_multi(out, outlen,
                                    LTC_ASN1_INTEGER, 1UL, r,
                                    LTC_ASN1_INTEGER, 1UL, s,
                                    LTC_ASN1_EOL, 0UL, NULL);
    goto errnokey;
error:
    ecc_free(&pubkey);
errnokey:
    mp_clear_multi(r, s, p, e, NULL);
    return err;
}
#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_sign_hash.c,v $ */
/* $Revision: 1.11 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ecc_sizes.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

void ecc_sizes(int *low, int *high) {
    int i;

    LTC_ARGCHKVD(low != NULL);
    LTC_ARGCHKVD(high != NULL);

    *low  = INT_MAX;
    *high = 0;
    for (i = 0; ltc_ecc_sets[i].size != 0; i++) {
        if (ltc_ecc_sets[i].size < *low) {
            *low = ltc_ecc_sets[i].size;
        }
        if (ltc_ecc_sets[i].size > *high) {
            *high = ltc_ecc_sets[i].size;
        }
    }
}
#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_sizes.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ecc_test.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

/**
   Perform on the ECC system
   @return CRYPT_OK if successful
 */
int ecc_test(void) {
    void      *modulus, *order;
    ecc_point *G, *GG;
    int       i, err, primality;

    if ((err = mp_init_multi(&modulus, &order, NULL)) != CRYPT_OK) {
        return err;
    }

    G  = ltc_ecc_new_point();
    GG = ltc_ecc_new_point();
    if ((G == NULL) || (GG == NULL)) {
        mp_clear_multi(modulus, order, NULL);
        ltc_ecc_del_point(G);
        ltc_ecc_del_point(GG);
        return CRYPT_MEM;
    }

    for (i = 0; ltc_ecc_sets[i].size; i++) {
 #if 0
        printf("Testing %d\n", ltc_ecc_sets[i].size);
 #endif
        if ((err = mp_read_radix(modulus, (char *)ltc_ecc_sets[i].prime, 16)) != CRYPT_OK) {
            goto done;
        }
        if ((err = mp_read_radix(order, (char *)ltc_ecc_sets[i].order, 16)) != CRYPT_OK) {
            goto done;
        }

        /* is prime actually prime? */
        if ((err = mp_prime_is_prime(modulus, 8, &primality)) != CRYPT_OK) {
            goto done;
        }
        if (primality == 0) {
            err = CRYPT_FAIL_TESTVECTOR;
            goto done;
        }

        /* is order prime ? */
        if ((err = mp_prime_is_prime(order, 8, &primality)) != CRYPT_OK) {
            goto done;
        }
        if (primality == 0) {
            err = CRYPT_FAIL_TESTVECTOR;
            goto done;
        }

        if ((err = mp_read_radix(G->x, (char *)ltc_ecc_sets[i].Gx, 16)) != CRYPT_OK) {
            goto done;
        }
        if ((err = mp_read_radix(G->y, (char *)ltc_ecc_sets[i].Gy, 16)) != CRYPT_OK) {
            goto done;
        }
        mp_set(G->z, 1);

        /* then we should have G == (order + 1)G */
        if ((err = mp_add_d(order, 1, order)) != CRYPT_OK) {
            goto done;
        }
        if ((err = ltc_mp.ecc_ptmul(order, G, GG, modulus, 1)) != CRYPT_OK) {
            goto done;
        }
        if ((mp_cmp(G->x, GG->x) != LTC_MP_EQ) || (mp_cmp(G->y, GG->y) != LTC_MP_EQ)) {
            err = CRYPT_FAIL_TESTVECTOR;
            goto done;
        }
    }
    err = CRYPT_OK;
done:
    ltc_ecc_del_point(GG);
    ltc_ecc_del_point(G);
    mp_clear_multi(order, modulus, NULL);
    return err;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_test.c,v $ */
/* $Revision: 1.12 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ecc_verify_hash.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

/* verify
 *
 * w  = s^-1 mod n
 * u1 = xw
 * u2 = rw
 * X = u1*G + u2*Q
 * v = X_x1 mod n
 * accept if v == r
 */

/**
   Verify an ECC signature
   @param sig         The signature to verify
   @param siglen      The length of the signature (octets)
   @param hash        The hash (message digest) that was signed
   @param hashlen     The length of the hash (octets)
   @param stat        Result of signature, 1==valid, 0==invalid
   @param key         The corresponding public ECC key
   @return CRYPT_OK if successful (even if the signature is not valid)
 */
int ecc_verify_hash(const unsigned char *sig, unsigned long siglen,
                    const unsigned char *hash, unsigned long hashlen,
                    int *stat, ecc_key *key) {
    ecc_point *mG, *mQ;
    void      *r, *s, *v, *w, *u1, *u2, *e, *p, *m;
    void      *mp;
    int       err;

    LTC_ARGCHK(sig != NULL);
    LTC_ARGCHK(hash != NULL);
    LTC_ARGCHK(stat != NULL);
    LTC_ARGCHK(key != NULL);

    /* default to invalid signature */
    *stat = 0;
    mp    = NULL;

    /* is the IDX valid ?  */
    if (ltc_ecc_is_valid_idx(key->idx) != 1) {
        return CRYPT_PK_INVALID_TYPE;
    }

    /* allocate ints */
    if ((err = mp_init_multi(&r, &s, &v, &w, &u1, &u2, &p, &e, &m, NULL)) != CRYPT_OK) {
        return CRYPT_MEM;
    }

    /* allocate points */
    mG = ltc_ecc_new_point();
    mQ = ltc_ecc_new_point();
    if ((mQ == NULL) || (mG == NULL)) {
        err = CRYPT_MEM;
        goto error;
    }

    /* parse header */
    if ((err = der_decode_sequence_multi(sig, siglen,
                                         LTC_ASN1_INTEGER, 1UL, r,
                                         LTC_ASN1_INTEGER, 1UL, s,
                                         LTC_ASN1_EOL, 0UL, NULL)) != CRYPT_OK) {
        goto error;
    }

    /* get the order */
    if ((err = mp_read_radix(p, (char *)key->dp->order, 16)) != CRYPT_OK) {
        goto error;
    }

    /* get the modulus */
    if ((err = mp_read_radix(m, (char *)key->dp->prime, 16)) != CRYPT_OK) {
        goto error;
    }

    /* check for zero */
    if (mp_iszero(r) || mp_iszero(s) || (mp_cmp(r, p) != LTC_MP_LT) || (mp_cmp(s, p) != LTC_MP_LT)) {
        err = CRYPT_INVALID_PACKET;
        goto error;
    }

    /* read hash */
    if ((err = mp_read_unsigned_bin(e, (unsigned char *)hash, (int)hashlen)) != CRYPT_OK) {
        goto error;
    }

    /*  w  = s^-1 mod n */
    if ((err = mp_invmod(s, p, w)) != CRYPT_OK) {
        goto error;
    }

    /* u1 = ew */
    if ((err = mp_mulmod(e, w, p, u1)) != CRYPT_OK) {
        goto error;
    }

    /* u2 = rw */
    if ((err = mp_mulmod(r, w, p, u2)) != CRYPT_OK) {
        goto error;
    }

    /* find mG and mQ */
    if ((err = mp_read_radix(mG->x, (char *)key->dp->Gx, 16)) != CRYPT_OK) {
        goto error;
    }
    if ((err = mp_read_radix(mG->y, (char *)key->dp->Gy, 16)) != CRYPT_OK) {
        goto error;
    }
    if ((err = mp_set(mG->z, 1)) != CRYPT_OK) {
        goto error;
    }

    if ((err = mp_copy(key->pubkey.x, mQ->x)) != CRYPT_OK) {
        goto error;
    }
    if ((err = mp_copy(key->pubkey.y, mQ->y)) != CRYPT_OK) {
        goto error;
    }
    if ((err = mp_copy(key->pubkey.z, mQ->z)) != CRYPT_OK) {
        goto error;
    }

    /* compute u1*mG + u2*mQ = mG */
    if (ltc_mp.ecc_mul2add == NULL) {
        if ((err = ltc_mp.ecc_ptmul(u1, mG, mG, m, 0)) != CRYPT_OK) {
            goto error;
        }
        if ((err = ltc_mp.ecc_ptmul(u2, mQ, mQ, m, 0)) != CRYPT_OK) {
            goto error;
        }

        /* find the montgomery mp */
        if ((err = mp_montgomery_setup(m, &mp)) != CRYPT_OK) {
            goto error;
        }

        /* add them */
        if ((err = ltc_mp.ecc_ptadd(mQ, mG, mG, m, mp)) != CRYPT_OK) {
            goto error;
        }

        /* reduce */
        if ((err = ltc_mp.ecc_map(mG, m, mp)) != CRYPT_OK) {
            goto error;
        }
    } else {
        /* use Shamir's trick to compute u1*mG + u2*mQ using half of the doubles */
        if ((err = ltc_mp.ecc_mul2add(mG, u1, mQ, u2, mG, m)) != CRYPT_OK) {
            goto error;
        }
    }

    /* v = X_x1 mod n */
    if ((err = mp_mod(mG->x, p, v)) != CRYPT_OK) {
        goto error;
    }

    /* does v == r */
    if (mp_cmp(v, r) == LTC_MP_EQ) {
        *stat = 1;
    }

    /* clear up and return */
    err = CRYPT_OK;
error:
    ltc_ecc_del_point(mG);
    ltc_ecc_del_point(mQ);
    mp_clear_multi(r, s, v, w, u1, u2, p, e, m, NULL);
    if (mp != NULL) {
        mp_montgomery_free(mp);
    }
    return err;
}
#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_verify_hash.c,v $ */
/* $Revision: 1.14 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */



/**
   @file error_to_string.c
   Convert error codes to ASCII strings, Tom St Denis
 */

static const char * const err_2_str[] =
{
    "CRYPT_OK",
    "CRYPT_ERROR",
    "Non-fatal 'no-operation' requested.",

    "Invalid keysize for block cipher.",
    "Invalid number of rounds for block cipher.",
    "Algorithm failed test vectors.",

    "Buffer overflow.",
    "Invalid input packet.",

    "Invalid number of bits for a PRNG.",
    "Error reading the PRNG.",

    "Invalid cipher specified.",
    "Invalid hash specified.",
    "Invalid PRNG specified.",

    "Out of memory.",

    "Invalid PK key or key type specified for function.",
    "A private PK key is required.",

    "Invalid argument provided.",
    "File Not Found",

    "Invalid PK type.",
    "Invalid PK system.",
    "Duplicate PK key found on keyring.",
    "Key not found in keyring.",
    "Invalid sized parameter.",

    "Invalid size for prime.",
};

/**
   Convert an LTC error code to ASCII
   @param err    The error code
   @return A pointer to the ASCII NUL terminated string for the error or "Invalid error code." if the err code was not valid.
 */
const char *error_to_string(int err) {
    if ((err < 0) || (err >= (int)(sizeof(err_2_str) / sizeof(err_2_str[0])))) {
        return "Invalid error code.";
    } else {
        return err_2_str[err];
    }
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/error_to_string.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

#define DESC_DEF_ONLY



/* $Source: /cvs/libtom/libtomcrypt/src/math/gmp_desc.c,v $ */
/* $Revision: 1.16 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file hash_file.c
   Hash a file, Tom St Denis
 */

/**
   @param hash   The index of the hash desired
   @param fname  The name of the file you wish to hash
   @param out    [out] The destination of the digest
   @param outlen [in/out] The max size and resulting size of the message digest
   @result CRYPT_OK if successful
 */
int hash_file(int hash, const char *fname, unsigned char *out, unsigned long *outlen) {
#ifdef LTC_NO_FILE
    return CRYPT_NOP;
#else
    FILE *in;
    int  err;
    LTC_ARGCHK(fname != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    if ((err = hash_is_valid(hash)) != CRYPT_OK) {
        return err;
    }

    in = fopen(fname, "rb");
    if (in == NULL) {
        return CRYPT_FILE_NOTFOUND;
    }

    err = hash_filehandle(hash, in, out, outlen);
    if (fclose(in) != 0) {
        return CRYPT_ERROR;
    }

    return err;
#endif
}

/* $Source: /cvs/libtom/libtomcrypt/src/hashes/helper/hash_file.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:23 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file hash_filehandle.c
   Hash open files, Tom St Denis
 */

/**
   Hash data from an open file handle.
   @param hash   The index of the hash you want to use
   @param in     The FILE* handle of the file you want to hash
   @param out    [out] The destination of the digest
   @param outlen [in/out] The max size and resulting size of the digest
   @result CRYPT_OK if successful
 */
int hash_filehandle(int hash, FILE *in, unsigned char *out, unsigned long *outlen) {
#ifdef LTC_NO_FILE
    return CRYPT_NOP;
#else
    hash_state    md;
    unsigned char buf[512];
    size_t        x;
    int           err;

    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);
    LTC_ARGCHK(in != NULL);

    if ((err = hash_is_valid(hash)) != CRYPT_OK) {
        return err;
    }

    if (*outlen < hash_descriptor[hash].hashsize) {
        *outlen = hash_descriptor[hash].hashsize;
        return CRYPT_BUFFER_OVERFLOW;
    }
    if ((err = hash_descriptor[hash].init(&md)) != CRYPT_OK) {
        return err;
    }

    *outlen = hash_descriptor[hash].hashsize;
    do {
        x = fread(buf, 1, sizeof(buf), in);
        if ((err = hash_descriptor[hash].process(&md, buf, x)) != CRYPT_OK) {
            return err;
        }
    } while (x == sizeof(buf));
    err = hash_descriptor[hash].done(&md, out);

 #ifdef LTC_CLEAN_STACK
    zeromem(buf, sizeof(buf));
 #endif
    return err;
#endif
}

/* $Source: /cvs/libtom/libtomcrypt/src/hashes/helper/hash_filehandle.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:23 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file hash_memory.c
   Hash memory helper, Tom St Denis
 */

/**
   Hash a block of memory and store the digest.
   @param hash   The index of the hash you wish to use
   @param in     The data you wish to hash
   @param inlen  The length of the data to hash (octets)
   @param out    [out] Where to store the digest
   @param outlen [in/out] Max size and resulting size of the digest
   @return CRYPT_OK if successful
 */
int hash_memory(int hash, const unsigned char *in, unsigned long inlen, unsigned char *out, unsigned long *outlen) {
    hash_state *md;
    int        err;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    if ((err = hash_is_valid(hash)) != CRYPT_OK) {
        return err;
    }

    if (*outlen < hash_descriptor[hash].hashsize) {
        *outlen = hash_descriptor[hash].hashsize;
        return CRYPT_BUFFER_OVERFLOW;
    }

    md = XMALLOC(sizeof(hash_state));
    if (md == NULL) {
        return CRYPT_MEM;
    }

    if ((err = hash_descriptor[hash].init(md)) != CRYPT_OK) {
        goto LBL_ERR;
    }
    if ((err = hash_descriptor[hash].process(md, in, inlen)) != CRYPT_OK) {
        goto LBL_ERR;
    }
    err     = hash_descriptor[hash].done(md, out);
    *outlen = hash_descriptor[hash].hashsize;
LBL_ERR:
#ifdef LTC_CLEAN_STACK
    zeromem(md, sizeof(hash_state));
#endif
    XFREE(md);

    return err;
}

/* $Source: /cvs/libtom/libtomcrypt/src/hashes/helper/hash_memory.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:23 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

#include <stdarg.h>

/**
   @file hash_memory_multi.c
   Hash (multiple buffers) memory helper, Tom St Denis
 */

/**
   Hash multiple (non-adjacent) blocks of memory at once.
   @param hash   The index of the hash you wish to use
   @param out    [out] Where to store the digest
   @param outlen [in/out] Max size and resulting size of the digest
   @param in     The data you wish to hash
   @param inlen  The length of the data to hash (octets)
   @param ...    tuples of (data,len) pairs to hash, terminated with a (NULL,x) (x=don't care)
   @return CRYPT_OK if successful
 */
int hash_memory_multi(int hash, unsigned char *out, unsigned long *outlen,
                      const unsigned char *in, unsigned long inlen, ...) {
    hash_state          *md;
    int                 err;
    va_list             args;
    const unsigned char *curptr;
    unsigned long       curlen;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    if ((err = hash_is_valid(hash)) != CRYPT_OK) {
        return err;
    }

    if (*outlen < hash_descriptor[hash].hashsize) {
        *outlen = hash_descriptor[hash].hashsize;
        return CRYPT_BUFFER_OVERFLOW;
    }

    md = XMALLOC(sizeof(hash_state));
    if (md == NULL) {
        return CRYPT_MEM;
    }

    if ((err = hash_descriptor[hash].init(md)) != CRYPT_OK) {
        goto LBL_ERR;
    }

    va_start(args, inlen);
    curptr = in;
    curlen = inlen;
    for ( ; ; ) {
        /* process buf */
        if ((err = hash_descriptor[hash].process(md, curptr, curlen)) != CRYPT_OK) {
            goto LBL_ERR;
        }
        /* step to next */
        curptr = va_arg(args, const unsigned char *);
        if (curptr == NULL) {
            break;
        }
        curlen = va_arg(args, unsigned long);
    }
    err     = hash_descriptor[hash].done(md, out);
    *outlen = hash_descriptor[hash].hashsize;
LBL_ERR:
#ifdef LTC_CLEAN_STACK
    zeromem(md, sizeof(hash_state));
#endif
    XFREE(md);
    va_end(args);
    return err;
}

/* $Source: /cvs/libtom/libtomcrypt/src/hashes/helper/hash_memory_multi.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:23 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ltc_ecc_is_valid_idx.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

/** Returns whether an ECC idx is valid or not
   @param n   The idx number to check
   @return 1 if valid, 0 if not
 */
int ltc_ecc_is_valid_idx(int n) {
    int x;

    for (x = 0; ltc_ecc_sets[x].size != 0; x++);
    /* -1 is a valid index --- indicating that the domain params were supplied by the user */
    if ((n >= -1) && (n < x)) {
        return 1;
    }
    return 0;
}
#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ltc_ecc_is_valid_idx.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ltc_ecc_map.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

/**
   Map a projective jacbobian point back to affine space
   @param P        [in/out] The point to map
   @param modulus  The modulus of the field the ECC curve is in
   @param mp       The "b" value from montgomery_setup()
   @return CRYPT_OK on success
 */
int ltc_ecc_map(ecc_point *P, void *modulus, void *mp) {
    void *t1, *t2;
    int  err;

    LTC_ARGCHK(P != NULL);
    LTC_ARGCHK(modulus != NULL);
    LTC_ARGCHK(mp != NULL);

    if ((err = mp_init_multi(&t1, &t2, NULL)) != CRYPT_OK) {
        return CRYPT_MEM;
    }

    /* first map z back to normal */
    if ((err = mp_montgomery_reduce(P->z, modulus, mp)) != CRYPT_OK) {
        goto done;
    }

    /* get 1/z */
    if ((err = mp_invmod(P->z, modulus, t1)) != CRYPT_OK) {
        goto done;
    }

    /* get 1/z^2 and 1/z^3 */
    if ((err = mp_sqr(t1, t2)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_mod(t2, modulus, t2)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_mul(t1, t2, t1)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_mod(t1, modulus, t1)) != CRYPT_OK) {
        goto done;
    }

    /* multiply against x/y */
    if ((err = mp_mul(P->x, t2, P->x)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(P->x, modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_mul(P->y, t1, P->y)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(P->y, modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_set(P->z, 1)) != CRYPT_OK) {
        goto done;
    }

    err = CRYPT_OK;
done:
    mp_clear_multi(t1, t2, NULL);
    return err;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ltc_ecc_map.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ltc_ecc_mul2add.c
   ECC Crypto, Shamir's Trick, Tom St Denis
 */

#ifdef LTC_MECC

 #ifdef LTC_ECC_SHAMIR

/** Computes kA*A + kB*B = C using Shamir's Trick
   @param A        First point to multiply
   @param kA       What to multiple A by
   @param B        Second point to multiply
   @param kB       What to multiple B by
   @param C        [out] Destination point (can overlap with A or B
   @param modulus  Modulus for curve
   @return CRYPT_OK on success
 */
int ltc_ecc_mul2add(ecc_point *A, void *kA,
                    ecc_point *B, void *kB,
                    ecc_point *C,
                    void *modulus) {
    ecc_point     *precomp[16];
    unsigned      bitbufA, bitbufB, lenA, lenB, len, x, y, nA, nB, nibble;
    unsigned char *tA, *tB;
    int           err, first;
    void          *mp, *mu;

    /* argchks */
    LTC_ARGCHK(A != NULL);
    LTC_ARGCHK(B != NULL);
    LTC_ARGCHK(C != NULL);
    LTC_ARGCHK(kA != NULL);
    LTC_ARGCHK(kB != NULL);
    LTC_ARGCHK(modulus != NULL);

    /* allocate memory */
    tA = XCALLOC(1, ECC_BUF_SIZE);
    if (tA == NULL) {
        return CRYPT_MEM;
    }
    tB = XCALLOC(1, ECC_BUF_SIZE);
    if (tB == NULL) {
        XFREE(tA);
        return CRYPT_MEM;
    }

    /* get sizes */
    lenA = mp_unsigned_bin_size(kA);
    lenB = mp_unsigned_bin_size(kB);
    len  = MAX(lenA, lenB);

    /* sanity check */
    if ((lenA > ECC_BUF_SIZE) || (lenB > ECC_BUF_SIZE)) {
        err = CRYPT_INVALID_ARG;
        goto ERR_T;
    }

    /* extract and justify kA */
    mp_to_unsigned_bin(kA, (len - lenA) + tA);

    /* extract and justify kB */
    mp_to_unsigned_bin(kB, (len - lenB) + tB);

    /* allocate the table */
    for (x = 0; x < 16; x++) {
        precomp[x] = ltc_ecc_new_point();
        if (precomp[x] == NULL) {
            for (y = 0; y < x; ++y) {
                ltc_ecc_del_point(precomp[y]);
            }
            err = CRYPT_MEM;
            goto ERR_T;
        }
    }

    /* init montgomery reduction */
    if ((err = mp_montgomery_setup(modulus, &mp)) != CRYPT_OK) {
        goto ERR_P;
    }
    if ((err = mp_init(&mu)) != CRYPT_OK) {
        goto ERR_MP;
    }
    if ((err = mp_montgomery_normalization(mu, modulus)) != CRYPT_OK) {
        goto ERR_MU;
    }

    /* copy ones ... */
    if ((err = mp_mulmod(A->x, mu, modulus, precomp[1]->x)) != CRYPT_OK) {
        goto ERR_MU;
    }
    if ((err = mp_mulmod(A->y, mu, modulus, precomp[1]->y)) != CRYPT_OK) {
        goto ERR_MU;
    }
    if ((err = mp_mulmod(A->z, mu, modulus, precomp[1]->z)) != CRYPT_OK) {
        goto ERR_MU;
    }

    if ((err = mp_mulmod(B->x, mu, modulus, precomp[1 << 2]->x)) != CRYPT_OK) {
        goto ERR_MU;
    }
    if ((err = mp_mulmod(B->y, mu, modulus, precomp[1 << 2]->y)) != CRYPT_OK) {
        goto ERR_MU;
    }
    if ((err = mp_mulmod(B->z, mu, modulus, precomp[1 << 2]->z)) != CRYPT_OK) {
        goto ERR_MU;
    }

    /* precomp [i,0](A + B) table */
    if ((err = ltc_mp.ecc_ptdbl(precomp[1], precomp[2], modulus, mp)) != CRYPT_OK) {
        goto ERR_MU;
    }
    if ((err = ltc_mp.ecc_ptadd(precomp[1], precomp[2], precomp[3], modulus, mp)) != CRYPT_OK) {
        goto ERR_MU;
    }

    /* precomp [0,i](A + B) table */
    if ((err = ltc_mp.ecc_ptdbl(precomp[1 << 2], precomp[2 << 2], modulus, mp)) != CRYPT_OK) {
        goto ERR_MU;
    }
    if ((err = ltc_mp.ecc_ptadd(precomp[1 << 2], precomp[2 << 2], precomp[3 << 2], modulus, mp)) != CRYPT_OK) {
        goto ERR_MU;
    }

    /* precomp [i,j](A + B) table (i != 0, j != 0) */
    for (x = 1; x < 4; x++) {
        for (y = 1; y < 4; y++) {
            if ((err = ltc_mp.ecc_ptadd(precomp[x], precomp[(y << 2)], precomp[x + (y << 2)], modulus, mp)) != CRYPT_OK) {
                goto ERR_MU;
            }
        }
    }

    nibble  = 3;
    first   = 1;
    bitbufA = tA[0];
    bitbufB = tB[0];

    /* for every byte of the multiplicands */
    for (x = -1; ; ) {
        /* grab a nibble */
        if (++nibble == 4) {
            ++x;
            if (x == len) break;
            bitbufA = tA[x];
            bitbufB = tB[x];
            nibble  = 0;
        }

        /* extract two bits from both, shift/update */
        nA      = (bitbufA >> 6) & 0x03;
        nB      = (bitbufB >> 6) & 0x03;
        bitbufA = (bitbufA << 2) & 0xFF;
        bitbufB = (bitbufB << 2) & 0xFF;

        /* if both zero, if first, continue */
        if ((nA == 0) && (nB == 0) && (first == 1)) {
            continue;
        }

        /* double twice, only if this isn't the first */
        if (first == 0) {
            /* double twice */
            if ((err = ltc_mp.ecc_ptdbl(C, C, modulus, mp)) != CRYPT_OK) {
                goto ERR_MU;
            }
            if ((err = ltc_mp.ecc_ptdbl(C, C, modulus, mp)) != CRYPT_OK) {
                goto ERR_MU;
            }
        }

        /* if not both zero */
        if ((nA != 0) || (nB != 0)) {
            if (first == 1) {
                /* if first, copy from table */
                first = 0;
                if ((err = mp_copy(precomp[nA + (nB << 2)]->x, C->x)) != CRYPT_OK) {
                    goto ERR_MU;
                }
                if ((err = mp_copy(precomp[nA + (nB << 2)]->y, C->y)) != CRYPT_OK) {
                    goto ERR_MU;
                }
                if ((err = mp_copy(precomp[nA + (nB << 2)]->z, C->z)) != CRYPT_OK) {
                    goto ERR_MU;
                }
            } else {
                /* if not first, add from table */
                if ((err = ltc_mp.ecc_ptadd(C, precomp[nA + (nB << 2)], C, modulus, mp)) != CRYPT_OK) {
                    goto ERR_MU;
                }
            }
        }
    }

    /* reduce to affine */
    err = ltc_ecc_map(C, modulus, mp);

    /* clean up */
ERR_MU:
    mp_clear(mu);
ERR_MP:
    mp_montgomery_free(mp);
ERR_P:
    for (x = 0; x < 16; x++) {
        ltc_ecc_del_point(precomp[x]);
    }
ERR_T:
  #ifdef LTC_CLEAN_STACK
    zeromem(tA, ECC_BUF_SIZE);
    zeromem(tB, ECC_BUF_SIZE);
  #endif
    XFREE(tA);
    XFREE(tB);

    return err;
}
 #endif
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ltc_ecc_mul2add.c,v $ */
/* $Revision: 1.8 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ltc_ecc_mulmod.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC
 #ifndef LTC_ECC_TIMING_RESISTANT

/* size of sliding window, don't change this! */
  #define WINSIZE    4

/**
   Perform a point multiplication
   @param k    The scalar to multiply by
   @param G    The base point
   @param R    [out] Destination for kG
   @param modulus  The modulus of the field the ECC curve is in
   @param map      Boolean whether to map back to affine or not (1==map, 0 == leave in projective)
   @return CRYPT_OK on success
 */
int ltc_ecc_mulmod(void *k, ecc_point *G, ecc_point *R, void *modulus, int map) {
    ecc_point     *tG, *M[8];
    int           i, j, err;
    void          *mu, *mp;
    unsigned long buf;
    int           first, bitbuf, bitcpy, bitcnt, mode, digidx;

    LTC_ARGCHK(k != NULL);
    LTC_ARGCHK(G != NULL);
    LTC_ARGCHK(R != NULL);
    LTC_ARGCHK(modulus != NULL);

    /* init montgomery reduction */
    if ((err = mp_montgomery_setup(modulus, &mp)) != CRYPT_OK) {
        return err;
    }
    if ((err = mp_init(&mu)) != CRYPT_OK) {
        mp_montgomery_free(mp);
        return err;
    }
    if ((err = mp_montgomery_normalization(mu, modulus)) != CRYPT_OK) {
        mp_montgomery_free(mp);
        mp_clear(mu);
        return err;
    }

    /* alloc ram for window temps */
    for (i = 0; i < 8; i++) {
        M[i] = ltc_ecc_new_point();
        if (M[i] == NULL) {
            for (j = 0; j < i; j++) {
                ltc_ecc_del_point(M[j]);
            }
            mp_montgomery_free(mp);
            mp_clear(mu);
            return CRYPT_MEM;
        }
    }

    /* make a copy of G incase R==G */
    tG = ltc_ecc_new_point();
    if (tG == NULL) {
        err = CRYPT_MEM;
        goto done;
    }

    /* tG = G  and convert to montgomery */
    if (mp_cmp_d(mu, 1) == LTC_MP_EQ) {
        if ((err = mp_copy(G->x, tG->x)) != CRYPT_OK) {
            goto done;
        }
        if ((err = mp_copy(G->y, tG->y)) != CRYPT_OK) {
            goto done;
        }
        if ((err = mp_copy(G->z, tG->z)) != CRYPT_OK) {
            goto done;
        }
    } else {
        if ((err = mp_mulmod(G->x, mu, modulus, tG->x)) != CRYPT_OK) {
            goto done;
        }
        if ((err = mp_mulmod(G->y, mu, modulus, tG->y)) != CRYPT_OK) {
            goto done;
        }
        if ((err = mp_mulmod(G->z, mu, modulus, tG->z)) != CRYPT_OK) {
            goto done;
        }
    }
    mp_clear(mu);
    mu = NULL;

    /* calc the M tab, which holds kG for k==8..15 */
    /* M[0] == 8G */
    if ((err = ltc_mp.ecc_ptdbl(tG, M[0], modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    if ((err = ltc_mp.ecc_ptdbl(M[0], M[0], modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    if ((err = ltc_mp.ecc_ptdbl(M[0], M[0], modulus, mp)) != CRYPT_OK) {
        goto done;
    }

    /* now find (8+k)G for k=1..7 */
    for (j = 9; j < 16; j++) {
        if ((err = ltc_mp.ecc_ptadd(M[j - 9], tG, M[j - 8], modulus, mp)) != CRYPT_OK) {
            goto done;
        }
    }

    /* setup sliding window */
    mode   = 0;
    bitcnt = 1;
    buf    = 0;
    digidx = mp_get_digit_count(k) - 1;
    bitcpy = bitbuf = 0;
    first  = 1;

    /* perform ops */
    for ( ; ; ) {
        /* grab next digit as required */
        if (--bitcnt == 0) {
            if (digidx == -1) {
                break;
            }
            buf    = mp_get_digit(k, digidx);
            bitcnt = (int)ltc_mp.bits_per_digit;
            --digidx;
        }

        /* grab the next msb from the ltiplicand */
        i     = (buf >> (ltc_mp.bits_per_digit - 1)) & 1;
        buf <<= 1;

        /* skip leading zero bits */
        if ((mode == 0) && (i == 0)) {
            continue;
        }

        /* if the bit is zero and mode == 1 then we double */
        if ((mode == 1) && (i == 0)) {
            if ((err = ltc_mp.ecc_ptdbl(R, R, modulus, mp)) != CRYPT_OK) {
                goto done;
            }
            continue;
        }

        /* else we add it to the window */
        bitbuf |= (i << (WINSIZE - ++bitcpy));
        mode    = 2;

        if (bitcpy == WINSIZE) {
            /* if this is the first window we do a simple copy */
            if (first == 1) {
                /* R = kG [k = first window] */
                if ((err = mp_copy(M[bitbuf - 8]->x, R->x)) != CRYPT_OK) {
                    goto done;
                }
                if ((err = mp_copy(M[bitbuf - 8]->y, R->y)) != CRYPT_OK) {
                    goto done;
                }
                if ((err = mp_copy(M[bitbuf - 8]->z, R->z)) != CRYPT_OK) {
                    goto done;
                }
                first = 0;
            } else {
                /* normal window */
                /* ok window is filled so double as required and add  */
                /* double first */
                for (j = 0; j < WINSIZE; j++) {
                    if ((err = ltc_mp.ecc_ptdbl(R, R, modulus, mp)) != CRYPT_OK) {
                        goto done;
                    }
                }

                /* then add, bitbuf will be 8..15 [8..2^WINSIZE] guaranteed */
                if ((err = ltc_mp.ecc_ptadd(R, M[bitbuf - 8], R, modulus, mp)) != CRYPT_OK) {
                    goto done;
                }
            }
            /* empty window and reset */
            bitcpy = bitbuf = 0;
            mode   = 1;
        }
    }

    /* if bits remain then double/add */
    if ((mode == 2) && (bitcpy > 0)) {
        /* double then add */
        for (j = 0; j < bitcpy; j++) {
            /* only double if we have had at least one add first */
            if (first == 0) {
                if ((err = ltc_mp.ecc_ptdbl(R, R, modulus, mp)) != CRYPT_OK) {
                    goto done;
                }
            }

            bitbuf <<= 1;
            if ((bitbuf & (1 << WINSIZE)) != 0) {
                if (first == 1) {
                    /* first add, so copy */
                    if ((err = mp_copy(tG->x, R->x)) != CRYPT_OK) {
                        goto done;
                    }
                    if ((err = mp_copy(tG->y, R->y)) != CRYPT_OK) {
                        goto done;
                    }
                    if ((err = mp_copy(tG->z, R->z)) != CRYPT_OK) {
                        goto done;
                    }
                    first = 0;
                } else {
                    /* then add */
                    if ((err = ltc_mp.ecc_ptadd(R, tG, R, modulus, mp)) != CRYPT_OK) {
                        goto done;
                    }
                }
            }
        }
    }

    /* map R back from projective space */
    if (map) {
        err = ltc_ecc_map(R, modulus, mp);
    } else {
        err = CRYPT_OK;
    }
done:
    if (mu != NULL) {
        mp_clear(mu);
    }
    mp_montgomery_free(mp);
    ltc_ecc_del_point(tG);
    for (i = 0; i < 8; i++) {
        ltc_ecc_del_point(M[i]);
    }
    return err;
}
 #endif

 #undef WINSIZE
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ltc_ecc_mulmod.c,v $ */
/* $Revision: 1.26 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ltc_ecc_mulmod_timing.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

 #ifdef LTC_ECC_TIMING_RESISTANT

/**
   Perform a point multiplication  (timing resistant)
   @param k    The scalar to multiply by
   @param G    The base point
   @param R    [out] Destination for kG
   @param modulus  The modulus of the field the ECC curve is in
   @param map      Boolean whether to map back to affine or not (1==map, 0 == leave in projective)
   @return CRYPT_OK on success
 */
int ltc_ecc_mulmod(void *k, ecc_point *G, ecc_point *R, void *modulus, int map) {
    ecc_point     *tG, *M[3];
    int           i, j, err;
    void          *mu, *mp;
    unsigned long buf;
    int           first, bitbuf, bitcpy, bitcnt, mode, digidx;

    LTC_ARGCHK(k != NULL);
    LTC_ARGCHK(G != NULL);
    LTC_ARGCHK(R != NULL);
    LTC_ARGCHK(modulus != NULL);

    /* init montgomery reduction */
    if ((err = mp_montgomery_setup(modulus, &mp)) != CRYPT_OK) {
        return err;
    }
    if ((err = mp_init(&mu)) != CRYPT_OK) {
        mp_montgomery_free(mp);
        return err;
    }
    if ((err = mp_montgomery_normalization(mu, modulus)) != CRYPT_OK) {
        mp_clear(mu);
        mp_montgomery_free(mp);
        return err;
    }

    /* alloc ram for window temps */
    for (i = 0; i < 3; i++) {
        M[i] = ltc_ecc_new_point();
        if (M[i] == NULL) {
            for (j = 0; j < i; j++) {
                ltc_ecc_del_point(M[j]);
            }
            mp_clear(mu);
            mp_montgomery_free(mp);
            return CRYPT_MEM;
        }
    }

    /* make a copy of G incase R==G */
    tG = ltc_ecc_new_point();
    if (tG == NULL) {
        err = CRYPT_MEM;
        goto done;
    }

    /* tG = G  and convert to montgomery */
    if ((err = mp_mulmod(G->x, mu, modulus, tG->x)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_mulmod(G->y, mu, modulus, tG->y)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_mulmod(G->z, mu, modulus, tG->z)) != CRYPT_OK) {
        goto done;
    }
    mp_clear(mu);
    mu = NULL;

    /* calc the M tab */
    /* M[0] == G */
    if ((err = mp_copy(tG->x, M[0]->x)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_copy(tG->y, M[0]->y)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_copy(tG->z, M[0]->z)) != CRYPT_OK) {
        goto done;
    }
    /* M[1] == 2G */
    if ((err = ltc_mp.ecc_ptdbl(tG, M[1], modulus, mp)) != CRYPT_OK) {
        goto done;
    }

    /* setup sliding window */
    mode   = 0;
    bitcnt = 1;
    buf    = 0;
    digidx = mp_get_digit_count(k) - 1;
    bitcpy = bitbuf = 0;
    first  = 1;

    /* perform ops */
    for ( ; ; ) {
        /* grab next digit as required */
        if (--bitcnt == 0) {
            if (digidx == -1) {
                break;
            }
            buf    = mp_get_digit(k, digidx);
            bitcnt = (int)MP_DIGIT_BIT;
            --digidx;
        }

        /* grab the next msb from the ltiplicand */
        i     = (buf >> (MP_DIGIT_BIT - 1)) & 1;
        buf <<= 1;

        if ((mode == 0) && (i == 0)) {
            /* dummy operations */
            if ((err = ltc_mp.ecc_ptadd(M[0], M[1], M[2], modulus, mp)) != CRYPT_OK) {
                goto done;
            }
            if ((err = ltc_mp.ecc_ptdbl(M[1], M[2], modulus, mp)) != CRYPT_OK) {
                goto done;
            }
            continue;
        }

        if ((mode == 0) && (i == 1)) {
            mode = 1;
            /* dummy operations */
            if ((err = ltc_mp.ecc_ptadd(M[0], M[1], M[2], modulus, mp)) != CRYPT_OK) {
                goto done;
            }
            if ((err = ltc_mp.ecc_ptdbl(M[1], M[2], modulus, mp)) != CRYPT_OK) {
                goto done;
            }
            continue;
        }

        if ((err = ltc_mp.ecc_ptadd(M[0], M[1], M[i ^ 1], modulus, mp)) != CRYPT_OK) {
            goto done;
        }
        if ((err = ltc_mp.ecc_ptdbl(M[i], M[i], modulus, mp)) != CRYPT_OK) {
            goto done;
        }
    }

    /* copy result out */
    if ((err = mp_copy(M[0]->x, R->x)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_copy(M[0]->y, R->y)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_copy(M[0]->z, R->z)) != CRYPT_OK) {
        goto done;
    }

    /* map R back from projective space */
    if (map) {
        err = ltc_ecc_map(R, modulus, mp);
    } else {
        err = CRYPT_OK;
    }
done:
    if (mu != NULL) {
        mp_clear(mu);
    }
    mp_montgomery_free(mp);
    ltc_ecc_del_point(tG);
    for (i = 0; i < 3; i++) {
        ltc_ecc_del_point(M[i]);
    }
    return err;
}
 #endif
#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ltc_ecc_mulmod_timing.c,v $ */
/* $Revision: 1.13 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ltc_ecc_points.c
   ECC Crypto, Tom St Denis
 */

#ifdef LTC_MECC

/**
   Allocate a new ECC point
   @return A newly allocated point or NULL on error
 */
ecc_point *ltc_ecc_new_point(void) {
    ecc_point *p;

    p = XCALLOC(1, sizeof(*p));
    if (p == NULL) {
        return NULL;
    }
    if (mp_init_multi(&p->x, &p->y, &p->z, NULL) != CRYPT_OK) {
        XFREE(p);
        return NULL;
    }
    return p;
}

/** Free an ECC point from memory
   @param p   The point to free
 */
void ltc_ecc_del_point(ecc_point *p) {
    /* prevents free'ing null arguments */
    if (p != NULL) {
        mp_clear_multi(p->x, p->y, p->z, NULL); /* note: p->z may be NULL but that's ok with this function anyways */
        XFREE(p);
    }
}
#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ltc_ecc_points.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ltc_ecc_projective_add_point.c
   ECC Crypto, Tom St Denis
 */

#if defined(LTC_MECC) && (!defined(LTC_MECC_ACCEL) || defined(LTM_LTC_DESC))

/**
   Add two ECC points
   @param P        The point to add
   @param Q        The point to add
   @param R        [out] The destination of the double
   @param modulus  The modulus of the field the ECC curve is in
   @param mp       The "b" value from montgomery_setup()
   @return CRYPT_OK on success
 */
int ltc_ecc_projective_add_point(ecc_point *P, ecc_point *Q, ecc_point *R, void *modulus, void *mp) {
    void *t1, *t2, *x, *y, *z;
    int  err;

    LTC_ARGCHK(P != NULL);
    LTC_ARGCHK(Q != NULL);
    LTC_ARGCHK(R != NULL);
    LTC_ARGCHK(modulus != NULL);
    LTC_ARGCHK(mp != NULL);

    if ((err = mp_init_multi(&t1, &t2, &x, &y, &z, NULL)) != CRYPT_OK) {
        return err;
    }

    /* should we dbl instead? */
    if ((err = mp_sub(modulus, Q->y, t1)) != CRYPT_OK) {
        goto done;
    }

    if ((mp_cmp(P->x, Q->x) == LTC_MP_EQ) &&
        ((Q->z != NULL) && (mp_cmp(P->z, Q->z) == LTC_MP_EQ)) &&
        ((mp_cmp(P->y, Q->y) == LTC_MP_EQ) || (mp_cmp(P->y, t1) == LTC_MP_EQ))) {
        mp_clear_multi(t1, t2, x, y, z, NULL);
        return ltc_ecc_projective_dbl_point(P, R, modulus, mp);
    }

    if ((err = mp_copy(P->x, x)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_copy(P->y, y)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_copy(P->z, z)) != CRYPT_OK) {
        goto done;
    }

    /* if Z is one then these are no-operations */
    if (Q->z != NULL) {
        /* T1 = Z' * Z' */
        if ((err = mp_sqr(Q->z, t1)) != CRYPT_OK) {
            goto done;
        }
        if ((err = mp_montgomery_reduce(t1, modulus, mp)) != CRYPT_OK) {
            goto done;
        }
        /* X = X * T1 */
        if ((err = mp_mul(t1, x, x)) != CRYPT_OK) {
            goto done;
        }
        if ((err = mp_montgomery_reduce(x, modulus, mp)) != CRYPT_OK) {
            goto done;
        }
        /* T1 = Z' * T1 */
        if ((err = mp_mul(Q->z, t1, t1)) != CRYPT_OK) {
            goto done;
        }
        if ((err = mp_montgomery_reduce(t1, modulus, mp)) != CRYPT_OK) {
            goto done;
        }
        /* Y = Y * T1 */
        if ((err = mp_mul(t1, y, y)) != CRYPT_OK) {
            goto done;
        }
        if ((err = mp_montgomery_reduce(y, modulus, mp)) != CRYPT_OK) {
            goto done;
        }
    }

    /* T1 = Z*Z */
    if ((err = mp_sqr(z, t1)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(t1, modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    /* T2 = X' * T1 */
    if ((err = mp_mul(Q->x, t1, t2)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(t2, modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    /* T1 = Z * T1 */
    if ((err = mp_mul(z, t1, t1)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(t1, modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    /* T1 = Y' * T1 */
    if ((err = mp_mul(Q->y, t1, t1)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(t1, modulus, mp)) != CRYPT_OK) {
        goto done;
    }

    /* Y = Y - T1 */
    if ((err = mp_sub(y, t1, y)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp_d(y, 0) == LTC_MP_LT) {
        if ((err = mp_add(y, modulus, y)) != CRYPT_OK) {
            goto done;
        }
    }
    /* T1 = 2T1 */
    if ((err = mp_add(t1, t1, t1)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp(t1, modulus) != LTC_MP_LT) {
        if ((err = mp_sub(t1, modulus, t1)) != CRYPT_OK) {
            goto done;
        }
    }
    /* T1 = Y + T1 */
    if ((err = mp_add(t1, y, t1)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp(t1, modulus) != LTC_MP_LT) {
        if ((err = mp_sub(t1, modulus, t1)) != CRYPT_OK) {
            goto done;
        }
    }
    /* X = X - T2 */
    if ((err = mp_sub(x, t2, x)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp_d(x, 0) == LTC_MP_LT) {
        if ((err = mp_add(x, modulus, x)) != CRYPT_OK) {
            goto done;
        }
    }
    /* T2 = 2T2 */
    if ((err = mp_add(t2, t2, t2)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp(t2, modulus) != LTC_MP_LT) {
        if ((err = mp_sub(t2, modulus, t2)) != CRYPT_OK) {
            goto done;
        }
    }
    /* T2 = X + T2 */
    if ((err = mp_add(t2, x, t2)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp(t2, modulus) != LTC_MP_LT) {
        if ((err = mp_sub(t2, modulus, t2)) != CRYPT_OK) {
            goto done;
        }
    }

    /* if Z' != 1 */
    if (Q->z != NULL) {
        /* Z = Z * Z' */
        if ((err = mp_mul(z, Q->z, z)) != CRYPT_OK) {
            goto done;
        }
        if ((err = mp_montgomery_reduce(z, modulus, mp)) != CRYPT_OK) {
            goto done;
        }
    }

    /* Z = Z * X */
    if ((err = mp_mul(z, x, z)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(z, modulus, mp)) != CRYPT_OK) {
        goto done;
    }

    /* T1 = T1 * X  */
    if ((err = mp_mul(t1, x, t1)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(t1, modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    /* X = X * X */
    if ((err = mp_sqr(x, x)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(x, modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    /* T2 = T2 * x */
    if ((err = mp_mul(t2, x, t2)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(t2, modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    /* T1 = T1 * X  */
    if ((err = mp_mul(t1, x, t1)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(t1, modulus, mp)) != CRYPT_OK) {
        goto done;
    }

    /* X = Y*Y */
    if ((err = mp_sqr(y, x)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(x, modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    /* X = X - T2 */
    if ((err = mp_sub(x, t2, x)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp_d(x, 0) == LTC_MP_LT) {
        if ((err = mp_add(x, modulus, x)) != CRYPT_OK) {
            goto done;
        }
    }

    /* T2 = T2 - X */
    if ((err = mp_sub(t2, x, t2)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp_d(t2, 0) == LTC_MP_LT) {
        if ((err = mp_add(t2, modulus, t2)) != CRYPT_OK) {
            goto done;
        }
    }
    /* T2 = T2 - X */
    if ((err = mp_sub(t2, x, t2)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp_d(t2, 0) == LTC_MP_LT) {
        if ((err = mp_add(t2, modulus, t2)) != CRYPT_OK) {
            goto done;
        }
    }
    /* T2 = T2 * Y */
    if ((err = mp_mul(t2, y, t2)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(t2, modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    /* Y = T2 - T1 */
    if ((err = mp_sub(t2, t1, y)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp_d(y, 0) == LTC_MP_LT) {
        if ((err = mp_add(y, modulus, y)) != CRYPT_OK) {
            goto done;
        }
    }
    /* Y = Y/2 */
    if (mp_isodd(y)) {
        if ((err = mp_add(y, modulus, y)) != CRYPT_OK) {
            goto done;
        }
    }
    if ((err = mp_div_2(y, y)) != CRYPT_OK) {
        goto done;
    }

    if ((err = mp_copy(x, R->x)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_copy(y, R->y)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_copy(z, R->z)) != CRYPT_OK) {
        goto done;
    }

    err = CRYPT_OK;
done:
    mp_clear_multi(t1, t2, x, y, z, NULL);
    return err;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ltc_ecc_projective_add_point.c,v $ */
/* $Revision: 1.16 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */


/**
   @file ltc_ecc_projective_dbl_point.c
   ECC Crypto, Tom St Denis
 */

#if defined(LTC_MECC) && (!defined(LTC_MECC_ACCEL) || defined(LTM_LTC_DESC))

/**
   Double an ECC point
   @param P   The point to double
   @param R   [out] The destination of the double
   @param modulus  The modulus of the field the ECC curve is in
   @param mp       The "b" value from montgomery_setup()
   @return CRYPT_OK on success
 */
int ltc_ecc_projective_dbl_point(ecc_point *P, ecc_point *R, void *modulus, void *mp) {
    void *t1, *t2;
    int  err;

    LTC_ARGCHK(P != NULL);
    LTC_ARGCHK(R != NULL);
    LTC_ARGCHK(modulus != NULL);
    LTC_ARGCHK(mp != NULL);

    if ((err = mp_init_multi(&t1, &t2, NULL)) != CRYPT_OK) {
        return err;
    }

    if (P != R) {
        if ((err = mp_copy(P->x, R->x)) != CRYPT_OK) {
            goto done;
        }
        if ((err = mp_copy(P->y, R->y)) != CRYPT_OK) {
            goto done;
        }
        if ((err = mp_copy(P->z, R->z)) != CRYPT_OK) {
            goto done;
        }
    }

    /* t1 = Z * Z */
    if ((err = mp_sqr(R->z, t1)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(t1, modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    /* Z = Y * Z */
    if ((err = mp_mul(R->z, R->y, R->z)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(R->z, modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    /* Z = 2Z */
    if ((err = mp_add(R->z, R->z, R->z)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp(R->z, modulus) != LTC_MP_LT) {
        if ((err = mp_sub(R->z, modulus, R->z)) != CRYPT_OK) {
            goto done;
        }
    }

    /* T2 = X - T1 */
    if ((err = mp_sub(R->x, t1, t2)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp_d(t2, 0) == LTC_MP_LT) {
        if ((err = mp_add(t2, modulus, t2)) != CRYPT_OK) {
            goto done;
        }
    }
    /* T1 = X + T1 */
    if ((err = mp_add(t1, R->x, t1)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp(t1, modulus) != LTC_MP_LT) {
        if ((err = mp_sub(t1, modulus, t1)) != CRYPT_OK) {
            goto done;
        }
    }
    /* T2 = T1 * T2 */
    if ((err = mp_mul(t1, t2, t2)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(t2, modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    /* T1 = 2T2 */
    if ((err = mp_add(t2, t2, t1)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp(t1, modulus) != LTC_MP_LT) {
        if ((err = mp_sub(t1, modulus, t1)) != CRYPT_OK) {
            goto done;
        }
    }
    /* T1 = T1 + T2 */
    if ((err = mp_add(t1, t2, t1)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp(t1, modulus) != LTC_MP_LT) {
        if ((err = mp_sub(t1, modulus, t1)) != CRYPT_OK) {
            goto done;
        }
    }

    /* Y = 2Y */
    if ((err = mp_add(R->y, R->y, R->y)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp(R->y, modulus) != LTC_MP_LT) {
        if ((err = mp_sub(R->y, modulus, R->y)) != CRYPT_OK) {
            goto done;
        }
    }
    /* Y = Y * Y */
    if ((err = mp_sqr(R->y, R->y)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(R->y, modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    /* T2 = Y * Y */
    if ((err = mp_sqr(R->y, t2)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(t2, modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    /* T2 = T2/2 */
    if (mp_isodd(t2)) {
        if ((err = mp_add(t2, modulus, t2)) != CRYPT_OK) {
            goto done;
        }
    }
    if ((err = mp_div_2(t2, t2)) != CRYPT_OK) {
        goto done;
    }
    /* Y = Y * X */
    if ((err = mp_mul(R->y, R->x, R->y)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(R->y, modulus, mp)) != CRYPT_OK) {
        goto done;
    }

    /* X  = T1 * T1 */
    if ((err = mp_sqr(t1, R->x)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(R->x, modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    /* X = X - Y */
    if ((err = mp_sub(R->x, R->y, R->x)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp_d(R->x, 0) == LTC_MP_LT) {
        if ((err = mp_add(R->x, modulus, R->x)) != CRYPT_OK) {
            goto done;
        }
    }
    /* X = X - Y */
    if ((err = mp_sub(R->x, R->y, R->x)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp_d(R->x, 0) == LTC_MP_LT) {
        if ((err = mp_add(R->x, modulus, R->x)) != CRYPT_OK) {
            goto done;
        }
    }

    /* Y = Y - X */
    if ((err = mp_sub(R->y, R->x, R->y)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp_d(R->y, 0) == LTC_MP_LT) {
        if ((err = mp_add(R->y, modulus, R->y)) != CRYPT_OK) {
            goto done;
        }
    }
    /* Y = Y * T1 */
    if ((err = mp_mul(R->y, t1, R->y)) != CRYPT_OK) {
        goto done;
    }
    if ((err = mp_montgomery_reduce(R->y, modulus, mp)) != CRYPT_OK) {
        goto done;
    }
    /* Y = Y - T2 */
    if ((err = mp_sub(R->y, t2, R->y)) != CRYPT_OK) {
        goto done;
    }
    if (mp_cmp_d(R->y, 0) == LTC_MP_LT) {
        if ((err = mp_add(R->y, modulus, R->y)) != CRYPT_OK) {
            goto done;
        }
    }

    err = CRYPT_OK;
done:
    mp_clear_multi(t1, t2, NULL);
    return err;
}
#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ltc_ecc_projective_dbl_point.c,v $ */
/* $Revision: 1.11 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

#define DESC_DEF_ONLY

#ifdef LTM_DESC

#undef mp_init
#undef mp_init_multi
#undef mp_clear
#undef mp_clear_multi
#undef mp_init_copy
#undef mp_neg
#undef mp_copy
#undef mp_set
#undef mp_set_int
#undef mp_get_int
#undef mp_get_digit
#undef mp_get_digit_count
#undef mp_cmp
#undef mp_cmp_d
#undef mp_count_bits
#undef mp_cnt_lsb
#undef mp_2expt
#undef mp_read_radix
#undef mp_toradix
#undef mp_unsigned_bin_size
#undef mp_to_unsigned_bin
#undef mp_read_unsigned_bin
#undef mp_add
#undef mp_add_d
#undef mp_sub
#undef mp_sub_d
#undef mp_mul
#undef mp_mul_d
#undef mp_sqr
#undef mp_div
#undef mp_div_2
#undef mp_mod
#undef mp_mod_d
#undef mp_gcd
#undef mp_lcm
#undef mp_mulmod
#undef mp_sqrmod
#undef mp_invmod
#undef mp_montgomery_setup
#undef mp_montgomery_normalization
#undef mp_montgomery_reduce
#undef mp_montgomery_free
#undef mp_exptmod
#undef mp_prime_is_prime
#undef mp_iszero
#undef mp_isodd
#undef mp_exch
#undef mp_tohex

static const struct {
    int mpi_code, ltc_code;
} mpi_to_ltc_codes[] = {
    { MP_OKAY, CRYPT_OK          },
    { MP_MEM,  CRYPT_MEM         },
    { MP_VAL,  CRYPT_INVALID_ARG },
};

/**
   Convert a MPI error to a LTC error (Possibly the most powerful function ever!  Oh wait... no)
   @param err    The error to convert
   @return The equivalent LTC error code or CRYPT_ERROR if none found
 */
static int mpi_to_ltc_error(int err) {
    int x;

    for (x = 0; x < (int)(sizeof(mpi_to_ltc_codes) / sizeof(mpi_to_ltc_codes[0])); x++) {
        if (err == mpi_to_ltc_codes[x].mpi_code) {
            return mpi_to_ltc_codes[x].ltc_code;
        }
    }
    return CRYPT_ERROR;
}

static int init(void **a) {
    int err;

    LTC_ARGCHK(a != NULL);

    *a = XCALLOC(1, sizeof(mp_int));
    if (*a == NULL) {
        return CRYPT_MEM;
    }
    if ((err = mpi_to_ltc_error(mp_init(*a))) != CRYPT_OK) {
        XFREE(*a);
    }
    return err;
}

static void deinit(void *a) {
    LTC_ARGCHKVD(a != NULL);
    mp_clear(a);
    XFREE(a);
}

static int neg(void *a, void *b) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    return mpi_to_ltc_error(mp_neg(a, b));
}

static int copy(void *a, void *b) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    return mpi_to_ltc_error(mp_copy(a, b));
}

static int init_copy(void **a, void *b) {
    if (init(a) != CRYPT_OK) {
        return CRYPT_MEM;
    }
    return copy(b, *a);
}

/* ---- trivial ---- */
static int set_int(void *a, unsigned long b) {
    LTC_ARGCHK(a != NULL);
    return mpi_to_ltc_error(mp_set_int(a, b));
}

static unsigned long get_int(void *a) {
    LTC_ARGCHK(a != NULL);
    return mp_get_int(a);
}

static unsigned long get_digit(void *a, int n) {
    mp_int *A;

    LTC_ARGCHK(a != NULL);
    A = a;
    return (n >= A->used || n < 0) ? 0 : A->dp[n];
}

static int get_digit_count(void *a) {
    mp_int *A;

    LTC_ARGCHK(a != NULL);
    A = a;
    return A->used;
}

static int compare(void *a, void *b) {
    int ret;

    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    ret = mp_cmp(a, b);
    switch (ret) {
        case MP_LT:
            return LTC_MP_LT;

        case MP_EQ:
            return LTC_MP_EQ;

        case MP_GT:
            return LTC_MP_GT;
    }
    return 0;
}

static int compare_d(void *a, unsigned long b) {
    int ret;

    LTC_ARGCHK(a != NULL);
    ret = mp_cmp_d(a, b);
    switch (ret) {
        case MP_LT:
            return LTC_MP_LT;

        case MP_EQ:
            return LTC_MP_EQ;

        case MP_GT:
            return LTC_MP_GT;
    }
    return 0;
}

static int count_bits(void *a) {
    LTC_ARGCHK(a != NULL);
    return mp_count_bits(a);
}

static int count_lsb_bits(void *a) {
    LTC_ARGCHK(a != NULL);
    return mp_cnt_lsb(a);
}

static int twoexpt(void *a, int n) {
    LTC_ARGCHK(a != NULL);
    return mpi_to_ltc_error(mp_2expt(a, n));
}

/* ---- conversions ---- */

/* read ascii string */
static int read_radix(void *a, const char *b, int radix) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    return mpi_to_ltc_error(mp_read_radix(a, b, radix));
}

/* write one */
static int write_radix(void *a, char *b, int radix) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    return mpi_to_ltc_error(mp_toradix(a, b, radix));
}

/* get size as unsigned char string */
static unsigned long unsigned_size(void *a) {
    LTC_ARGCHK(a != NULL);
    return mp_unsigned_bin_size(a);
}

/* store */
static int unsigned_write(void *a, unsigned char *b) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    return mpi_to_ltc_error(mp_to_unsigned_bin(a, b));
}

/* read */
static int unsigned_read(void *a, unsigned char *b, unsigned long len) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    return mpi_to_ltc_error(mp_read_unsigned_bin(a, b, len));
}

/* add */
static int add(void *a, void *b, void *c) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    LTC_ARGCHK(c != NULL);
    return mpi_to_ltc_error(mp_add(a, b, c));
}

static int addi(void *a, unsigned long b, void *c) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(c != NULL);
    return mpi_to_ltc_error(mp_add_d(a, b, c));
}

/* sub */
static int sub(void *a, void *b, void *c) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    LTC_ARGCHK(c != NULL);
    return mpi_to_ltc_error(mp_sub(a, b, c));
}

static int subi(void *a, unsigned long b, void *c) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(c != NULL);
    return mpi_to_ltc_error(mp_sub_d(a, b, c));
}

/* mul */
static int mul(void *a, void *b, void *c) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    LTC_ARGCHK(c != NULL);
    return mpi_to_ltc_error(mp_mul(a, b, c));
}

static int muli(void *a, unsigned long b, void *c) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(c != NULL);
    return mpi_to_ltc_error(mp_mul_d(a, b, c));
}

/* sqr */
static int sqr(void *a, void *b) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    return mpi_to_ltc_error(mp_sqr(a, b));
}

/* div */
static int divide(void *a, void *b, void *c, void *d) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    return mpi_to_ltc_error(mp_div(a, b, c, d));
}

static int div_2(void *a, void *b) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    return mpi_to_ltc_error(mp_div_2(a, b));
}

/* modi */
static int modi(void *a, unsigned long b, unsigned long *c) {
    mp_digit tmp;
    int      err;

    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(c != NULL);

    if ((err = mpi_to_ltc_error(mp_mod_d(a, b, &tmp))) != CRYPT_OK) {
        return err;
    }
    *c = tmp;
    return CRYPT_OK;
}

/* gcd */
static int gcd(void *a, void *b, void *c) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    LTC_ARGCHK(c != NULL);
    return mpi_to_ltc_error(mp_gcd(a, b, c));
}

/* lcm */
static int lcm(void *a, void *b, void *c) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    LTC_ARGCHK(c != NULL);
    return mpi_to_ltc_error(mp_lcm(a, b, c));
}

static int mulmod(void *a, void *b, void *c, void *d) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    LTC_ARGCHK(c != NULL);
    LTC_ARGCHK(d != NULL);
    return mpi_to_ltc_error(mp_mulmod(a, b, c, d));
}

static int sqrmod(void *a, void *b, void *c) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    LTC_ARGCHK(c != NULL);
    return mpi_to_ltc_error(mp_sqrmod(a, b, c));
}

/* invmod */
static int invmod(void *a, void *b, void *c) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    LTC_ARGCHK(c != NULL);
    return mpi_to_ltc_error(mp_invmod(a, b, c));
}

/* setup */
static int montgomery_setup(void *a, void **b) {
    int err;

    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    *b = XCALLOC(1, sizeof(mp_digit));
    if (*b == NULL) {
        return CRYPT_MEM;
    }
    if ((err = mpi_to_ltc_error(mp_montgomery_setup(a, (mp_digit *)*b))) != CRYPT_OK) {
        XFREE(*b);
    }
    return err;
}

/* get normalization value */
static int montgomery_normalization(void *a, void *b) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    return mpi_to_ltc_error(mp_montgomery_calc_normalization(a, b));
}

/* reduce */
static int montgomery_reduce(void *a, void *b, void *c) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    LTC_ARGCHK(c != NULL);
    return mpi_to_ltc_error(mp_montgomery_reduce(a, b, *((mp_digit *)c)));
}

/* clean up */
static void montgomery_deinit(void *a) {
    XFREE(a);
}

static int exptmod(void *a, void *b, void *c, void *d) {
    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    LTC_ARGCHK(c != NULL);
    LTC_ARGCHK(d != NULL);
    return mpi_to_ltc_error(mp_exptmod(a, b, c, d));
}

static int isprime(void *a, int *b) {
    int err;

    LTC_ARGCHK(a != NULL);
    LTC_ARGCHK(b != NULL);
    err = mpi_to_ltc_error(mp_prime_is_prime(a, 8, b));
    *b  = (*b == MP_YES) ? LTC_MP_YES : LTC_MP_NO;
    return err;
}

const ltc_math_descriptor ltm_desc = {
    "LibTomMath",
    (int)DIGIT_BIT,

    &init,
    &init_copy,
    &deinit,

    &neg,
    &copy,

    &set_int,
    &get_int,
    &get_digit,
    &get_digit_count,
    &compare,
    &compare_d,
    &count_bits,
    &count_lsb_bits,
    &twoexpt,

    &read_radix,
    &write_radix,
    &unsigned_size,
    &unsigned_write,
    &unsigned_read,

    &add,
    &addi,
    &sub,
    &subi,
    &mul,
    &muli,
    &sqr,
    &divide,
    &div_2,
    &modi,
    &gcd,
    &lcm,

    &mulmod,
    &sqrmod,
    &invmod,

    &montgomery_setup,
    &montgomery_normalization,
    &montgomery_reduce,
    &montgomery_deinit,

    &exptmod,
    &isprime,

 #ifdef LTC_MECC
  #ifdef LTC_MECC_FP
    &ltc_ecc_fp_mulmod,
  #else
    &ltc_ecc_mulmod,
  #endif
    &ltc_ecc_projective_add_point,
    &ltc_ecc_projective_dbl_point,
    &ltc_ecc_map,
  #ifdef LTC_ECC_SHAMIR
   #ifdef LTC_MECC_FP
    &ltc_ecc_fp_mul2add,
   #else
    &ltc_ecc_mul2add,
   #endif /* LTC_MECC_FP */
  #else
    NULL,
  #endif /* LTC_ECC_SHAMIR */
 #else
    NULL,                         NULL,NULL, NULL, NULL,
 #endif /* LTC_MECC */

 #ifdef LTC_MRSA
    &rsa_make_key,
    &rsa_exptmod,
 #else
    NULL,                         NULL
 #endif
};

 #define mp_init(a)                           ltc_mp.init(a)
 #define mp_init_multi     ltc_init_multi
 #define mp_clear(a)                          ltc_mp.deinit(a)
 #define mp_clear_multi    ltc_deinit_multi
 #define mp_init_copy(a, b)                   ltc_mp.init_copy(a, b)

 #define mp_neg(a, b)                         ltc_mp.neg(a, b)
 #define mp_copy(a, b)                        ltc_mp.copy(a, b)

 #define mp_set(a, b)                         ltc_mp.set_int(a, b)
 #define mp_set_int(a, b)                     ltc_mp.set_int(a, b)
 #define mp_get_int(a)                        ltc_mp.get_int(a)
 #define mp_get_digit(a, n)                   ltc_mp.get_digit(a, n)
 #define mp_get_digit_count(a)                ltc_mp.get_digit_count(a)
 #define mp_cmp(a, b)                         ltc_mp.compare(a, b)
 #define mp_cmp_d(a, b)                       ltc_mp.compare_d(a, b)
 #define mp_count_bits(a)                     ltc_mp.count_bits(a)
 #define mp_cnt_lsb(a)                        ltc_mp.count_lsb_bits(a)
 #define mp_2expt(a, b)                       ltc_mp.twoexpt(a, b)

 #define mp_read_radix(a, b, c)               ltc_mp.read_radix(a, b, c)
 #define mp_toradix(a, b, c)                  ltc_mp.write_radix(a, b, c)
 #define mp_unsigned_bin_size(a)              ltc_mp.unsigned_size(a)
 #define mp_to_unsigned_bin(a, b)             ltc_mp.unsigned_write(a, b)
 #define mp_read_unsigned_bin(a, b, c)        ltc_mp.unsigned_read(a, b, c)

 #define mp_add(a, b, c)                      ltc_mp.add(a, b, c)
 #define mp_add_d(a, b, c)                    ltc_mp.addi(a, b, c)
 #define mp_sub(a, b, c)                      ltc_mp.sub(a, b, c)
 #define mp_sub_d(a, b, c)                    ltc_mp.subi(a, b, c)
 #define mp_mul(a, b, c)                      ltc_mp.mul(a, b, c)
 #define mp_mul_d(a, b, c)                    ltc_mp.muli(a, b, c)
 #define mp_sqr(a, b)                         ltc_mp.sqr(a, b)
 #define mp_div(a, b, c, d)                   ltc_mp.mpdiv(a, b, c, d)
 #define mp_div_2(a, b)                       ltc_mp.div_2(a, b)
 #define mp_mod(a, b, c)                      ltc_mp.mpdiv(a, b, NULL, c)
 #define mp_mod_d(a, b, c)                    ltc_mp.modi(a, b, c)
 #define mp_gcd(a, b, c)                      ltc_mp.gcd(a, b, c)
 #define mp_lcm(a, b, c)                      ltc_mp.lcm(a, b, c)

 #define mp_mulmod(a, b, c, d)                ltc_mp.mulmod(a, b, c, d)
 #define mp_sqrmod(a, b, c)                   ltc_mp.sqrmod(a, b, c)
 #define mp_invmod(a, b, c)                   ltc_mp.invmod(a, b, c)

 #define mp_montgomery_setup(a, b)            ltc_mp.montgomery_setup(a, b)
 #define mp_montgomery_normalization(a, b)    ltc_mp.montgomery_normalization(a, b)
 #define mp_montgomery_reduce(a, b, c)        ltc_mp.montgomery_reduce(a, b, c)
 #define mp_montgomery_free(a)                ltc_mp.montgomery_deinit(a)

 #define mp_exptmod(a, b, c, d)               ltc_mp.exptmod(a, b, c, d)
 #define mp_prime_is_prime(a, b, c)           ltc_mp.isprime(a, c)

 #define mp_iszero(a)                         (mp_cmp_d(a, 0) == LTC_MP_EQ ? LTC_MP_YES : LTC_MP_NO)
 #define mp_isodd(a)                          (mp_get_digit_count(a) > 0 ? (mp_get_digit(a, 0) & 1 ? LTC_MP_YES : LTC_MP_NO) : LTC_MP_NO)
 #define mp_exch(a, b)                        do { void *ABC__tmp = a; a = b; b = ABC__tmp; } while (0);

 #define mp_tohex(a, b)                       mp_toradix(a, b, 16)

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/math/ltm_desc.c,v $ */
/* $Revision: 1.31 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


#ifdef MPI
#include <stdarg.h>

int ltc_init_multi(void **a, ...) {
    void    **cur = a;
    int     np    = 0;
    va_list args;

    va_start(args, a);
    while (cur != NULL) {
        if (mp_init(cur) != CRYPT_OK) {
            /* failed */
            va_list clean_list;

            va_start(clean_list, a);
            cur = a;
            while (np--) {
                mp_clear(*cur);
                cur = va_arg(clean_list, void **);
            }
            va_end(clean_list);
            return CRYPT_MEM;
        }
        ++np;
        cur = va_arg(args, void **);
    }
    va_end(args);
    return CRYPT_OK;
}

void ltc_deinit_multi(void *a, ...) {
    void    *cur = a;
    va_list args;

    va_start(args, a);
    while (cur != NULL) {
        mp_clear(cur);
        cur = va_arg(args, void *);
    }
    va_end(args);
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/math/multi.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:23 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file pkcs_1_i2osp.c
   Integer to Octet I2OSP, Tom St Denis
 */

#ifdef LTC_PKCS_1

/* always stores the same # of bytes, pads with leading zero bytes
   as required
 */

/**
   LTC_PKCS #1 Integer to binary
   @param n             The integer to store
   @param modulus_len   The length of the RSA modulus
   @param out           [out] The destination for the integer
   @return CRYPT_OK if successful
 */
int pkcs_1_i2osp(void *n, unsigned long modulus_len, unsigned char *out) {
    unsigned long size;

    size = mp_unsigned_bin_size(n);

    if (size > modulus_len) {
        return CRYPT_BUFFER_OVERFLOW;
    }

    /* store it */
    zeromem(out, modulus_len);
    return mp_to_unsigned_bin(n, out + (modulus_len - size));
}
#endif /* LTC_PKCS_1 */


/* $Source: /cvs/libtom/libtomcrypt/src/pk/pkcs1/pkcs_1_i2osp.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file pkcs_1_mgf1.c
   The Mask Generation Function (MGF1) for LTC_PKCS #1, Tom St Denis
 */

#ifdef LTC_PKCS_1

/**
   Perform LTC_PKCS #1 MGF1 (internal)
   @param seed        The seed for MGF1
   @param seedlen     The length of the seed
   @param hash_idx    The index of the hash desired
   @param mask        [out] The destination
   @param masklen     The length of the mask desired
   @return CRYPT_OK if successful
 */
int pkcs_1_mgf1(int hash_idx,
                const unsigned char *seed, unsigned long seedlen,
                unsigned char *mask, unsigned long masklen) {
    unsigned long hLen, x;
    ulong32       counter;
    int           err;
    hash_state    *md;
    unsigned char *buf;

    LTC_ARGCHK(seed != NULL);
    LTC_ARGCHK(mask != NULL);

    /* ensure valid hash */
    if ((err = hash_is_valid(hash_idx)) != CRYPT_OK) {
        return err;
    }

    /* get hash output size */
    hLen = hash_descriptor[hash_idx].hashsize;

    /* allocate memory */
    md  = XMALLOC(sizeof(hash_state));
    buf = XMALLOC(hLen);
    if ((md == NULL) || (buf == NULL)) {
        if (md != NULL) {
            XFREE(md);
        }
        if (buf != NULL) {
            XFREE(buf);
        }
        return CRYPT_MEM;
    }

    /* start counter */
    counter = 0;

    while (masklen > 0) {
        /* handle counter */
        STORE32H(counter, buf);
        ++counter;

        /* get hash of seed || counter */
        if ((err = hash_descriptor[hash_idx].init(md)) != CRYPT_OK) {
            goto LBL_ERR;
        }
        if ((err = hash_descriptor[hash_idx].process(md, seed, seedlen)) != CRYPT_OK) {
            goto LBL_ERR;
        }
        if ((err = hash_descriptor[hash_idx].process(md, buf, 4)) != CRYPT_OK) {
            goto LBL_ERR;
        }
        if ((err = hash_descriptor[hash_idx].done(md, buf)) != CRYPT_OK) {
            goto LBL_ERR;
        }

        /* store it */
        for (x = 0; x < hLen && masklen > 0; x++, masklen--) {
            *mask++ = buf[x];
        }
    }

    err = CRYPT_OK;
LBL_ERR:
 #ifdef LTC_CLEAN_STACK
    zeromem(buf, hLen);
    zeromem(md, sizeof(hash_state));
 #endif

    XFREE(buf);
    XFREE(md);

    return err;
}
#endif /* LTC_PKCS_1 */

/* $Source: /cvs/libtom/libtomcrypt/src/pk/pkcs1/pkcs_1_mgf1.c,v $ */
/* $Revision: 1.8 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file pkcs_1_oaep_decode.c
   OAEP Padding for LTC_PKCS #1, Tom St Denis
 */

#ifdef LTC_PKCS_1

/**
   LTC_PKCS #1 v2.00 OAEP decode
   @param msg              The encoded data to decode
   @param msglen           The length of the encoded data (octets)
   @param lparam           The session or system data (can be NULL)
   @param lparamlen        The length of the lparam
   @param modulus_bitlen   The bit length of the RSA modulus
   @param hash_idx         The index of the hash desired
   @param out              [out] Destination of decoding
   @param outlen           [in/out] The max size and resulting size of the decoding
   @param res              [out] Result of decoding, 1==valid, 0==invalid
   @return CRYPT_OK if successful (even if invalid)
 */
int pkcs_1_oaep_decode(const unsigned char *msg, unsigned long msglen,
                       const unsigned char *lparam, unsigned long lparamlen,
                       unsigned long modulus_bitlen, int hash_idx,
                       unsigned char *out, unsigned long *outlen,
                       int *res) {
    unsigned char *DB, *seed, *mask;
    unsigned long hLen, x, y, modulus_len;
    int           err;

    LTC_ARGCHK(msg != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);
    LTC_ARGCHK(res != NULL);

    /* default to invalid packet */
    *res = 0;

    /* test valid hash */
    if ((err = hash_is_valid(hash_idx)) != CRYPT_OK) {
        return err;
    }
    hLen        = hash_descriptor[hash_idx].hashsize;
    modulus_len = (modulus_bitlen >> 3) + (modulus_bitlen & 7 ? 1 : 0);

    /* test hash/message size */
    if ((2 * hLen >= (modulus_len - 2)) || (msglen != modulus_len)) {
        return CRYPT_PK_INVALID_SIZE;
    }

    /* allocate ram for DB/mask/salt of size modulus_len */
    DB   = XMALLOC(modulus_len);
    mask = XMALLOC(modulus_len);
    seed = XMALLOC(hLen);
    if ((DB == NULL) || (mask == NULL) || (seed == NULL)) {
        if (DB != NULL) {
            XFREE(DB);
        }
        if (mask != NULL) {
            XFREE(mask);
        }
        if (seed != NULL) {
            XFREE(seed);
        }
        return CRYPT_MEM;
    }

    /* ok so it's now in the form

       0x00  || maskedseed || maskedDB

        1    ||   hLen     ||  modulus_len - hLen - 1

     */

    /* must have leading 0x00 byte */
    if (msg[0] != 0x00) {
        err = CRYPT_OK;
        goto LBL_ERR;
    }

    /* now read the masked seed */
    x = 1;
    XMEMCPY(seed, msg + x, hLen);
    x += hLen;

    /* now read the masked DB */
    XMEMCPY(DB, msg + x, modulus_len - hLen - 1);
    x += modulus_len - hLen - 1;

    /* compute MGF1 of maskedDB (hLen) */
    if ((err = pkcs_1_mgf1(hash_idx, DB, modulus_len - hLen - 1, mask, hLen)) != CRYPT_OK) {
        goto LBL_ERR;
    }

    /* XOR against seed */
    for (y = 0; y < hLen; y++) {
        seed[y] ^= mask[y];
    }

    /* compute MGF1 of seed (k - hlen - 1) */
    if ((err = pkcs_1_mgf1(hash_idx, seed, hLen, mask, modulus_len - hLen - 1)) != CRYPT_OK) {
        goto LBL_ERR;
    }

    /* xor against DB */
    for (y = 0; y < (modulus_len - hLen - 1); y++) {
        DB[y] ^= mask[y];
    }

    /* now DB == lhash || PS || 0x01 || M, PS == k - mlen - 2hlen - 2 zeroes */

    /* compute lhash and store it in seed [reuse temps!] */
    x = modulus_len;
    if (lparam != NULL) {
        if ((err = hash_memory(hash_idx, lparam, lparamlen, seed, &x)) != CRYPT_OK) {
            goto LBL_ERR;
        }
    } else {
        /* can't pass hash_memory a NULL so use DB with zero length */
        if ((err = hash_memory(hash_idx, DB, 0, seed, &x)) != CRYPT_OK) {
            goto LBL_ERR;
        }
    }

    /* compare the lhash'es */
    if (XMEMCMP(seed, DB, hLen) != 0) {
        err = CRYPT_OK;
        goto LBL_ERR;
    }

    /* now zeroes before a 0x01 */
    for (x = hLen; x < (modulus_len - hLen - 1) && DB[x] == 0x00; x++) {
        /* step... */
    }

    /* error out if wasn't 0x01 */
    if ((x == (modulus_len - hLen - 1)) || (DB[x] != 0x01)) {
        err = CRYPT_INVALID_PACKET;
        goto LBL_ERR;
    }

    /* rest is the message (and skip 0x01) */
    if ((modulus_len - hLen - 1 - ++x) > *outlen) {
        *outlen = modulus_len - hLen - 1 - x;
        err     = CRYPT_BUFFER_OVERFLOW;
        goto LBL_ERR;
    }

    /* copy message */
    *outlen = modulus_len - hLen - 1 - x;
    XMEMCPY(out, DB + x, modulus_len - hLen - 1 - x);
    x += modulus_len - hLen - 1;

    /* valid packet */
    *res = 1;

    err = CRYPT_OK;
LBL_ERR:
 #ifdef LTC_CLEAN_STACK
    zeromem(DB, modulus_len);
    zeromem(seed, hLen);
    zeromem(mask, modulus_len);
 #endif

    XFREE(seed);
    XFREE(mask);
    XFREE(DB);

    return err;
}
#endif /* LTC_PKCS_1 */

/* $Source: /cvs/libtom/libtomcrypt/src/pk/pkcs1/pkcs_1_oaep_decode.c,v $ */
/* $Revision: 1.13 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file pkcs_1_oaep_encode.c
   OAEP Padding for LTC_PKCS #1, Tom St Denis
 */

#ifdef LTC_PKCS_1

/**
   LTC_PKCS #1 v2.00 OAEP encode
   @param msg             The data to encode
   @param msglen          The length of the data to encode (octets)
   @param lparam          A session or system parameter (can be NULL)
   @param lparamlen       The length of the lparam data
   @param modulus_bitlen  The bit length of the RSA modulus
   @param prng            An active PRNG state
   @param prng_idx        The index of the PRNG desired
   @param hash_idx        The index of the hash desired
   @param out             [out] The destination for the encoded data
   @param outlen          [in/out] The max size and resulting size of the encoded data
   @return CRYPT_OK if successful
 */
int pkcs_1_oaep_encode(const unsigned char *msg, unsigned long msglen,
                       const unsigned char *lparam, unsigned long lparamlen,
                       unsigned long modulus_bitlen, prng_state *prng,
                       int prng_idx, int hash_idx,
                       unsigned char *out, unsigned long *outlen) {
    unsigned char *DB, *seed, *mask;
    unsigned long hLen, x, y, modulus_len;
    int           err;

    LTC_ARGCHK(msg != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* test valid hash */
    if ((err = hash_is_valid(hash_idx)) != CRYPT_OK) {
        return err;
    }

    /* valid prng */
    if ((err = prng_is_valid(prng_idx)) != CRYPT_OK) {
        return err;
    }

    hLen        = hash_descriptor[hash_idx].hashsize;
    modulus_len = (modulus_bitlen >> 3) + (modulus_bitlen & 7 ? 1 : 0);

    /* test message size */
    if ((2 * hLen >= (modulus_len - 2)) || (msglen > (modulus_len - 2 * hLen - 2))) {
        return CRYPT_PK_INVALID_SIZE;
    }

    /* allocate ram for DB/mask/salt of size modulus_len */
    DB   = XMALLOC(modulus_len);
    mask = XMALLOC(modulus_len);
    seed = XMALLOC(hLen);
    if ((DB == NULL) || (mask == NULL) || (seed == NULL)) {
        if (DB != NULL) {
            XFREE(DB);
        }
        if (mask != NULL) {
            XFREE(mask);
        }
        if (seed != NULL) {
            XFREE(seed);
        }
        return CRYPT_MEM;
    }

    /* get lhash */
    /* DB == lhash || PS || 0x01 || M, PS == k - mlen - 2hlen - 2 zeroes */
    x = modulus_len;
    if (lparam != NULL) {
        if ((err = hash_memory(hash_idx, lparam, lparamlen, DB, &x)) != CRYPT_OK) {
            goto LBL_ERR;
        }
    } else {
        /* can't pass hash_memory a NULL so use DB with zero length */
        if ((err = hash_memory(hash_idx, DB, 0, DB, &x)) != CRYPT_OK) {
            goto LBL_ERR;
        }
    }

    /* append PS then 0x01 (to lhash)  */
    x = hLen;
    y = modulus_len - msglen - 2 * hLen - 2;
    XMEMSET(DB + x, 0, y);
    x += y;

    /* 0x01 byte */
    DB[x++] = 0x01;

    /* message (length = msglen) */
    XMEMCPY(DB + x, msg, msglen);
    x += msglen;

    /* now choose a random seed */
    if (prng_descriptor[prng_idx].read(seed, hLen, prng) != hLen) {
        err = CRYPT_ERROR_READPRNG;
        goto LBL_ERR;
    }

    /* compute MGF1 of seed (k - hlen - 1) */
    if ((err = pkcs_1_mgf1(hash_idx, seed, hLen, mask, modulus_len - hLen - 1)) != CRYPT_OK) {
        goto LBL_ERR;
    }

    /* xor against DB */
    for (y = 0; y < (modulus_len - hLen - 1); y++) {
        DB[y] ^= mask[y];
    }

    /* compute MGF1 of maskedDB (hLen) */
    if ((err = pkcs_1_mgf1(hash_idx, DB, modulus_len - hLen - 1, mask, hLen)) != CRYPT_OK) {
        goto LBL_ERR;
    }

    /* XOR against seed */
    for (y = 0; y < hLen; y++) {
        seed[y] ^= mask[y];
    }

    /* create string of length modulus_len */
    if (*outlen < modulus_len) {
        *outlen = modulus_len;
        err     = CRYPT_BUFFER_OVERFLOW;
        goto LBL_ERR;
    }

    /* start output which is 0x00 || maskedSeed || maskedDB */
    x        = 0;
    out[x++] = 0x00;
    XMEMCPY(out + x, seed, hLen);
    x += hLen;
    XMEMCPY(out + x, DB, modulus_len - hLen - 1);
    x += modulus_len - hLen - 1;

    *outlen = x;

    err = CRYPT_OK;
LBL_ERR:
 #ifdef LTC_CLEAN_STACK
    zeromem(DB, modulus_len);
    zeromem(seed, hLen);
    zeromem(mask, modulus_len);
 #endif

    XFREE(seed);
    XFREE(mask);
    XFREE(DB);

    return err;
}
#endif /* LTC_PKCS_1 */


/* $Source: /cvs/libtom/libtomcrypt/src/pk/pkcs1/pkcs_1_oaep_encode.c,v $ */
/* $Revision: 1.9 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file pkcs_1_os2ip.c
   Octet to Integer OS2IP, Tom St Denis
 */
#ifdef LTC_PKCS_1

/**
   Read a binary string into an mp_int
   @param n          [out] The mp_int destination
   @param in         The binary string to read
   @param inlen      The length of the binary string
   @return CRYPT_OK if successful
 */
int pkcs_1_os2ip(void *n, unsigned char *in, unsigned long inlen) {
    return mp_read_unsigned_bin(n, in, inlen);
}
#endif /* LTC_PKCS_1 */


/* $Source: /cvs/libtom/libtomcrypt/src/pk/pkcs1/pkcs_1_os2ip.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file pkcs_1_pss_decode.c
   LTC_PKCS #1 PSS Signature Padding, Tom St Denis
 */

#ifdef LTC_PKCS_1

/**
   LTC_PKCS #1 v2.00 PSS decode
   @param  msghash         The hash to verify
   @param  msghashlen      The length of the hash (octets)
   @param  sig             The signature data (encoded data)
   @param  siglen          The length of the signature data (octets)
   @param  saltlen         The length of the salt used (octets)
   @param  hash_idx        The index of the hash desired
   @param  modulus_bitlen  The bit length of the RSA modulus
   @param  res             [out] The result of the comparison, 1==valid, 0==invalid
   @return CRYPT_OK if successful (even if the comparison failed)
 */
int pkcs_1_pss_decode(const unsigned char *msghash, unsigned long msghashlen,
                      const unsigned char *sig, unsigned long siglen,
                      unsigned long saltlen, int hash_idx,
                      unsigned long modulus_bitlen, int *res) {
    unsigned char *DB, *mask, *salt, *hash;
    unsigned long x, y, hLen, modulus_len;
    int           err;
    hash_state    md;

    LTC_ARGCHK(msghash != NULL);
    LTC_ARGCHK(res != NULL);

    /* default to invalid */
    *res = 0;

    /* ensure hash is valid */
    if ((err = hash_is_valid(hash_idx)) != CRYPT_OK) {
        return err;
    }

    hLen        = hash_descriptor[hash_idx].hashsize;
    modulus_len = (modulus_bitlen >> 3) + (modulus_bitlen & 7 ? 1 : 0);

    /* check sizes */
    if ((saltlen > modulus_len) ||
        (modulus_len < hLen + saltlen + 2) || (siglen != modulus_len)) {
        return CRYPT_PK_INVALID_SIZE;
    }

    /* allocate ram for DB/mask/salt/hash of size modulus_len */
    DB   = XMALLOC(modulus_len);
    mask = XMALLOC(modulus_len);
    salt = XMALLOC(modulus_len);
    hash = XMALLOC(modulus_len);
    if ((DB == NULL) || (mask == NULL) || (salt == NULL) || (hash == NULL)) {
        if (DB != NULL) {
            XFREE(DB);
        }
        if (mask != NULL) {
            XFREE(mask);
        }
        if (salt != NULL) {
            XFREE(salt);
        }
        if (hash != NULL) {
            XFREE(hash);
        }
        return CRYPT_MEM;
    }

    /* ensure the 0xBC byte */
    if (sig[siglen - 1] != 0xBC) {
        err = CRYPT_INVALID_PACKET;
        goto LBL_ERR;
    }

    /* copy out the DB */
    x = 0;
    XMEMCPY(DB, sig + x, modulus_len - hLen - 1);
    x += modulus_len - hLen - 1;

    /* copy out the hash */
    XMEMCPY(hash, sig + x, hLen);
    x += hLen;

    /* check the MSB */
    if ((sig[0] & ~(0xFF >> ((modulus_len << 3) - (modulus_bitlen - 1)))) != 0) {
        err = CRYPT_INVALID_PACKET;
        goto LBL_ERR;
    }

    /* generate mask of length modulus_len - hLen - 1 from hash */
    if ((err = pkcs_1_mgf1(hash_idx, hash, hLen, mask, modulus_len - hLen - 1)) != CRYPT_OK) {
        goto LBL_ERR;
    }

    /* xor against DB */
    for (y = 0; y < (modulus_len - hLen - 1); y++) {
        DB[y] ^= mask[y];
    }

    /* now clear the first byte [make sure smaller than modulus] */
    DB[0] &= 0xFF >> ((modulus_len << 3) - (modulus_bitlen - 1));

    /* DB = PS || 0x01 || salt, PS == modulus_len - saltlen - hLen - 2 zero bytes */

    /* check for zeroes and 0x01 */
    for (x = 0; x < modulus_len - saltlen - hLen - 2; x++) {
        if (DB[x] != 0x00) {
            err = CRYPT_INVALID_PACKET;
            goto LBL_ERR;
        }
    }

    /* check for the 0x01 */
    if (DB[x++] != 0x01) {
        err = CRYPT_INVALID_PACKET;
        goto LBL_ERR;
    }

    /* M = (eight) 0x00 || msghash || salt, mask = H(M) */
    if ((err = hash_descriptor[hash_idx].init(&md)) != CRYPT_OK) {
        goto LBL_ERR;
    }
    zeromem(mask, 8);
    if ((err = hash_descriptor[hash_idx].process(&md, mask, 8)) != CRYPT_OK) {
        goto LBL_ERR;
    }
    if ((err = hash_descriptor[hash_idx].process(&md, msghash, msghashlen)) != CRYPT_OK) {
        goto LBL_ERR;
    }
    if ((err = hash_descriptor[hash_idx].process(&md, DB + x, saltlen)) != CRYPT_OK) {
        goto LBL_ERR;
    }
    if ((err = hash_descriptor[hash_idx].done(&md, mask)) != CRYPT_OK) {
        goto LBL_ERR;
    }

    /* mask == hash means valid signature */
    if (XMEMCMP(mask, hash, hLen) == 0) {
        *res = 1;
    }

    err = CRYPT_OK;
LBL_ERR:
 #ifdef LTC_CLEAN_STACK
    zeromem(DB, modulus_len);
    zeromem(mask, modulus_len);
    zeromem(salt, modulus_len);
    zeromem(hash, modulus_len);
 #endif

    XFREE(hash);
    XFREE(salt);
    XFREE(mask);
    XFREE(DB);

    return err;
}
#endif /* LTC_PKCS_1 */

/* $Source: /cvs/libtom/libtomcrypt/src/pk/pkcs1/pkcs_1_pss_decode.c,v $ */
/* $Revision: 1.11 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file pkcs_1_pss_encode.c
   LTC_PKCS #1 PSS Signature Padding, Tom St Denis
 */

#ifdef LTC_PKCS_1

/**
   LTC_PKCS #1 v2.00 Signature Encoding
   @param msghash          The hash to encode
   @param msghashlen       The length of the hash (octets)
   @param saltlen          The length of the salt desired (octets)
   @param prng             An active PRNG context
   @param prng_idx         The index of the PRNG desired
   @param hash_idx         The index of the hash desired
   @param modulus_bitlen   The bit length of the RSA modulus
   @param out              [out] The destination of the encoding
   @param outlen           [in/out] The max size and resulting size of the encoded data
   @return CRYPT_OK if successful
 */
int pkcs_1_pss_encode(const unsigned char *msghash, unsigned long msghashlen,
                      unsigned long saltlen, prng_state *prng,
                      int prng_idx, int hash_idx,
                      unsigned long modulus_bitlen,
                      unsigned char *out, unsigned long *outlen) {
    unsigned char *DB, *mask, *salt, *hash;
    unsigned long x, y, hLen, modulus_len;
    int           err;
    hash_state    md;

    LTC_ARGCHK(msghash != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* ensure hash and PRNG are valid */
    if ((err = hash_is_valid(hash_idx)) != CRYPT_OK) {
        return err;
    }
    if ((err = prng_is_valid(prng_idx)) != CRYPT_OK) {
        return err;
    }

    hLen        = hash_descriptor[hash_idx].hashsize;
    modulus_len = (modulus_bitlen >> 3) + (modulus_bitlen & 7 ? 1 : 0);

    /* check sizes */
    if ((saltlen > modulus_len) || (modulus_len < hLen + saltlen + 2)) {
        return CRYPT_PK_INVALID_SIZE;
    }

    /* allocate ram for DB/mask/salt/hash of size modulus_len */
    DB   = XMALLOC(modulus_len);
    mask = XMALLOC(modulus_len);
    salt = XMALLOC(modulus_len);
    hash = XMALLOC(modulus_len);
    if ((DB == NULL) || (mask == NULL) || (salt == NULL) || (hash == NULL)) {
        if (DB != NULL) {
            XFREE(DB);
        }
        if (mask != NULL) {
            XFREE(mask);
        }
        if (salt != NULL) {
            XFREE(salt);
        }
        if (hash != NULL) {
            XFREE(hash);
        }
        return CRYPT_MEM;
    }


    /* generate random salt */
    if (saltlen > 0) {
        if (prng_descriptor[prng_idx].read(salt, saltlen, prng) != saltlen) {
            err = CRYPT_ERROR_READPRNG;
            goto LBL_ERR;
        }
    }

    /* M = (eight) 0x00 || msghash || salt, hash = H(M) */
    if ((err = hash_descriptor[hash_idx].init(&md)) != CRYPT_OK) {
        goto LBL_ERR;
    }
    zeromem(DB, 8);
    if ((err = hash_descriptor[hash_idx].process(&md, DB, 8)) != CRYPT_OK) {
        goto LBL_ERR;
    }
    if ((err = hash_descriptor[hash_idx].process(&md, msghash, msghashlen)) != CRYPT_OK) {
        goto LBL_ERR;
    }
    if ((err = hash_descriptor[hash_idx].process(&md, salt, saltlen)) != CRYPT_OK) {
        goto LBL_ERR;
    }
    if ((err = hash_descriptor[hash_idx].done(&md, hash)) != CRYPT_OK) {
        goto LBL_ERR;
    }

    /* generate DB = PS || 0x01 || salt, PS == modulus_len - saltlen - hLen - 2 zero bytes */
    x = 0;
    XMEMSET(DB + x, 0, modulus_len - saltlen - hLen - 2);
    x      += modulus_len - saltlen - hLen - 2;
    DB[x++] = 0x01;
    XMEMCPY(DB + x, salt, saltlen);
    x += saltlen;

    /* generate mask of length modulus_len - hLen - 1 from hash */
    if ((err = pkcs_1_mgf1(hash_idx, hash, hLen, mask, modulus_len - hLen - 1)) != CRYPT_OK) {
        goto LBL_ERR;
    }

    /* xor against DB */
    for (y = 0; y < (modulus_len - hLen - 1); y++) {
        DB[y] ^= mask[y];
    }

    /* output is DB || hash || 0xBC */
    if (*outlen < modulus_len) {
        *outlen = modulus_len;
        err     = CRYPT_BUFFER_OVERFLOW;
        goto LBL_ERR;
    }

    /* DB len = modulus_len - hLen - 1 */
    y = 0;
    XMEMCPY(out + y, DB, modulus_len - hLen - 1);
    y += modulus_len - hLen - 1;

    /* hash */
    XMEMCPY(out + y, hash, hLen);
    y += hLen;

    /* 0xBC */
    out[y] = 0xBC;

    /* now clear the 8*modulus_len - modulus_bitlen most significant bits */
    out[0] &= 0xFF >> ((modulus_len << 3) - (modulus_bitlen - 1));

    /* store output size */
    *outlen = modulus_len;
    err     = CRYPT_OK;
LBL_ERR:
 #ifdef LTC_CLEAN_STACK
    zeromem(DB, modulus_len);
    zeromem(mask, modulus_len);
    zeromem(salt, modulus_len);
    zeromem(hash, modulus_len);
 #endif

    XFREE(hash);
    XFREE(salt);
    XFREE(mask);
    XFREE(DB);

    return err;
}
#endif /* LTC_PKCS_1 */

/* $Source: /cvs/libtom/libtomcrypt/src/pk/pkcs1/pkcs_1_pss_encode.c,v $ */
/* $Revision: 1.9 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/** @file pkcs_1_v1_5_decode.c
 *
 *  LTC_PKCS #1 v1.5 Padding. (Andreas Lange)
 */

#ifdef LTC_PKCS_1

/** @brief LTC_PKCS #1 v1.5 decode.
 *
 *  @param msg              The encoded data to decode
 *  @param msglen           The length of the encoded data (octets)
 *  @param block_type       Block type to use in padding (\sa ltc_pkcs_1_v1_5_blocks)
 *  @param modulus_bitlen   The bit length of the RSA modulus
 *  @param out              [out] Destination of decoding
 *  @param outlen           [in/out] The max size and resulting size of the decoding
 *  @param is_valid         [out] Boolean whether the padding was valid
 *
 *  @return CRYPT_OK if successful (even if invalid)
 */
int pkcs_1_v1_5_decode(const unsigned char *msg,
                       unsigned long       msglen,
                       int                 block_type,
                       unsigned long       modulus_bitlen,
                       unsigned char       *out,
                       unsigned long       *outlen,
                       int                 *is_valid) {
    unsigned long modulus_len, ps_len, i;
    int           result;

    /* default to invalid packet */
    *is_valid = 0;

    modulus_len = (modulus_bitlen >> 3) + (modulus_bitlen & 7 ? 1 : 0);

    /* test message size */

    if ((msglen > modulus_len) || (modulus_len < 11)) {
        return CRYPT_PK_INVALID_SIZE;
    }

    /* separate encoded message */

    if ((msg[0] != 0x00) || (msg[1] != (unsigned char)block_type)) {
        result = CRYPT_INVALID_PACKET;
        goto bail;
    }

    if (block_type == LTC_LTC_PKCS_1_EME) {
        for (i = 2; i < modulus_len; i++) {
            /* separator */
            if (msg[i] == 0x00) {
                break;
            }
        }
        ps_len = i++ - 2;

        if ((i >= modulus_len) || (ps_len < 8)) {
            /* There was no octet with hexadecimal value 0x00 to separate ps from m,
             * or the length of ps is less than 8 octets.
             */
            result = CRYPT_INVALID_PACKET;
            goto bail;
        }
    } else {
        for (i = 2; i < modulus_len - 1; i++) {
            if (msg[i] != 0xFF) {
                break;
            }
        }

        /* separator check */
        if (msg[i] != 0) {
            /* There was no octet with hexadecimal value 0x00 to separate ps from m. */
            result = CRYPT_INVALID_PACKET;
            goto bail;
        }

        ps_len = i - 2;
    }

    if (*outlen < (msglen - (2 + ps_len + 1))) {
        *outlen = msglen - (2 + ps_len + 1);
        result  = CRYPT_BUFFER_OVERFLOW;
        goto bail;
    }

    *outlen = (msglen - (2 + ps_len + 1));
    XMEMCPY(out, &msg[2 + ps_len + 1], *outlen);

    /* valid packet */
    *is_valid = 1;
    result    = CRYPT_OK;
bail:
    return result;
} /* pkcs_1_v1_5_decode */
#endif /* #ifdef LTC_PKCS_1 */

/* $Source: /cvs/libtom/libtomcrypt/src/pk/pkcs1/pkcs_1_v1_5_decode.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/*! \file pkcs_1_v1_5_encode.c
 *
 *  LTC_PKCS #1 v1.5 Padding (Andreas Lange)
 */

#ifdef LTC_PKCS_1

/*! \brief LTC_PKCS #1 v1.5 encode.
 *
 *  \param msg              The data to encode
 *  \param msglen           The length of the data to encode (octets)
 *  \param block_type       Block type to use in padding (\sa ltc_pkcs_1_v1_5_blocks)
 *  \param modulus_bitlen   The bit length of the RSA modulus
 *  \param prng             An active PRNG state (only for LTC_LTC_PKCS_1_EME)
 *  \param prng_idx         The index of the PRNG desired (only for LTC_LTC_PKCS_1_EME)
 *  \param out              [out] The destination for the encoded data
 *  \param outlen           [in/out] The max size and resulting size of the encoded data
 *
 *  \return CRYPT_OK if successful
 */
int pkcs_1_v1_5_encode(const unsigned char *msg,
                       unsigned long       msglen,
                       int                 block_type,
                       unsigned long       modulus_bitlen,
                       prng_state          *prng,
                       int                 prng_idx,
                       unsigned char       *out,
                       unsigned long       *outlen) {
    unsigned long modulus_len, ps_len, i;
    unsigned char *ps;
    int           result;

    /* valid block_type? */
    if ((block_type != LTC_LTC_PKCS_1_EMSA) &&
        (block_type != LTC_LTC_PKCS_1_EME)) {
        return CRYPT_PK_INVALID_PADDING;
    }

    if (block_type == LTC_LTC_PKCS_1_EME) {  /* encryption padding, we need a valid PRNG */
        if ((result = prng_is_valid(prng_idx)) != CRYPT_OK) {
            return result;
        }
    }

    modulus_len = (modulus_bitlen >> 3) + (modulus_bitlen & 7 ? 1 : 0);

    /* test message size */
    if ((msglen + 11) > modulus_len) {
        return CRYPT_PK_INVALID_SIZE;
    }

    if (*outlen < modulus_len) {
        *outlen = modulus_len;
        result  = CRYPT_BUFFER_OVERFLOW;
        goto bail;
    }

    /* generate an octets string PS */
    ps     = &out[2];
    ps_len = modulus_len - msglen - 3;

    if (block_type == LTC_LTC_PKCS_1_EME) {
        /* now choose a random ps */
        if (prng_descriptor[prng_idx].read(ps, ps_len, prng) != ps_len) {
            result = CRYPT_ERROR_READPRNG;
            goto bail;
        }

        /* transform zero bytes (if any) to non-zero random bytes */
        for (i = 0; i < ps_len; i++) {
            while (ps[i] == 0) {
                if (prng_descriptor[prng_idx].read(&ps[i], 1, prng) != 1) {
                    result = CRYPT_ERROR_READPRNG;
                    goto bail;
                }
            }
        }
    } else {
        XMEMSET(ps, 0xFF, ps_len);
    }

    /* create string of length modulus_len */
    out[0]          = 0x00;
    out[1]          = (unsigned char)block_type;/* block_type 1 or 2 */
    out[2 + ps_len] = 0x00;
    XMEMCPY(&out[2 + ps_len + 1], msg, msglen);
    *outlen = modulus_len;

    result = CRYPT_OK;
bail:
    return result;
} /* pkcs_1_v1_5_encode */
#endif /* #ifdef LTC_PKCS_1 */

/* $Source: /cvs/libtom/libtomcrypt/src/pk/pkcs1/pkcs_1_v1_5_encode.c,v $ */
/* $Revision: 1.4 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file rand_prime.c
   Generate a random prime, Tom St Denis
 */

#define USE_BBS    1

int rand_prime(void *N, long len, prng_state *prng, int wprng) {
    int           err, res, type;
    unsigned char *buf;

    LTC_ARGCHK(N != NULL);

    /* get type */
    if (len < 0) {
        type = USE_BBS;
        len  = -len;
    } else {
        type = 0;
    }

    /* allow sizes between 2 and 512 bytes for a prime size */
    if ((len < 2) || (len > 512)) {
        return CRYPT_INVALID_PRIME_SIZE;
    }

    /* valid PRNG? Better be! */
    if ((err = prng_is_valid(wprng)) != CRYPT_OK) {
        return err;
    }

    /* allocate buffer to work with */
    buf = XCALLOC(1, len);
    if (buf == NULL) {
        return CRYPT_MEM;
    }

    do {
        /* generate value */
        if (prng_descriptor[wprng].read(buf, len, prng) != (unsigned long)len) {
            XFREE(buf);
            return CRYPT_ERROR_READPRNG;
        }

        /* munge bits */
        buf[0]       |= 0x80 | 0x40;
        buf[len - 1] |= 0x01 | ((type & USE_BBS) ? 0x02 : 0x00);

        /* load value */
        if ((err = mp_read_unsigned_bin(N, buf, len)) != CRYPT_OK) {
            XFREE(buf);
            return err;
        }

        /* test */
        if ((err = mp_prime_is_prime(N, 8, &res)) != CRYPT_OK) {
            XFREE(buf);
            return err;
        }
    } while (res == LTC_MP_NO);

#ifdef LTC_CLEAN_STACK
    zeromem(buf, len);
#endif

    XFREE(buf);
    return CRYPT_OK;
}

/* $Source: /cvs/libtom/libtomcrypt/src/math/rand_prime.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:23 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file rng_get_bytes.c
   portable way to get secure random bits to feed a PRNG (Tom St Denis)
 */

#ifdef LTC_DEVRANDOM
/* on *NIX read /dev/random */
static unsigned long rng_nix(unsigned char *buf, unsigned long len,
                             void (*callback)(void)) {
 #ifdef LTC_NO_FILE
    return 0;
 #else
    FILE          *f;
    unsigned long x;
  #ifdef TRY_URANDOM_FIRST
    f = fopen("/dev/urandom", "rb");
    if (f == NULL)
  #endif /* TRY_URANDOM_FIRST */
    f = fopen("/dev/random", "rb");

    if (f == NULL) {
        return 0;
    }

    /* disable buffering */
    if (setvbuf(f, NULL, _IONBF, 0) != 0) {
        fclose(f);
        return 0;
    }

    x = (unsigned long)fread(buf, 1, (size_t)len, f);
    fclose(f);
    return x;
 #endif /* LTC_NO_FILE */
}
#endif  /* LTC_DEVRANDOM */

/* on ANSI C platforms with 100 < CLOCKS_PER_SEC < 10000 */
#if defined(CLOCKS_PER_SEC) && !defined(WINCE)

 #define ANSI_RNG

static unsigned long rng_ansic(unsigned char *buf, unsigned long len,
                               void (*callback)(void)) {
    clock_t t1;
    int     l, acc, bits, a, b;

    if ((XCLOCKS_PER_SEC < 100) || (XCLOCKS_PER_SEC > 10000)) {
        return 0;
    }

    l    = len;
    bits = 8;
    acc  = a = b = 0;
    while (len--) {
        if (callback != NULL) callback();
        while (bits--) {
            do {
                t1 = XCLOCK();
                while (t1 == XCLOCK()) a ^= 1;
                t1 = XCLOCK();
                while (t1 == XCLOCK()) b ^= 1;
            } while (a == b);
            acc = (acc << 1) | a;
        }
        *buf++ = acc;
        acc    = 0;
        bits   = 8;
    }
    acc = bits = a = b = 0;
    return l;
}
#endif

/* Try the Microsoft CSP */
#if defined(WIN32) || defined(WINCE)
 #define _WIN32_WINNT    0x0400
 #ifdef WINCE
  #define UNDER_CE
  #define ARM
 #endif
#include <windows.h>
#include <wincrypt.h>

static unsigned long rng_win32(unsigned char *buf, unsigned long len,
                               void (*callback)(void)) {
    HCRYPTPROV hProv = 0;

    if (!CryptAcquireContext(&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL,
                             (CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET)) &&
        !CryptAcquireContext(&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL,
                             CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET | CRYPT_NEWKEYSET))
        return 0;

    if (CryptGenRandom(hProv, len, buf) == TRUE) {
        CryptReleaseContext(hProv, 0);
        return len;
    } else {
        CryptReleaseContext(hProv, 0);
        return 0;
    }
}
#endif /* WIN32 */

/**
   Read the system RNG
   @param out       Destination
   @param outlen    Length desired (octets)
   @param callback  Pointer to void function to act as "callback" when RNG is slow.  This can be NULL
   @return Number of octets read
 */
unsigned long rng_get_bytes(unsigned char *out, unsigned long outlen,
                            void (*callback)(void)) {
    unsigned long x;

    LTC_ARGCHK(out != NULL);

#if defined(LTC_DEVRANDOM)
    x = rng_nix(out, outlen, callback);
    if (x != 0) {
        return x;
    }
#endif
#ifdef WIN32
    x = rng_win32(out, outlen, callback);
    if (x != 0) {
        return x;
    }
#endif
#ifdef ANSI_RNG
    x = rng_ansic(out, outlen, callback);
    if (x != 0) {
        return x;
    }
#endif
    return 0;
}

/* $Source: /cvs/libtom/libtomcrypt/src/prngs/rng_get_bytes.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file rng_make_prng.c
   portable way to get secure random bits to feed a PRNG  (Tom St Denis)
 */

/**
   Create a PRNG from a RNG
   @param bits     Number of bits of entropy desired (64 ... 1024)
   @param wprng    Index of which PRNG to setup
   @param prng     [out] PRNG state to initialize
   @param callback A pointer to a void function for when the RNG is slow, this can be NULL
   @return CRYPT_OK if successful
 */
int rng_make_prng(int bits, int wprng, prng_state *prng,
                  void (*callback)(void)) {
    unsigned char buf[256];
    int           err;

    LTC_ARGCHK(prng != NULL);

    /* check parameter */
    if ((err = prng_is_valid(wprng)) != CRYPT_OK) {
        return err;
    }

    if ((bits < 64) || (bits > 1024)) {
        return CRYPT_INVALID_PRNGSIZE;
    }

    if ((err = prng_descriptor[wprng].start(prng)) != CRYPT_OK) {
        return err;
    }

    bits = ((bits / 8) + ((bits & 7) != 0 ? 1 : 0)) * 2;
    if (rng_get_bytes(buf, (unsigned long)bits, callback) != (unsigned long)bits) {
        return CRYPT_ERROR_READPRNG;
    }

    if ((err = prng_descriptor[wprng].add_entropy(buf, (unsigned long)bits, prng)) != CRYPT_OK) {
        return err;
    }

    if ((err = prng_descriptor[wprng].ready(prng)) != CRYPT_OK) {
        return err;
    }

#ifdef LTC_CLEAN_STACK
    zeromem(buf, sizeof(buf));
#endif
    return CRYPT_OK;
}

/* $Source: /cvs/libtom/libtomcrypt/src/prngs/rng_make_prng.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file rsa_decrypt_key.c
   RSA LTC_PKCS #1 Decryption, Tom St Denis and Andreas Lange
 */

#ifdef LTC_MRSA

/**
   LTC_PKCS #1 decrypt then v1.5 or OAEP depad
   @param in          The ciphertext
   @param inlen       The length of the ciphertext (octets)
   @param out         [out] The plaintext
   @param outlen      [in/out] The max size and resulting size of the plaintext (octets)
   @param lparam      The system "lparam" value
   @param lparamlen   The length of the lparam value (octets)
   @param hash_idx    The index of the hash desired
   @param padding     Type of padding (LTC_LTC_PKCS_1_OAEP or LTC_LTC_PKCS_1_V1_5)
   @param stat        [out] Result of the decryption, 1==valid, 0==invalid
   @param key         The corresponding private RSA key
   @return CRYPT_OK if succcessul (even if invalid)
 */
int rsa_decrypt_key_ex(const unsigned char *in, unsigned long inlen,
                       unsigned char *out, unsigned long *outlen,
                       const unsigned char *lparam, unsigned long lparamlen,
                       int hash_idx, int padding,
                       int *stat, rsa_key *key) {
    unsigned long modulus_bitlen, modulus_bytelen, x;
    int           err;
    unsigned char *tmp;

    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);
    LTC_ARGCHK(key != NULL);
    LTC_ARGCHK(stat != NULL);

    /* default to invalid */
    *stat = 0;

    /* valid padding? */

    if ((padding != LTC_LTC_PKCS_1_V1_5) &&
        (padding != LTC_LTC_PKCS_1_OAEP)) {
        return CRYPT_PK_INVALID_PADDING;
    }

    if (padding == LTC_LTC_PKCS_1_OAEP) {
        /* valid hash ? */
        if ((err = hash_is_valid(hash_idx)) != CRYPT_OK) {
            return err;
        }
    }

    /* get modulus len in bits */
    modulus_bitlen = mp_count_bits((key->N));

    /* outlen must be at least the size of the modulus */
    modulus_bytelen = mp_unsigned_bin_size((key->N));
    if (modulus_bytelen != inlen) {
        return CRYPT_INVALID_PACKET;
    }

    /* allocate ram */
    tmp = XMALLOC(inlen);
    if (tmp == NULL) {
        return CRYPT_MEM;
    }

    /* rsa decode the packet */
    x = inlen;
    if ((err = ltc_mp.rsa_me(in, inlen, tmp, &x, PK_PRIVATE, key)) != CRYPT_OK) {
        XFREE(tmp);
        return err;
    }

    if (padding == LTC_LTC_PKCS_1_OAEP) {
        /* now OAEP decode the packet */
        err = pkcs_1_oaep_decode(tmp, x, lparam, lparamlen, modulus_bitlen, hash_idx,
                                 out, outlen, stat);
    } else {
        /* now LTC_PKCS #1 v1.5 depad the packet */
        err = pkcs_1_v1_5_decode(tmp, x, LTC_LTC_PKCS_1_EME, modulus_bitlen, out, outlen, stat);
    }

    XFREE(tmp);
    return err;
}
#endif /* LTC_MRSA */

/* $Source: /cvs/libtom/libtomcrypt/src/pk/rsa/rsa_decrypt_key.c,v $ */
/* $Revision: 1.10 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file rsa_encrypt_key.c
   RSA LTC_PKCS #1 encryption, Tom St Denis and Andreas Lange
 */

#ifdef LTC_MRSA

/**
    (LTC_PKCS #1 v2.0) OAEP pad then encrypt
    @param in          The plaintext
    @param inlen       The length of the plaintext (octets)
    @param out         [out] The ciphertext
    @param outlen      [in/out] The max size and resulting size of the ciphertext
    @param lparam      The system "lparam" for the encryption
    @param lparamlen   The length of lparam (octets)
    @param prng        An active PRNG
    @param prng_idx    The index of the desired prng
    @param hash_idx    The index of the desired hash
    @param padding     Type of padding (LTC_LTC_PKCS_1_OAEP or LTC_LTC_PKCS_1_V1_5)
    @param key         The RSA key to encrypt to
    @return CRYPT_OK if successful
 */
int rsa_encrypt_key_ex(const unsigned char *in, unsigned long inlen,
                       unsigned char *out, unsigned long *outlen,
                       const unsigned char *lparam, unsigned long lparamlen,
                       prng_state *prng, int prng_idx, int hash_idx, int padding, rsa_key *key) {
    unsigned long modulus_bitlen, modulus_bytelen, x;
    int           err;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);
    LTC_ARGCHK(key != NULL);

    /* valid padding? */
    if ((padding != LTC_LTC_PKCS_1_V1_5) &&
        (padding != LTC_LTC_PKCS_1_OAEP)) {
        return CRYPT_PK_INVALID_PADDING;
    }

    /* valid prng? */
    if ((err = prng_is_valid(prng_idx)) != CRYPT_OK) {
        return err;
    }

    if (padding == LTC_LTC_PKCS_1_OAEP) {
        /* valid hash? */
        if ((err = hash_is_valid(hash_idx)) != CRYPT_OK) {
            return err;
        }
    }

    /* get modulus len in bits */
    modulus_bitlen = mp_count_bits((key->N));

    /* outlen must be at least the size of the modulus */
    modulus_bytelen = mp_unsigned_bin_size((key->N));
    if (modulus_bytelen > *outlen) {
        *outlen = modulus_bytelen;
        return CRYPT_BUFFER_OVERFLOW;
    }

    if (padding == LTC_LTC_PKCS_1_OAEP) {
        /* OAEP pad the key */
        x = *outlen;
        if ((err = pkcs_1_oaep_encode(in, inlen, lparam,
                                      lparamlen, modulus_bitlen, prng, prng_idx, hash_idx,
                                      out, &x)) != CRYPT_OK) {
            return err;
        }
    } else {
        /* LTC_PKCS #1 v1.5 pad the key */
        x = *outlen;
        if ((err = pkcs_1_v1_5_encode(in, inlen, LTC_LTC_PKCS_1_EME,
                                      modulus_bitlen, prng, prng_idx,
                                      out, &x)) != CRYPT_OK) {
            return err;
        }
    }

    /* rsa exptmod the OAEP or LTC_PKCS #1 v1.5 pad */
    return ltc_mp.rsa_me(out, x, out, outlen, PK_PUBLIC, key);
}
#endif /* LTC_MRSA */

/* $Source: /cvs/libtom/libtomcrypt/src/pk/rsa/rsa_encrypt_key.c,v $ */
/* $Revision: 1.10 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file rsa_exptmod.c
   RSA LTC_PKCS exptmod, Tom St Denis
 */

#ifdef LTC_MRSA

/**
   Compute an RSA modular exponentiation
   @param in         The input data to send into RSA
   @param inlen      The length of the input (octets)
   @param out        [out] The destination
   @param outlen     [in/out] The max size and resulting size of the output
   @param which      Which exponent to use, e.g. PK_PRIVATE or PK_PUBLIC
   @param key        The RSA key to use
   @return CRYPT_OK if successful
 */
int rsa_exptmod(const unsigned char *in, unsigned long inlen,
                unsigned char *out, unsigned long *outlen, int which,
                rsa_key *key) {
    void          *tmp, *tmpa, *tmpb;
    unsigned long x;
    int           err;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);
    LTC_ARGCHK(key != NULL);

    /* is the key of the right type for the operation? */
    if ((which == PK_PRIVATE) && (key->type != PK_PRIVATE)) {
        return CRYPT_PK_NOT_PRIVATE;
    }

    /* must be a private or public operation */
    if ((which != PK_PRIVATE) && (which != PK_PUBLIC)) {
        return CRYPT_PK_INVALID_TYPE;
    }

    /* init and copy into tmp */
    if ((err = mp_init_multi(&tmp, &tmpa, &tmpb, NULL)) != CRYPT_OK) {
        return err;
    }
    if ((err = mp_read_unsigned_bin(tmp, (unsigned char *)in, (int)inlen)) != CRYPT_OK) {
        goto error;
    }

    /* sanity check on the input */
    if (mp_cmp(key->N, tmp) == LTC_MP_LT) {
        err = CRYPT_PK_INVALID_SIZE;
        goto error;
    }

    /* are we using the private exponent and is the key optimized? */
    if (which == PK_PRIVATE) {
        /* tmpa = tmp^dP mod p */
        if ((err = mp_exptmod(tmp, key->dP, key->p, tmpa)) != CRYPT_OK) {
            goto error;
        }

        /* tmpb = tmp^dQ mod q */
        if ((err = mp_exptmod(tmp, key->dQ, key->q, tmpb)) != CRYPT_OK) {
            goto error;
        }

        /* tmp = (tmpa - tmpb) * qInv (mod p) */
        if ((err = mp_sub(tmpa, tmpb, tmp)) != CRYPT_OK) {
            goto error;
        }
        if ((err = mp_mulmod(tmp, key->qP, key->p, tmp)) != CRYPT_OK) {
            goto error;
        }

        /* tmp = tmpb + q * tmp */
        if ((err = mp_mul(tmp, key->q, tmp)) != CRYPT_OK) {
            goto error;
        }
        if ((err = mp_add(tmp, tmpb, tmp)) != CRYPT_OK) {
            goto error;
        }
    } else {
        /* exptmod it */
        if ((err = mp_exptmod(tmp, key->e, key->N, tmp)) != CRYPT_OK) {
            goto error;
        }
    }

    /* read it back */
    x = (unsigned long)mp_unsigned_bin_size(key->N);
    if (x > *outlen) {
        *outlen = x;
        err     = CRYPT_BUFFER_OVERFLOW;
        goto error;
    }

    /* this should never happen ... */
    if (mp_unsigned_bin_size(tmp) > mp_unsigned_bin_size(key->N)) {
        err = CRYPT_ERROR;
        goto error;
    }
    *outlen = x;

    /* convert it */
    zeromem(out, x);
    if ((err = mp_to_unsigned_bin(tmp, out + (x - mp_unsigned_bin_size(tmp)))) != CRYPT_OK) {
        goto error;
    }

    /* clean up and return */
    err = CRYPT_OK;
error:
    mp_clear_multi(tmp, tmpa, tmpb, NULL);
    return err;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/rsa/rsa_exptmod.c,v $ */
/* $Revision: 1.18 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file rsa_free.c
   Free an RSA key, Tom St Denis
 */

#ifdef LTC_MRSA

/**
   Free an RSA key from memory
   @param key   The RSA key to free
 */
void rsa_free(rsa_key *key) {
    LTC_ARGCHKVD(key != NULL);
    mp_clear_multi(key->e, key->d, key->N, key->dQ, key->dP, key->qP, key->p, key->q, NULL);
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/rsa/rsa_free.c,v $ */
/* $Revision: 1.10 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file rsa_import.c
   Import a LTC_PKCS RSA key, Tom St Denis
 */

#ifdef LTC_MRSA

/**
   Import an RSAPublicKey or RSAPrivateKey [two-prime only, only support >= 1024-bit keys, defined in LTC_PKCS #1 v2.1]
   @param in      The packet to import from
   @param inlen   It's length (octets)
   @param key     [out] Destination for newly imported key
   @return CRYPT_OK if successful, upon error allocated memory is freed
 */
int rsa_import(const unsigned char *in, unsigned long inlen, rsa_key *key) {
    int           err;
    void          *zero;
    unsigned char *tmpbuf;
    unsigned long t, x, y, z, tmpoid[16];
    ltc_asn1_list ssl_pubkey_hashoid[2];
    ltc_asn1_list ssl_pubkey[2];

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(key != NULL);
    LTC_ARGCHK(ltc_mp.name != NULL);

    /* init key */
    if ((err = mp_init_multi(&key->e, &key->d, &key->N, &key->dQ,
                             &key->dP, &key->qP, &key->p, &key->q, NULL)) != CRYPT_OK) {
        return err;
    }

    /* see if the OpenSSL DER format RSA public key will work */
    tmpbuf = XCALLOC(1, MAX_RSA_SIZE * 8);
    if (tmpbuf == NULL) {
        err = CRYPT_MEM;
        goto LBL_ERR;
    }

    /* this includes the internal hash ID and optional params (NULL in this case) */
    LTC_SET_ASN1(ssl_pubkey_hashoid, 0, LTC_ASN1_OBJECT_IDENTIFIER, tmpoid, sizeof(tmpoid) / sizeof(tmpoid[0]));
    LTC_SET_ASN1(ssl_pubkey_hashoid, 1, LTC_ASN1_NULL, NULL, 0);

    /* the actual format of the SSL DER key is odd, it stores a RSAPublicKey in a **BIT** string ... so we have to extract it
       then proceed to convert bit to octet
     */
    LTC_SET_ASN1(ssl_pubkey, 0, LTC_ASN1_SEQUENCE, &ssl_pubkey_hashoid, 2);
    LTC_SET_ASN1(ssl_pubkey, 1, LTC_ASN1_BIT_STRING, tmpbuf, MAX_RSA_SIZE * 8);

    if (der_decode_sequence(in, inlen,
                            ssl_pubkey, 2UL) == CRYPT_OK) {
        /* ok now we have to reassemble the BIT STRING to an OCTET STRING.  Thanks OpenSSL... */
        for (t = y = z = x = 0; x < ssl_pubkey[1].size; x++) {
            y = (y << 1) | tmpbuf[x];
            if (++z == 8) {
                tmpbuf[t++] = (unsigned char)y;
                y           = 0;
                z           = 0;
            }
        }

        /* now it should be SEQUENCE { INTEGER, INTEGER } */
        if ((err = der_decode_sequence_multi(tmpbuf, t,
                                             LTC_ASN1_INTEGER, 1UL, key->N,
                                             LTC_ASN1_INTEGER, 1UL, key->e,
                                             LTC_ASN1_EOL, 0UL, NULL)) != CRYPT_OK) {
            XFREE(tmpbuf);
            goto LBL_ERR;
        }
        XFREE(tmpbuf);
        key->type = PK_PUBLIC;
        return CRYPT_OK;
    }
    XFREE(tmpbuf);

    /* not SSL public key, try to match against LTC_PKCS #1 standards */
    if ((err = der_decode_sequence_multi(in, inlen,
                                         LTC_ASN1_INTEGER, 1UL, key->N,
                                         LTC_ASN1_EOL, 0UL, NULL)) != CRYPT_OK) {
        goto LBL_ERR;
    }

    if (mp_cmp_d(key->N, 0) == LTC_MP_EQ) {
        if ((err = mp_init(&zero)) != CRYPT_OK) {
            goto LBL_ERR;
        }
        /* it's a private key */
        if ((err = der_decode_sequence_multi(in, inlen,
                                             LTC_ASN1_INTEGER, 1UL, zero,
                                             LTC_ASN1_INTEGER, 1UL, key->N,
                                             LTC_ASN1_INTEGER, 1UL, key->e,
                                             LTC_ASN1_INTEGER, 1UL, key->d,
                                             LTC_ASN1_INTEGER, 1UL, key->p,
                                             LTC_ASN1_INTEGER, 1UL, key->q,
                                             LTC_ASN1_INTEGER, 1UL, key->dP,
                                             LTC_ASN1_INTEGER, 1UL, key->dQ,
                                             LTC_ASN1_INTEGER, 1UL, key->qP,
                                             LTC_ASN1_EOL, 0UL, NULL)) != CRYPT_OK) {
            mp_clear(zero);
            goto LBL_ERR;
        }
        mp_clear(zero);
        key->type = PK_PRIVATE;
    } else if (mp_cmp_d(key->N, 1) == LTC_MP_EQ) {
        /* we don't support multi-prime RSA */
        err = CRYPT_PK_INVALID_TYPE;
        goto LBL_ERR;
    } else {
        /* it's a public key and we lack e */
        if ((err = der_decode_sequence_multi(in, inlen,
                                             LTC_ASN1_INTEGER, 1UL, key->N,
                                             LTC_ASN1_INTEGER, 1UL, key->e,
                                             LTC_ASN1_EOL, 0UL, NULL)) != CRYPT_OK) {
            goto LBL_ERR;
        }
        key->type = PK_PUBLIC;
    }
    return CRYPT_OK;
LBL_ERR:
    mp_clear_multi(key->d, key->e, key->N, key->dQ, key->dP, key->qP, key->p, key->q, NULL);
    return err;
}
#endif /* LTC_MRSA */


/* $Source: /cvs/libtom/libtomcrypt/src/pk/rsa/rsa_import.c,v $ */
/* $Revision: 1.23 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file rsa_make_key.c
   RSA key generation, Tom St Denis
 */

#ifdef LTC_MRSA

/**
   Create an RSA key
   @param prng     An active PRNG state
   @param wprng    The index of the PRNG desired
   @param size     The size of the modulus (key size) desired (octets)
   @param e        The "e" value (public key).  e==65537 is a good choice
   @param key      [out] Destination of a newly created private key pair
   @return CRYPT_OK if successful, upon error all allocated ram is freed
 */
int rsa_make_key(prng_state *prng, int wprng, int size, long e, rsa_key *key) {
    void *p, *q, *tmp1, *tmp2, *tmp3;
    int  err;

    LTC_ARGCHK(ltc_mp.name != NULL);
    LTC_ARGCHK(key != NULL);

    if ((size < (MIN_RSA_SIZE / 8)) || (size > (MAX_RSA_SIZE / 8))) {
        return CRYPT_INVALID_KEYSIZE;
    }

    if ((e < 3) || ((e & 1) == 0)) {
        return CRYPT_INVALID_ARG;
    }

    if ((err = prng_is_valid(wprng)) != CRYPT_OK) {
        return err;
    }

    if ((err = mp_init_multi(&p, &q, &tmp1, &tmp2, &tmp3, NULL)) != CRYPT_OK) {
        return err;
    }

    /* make primes p and q (optimization provided by Wayne Scott) */
    if ((err = mp_set_int(tmp3, e)) != CRYPT_OK) {
        goto errkey;
    }                                                                                  /* tmp3 = e */

    /* make prime "p" */
    do {
        if ((err = rand_prime(p, size / 2, prng, wprng)) != CRYPT_OK) {
            goto errkey;
        }
        if ((err = mp_sub_d(p, 1, tmp1)) != CRYPT_OK) {
            goto errkey;
        }                                                                              /* tmp1 = p-1 */
        if ((err = mp_gcd(tmp1, tmp3, tmp2)) != CRYPT_OK) {
            goto errkey;
        }                                                                              /* tmp2 = gcd(p-1, e) */
    } while (mp_cmp_d(tmp2, 1) != 0);                                                  /* while e divides p-1 */

    /* make prime "q" */
    do {
        if ((err = rand_prime(q, size / 2, prng, wprng)) != CRYPT_OK) {
            goto errkey;
        }
        if ((err = mp_sub_d(q, 1, tmp1)) != CRYPT_OK) {
            goto errkey;
        }                                                                             /* tmp1 = q-1 */
        if ((err = mp_gcd(tmp1, tmp3, tmp2)) != CRYPT_OK) {
            goto errkey;
        }                                                                             /* tmp2 = gcd(q-1, e) */
    } while (mp_cmp_d(tmp2, 1) != 0);                                                 /* while e divides q-1 */

    /* tmp1 = lcm(p-1, q-1) */
    if ((err = mp_sub_d(p, 1, tmp2)) != CRYPT_OK) {
        goto errkey;
    }                                                                                 /* tmp2 = p-1 */
                                                                                      /* tmp1 = q-1 (previous do/while loop) */
    if ((err = mp_lcm(tmp1, tmp2, tmp1)) != CRYPT_OK) {
        goto errkey;
    }                                                                                 /* tmp1 = lcm(p-1, q-1) */

    /* make key */
    if ((err = mp_init_multi(&key->e, &key->d, &key->N, &key->dQ, &key->dP, &key->qP, &key->p, &key->q, NULL)) != CRYPT_OK) {
        goto errkey;
    }

    if ((err = mp_set_int(key->e, e)) != CRYPT_OK) {
        goto errkey;
    }                                                                                   /* key->e =  e */
    if ((err = mp_invmod(key->e, tmp1, key->d)) != CRYPT_OK) {
        goto errkey;
    }                                                                                   /* key->d = 1/e mod lcm(p-1,q-1) */
    if ((err = mp_mul(p, q, key->N)) != CRYPT_OK) {
        goto errkey;
    }                                                                                   /* key->N = pq */

    /* optimize for CRT now */
    /* find d mod q-1 and d mod p-1 */
    if ((err = mp_sub_d(p, 1, tmp1)) != CRYPT_OK) {
        goto errkey;
    }                                                                                   /* tmp1 = q-1 */
    if ((err = mp_sub_d(q, 1, tmp2)) != CRYPT_OK) {
        goto errkey;
    }                                                                                   /* tmp2 = p-1 */
    if ((err = mp_mod(key->d, tmp1, key->dP)) != CRYPT_OK) {
        goto errkey;
    }                                                                                   /* dP = d mod p-1 */
    if ((err = mp_mod(key->d, tmp2, key->dQ)) != CRYPT_OK) {
        goto errkey;
    }                                                                                   /* dQ = d mod q-1 */
    if ((err = mp_invmod(q, p, key->qP)) != CRYPT_OK) {
        goto errkey;
    }                                                                                   /* qP = 1/q mod p */

    if ((err = mp_copy(p, key->p)) != CRYPT_OK) {
        goto errkey;
    }
    if ((err = mp_copy(q, key->q)) != CRYPT_OK) {
        goto errkey;
    }

    /* set key type (in this case it's CRT optimized) */
    key->type = PK_PRIVATE;

    /* return ok and free temps */
    err = CRYPT_OK;
    goto cleanup;
errkey:
    mp_clear_multi(key->d, key->e, key->N, key->dQ, key->dP, key->qP, key->p, key->q, NULL);
cleanup:
    mp_clear_multi(tmp3, tmp2, tmp1, p, q, NULL);
    return err;
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/rsa/rsa_make_key.c,v $ */
/* $Revision: 1.16 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file rsa_sign_hash.c
   RSA LTC_PKCS #1 v1.5 and v2 PSS sign hash, Tom St Denis and Andreas Lange
 */

#ifdef LTC_MRSA

/**
   LTC_PKCS #1 pad then sign
   @param in        The hash to sign
   @param inlen     The length of the hash to sign (octets)
   @param out       [out] The signature
   @param outlen    [in/out] The max size and resulting size of the signature
   @param padding   Type of padding (LTC_LTC_PKCS_1_PSS or LTC_LTC_PKCS_1_V1_5)
   @param prng      An active PRNG state
   @param prng_idx  The index of the PRNG desired
   @param hash_idx  The index of the hash desired
   @param saltlen   The length of the salt desired (octets)
   @param key       The private RSA key to use
   @return CRYPT_OK if successful
 */
int rsa_sign_hash_ex(const unsigned char *in, unsigned long inlen,
                     unsigned char *out, unsigned long *outlen,
                     int padding,
                     prng_state *prng, int prng_idx,
                     int hash_idx, unsigned long saltlen,
                     rsa_key *key) {
    unsigned long modulus_bitlen, modulus_bytelen, x, y;
    int           err;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(out != NULL);
    LTC_ARGCHK(outlen != NULL);
    LTC_ARGCHK(key != NULL);

    /* valid padding? */
    if ((padding != LTC_LTC_PKCS_1_V1_5) && (padding != LTC_LTC_PKCS_1_PSS)) {
        return CRYPT_PK_INVALID_PADDING;
    }

    if (padding == LTC_LTC_PKCS_1_PSS) {
        /* valid prng and hash ? */
        if ((err = prng_is_valid(prng_idx)) != CRYPT_OK) {
            return err;
        }
        if ((err = hash_is_valid(hash_idx)) != CRYPT_OK) {
            return err;
        }
    }

    /* get modulus len in bits */
    modulus_bitlen = mp_count_bits((key->N));

    /* outlen must be at least the size of the modulus */
    modulus_bytelen = mp_unsigned_bin_size((key->N));
    if (modulus_bytelen > *outlen) {
        *outlen = modulus_bytelen;
        return CRYPT_BUFFER_OVERFLOW;
    }

    if (padding == LTC_LTC_PKCS_1_PSS) {
        /* PSS pad the key */
        x = *outlen;
        if ((err = pkcs_1_pss_encode(in, inlen, saltlen, prng, prng_idx,
                                     hash_idx, modulus_bitlen, out, &x)) != CRYPT_OK) {
            return err;
        }
    } else {
        /* LTC_PKCS #1 v1.5 pad the hash */
        unsigned char *tmpin;
        ltc_asn1_list digestinfo[2], siginfo[2];

        /* not all hashes have OIDs... so sad */
        if (hash_descriptor[hash_idx].OIDlen == 0) {
            return CRYPT_INVALID_ARG;
        }

        /* construct the SEQUENCE
           SEQUENCE {
             SEQUENCE {hashoid OID
                       blah    NULL
             }
             hash    OCTET STRING
           }
         */
        LTC_SET_ASN1(digestinfo, 0, LTC_ASN1_OBJECT_IDENTIFIER, hash_descriptor[hash_idx].OID, hash_descriptor[hash_idx].OIDlen);
        LTC_SET_ASN1(digestinfo, 1, LTC_ASN1_NULL, NULL, 0);
        LTC_SET_ASN1(siginfo, 0, LTC_ASN1_SEQUENCE, digestinfo, 2);
        LTC_SET_ASN1(siginfo, 1, LTC_ASN1_OCTET_STRING, in, inlen);

        /* allocate memory for the encoding */
        y     = mp_unsigned_bin_size(key->N);
        tmpin = XMALLOC(y);
        if (tmpin == NULL) {
            return CRYPT_MEM;
        }

        if ((err = der_encode_sequence(siginfo, 2, tmpin, &y)) != CRYPT_OK) {
            XFREE(tmpin);
            return err;
        }

        x = *outlen;
        if ((err = pkcs_1_v1_5_encode(tmpin, y, LTC_LTC_PKCS_1_EMSA,
                                      modulus_bitlen, NULL, 0,
                                      out, &x)) != CRYPT_OK) {
            XFREE(tmpin);
            return err;
        }
        XFREE(tmpin);
    }

    /* RSA encode it */
    return ltc_mp.rsa_me(out, x, out, outlen, PK_PRIVATE, key);
}
#endif /* LTC_MRSA */

/* $Source: /cvs/libtom/libtomcrypt/src/pk/rsa/rsa_sign_hash.c,v $ */
/* $Revision: 1.11 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file rsa_verify_hash.c
   RSA LTC_PKCS #1 v1.5 or v2 PSS signature verification, Tom St Denis and Andreas Lange
 */

#ifdef LTC_MRSA

/**
   LTC_PKCS #1 de-sign then v1.5 or PSS depad
   @param sig              The signature data
   @param siglen           The length of the signature data (octets)
   @param hash             The hash of the message that was signed
   @param hashlen          The length of the hash of the message that was signed (octets)
   @param padding          Type of padding (LTC_LTC_PKCS_1_PSS or LTC_LTC_PKCS_1_V1_5)
   @param hash_idx         The index of the desired hash
   @param saltlen          The length of the salt used during signature
   @param stat             [out] The result of the signature comparison, 1==valid, 0==invalid
   @param key              The public RSA key corresponding to the key that performed the signature
   @return CRYPT_OK on success (even if the signature is invalid)
 */
int rsa_verify_hash_ex(const unsigned char *sig, unsigned long siglen,
                       const unsigned char *hash, unsigned long hashlen,
                       int padding,
                       int hash_idx, unsigned long saltlen,
                       int *stat, rsa_key *key) {
    unsigned long modulus_bitlen, modulus_bytelen, x;
    int           err;
    unsigned char *tmpbuf;

    LTC_ARGCHK(hash != NULL);
    LTC_ARGCHK(sig != NULL);
    LTC_ARGCHK(stat != NULL);
    LTC_ARGCHK(key != NULL);

    /* default to invalid */
    *stat = 0;

    /* valid padding? */

    if ((padding != LTC_LTC_PKCS_1_V1_5) &&
        (padding != LTC_LTC_PKCS_1_PSS)) {
        return CRYPT_PK_INVALID_PADDING;
    }

    if (padding == LTC_LTC_PKCS_1_PSS) {
        /* valid hash ? */
        if ((err = hash_is_valid(hash_idx)) != CRYPT_OK) {
            return err;
        }
    }

    /* get modulus len in bits */
    modulus_bitlen = mp_count_bits((key->N));

    /* outlen must be at least the size of the modulus */
    modulus_bytelen = mp_unsigned_bin_size((key->N));
    if (modulus_bytelen != siglen) {
        return CRYPT_INVALID_PACKET;
    }

    /* allocate temp buffer for decoded sig */
    tmpbuf = XMALLOC(siglen);
    if (tmpbuf == NULL) {
        return CRYPT_MEM;
    }

    /* RSA decode it  */
    x = siglen;
    if ((err = ltc_mp.rsa_me(sig, siglen, tmpbuf, &x, PK_PUBLIC, key)) != CRYPT_OK) {
        XFREE(tmpbuf);
        return err;
    }

    /* make sure the output is the right size */
    if (x != siglen) {
        XFREE(tmpbuf);
        return CRYPT_INVALID_PACKET;
    }

    if (padding == LTC_LTC_PKCS_1_PSS) {
        /* PSS decode and verify it */
        err = pkcs_1_pss_decode(hash, hashlen, tmpbuf, x, saltlen, hash_idx, modulus_bitlen, stat);
    } else {
        /* LTC_PKCS #1 v1.5 decode it */
        unsigned char *out;
        unsigned long outlen, loid[16];
        int           decoded;
        ltc_asn1_list digestinfo[2], siginfo[2];

        /* not all hashes have OIDs... so sad */
        if (hash_descriptor[hash_idx].OIDlen == 0) {
            err = CRYPT_INVALID_ARG;
            goto bail_2;
        }

        /* allocate temp buffer for decoded hash */
        outlen = ((modulus_bitlen >> 3) + (modulus_bitlen & 7 ? 1 : 0)) - 3;
        out    = XMALLOC(outlen);
        if (out == NULL) {
            err = CRYPT_MEM;
            goto bail_2;
        }

        if ((err = pkcs_1_v1_5_decode(tmpbuf, x, LTC_LTC_PKCS_1_EMSA, modulus_bitlen, out, &outlen, &decoded)) != CRYPT_OK) {
            XFREE(out);
            goto bail_2;
        }

        /* now we must decode out[0...outlen-1] using ASN.1, test the OID and then test the hash */

        /* construct the SEQUENCE
           SEQUENCE {
             SEQUENCE {hashoid OID
                       blah    NULL
             }
             hash    OCTET STRING
           }
         */
        LTC_SET_ASN1(digestinfo, 0, LTC_ASN1_OBJECT_IDENTIFIER, loid, sizeof(loid) / sizeof(loid[0]));
        LTC_SET_ASN1(digestinfo, 1, LTC_ASN1_NULL, NULL, 0);
        LTC_SET_ASN1(siginfo, 0, LTC_ASN1_SEQUENCE, digestinfo, 2);
        LTC_SET_ASN1(siginfo, 1, LTC_ASN1_OCTET_STRING, tmpbuf, siglen);

        if ((err = der_decode_sequence(out, outlen, siginfo, 2)) != CRYPT_OK) {
            XFREE(out);
            goto bail_2;
        }

        /* test OID */
        if ((digestinfo[0].size == hash_descriptor[hash_idx].OIDlen) &&
            (XMEMCMP(digestinfo[0].data, hash_descriptor[hash_idx].OID, sizeof(unsigned long) * hash_descriptor[hash_idx].OIDlen) == 0) &&
            (siginfo[1].size == hashlen) &&
            (XMEMCMP(siginfo[1].data, hash, hashlen) == 0)) {
            *stat = 1;
        }

 #ifdef LTC_CLEAN_STACK
        zeromem(out, outlen);
 #endif
        XFREE(out);
    }

bail_2:
 #ifdef LTC_CLEAN_STACK
    zeromem(tmpbuf, siglen);
 #endif
    XFREE(tmpbuf);
    return err;
}
#endif /* LTC_MRSA */

/* $Source: /cvs/libtom/libtomcrypt/src/pk/rsa/rsa_verify_hash.c,v $ */
/* $Revision: 1.13 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file sprng.c
   Secure PRNG, Tom St Denis
 */

/* A secure PRNG using the RNG functions.  Basically this is a
 * wrapper that allows you to use a secure RNG as a PRNG
 * in the various other functions.
 */

#ifdef LTC_SPRNG

const struct ltc_prng_descriptor sprng_desc =
{
    "sprng",            0,
    &sprng_start,
    &sprng_add_entropy,
    &sprng_ready,
    &sprng_read,
    &sprng_done,
    &sprng_export,
    &sprng_import,
    &sprng_test
};

/**
   Start the PRNG
   @param prng     [out] The PRNG state to initialize
   @return CRYPT_OK if successful
 */
int sprng_start(prng_state *prng) {
    return CRYPT_OK;
}

/**
   Add entropy to the PRNG state
   @param in       The data to add
   @param inlen    Length of the data to add
   @param prng     PRNG state to update
   @return CRYPT_OK if successful
 */
int sprng_add_entropy(const unsigned char *in, unsigned long inlen, prng_state *prng) {
    return CRYPT_OK;
}

/**
   Make the PRNG ready to read from
   @param prng   The PRNG to make active
   @return CRYPT_OK if successful
 */
int sprng_ready(prng_state *prng) {
    return CRYPT_OK;
}

/**
   Read from the PRNG
   @param out      Destination
   @param outlen   Length of output
   @param prng     The active PRNG to read from
   @return Number of octets read
 */
unsigned long sprng_read(unsigned char *out, unsigned long outlen, prng_state *prng) {
    LTC_ARGCHK(out != NULL);
    return rng_get_bytes(out, outlen, NULL);
}

/**
   Terminate the PRNG
   @param prng   The PRNG to terminate
   @return CRYPT_OK if successful
 */
int sprng_done(prng_state *prng) {
    return CRYPT_OK;
}

/**
   Export the PRNG state
   @param out       [out] Destination
   @param outlen    [in/out] Max size and resulting size of the state
   @param prng      The PRNG to export
   @return CRYPT_OK if successful
 */
int sprng_export(unsigned char *out, unsigned long *outlen, prng_state *prng) {
    LTC_ARGCHK(outlen != NULL);

    *outlen = 0;
    return CRYPT_OK;
}

/**
   Import a PRNG state
   @param in       The PRNG state
   @param inlen    Size of the state
   @param prng     The PRNG to import
   @return CRYPT_OK if successful
 */
int sprng_import(const unsigned char *in, unsigned long inlen, prng_state *prng) {
    return CRYPT_OK;
}

/**
   PRNG self-test
   @return CRYPT_OK if successful, CRYPT_NOP if self-testing has been disabled
 */
int sprng_test(void) {
    return CRYPT_OK;
}
#endif



/* $Source: /cvs/libtom/libtomcrypt/src/prngs/sprng.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file zeromem.c
   Zero a block of memory, Tom St Denis
 */

/**
   Zero a block of memory
   @param out    The destination of the area to zero
   @param outlen The length of the area to zero (octets)
 */
void zeromem(void *out, size_t outlen) {
    unsigned char *mem = out;

    LTC_ARGCHKVD(out != NULL);
    while (outlen-- > 0) {
        *mem++ = 0;
    }
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/zeromem.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file sha1.c
   LTC_SHA1 code by Tom St Denis
 */


#ifdef LTC_SHA1

const struct ltc_hash_descriptor sha1_desc =
{
    "sha1",
    2,
    20,
    64,

    /* OID */
    { 1,        3, 14, 3, 2, 26, },
    6,

    &sha1_init,
    &sha1_process,
    &sha1_done,
    &sha1_test,
    NULL
};

 #define F0(x, y, z)    (z ^ (x & (y ^ z)))
 #define F1(x, y, z)    (x ^ y ^ z)
 #define F2(x, y, z)    ((x & y) | (z & (x | y)))
 #define F3(x, y, z)    (x ^ y ^ z)

 #ifdef LTC_CLEAN_STACK
static int _sha1_compress(hash_state *md, unsigned char *buf)
 #else
static int  sha1_compress(hash_state *md, unsigned char *buf)
 #endif
{
    ulong32 a, b, c, d, e, W[80], i;

 #ifdef LTC_SMALL_CODE
    ulong32 t;
 #endif

    /* copy the state into 512-bits into W[0..15] */
    for (i = 0; i < 16; i++) {
        LOAD32H(W[i], buf + (4 * i));
    }

    /* copy state */
    a = md->sha1.state[0];
    b = md->sha1.state[1];
    c = md->sha1.state[2];
    d = md->sha1.state[3];
    e = md->sha1.state[4];

    /* expand it */
    for (i = 16; i < 80; i++) {
        W[i] = ROL(W[i - 3] ^ W[i - 8] ^ W[i - 14] ^ W[i - 16], 1);
    }

    /* compress */
    /* round one */
 #define FF0(a, b, c, d, e, i)    e = (ROLc(a, 5) + F0(b, c, d) + e + W[i] + 0x5a827999UL); b = ROLc(b, 30);
 #define FF1(a, b, c, d, e, i)    e = (ROLc(a, 5) + F1(b, c, d) + e + W[i] + 0x6ed9eba1UL); b = ROLc(b, 30);
 #define FF2(a, b, c, d, e, i)    e = (ROLc(a, 5) + F2(b, c, d) + e + W[i] + 0x8f1bbcdcUL); b = ROLc(b, 30);
 #define FF3(a, b, c, d, e, i)    e = (ROLc(a, 5) + F3(b, c, d) + e + W[i] + 0xca62c1d6UL); b = ROLc(b, 30);

 #ifdef LTC_SMALL_CODE
    for (i = 0; i < 20; ) {
        FF0(a, b, c, d, e, i++);
        t = e;
        e = d;
        d = c;
        c = b;
        b = a;
        a = t;
    }

    for ( ; i < 40; ) {
        FF1(a, b, c, d, e, i++);
        t = e;
        e = d;
        d = c;
        c = b;
        b = a;
        a = t;
    }

    for ( ; i < 60; ) {
        FF2(a, b, c, d, e, i++);
        t = e;
        e = d;
        d = c;
        c = b;
        b = a;
        a = t;
    }

    for ( ; i < 80; ) {
        FF3(a, b, c, d, e, i++);
        t = e;
        e = d;
        d = c;
        c = b;
        b = a;
        a = t;
    }

 #else
    for (i = 0; i < 20; ) {
        FF0(a, b, c, d, e, i++);
        FF0(e, a, b, c, d, i++);
        FF0(d, e, a, b, c, i++);
        FF0(c, d, e, a, b, i++);
        FF0(b, c, d, e, a, i++);
    }

    /* round two */
    for ( ; i < 40; ) {
        FF1(a, b, c, d, e, i++);
        FF1(e, a, b, c, d, i++);
        FF1(d, e, a, b, c, i++);
        FF1(c, d, e, a, b, i++);
        FF1(b, c, d, e, a, i++);
    }

    /* round three */
    for ( ; i < 60; ) {
        FF2(a, b, c, d, e, i++);
        FF2(e, a, b, c, d, i++);
        FF2(d, e, a, b, c, i++);
        FF2(c, d, e, a, b, i++);
        FF2(b, c, d, e, a, i++);
    }

    /* round four */
    for ( ; i < 80; ) {
        FF3(a, b, c, d, e, i++);
        FF3(e, a, b, c, d, i++);
        FF3(d, e, a, b, c, i++);
        FF3(c, d, e, a, b, i++);
        FF3(b, c, d, e, a, i++);
    }
 #endif

 #undef FF0
 #undef FF1
 #undef FF2
 #undef FF3

    /* store */
    md->sha1.state[0] = md->sha1.state[0] + a;
    md->sha1.state[1] = md->sha1.state[1] + b;
    md->sha1.state[2] = md->sha1.state[2] + c;
    md->sha1.state[3] = md->sha1.state[3] + d;
    md->sha1.state[4] = md->sha1.state[4] + e;

    return CRYPT_OK;
}

 #ifdef LTC_CLEAN_STACK
static int sha1_compress(hash_state *md, unsigned char *buf) {
    int err;

    err = _sha1_compress(md, buf);
    burn_stack(sizeof(ulong32) * 87);
    return err;
}
 #endif

/**
   Initialize the hash state
   @param md   The hash state you wish to initialize
   @return CRYPT_OK if successful
 */
int sha1_init(hash_state *md) {
    LTC_ARGCHK(md != NULL);
    md->sha1.state[0] = 0x67452301UL;
    md->sha1.state[1] = 0xefcdab89UL;
    md->sha1.state[2] = 0x98badcfeUL;
    md->sha1.state[3] = 0x10325476UL;
    md->sha1.state[4] = 0xc3d2e1f0UL;
    md->sha1.curlen   = 0;
    md->sha1.length   = 0;
    return CRYPT_OK;
}

/**
   Process a block of memory though the hash
   @param md     The hash state
   @param in     The data to hash
   @param inlen  The length of the data (octets)
   @return CRYPT_OK if successful
 */
HASH_PROCESS(sha1_process, sha1_compress, sha1, 64)

/**
   Terminate the hash to get the digest
   @param md  The hash state
   @param out [out] The destination of the hash (20 bytes)
   @return CRYPT_OK if successful
 */
int sha1_done(hash_state *md, unsigned char *out) {
    int i;

    LTC_ARGCHK(md != NULL);
    LTC_ARGCHK(out != NULL);

    if (md->sha1.curlen >= sizeof(md->sha1.buf)) {
        return CRYPT_INVALID_ARG;
    }

    /* increase the length of the message */
    md->sha1.length += md->sha1.curlen * 8;

    /* append the '1' bit */
    md->sha1.buf[md->sha1.curlen++] = (unsigned char)0x80;

    /* if the length is currently above 56 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (md->sha1.curlen > 56) {
        while (md->sha1.curlen < 64) {
            md->sha1.buf[md->sha1.curlen++] = (unsigned char)0;
        }
        sha1_compress(md, md->sha1.buf);
        md->sha1.curlen = 0;
    }

    /* pad upto 56 bytes of zeroes */
    while (md->sha1.curlen < 56) {
        md->sha1.buf[md->sha1.curlen++] = (unsigned char)0;
    }

    /* store length */
    STORE64H(md->sha1.length, md->sha1.buf + 56);
    sha1_compress(md, md->sha1.buf);

    /* copy output */
    for (i = 0; i < 5; i++) {
        STORE32H(md->sha1.state[i], out + (4 * i));
    }
 #ifdef LTC_CLEAN_STACK
    zeromem(md, sizeof(hash_state));
 #endif
    return CRYPT_OK;
}

/**
   Self-test the hash
   @return CRYPT_OK if successful, CRYPT_NOP if self-tests have been disabled
 */
int  sha1_test(void) {
 #ifndef LTC_TEST
    return CRYPT_NOP;
 #else
    static const struct {
        char          *msg;
        unsigned char hash[20];
    } tests[] = {
        { "abc",
            {                                                       0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a,
                                                                  0xba, 0x3e, 0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c,
                                                                  0x9c, 0xd0, 0xd8, 0x9d } },
        { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
            {                                                       0x84, 0x98, 0x3E, 0x44, 0x1C, 0x3B, 0xD2, 0x6E,
                                                                  0xBA, 0xAE, 0x4A, 0xA1, 0xF9, 0x51, 0x29, 0xE5,
                                                                  0xE5, 0x46, 0x70, 0xF1 } }
    };

    int           i;
    unsigned char tmp[20];
    hash_state    md;

    for (i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++) {
        sha1_init(&md);
        sha1_process(&md, (unsigned char *)tests[i].msg, (unsigned long)strlen(tests[i].msg));
        sha1_done(&md, tmp);
        if (XMEMCMP(tmp, tests[i].hash, 20) != 0) {
            return CRYPT_FAIL_TESTVECTOR;
        }
    }
    return CRYPT_OK;
 #endif
}
#endif



/* $Source: /cvs/libtom/libtomcrypt/src/hashes/sha1.c,v $ */
/* $Revision: 1.10 $ */
/* $Date: 2007/05/12 14:25:28 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
  @file sha256.c
  LTC_SHA256 by Tom St Denis
*/

#ifdef LTC_SHA256

const struct ltc_hash_descriptor sha256_desc =
{
    "sha256",
    0,
    32,
    64,

    /* OID */
   { 2, 16, 840, 1, 101, 3, 4, 2, 1,  },
   9,

    &sha256_init,
    &sha256_process,
    &sha256_done,
    &sha256_test,
    NULL
};

#ifdef LTC_SMALL_CODE
/* the K array */
static const ulong32 K[64] = {
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL,
    0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL, 0xd807aa98UL, 0x12835b01UL,
    0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL,
    0xc19bf174UL, 0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
    0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL, 0x983e5152UL,
    0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL,
    0x06ca6351UL, 0x14292967UL, 0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL,
    0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL,
    0xd6990624UL, 0xf40e3585UL, 0x106aa070UL, 0x19a4c116UL, 0x1e376c08UL,
    0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL,
    0x682e6ff3UL, 0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
    0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};
#endif

/* Various logical functions */
#define Ch(x,y,z)       (z ^ (x & (y ^ z)))
#define Maj(x,y,z)      (((x | y) & z) | (x & y))
#define S(x, n)         RORc((x),(n))
#define R(x, n)         (((x)&0xFFFFFFFFUL)>>(n))
#define Sigma0(x)       (S(x, 2) ^ S(x, 13) ^ S(x, 22))
#define Sigma1(x)       (S(x, 6) ^ S(x, 11) ^ S(x, 25))
#define Gamma0(x)       (S(x, 7) ^ S(x, 18) ^ R(x, 3))
#define Gamma1(x)       (S(x, 17) ^ S(x, 19) ^ R(x, 10))

/* compress 512-bits */
#ifdef LTC_CLEAN_STACK
static int _sha256_compress(hash_state * md, unsigned char *buf)
#else
static int  sha256_compress(hash_state * md, unsigned char *buf)
#endif
{
    ulong32 S[8], W[64], t0, t1;
#ifdef LTC_SMALL_CODE
    ulong32 t;
#endif
    int i;

    /* copy state into S */
    for (i = 0; i < 8; i++) {
        S[i] = md->sha256.state[i];
    }

    /* copy the state into 512-bits into W[0..15] */
    for (i = 0; i < 16; i++) {
        LOAD32H(W[i], buf + (4*i));
    }

    /* fill W[16..63] */
    for (i = 16; i < 64; i++) {
        W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];
    }

    /* Compress */
#ifdef LTC_SMALL_CODE
#define RND(a,b,c,d,e,f,g,h,i)                         \
     t0 = h + Sigma1(e) + Ch(e, f, g) + K[i] + W[i];   \
     t1 = Sigma0(a) + Maj(a, b, c);                    \
     d += t0;                                          \
     h  = t0 + t1;

     for (i = 0; i < 64; ++i) {
         RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],i);
         t = S[7]; S[7] = S[6]; S[6] = S[5]; S[5] = S[4];
         S[4] = S[3]; S[3] = S[2]; S[2] = S[1]; S[1] = S[0]; S[0] = t;
     }
#else
#define RND(a,b,c,d,e,f,g,h,i,ki)                    \
     t0 = h + Sigma1(e) + Ch(e, f, g) + ki + W[i];   \
     t1 = Sigma0(a) + Maj(a, b, c);                  \
     d += t0;                                        \
     h  = t0 + t1;

    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],0,0x428a2f98);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],1,0x71374491);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],2,0xb5c0fbcf);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],3,0xe9b5dba5);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],4,0x3956c25b);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],5,0x59f111f1);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],6,0x923f82a4);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],7,0xab1c5ed5);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],8,0xd807aa98);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],9,0x12835b01);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],10,0x243185be);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],11,0x550c7dc3);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],12,0x72be5d74);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],13,0x80deb1fe);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],14,0x9bdc06a7);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],15,0xc19bf174);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],16,0xe49b69c1);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],17,0xefbe4786);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],18,0x0fc19dc6);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],19,0x240ca1cc);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],20,0x2de92c6f);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],21,0x4a7484aa);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],22,0x5cb0a9dc);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],23,0x76f988da);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],24,0x983e5152);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],25,0xa831c66d);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],26,0xb00327c8);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],27,0xbf597fc7);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],28,0xc6e00bf3);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],29,0xd5a79147);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],30,0x06ca6351);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],31,0x14292967);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],32,0x27b70a85);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],33,0x2e1b2138);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],34,0x4d2c6dfc);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],35,0x53380d13);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],36,0x650a7354);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],37,0x766a0abb);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],38,0x81c2c92e);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],39,0x92722c85);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],40,0xa2bfe8a1);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],41,0xa81a664b);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],42,0xc24b8b70);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],43,0xc76c51a3);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],44,0xd192e819);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],45,0xd6990624);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],46,0xf40e3585);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],47,0x106aa070);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],48,0x19a4c116);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],49,0x1e376c08);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],50,0x2748774c);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],51,0x34b0bcb5);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],52,0x391c0cb3);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],53,0x4ed8aa4a);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],54,0x5b9cca4f);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],55,0x682e6ff3);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],56,0x748f82ee);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],57,0x78a5636f);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],58,0x84c87814);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],59,0x8cc70208);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],60,0x90befffa);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],61,0xa4506ceb);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],62,0xbef9a3f7);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],63,0xc67178f2);

#undef RND

#endif

    /* feedback */
    for (i = 0; i < 8; i++) {
        md->sha256.state[i] = md->sha256.state[i] + S[i];
    }
    return CRYPT_OK;
}

#ifdef LTC_CLEAN_STACK
static int sha256_compress(hash_state * md, unsigned char *buf)
{
    int err;
    err = _sha256_compress(md, buf);
    burn_stack(sizeof(ulong32) * 74);
    return err;
}
#endif

/**
   Initialize the hash state
   @param md   The hash state you wish to initialize
   @return CRYPT_OK if successful
*/
int sha256_init(hash_state * md)
{
    LTC_ARGCHK(md != NULL);

    md->sha256.curlen = 0;
    md->sha256.length = 0;
    md->sha256.state[0] = 0x6A09E667UL;
    md->sha256.state[1] = 0xBB67AE85UL;
    md->sha256.state[2] = 0x3C6EF372UL;
    md->sha256.state[3] = 0xA54FF53AUL;
    md->sha256.state[4] = 0x510E527FUL;
    md->sha256.state[5] = 0x9B05688CUL;
    md->sha256.state[6] = 0x1F83D9ABUL;
    md->sha256.state[7] = 0x5BE0CD19UL;
    return CRYPT_OK;
}

/**
   Process a block of memory though the hash
   @param md     The hash state
   @param in     The data to hash
   @param inlen  The length of the data (octets)
   @return CRYPT_OK if successful
*/
HASH_PROCESS(sha256_process, sha256_compress, sha256, 64)

/**
   Terminate the hash to get the digest
   @param md  The hash state
   @param out [out] The destination of the hash (32 bytes)
   @return CRYPT_OK if successful
*/
int sha256_done(hash_state * md, unsigned char *out)
{
    int i;

    LTC_ARGCHK(md  != NULL);
    LTC_ARGCHK(out != NULL);

    if (md->sha256.curlen >= sizeof(md->sha256.buf)) {
       return CRYPT_INVALID_ARG;
    }


    /* increase the length of the message */
    md->sha256.length += md->sha256.curlen * 8;

    /* append the '1' bit */
    md->sha256.buf[md->sha256.curlen++] = (unsigned char)0x80;

    /* if the length is currently above 56 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (md->sha256.curlen > 56) {
        while (md->sha256.curlen < 64) {
            md->sha256.buf[md->sha256.curlen++] = (unsigned char)0;
        }
        sha256_compress(md, md->sha256.buf);
        md->sha256.curlen = 0;
    }

    /* pad upto 56 bytes of zeroes */
    while (md->sha256.curlen < 56) {
        md->sha256.buf[md->sha256.curlen++] = (unsigned char)0;
    }

    /* store length */
    STORE64H(md->sha256.length, md->sha256.buf+56);
    sha256_compress(md, md->sha256.buf);

    /* copy output */
    for (i = 0; i < 8; i++) {
        STORE32H(md->sha256.state[i], out+(4*i));
    }
#ifdef LTC_CLEAN_STACK
    zeromem(md, sizeof(hash_state));
#endif
    return CRYPT_OK;
}

/**
  Self-test the hash
  @return CRYPT_OK if successful, CRYPT_NOP if self-tests have been disabled
*/
int  sha256_test(void)
{
 #ifndef LTC_TEST
    return CRYPT_NOP;
 #else
  static const struct {
      char *msg;
      unsigned char hash[32];
  } tests[] = {
    { "abc",
      { 0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
        0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
        0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad }
    },
    { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
      { 0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8,
        0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39,
        0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67,
        0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1 }
    },
  };

  int i;
  unsigned char tmp[32];
  hash_state md;

  for (i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++) {
      sha256_init(&md);
      sha256_process(&md, (unsigned char*)tests[i].msg, (unsigned long)strlen(tests[i].msg));
      sha256_done(&md, tmp);
      if (XMEMCMP(tmp, tests[i].hash, 32) != 0) {
         return CRYPT_FAIL_TESTVECTOR;
      }
  }
  return CRYPT_OK;
 #endif
}

#endif



/* $Source$ */
/* $Revision$ */
/* $Date$ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */
/**
   @param sha384.c
   LTC_SHA384 hash included in sha512.c, Tom St Denis
*/



#if defined(LTC_SHA384) && defined(LTC_SHA512)

const struct ltc_hash_descriptor sha384_desc =
{
    "sha384",
    4,
    48,
    128,

    /* OID */
   { 2, 16, 840, 1, 101, 3, 4, 2, 2,  },
   9,

    &sha384_init,
    &sha512_process,
    &sha384_done,
    &sha384_test,
    NULL
};

/**
   Initialize the hash state
   @param md   The hash state you wish to initialize
   @return CRYPT_OK if successful
*/
int sha384_init(hash_state * md)
{
    LTC_ARGCHK(md != NULL);

    md->sha512.curlen = 0;
    md->sha512.length = 0;
    md->sha512.state[0] = CONST64(0xcbbb9d5dc1059ed8);
    md->sha512.state[1] = CONST64(0x629a292a367cd507);
    md->sha512.state[2] = CONST64(0x9159015a3070dd17);
    md->sha512.state[3] = CONST64(0x152fecd8f70e5939);
    md->sha512.state[4] = CONST64(0x67332667ffc00b31);
    md->sha512.state[5] = CONST64(0x8eb44a8768581511);
    md->sha512.state[6] = CONST64(0xdb0c2e0d64f98fa7);
    md->sha512.state[7] = CONST64(0x47b5481dbefa4fa4);
    return CRYPT_OK;
}

/**
   Terminate the hash to get the digest
   @param md  The hash state
   @param out [out] The destination of the hash (48 bytes)
   @return CRYPT_OK if successful
*/
int sha384_done(hash_state * md, unsigned char *out)
{
   unsigned char buf[64];

   LTC_ARGCHK(md  != NULL);
   LTC_ARGCHK(out != NULL);

    if (md->sha512.curlen >= sizeof(md->sha512.buf)) {
       return CRYPT_INVALID_ARG;
    }

   sha512_done(md, buf);
   XMEMCPY(out, buf, 48);
#ifdef LTC_CLEAN_STACK
   zeromem(buf, sizeof(buf));
#endif
   return CRYPT_OK;
}

/**
  Self-test the hash
  @return CRYPT_OK if successful, CRYPT_NOP if self-tests have been disabled
*/
int  sha384_test(void)
{
 #ifndef LTC_TEST
    return CRYPT_NOP;
 #else
  static const struct {
      char *msg;
      unsigned char hash[48];
  } tests[] = {
    { "abc",
      { 0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b,
        0xb5, 0xa0, 0x3d, 0x69, 0x9a, 0xc6, 0x50, 0x07,
        0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63,
        0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed,
        0x80, 0x86, 0x07, 0x2b, 0xa1, 0xe7, 0xcc, 0x23,
        0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7 }
    },
    { "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
      { 0x09, 0x33, 0x0c, 0x33, 0xf7, 0x11, 0x47, 0xe8,
        0x3d, 0x19, 0x2f, 0xc7, 0x82, 0xcd, 0x1b, 0x47,
        0x53, 0x11, 0x1b, 0x17, 0x3b, 0x3b, 0x05, 0xd2,
        0x2f, 0xa0, 0x80, 0x86, 0xe3, 0xb0, 0xf7, 0x12,
        0xfc, 0xc7, 0xc7, 0x1a, 0x55, 0x7e, 0x2d, 0xb9,
        0x66, 0xc3, 0xe9, 0xfa, 0x91, 0x74, 0x60, 0x39 }
    },
  };

  int i;
  unsigned char tmp[48];
  hash_state md;

  for (i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++) {
      sha384_init(&md);
      sha384_process(&md, (unsigned char*)tests[i].msg, (unsigned long)strlen(tests[i].msg));
      sha384_done(&md, tmp);
      if (XMEMCMP(tmp, tests[i].hash, 48) != 0) {
         return CRYPT_FAIL_TESTVECTOR;
      }
  }
  return CRYPT_OK;
 #endif
}

#endif /* defined(LTC_SHA384) && defined(LTC_SHA512) */

/* $Source$ */
/* $Revision$ */
/* $Date$ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @param sha512.c
   LTC_SHA512 by Tom St Denis
*/

#ifdef LTC_SHA512

const struct ltc_hash_descriptor sha512_desc =
{
    "sha512",
    5,
    64,
    128,

    /* OID */
   { 2, 16, 840, 1, 101, 3, 4, 2, 3,  },
   9,

    &sha512_init,
    &sha512_process,
    &sha512_done,
    &sha512_test,
    NULL
};

/* the K array */
static const ulong64 K[80] = {
CONST64(0x428a2f98d728ae22), CONST64(0x7137449123ef65cd),
CONST64(0xb5c0fbcfec4d3b2f), CONST64(0xe9b5dba58189dbbc),
CONST64(0x3956c25bf348b538), CONST64(0x59f111f1b605d019),
CONST64(0x923f82a4af194f9b), CONST64(0xab1c5ed5da6d8118),
CONST64(0xd807aa98a3030242), CONST64(0x12835b0145706fbe),
CONST64(0x243185be4ee4b28c), CONST64(0x550c7dc3d5ffb4e2),
CONST64(0x72be5d74f27b896f), CONST64(0x80deb1fe3b1696b1),
CONST64(0x9bdc06a725c71235), CONST64(0xc19bf174cf692694),
CONST64(0xe49b69c19ef14ad2), CONST64(0xefbe4786384f25e3),
CONST64(0x0fc19dc68b8cd5b5), CONST64(0x240ca1cc77ac9c65),
CONST64(0x2de92c6f592b0275), CONST64(0x4a7484aa6ea6e483),
CONST64(0x5cb0a9dcbd41fbd4), CONST64(0x76f988da831153b5),
CONST64(0x983e5152ee66dfab), CONST64(0xa831c66d2db43210),
CONST64(0xb00327c898fb213f), CONST64(0xbf597fc7beef0ee4),
CONST64(0xc6e00bf33da88fc2), CONST64(0xd5a79147930aa725),
CONST64(0x06ca6351e003826f), CONST64(0x142929670a0e6e70),
CONST64(0x27b70a8546d22ffc), CONST64(0x2e1b21385c26c926),
CONST64(0x4d2c6dfc5ac42aed), CONST64(0x53380d139d95b3df),
CONST64(0x650a73548baf63de), CONST64(0x766a0abb3c77b2a8),
CONST64(0x81c2c92e47edaee6), CONST64(0x92722c851482353b),
CONST64(0xa2bfe8a14cf10364), CONST64(0xa81a664bbc423001),
CONST64(0xc24b8b70d0f89791), CONST64(0xc76c51a30654be30),
CONST64(0xd192e819d6ef5218), CONST64(0xd69906245565a910),
CONST64(0xf40e35855771202a), CONST64(0x106aa07032bbd1b8),
CONST64(0x19a4c116b8d2d0c8), CONST64(0x1e376c085141ab53),
CONST64(0x2748774cdf8eeb99), CONST64(0x34b0bcb5e19b48a8),
CONST64(0x391c0cb3c5c95a63), CONST64(0x4ed8aa4ae3418acb),
CONST64(0x5b9cca4f7763e373), CONST64(0x682e6ff3d6b2b8a3),
CONST64(0x748f82ee5defb2fc), CONST64(0x78a5636f43172f60),
CONST64(0x84c87814a1f0ab72), CONST64(0x8cc702081a6439ec),
CONST64(0x90befffa23631e28), CONST64(0xa4506cebde82bde9),
CONST64(0xbef9a3f7b2c67915), CONST64(0xc67178f2e372532b),
CONST64(0xca273eceea26619c), CONST64(0xd186b8c721c0c207),
CONST64(0xeada7dd6cde0eb1e), CONST64(0xf57d4f7fee6ed178),
CONST64(0x06f067aa72176fba), CONST64(0x0a637dc5a2c898a6),
CONST64(0x113f9804bef90dae), CONST64(0x1b710b35131c471b),
CONST64(0x28db77f523047d84), CONST64(0x32caab7b40c72493),
CONST64(0x3c9ebe0a15c9bebc), CONST64(0x431d67c49c100d4c),
CONST64(0x4cc5d4becb3e42b6), CONST64(0x597f299cfc657e2a),
CONST64(0x5fcb6fab3ad6faec), CONST64(0x6c44198c4a475817)
};

/* Various logical functions */
#undef S
#undef R
#undef Sigma0
#undef Sigma1
#undef Gamma0
#undef Gamma1

#define Ch(x,y,z)       (z ^ (x & (y ^ z)))
#define Maj(x,y,z)      (((x | y) & z) | (x & y))
#define S(x, n)         ROR64c(x, n)
#define R(x, n)         (((x)&CONST64(0xFFFFFFFFFFFFFFFF))>>((ulong64)n))
#define Sigma0(x)       (S(x, 28) ^ S(x, 34) ^ S(x, 39))
#define Sigma1(x)       (S(x, 14) ^ S(x, 18) ^ S(x, 41))
#define Gamma0(x)       (S(x, 1) ^ S(x, 8) ^ R(x, 7))
#define Gamma1(x)       (S(x, 19) ^ S(x, 61) ^ R(x, 6))

/* compress 1024-bits */
#ifdef LTC_CLEAN_STACK
static int _sha512_compress(hash_state * md, unsigned char *buf)
#else
static int  sha512_compress(hash_state * md, unsigned char *buf)
#endif
{
    ulong64 S[8], W[80], t0, t1;
    int i;

    /* copy state into S */
    for (i = 0; i < 8; i++) {
        S[i] = md->sha512.state[i];
    }

    /* copy the state into 1024-bits into W[0..15] */
    for (i = 0; i < 16; i++) {
        LOAD64H(W[i], buf + (8*i));
    }

    /* fill W[16..79] */
    for (i = 16; i < 80; i++) {
        W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];
    }

    /* Compress */
#ifdef LTC_SMALL_CODE
    for (i = 0; i < 80; i++) {
        t0 = S[7] + Sigma1(S[4]) + Ch(S[4], S[5], S[6]) + K[i] + W[i];
        t1 = Sigma0(S[0]) + Maj(S[0], S[1], S[2]);
        S[7] = S[6];
        S[6] = S[5];
        S[5] = S[4];
        S[4] = S[3] + t0;
        S[3] = S[2];
        S[2] = S[1];
        S[1] = S[0];
        S[0] = t0 + t1;
    }
#else
#define RND(a,b,c,d,e,f,g,h,i)                    \
     t0 = h + Sigma1(e) + Ch(e, f, g) + K[i] + W[i];   \
     t1 = Sigma0(a) + Maj(a, b, c);                  \
     d += t0;                                        \
     h  = t0 + t1;

     for (i = 0; i < 80; i += 8) {
         RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],i+0);
         RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],i+1);
         RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],i+2);
         RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],i+3);
         RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],i+4);
         RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],i+5);
         RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],i+6);
         RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],i+7);
     }
#endif


    /* feedback */
    for (i = 0; i < 8; i++) {
        md->sha512.state[i] = md->sha512.state[i] + S[i];
    }

    return CRYPT_OK;
}

/* compress 1024-bits */
#ifdef LTC_CLEAN_STACK
static int sha512_compress(hash_state * md, unsigned char *buf)
{
    int err;
    err = _sha512_compress(md, buf);
    burn_stack(sizeof(ulong64) * 90 + sizeof(int));
    return err;
}
#endif

/**
   Initialize the hash state
   @param md   The hash state you wish to initialize
   @return CRYPT_OK if successful
*/
int sha512_init(hash_state * md)
{
    LTC_ARGCHK(md != NULL);
    md->sha512.curlen = 0;
    md->sha512.length = 0;
    md->sha512.state[0] = CONST64(0x6a09e667f3bcc908);
    md->sha512.state[1] = CONST64(0xbb67ae8584caa73b);
    md->sha512.state[2] = CONST64(0x3c6ef372fe94f82b);
    md->sha512.state[3] = CONST64(0xa54ff53a5f1d36f1);
    md->sha512.state[4] = CONST64(0x510e527fade682d1);
    md->sha512.state[5] = CONST64(0x9b05688c2b3e6c1f);
    md->sha512.state[6] = CONST64(0x1f83d9abfb41bd6b);
    md->sha512.state[7] = CONST64(0x5be0cd19137e2179);
    return CRYPT_OK;
}

/**
   Process a block of memory though the hash
   @param md     The hash state
   @param in     The data to hash
   @param inlen  The length of the data (octets)
   @return CRYPT_OK if successful
*/
HASH_PROCESS(sha512_process, sha512_compress, sha512, 128)

/**
   Terminate the hash to get the digest
   @param md  The hash state
   @param out [out] The destination of the hash (64 bytes)
   @return CRYPT_OK if successful
*/
int sha512_done(hash_state * md, unsigned char *out)
{
    int i;

    LTC_ARGCHK(md  != NULL);
    LTC_ARGCHK(out != NULL);

    if (md->sha512.curlen >= sizeof(md->sha512.buf)) {
       return CRYPT_INVALID_ARG;
    }

    /* increase the length of the message */
    md->sha512.length += md->sha512.curlen * CONST64(8);

    /* append the '1' bit */
    md->sha512.buf[md->sha512.curlen++] = (unsigned char)0x80;

    /* if the length is currently above 112 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (md->sha512.curlen > 112) {
        while (md->sha512.curlen < 128) {
            md->sha512.buf[md->sha512.curlen++] = (unsigned char)0;
        }
        sha512_compress(md, md->sha512.buf);
        md->sha512.curlen = 0;
    }

    /* pad upto 120 bytes of zeroes
     * note: that from 112 to 120 is the 64 MSB of the length.  We assume that you won't hash
     * > 2^64 bits of data... :-)
     */
    while (md->sha512.curlen < 120) {
        md->sha512.buf[md->sha512.curlen++] = (unsigned char)0;
    }

    /* store length */
    STORE64H(md->sha512.length, md->sha512.buf+120);
    sha512_compress(md, md->sha512.buf);

    /* copy output */
    for (i = 0; i < 8; i++) {
        STORE64H(md->sha512.state[i], out+(8*i));
    }
#ifdef LTC_CLEAN_STACK
    zeromem(md, sizeof(hash_state));
#endif
    return CRYPT_OK;
}

/**
  Self-test the hash
  @return CRYPT_OK if successful, CRYPT_NOP if self-tests have been disabled
*/
int  sha512_test(void)
{
 #ifndef LTC_TEST
    return CRYPT_NOP;
 #else
  static const struct {
      char *msg;
      unsigned char hash[64];
  } tests[] = {
    { "abc",
     { 0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba,
       0xcc, 0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31,
       0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2,
       0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a,
       0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8,
       0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
       0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e,
       0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f }
    },
    { "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
     { 0x8e, 0x95, 0x9b, 0x75, 0xda, 0xe3, 0x13, 0xda,
       0x8c, 0xf4, 0xf7, 0x28, 0x14, 0xfc, 0x14, 0x3f,
       0x8f, 0x77, 0x79, 0xc6, 0xeb, 0x9f, 0x7f, 0xa1,
       0x72, 0x99, 0xae, 0xad, 0xb6, 0x88, 0x90, 0x18,
       0x50, 0x1d, 0x28, 0x9e, 0x49, 0x00, 0xf7, 0xe4,
       0x33, 0x1b, 0x99, 0xde, 0xc4, 0xb5, 0x43, 0x3a,
       0xc7, 0xd3, 0x29, 0xee, 0xb6, 0xdd, 0x26, 0x54,
       0x5e, 0x96, 0xe5, 0x5b, 0x87, 0x4b, 0xe9, 0x09 }
    },
  };

  int i;
  unsigned char tmp[64];
  hash_state md;

  for (i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++) {
      sha512_init(&md);
      sha512_process(&md, (unsigned char *)tests[i].msg, (unsigned long)strlen(tests[i].msg));
      sha512_done(&md, tmp);
      if (XMEMCMP(tmp, tests[i].hash, 64) != 0) {
         return CRYPT_FAIL_TESTVECTOR;
      }
  }
  return CRYPT_OK;
  #endif
}

#endif




/* $Source$ */
/* $Revision$ */
/* $Date$ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
  @file hmac_init.c
  HMAC support, initialize state, Tom St Denis/Dobes Vandermeer
*/

#ifdef LTC_HMAC

#define LTC_HMAC_BLOCKSIZE hash_descriptor[hash].blocksize

/**
   Initialize an HMAC context.
   @param hmac     The HMAC state
   @param hash     The index of the hash you want to use
   @param key      The secret key
   @param keylen   The length of the secret key (octets)
   @return CRYPT_OK if successful
*/
int hmac_init(hmac_state *hmac, int hash, const unsigned char *key, unsigned long keylen)
{
    unsigned char *buf;
    unsigned long hashsize;
    unsigned long i, z;
    int err;

    LTC_ARGCHK(hmac != NULL);
    LTC_ARGCHK(key  != NULL);

    /* valid hash? */
    if ((err = hash_is_valid(hash)) != CRYPT_OK) {
        return err;
    }
    hmac->hash = hash;
    hashsize   = hash_descriptor[hash].hashsize;

    /* valid key length? */
    if (keylen == 0) {
        return CRYPT_INVALID_KEYSIZE;
    }

    /* allocate ram for buf */
    buf = XMALLOC(LTC_HMAC_BLOCKSIZE);
    if (buf == NULL) {
       return CRYPT_MEM;
    }

    /* allocate memory for key */
    hmac->key = XMALLOC(LTC_HMAC_BLOCKSIZE);
    if (hmac->key == NULL) {
       XFREE(buf);
       return CRYPT_MEM;
    }

    /* (1) make sure we have a large enough key */
    if(keylen > LTC_HMAC_BLOCKSIZE) {
        z = LTC_HMAC_BLOCKSIZE;
        if ((err = hash_memory(hash, key, keylen, hmac->key, &z)) != CRYPT_OK) {
           goto LBL_ERR;
        }
        keylen = hashsize;
    } else {
        XMEMCPY(hmac->key, key, (size_t)keylen);
    }

    if(keylen < LTC_HMAC_BLOCKSIZE) {
       zeromem((hmac->key) + keylen, (size_t)(LTC_HMAC_BLOCKSIZE - keylen));
    }

    /* Create the initial vector for step (3) */
    for(i=0; i < LTC_HMAC_BLOCKSIZE;   i++) {
       buf[i] = hmac->key[i] ^ 0x36;
    }

    /* Pre-pend that to the hash data */
    if ((err = hash_descriptor[hash].init(&hmac->md)) != CRYPT_OK) {
       goto LBL_ERR;
    }

    if ((err = hash_descriptor[hash].process(&hmac->md, buf, LTC_HMAC_BLOCKSIZE)) != CRYPT_OK) {
       goto LBL_ERR;
    }
    goto done;
LBL_ERR:
    /* free the key since we failed */
    XFREE(hmac->key);
done:
#ifdef LTC_CLEAN_STACK
   zeromem(buf, LTC_HMAC_BLOCKSIZE);
#endif

   XFREE(buf);
   return err;
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
  @file hmac_process.c
  HMAC support, process data, Tom St Denis/Dobes Vandermeer
*/

#ifdef LTC_HMAC

/**
  Process data through HMAC
  @param hmac    The hmac state
  @param in      The data to send through HMAC
  @param inlen   The length of the data to HMAC (octets)
  @return CRYPT_OK if successful
*/
int hmac_process(hmac_state *hmac, const unsigned char *in, unsigned long inlen)
{
    int err;
    LTC_ARGCHK(hmac != NULL);
    LTC_ARGCHK(in != NULL);
    if ((err = hash_is_valid(hmac->hash)) != CRYPT_OK) {
        return err;
    }
    return hash_descriptor[hmac->hash].process(&hmac->md, in, inlen);
}

#endif


/* $Source$ */
/* $Revision$ */
/* $Date$ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
  @file hmac_done.c
  HMAC support, terminate stream, Tom St Denis/Dobes Vandermeer
*/

#ifdef LTC_HMAC

#define LTC_HMAC_BLOCKSIZE hash_descriptor[hash].blocksize

/**
   Terminate an HMAC session
   @param hmac    The HMAC state
   @param out     [out] The destination of the HMAC authentication tag
   @param outlen  [in/out]  The max size and resulting size of the HMAC authentication tag
   @return CRYPT_OK if successful
*/
int hmac_done(hmac_state *hmac, unsigned char *out, unsigned long *outlen)
{
    unsigned char *buf, *isha;
    unsigned long hashsize, i;
    int hash, err;

    LTC_ARGCHK(hmac  != NULL);
    LTC_ARGCHK(out   != NULL);

    /* test hash */
    hash = hmac->hash;
    if((err = hash_is_valid(hash)) != CRYPT_OK) {
        return err;
    }

    /* get the hash message digest size */
    hashsize = hash_descriptor[hash].hashsize;

    /* allocate buffers */
    buf  = XMALLOC(LTC_HMAC_BLOCKSIZE);
    isha = XMALLOC(hashsize);
    if (buf == NULL || isha == NULL) {
       if (buf != NULL) {
          XFREE(buf);
       }
       if (isha != NULL) {
          XFREE(isha);
       }
       return CRYPT_MEM;
    }

    /* Get the hash of the first HMAC vector plus the data */
    if ((err = hash_descriptor[hash].done(&hmac->md, isha)) != CRYPT_OK) {
       goto LBL_ERR;
    }

    /* Create the second HMAC vector vector for step (3) */
    for(i=0; i < LTC_HMAC_BLOCKSIZE; i++) {
        buf[i] = hmac->key[i] ^ 0x5C;
    }

    /* Now calculate the "outer" hash for step (5), (6), and (7) */
    if ((err = hash_descriptor[hash].init(&hmac->md)) != CRYPT_OK) {
       goto LBL_ERR;
    }
    if ((err = hash_descriptor[hash].process(&hmac->md, buf, LTC_HMAC_BLOCKSIZE)) != CRYPT_OK) {
       goto LBL_ERR;
    }
    if ((err = hash_descriptor[hash].process(&hmac->md, isha, hashsize)) != CRYPT_OK) {
       goto LBL_ERR;
    }
    if ((err = hash_descriptor[hash].done(&hmac->md, buf)) != CRYPT_OK) {
       goto LBL_ERR;
    }

    /* copy to output  */
    for (i = 0; i < hashsize && i < *outlen; i++) {
        out[i] = buf[i];
    }
    *outlen = i;

    err = CRYPT_OK;
LBL_ERR:
    XFREE(hmac->key);
#ifdef LTC_CLEAN_STACK
    zeromem(isha, hashsize);
    zeromem(buf,  hashsize);
    zeromem(hmac, sizeof(*hmac));
#endif

    XFREE(isha);
    XFREE(buf);

    return err;
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */


#define __LTC_AES_TAB_C__
/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */
/* The precomputed tables for AES */
/*
Te0[x] = S [x].[02, 01, 01, 03];
Te1[x] = S [x].[03, 02, 01, 01];
Te2[x] = S [x].[01, 03, 02, 01];
Te3[x] = S [x].[01, 01, 03, 02];
Te4[x] = S [x].[01, 01, 01, 01];

Td0[x] = Si[x].[0e, 09, 0d, 0b];
Td1[x] = Si[x].[0b, 0e, 09, 0d];
Td2[x] = Si[x].[0d, 0b, 0e, 09];
Td3[x] = Si[x].[09, 0d, 0b, 0e];
Td4[x] = Si[x].[01, 01, 01, 01];
*/

#ifdef __LTC_AES_TAB_C__

/**
  @file aes_tab.c
  AES tables
*/
static const ulong32 TE0[256] = {
    0xc66363a5UL, 0xf87c7c84UL, 0xee777799UL, 0xf67b7b8dUL,
    0xfff2f20dUL, 0xd66b6bbdUL, 0xde6f6fb1UL, 0x91c5c554UL,
    0x60303050UL, 0x02010103UL, 0xce6767a9UL, 0x562b2b7dUL,
    0xe7fefe19UL, 0xb5d7d762UL, 0x4dababe6UL, 0xec76769aUL,
    0x8fcaca45UL, 0x1f82829dUL, 0x89c9c940UL, 0xfa7d7d87UL,
    0xeffafa15UL, 0xb25959ebUL, 0x8e4747c9UL, 0xfbf0f00bUL,
    0x41adadecUL, 0xb3d4d467UL, 0x5fa2a2fdUL, 0x45afafeaUL,
    0x239c9cbfUL, 0x53a4a4f7UL, 0xe4727296UL, 0x9bc0c05bUL,
    0x75b7b7c2UL, 0xe1fdfd1cUL, 0x3d9393aeUL, 0x4c26266aUL,
    0x6c36365aUL, 0x7e3f3f41UL, 0xf5f7f702UL, 0x83cccc4fUL,
    0x6834345cUL, 0x51a5a5f4UL, 0xd1e5e534UL, 0xf9f1f108UL,
    0xe2717193UL, 0xabd8d873UL, 0x62313153UL, 0x2a15153fUL,
    0x0804040cUL, 0x95c7c752UL, 0x46232365UL, 0x9dc3c35eUL,
    0x30181828UL, 0x379696a1UL, 0x0a05050fUL, 0x2f9a9ab5UL,
    0x0e070709UL, 0x24121236UL, 0x1b80809bUL, 0xdfe2e23dUL,
    0xcdebeb26UL, 0x4e272769UL, 0x7fb2b2cdUL, 0xea75759fUL,
    0x1209091bUL, 0x1d83839eUL, 0x582c2c74UL, 0x341a1a2eUL,
    0x361b1b2dUL, 0xdc6e6eb2UL, 0xb45a5aeeUL, 0x5ba0a0fbUL,
    0xa45252f6UL, 0x763b3b4dUL, 0xb7d6d661UL, 0x7db3b3ceUL,
    0x5229297bUL, 0xdde3e33eUL, 0x5e2f2f71UL, 0x13848497UL,
    0xa65353f5UL, 0xb9d1d168UL, 0x00000000UL, 0xc1eded2cUL,
    0x40202060UL, 0xe3fcfc1fUL, 0x79b1b1c8UL, 0xb65b5bedUL,
    0xd46a6abeUL, 0x8dcbcb46UL, 0x67bebed9UL, 0x7239394bUL,
    0x944a4adeUL, 0x984c4cd4UL, 0xb05858e8UL, 0x85cfcf4aUL,
    0xbbd0d06bUL, 0xc5efef2aUL, 0x4faaaae5UL, 0xedfbfb16UL,
    0x864343c5UL, 0x9a4d4dd7UL, 0x66333355UL, 0x11858594UL,
    0x8a4545cfUL, 0xe9f9f910UL, 0x04020206UL, 0xfe7f7f81UL,
    0xa05050f0UL, 0x783c3c44UL, 0x259f9fbaUL, 0x4ba8a8e3UL,
    0xa25151f3UL, 0x5da3a3feUL, 0x804040c0UL, 0x058f8f8aUL,
    0x3f9292adUL, 0x219d9dbcUL, 0x70383848UL, 0xf1f5f504UL,
    0x63bcbcdfUL, 0x77b6b6c1UL, 0xafdada75UL, 0x42212163UL,
    0x20101030UL, 0xe5ffff1aUL, 0xfdf3f30eUL, 0xbfd2d26dUL,
    0x81cdcd4cUL, 0x180c0c14UL, 0x26131335UL, 0xc3ecec2fUL,
    0xbe5f5fe1UL, 0x359797a2UL, 0x884444ccUL, 0x2e171739UL,
    0x93c4c457UL, 0x55a7a7f2UL, 0xfc7e7e82UL, 0x7a3d3d47UL,
    0xc86464acUL, 0xba5d5de7UL, 0x3219192bUL, 0xe6737395UL,
    0xc06060a0UL, 0x19818198UL, 0x9e4f4fd1UL, 0xa3dcdc7fUL,
    0x44222266UL, 0x542a2a7eUL, 0x3b9090abUL, 0x0b888883UL,
    0x8c4646caUL, 0xc7eeee29UL, 0x6bb8b8d3UL, 0x2814143cUL,
    0xa7dede79UL, 0xbc5e5ee2UL, 0x160b0b1dUL, 0xaddbdb76UL,
    0xdbe0e03bUL, 0x64323256UL, 0x743a3a4eUL, 0x140a0a1eUL,
    0x924949dbUL, 0x0c06060aUL, 0x4824246cUL, 0xb85c5ce4UL,
    0x9fc2c25dUL, 0xbdd3d36eUL, 0x43acacefUL, 0xc46262a6UL,
    0x399191a8UL, 0x319595a4UL, 0xd3e4e437UL, 0xf279798bUL,
    0xd5e7e732UL, 0x8bc8c843UL, 0x6e373759UL, 0xda6d6db7UL,
    0x018d8d8cUL, 0xb1d5d564UL, 0x9c4e4ed2UL, 0x49a9a9e0UL,
    0xd86c6cb4UL, 0xac5656faUL, 0xf3f4f407UL, 0xcfeaea25UL,
    0xca6565afUL, 0xf47a7a8eUL, 0x47aeaee9UL, 0x10080818UL,
    0x6fbabad5UL, 0xf0787888UL, 0x4a25256fUL, 0x5c2e2e72UL,
    0x381c1c24UL, 0x57a6a6f1UL, 0x73b4b4c7UL, 0x97c6c651UL,
    0xcbe8e823UL, 0xa1dddd7cUL, 0xe874749cUL, 0x3e1f1f21UL,
    0x964b4bddUL, 0x61bdbddcUL, 0x0d8b8b86UL, 0x0f8a8a85UL,
    0xe0707090UL, 0x7c3e3e42UL, 0x71b5b5c4UL, 0xcc6666aaUL,
    0x904848d8UL, 0x06030305UL, 0xf7f6f601UL, 0x1c0e0e12UL,
    0xc26161a3UL, 0x6a35355fUL, 0xae5757f9UL, 0x69b9b9d0UL,
    0x17868691UL, 0x99c1c158UL, 0x3a1d1d27UL, 0x279e9eb9UL,
    0xd9e1e138UL, 0xebf8f813UL, 0x2b9898b3UL, 0x22111133UL,
    0xd26969bbUL, 0xa9d9d970UL, 0x078e8e89UL, 0x339494a7UL,
    0x2d9b9bb6UL, 0x3c1e1e22UL, 0x15878792UL, 0xc9e9e920UL,
    0x87cece49UL, 0xaa5555ffUL, 0x50282878UL, 0xa5dfdf7aUL,
    0x038c8c8fUL, 0x59a1a1f8UL, 0x09898980UL, 0x1a0d0d17UL,
    0x65bfbfdaUL, 0xd7e6e631UL, 0x844242c6UL, 0xd06868b8UL,
    0x824141c3UL, 0x299999b0UL, 0x5a2d2d77UL, 0x1e0f0f11UL,
    0x7bb0b0cbUL, 0xa85454fcUL, 0x6dbbbbd6UL, 0x2c16163aUL,
};

#ifndef PELI_TAB
static const ulong32 Te4[256] = {
    0x63636363UL, 0x7c7c7c7cUL, 0x77777777UL, 0x7b7b7b7bUL,
    0xf2f2f2f2UL, 0x6b6b6b6bUL, 0x6f6f6f6fUL, 0xc5c5c5c5UL,
    0x30303030UL, 0x01010101UL, 0x67676767UL, 0x2b2b2b2bUL,
    0xfefefefeUL, 0xd7d7d7d7UL, 0xababababUL, 0x76767676UL,
    0xcacacacaUL, 0x82828282UL, 0xc9c9c9c9UL, 0x7d7d7d7dUL,
    0xfafafafaUL, 0x59595959UL, 0x47474747UL, 0xf0f0f0f0UL,
    0xadadadadUL, 0xd4d4d4d4UL, 0xa2a2a2a2UL, 0xafafafafUL,
    0x9c9c9c9cUL, 0xa4a4a4a4UL, 0x72727272UL, 0xc0c0c0c0UL,
    0xb7b7b7b7UL, 0xfdfdfdfdUL, 0x93939393UL, 0x26262626UL,
    0x36363636UL, 0x3f3f3f3fUL, 0xf7f7f7f7UL, 0xccccccccUL,
    0x34343434UL, 0xa5a5a5a5UL, 0xe5e5e5e5UL, 0xf1f1f1f1UL,
    0x71717171UL, 0xd8d8d8d8UL, 0x31313131UL, 0x15151515UL,
    0x04040404UL, 0xc7c7c7c7UL, 0x23232323UL, 0xc3c3c3c3UL,
    0x18181818UL, 0x96969696UL, 0x05050505UL, 0x9a9a9a9aUL,
    0x07070707UL, 0x12121212UL, 0x80808080UL, 0xe2e2e2e2UL,
    0xebebebebUL, 0x27272727UL, 0xb2b2b2b2UL, 0x75757575UL,
    0x09090909UL, 0x83838383UL, 0x2c2c2c2cUL, 0x1a1a1a1aUL,
    0x1b1b1b1bUL, 0x6e6e6e6eUL, 0x5a5a5a5aUL, 0xa0a0a0a0UL,
    0x52525252UL, 0x3b3b3b3bUL, 0xd6d6d6d6UL, 0xb3b3b3b3UL,
    0x29292929UL, 0xe3e3e3e3UL, 0x2f2f2f2fUL, 0x84848484UL,
    0x53535353UL, 0xd1d1d1d1UL, 0x00000000UL, 0xededededUL,
    0x20202020UL, 0xfcfcfcfcUL, 0xb1b1b1b1UL, 0x5b5b5b5bUL,
    0x6a6a6a6aUL, 0xcbcbcbcbUL, 0xbebebebeUL, 0x39393939UL,
    0x4a4a4a4aUL, 0x4c4c4c4cUL, 0x58585858UL, 0xcfcfcfcfUL,
    0xd0d0d0d0UL, 0xefefefefUL, 0xaaaaaaaaUL, 0xfbfbfbfbUL,
    0x43434343UL, 0x4d4d4d4dUL, 0x33333333UL, 0x85858585UL,
    0x45454545UL, 0xf9f9f9f9UL, 0x02020202UL, 0x7f7f7f7fUL,
    0x50505050UL, 0x3c3c3c3cUL, 0x9f9f9f9fUL, 0xa8a8a8a8UL,
    0x51515151UL, 0xa3a3a3a3UL, 0x40404040UL, 0x8f8f8f8fUL,
    0x92929292UL, 0x9d9d9d9dUL, 0x38383838UL, 0xf5f5f5f5UL,
    0xbcbcbcbcUL, 0xb6b6b6b6UL, 0xdadadadaUL, 0x21212121UL,
    0x10101010UL, 0xffffffffUL, 0xf3f3f3f3UL, 0xd2d2d2d2UL,
    0xcdcdcdcdUL, 0x0c0c0c0cUL, 0x13131313UL, 0xececececUL,
    0x5f5f5f5fUL, 0x97979797UL, 0x44444444UL, 0x17171717UL,
    0xc4c4c4c4UL, 0xa7a7a7a7UL, 0x7e7e7e7eUL, 0x3d3d3d3dUL,
    0x64646464UL, 0x5d5d5d5dUL, 0x19191919UL, 0x73737373UL,
    0x60606060UL, 0x81818181UL, 0x4f4f4f4fUL, 0xdcdcdcdcUL,
    0x22222222UL, 0x2a2a2a2aUL, 0x90909090UL, 0x88888888UL,
    0x46464646UL, 0xeeeeeeeeUL, 0xb8b8b8b8UL, 0x14141414UL,
    0xdedededeUL, 0x5e5e5e5eUL, 0x0b0b0b0bUL, 0xdbdbdbdbUL,
    0xe0e0e0e0UL, 0x32323232UL, 0x3a3a3a3aUL, 0x0a0a0a0aUL,
    0x49494949UL, 0x06060606UL, 0x24242424UL, 0x5c5c5c5cUL,
    0xc2c2c2c2UL, 0xd3d3d3d3UL, 0xacacacacUL, 0x62626262UL,
    0x91919191UL, 0x95959595UL, 0xe4e4e4e4UL, 0x79797979UL,
    0xe7e7e7e7UL, 0xc8c8c8c8UL, 0x37373737UL, 0x6d6d6d6dUL,
    0x8d8d8d8dUL, 0xd5d5d5d5UL, 0x4e4e4e4eUL, 0xa9a9a9a9UL,
    0x6c6c6c6cUL, 0x56565656UL, 0xf4f4f4f4UL, 0xeaeaeaeaUL,
    0x65656565UL, 0x7a7a7a7aUL, 0xaeaeaeaeUL, 0x08080808UL,
    0xbabababaUL, 0x78787878UL, 0x25252525UL, 0x2e2e2e2eUL,
    0x1c1c1c1cUL, 0xa6a6a6a6UL, 0xb4b4b4b4UL, 0xc6c6c6c6UL,
    0xe8e8e8e8UL, 0xddddddddUL, 0x74747474UL, 0x1f1f1f1fUL,
    0x4b4b4b4bUL, 0xbdbdbdbdUL, 0x8b8b8b8bUL, 0x8a8a8a8aUL,
    0x70707070UL, 0x3e3e3e3eUL, 0xb5b5b5b5UL, 0x66666666UL,
    0x48484848UL, 0x03030303UL, 0xf6f6f6f6UL, 0x0e0e0e0eUL,
    0x61616161UL, 0x35353535UL, 0x57575757UL, 0xb9b9b9b9UL,
    0x86868686UL, 0xc1c1c1c1UL, 0x1d1d1d1dUL, 0x9e9e9e9eUL,
    0xe1e1e1e1UL, 0xf8f8f8f8UL, 0x98989898UL, 0x11111111UL,
    0x69696969UL, 0xd9d9d9d9UL, 0x8e8e8e8eUL, 0x94949494UL,
    0x9b9b9b9bUL, 0x1e1e1e1eUL, 0x87878787UL, 0xe9e9e9e9UL,
    0xcecececeUL, 0x55555555UL, 0x28282828UL, 0xdfdfdfdfUL,
    0x8c8c8c8cUL, 0xa1a1a1a1UL, 0x89898989UL, 0x0d0d0d0dUL,
    0xbfbfbfbfUL, 0xe6e6e6e6UL, 0x42424242UL, 0x68686868UL,
    0x41414141UL, 0x99999999UL, 0x2d2d2d2dUL, 0x0f0f0f0fUL,
    0xb0b0b0b0UL, 0x54545454UL, 0xbbbbbbbbUL, 0x16161616UL,
};
#endif

#ifndef ENCRYPT_ONLY

static const ulong32 TD0[256] = {
    0x51f4a750UL, 0x7e416553UL, 0x1a17a4c3UL, 0x3a275e96UL,
    0x3bab6bcbUL, 0x1f9d45f1UL, 0xacfa58abUL, 0x4be30393UL,
    0x2030fa55UL, 0xad766df6UL, 0x88cc7691UL, 0xf5024c25UL,
    0x4fe5d7fcUL, 0xc52acbd7UL, 0x26354480UL, 0xb562a38fUL,
    0xdeb15a49UL, 0x25ba1b67UL, 0x45ea0e98UL, 0x5dfec0e1UL,
    0xc32f7502UL, 0x814cf012UL, 0x8d4697a3UL, 0x6bd3f9c6UL,
    0x038f5fe7UL, 0x15929c95UL, 0xbf6d7aebUL, 0x955259daUL,
    0xd4be832dUL, 0x587421d3UL, 0x49e06929UL, 0x8ec9c844UL,
    0x75c2896aUL, 0xf48e7978UL, 0x99583e6bUL, 0x27b971ddUL,
    0xbee14fb6UL, 0xf088ad17UL, 0xc920ac66UL, 0x7dce3ab4UL,
    0x63df4a18UL, 0xe51a3182UL, 0x97513360UL, 0x62537f45UL,
    0xb16477e0UL, 0xbb6bae84UL, 0xfe81a01cUL, 0xf9082b94UL,
    0x70486858UL, 0x8f45fd19UL, 0x94de6c87UL, 0x527bf8b7UL,
    0xab73d323UL, 0x724b02e2UL, 0xe31f8f57UL, 0x6655ab2aUL,
    0xb2eb2807UL, 0x2fb5c203UL, 0x86c57b9aUL, 0xd33708a5UL,
    0x302887f2UL, 0x23bfa5b2UL, 0x02036abaUL, 0xed16825cUL,
    0x8acf1c2bUL, 0xa779b492UL, 0xf307f2f0UL, 0x4e69e2a1UL,
    0x65daf4cdUL, 0x0605bed5UL, 0xd134621fUL, 0xc4a6fe8aUL,
    0x342e539dUL, 0xa2f355a0UL, 0x058ae132UL, 0xa4f6eb75UL,
    0x0b83ec39UL, 0x4060efaaUL, 0x5e719f06UL, 0xbd6e1051UL,
    0x3e218af9UL, 0x96dd063dUL, 0xdd3e05aeUL, 0x4de6bd46UL,
    0x91548db5UL, 0x71c45d05UL, 0x0406d46fUL, 0x605015ffUL,
    0x1998fb24UL, 0xd6bde997UL, 0x894043ccUL, 0x67d99e77UL,
    0xb0e842bdUL, 0x07898b88UL, 0xe7195b38UL, 0x79c8eedbUL,
    0xa17c0a47UL, 0x7c420fe9UL, 0xf8841ec9UL, 0x00000000UL,
    0x09808683UL, 0x322bed48UL, 0x1e1170acUL, 0x6c5a724eUL,
    0xfd0efffbUL, 0x0f853856UL, 0x3daed51eUL, 0x362d3927UL,
    0x0a0fd964UL, 0x685ca621UL, 0x9b5b54d1UL, 0x24362e3aUL,
    0x0c0a67b1UL, 0x9357e70fUL, 0xb4ee96d2UL, 0x1b9b919eUL,
    0x80c0c54fUL, 0x61dc20a2UL, 0x5a774b69UL, 0x1c121a16UL,
    0xe293ba0aUL, 0xc0a02ae5UL, 0x3c22e043UL, 0x121b171dUL,
    0x0e090d0bUL, 0xf28bc7adUL, 0x2db6a8b9UL, 0x141ea9c8UL,
    0x57f11985UL, 0xaf75074cUL, 0xee99ddbbUL, 0xa37f60fdUL,
    0xf701269fUL, 0x5c72f5bcUL, 0x44663bc5UL, 0x5bfb7e34UL,
    0x8b432976UL, 0xcb23c6dcUL, 0xb6edfc68UL, 0xb8e4f163UL,
    0xd731dccaUL, 0x42638510UL, 0x13972240UL, 0x84c61120UL,
    0x854a247dUL, 0xd2bb3df8UL, 0xaef93211UL, 0xc729a16dUL,
    0x1d9e2f4bUL, 0xdcb230f3UL, 0x0d8652ecUL, 0x77c1e3d0UL,
    0x2bb3166cUL, 0xa970b999UL, 0x119448faUL, 0x47e96422UL,
    0xa8fc8cc4UL, 0xa0f03f1aUL, 0x567d2cd8UL, 0x223390efUL,
    0x87494ec7UL, 0xd938d1c1UL, 0x8ccaa2feUL, 0x98d40b36UL,
    0xa6f581cfUL, 0xa57ade28UL, 0xdab78e26UL, 0x3fadbfa4UL,
    0x2c3a9de4UL, 0x5078920dUL, 0x6a5fcc9bUL, 0x547e4662UL,
    0xf68d13c2UL, 0x90d8b8e8UL, 0x2e39f75eUL, 0x82c3aff5UL,
    0x9f5d80beUL, 0x69d0937cUL, 0x6fd52da9UL, 0xcf2512b3UL,
    0xc8ac993bUL, 0x10187da7UL, 0xe89c636eUL, 0xdb3bbb7bUL,
    0xcd267809UL, 0x6e5918f4UL, 0xec9ab701UL, 0x834f9aa8UL,
    0xe6956e65UL, 0xaaffe67eUL, 0x21bccf08UL, 0xef15e8e6UL,
    0xbae79bd9UL, 0x4a6f36ceUL, 0xea9f09d4UL, 0x29b07cd6UL,
    0x31a4b2afUL, 0x2a3f2331UL, 0xc6a59430UL, 0x35a266c0UL,
    0x744ebc37UL, 0xfc82caa6UL, 0xe090d0b0UL, 0x33a7d815UL,
    0xf104984aUL, 0x41ecdaf7UL, 0x7fcd500eUL, 0x1791f62fUL,
    0x764dd68dUL, 0x43efb04dUL, 0xccaa4d54UL, 0xe49604dfUL,
    0x9ed1b5e3UL, 0x4c6a881bUL, 0xc12c1fb8UL, 0x4665517fUL,
    0x9d5eea04UL, 0x018c355dUL, 0xfa877473UL, 0xfb0b412eUL,
    0xb3671d5aUL, 0x92dbd252UL, 0xe9105633UL, 0x6dd64713UL,
    0x9ad7618cUL, 0x37a10c7aUL, 0x59f8148eUL, 0xeb133c89UL,
    0xcea927eeUL, 0xb761c935UL, 0xe11ce5edUL, 0x7a47b13cUL,
    0x9cd2df59UL, 0x55f2733fUL, 0x1814ce79UL, 0x73c737bfUL,
    0x53f7cdeaUL, 0x5ffdaa5bUL, 0xdf3d6f14UL, 0x7844db86UL,
    0xcaaff381UL, 0xb968c43eUL, 0x3824342cUL, 0xc2a3405fUL,
    0x161dc372UL, 0xbce2250cUL, 0x283c498bUL, 0xff0d9541UL,
    0x39a80171UL, 0x080cb3deUL, 0xd8b4e49cUL, 0x6456c190UL,
    0x7bcb8461UL, 0xd532b670UL, 0x486c5c74UL, 0xd0b85742UL,
};

static const ulong32 Td4[256] = {
    0x52525252UL, 0x09090909UL, 0x6a6a6a6aUL, 0xd5d5d5d5UL,
    0x30303030UL, 0x36363636UL, 0xa5a5a5a5UL, 0x38383838UL,
    0xbfbfbfbfUL, 0x40404040UL, 0xa3a3a3a3UL, 0x9e9e9e9eUL,
    0x81818181UL, 0xf3f3f3f3UL, 0xd7d7d7d7UL, 0xfbfbfbfbUL,
    0x7c7c7c7cUL, 0xe3e3e3e3UL, 0x39393939UL, 0x82828282UL,
    0x9b9b9b9bUL, 0x2f2f2f2fUL, 0xffffffffUL, 0x87878787UL,
    0x34343434UL, 0x8e8e8e8eUL, 0x43434343UL, 0x44444444UL,
    0xc4c4c4c4UL, 0xdedededeUL, 0xe9e9e9e9UL, 0xcbcbcbcbUL,
    0x54545454UL, 0x7b7b7b7bUL, 0x94949494UL, 0x32323232UL,
    0xa6a6a6a6UL, 0xc2c2c2c2UL, 0x23232323UL, 0x3d3d3d3dUL,
    0xeeeeeeeeUL, 0x4c4c4c4cUL, 0x95959595UL, 0x0b0b0b0bUL,
    0x42424242UL, 0xfafafafaUL, 0xc3c3c3c3UL, 0x4e4e4e4eUL,
    0x08080808UL, 0x2e2e2e2eUL, 0xa1a1a1a1UL, 0x66666666UL,
    0x28282828UL, 0xd9d9d9d9UL, 0x24242424UL, 0xb2b2b2b2UL,
    0x76767676UL, 0x5b5b5b5bUL, 0xa2a2a2a2UL, 0x49494949UL,
    0x6d6d6d6dUL, 0x8b8b8b8bUL, 0xd1d1d1d1UL, 0x25252525UL,
    0x72727272UL, 0xf8f8f8f8UL, 0xf6f6f6f6UL, 0x64646464UL,
    0x86868686UL, 0x68686868UL, 0x98989898UL, 0x16161616UL,
    0xd4d4d4d4UL, 0xa4a4a4a4UL, 0x5c5c5c5cUL, 0xccccccccUL,
    0x5d5d5d5dUL, 0x65656565UL, 0xb6b6b6b6UL, 0x92929292UL,
    0x6c6c6c6cUL, 0x70707070UL, 0x48484848UL, 0x50505050UL,
    0xfdfdfdfdUL, 0xededededUL, 0xb9b9b9b9UL, 0xdadadadaUL,
    0x5e5e5e5eUL, 0x15151515UL, 0x46464646UL, 0x57575757UL,
    0xa7a7a7a7UL, 0x8d8d8d8dUL, 0x9d9d9d9dUL, 0x84848484UL,
    0x90909090UL, 0xd8d8d8d8UL, 0xababababUL, 0x00000000UL,
    0x8c8c8c8cUL, 0xbcbcbcbcUL, 0xd3d3d3d3UL, 0x0a0a0a0aUL,
    0xf7f7f7f7UL, 0xe4e4e4e4UL, 0x58585858UL, 0x05050505UL,
    0xb8b8b8b8UL, 0xb3b3b3b3UL, 0x45454545UL, 0x06060606UL,
    0xd0d0d0d0UL, 0x2c2c2c2cUL, 0x1e1e1e1eUL, 0x8f8f8f8fUL,
    0xcacacacaUL, 0x3f3f3f3fUL, 0x0f0f0f0fUL, 0x02020202UL,
    0xc1c1c1c1UL, 0xafafafafUL, 0xbdbdbdbdUL, 0x03030303UL,
    0x01010101UL, 0x13131313UL, 0x8a8a8a8aUL, 0x6b6b6b6bUL,
    0x3a3a3a3aUL, 0x91919191UL, 0x11111111UL, 0x41414141UL,
    0x4f4f4f4fUL, 0x67676767UL, 0xdcdcdcdcUL, 0xeaeaeaeaUL,
    0x97979797UL, 0xf2f2f2f2UL, 0xcfcfcfcfUL, 0xcecececeUL,
    0xf0f0f0f0UL, 0xb4b4b4b4UL, 0xe6e6e6e6UL, 0x73737373UL,
    0x96969696UL, 0xacacacacUL, 0x74747474UL, 0x22222222UL,
    0xe7e7e7e7UL, 0xadadadadUL, 0x35353535UL, 0x85858585UL,
    0xe2e2e2e2UL, 0xf9f9f9f9UL, 0x37373737UL, 0xe8e8e8e8UL,
    0x1c1c1c1cUL, 0x75757575UL, 0xdfdfdfdfUL, 0x6e6e6e6eUL,
    0x47474747UL, 0xf1f1f1f1UL, 0x1a1a1a1aUL, 0x71717171UL,
    0x1d1d1d1dUL, 0x29292929UL, 0xc5c5c5c5UL, 0x89898989UL,
    0x6f6f6f6fUL, 0xb7b7b7b7UL, 0x62626262UL, 0x0e0e0e0eUL,
    0xaaaaaaaaUL, 0x18181818UL, 0xbebebebeUL, 0x1b1b1b1bUL,
    0xfcfcfcfcUL, 0x56565656UL, 0x3e3e3e3eUL, 0x4b4b4b4bUL,
    0xc6c6c6c6UL, 0xd2d2d2d2UL, 0x79797979UL, 0x20202020UL,
    0x9a9a9a9aUL, 0xdbdbdbdbUL, 0xc0c0c0c0UL, 0xfefefefeUL,
    0x78787878UL, 0xcdcdcdcdUL, 0x5a5a5a5aUL, 0xf4f4f4f4UL,
    0x1f1f1f1fUL, 0xddddddddUL, 0xa8a8a8a8UL, 0x33333333UL,
    0x88888888UL, 0x07070707UL, 0xc7c7c7c7UL, 0x31313131UL,
    0xb1b1b1b1UL, 0x12121212UL, 0x10101010UL, 0x59595959UL,
    0x27272727UL, 0x80808080UL, 0xececececUL, 0x5f5f5f5fUL,
    0x60606060UL, 0x51515151UL, 0x7f7f7f7fUL, 0xa9a9a9a9UL,
    0x19191919UL, 0xb5b5b5b5UL, 0x4a4a4a4aUL, 0x0d0d0d0dUL,
    0x2d2d2d2dUL, 0xe5e5e5e5UL, 0x7a7a7a7aUL, 0x9f9f9f9fUL,
    0x93939393UL, 0xc9c9c9c9UL, 0x9c9c9c9cUL, 0xefefefefUL,
    0xa0a0a0a0UL, 0xe0e0e0e0UL, 0x3b3b3b3bUL, 0x4d4d4d4dUL,
    0xaeaeaeaeUL, 0x2a2a2a2aUL, 0xf5f5f5f5UL, 0xb0b0b0b0UL,
    0xc8c8c8c8UL, 0xebebebebUL, 0xbbbbbbbbUL, 0x3c3c3c3cUL,
    0x83838383UL, 0x53535353UL, 0x99999999UL, 0x61616161UL,
    0x17171717UL, 0x2b2b2b2bUL, 0x04040404UL, 0x7e7e7e7eUL,
    0xbabababaUL, 0x77777777UL, 0xd6d6d6d6UL, 0x26262626UL,
    0xe1e1e1e1UL, 0x69696969UL, 0x14141414UL, 0x63636363UL,
    0x55555555UL, 0x21212121UL, 0x0c0c0c0cUL, 0x7d7d7d7dUL,
};

#endif /* ENCRYPT_ONLY */

#ifdef LTC_SMALL_CODE

#define Te0(x) TE0[x]
#define Te1(x) RORc(TE0[x], 8)
#define Te2(x) RORc(TE0[x], 16)
#define Te3(x) RORc(TE0[x], 24)

#define Td0(x) TD0[x]
#define Td1(x) RORc(TD0[x], 8)
#define Td2(x) RORc(TD0[x], 16)
#define Td3(x) RORc(TD0[x], 24)

#define Te4_0 0x000000FF & Te4
#define Te4_1 0x0000FF00 & Te4
#define Te4_2 0x00FF0000 & Te4
#define Te4_3 0xFF000000 & Te4

#else

#define Te0(x) TE0[x]
#define Te1(x) TE1[x]
#define Te2(x) TE2[x]
#define Te3(x) TE3[x]

#define Td0(x) TD0[x]
#define Td1(x) TD1[x]
#define Td2(x) TD2[x]
#define Td3(x) TD3[x]

static const ulong32 TE1[256] = {
    0xa5c66363UL, 0x84f87c7cUL, 0x99ee7777UL, 0x8df67b7bUL,
    0x0dfff2f2UL, 0xbdd66b6bUL, 0xb1de6f6fUL, 0x5491c5c5UL,
    0x50603030UL, 0x03020101UL, 0xa9ce6767UL, 0x7d562b2bUL,
    0x19e7fefeUL, 0x62b5d7d7UL, 0xe64dababUL, 0x9aec7676UL,
    0x458fcacaUL, 0x9d1f8282UL, 0x4089c9c9UL, 0x87fa7d7dUL,
    0x15effafaUL, 0xebb25959UL, 0xc98e4747UL, 0x0bfbf0f0UL,
    0xec41adadUL, 0x67b3d4d4UL, 0xfd5fa2a2UL, 0xea45afafUL,
    0xbf239c9cUL, 0xf753a4a4UL, 0x96e47272UL, 0x5b9bc0c0UL,
    0xc275b7b7UL, 0x1ce1fdfdUL, 0xae3d9393UL, 0x6a4c2626UL,
    0x5a6c3636UL, 0x417e3f3fUL, 0x02f5f7f7UL, 0x4f83ccccUL,
    0x5c683434UL, 0xf451a5a5UL, 0x34d1e5e5UL, 0x08f9f1f1UL,
    0x93e27171UL, 0x73abd8d8UL, 0x53623131UL, 0x3f2a1515UL,
    0x0c080404UL, 0x5295c7c7UL, 0x65462323UL, 0x5e9dc3c3UL,
    0x28301818UL, 0xa1379696UL, 0x0f0a0505UL, 0xb52f9a9aUL,
    0x090e0707UL, 0x36241212UL, 0x9b1b8080UL, 0x3ddfe2e2UL,
    0x26cdebebUL, 0x694e2727UL, 0xcd7fb2b2UL, 0x9fea7575UL,
    0x1b120909UL, 0x9e1d8383UL, 0x74582c2cUL, 0x2e341a1aUL,
    0x2d361b1bUL, 0xb2dc6e6eUL, 0xeeb45a5aUL, 0xfb5ba0a0UL,
    0xf6a45252UL, 0x4d763b3bUL, 0x61b7d6d6UL, 0xce7db3b3UL,
    0x7b522929UL, 0x3edde3e3UL, 0x715e2f2fUL, 0x97138484UL,
    0xf5a65353UL, 0x68b9d1d1UL, 0x00000000UL, 0x2cc1ededUL,
    0x60402020UL, 0x1fe3fcfcUL, 0xc879b1b1UL, 0xedb65b5bUL,
    0xbed46a6aUL, 0x468dcbcbUL, 0xd967bebeUL, 0x4b723939UL,
    0xde944a4aUL, 0xd4984c4cUL, 0xe8b05858UL, 0x4a85cfcfUL,
    0x6bbbd0d0UL, 0x2ac5efefUL, 0xe54faaaaUL, 0x16edfbfbUL,
    0xc5864343UL, 0xd79a4d4dUL, 0x55663333UL, 0x94118585UL,
    0xcf8a4545UL, 0x10e9f9f9UL, 0x06040202UL, 0x81fe7f7fUL,
    0xf0a05050UL, 0x44783c3cUL, 0xba259f9fUL, 0xe34ba8a8UL,
    0xf3a25151UL, 0xfe5da3a3UL, 0xc0804040UL, 0x8a058f8fUL,
    0xad3f9292UL, 0xbc219d9dUL, 0x48703838UL, 0x04f1f5f5UL,
    0xdf63bcbcUL, 0xc177b6b6UL, 0x75afdadaUL, 0x63422121UL,
    0x30201010UL, 0x1ae5ffffUL, 0x0efdf3f3UL, 0x6dbfd2d2UL,
    0x4c81cdcdUL, 0x14180c0cUL, 0x35261313UL, 0x2fc3ececUL,
    0xe1be5f5fUL, 0xa2359797UL, 0xcc884444UL, 0x392e1717UL,
    0x5793c4c4UL, 0xf255a7a7UL, 0x82fc7e7eUL, 0x477a3d3dUL,
    0xacc86464UL, 0xe7ba5d5dUL, 0x2b321919UL, 0x95e67373UL,
    0xa0c06060UL, 0x98198181UL, 0xd19e4f4fUL, 0x7fa3dcdcUL,
    0x66442222UL, 0x7e542a2aUL, 0xab3b9090UL, 0x830b8888UL,
    0xca8c4646UL, 0x29c7eeeeUL, 0xd36bb8b8UL, 0x3c281414UL,
    0x79a7dedeUL, 0xe2bc5e5eUL, 0x1d160b0bUL, 0x76addbdbUL,
    0x3bdbe0e0UL, 0x56643232UL, 0x4e743a3aUL, 0x1e140a0aUL,
    0xdb924949UL, 0x0a0c0606UL, 0x6c482424UL, 0xe4b85c5cUL,
    0x5d9fc2c2UL, 0x6ebdd3d3UL, 0xef43acacUL, 0xa6c46262UL,
    0xa8399191UL, 0xa4319595UL, 0x37d3e4e4UL, 0x8bf27979UL,
    0x32d5e7e7UL, 0x438bc8c8UL, 0x596e3737UL, 0xb7da6d6dUL,
    0x8c018d8dUL, 0x64b1d5d5UL, 0xd29c4e4eUL, 0xe049a9a9UL,
    0xb4d86c6cUL, 0xfaac5656UL, 0x07f3f4f4UL, 0x25cfeaeaUL,
    0xafca6565UL, 0x8ef47a7aUL, 0xe947aeaeUL, 0x18100808UL,
    0xd56fbabaUL, 0x88f07878UL, 0x6f4a2525UL, 0x725c2e2eUL,
    0x24381c1cUL, 0xf157a6a6UL, 0xc773b4b4UL, 0x5197c6c6UL,
    0x23cbe8e8UL, 0x7ca1ddddUL, 0x9ce87474UL, 0x213e1f1fUL,
    0xdd964b4bUL, 0xdc61bdbdUL, 0x860d8b8bUL, 0x850f8a8aUL,
    0x90e07070UL, 0x427c3e3eUL, 0xc471b5b5UL, 0xaacc6666UL,
    0xd8904848UL, 0x05060303UL, 0x01f7f6f6UL, 0x121c0e0eUL,
    0xa3c26161UL, 0x5f6a3535UL, 0xf9ae5757UL, 0xd069b9b9UL,
    0x91178686UL, 0x5899c1c1UL, 0x273a1d1dUL, 0xb9279e9eUL,
    0x38d9e1e1UL, 0x13ebf8f8UL, 0xb32b9898UL, 0x33221111UL,
    0xbbd26969UL, 0x70a9d9d9UL, 0x89078e8eUL, 0xa7339494UL,
    0xb62d9b9bUL, 0x223c1e1eUL, 0x92158787UL, 0x20c9e9e9UL,
    0x4987ceceUL, 0xffaa5555UL, 0x78502828UL, 0x7aa5dfdfUL,
    0x8f038c8cUL, 0xf859a1a1UL, 0x80098989UL, 0x171a0d0dUL,
    0xda65bfbfUL, 0x31d7e6e6UL, 0xc6844242UL, 0xb8d06868UL,
    0xc3824141UL, 0xb0299999UL, 0x775a2d2dUL, 0x111e0f0fUL,
    0xcb7bb0b0UL, 0xfca85454UL, 0xd66dbbbbUL, 0x3a2c1616UL,
};
static const ulong32 TE2[256] = {
    0x63a5c663UL, 0x7c84f87cUL, 0x7799ee77UL, 0x7b8df67bUL,
    0xf20dfff2UL, 0x6bbdd66bUL, 0x6fb1de6fUL, 0xc55491c5UL,
    0x30506030UL, 0x01030201UL, 0x67a9ce67UL, 0x2b7d562bUL,
    0xfe19e7feUL, 0xd762b5d7UL, 0xabe64dabUL, 0x769aec76UL,
    0xca458fcaUL, 0x829d1f82UL, 0xc94089c9UL, 0x7d87fa7dUL,
    0xfa15effaUL, 0x59ebb259UL, 0x47c98e47UL, 0xf00bfbf0UL,
    0xadec41adUL, 0xd467b3d4UL, 0xa2fd5fa2UL, 0xafea45afUL,
    0x9cbf239cUL, 0xa4f753a4UL, 0x7296e472UL, 0xc05b9bc0UL,
    0xb7c275b7UL, 0xfd1ce1fdUL, 0x93ae3d93UL, 0x266a4c26UL,
    0x365a6c36UL, 0x3f417e3fUL, 0xf702f5f7UL, 0xcc4f83ccUL,
    0x345c6834UL, 0xa5f451a5UL, 0xe534d1e5UL, 0xf108f9f1UL,
    0x7193e271UL, 0xd873abd8UL, 0x31536231UL, 0x153f2a15UL,
    0x040c0804UL, 0xc75295c7UL, 0x23654623UL, 0xc35e9dc3UL,
    0x18283018UL, 0x96a13796UL, 0x050f0a05UL, 0x9ab52f9aUL,
    0x07090e07UL, 0x12362412UL, 0x809b1b80UL, 0xe23ddfe2UL,
    0xeb26cdebUL, 0x27694e27UL, 0xb2cd7fb2UL, 0x759fea75UL,
    0x091b1209UL, 0x839e1d83UL, 0x2c74582cUL, 0x1a2e341aUL,
    0x1b2d361bUL, 0x6eb2dc6eUL, 0x5aeeb45aUL, 0xa0fb5ba0UL,
    0x52f6a452UL, 0x3b4d763bUL, 0xd661b7d6UL, 0xb3ce7db3UL,
    0x297b5229UL, 0xe33edde3UL, 0x2f715e2fUL, 0x84971384UL,
    0x53f5a653UL, 0xd168b9d1UL, 0x00000000UL, 0xed2cc1edUL,
    0x20604020UL, 0xfc1fe3fcUL, 0xb1c879b1UL, 0x5bedb65bUL,
    0x6abed46aUL, 0xcb468dcbUL, 0xbed967beUL, 0x394b7239UL,
    0x4ade944aUL, 0x4cd4984cUL, 0x58e8b058UL, 0xcf4a85cfUL,
    0xd06bbbd0UL, 0xef2ac5efUL, 0xaae54faaUL, 0xfb16edfbUL,
    0x43c58643UL, 0x4dd79a4dUL, 0x33556633UL, 0x85941185UL,
    0x45cf8a45UL, 0xf910e9f9UL, 0x02060402UL, 0x7f81fe7fUL,
    0x50f0a050UL, 0x3c44783cUL, 0x9fba259fUL, 0xa8e34ba8UL,
    0x51f3a251UL, 0xa3fe5da3UL, 0x40c08040UL, 0x8f8a058fUL,
    0x92ad3f92UL, 0x9dbc219dUL, 0x38487038UL, 0xf504f1f5UL,
    0xbcdf63bcUL, 0xb6c177b6UL, 0xda75afdaUL, 0x21634221UL,
    0x10302010UL, 0xff1ae5ffUL, 0xf30efdf3UL, 0xd26dbfd2UL,
    0xcd4c81cdUL, 0x0c14180cUL, 0x13352613UL, 0xec2fc3ecUL,
    0x5fe1be5fUL, 0x97a23597UL, 0x44cc8844UL, 0x17392e17UL,
    0xc45793c4UL, 0xa7f255a7UL, 0x7e82fc7eUL, 0x3d477a3dUL,
    0x64acc864UL, 0x5de7ba5dUL, 0x192b3219UL, 0x7395e673UL,
    0x60a0c060UL, 0x81981981UL, 0x4fd19e4fUL, 0xdc7fa3dcUL,
    0x22664422UL, 0x2a7e542aUL, 0x90ab3b90UL, 0x88830b88UL,
    0x46ca8c46UL, 0xee29c7eeUL, 0xb8d36bb8UL, 0x143c2814UL,
    0xde79a7deUL, 0x5ee2bc5eUL, 0x0b1d160bUL, 0xdb76addbUL,
    0xe03bdbe0UL, 0x32566432UL, 0x3a4e743aUL, 0x0a1e140aUL,
    0x49db9249UL, 0x060a0c06UL, 0x246c4824UL, 0x5ce4b85cUL,
    0xc25d9fc2UL, 0xd36ebdd3UL, 0xacef43acUL, 0x62a6c462UL,
    0x91a83991UL, 0x95a43195UL, 0xe437d3e4UL, 0x798bf279UL,
    0xe732d5e7UL, 0xc8438bc8UL, 0x37596e37UL, 0x6db7da6dUL,
    0x8d8c018dUL, 0xd564b1d5UL, 0x4ed29c4eUL, 0xa9e049a9UL,
    0x6cb4d86cUL, 0x56faac56UL, 0xf407f3f4UL, 0xea25cfeaUL,
    0x65afca65UL, 0x7a8ef47aUL, 0xaee947aeUL, 0x08181008UL,
    0xbad56fbaUL, 0x7888f078UL, 0x256f4a25UL, 0x2e725c2eUL,
    0x1c24381cUL, 0xa6f157a6UL, 0xb4c773b4UL, 0xc65197c6UL,
    0xe823cbe8UL, 0xdd7ca1ddUL, 0x749ce874UL, 0x1f213e1fUL,
    0x4bdd964bUL, 0xbddc61bdUL, 0x8b860d8bUL, 0x8a850f8aUL,
    0x7090e070UL, 0x3e427c3eUL, 0xb5c471b5UL, 0x66aacc66UL,
    0x48d89048UL, 0x03050603UL, 0xf601f7f6UL, 0x0e121c0eUL,
    0x61a3c261UL, 0x355f6a35UL, 0x57f9ae57UL, 0xb9d069b9UL,
    0x86911786UL, 0xc15899c1UL, 0x1d273a1dUL, 0x9eb9279eUL,
    0xe138d9e1UL, 0xf813ebf8UL, 0x98b32b98UL, 0x11332211UL,
    0x69bbd269UL, 0xd970a9d9UL, 0x8e89078eUL, 0x94a73394UL,
    0x9bb62d9bUL, 0x1e223c1eUL, 0x87921587UL, 0xe920c9e9UL,
    0xce4987ceUL, 0x55ffaa55UL, 0x28785028UL, 0xdf7aa5dfUL,
    0x8c8f038cUL, 0xa1f859a1UL, 0x89800989UL, 0x0d171a0dUL,
    0xbfda65bfUL, 0xe631d7e6UL, 0x42c68442UL, 0x68b8d068UL,
    0x41c38241UL, 0x99b02999UL, 0x2d775a2dUL, 0x0f111e0fUL,
    0xb0cb7bb0UL, 0x54fca854UL, 0xbbd66dbbUL, 0x163a2c16UL,
};
static const ulong32 TE3[256] = {

    0x6363a5c6UL, 0x7c7c84f8UL, 0x777799eeUL, 0x7b7b8df6UL,
    0xf2f20dffUL, 0x6b6bbdd6UL, 0x6f6fb1deUL, 0xc5c55491UL,
    0x30305060UL, 0x01010302UL, 0x6767a9ceUL, 0x2b2b7d56UL,
    0xfefe19e7UL, 0xd7d762b5UL, 0xababe64dUL, 0x76769aecUL,
    0xcaca458fUL, 0x82829d1fUL, 0xc9c94089UL, 0x7d7d87faUL,
    0xfafa15efUL, 0x5959ebb2UL, 0x4747c98eUL, 0xf0f00bfbUL,
    0xadadec41UL, 0xd4d467b3UL, 0xa2a2fd5fUL, 0xafafea45UL,
    0x9c9cbf23UL, 0xa4a4f753UL, 0x727296e4UL, 0xc0c05b9bUL,
    0xb7b7c275UL, 0xfdfd1ce1UL, 0x9393ae3dUL, 0x26266a4cUL,
    0x36365a6cUL, 0x3f3f417eUL, 0xf7f702f5UL, 0xcccc4f83UL,
    0x34345c68UL, 0xa5a5f451UL, 0xe5e534d1UL, 0xf1f108f9UL,
    0x717193e2UL, 0xd8d873abUL, 0x31315362UL, 0x15153f2aUL,
    0x04040c08UL, 0xc7c75295UL, 0x23236546UL, 0xc3c35e9dUL,
    0x18182830UL, 0x9696a137UL, 0x05050f0aUL, 0x9a9ab52fUL,
    0x0707090eUL, 0x12123624UL, 0x80809b1bUL, 0xe2e23ddfUL,
    0xebeb26cdUL, 0x2727694eUL, 0xb2b2cd7fUL, 0x75759feaUL,
    0x09091b12UL, 0x83839e1dUL, 0x2c2c7458UL, 0x1a1a2e34UL,
    0x1b1b2d36UL, 0x6e6eb2dcUL, 0x5a5aeeb4UL, 0xa0a0fb5bUL,
    0x5252f6a4UL, 0x3b3b4d76UL, 0xd6d661b7UL, 0xb3b3ce7dUL,
    0x29297b52UL, 0xe3e33eddUL, 0x2f2f715eUL, 0x84849713UL,
    0x5353f5a6UL, 0xd1d168b9UL, 0x00000000UL, 0xeded2cc1UL,
    0x20206040UL, 0xfcfc1fe3UL, 0xb1b1c879UL, 0x5b5bedb6UL,
    0x6a6abed4UL, 0xcbcb468dUL, 0xbebed967UL, 0x39394b72UL,
    0x4a4ade94UL, 0x4c4cd498UL, 0x5858e8b0UL, 0xcfcf4a85UL,
    0xd0d06bbbUL, 0xefef2ac5UL, 0xaaaae54fUL, 0xfbfb16edUL,
    0x4343c586UL, 0x4d4dd79aUL, 0x33335566UL, 0x85859411UL,
    0x4545cf8aUL, 0xf9f910e9UL, 0x02020604UL, 0x7f7f81feUL,
    0x5050f0a0UL, 0x3c3c4478UL, 0x9f9fba25UL, 0xa8a8e34bUL,
    0x5151f3a2UL, 0xa3a3fe5dUL, 0x4040c080UL, 0x8f8f8a05UL,
    0x9292ad3fUL, 0x9d9dbc21UL, 0x38384870UL, 0xf5f504f1UL,
    0xbcbcdf63UL, 0xb6b6c177UL, 0xdada75afUL, 0x21216342UL,
    0x10103020UL, 0xffff1ae5UL, 0xf3f30efdUL, 0xd2d26dbfUL,
    0xcdcd4c81UL, 0x0c0c1418UL, 0x13133526UL, 0xecec2fc3UL,
    0x5f5fe1beUL, 0x9797a235UL, 0x4444cc88UL, 0x1717392eUL,
    0xc4c45793UL, 0xa7a7f255UL, 0x7e7e82fcUL, 0x3d3d477aUL,
    0x6464acc8UL, 0x5d5de7baUL, 0x19192b32UL, 0x737395e6UL,
    0x6060a0c0UL, 0x81819819UL, 0x4f4fd19eUL, 0xdcdc7fa3UL,
    0x22226644UL, 0x2a2a7e54UL, 0x9090ab3bUL, 0x8888830bUL,
    0x4646ca8cUL, 0xeeee29c7UL, 0xb8b8d36bUL, 0x14143c28UL,
    0xdede79a7UL, 0x5e5ee2bcUL, 0x0b0b1d16UL, 0xdbdb76adUL,
    0xe0e03bdbUL, 0x32325664UL, 0x3a3a4e74UL, 0x0a0a1e14UL,
    0x4949db92UL, 0x06060a0cUL, 0x24246c48UL, 0x5c5ce4b8UL,
    0xc2c25d9fUL, 0xd3d36ebdUL, 0xacacef43UL, 0x6262a6c4UL,
    0x9191a839UL, 0x9595a431UL, 0xe4e437d3UL, 0x79798bf2UL,
    0xe7e732d5UL, 0xc8c8438bUL, 0x3737596eUL, 0x6d6db7daUL,
    0x8d8d8c01UL, 0xd5d564b1UL, 0x4e4ed29cUL, 0xa9a9e049UL,
    0x6c6cb4d8UL, 0x5656faacUL, 0xf4f407f3UL, 0xeaea25cfUL,
    0x6565afcaUL, 0x7a7a8ef4UL, 0xaeaee947UL, 0x08081810UL,
    0xbabad56fUL, 0x787888f0UL, 0x25256f4aUL, 0x2e2e725cUL,
    0x1c1c2438UL, 0xa6a6f157UL, 0xb4b4c773UL, 0xc6c65197UL,
    0xe8e823cbUL, 0xdddd7ca1UL, 0x74749ce8UL, 0x1f1f213eUL,
    0x4b4bdd96UL, 0xbdbddc61UL, 0x8b8b860dUL, 0x8a8a850fUL,
    0x707090e0UL, 0x3e3e427cUL, 0xb5b5c471UL, 0x6666aaccUL,
    0x4848d890UL, 0x03030506UL, 0xf6f601f7UL, 0x0e0e121cUL,
    0x6161a3c2UL, 0x35355f6aUL, 0x5757f9aeUL, 0xb9b9d069UL,
    0x86869117UL, 0xc1c15899UL, 0x1d1d273aUL, 0x9e9eb927UL,
    0xe1e138d9UL, 0xf8f813ebUL, 0x9898b32bUL, 0x11113322UL,
    0x6969bbd2UL, 0xd9d970a9UL, 0x8e8e8907UL, 0x9494a733UL,
    0x9b9bb62dUL, 0x1e1e223cUL, 0x87879215UL, 0xe9e920c9UL,
    0xcece4987UL, 0x5555ffaaUL, 0x28287850UL, 0xdfdf7aa5UL,
    0x8c8c8f03UL, 0xa1a1f859UL, 0x89898009UL, 0x0d0d171aUL,
    0xbfbfda65UL, 0xe6e631d7UL, 0x4242c684UL, 0x6868b8d0UL,
    0x4141c382UL, 0x9999b029UL, 0x2d2d775aUL, 0x0f0f111eUL,
    0xb0b0cb7bUL, 0x5454fca8UL, 0xbbbbd66dUL, 0x16163a2cUL,
};

#ifndef PELI_TAB
static const ulong32 Te4_0[] = {
0x00000063UL, 0x0000007cUL, 0x00000077UL, 0x0000007bUL, 0x000000f2UL, 0x0000006bUL, 0x0000006fUL, 0x000000c5UL,
0x00000030UL, 0x00000001UL, 0x00000067UL, 0x0000002bUL, 0x000000feUL, 0x000000d7UL, 0x000000abUL, 0x00000076UL,
0x000000caUL, 0x00000082UL, 0x000000c9UL, 0x0000007dUL, 0x000000faUL, 0x00000059UL, 0x00000047UL, 0x000000f0UL,
0x000000adUL, 0x000000d4UL, 0x000000a2UL, 0x000000afUL, 0x0000009cUL, 0x000000a4UL, 0x00000072UL, 0x000000c0UL,
0x000000b7UL, 0x000000fdUL, 0x00000093UL, 0x00000026UL, 0x00000036UL, 0x0000003fUL, 0x000000f7UL, 0x000000ccUL,
0x00000034UL, 0x000000a5UL, 0x000000e5UL, 0x000000f1UL, 0x00000071UL, 0x000000d8UL, 0x00000031UL, 0x00000015UL,
0x00000004UL, 0x000000c7UL, 0x00000023UL, 0x000000c3UL, 0x00000018UL, 0x00000096UL, 0x00000005UL, 0x0000009aUL,
0x00000007UL, 0x00000012UL, 0x00000080UL, 0x000000e2UL, 0x000000ebUL, 0x00000027UL, 0x000000b2UL, 0x00000075UL,
0x00000009UL, 0x00000083UL, 0x0000002cUL, 0x0000001aUL, 0x0000001bUL, 0x0000006eUL, 0x0000005aUL, 0x000000a0UL,
0x00000052UL, 0x0000003bUL, 0x000000d6UL, 0x000000b3UL, 0x00000029UL, 0x000000e3UL, 0x0000002fUL, 0x00000084UL,
0x00000053UL, 0x000000d1UL, 0x00000000UL, 0x000000edUL, 0x00000020UL, 0x000000fcUL, 0x000000b1UL, 0x0000005bUL,
0x0000006aUL, 0x000000cbUL, 0x000000beUL, 0x00000039UL, 0x0000004aUL, 0x0000004cUL, 0x00000058UL, 0x000000cfUL,
0x000000d0UL, 0x000000efUL, 0x000000aaUL, 0x000000fbUL, 0x00000043UL, 0x0000004dUL, 0x00000033UL, 0x00000085UL,
0x00000045UL, 0x000000f9UL, 0x00000002UL, 0x0000007fUL, 0x00000050UL, 0x0000003cUL, 0x0000009fUL, 0x000000a8UL,
0x00000051UL, 0x000000a3UL, 0x00000040UL, 0x0000008fUL, 0x00000092UL, 0x0000009dUL, 0x00000038UL, 0x000000f5UL,
0x000000bcUL, 0x000000b6UL, 0x000000daUL, 0x00000021UL, 0x00000010UL, 0x000000ffUL, 0x000000f3UL, 0x000000d2UL,
0x000000cdUL, 0x0000000cUL, 0x00000013UL, 0x000000ecUL, 0x0000005fUL, 0x00000097UL, 0x00000044UL, 0x00000017UL,
0x000000c4UL, 0x000000a7UL, 0x0000007eUL, 0x0000003dUL, 0x00000064UL, 0x0000005dUL, 0x00000019UL, 0x00000073UL,
0x00000060UL, 0x00000081UL, 0x0000004fUL, 0x000000dcUL, 0x00000022UL, 0x0000002aUL, 0x00000090UL, 0x00000088UL,
0x00000046UL, 0x000000eeUL, 0x000000b8UL, 0x00000014UL, 0x000000deUL, 0x0000005eUL, 0x0000000bUL, 0x000000dbUL,
0x000000e0UL, 0x00000032UL, 0x0000003aUL, 0x0000000aUL, 0x00000049UL, 0x00000006UL, 0x00000024UL, 0x0000005cUL,
0x000000c2UL, 0x000000d3UL, 0x000000acUL, 0x00000062UL, 0x00000091UL, 0x00000095UL, 0x000000e4UL, 0x00000079UL,
0x000000e7UL, 0x000000c8UL, 0x00000037UL, 0x0000006dUL, 0x0000008dUL, 0x000000d5UL, 0x0000004eUL, 0x000000a9UL,
0x0000006cUL, 0x00000056UL, 0x000000f4UL, 0x000000eaUL, 0x00000065UL, 0x0000007aUL, 0x000000aeUL, 0x00000008UL,
0x000000baUL, 0x00000078UL, 0x00000025UL, 0x0000002eUL, 0x0000001cUL, 0x000000a6UL, 0x000000b4UL, 0x000000c6UL,
0x000000e8UL, 0x000000ddUL, 0x00000074UL, 0x0000001fUL, 0x0000004bUL, 0x000000bdUL, 0x0000008bUL, 0x0000008aUL,
0x00000070UL, 0x0000003eUL, 0x000000b5UL, 0x00000066UL, 0x00000048UL, 0x00000003UL, 0x000000f6UL, 0x0000000eUL,
0x00000061UL, 0x00000035UL, 0x00000057UL, 0x000000b9UL, 0x00000086UL, 0x000000c1UL, 0x0000001dUL, 0x0000009eUL,
0x000000e1UL, 0x000000f8UL, 0x00000098UL, 0x00000011UL, 0x00000069UL, 0x000000d9UL, 0x0000008eUL, 0x00000094UL,
0x0000009bUL, 0x0000001eUL, 0x00000087UL, 0x000000e9UL, 0x000000ceUL, 0x00000055UL, 0x00000028UL, 0x000000dfUL,
0x0000008cUL, 0x000000a1UL, 0x00000089UL, 0x0000000dUL, 0x000000bfUL, 0x000000e6UL, 0x00000042UL, 0x00000068UL,
0x00000041UL, 0x00000099UL, 0x0000002dUL, 0x0000000fUL, 0x000000b0UL, 0x00000054UL, 0x000000bbUL, 0x00000016UL
};

static const ulong32 Te4_1[] = {
0x00006300UL, 0x00007c00UL, 0x00007700UL, 0x00007b00UL, 0x0000f200UL, 0x00006b00UL, 0x00006f00UL, 0x0000c500UL,
0x00003000UL, 0x00000100UL, 0x00006700UL, 0x00002b00UL, 0x0000fe00UL, 0x0000d700UL, 0x0000ab00UL, 0x00007600UL,
0x0000ca00UL, 0x00008200UL, 0x0000c900UL, 0x00007d00UL, 0x0000fa00UL, 0x00005900UL, 0x00004700UL, 0x0000f000UL,
0x0000ad00UL, 0x0000d400UL, 0x0000a200UL, 0x0000af00UL, 0x00009c00UL, 0x0000a400UL, 0x00007200UL, 0x0000c000UL,
0x0000b700UL, 0x0000fd00UL, 0x00009300UL, 0x00002600UL, 0x00003600UL, 0x00003f00UL, 0x0000f700UL, 0x0000cc00UL,
0x00003400UL, 0x0000a500UL, 0x0000e500UL, 0x0000f100UL, 0x00007100UL, 0x0000d800UL, 0x00003100UL, 0x00001500UL,
0x00000400UL, 0x0000c700UL, 0x00002300UL, 0x0000c300UL, 0x00001800UL, 0x00009600UL, 0x00000500UL, 0x00009a00UL,
0x00000700UL, 0x00001200UL, 0x00008000UL, 0x0000e200UL, 0x0000eb00UL, 0x00002700UL, 0x0000b200UL, 0x00007500UL,
0x00000900UL, 0x00008300UL, 0x00002c00UL, 0x00001a00UL, 0x00001b00UL, 0x00006e00UL, 0x00005a00UL, 0x0000a000UL,
0x00005200UL, 0x00003b00UL, 0x0000d600UL, 0x0000b300UL, 0x00002900UL, 0x0000e300UL, 0x00002f00UL, 0x00008400UL,
0x00005300UL, 0x0000d100UL, 0x00000000UL, 0x0000ed00UL, 0x00002000UL, 0x0000fc00UL, 0x0000b100UL, 0x00005b00UL,
0x00006a00UL, 0x0000cb00UL, 0x0000be00UL, 0x00003900UL, 0x00004a00UL, 0x00004c00UL, 0x00005800UL, 0x0000cf00UL,
0x0000d000UL, 0x0000ef00UL, 0x0000aa00UL, 0x0000fb00UL, 0x00004300UL, 0x00004d00UL, 0x00003300UL, 0x00008500UL,
0x00004500UL, 0x0000f900UL, 0x00000200UL, 0x00007f00UL, 0x00005000UL, 0x00003c00UL, 0x00009f00UL, 0x0000a800UL,
0x00005100UL, 0x0000a300UL, 0x00004000UL, 0x00008f00UL, 0x00009200UL, 0x00009d00UL, 0x00003800UL, 0x0000f500UL,
0x0000bc00UL, 0x0000b600UL, 0x0000da00UL, 0x00002100UL, 0x00001000UL, 0x0000ff00UL, 0x0000f300UL, 0x0000d200UL,
0x0000cd00UL, 0x00000c00UL, 0x00001300UL, 0x0000ec00UL, 0x00005f00UL, 0x00009700UL, 0x00004400UL, 0x00001700UL,
0x0000c400UL, 0x0000a700UL, 0x00007e00UL, 0x00003d00UL, 0x00006400UL, 0x00005d00UL, 0x00001900UL, 0x00007300UL,
0x00006000UL, 0x00008100UL, 0x00004f00UL, 0x0000dc00UL, 0x00002200UL, 0x00002a00UL, 0x00009000UL, 0x00008800UL,
0x00004600UL, 0x0000ee00UL, 0x0000b800UL, 0x00001400UL, 0x0000de00UL, 0x00005e00UL, 0x00000b00UL, 0x0000db00UL,
0x0000e000UL, 0x00003200UL, 0x00003a00UL, 0x00000a00UL, 0x00004900UL, 0x00000600UL, 0x00002400UL, 0x00005c00UL,
0x0000c200UL, 0x0000d300UL, 0x0000ac00UL, 0x00006200UL, 0x00009100UL, 0x00009500UL, 0x0000e400UL, 0x00007900UL,
0x0000e700UL, 0x0000c800UL, 0x00003700UL, 0x00006d00UL, 0x00008d00UL, 0x0000d500UL, 0x00004e00UL, 0x0000a900UL,
0x00006c00UL, 0x00005600UL, 0x0000f400UL, 0x0000ea00UL, 0x00006500UL, 0x00007a00UL, 0x0000ae00UL, 0x00000800UL,
0x0000ba00UL, 0x00007800UL, 0x00002500UL, 0x00002e00UL, 0x00001c00UL, 0x0000a600UL, 0x0000b400UL, 0x0000c600UL,
0x0000e800UL, 0x0000dd00UL, 0x00007400UL, 0x00001f00UL, 0x00004b00UL, 0x0000bd00UL, 0x00008b00UL, 0x00008a00UL,
0x00007000UL, 0x00003e00UL, 0x0000b500UL, 0x00006600UL, 0x00004800UL, 0x00000300UL, 0x0000f600UL, 0x00000e00UL,
0x00006100UL, 0x00003500UL, 0x00005700UL, 0x0000b900UL, 0x00008600UL, 0x0000c100UL, 0x00001d00UL, 0x00009e00UL,
0x0000e100UL, 0x0000f800UL, 0x00009800UL, 0x00001100UL, 0x00006900UL, 0x0000d900UL, 0x00008e00UL, 0x00009400UL,
0x00009b00UL, 0x00001e00UL, 0x00008700UL, 0x0000e900UL, 0x0000ce00UL, 0x00005500UL, 0x00002800UL, 0x0000df00UL,
0x00008c00UL, 0x0000a100UL, 0x00008900UL, 0x00000d00UL, 0x0000bf00UL, 0x0000e600UL, 0x00004200UL, 0x00006800UL,
0x00004100UL, 0x00009900UL, 0x00002d00UL, 0x00000f00UL, 0x0000b000UL, 0x00005400UL, 0x0000bb00UL, 0x00001600UL
};

static const ulong32 Te4_2[] = {
0x00630000UL, 0x007c0000UL, 0x00770000UL, 0x007b0000UL, 0x00f20000UL, 0x006b0000UL, 0x006f0000UL, 0x00c50000UL,
0x00300000UL, 0x00010000UL, 0x00670000UL, 0x002b0000UL, 0x00fe0000UL, 0x00d70000UL, 0x00ab0000UL, 0x00760000UL,
0x00ca0000UL, 0x00820000UL, 0x00c90000UL, 0x007d0000UL, 0x00fa0000UL, 0x00590000UL, 0x00470000UL, 0x00f00000UL,
0x00ad0000UL, 0x00d40000UL, 0x00a20000UL, 0x00af0000UL, 0x009c0000UL, 0x00a40000UL, 0x00720000UL, 0x00c00000UL,
0x00b70000UL, 0x00fd0000UL, 0x00930000UL, 0x00260000UL, 0x00360000UL, 0x003f0000UL, 0x00f70000UL, 0x00cc0000UL,
0x00340000UL, 0x00a50000UL, 0x00e50000UL, 0x00f10000UL, 0x00710000UL, 0x00d80000UL, 0x00310000UL, 0x00150000UL,
0x00040000UL, 0x00c70000UL, 0x00230000UL, 0x00c30000UL, 0x00180000UL, 0x00960000UL, 0x00050000UL, 0x009a0000UL,
0x00070000UL, 0x00120000UL, 0x00800000UL, 0x00e20000UL, 0x00eb0000UL, 0x00270000UL, 0x00b20000UL, 0x00750000UL,
0x00090000UL, 0x00830000UL, 0x002c0000UL, 0x001a0000UL, 0x001b0000UL, 0x006e0000UL, 0x005a0000UL, 0x00a00000UL,
0x00520000UL, 0x003b0000UL, 0x00d60000UL, 0x00b30000UL, 0x00290000UL, 0x00e30000UL, 0x002f0000UL, 0x00840000UL,
0x00530000UL, 0x00d10000UL, 0x00000000UL, 0x00ed0000UL, 0x00200000UL, 0x00fc0000UL, 0x00b10000UL, 0x005b0000UL,
0x006a0000UL, 0x00cb0000UL, 0x00be0000UL, 0x00390000UL, 0x004a0000UL, 0x004c0000UL, 0x00580000UL, 0x00cf0000UL,
0x00d00000UL, 0x00ef0000UL, 0x00aa0000UL, 0x00fb0000UL, 0x00430000UL, 0x004d0000UL, 0x00330000UL, 0x00850000UL,
0x00450000UL, 0x00f90000UL, 0x00020000UL, 0x007f0000UL, 0x00500000UL, 0x003c0000UL, 0x009f0000UL, 0x00a80000UL,
0x00510000UL, 0x00a30000UL, 0x00400000UL, 0x008f0000UL, 0x00920000UL, 0x009d0000UL, 0x00380000UL, 0x00f50000UL,
0x00bc0000UL, 0x00b60000UL, 0x00da0000UL, 0x00210000UL, 0x00100000UL, 0x00ff0000UL, 0x00f30000UL, 0x00d20000UL,
0x00cd0000UL, 0x000c0000UL, 0x00130000UL, 0x00ec0000UL, 0x005f0000UL, 0x00970000UL, 0x00440000UL, 0x00170000UL,
0x00c40000UL, 0x00a70000UL, 0x007e0000UL, 0x003d0000UL, 0x00640000UL, 0x005d0000UL, 0x00190000UL, 0x00730000UL,
0x00600000UL, 0x00810000UL, 0x004f0000UL, 0x00dc0000UL, 0x00220000UL, 0x002a0000UL, 0x00900000UL, 0x00880000UL,
0x00460000UL, 0x00ee0000UL, 0x00b80000UL, 0x00140000UL, 0x00de0000UL, 0x005e0000UL, 0x000b0000UL, 0x00db0000UL,
0x00e00000UL, 0x00320000UL, 0x003a0000UL, 0x000a0000UL, 0x00490000UL, 0x00060000UL, 0x00240000UL, 0x005c0000UL,
0x00c20000UL, 0x00d30000UL, 0x00ac0000UL, 0x00620000UL, 0x00910000UL, 0x00950000UL, 0x00e40000UL, 0x00790000UL,
0x00e70000UL, 0x00c80000UL, 0x00370000UL, 0x006d0000UL, 0x008d0000UL, 0x00d50000UL, 0x004e0000UL, 0x00a90000UL,
0x006c0000UL, 0x00560000UL, 0x00f40000UL, 0x00ea0000UL, 0x00650000UL, 0x007a0000UL, 0x00ae0000UL, 0x00080000UL,
0x00ba0000UL, 0x00780000UL, 0x00250000UL, 0x002e0000UL, 0x001c0000UL, 0x00a60000UL, 0x00b40000UL, 0x00c60000UL,
0x00e80000UL, 0x00dd0000UL, 0x00740000UL, 0x001f0000UL, 0x004b0000UL, 0x00bd0000UL, 0x008b0000UL, 0x008a0000UL,
0x00700000UL, 0x003e0000UL, 0x00b50000UL, 0x00660000UL, 0x00480000UL, 0x00030000UL, 0x00f60000UL, 0x000e0000UL,
0x00610000UL, 0x00350000UL, 0x00570000UL, 0x00b90000UL, 0x00860000UL, 0x00c10000UL, 0x001d0000UL, 0x009e0000UL,
0x00e10000UL, 0x00f80000UL, 0x00980000UL, 0x00110000UL, 0x00690000UL, 0x00d90000UL, 0x008e0000UL, 0x00940000UL,
0x009b0000UL, 0x001e0000UL, 0x00870000UL, 0x00e90000UL, 0x00ce0000UL, 0x00550000UL, 0x00280000UL, 0x00df0000UL,
0x008c0000UL, 0x00a10000UL, 0x00890000UL, 0x000d0000UL, 0x00bf0000UL, 0x00e60000UL, 0x00420000UL, 0x00680000UL,
0x00410000UL, 0x00990000UL, 0x002d0000UL, 0x000f0000UL, 0x00b00000UL, 0x00540000UL, 0x00bb0000UL, 0x00160000UL
};

static const ulong32 Te4_3[] = {
0x63000000UL, 0x7c000000UL, 0x77000000UL, 0x7b000000UL, 0xf2000000UL, 0x6b000000UL, 0x6f000000UL, 0xc5000000UL,
0x30000000UL, 0x01000000UL, 0x67000000UL, 0x2b000000UL, 0xfe000000UL, 0xd7000000UL, 0xab000000UL, 0x76000000UL,
0xca000000UL, 0x82000000UL, 0xc9000000UL, 0x7d000000UL, 0xfa000000UL, 0x59000000UL, 0x47000000UL, 0xf0000000UL,
0xad000000UL, 0xd4000000UL, 0xa2000000UL, 0xaf000000UL, 0x9c000000UL, 0xa4000000UL, 0x72000000UL, 0xc0000000UL,
0xb7000000UL, 0xfd000000UL, 0x93000000UL, 0x26000000UL, 0x36000000UL, 0x3f000000UL, 0xf7000000UL, 0xcc000000UL,
0x34000000UL, 0xa5000000UL, 0xe5000000UL, 0xf1000000UL, 0x71000000UL, 0xd8000000UL, 0x31000000UL, 0x15000000UL,
0x04000000UL, 0xc7000000UL, 0x23000000UL, 0xc3000000UL, 0x18000000UL, 0x96000000UL, 0x05000000UL, 0x9a000000UL,
0x07000000UL, 0x12000000UL, 0x80000000UL, 0xe2000000UL, 0xeb000000UL, 0x27000000UL, 0xb2000000UL, 0x75000000UL,
0x09000000UL, 0x83000000UL, 0x2c000000UL, 0x1a000000UL, 0x1b000000UL, 0x6e000000UL, 0x5a000000UL, 0xa0000000UL,
0x52000000UL, 0x3b000000UL, 0xd6000000UL, 0xb3000000UL, 0x29000000UL, 0xe3000000UL, 0x2f000000UL, 0x84000000UL,
0x53000000UL, 0xd1000000UL, 0x00000000UL, 0xed000000UL, 0x20000000UL, 0xfc000000UL, 0xb1000000UL, 0x5b000000UL,
0x6a000000UL, 0xcb000000UL, 0xbe000000UL, 0x39000000UL, 0x4a000000UL, 0x4c000000UL, 0x58000000UL, 0xcf000000UL,
0xd0000000UL, 0xef000000UL, 0xaa000000UL, 0xfb000000UL, 0x43000000UL, 0x4d000000UL, 0x33000000UL, 0x85000000UL,
0x45000000UL, 0xf9000000UL, 0x02000000UL, 0x7f000000UL, 0x50000000UL, 0x3c000000UL, 0x9f000000UL, 0xa8000000UL,
0x51000000UL, 0xa3000000UL, 0x40000000UL, 0x8f000000UL, 0x92000000UL, 0x9d000000UL, 0x38000000UL, 0xf5000000UL,
0xbc000000UL, 0xb6000000UL, 0xda000000UL, 0x21000000UL, 0x10000000UL, 0xff000000UL, 0xf3000000UL, 0xd2000000UL,
0xcd000000UL, 0x0c000000UL, 0x13000000UL, 0xec000000UL, 0x5f000000UL, 0x97000000UL, 0x44000000UL, 0x17000000UL,
0xc4000000UL, 0xa7000000UL, 0x7e000000UL, 0x3d000000UL, 0x64000000UL, 0x5d000000UL, 0x19000000UL, 0x73000000UL,
0x60000000UL, 0x81000000UL, 0x4f000000UL, 0xdc000000UL, 0x22000000UL, 0x2a000000UL, 0x90000000UL, 0x88000000UL,
0x46000000UL, 0xee000000UL, 0xb8000000UL, 0x14000000UL, 0xde000000UL, 0x5e000000UL, 0x0b000000UL, 0xdb000000UL,
0xe0000000UL, 0x32000000UL, 0x3a000000UL, 0x0a000000UL, 0x49000000UL, 0x06000000UL, 0x24000000UL, 0x5c000000UL,
0xc2000000UL, 0xd3000000UL, 0xac000000UL, 0x62000000UL, 0x91000000UL, 0x95000000UL, 0xe4000000UL, 0x79000000UL,
0xe7000000UL, 0xc8000000UL, 0x37000000UL, 0x6d000000UL, 0x8d000000UL, 0xd5000000UL, 0x4e000000UL, 0xa9000000UL,
0x6c000000UL, 0x56000000UL, 0xf4000000UL, 0xea000000UL, 0x65000000UL, 0x7a000000UL, 0xae000000UL, 0x08000000UL,
0xba000000UL, 0x78000000UL, 0x25000000UL, 0x2e000000UL, 0x1c000000UL, 0xa6000000UL, 0xb4000000UL, 0xc6000000UL,
0xe8000000UL, 0xdd000000UL, 0x74000000UL, 0x1f000000UL, 0x4b000000UL, 0xbd000000UL, 0x8b000000UL, 0x8a000000UL,
0x70000000UL, 0x3e000000UL, 0xb5000000UL, 0x66000000UL, 0x48000000UL, 0x03000000UL, 0xf6000000UL, 0x0e000000UL,
0x61000000UL, 0x35000000UL, 0x57000000UL, 0xb9000000UL, 0x86000000UL, 0xc1000000UL, 0x1d000000UL, 0x9e000000UL,
0xe1000000UL, 0xf8000000UL, 0x98000000UL, 0x11000000UL, 0x69000000UL, 0xd9000000UL, 0x8e000000UL, 0x94000000UL,
0x9b000000UL, 0x1e000000UL, 0x87000000UL, 0xe9000000UL, 0xce000000UL, 0x55000000UL, 0x28000000UL, 0xdf000000UL,
0x8c000000UL, 0xa1000000UL, 0x89000000UL, 0x0d000000UL, 0xbf000000UL, 0xe6000000UL, 0x42000000UL, 0x68000000UL,
0x41000000UL, 0x99000000UL, 0x2d000000UL, 0x0f000000UL, 0xb0000000UL, 0x54000000UL, 0xbb000000UL, 0x16000000UL
};
#endif /* pelimac */

#ifndef ENCRYPT_ONLY

static const ulong32 TD1[256] = {
    0x5051f4a7UL, 0x537e4165UL, 0xc31a17a4UL, 0x963a275eUL,
    0xcb3bab6bUL, 0xf11f9d45UL, 0xabacfa58UL, 0x934be303UL,
    0x552030faUL, 0xf6ad766dUL, 0x9188cc76UL, 0x25f5024cUL,
    0xfc4fe5d7UL, 0xd7c52acbUL, 0x80263544UL, 0x8fb562a3UL,
    0x49deb15aUL, 0x6725ba1bUL, 0x9845ea0eUL, 0xe15dfec0UL,
    0x02c32f75UL, 0x12814cf0UL, 0xa38d4697UL, 0xc66bd3f9UL,
    0xe7038f5fUL, 0x9515929cUL, 0xebbf6d7aUL, 0xda955259UL,
    0x2dd4be83UL, 0xd3587421UL, 0x2949e069UL, 0x448ec9c8UL,
    0x6a75c289UL, 0x78f48e79UL, 0x6b99583eUL, 0xdd27b971UL,
    0xb6bee14fUL, 0x17f088adUL, 0x66c920acUL, 0xb47dce3aUL,
    0x1863df4aUL, 0x82e51a31UL, 0x60975133UL, 0x4562537fUL,
    0xe0b16477UL, 0x84bb6baeUL, 0x1cfe81a0UL, 0x94f9082bUL,
    0x58704868UL, 0x198f45fdUL, 0x8794de6cUL, 0xb7527bf8UL,
    0x23ab73d3UL, 0xe2724b02UL, 0x57e31f8fUL, 0x2a6655abUL,
    0x07b2eb28UL, 0x032fb5c2UL, 0x9a86c57bUL, 0xa5d33708UL,
    0xf2302887UL, 0xb223bfa5UL, 0xba02036aUL, 0x5ced1682UL,
    0x2b8acf1cUL, 0x92a779b4UL, 0xf0f307f2UL, 0xa14e69e2UL,
    0xcd65daf4UL, 0xd50605beUL, 0x1fd13462UL, 0x8ac4a6feUL,
    0x9d342e53UL, 0xa0a2f355UL, 0x32058ae1UL, 0x75a4f6ebUL,
    0x390b83ecUL, 0xaa4060efUL, 0x065e719fUL, 0x51bd6e10UL,
    0xf93e218aUL, 0x3d96dd06UL, 0xaedd3e05UL, 0x464de6bdUL,
    0xb591548dUL, 0x0571c45dUL, 0x6f0406d4UL, 0xff605015UL,
    0x241998fbUL, 0x97d6bde9UL, 0xcc894043UL, 0x7767d99eUL,
    0xbdb0e842UL, 0x8807898bUL, 0x38e7195bUL, 0xdb79c8eeUL,
    0x47a17c0aUL, 0xe97c420fUL, 0xc9f8841eUL, 0x00000000UL,
    0x83098086UL, 0x48322bedUL, 0xac1e1170UL, 0x4e6c5a72UL,
    0xfbfd0effUL, 0x560f8538UL, 0x1e3daed5UL, 0x27362d39UL,
    0x640a0fd9UL, 0x21685ca6UL, 0xd19b5b54UL, 0x3a24362eUL,
    0xb10c0a67UL, 0x0f9357e7UL, 0xd2b4ee96UL, 0x9e1b9b91UL,
    0x4f80c0c5UL, 0xa261dc20UL, 0x695a774bUL, 0x161c121aUL,
    0x0ae293baUL, 0xe5c0a02aUL, 0x433c22e0UL, 0x1d121b17UL,
    0x0b0e090dUL, 0xadf28bc7UL, 0xb92db6a8UL, 0xc8141ea9UL,
    0x8557f119UL, 0x4caf7507UL, 0xbbee99ddUL, 0xfda37f60UL,
    0x9ff70126UL, 0xbc5c72f5UL, 0xc544663bUL, 0x345bfb7eUL,
    0x768b4329UL, 0xdccb23c6UL, 0x68b6edfcUL, 0x63b8e4f1UL,
    0xcad731dcUL, 0x10426385UL, 0x40139722UL, 0x2084c611UL,
    0x7d854a24UL, 0xf8d2bb3dUL, 0x11aef932UL, 0x6dc729a1UL,
    0x4b1d9e2fUL, 0xf3dcb230UL, 0xec0d8652UL, 0xd077c1e3UL,
    0x6c2bb316UL, 0x99a970b9UL, 0xfa119448UL, 0x2247e964UL,
    0xc4a8fc8cUL, 0x1aa0f03fUL, 0xd8567d2cUL, 0xef223390UL,
    0xc787494eUL, 0xc1d938d1UL, 0xfe8ccaa2UL, 0x3698d40bUL,
    0xcfa6f581UL, 0x28a57adeUL, 0x26dab78eUL, 0xa43fadbfUL,
    0xe42c3a9dUL, 0x0d507892UL, 0x9b6a5fccUL, 0x62547e46UL,
    0xc2f68d13UL, 0xe890d8b8UL, 0x5e2e39f7UL, 0xf582c3afUL,
    0xbe9f5d80UL, 0x7c69d093UL, 0xa96fd52dUL, 0xb3cf2512UL,
    0x3bc8ac99UL, 0xa710187dUL, 0x6ee89c63UL, 0x7bdb3bbbUL,
    0x09cd2678UL, 0xf46e5918UL, 0x01ec9ab7UL, 0xa8834f9aUL,
    0x65e6956eUL, 0x7eaaffe6UL, 0x0821bccfUL, 0xe6ef15e8UL,
    0xd9bae79bUL, 0xce4a6f36UL, 0xd4ea9f09UL, 0xd629b07cUL,
    0xaf31a4b2UL, 0x312a3f23UL, 0x30c6a594UL, 0xc035a266UL,
    0x37744ebcUL, 0xa6fc82caUL, 0xb0e090d0UL, 0x1533a7d8UL,
    0x4af10498UL, 0xf741ecdaUL, 0x0e7fcd50UL, 0x2f1791f6UL,
    0x8d764dd6UL, 0x4d43efb0UL, 0x54ccaa4dUL, 0xdfe49604UL,
    0xe39ed1b5UL, 0x1b4c6a88UL, 0xb8c12c1fUL, 0x7f466551UL,
    0x049d5eeaUL, 0x5d018c35UL, 0x73fa8774UL, 0x2efb0b41UL,
    0x5ab3671dUL, 0x5292dbd2UL, 0x33e91056UL, 0x136dd647UL,
    0x8c9ad761UL, 0x7a37a10cUL, 0x8e59f814UL, 0x89eb133cUL,
    0xeecea927UL, 0x35b761c9UL, 0xede11ce5UL, 0x3c7a47b1UL,
    0x599cd2dfUL, 0x3f55f273UL, 0x791814ceUL, 0xbf73c737UL,
    0xea53f7cdUL, 0x5b5ffdaaUL, 0x14df3d6fUL, 0x867844dbUL,
    0x81caaff3UL, 0x3eb968c4UL, 0x2c382434UL, 0x5fc2a340UL,
    0x72161dc3UL, 0x0cbce225UL, 0x8b283c49UL, 0x41ff0d95UL,
    0x7139a801UL, 0xde080cb3UL, 0x9cd8b4e4UL, 0x906456c1UL,
    0x617bcb84UL, 0x70d532b6UL, 0x74486c5cUL, 0x42d0b857UL,
};
static const ulong32 TD2[256] = {
    0xa75051f4UL, 0x65537e41UL, 0xa4c31a17UL, 0x5e963a27UL,
    0x6bcb3babUL, 0x45f11f9dUL, 0x58abacfaUL, 0x03934be3UL,
    0xfa552030UL, 0x6df6ad76UL, 0x769188ccUL, 0x4c25f502UL,
    0xd7fc4fe5UL, 0xcbd7c52aUL, 0x44802635UL, 0xa38fb562UL,
    0x5a49deb1UL, 0x1b6725baUL, 0x0e9845eaUL, 0xc0e15dfeUL,
    0x7502c32fUL, 0xf012814cUL, 0x97a38d46UL, 0xf9c66bd3UL,
    0x5fe7038fUL, 0x9c951592UL, 0x7aebbf6dUL, 0x59da9552UL,
    0x832dd4beUL, 0x21d35874UL, 0x692949e0UL, 0xc8448ec9UL,
    0x896a75c2UL, 0x7978f48eUL, 0x3e6b9958UL, 0x71dd27b9UL,
    0x4fb6bee1UL, 0xad17f088UL, 0xac66c920UL, 0x3ab47dceUL,
    0x4a1863dfUL, 0x3182e51aUL, 0x33609751UL, 0x7f456253UL,
    0x77e0b164UL, 0xae84bb6bUL, 0xa01cfe81UL, 0x2b94f908UL,
    0x68587048UL, 0xfd198f45UL, 0x6c8794deUL, 0xf8b7527bUL,
    0xd323ab73UL, 0x02e2724bUL, 0x8f57e31fUL, 0xab2a6655UL,
    0x2807b2ebUL, 0xc2032fb5UL, 0x7b9a86c5UL, 0x08a5d337UL,
    0x87f23028UL, 0xa5b223bfUL, 0x6aba0203UL, 0x825ced16UL,
    0x1c2b8acfUL, 0xb492a779UL, 0xf2f0f307UL, 0xe2a14e69UL,
    0xf4cd65daUL, 0xbed50605UL, 0x621fd134UL, 0xfe8ac4a6UL,
    0x539d342eUL, 0x55a0a2f3UL, 0xe132058aUL, 0xeb75a4f6UL,
    0xec390b83UL, 0xefaa4060UL, 0x9f065e71UL, 0x1051bd6eUL,
    0x8af93e21UL, 0x063d96ddUL, 0x05aedd3eUL, 0xbd464de6UL,
    0x8db59154UL, 0x5d0571c4UL, 0xd46f0406UL, 0x15ff6050UL,
    0xfb241998UL, 0xe997d6bdUL, 0x43cc8940UL, 0x9e7767d9UL,
    0x42bdb0e8UL, 0x8b880789UL, 0x5b38e719UL, 0xeedb79c8UL,
    0x0a47a17cUL, 0x0fe97c42UL, 0x1ec9f884UL, 0x00000000UL,
    0x86830980UL, 0xed48322bUL, 0x70ac1e11UL, 0x724e6c5aUL,
    0xfffbfd0eUL, 0x38560f85UL, 0xd51e3daeUL, 0x3927362dUL,
    0xd9640a0fUL, 0xa621685cUL, 0x54d19b5bUL, 0x2e3a2436UL,
    0x67b10c0aUL, 0xe70f9357UL, 0x96d2b4eeUL, 0x919e1b9bUL,
    0xc54f80c0UL, 0x20a261dcUL, 0x4b695a77UL, 0x1a161c12UL,
    0xba0ae293UL, 0x2ae5c0a0UL, 0xe0433c22UL, 0x171d121bUL,
    0x0d0b0e09UL, 0xc7adf28bUL, 0xa8b92db6UL, 0xa9c8141eUL,
    0x198557f1UL, 0x074caf75UL, 0xddbbee99UL, 0x60fda37fUL,
    0x269ff701UL, 0xf5bc5c72UL, 0x3bc54466UL, 0x7e345bfbUL,
    0x29768b43UL, 0xc6dccb23UL, 0xfc68b6edUL, 0xf163b8e4UL,
    0xdccad731UL, 0x85104263UL, 0x22401397UL, 0x112084c6UL,
    0x247d854aUL, 0x3df8d2bbUL, 0x3211aef9UL, 0xa16dc729UL,
    0x2f4b1d9eUL, 0x30f3dcb2UL, 0x52ec0d86UL, 0xe3d077c1UL,
    0x166c2bb3UL, 0xb999a970UL, 0x48fa1194UL, 0x642247e9UL,
    0x8cc4a8fcUL, 0x3f1aa0f0UL, 0x2cd8567dUL, 0x90ef2233UL,
    0x4ec78749UL, 0xd1c1d938UL, 0xa2fe8ccaUL, 0x0b3698d4UL,
    0x81cfa6f5UL, 0xde28a57aUL, 0x8e26dab7UL, 0xbfa43fadUL,
    0x9de42c3aUL, 0x920d5078UL, 0xcc9b6a5fUL, 0x4662547eUL,
    0x13c2f68dUL, 0xb8e890d8UL, 0xf75e2e39UL, 0xaff582c3UL,
    0x80be9f5dUL, 0x937c69d0UL, 0x2da96fd5UL, 0x12b3cf25UL,
    0x993bc8acUL, 0x7da71018UL, 0x636ee89cUL, 0xbb7bdb3bUL,
    0x7809cd26UL, 0x18f46e59UL, 0xb701ec9aUL, 0x9aa8834fUL,
    0x6e65e695UL, 0xe67eaaffUL, 0xcf0821bcUL, 0xe8e6ef15UL,
    0x9bd9bae7UL, 0x36ce4a6fUL, 0x09d4ea9fUL, 0x7cd629b0UL,
    0xb2af31a4UL, 0x23312a3fUL, 0x9430c6a5UL, 0x66c035a2UL,
    0xbc37744eUL, 0xcaa6fc82UL, 0xd0b0e090UL, 0xd81533a7UL,
    0x984af104UL, 0xdaf741ecUL, 0x500e7fcdUL, 0xf62f1791UL,
    0xd68d764dUL, 0xb04d43efUL, 0x4d54ccaaUL, 0x04dfe496UL,
    0xb5e39ed1UL, 0x881b4c6aUL, 0x1fb8c12cUL, 0x517f4665UL,
    0xea049d5eUL, 0x355d018cUL, 0x7473fa87UL, 0x412efb0bUL,
    0x1d5ab367UL, 0xd25292dbUL, 0x5633e910UL, 0x47136dd6UL,
    0x618c9ad7UL, 0x0c7a37a1UL, 0x148e59f8UL, 0x3c89eb13UL,
    0x27eecea9UL, 0xc935b761UL, 0xe5ede11cUL, 0xb13c7a47UL,
    0xdf599cd2UL, 0x733f55f2UL, 0xce791814UL, 0x37bf73c7UL,
    0xcdea53f7UL, 0xaa5b5ffdUL, 0x6f14df3dUL, 0xdb867844UL,
    0xf381caafUL, 0xc43eb968UL, 0x342c3824UL, 0x405fc2a3UL,
    0xc372161dUL, 0x250cbce2UL, 0x498b283cUL, 0x9541ff0dUL,
    0x017139a8UL, 0xb3de080cUL, 0xe49cd8b4UL, 0xc1906456UL,
    0x84617bcbUL, 0xb670d532UL, 0x5c74486cUL, 0x5742d0b8UL,
};
static const ulong32 TD3[256] = {
    0xf4a75051UL, 0x4165537eUL, 0x17a4c31aUL, 0x275e963aUL,
    0xab6bcb3bUL, 0x9d45f11fUL, 0xfa58abacUL, 0xe303934bUL,
    0x30fa5520UL, 0x766df6adUL, 0xcc769188UL, 0x024c25f5UL,
    0xe5d7fc4fUL, 0x2acbd7c5UL, 0x35448026UL, 0x62a38fb5UL,
    0xb15a49deUL, 0xba1b6725UL, 0xea0e9845UL, 0xfec0e15dUL,
    0x2f7502c3UL, 0x4cf01281UL, 0x4697a38dUL, 0xd3f9c66bUL,
    0x8f5fe703UL, 0x929c9515UL, 0x6d7aebbfUL, 0x5259da95UL,
    0xbe832dd4UL, 0x7421d358UL, 0xe0692949UL, 0xc9c8448eUL,
    0xc2896a75UL, 0x8e7978f4UL, 0x583e6b99UL, 0xb971dd27UL,
    0xe14fb6beUL, 0x88ad17f0UL, 0x20ac66c9UL, 0xce3ab47dUL,
    0xdf4a1863UL, 0x1a3182e5UL, 0x51336097UL, 0x537f4562UL,
    0x6477e0b1UL, 0x6bae84bbUL, 0x81a01cfeUL, 0x082b94f9UL,
    0x48685870UL, 0x45fd198fUL, 0xde6c8794UL, 0x7bf8b752UL,
    0x73d323abUL, 0x4b02e272UL, 0x1f8f57e3UL, 0x55ab2a66UL,
    0xeb2807b2UL, 0xb5c2032fUL, 0xc57b9a86UL, 0x3708a5d3UL,
    0x2887f230UL, 0xbfa5b223UL, 0x036aba02UL, 0x16825cedUL,
    0xcf1c2b8aUL, 0x79b492a7UL, 0x07f2f0f3UL, 0x69e2a14eUL,
    0xdaf4cd65UL, 0x05bed506UL, 0x34621fd1UL, 0xa6fe8ac4UL,
    0x2e539d34UL, 0xf355a0a2UL, 0x8ae13205UL, 0xf6eb75a4UL,
    0x83ec390bUL, 0x60efaa40UL, 0x719f065eUL, 0x6e1051bdUL,
    0x218af93eUL, 0xdd063d96UL, 0x3e05aeddUL, 0xe6bd464dUL,
    0x548db591UL, 0xc45d0571UL, 0x06d46f04UL, 0x5015ff60UL,
    0x98fb2419UL, 0xbde997d6UL, 0x4043cc89UL, 0xd99e7767UL,
    0xe842bdb0UL, 0x898b8807UL, 0x195b38e7UL, 0xc8eedb79UL,
    0x7c0a47a1UL, 0x420fe97cUL, 0x841ec9f8UL, 0x00000000UL,
    0x80868309UL, 0x2bed4832UL, 0x1170ac1eUL, 0x5a724e6cUL,
    0x0efffbfdUL, 0x8538560fUL, 0xaed51e3dUL, 0x2d392736UL,
    0x0fd9640aUL, 0x5ca62168UL, 0x5b54d19bUL, 0x362e3a24UL,
    0x0a67b10cUL, 0x57e70f93UL, 0xee96d2b4UL, 0x9b919e1bUL,
    0xc0c54f80UL, 0xdc20a261UL, 0x774b695aUL, 0x121a161cUL,
    0x93ba0ae2UL, 0xa02ae5c0UL, 0x22e0433cUL, 0x1b171d12UL,
    0x090d0b0eUL, 0x8bc7adf2UL, 0xb6a8b92dUL, 0x1ea9c814UL,
    0xf1198557UL, 0x75074cafUL, 0x99ddbbeeUL, 0x7f60fda3UL,
    0x01269ff7UL, 0x72f5bc5cUL, 0x663bc544UL, 0xfb7e345bUL,
    0x4329768bUL, 0x23c6dccbUL, 0xedfc68b6UL, 0xe4f163b8UL,
    0x31dccad7UL, 0x63851042UL, 0x97224013UL, 0xc6112084UL,
    0x4a247d85UL, 0xbb3df8d2UL, 0xf93211aeUL, 0x29a16dc7UL,
    0x9e2f4b1dUL, 0xb230f3dcUL, 0x8652ec0dUL, 0xc1e3d077UL,
    0xb3166c2bUL, 0x70b999a9UL, 0x9448fa11UL, 0xe9642247UL,
    0xfc8cc4a8UL, 0xf03f1aa0UL, 0x7d2cd856UL, 0x3390ef22UL,
    0x494ec787UL, 0x38d1c1d9UL, 0xcaa2fe8cUL, 0xd40b3698UL,
    0xf581cfa6UL, 0x7ade28a5UL, 0xb78e26daUL, 0xadbfa43fUL,
    0x3a9de42cUL, 0x78920d50UL, 0x5fcc9b6aUL, 0x7e466254UL,
    0x8d13c2f6UL, 0xd8b8e890UL, 0x39f75e2eUL, 0xc3aff582UL,
    0x5d80be9fUL, 0xd0937c69UL, 0xd52da96fUL, 0x2512b3cfUL,
    0xac993bc8UL, 0x187da710UL, 0x9c636ee8UL, 0x3bbb7bdbUL,
    0x267809cdUL, 0x5918f46eUL, 0x9ab701ecUL, 0x4f9aa883UL,
    0x956e65e6UL, 0xffe67eaaUL, 0xbccf0821UL, 0x15e8e6efUL,
    0xe79bd9baUL, 0x6f36ce4aUL, 0x9f09d4eaUL, 0xb07cd629UL,
    0xa4b2af31UL, 0x3f23312aUL, 0xa59430c6UL, 0xa266c035UL,
    0x4ebc3774UL, 0x82caa6fcUL, 0x90d0b0e0UL, 0xa7d81533UL,
    0x04984af1UL, 0xecdaf741UL, 0xcd500e7fUL, 0x91f62f17UL,
    0x4dd68d76UL, 0xefb04d43UL, 0xaa4d54ccUL, 0x9604dfe4UL,
    0xd1b5e39eUL, 0x6a881b4cUL, 0x2c1fb8c1UL, 0x65517f46UL,
    0x5eea049dUL, 0x8c355d01UL, 0x877473faUL, 0x0b412efbUL,
    0x671d5ab3UL, 0xdbd25292UL, 0x105633e9UL, 0xd647136dUL,
    0xd7618c9aUL, 0xa10c7a37UL, 0xf8148e59UL, 0x133c89ebUL,
    0xa927eeceUL, 0x61c935b7UL, 0x1ce5ede1UL, 0x47b13c7aUL,
    0xd2df599cUL, 0xf2733f55UL, 0x14ce7918UL, 0xc737bf73UL,
    0xf7cdea53UL, 0xfdaa5b5fUL, 0x3d6f14dfUL, 0x44db8678UL,
    0xaff381caUL, 0x68c43eb9UL, 0x24342c38UL, 0xa3405fc2UL,
    0x1dc37216UL, 0xe2250cbcUL, 0x3c498b28UL, 0x0d9541ffUL,
    0xa8017139UL, 0x0cb3de08UL, 0xb4e49cd8UL, 0x56c19064UL,
    0xcb84617bUL, 0x32b670d5UL, 0x6c5c7448UL, 0xb85742d0UL,
};

static const ulong32 Tks0[] = {
0x00000000UL, 0x0e090d0bUL, 0x1c121a16UL, 0x121b171dUL, 0x3824342cUL, 0x362d3927UL, 0x24362e3aUL, 0x2a3f2331UL,
0x70486858UL, 0x7e416553UL, 0x6c5a724eUL, 0x62537f45UL, 0x486c5c74UL, 0x4665517fUL, 0x547e4662UL, 0x5a774b69UL,
0xe090d0b0UL, 0xee99ddbbUL, 0xfc82caa6UL, 0xf28bc7adUL, 0xd8b4e49cUL, 0xd6bde997UL, 0xc4a6fe8aUL, 0xcaaff381UL,
0x90d8b8e8UL, 0x9ed1b5e3UL, 0x8ccaa2feUL, 0x82c3aff5UL, 0xa8fc8cc4UL, 0xa6f581cfUL, 0xb4ee96d2UL, 0xbae79bd9UL,
0xdb3bbb7bUL, 0xd532b670UL, 0xc729a16dUL, 0xc920ac66UL, 0xe31f8f57UL, 0xed16825cUL, 0xff0d9541UL, 0xf104984aUL,
0xab73d323UL, 0xa57ade28UL, 0xb761c935UL, 0xb968c43eUL, 0x9357e70fUL, 0x9d5eea04UL, 0x8f45fd19UL, 0x814cf012UL,
0x3bab6bcbUL, 0x35a266c0UL, 0x27b971ddUL, 0x29b07cd6UL, 0x038f5fe7UL, 0x0d8652ecUL, 0x1f9d45f1UL, 0x119448faUL,
0x4be30393UL, 0x45ea0e98UL, 0x57f11985UL, 0x59f8148eUL, 0x73c737bfUL, 0x7dce3ab4UL, 0x6fd52da9UL, 0x61dc20a2UL,
0xad766df6UL, 0xa37f60fdUL, 0xb16477e0UL, 0xbf6d7aebUL, 0x955259daUL, 0x9b5b54d1UL, 0x894043ccUL, 0x87494ec7UL,
0xdd3e05aeUL, 0xd33708a5UL, 0xc12c1fb8UL, 0xcf2512b3UL, 0xe51a3182UL, 0xeb133c89UL, 0xf9082b94UL, 0xf701269fUL,
0x4de6bd46UL, 0x43efb04dUL, 0x51f4a750UL, 0x5ffdaa5bUL, 0x75c2896aUL, 0x7bcb8461UL, 0x69d0937cUL, 0x67d99e77UL,
0x3daed51eUL, 0x33a7d815UL, 0x21bccf08UL, 0x2fb5c203UL, 0x058ae132UL, 0x0b83ec39UL, 0x1998fb24UL, 0x1791f62fUL,
0x764dd68dUL, 0x7844db86UL, 0x6a5fcc9bUL, 0x6456c190UL, 0x4e69e2a1UL, 0x4060efaaUL, 0x527bf8b7UL, 0x5c72f5bcUL,
0x0605bed5UL, 0x080cb3deUL, 0x1a17a4c3UL, 0x141ea9c8UL, 0x3e218af9UL, 0x302887f2UL, 0x223390efUL, 0x2c3a9de4UL,
0x96dd063dUL, 0x98d40b36UL, 0x8acf1c2bUL, 0x84c61120UL, 0xaef93211UL, 0xa0f03f1aUL, 0xb2eb2807UL, 0xbce2250cUL,
0xe6956e65UL, 0xe89c636eUL, 0xfa877473UL, 0xf48e7978UL, 0xdeb15a49UL, 0xd0b85742UL, 0xc2a3405fUL, 0xccaa4d54UL,
0x41ecdaf7UL, 0x4fe5d7fcUL, 0x5dfec0e1UL, 0x53f7cdeaUL, 0x79c8eedbUL, 0x77c1e3d0UL, 0x65daf4cdUL, 0x6bd3f9c6UL,
0x31a4b2afUL, 0x3fadbfa4UL, 0x2db6a8b9UL, 0x23bfa5b2UL, 0x09808683UL, 0x07898b88UL, 0x15929c95UL, 0x1b9b919eUL,
0xa17c0a47UL, 0xaf75074cUL, 0xbd6e1051UL, 0xb3671d5aUL, 0x99583e6bUL, 0x97513360UL, 0x854a247dUL, 0x8b432976UL,
0xd134621fUL, 0xdf3d6f14UL, 0xcd267809UL, 0xc32f7502UL, 0xe9105633UL, 0xe7195b38UL, 0xf5024c25UL, 0xfb0b412eUL,
0x9ad7618cUL, 0x94de6c87UL, 0x86c57b9aUL, 0x88cc7691UL, 0xa2f355a0UL, 0xacfa58abUL, 0xbee14fb6UL, 0xb0e842bdUL,
0xea9f09d4UL, 0xe49604dfUL, 0xf68d13c2UL, 0xf8841ec9UL, 0xd2bb3df8UL, 0xdcb230f3UL, 0xcea927eeUL, 0xc0a02ae5UL,
0x7a47b13cUL, 0x744ebc37UL, 0x6655ab2aUL, 0x685ca621UL, 0x42638510UL, 0x4c6a881bUL, 0x5e719f06UL, 0x5078920dUL,
0x0a0fd964UL, 0x0406d46fUL, 0x161dc372UL, 0x1814ce79UL, 0x322bed48UL, 0x3c22e043UL, 0x2e39f75eUL, 0x2030fa55UL,
0xec9ab701UL, 0xe293ba0aUL, 0xf088ad17UL, 0xfe81a01cUL, 0xd4be832dUL, 0xdab78e26UL, 0xc8ac993bUL, 0xc6a59430UL,
0x9cd2df59UL, 0x92dbd252UL, 0x80c0c54fUL, 0x8ec9c844UL, 0xa4f6eb75UL, 0xaaffe67eUL, 0xb8e4f163UL, 0xb6edfc68UL,
0x0c0a67b1UL, 0x02036abaUL, 0x10187da7UL, 0x1e1170acUL, 0x342e539dUL, 0x3a275e96UL, 0x283c498bUL, 0x26354480UL,
0x7c420fe9UL, 0x724b02e2UL, 0x605015ffUL, 0x6e5918f4UL, 0x44663bc5UL, 0x4a6f36ceUL, 0x587421d3UL, 0x567d2cd8UL,
0x37a10c7aUL, 0x39a80171UL, 0x2bb3166cUL, 0x25ba1b67UL, 0x0f853856UL, 0x018c355dUL, 0x13972240UL, 0x1d9e2f4bUL,
0x47e96422UL, 0x49e06929UL, 0x5bfb7e34UL, 0x55f2733fUL, 0x7fcd500eUL, 0x71c45d05UL, 0x63df4a18UL, 0x6dd64713UL,
0xd731dccaUL, 0xd938d1c1UL, 0xcb23c6dcUL, 0xc52acbd7UL, 0xef15e8e6UL, 0xe11ce5edUL, 0xf307f2f0UL, 0xfd0efffbUL,
0xa779b492UL, 0xa970b999UL, 0xbb6bae84UL, 0xb562a38fUL, 0x9f5d80beUL, 0x91548db5UL, 0x834f9aa8UL, 0x8d4697a3UL
};

static const ulong32 Tks1[] = {
0x00000000UL, 0x0b0e090dUL, 0x161c121aUL, 0x1d121b17UL, 0x2c382434UL, 0x27362d39UL, 0x3a24362eUL, 0x312a3f23UL,
0x58704868UL, 0x537e4165UL, 0x4e6c5a72UL, 0x4562537fUL, 0x74486c5cUL, 0x7f466551UL, 0x62547e46UL, 0x695a774bUL,
0xb0e090d0UL, 0xbbee99ddUL, 0xa6fc82caUL, 0xadf28bc7UL, 0x9cd8b4e4UL, 0x97d6bde9UL, 0x8ac4a6feUL, 0x81caaff3UL,
0xe890d8b8UL, 0xe39ed1b5UL, 0xfe8ccaa2UL, 0xf582c3afUL, 0xc4a8fc8cUL, 0xcfa6f581UL, 0xd2b4ee96UL, 0xd9bae79bUL,
0x7bdb3bbbUL, 0x70d532b6UL, 0x6dc729a1UL, 0x66c920acUL, 0x57e31f8fUL, 0x5ced1682UL, 0x41ff0d95UL, 0x4af10498UL,
0x23ab73d3UL, 0x28a57adeUL, 0x35b761c9UL, 0x3eb968c4UL, 0x0f9357e7UL, 0x049d5eeaUL, 0x198f45fdUL, 0x12814cf0UL,
0xcb3bab6bUL, 0xc035a266UL, 0xdd27b971UL, 0xd629b07cUL, 0xe7038f5fUL, 0xec0d8652UL, 0xf11f9d45UL, 0xfa119448UL,
0x934be303UL, 0x9845ea0eUL, 0x8557f119UL, 0x8e59f814UL, 0xbf73c737UL, 0xb47dce3aUL, 0xa96fd52dUL, 0xa261dc20UL,
0xf6ad766dUL, 0xfda37f60UL, 0xe0b16477UL, 0xebbf6d7aUL, 0xda955259UL, 0xd19b5b54UL, 0xcc894043UL, 0xc787494eUL,
0xaedd3e05UL, 0xa5d33708UL, 0xb8c12c1fUL, 0xb3cf2512UL, 0x82e51a31UL, 0x89eb133cUL, 0x94f9082bUL, 0x9ff70126UL,
0x464de6bdUL, 0x4d43efb0UL, 0x5051f4a7UL, 0x5b5ffdaaUL, 0x6a75c289UL, 0x617bcb84UL, 0x7c69d093UL, 0x7767d99eUL,
0x1e3daed5UL, 0x1533a7d8UL, 0x0821bccfUL, 0x032fb5c2UL, 0x32058ae1UL, 0x390b83ecUL, 0x241998fbUL, 0x2f1791f6UL,
0x8d764dd6UL, 0x867844dbUL, 0x9b6a5fccUL, 0x906456c1UL, 0xa14e69e2UL, 0xaa4060efUL, 0xb7527bf8UL, 0xbc5c72f5UL,
0xd50605beUL, 0xde080cb3UL, 0xc31a17a4UL, 0xc8141ea9UL, 0xf93e218aUL, 0xf2302887UL, 0xef223390UL, 0xe42c3a9dUL,
0x3d96dd06UL, 0x3698d40bUL, 0x2b8acf1cUL, 0x2084c611UL, 0x11aef932UL, 0x1aa0f03fUL, 0x07b2eb28UL, 0x0cbce225UL,
0x65e6956eUL, 0x6ee89c63UL, 0x73fa8774UL, 0x78f48e79UL, 0x49deb15aUL, 0x42d0b857UL, 0x5fc2a340UL, 0x54ccaa4dUL,
0xf741ecdaUL, 0xfc4fe5d7UL, 0xe15dfec0UL, 0xea53f7cdUL, 0xdb79c8eeUL, 0xd077c1e3UL, 0xcd65daf4UL, 0xc66bd3f9UL,
0xaf31a4b2UL, 0xa43fadbfUL, 0xb92db6a8UL, 0xb223bfa5UL, 0x83098086UL, 0x8807898bUL, 0x9515929cUL, 0x9e1b9b91UL,
0x47a17c0aUL, 0x4caf7507UL, 0x51bd6e10UL, 0x5ab3671dUL, 0x6b99583eUL, 0x60975133UL, 0x7d854a24UL, 0x768b4329UL,
0x1fd13462UL, 0x14df3d6fUL, 0x09cd2678UL, 0x02c32f75UL, 0x33e91056UL, 0x38e7195bUL, 0x25f5024cUL, 0x2efb0b41UL,
0x8c9ad761UL, 0x8794de6cUL, 0x9a86c57bUL, 0x9188cc76UL, 0xa0a2f355UL, 0xabacfa58UL, 0xb6bee14fUL, 0xbdb0e842UL,
0xd4ea9f09UL, 0xdfe49604UL, 0xc2f68d13UL, 0xc9f8841eUL, 0xf8d2bb3dUL, 0xf3dcb230UL, 0xeecea927UL, 0xe5c0a02aUL,
0x3c7a47b1UL, 0x37744ebcUL, 0x2a6655abUL, 0x21685ca6UL, 0x10426385UL, 0x1b4c6a88UL, 0x065e719fUL, 0x0d507892UL,
0x640a0fd9UL, 0x6f0406d4UL, 0x72161dc3UL, 0x791814ceUL, 0x48322bedUL, 0x433c22e0UL, 0x5e2e39f7UL, 0x552030faUL,
0x01ec9ab7UL, 0x0ae293baUL, 0x17f088adUL, 0x1cfe81a0UL, 0x2dd4be83UL, 0x26dab78eUL, 0x3bc8ac99UL, 0x30c6a594UL,
0x599cd2dfUL, 0x5292dbd2UL, 0x4f80c0c5UL, 0x448ec9c8UL, 0x75a4f6ebUL, 0x7eaaffe6UL, 0x63b8e4f1UL, 0x68b6edfcUL,
0xb10c0a67UL, 0xba02036aUL, 0xa710187dUL, 0xac1e1170UL, 0x9d342e53UL, 0x963a275eUL, 0x8b283c49UL, 0x80263544UL,
0xe97c420fUL, 0xe2724b02UL, 0xff605015UL, 0xf46e5918UL, 0xc544663bUL, 0xce4a6f36UL, 0xd3587421UL, 0xd8567d2cUL,
0x7a37a10cUL, 0x7139a801UL, 0x6c2bb316UL, 0x6725ba1bUL, 0x560f8538UL, 0x5d018c35UL, 0x40139722UL, 0x4b1d9e2fUL,
0x2247e964UL, 0x2949e069UL, 0x345bfb7eUL, 0x3f55f273UL, 0x0e7fcd50UL, 0x0571c45dUL, 0x1863df4aUL, 0x136dd647UL,
0xcad731dcUL, 0xc1d938d1UL, 0xdccb23c6UL, 0xd7c52acbUL, 0xe6ef15e8UL, 0xede11ce5UL, 0xf0f307f2UL, 0xfbfd0effUL,
0x92a779b4UL, 0x99a970b9UL, 0x84bb6baeUL, 0x8fb562a3UL, 0xbe9f5d80UL, 0xb591548dUL, 0xa8834f9aUL, 0xa38d4697UL
};

static const ulong32 Tks2[] = {
0x00000000UL, 0x0d0b0e09UL, 0x1a161c12UL, 0x171d121bUL, 0x342c3824UL, 0x3927362dUL, 0x2e3a2436UL, 0x23312a3fUL,
0x68587048UL, 0x65537e41UL, 0x724e6c5aUL, 0x7f456253UL, 0x5c74486cUL, 0x517f4665UL, 0x4662547eUL, 0x4b695a77UL,
0xd0b0e090UL, 0xddbbee99UL, 0xcaa6fc82UL, 0xc7adf28bUL, 0xe49cd8b4UL, 0xe997d6bdUL, 0xfe8ac4a6UL, 0xf381caafUL,
0xb8e890d8UL, 0xb5e39ed1UL, 0xa2fe8ccaUL, 0xaff582c3UL, 0x8cc4a8fcUL, 0x81cfa6f5UL, 0x96d2b4eeUL, 0x9bd9bae7UL,
0xbb7bdb3bUL, 0xb670d532UL, 0xa16dc729UL, 0xac66c920UL, 0x8f57e31fUL, 0x825ced16UL, 0x9541ff0dUL, 0x984af104UL,
0xd323ab73UL, 0xde28a57aUL, 0xc935b761UL, 0xc43eb968UL, 0xe70f9357UL, 0xea049d5eUL, 0xfd198f45UL, 0xf012814cUL,
0x6bcb3babUL, 0x66c035a2UL, 0x71dd27b9UL, 0x7cd629b0UL, 0x5fe7038fUL, 0x52ec0d86UL, 0x45f11f9dUL, 0x48fa1194UL,
0x03934be3UL, 0x0e9845eaUL, 0x198557f1UL, 0x148e59f8UL, 0x37bf73c7UL, 0x3ab47dceUL, 0x2da96fd5UL, 0x20a261dcUL,
0x6df6ad76UL, 0x60fda37fUL, 0x77e0b164UL, 0x7aebbf6dUL, 0x59da9552UL, 0x54d19b5bUL, 0x43cc8940UL, 0x4ec78749UL,
0x05aedd3eUL, 0x08a5d337UL, 0x1fb8c12cUL, 0x12b3cf25UL, 0x3182e51aUL, 0x3c89eb13UL, 0x2b94f908UL, 0x269ff701UL,
0xbd464de6UL, 0xb04d43efUL, 0xa75051f4UL, 0xaa5b5ffdUL, 0x896a75c2UL, 0x84617bcbUL, 0x937c69d0UL, 0x9e7767d9UL,
0xd51e3daeUL, 0xd81533a7UL, 0xcf0821bcUL, 0xc2032fb5UL, 0xe132058aUL, 0xec390b83UL, 0xfb241998UL, 0xf62f1791UL,
0xd68d764dUL, 0xdb867844UL, 0xcc9b6a5fUL, 0xc1906456UL, 0xe2a14e69UL, 0xefaa4060UL, 0xf8b7527bUL, 0xf5bc5c72UL,
0xbed50605UL, 0xb3de080cUL, 0xa4c31a17UL, 0xa9c8141eUL, 0x8af93e21UL, 0x87f23028UL, 0x90ef2233UL, 0x9de42c3aUL,
0x063d96ddUL, 0x0b3698d4UL, 0x1c2b8acfUL, 0x112084c6UL, 0x3211aef9UL, 0x3f1aa0f0UL, 0x2807b2ebUL, 0x250cbce2UL,
0x6e65e695UL, 0x636ee89cUL, 0x7473fa87UL, 0x7978f48eUL, 0x5a49deb1UL, 0x5742d0b8UL, 0x405fc2a3UL, 0x4d54ccaaUL,
0xdaf741ecUL, 0xd7fc4fe5UL, 0xc0e15dfeUL, 0xcdea53f7UL, 0xeedb79c8UL, 0xe3d077c1UL, 0xf4cd65daUL, 0xf9c66bd3UL,
0xb2af31a4UL, 0xbfa43fadUL, 0xa8b92db6UL, 0xa5b223bfUL, 0x86830980UL, 0x8b880789UL, 0x9c951592UL, 0x919e1b9bUL,
0x0a47a17cUL, 0x074caf75UL, 0x1051bd6eUL, 0x1d5ab367UL, 0x3e6b9958UL, 0x33609751UL, 0x247d854aUL, 0x29768b43UL,
0x621fd134UL, 0x6f14df3dUL, 0x7809cd26UL, 0x7502c32fUL, 0x5633e910UL, 0x5b38e719UL, 0x4c25f502UL, 0x412efb0bUL,
0x618c9ad7UL, 0x6c8794deUL, 0x7b9a86c5UL, 0x769188ccUL, 0x55a0a2f3UL, 0x58abacfaUL, 0x4fb6bee1UL, 0x42bdb0e8UL,
0x09d4ea9fUL, 0x04dfe496UL, 0x13c2f68dUL, 0x1ec9f884UL, 0x3df8d2bbUL, 0x30f3dcb2UL, 0x27eecea9UL, 0x2ae5c0a0UL,
0xb13c7a47UL, 0xbc37744eUL, 0xab2a6655UL, 0xa621685cUL, 0x85104263UL, 0x881b4c6aUL, 0x9f065e71UL, 0x920d5078UL,
0xd9640a0fUL, 0xd46f0406UL, 0xc372161dUL, 0xce791814UL, 0xed48322bUL, 0xe0433c22UL, 0xf75e2e39UL, 0xfa552030UL,
0xb701ec9aUL, 0xba0ae293UL, 0xad17f088UL, 0xa01cfe81UL, 0x832dd4beUL, 0x8e26dab7UL, 0x993bc8acUL, 0x9430c6a5UL,
0xdf599cd2UL, 0xd25292dbUL, 0xc54f80c0UL, 0xc8448ec9UL, 0xeb75a4f6UL, 0xe67eaaffUL, 0xf163b8e4UL, 0xfc68b6edUL,
0x67b10c0aUL, 0x6aba0203UL, 0x7da71018UL, 0x70ac1e11UL, 0x539d342eUL, 0x5e963a27UL, 0x498b283cUL, 0x44802635UL,
0x0fe97c42UL, 0x02e2724bUL, 0x15ff6050UL, 0x18f46e59UL, 0x3bc54466UL, 0x36ce4a6fUL, 0x21d35874UL, 0x2cd8567dUL,
0x0c7a37a1UL, 0x017139a8UL, 0x166c2bb3UL, 0x1b6725baUL, 0x38560f85UL, 0x355d018cUL, 0x22401397UL, 0x2f4b1d9eUL,
0x642247e9UL, 0x692949e0UL, 0x7e345bfbUL, 0x733f55f2UL, 0x500e7fcdUL, 0x5d0571c4UL, 0x4a1863dfUL, 0x47136dd6UL,
0xdccad731UL, 0xd1c1d938UL, 0xc6dccb23UL, 0xcbd7c52aUL, 0xe8e6ef15UL, 0xe5ede11cUL, 0xf2f0f307UL, 0xfffbfd0eUL,
0xb492a779UL, 0xb999a970UL, 0xae84bb6bUL, 0xa38fb562UL, 0x80be9f5dUL, 0x8db59154UL, 0x9aa8834fUL, 0x97a38d46UL
};

static const ulong32 Tks3[] = {
0x00000000UL, 0x090d0b0eUL, 0x121a161cUL, 0x1b171d12UL, 0x24342c38UL, 0x2d392736UL, 0x362e3a24UL, 0x3f23312aUL,
0x48685870UL, 0x4165537eUL, 0x5a724e6cUL, 0x537f4562UL, 0x6c5c7448UL, 0x65517f46UL, 0x7e466254UL, 0x774b695aUL,
0x90d0b0e0UL, 0x99ddbbeeUL, 0x82caa6fcUL, 0x8bc7adf2UL, 0xb4e49cd8UL, 0xbde997d6UL, 0xa6fe8ac4UL, 0xaff381caUL,
0xd8b8e890UL, 0xd1b5e39eUL, 0xcaa2fe8cUL, 0xc3aff582UL, 0xfc8cc4a8UL, 0xf581cfa6UL, 0xee96d2b4UL, 0xe79bd9baUL,
0x3bbb7bdbUL, 0x32b670d5UL, 0x29a16dc7UL, 0x20ac66c9UL, 0x1f8f57e3UL, 0x16825cedUL, 0x0d9541ffUL, 0x04984af1UL,
0x73d323abUL, 0x7ade28a5UL, 0x61c935b7UL, 0x68c43eb9UL, 0x57e70f93UL, 0x5eea049dUL, 0x45fd198fUL, 0x4cf01281UL,
0xab6bcb3bUL, 0xa266c035UL, 0xb971dd27UL, 0xb07cd629UL, 0x8f5fe703UL, 0x8652ec0dUL, 0x9d45f11fUL, 0x9448fa11UL,
0xe303934bUL, 0xea0e9845UL, 0xf1198557UL, 0xf8148e59UL, 0xc737bf73UL, 0xce3ab47dUL, 0xd52da96fUL, 0xdc20a261UL,
0x766df6adUL, 0x7f60fda3UL, 0x6477e0b1UL, 0x6d7aebbfUL, 0x5259da95UL, 0x5b54d19bUL, 0x4043cc89UL, 0x494ec787UL,
0x3e05aeddUL, 0x3708a5d3UL, 0x2c1fb8c1UL, 0x2512b3cfUL, 0x1a3182e5UL, 0x133c89ebUL, 0x082b94f9UL, 0x01269ff7UL,
0xe6bd464dUL, 0xefb04d43UL, 0xf4a75051UL, 0xfdaa5b5fUL, 0xc2896a75UL, 0xcb84617bUL, 0xd0937c69UL, 0xd99e7767UL,
0xaed51e3dUL, 0xa7d81533UL, 0xbccf0821UL, 0xb5c2032fUL, 0x8ae13205UL, 0x83ec390bUL, 0x98fb2419UL, 0x91f62f17UL,
0x4dd68d76UL, 0x44db8678UL, 0x5fcc9b6aUL, 0x56c19064UL, 0x69e2a14eUL, 0x60efaa40UL, 0x7bf8b752UL, 0x72f5bc5cUL,
0x05bed506UL, 0x0cb3de08UL, 0x17a4c31aUL, 0x1ea9c814UL, 0x218af93eUL, 0x2887f230UL, 0x3390ef22UL, 0x3a9de42cUL,
0xdd063d96UL, 0xd40b3698UL, 0xcf1c2b8aUL, 0xc6112084UL, 0xf93211aeUL, 0xf03f1aa0UL, 0xeb2807b2UL, 0xe2250cbcUL,
0x956e65e6UL, 0x9c636ee8UL, 0x877473faUL, 0x8e7978f4UL, 0xb15a49deUL, 0xb85742d0UL, 0xa3405fc2UL, 0xaa4d54ccUL,
0xecdaf741UL, 0xe5d7fc4fUL, 0xfec0e15dUL, 0xf7cdea53UL, 0xc8eedb79UL, 0xc1e3d077UL, 0xdaf4cd65UL, 0xd3f9c66bUL,
0xa4b2af31UL, 0xadbfa43fUL, 0xb6a8b92dUL, 0xbfa5b223UL, 0x80868309UL, 0x898b8807UL, 0x929c9515UL, 0x9b919e1bUL,
0x7c0a47a1UL, 0x75074cafUL, 0x6e1051bdUL, 0x671d5ab3UL, 0x583e6b99UL, 0x51336097UL, 0x4a247d85UL, 0x4329768bUL,
0x34621fd1UL, 0x3d6f14dfUL, 0x267809cdUL, 0x2f7502c3UL, 0x105633e9UL, 0x195b38e7UL, 0x024c25f5UL, 0x0b412efbUL,
0xd7618c9aUL, 0xde6c8794UL, 0xc57b9a86UL, 0xcc769188UL, 0xf355a0a2UL, 0xfa58abacUL, 0xe14fb6beUL, 0xe842bdb0UL,
0x9f09d4eaUL, 0x9604dfe4UL, 0x8d13c2f6UL, 0x841ec9f8UL, 0xbb3df8d2UL, 0xb230f3dcUL, 0xa927eeceUL, 0xa02ae5c0UL,
0x47b13c7aUL, 0x4ebc3774UL, 0x55ab2a66UL, 0x5ca62168UL, 0x63851042UL, 0x6a881b4cUL, 0x719f065eUL, 0x78920d50UL,
0x0fd9640aUL, 0x06d46f04UL, 0x1dc37216UL, 0x14ce7918UL, 0x2bed4832UL, 0x22e0433cUL, 0x39f75e2eUL, 0x30fa5520UL,
0x9ab701ecUL, 0x93ba0ae2UL, 0x88ad17f0UL, 0x81a01cfeUL, 0xbe832dd4UL, 0xb78e26daUL, 0xac993bc8UL, 0xa59430c6UL,
0xd2df599cUL, 0xdbd25292UL, 0xc0c54f80UL, 0xc9c8448eUL, 0xf6eb75a4UL, 0xffe67eaaUL, 0xe4f163b8UL, 0xedfc68b6UL,
0x0a67b10cUL, 0x036aba02UL, 0x187da710UL, 0x1170ac1eUL, 0x2e539d34UL, 0x275e963aUL, 0x3c498b28UL, 0x35448026UL,
0x420fe97cUL, 0x4b02e272UL, 0x5015ff60UL, 0x5918f46eUL, 0x663bc544UL, 0x6f36ce4aUL, 0x7421d358UL, 0x7d2cd856UL,
0xa10c7a37UL, 0xa8017139UL, 0xb3166c2bUL, 0xba1b6725UL, 0x8538560fUL, 0x8c355d01UL, 0x97224013UL, 0x9e2f4b1dUL,
0xe9642247UL, 0xe0692949UL, 0xfb7e345bUL, 0xf2733f55UL, 0xcd500e7fUL, 0xc45d0571UL, 0xdf4a1863UL, 0xd647136dUL,
0x31dccad7UL, 0x38d1c1d9UL, 0x23c6dccbUL, 0x2acbd7c5UL, 0x15e8e6efUL, 0x1ce5ede1UL, 0x07f2f0f3UL, 0x0efffbfdUL,
0x79b492a7UL, 0x70b999a9UL, 0x6bae84bbUL, 0x62a38fb5UL, 0x5d80be9fUL, 0x548db591UL, 0x4f9aa883UL, 0x4697a38dUL
};

#endif /* ENCRYPT_ONLY */

#endif /* SMALL CODE */

static const ulong32 rcon[] = {
    0x01000000UL, 0x02000000UL, 0x04000000UL, 0x08000000UL,
    0x10000000UL, 0x20000000UL, 0x40000000UL, 0x80000000UL,
    0x1B000000UL, 0x36000000UL, /* for 128-bit blocks, Rijndael never uses more than 10 rcon values */
};

#endif /* __LTC_AES_TAB_C__ */

/* $Source$ */
/* $Revision$ */
/* $Date$ */

/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* AES implementation by Tom St Denis
 *
 * Derived from the Public Domain source code by

---
  * rijndael-alg-fst.c
  *
  * @version 3.0 (December 2000)
  *
  * Optimised ANSI C code for the Rijndael cipher (now AES)
  *
  * @author Vincent Rijmen <vincent.rijmen@esat.kuleuven.ac.be>
  * @author Antoon Bosselaers <antoon.bosselaers@esat.kuleuven.ac.be>
  * @author Paulo Barreto <paulo.barreto@terra.com.br>
---
 */
/**
  @file aes.c
  Implementation of AES
*/



#ifdef LTC_RIJNDAEL

#ifndef ENCRYPT_ONLY

#define SETUP    rijndael_setup
#define ECB_ENC  rijndael_ecb_encrypt
#define ECB_DEC  rijndael_ecb_decrypt
#define ECB_DONE rijndael_done
#define ECB_TEST rijndael_test
#define ECB_KS   rijndael_keysize

const struct ltc_cipher_descriptor rijndael_desc =
{
    "rijndael",
    6,
    16, 32, 16, 10,
    SETUP, ECB_ENC, ECB_DEC, ECB_TEST, ECB_DONE, ECB_KS,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

const struct ltc_cipher_descriptor aes_desc =
{
    "aes",
    6,
    16, 32, 16, 10,
    SETUP, ECB_ENC, ECB_DEC, ECB_TEST, ECB_DONE, ECB_KS,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

#else

#define SETUP    rijndael_enc_setup
#define ECB_ENC  rijndael_enc_ecb_encrypt
#define ECB_KS   rijndael_enc_keysize
#define ECB_DONE rijndael_enc_done

const struct ltc_cipher_descriptor rijndael_enc_desc =
{
    "rijndael",
    6,
    16, 32, 16, 10,
    SETUP, ECB_ENC, NULL, NULL, ECB_DONE, ECB_KS,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

const struct ltc_cipher_descriptor aes_enc_desc =
{
    "aes",
    6,
    16, 32, 16, 10,
    SETUP, ECB_ENC, NULL, NULL, ECB_DONE, ECB_KS,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

#endif

#define __LTC_AES_TAB_C__


static ulong32 setup_mix(ulong32 temp)
{
   return (Te4_3[byte(temp, 2)]) ^
          (Te4_2[byte(temp, 1)]) ^
          (Te4_1[byte(temp, 0)]) ^
          (Te4_0[byte(temp, 3)]);
}

#ifndef ENCRYPT_ONLY
#ifdef LTC_SMALL_CODE
static ulong32 setup_mix2(ulong32 temp)
{
   return Td0(255 & Te4[byte(temp, 3)]) ^
          Td1(255 & Te4[byte(temp, 2)]) ^
          Td2(255 & Te4[byte(temp, 1)]) ^
          Td3(255 & Te4[byte(temp, 0)]);
}
#endif
#endif

 /**
    Initialize the AES (Rijndael) block cipher
    @param key The symmetric key you wish to pass
    @param keylen The key length in bytes
    @param num_rounds The number of rounds desired (0 for default)
    @param skey The key in as scheduled by this function.
    @return CRYPT_OK if successful
 */
int SETUP(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey)
{
    int i;
    ulong32 temp, *rk;
#ifndef ENCRYPT_ONLY
    ulong32 *rrk;
#endif
    LTC_ARGCHK(key  != NULL);
    LTC_ARGCHK(skey != NULL);

    if (keylen != 16 && keylen != 24 && keylen != 32) {
       return CRYPT_INVALID_KEYSIZE;
    }

    if (num_rounds != 0 && num_rounds != (10 + ((keylen/8)-2)*2)) {
       return CRYPT_INVALID_ROUNDS;
    }

    skey->rijndael.Nr = 10 + ((keylen/8)-2)*2;

    /* setup the forward key */
    i                 = 0;
    rk                = skey->rijndael.eK;
    LOAD32H(rk[0], key     );
    LOAD32H(rk[1], key +  4);
    LOAD32H(rk[2], key +  8);
    LOAD32H(rk[3], key + 12);
    if (keylen == 16) {
        for (;;) {
            temp  = rk[3];
            rk[4] = rk[0] ^ setup_mix(temp) ^ rcon[i];
            rk[5] = rk[1] ^ rk[4];
            rk[6] = rk[2] ^ rk[5];
            rk[7] = rk[3] ^ rk[6];
            if (++i == 10) {
               break;
            }
            rk += 4;
        }
    } else if (keylen == 24) {
        LOAD32H(rk[4], key + 16);
        LOAD32H(rk[5], key + 20);
        for (;;) {
        #ifdef _MSC_VER
            temp = skey->rijndael.eK[rk - skey->rijndael.eK + 5];
        #else
            temp = rk[5];
        #endif
            rk[ 6] = rk[ 0] ^ setup_mix(temp) ^ rcon[i];
            rk[ 7] = rk[ 1] ^ rk[ 6];
            rk[ 8] = rk[ 2] ^ rk[ 7];
            rk[ 9] = rk[ 3] ^ rk[ 8];
            if (++i == 8) {
                break;
            }
            rk[10] = rk[ 4] ^ rk[ 9];
            rk[11] = rk[ 5] ^ rk[10];
            rk += 6;
        }
    } else if (keylen == 32) {
        LOAD32H(rk[4], key + 16);
        LOAD32H(rk[5], key + 20);
        LOAD32H(rk[6], key + 24);
        LOAD32H(rk[7], key + 28);
        for (;;) {
        #ifdef _MSC_VER
            temp = skey->rijndael.eK[rk - skey->rijndael.eK + 7];
        #else
            temp = rk[7];
        #endif
            rk[ 8] = rk[ 0] ^ setup_mix(temp) ^ rcon[i];
            rk[ 9] = rk[ 1] ^ rk[ 8];
            rk[10] = rk[ 2] ^ rk[ 9];
            rk[11] = rk[ 3] ^ rk[10];
            if (++i == 7) {
                break;
            }
            temp = rk[11];
            rk[12] = rk[ 4] ^ setup_mix(RORc(temp, 8));
            rk[13] = rk[ 5] ^ rk[12];
            rk[14] = rk[ 6] ^ rk[13];
            rk[15] = rk[ 7] ^ rk[14];
            rk += 8;
        }
    } else {
       /* this can't happen */
       /* coverity[dead_error_line] */
       return CRYPT_ERROR;
    }

#ifndef ENCRYPT_ONLY
    /* setup the inverse key now */
    rk   = skey->rijndael.dK;
    rrk  = skey->rijndael.eK + (28 + keylen) - 4;

    /* apply the inverse MixColumn transform to all round keys but the first and the last: */
    /* copy first */
    *rk++ = *rrk++;
    *rk++ = *rrk++;
    *rk++ = *rrk++;
    *rk   = *rrk;
    rk -= 3; rrk -= 3;

    for (i = 1; i < skey->rijndael.Nr; i++) {
        rrk -= 4;
        rk  += 4;
    #ifdef LTC_SMALL_CODE
        temp = rrk[0];
        rk[0] = setup_mix2(temp);
        temp = rrk[1];
        rk[1] = setup_mix2(temp);
        temp = rrk[2];
        rk[2] = setup_mix2(temp);
        temp = rrk[3];
        rk[3] = setup_mix2(temp);
     #else
        temp = rrk[0];
        rk[0] =
            Tks0[byte(temp, 3)] ^
            Tks1[byte(temp, 2)] ^
            Tks2[byte(temp, 1)] ^
            Tks3[byte(temp, 0)];
        temp = rrk[1];
        rk[1] =
            Tks0[byte(temp, 3)] ^
            Tks1[byte(temp, 2)] ^
            Tks2[byte(temp, 1)] ^
            Tks3[byte(temp, 0)];
        temp = rrk[2];
        rk[2] =
            Tks0[byte(temp, 3)] ^
            Tks1[byte(temp, 2)] ^
            Tks2[byte(temp, 1)] ^
            Tks3[byte(temp, 0)];
        temp = rrk[3];
        rk[3] =
            Tks0[byte(temp, 3)] ^
            Tks1[byte(temp, 2)] ^
            Tks2[byte(temp, 1)] ^
            Tks3[byte(temp, 0)];
      #endif

    }

    /* copy last */
    rrk -= 4;
    rk  += 4;
    *rk++ = *rrk++;
    *rk++ = *rrk++;
    *rk++ = *rrk++;
    *rk   = *rrk;
#endif /* ENCRYPT_ONLY */

    return CRYPT_OK;
}

/**
  Encrypts a block of text with AES
  @param pt The input plaintext (16 bytes)
  @param ct The output ciphertext (16 bytes)
  @param skey The key as scheduled
  @return CRYPT_OK if successful
*/
#ifdef LTC_CLEAN_STACK
static int _rijndael_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
#else
int ECB_ENC(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
#endif
{
    ulong32 s0, s1, s2, s3, t0, t1, t2, t3, *rk;
    int Nr, r;

    LTC_ARGCHK(pt != NULL);
    LTC_ARGCHK(ct != NULL);
    LTC_ARGCHK(skey != NULL);

    Nr = skey->rijndael.Nr;
    rk = skey->rijndael.eK;

    /*
     * map byte array block to cipher state
     * and add initial round key:
     */
    LOAD32H(s0, pt      ); s0 ^= rk[0];
    LOAD32H(s1, pt  +  4); s1 ^= rk[1];
    LOAD32H(s2, pt  +  8); s2 ^= rk[2];
    LOAD32H(s3, pt  + 12); s3 ^= rk[3];

#ifdef LTC_SMALL_CODE

    for (r = 0; ; r++) {
        rk += 4;
        t0 =
            Te0(byte(s0, 3)) ^
            Te1(byte(s1, 2)) ^
            Te2(byte(s2, 1)) ^
            Te3(byte(s3, 0)) ^
            rk[0];
        t1 =
            Te0(byte(s1, 3)) ^
            Te1(byte(s2, 2)) ^
            Te2(byte(s3, 1)) ^
            Te3(byte(s0, 0)) ^
            rk[1];
        t2 =
            Te0(byte(s2, 3)) ^
            Te1(byte(s3, 2)) ^
            Te2(byte(s0, 1)) ^
            Te3(byte(s1, 0)) ^
            rk[2];
        t3 =
            Te0(byte(s3, 3)) ^
            Te1(byte(s0, 2)) ^
            Te2(byte(s1, 1)) ^
            Te3(byte(s2, 0)) ^
            rk[3];
        if (r == Nr-2) {
           break;
        }
        s0 = t0; s1 = t1; s2 = t2; s3 = t3;
    }
    rk += 4;

#else

    /*
     * Nr - 1 full rounds:
     */
    r = Nr >> 1;
    for (;;) {
        t0 =
            Te0(byte(s0, 3)) ^
            Te1(byte(s1, 2)) ^
            Te2(byte(s2, 1)) ^
            Te3(byte(s3, 0)) ^
            rk[4];
        t1 =
            Te0(byte(s1, 3)) ^
            Te1(byte(s2, 2)) ^
            Te2(byte(s3, 1)) ^
            Te3(byte(s0, 0)) ^
            rk[5];
        t2 =
            Te0(byte(s2, 3)) ^
            Te1(byte(s3, 2)) ^
            Te2(byte(s0, 1)) ^
            Te3(byte(s1, 0)) ^
            rk[6];
        t3 =
            Te0(byte(s3, 3)) ^
            Te1(byte(s0, 2)) ^
            Te2(byte(s1, 1)) ^
            Te3(byte(s2, 0)) ^
            rk[7];

        rk += 8;
        if (--r == 0) {
            break;
        }

        s0 =
            Te0(byte(t0, 3)) ^
            Te1(byte(t1, 2)) ^
            Te2(byte(t2, 1)) ^
            Te3(byte(t3, 0)) ^
            rk[0];
        s1 =
            Te0(byte(t1, 3)) ^
            Te1(byte(t2, 2)) ^
            Te2(byte(t3, 1)) ^
            Te3(byte(t0, 0)) ^
            rk[1];
        s2 =
            Te0(byte(t2, 3)) ^
            Te1(byte(t3, 2)) ^
            Te2(byte(t0, 1)) ^
            Te3(byte(t1, 0)) ^
            rk[2];
        s3 =
            Te0(byte(t3, 3)) ^
            Te1(byte(t0, 2)) ^
            Te2(byte(t1, 1)) ^
            Te3(byte(t2, 0)) ^
            rk[3];
    }

#endif

    /*
     * apply last round and
     * map cipher state to byte array block:
     */
    s0 =
        (Te4_3[byte(t0, 3)]) ^
        (Te4_2[byte(t1, 2)]) ^
        (Te4_1[byte(t2, 1)]) ^
        (Te4_0[byte(t3, 0)]) ^
        rk[0];
    STORE32H(s0, ct);
    s1 =
        (Te4_3[byte(t1, 3)]) ^
        (Te4_2[byte(t2, 2)]) ^
        (Te4_1[byte(t3, 1)]) ^
        (Te4_0[byte(t0, 0)]) ^
        rk[1];
    STORE32H(s1, ct+4);
    s2 =
        (Te4_3[byte(t2, 3)]) ^
        (Te4_2[byte(t3, 2)]) ^
        (Te4_1[byte(t0, 1)]) ^
        (Te4_0[byte(t1, 0)]) ^
        rk[2];
    STORE32H(s2, ct+8);
    s3 =
        (Te4_3[byte(t3, 3)]) ^
        (Te4_2[byte(t0, 2)]) ^
        (Te4_1[byte(t1, 1)]) ^
        (Te4_0[byte(t2, 0)]) ^
        rk[3];
    STORE32H(s3, ct+12);

    return CRYPT_OK;
}

#ifdef LTC_CLEAN_STACK
int ECB_ENC(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
{
   int err = _rijndael_ecb_encrypt(pt, ct, skey);
   burn_stack(sizeof(unsigned long)*8 + sizeof(unsigned long*) + sizeof(int)*2);
   return err;
}
#endif

#ifndef ENCRYPT_ONLY

/**
  Decrypts a block of text with AES
  @param ct The input ciphertext (16 bytes)
  @param pt The output plaintext (16 bytes)
  @param skey The key as scheduled
  @return CRYPT_OK if successful
*/
#ifdef LTC_CLEAN_STACK
static int _rijndael_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
#else
int ECB_DEC(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
#endif
{
    ulong32 s0, s1, s2, s3, t0, t1, t2, t3, *rk;
    int Nr, r;

    LTC_ARGCHK(pt != NULL);
    LTC_ARGCHK(ct != NULL);
    LTC_ARGCHK(skey != NULL);

    Nr = skey->rijndael.Nr;
    rk = skey->rijndael.dK;

    /*
     * map byte array block to cipher state
     * and add initial round key:
     */
    LOAD32H(s0, ct      ); s0 ^= rk[0];
    LOAD32H(s1, ct  +  4); s1 ^= rk[1];
    LOAD32H(s2, ct  +  8); s2 ^= rk[2];
    LOAD32H(s3, ct  + 12); s3 ^= rk[3];

#ifdef LTC_SMALL_CODE
    for (r = 0; ; r++) {
        rk += 4;
        t0 =
            Td0(byte(s0, 3)) ^
            Td1(byte(s3, 2)) ^
            Td2(byte(s2, 1)) ^
            Td3(byte(s1, 0)) ^
            rk[0];
        t1 =
            Td0(byte(s1, 3)) ^
            Td1(byte(s0, 2)) ^
            Td2(byte(s3, 1)) ^
            Td3(byte(s2, 0)) ^
            rk[1];
        t2 =
            Td0(byte(s2, 3)) ^
            Td1(byte(s1, 2)) ^
            Td2(byte(s0, 1)) ^
            Td3(byte(s3, 0)) ^
            rk[2];
        t3 =
            Td0(byte(s3, 3)) ^
            Td1(byte(s2, 2)) ^
            Td2(byte(s1, 1)) ^
            Td3(byte(s0, 0)) ^
            rk[3];
        if (r == Nr-2) {
           break;
        }
        s0 = t0; s1 = t1; s2 = t2; s3 = t3;
    }
    rk += 4;

#else

    /*
     * Nr - 1 full rounds:
     */
    r = Nr >> 1;
    for (;;) {

        t0 =
            Td0(byte(s0, 3)) ^
            Td1(byte(s3, 2)) ^
            Td2(byte(s2, 1)) ^
            Td3(byte(s1, 0)) ^
            rk[4];
        t1 =
            Td0(byte(s1, 3)) ^
            Td1(byte(s0, 2)) ^
            Td2(byte(s3, 1)) ^
            Td3(byte(s2, 0)) ^
            rk[5];
        t2 =
            Td0(byte(s2, 3)) ^
            Td1(byte(s1, 2)) ^
            Td2(byte(s0, 1)) ^
            Td3(byte(s3, 0)) ^
            rk[6];
        t3 =
            Td0(byte(s3, 3)) ^
            Td1(byte(s2, 2)) ^
            Td2(byte(s1, 1)) ^
            Td3(byte(s0, 0)) ^
            rk[7];

        rk += 8;
        if (--r == 0) {
            break;
        }


        s0 =
            Td0(byte(t0, 3)) ^
            Td1(byte(t3, 2)) ^
            Td2(byte(t2, 1)) ^
            Td3(byte(t1, 0)) ^
            rk[0];
        s1 =
            Td0(byte(t1, 3)) ^
            Td1(byte(t0, 2)) ^
            Td2(byte(t3, 1)) ^
            Td3(byte(t2, 0)) ^
            rk[1];
        s2 =
            Td0(byte(t2, 3)) ^
            Td1(byte(t1, 2)) ^
            Td2(byte(t0, 1)) ^
            Td3(byte(t3, 0)) ^
            rk[2];
        s3 =
            Td0(byte(t3, 3)) ^
            Td1(byte(t2, 2)) ^
            Td2(byte(t1, 1)) ^
            Td3(byte(t0, 0)) ^
            rk[3];
    }
#endif

    /*
     * apply last round and
     * map cipher state to byte array block:
     */
    s0 =
        (Td4[byte(t0, 3)] & 0xff000000) ^
        (Td4[byte(t3, 2)] & 0x00ff0000) ^
        (Td4[byte(t2, 1)] & 0x0000ff00) ^
        (Td4[byte(t1, 0)] & 0x000000ff) ^
        rk[0];
    STORE32H(s0, pt);
    s1 =
        (Td4[byte(t1, 3)] & 0xff000000) ^
        (Td4[byte(t0, 2)] & 0x00ff0000) ^
        (Td4[byte(t3, 1)] & 0x0000ff00) ^
        (Td4[byte(t2, 0)] & 0x000000ff) ^
        rk[1];
    STORE32H(s1, pt+4);
    s2 =
        (Td4[byte(t2, 3)] & 0xff000000) ^
        (Td4[byte(t1, 2)] & 0x00ff0000) ^
        (Td4[byte(t0, 1)] & 0x0000ff00) ^
        (Td4[byte(t3, 0)] & 0x000000ff) ^
        rk[2];
    STORE32H(s2, pt+8);
    s3 =
        (Td4[byte(t3, 3)] & 0xff000000) ^
        (Td4[byte(t2, 2)] & 0x00ff0000) ^
        (Td4[byte(t1, 1)] & 0x0000ff00) ^
        (Td4[byte(t0, 0)] & 0x000000ff) ^
        rk[3];
    STORE32H(s3, pt+12);

    return CRYPT_OK;
}


#ifdef LTC_CLEAN_STACK
int ECB_DEC(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
{
   int err = _rijndael_ecb_decrypt(ct, pt, skey);
   burn_stack(sizeof(unsigned long)*8 + sizeof(unsigned long*) + sizeof(int)*2);
   return err;
}
#endif

/**
  Performs a self-test of the AES block cipher
  @return CRYPT_OK if functional, CRYPT_NOP if self-test has been disabled
*/
int ECB_TEST(void)
{
 #ifndef LTC_TEST
    return CRYPT_NOP;
 #else
 int err;
 static const struct {
     int keylen;
     unsigned char key[32], pt[16], ct[16];
 } tests[] = {
    { 16,
      { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f },
      { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff },
      { 0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
        0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a }
    }, {
      24,
      { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17 },
      { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff },
      { 0xdd, 0xa9, 0x7c, 0xa4, 0x86, 0x4c, 0xdf, 0xe0,
        0x6e, 0xaf, 0x70, 0xa0, 0xec, 0x0d, 0x71, 0x91 }
    }, {
      32,
      { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f },
      { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff },
      { 0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67, 0x45, 0xbf,
        0xea, 0xfc, 0x49, 0x90, 0x4b, 0x49, 0x60, 0x89 }
    }
 };

 symmetric_key key;
 unsigned char tmp[2][16];
 int i, y;

 for (i = 0; i < (int)(sizeof(tests)/sizeof(tests[0])); i++) {
    zeromem(&key, sizeof(key));
    if ((err = rijndael_setup(tests[i].key, tests[i].keylen, 0, &key)) != CRYPT_OK) {
       return err;
    }

    rijndael_ecb_encrypt(tests[i].pt, tmp[0], &key);
    rijndael_ecb_decrypt(tmp[0], tmp[1], &key);
    if (XMEMCMP(tmp[0], tests[i].ct, 16) || XMEMCMP(tmp[1], tests[i].pt, 16)) {
#if 0
       printf("\n\nTest %d failed\n", i);
       if (XMEMCMP(tmp[0], tests[i].ct, 16)) {
          printf("CT: ");
          for (i = 0; i < 16; i++) {
             printf("%02x ", tmp[0][i]);
          }
          printf("\n");
       } else {
          printf("PT: ");
          for (i = 0; i < 16; i++) {
             printf("%02x ", tmp[1][i]);
          }
          printf("\n");
       }
#endif
        return CRYPT_FAIL_TESTVECTOR;
    }

      /* now see if we can encrypt all zero bytes 1000 times, decrypt and come back where we started */
      for (y = 0; y < 16; y++) tmp[0][y] = 0;
      for (y = 0; y < 1000; y++) rijndael_ecb_encrypt(tmp[0], tmp[0], &key);
      for (y = 0; y < 1000; y++) rijndael_ecb_decrypt(tmp[0], tmp[0], &key);
      for (y = 0; y < 16; y++) if (tmp[0][y] != 0) return CRYPT_FAIL_TESTVECTOR;
 }
 return CRYPT_OK;
 #endif
}

#endif /* ENCRYPT_ONLY */


/** Terminate the context
   @param skey    The scheduled key
*/
void ECB_DONE(symmetric_key *skey)
{
  //LTC_UNUSED_PARAM(skey);
}


/**
  Gets suitable key size
  @param keysize [in/out] The length of the recommended key (in bytes).  This function will store the suitable size back in this variable.
  @return CRYPT_OK if the input key size is acceptable.
*/
int ECB_KS(int *keysize)
{
   LTC_ARGCHK(keysize != NULL);

   if (*keysize < 16)
      return CRYPT_INVALID_KEYSIZE;
   if (*keysize < 24) {
      *keysize = 16;
      return CRYPT_OK;
   } else if (*keysize < 32) {
      *keysize = 24;
      return CRYPT_OK;
   } else {
      *keysize = 32;
      return CRYPT_OK;
   }
}

#endif


/* $Source$ */
/* $Revision$ */
/* $Date$ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file cbc_decrypt.c
   CBC implementation, encrypt block, Tom St Denis
*/


#ifdef LTC_CBC_MODE

/**
  CBC decrypt
  @param ct     Ciphertext
  @param pt     [out] Plaintext
  @param len    The number of bytes to process (must be multiple of block length)
  @param cbc    CBC state
  @return CRYPT_OK if successful
*/
int cbc_decrypt(const unsigned char *ct, unsigned char *pt, unsigned long len, symmetric_CBC *cbc)
{
   int x, err;
   unsigned char tmp[16];
#ifdef LTC_FAST
   LTC_FAST_TYPE tmpy;
#else
   unsigned char tmpy;
#endif

   LTC_ARGCHK(pt  != NULL);
   LTC_ARGCHK(ct  != NULL);
   LTC_ARGCHK(cbc != NULL);

   if ((err = cipher_is_valid(cbc->cipher)) != CRYPT_OK) {
       return err;
   }

   /* is blocklen valid? */
   if (cbc->blocklen < 1 || cbc->blocklen > (int)sizeof(cbc->IV)) {
      return CRYPT_INVALID_ARG;
   }

   if (len % cbc->blocklen) {
      return CRYPT_INVALID_ARG;
   }
#ifdef LTC_FAST
   if (cbc->blocklen % sizeof(LTC_FAST_TYPE)) {
      return CRYPT_INVALID_ARG;
   }
#endif

   if (cipher_descriptor[cbc->cipher].accel_cbc_decrypt != NULL) {
      return cipher_descriptor[cbc->cipher].accel_cbc_decrypt(ct, pt, len / cbc->blocklen, cbc->IV, &cbc->key);
   } else {
      while (len) {
         /* decrypt */
         if ((err = cipher_descriptor[cbc->cipher].ecb_decrypt(ct, tmp, &cbc->key)) != CRYPT_OK) {
            return err;
         }

         /* xor IV against plaintext */
         #if defined(LTC_FAST)
        for (x = 0; x < cbc->blocklen; x += sizeof(LTC_FAST_TYPE)) {
            tmpy = *((LTC_FAST_TYPE*)((unsigned char *)cbc->IV + x)) ^ *((LTC_FAST_TYPE*)((unsigned char *)tmp + x));
       *((LTC_FAST_TYPE*)((unsigned char *)cbc->IV + x)) = *((LTC_FAST_TYPE*)((unsigned char *)ct + x));
       *((LTC_FAST_TYPE*)((unsigned char *)pt + x)) = tmpy;
        }
    #else
            for (x = 0; x < cbc->blocklen; x++) {
               tmpy       = tmp[x] ^ cbc->IV[x];
               cbc->IV[x] = ct[x];
               pt[x]      = tmpy;
            }
    #endif

         ct  += cbc->blocklen;
         pt  += cbc->blocklen;
         len -= cbc->blocklen;
      }
   }
   return CRYPT_OK;
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file cbc_done.c
   CBC implementation, finish chain, Tom St Denis
*/

#ifdef LTC_CBC_MODE

/** Terminate the chain
  @param cbc    The CBC chain to terminate
  @return CRYPT_OK on success
*/
int cbc_done(symmetric_CBC *cbc)
{
   int err;
   LTC_ARGCHK(cbc != NULL);

   if ((err = cipher_is_valid(cbc->cipher)) != CRYPT_OK) {
      return err;
   }
   cipher_descriptor[cbc->cipher].done(&cbc->key);
   return CRYPT_OK;
}



#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file cbc_encrypt.c
   CBC implementation, encrypt block, Tom St Denis
*/


#ifdef LTC_CBC_MODE

/**
  CBC encrypt
  @param pt     Plaintext
  @param ct     [out] Ciphertext
  @param len    The number of bytes to process (must be multiple of block length)
  @param cbc    CBC state
  @return CRYPT_OK if successful
*/
int cbc_encrypt(const unsigned char *pt, unsigned char *ct, unsigned long len, symmetric_CBC *cbc)
{
   int x, err;

   LTC_ARGCHK(pt != NULL);
   LTC_ARGCHK(ct != NULL);
   LTC_ARGCHK(cbc != NULL);

   if ((err = cipher_is_valid(cbc->cipher)) != CRYPT_OK) {
       return err;
   }

   /* is blocklen valid? */
   if (cbc->blocklen < 1 || cbc->blocklen > (int)sizeof(cbc->IV)) {
      return CRYPT_INVALID_ARG;
   }

   if (len % cbc->blocklen) {
      return CRYPT_INVALID_ARG;
   }
#ifdef LTC_FAST
   if (cbc->blocklen % sizeof(LTC_FAST_TYPE)) {
      return CRYPT_INVALID_ARG;
   }
#endif

   if (cipher_descriptor[cbc->cipher].accel_cbc_encrypt != NULL) {
      return cipher_descriptor[cbc->cipher].accel_cbc_encrypt(pt, ct, len / cbc->blocklen, cbc->IV, &cbc->key);
   } else {
      while (len) {
         /* xor IV against plaintext */
         #if defined(LTC_FAST)
        for (x = 0; x < cbc->blocklen; x += sizeof(LTC_FAST_TYPE)) {
            *((LTC_FAST_TYPE*)((unsigned char *)cbc->IV + x)) ^= *((LTC_FAST_TYPE*)((unsigned char *)pt + x));
        }
    #else
            for (x = 0; x < cbc->blocklen; x++) {
               cbc->IV[x] ^= pt[x];
            }
    #endif

         /* encrypt */
         if ((err = cipher_descriptor[cbc->cipher].ecb_encrypt(cbc->IV, ct, &cbc->key)) != CRYPT_OK) {
            return err;
         }

        /* store IV [ciphertext] for a future block */
         #if defined(LTC_FAST)
        for (x = 0; x < cbc->blocklen; x += sizeof(LTC_FAST_TYPE)) {
            *((LTC_FAST_TYPE*)((unsigned char *)cbc->IV + x)) = *((LTC_FAST_TYPE*)((unsigned char *)ct + x));
        }
    #else
             for (x = 0; x < cbc->blocklen; x++) {
                cbc->IV[x] = ct[x];
             }
    #endif

        ct  += cbc->blocklen;
        pt  += cbc->blocklen;
        len -= cbc->blocklen;
     }
   }
   return CRYPT_OK;
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file cbc_getiv.c
   CBC implementation, get IV, Tom St Denis
*/

#ifdef LTC_CBC_MODE

/**
   Get the current initial vector
   @param IV   [out] The destination of the initial vector
   @param len  [in/out]  The max size and resulting size of the initial vector
   @param cbc  The CBC state
   @return CRYPT_OK if successful
*/
int cbc_getiv(unsigned char *IV, unsigned long *len, symmetric_CBC *cbc)
{
   LTC_ARGCHK(IV  != NULL);
   LTC_ARGCHK(len != NULL);
   LTC_ARGCHK(cbc != NULL);
   if ((unsigned long)cbc->blocklen > *len) {
      *len = cbc->blocklen;
      return CRYPT_BUFFER_OVERFLOW;
   }
   XMEMCPY(IV, cbc->IV, cbc->blocklen);
   *len = cbc->blocklen;

   return CRYPT_OK;
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file cbc_setiv.c
   CBC implementation, set IV, Tom St Denis
*/


#ifdef LTC_CBC_MODE

/**
   Set an initial vector
   @param IV   The initial vector
   @param len  The length of the vector (in octets)
   @param cbc  The CBC state
   @return CRYPT_OK if successful
*/
int cbc_setiv(const unsigned char *IV, unsigned long len, symmetric_CBC *cbc)
{
   LTC_ARGCHK(IV  != NULL);
   LTC_ARGCHK(cbc != NULL);
   if (len != (unsigned long)cbc->blocklen) {
      return CRYPT_INVALID_ARG;
   }
   XMEMCPY(cbc->IV, IV, len);
   return CRYPT_OK;
}

#endif


/* $Source$ */
/* $Revision$ */
/* $Date$ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */


/**
   @file cbc_start.c
   CBC implementation, start chain, Tom St Denis
*/

#ifdef LTC_CBC_MODE

/**
   Initialize a CBC context
   @param cipher      The index of the cipher desired
   @param IV          The initial vector
   @param key         The secret key
   @param keylen      The length of the secret key (octets)
   @param num_rounds  Number of rounds in the cipher desired (0 for default)
   @param cbc         The CBC state to initialize
   @return CRYPT_OK if successful
*/
int cbc_start(int cipher, const unsigned char *IV, const unsigned char *key,
              int keylen, int num_rounds, symmetric_CBC *cbc)
{
   int x, err;

   LTC_ARGCHK(IV != NULL);
   LTC_ARGCHK(key != NULL);
   LTC_ARGCHK(cbc != NULL);

   /* bad param? */
   if ((err = cipher_is_valid(cipher)) != CRYPT_OK) {
      return err;
   }

   /* setup cipher */
   if ((err = cipher_descriptor[cipher].setup(key, keylen, num_rounds, &cbc->key)) != CRYPT_OK) {
      return err;
   }

   /* copy IV */
   cbc->blocklen = cipher_descriptor[cipher].block_length;
   cbc->cipher   = cipher;
   for (x = 0; x < cbc->blocklen; x++) {
       cbc->IV[x] = IV[x];
   }
   return CRYPT_OK;
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/**
   @file gcm_add_iv.c
   GCM implementation, add IV data to the state, by Tom St Denis
*/


#ifdef LTC_GCM_MODE

/**
  Add IV data to the GCM state
  @param gcm    The GCM state
  @param IV     The initial value data to add
  @param IVlen  The length of the IV
  @return CRYPT_OK on success
 */
int gcm_add_iv(gcm_state *gcm,
               const unsigned char *IV,     unsigned long IVlen)
{
   unsigned long x, y;
   int           err;

   LTC_ARGCHK(gcm != NULL);
   if (IVlen > 0) {
      LTC_ARGCHK(IV  != NULL);
   }

   /* must be in IV mode */
   if (gcm->mode != LTC_GCM_MODE_IV) {
      return CRYPT_INVALID_ARG;
   }

   if (gcm->buflen >= 16 || gcm->buflen < 0) {
      return CRYPT_INVALID_ARG;
   }

   if ((err = cipher_is_valid(gcm->cipher)) != CRYPT_OK) {
      return err;
   }


   /* trip the ivmode flag */
   if (IVlen + gcm->buflen > 12) {
      gcm->ivmode |= 1;
   }

   x = 0;
#ifdef LTC_FAST
   if (gcm->buflen == 0) {
      for (x = 0; x < (IVlen & ~15); x += 16) {
          for (y = 0; y < 16; y += sizeof(LTC_FAST_TYPE)) {
              *((LTC_FAST_TYPE*)(&gcm->X[y])) ^= *((LTC_FAST_TYPE*)(&IV[x + y]));
          }
          gcm_mult_h(gcm, gcm->X);
          gcm->totlen += 128;
      }
      IV += x;
   }
#endif

   /* start adding IV data to the state */
   for (; x < IVlen; x++) {
       gcm->buf[gcm->buflen++] = *IV++;

       if (gcm->buflen == 16) {
         /* GF mult it */
         for (y = 0; y < 16; y++) {
             gcm->X[y] ^= gcm->buf[y];
         }
         gcm_mult_h(gcm, gcm->X);
         gcm->buflen = 0;
         gcm->totlen += 128;
      }
   }

   return CRYPT_OK;
}

#endif


/* $Source: /cvs/libtom/libtomcrypt/src/encauth/gcm/gcm_add_iv.c,v $ */
/* $Revision: 1.9 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/**
   @file gcm_done.c
   GCM implementation, Terminate the stream, by Tom St Denis
*/


#ifdef LTC_GCM_MODE

/**
  Terminate a GCM stream
  @param gcm     The GCM state
  @param tag     [out] The destination for the MAC tag
  @param taglen  [in/out]  The length of the MAC tag
  @return CRYPT_OK on success
 */
int gcm_done(gcm_state *gcm,
                     unsigned char *tag,    unsigned long *taglen)
{
   unsigned long x;
   int err;

   LTC_ARGCHK(gcm     != NULL);
   LTC_ARGCHK(tag     != NULL);
   LTC_ARGCHK(taglen  != NULL);

   if (gcm->buflen > 16 || gcm->buflen < 0) {
      return CRYPT_INVALID_ARG;
   }

   if ((err = cipher_is_valid(gcm->cipher)) != CRYPT_OK) {
      return err;
   }


   if (gcm->mode != LTC_GCM_MODE_TEXT) {
      return CRYPT_INVALID_ARG;
   }

   /* handle remaining ciphertext */
   if (gcm->buflen) {
      gcm->pttotlen += gcm->buflen * CONST64(8);
      gcm_mult_h(gcm, gcm->X);
   }

   /* length */
   STORE64H(gcm->totlen, gcm->buf);
   STORE64H(gcm->pttotlen, gcm->buf+8);
   for (x = 0; x < 16; x++) {
       gcm->X[x] ^= gcm->buf[x];
   }
   gcm_mult_h(gcm, gcm->X);

   /* encrypt original counter */
   if ((err = cipher_descriptor[gcm->cipher].ecb_encrypt(gcm->Y_0, gcm->buf, &gcm->K)) != CRYPT_OK) {
      return err;
   }
   for (x = 0; x < 16 && x < *taglen; x++) {
       tag[x] = gcm->buf[x] ^ gcm->X[x];
   }
   *taglen = x;

   cipher_descriptor[gcm->cipher].done(&gcm->K);

   return CRYPT_OK;
}

#endif


/* $Source: /cvs/libtom/libtomcrypt/src/encauth/gcm/gcm_done.c,v $ */
/* $Revision: 1.11 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/**
   @file gcm_init.c
   GCM implementation, initialize state, by Tom St Denis
*/


#ifdef LTC_GCM_MODE

/**
  Initialize a GCM state
  @param gcm     The GCM state to initialize
  @param cipher  The index of the cipher to use
  @param key     The secret key
  @param keylen  The length of the secret key
  @return CRYPT_OK on success
 */
int gcm_init(gcm_state *gcm, int cipher,
             const unsigned char *key,  int keylen)
{
   int           err;
   unsigned char B[16];
#ifdef LTC_GCM_TABLES
   int           x, y, z, t;
#endif

   LTC_ARGCHK(gcm != NULL);
   LTC_ARGCHK(key != NULL);

#ifdef LTC_FAST
   if (16 % sizeof(LTC_FAST_TYPE)) {
      return CRYPT_INVALID_ARG;
   }
#endif

   /* is cipher valid? */
   if ((err = cipher_is_valid(cipher)) != CRYPT_OK) {
      return err;
   }
   if (cipher_descriptor[cipher].block_length != 16) {
      return CRYPT_INVALID_CIPHER;
   }

   /* schedule key */
   if ((err = cipher_descriptor[cipher].setup(key, keylen, 0, &gcm->K)) != CRYPT_OK) {
      return err;
   }

   /* H = E(0) */
   zeromem(B, 16);
   if ((err = cipher_descriptor[cipher].ecb_encrypt(B, gcm->H, &gcm->K)) != CRYPT_OK) {
      return err;
   }

   /* setup state */
   zeromem(gcm->buf, sizeof(gcm->buf));
   zeromem(gcm->X,   sizeof(gcm->X));
   gcm->cipher   = cipher;
   gcm->mode     = LTC_GCM_MODE_IV;
   gcm->ivmode   = 0;
   gcm->buflen   = 0;
   gcm->totlen   = 0;
   gcm->pttotlen = 0;

#ifdef LTC_GCM_TABLES
   /* setup tables */

   /* generate the first table as it has no shifting (from which we make the other tables) */
   zeromem(B, 16);
   for (y = 0; y < 256; y++) {
        B[0] = y;
        gcm_gf_mult(gcm->H, B, &gcm->PC[0][y][0]);
   }

   /* now generate the rest of the tables based the previous table */
   for (x = 1; x < 16; x++) {
      for (y = 0; y < 256; y++) {
         /* now shift it right by 8 bits */
         t = gcm->PC[x-1][y][15];
         for (z = 15; z > 0; z--) {
             gcm->PC[x][y][z] = gcm->PC[x-1][y][z-1];
         }
         gcm->PC[x][y][0] = gcm_shift_table[t<<1];
         gcm->PC[x][y][1] ^= gcm_shift_table[(t<<1)+1];
     }
  }

#endif

   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/encauth/gcm/gcm_init.c,v $ */
/* $Revision: 1.20 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/**
   @file gcm_process.c
   GCM implementation, process message data, by Tom St Denis
*/


#ifdef LTC_GCM_MODE

/**
  Process plaintext/ciphertext through GCM
  @param gcm       The GCM state
  @param pt        The plaintext
  @param ptlen     The plaintext length (ciphertext length is the same)
  @param ct        The ciphertext
  @param direction Encrypt or Decrypt mode (GCM_ENCRYPT or GCM_DECRYPT)
  @return CRYPT_OK on success
 */
int gcm_process(gcm_state *gcm,
                     unsigned char *pt,     unsigned long ptlen,
                     unsigned char *ct,
                     int direction)
{
   unsigned long x;
   int           y, err;
   unsigned char b;

   LTC_ARGCHK(gcm != NULL);
   if (ptlen > 0) {
      LTC_ARGCHK(pt  != NULL);
      LTC_ARGCHK(ct  != NULL);
   }

   if (gcm->buflen > 16 || gcm->buflen < 0) {
      return CRYPT_INVALID_ARG;
   }

   if ((err = cipher_is_valid(gcm->cipher)) != CRYPT_OK) {
      return err;
   }

   /* in AAD mode? */
   if (gcm->mode == LTC_GCM_MODE_AAD) {
      /* let's process the AAD */
      if (gcm->buflen) {
         gcm->totlen += gcm->buflen * CONST64(8);
         gcm_mult_h(gcm, gcm->X);
      }

      /* increment counter */
      for (y = 15; y >= 12; y--) {
          if (++gcm->Y[y] & 255) { break; }
      }
      /* encrypt the counter */
      if ((err = cipher_descriptor[gcm->cipher].ecb_encrypt(gcm->Y, gcm->buf, &gcm->K)) != CRYPT_OK) {
         return err;
      }

      gcm->buflen = 0;
      gcm->mode   = LTC_GCM_MODE_TEXT;
   }

   if (gcm->mode != LTC_GCM_MODE_TEXT) {
      return CRYPT_INVALID_ARG;
   }

   x = 0;
#ifdef LTC_FAST
   if (gcm->buflen == 0) {
      if (direction == GCM_ENCRYPT) {
         for (x = 0; x < (ptlen & ~15); x += 16) {
             /* ctr encrypt */
             for (y = 0; y < 16; y += sizeof(LTC_FAST_TYPE)) {
                 *((LTC_FAST_TYPE*)(&ct[x + y])) = *((LTC_FAST_TYPE*)(&pt[x+y])) ^ *((LTC_FAST_TYPE*)(&gcm->buf[y]));
                 *((LTC_FAST_TYPE*)(&gcm->X[y])) ^= *((LTC_FAST_TYPE*)(&ct[x+y]));
             }
             /* GMAC it */
             gcm->pttotlen += 128;
             gcm_mult_h(gcm, gcm->X);
             /* increment counter */
             for (y = 15; y >= 12; y--) {
                 if (++gcm->Y[y] & 255) { break; }
             }
             if ((err = cipher_descriptor[gcm->cipher].ecb_encrypt(gcm->Y, gcm->buf, &gcm->K)) != CRYPT_OK) {
                return err;
             }
         }
      } else {
         for (x = 0; x < (ptlen & ~15); x += 16) {
             /* ctr encrypt */
             for (y = 0; y < 16; y += sizeof(LTC_FAST_TYPE)) {
                 *((LTC_FAST_TYPE*)(&gcm->X[y])) ^= *((LTC_FAST_TYPE*)(&ct[x+y]));
                 *((LTC_FAST_TYPE*)(&pt[x + y])) = *((LTC_FAST_TYPE*)(&ct[x+y])) ^ *((LTC_FAST_TYPE*)(&gcm->buf[y]));
             }
             /* GMAC it */
             gcm->pttotlen += 128;
             gcm_mult_h(gcm, gcm->X);
             /* increment counter */
             for (y = 15; y >= 12; y--) {
                 if (++gcm->Y[y] & 255) { break; }
             }
             if ((err = cipher_descriptor[gcm->cipher].ecb_encrypt(gcm->Y, gcm->buf, &gcm->K)) != CRYPT_OK) {
                return err;
             }
         }
     }
   }
#endif

   /* process text */
   for (; x < ptlen; x++) {
       if (gcm->buflen == 16) {
          gcm->pttotlen += 128;
          gcm_mult_h(gcm, gcm->X);

          /* increment counter */
          for (y = 15; y >= 12; y--) {
              if (++gcm->Y[y] & 255) { break; }
          }
          if ((err = cipher_descriptor[gcm->cipher].ecb_encrypt(gcm->Y, gcm->buf, &gcm->K)) != CRYPT_OK) {
             return err;
          }
          gcm->buflen = 0;
       }

       if (direction == GCM_ENCRYPT) {
          b = ct[x] = pt[x] ^ gcm->buf[gcm->buflen];
       } else {
          b = ct[x];
          pt[x] = ct[x] ^ gcm->buf[gcm->buflen];
       }
       gcm->X[gcm->buflen++] ^= b;
   }

   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/encauth/gcm/gcm_process.c,v $ */
/* $Revision: 1.16 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/**
   @file gcm_mult_h.c
   GCM implementation, do the GF mult, by Tom St Denis
*/


#if defined(LTC_GCM_MODE)
/**
  GCM multiply by H
  @param gcm   The GCM state which holds the H value
  @param I     The value to multiply H by
 */
void gcm_mult_h(gcm_state *gcm, unsigned char *I)
{
   unsigned char T[16];
#ifdef LTC_GCM_TABLES
   int x, y;
#ifdef LTC_GCM_TABLES_SSE2
   asm("movdqa (%0),%%xmm0"::"r"(&gcm->PC[0][I[0]][0]));
   for (x = 1; x < 16; x++) {
      asm("pxor (%0),%%xmm0"::"r"(&gcm->PC[x][I[x]][0]));
   }
   asm("movdqa %%xmm0,(%0)"::"r"(&T));
#else
   XMEMCPY(T, &gcm->PC[0][I[0]][0], 16);
   for (x = 1; x < 16; x++) {
#ifdef LTC_FAST
       for (y = 0; y < 16; y += sizeof(LTC_FAST_TYPE)) {
           *((LTC_FAST_TYPE *)(T + y)) ^= *((LTC_FAST_TYPE *)(&gcm->PC[x][I[x]][y]));
       }
#else
       for (y = 0; y < 16; y++) {
           T[y] ^= gcm->PC[x][I[x]][y];
       }
#endif /* LTC_FAST */
   }
#endif /* LTC_GCM_TABLES_SSE2 */
#else
   gcm_gf_mult(gcm->H, I, T);
#endif
   XMEMCPY(I, T, 16);
}
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/encauth/gcm/gcm_mult_h.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2007/05/12 14:32:35 $ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/**
   @file gcm_gf_mult.c
   GCM implementation, do the GF mult, by Tom St Denis
*/


#if defined(LTC_GCM_TABLES) || defined(LRW_TABLES) || ((defined(LTC_GCM_MODE) || defined(LTC_GCM_MODE)) && defined(LTC_FAST))

/* this is x*2^128 mod p(x) ... the results are 16 bytes each stored in a packed format.  Since only the
 * lower 16 bits are not zero'ed I removed the upper 14 bytes */
const unsigned char gcm_shift_table[256*2] = {
0x00, 0x00, 0x01, 0xc2, 0x03, 0x84, 0x02, 0x46, 0x07, 0x08, 0x06, 0xca, 0x04, 0x8c, 0x05, 0x4e,
0x0e, 0x10, 0x0f, 0xd2, 0x0d, 0x94, 0x0c, 0x56, 0x09, 0x18, 0x08, 0xda, 0x0a, 0x9c, 0x0b, 0x5e,
0x1c, 0x20, 0x1d, 0xe2, 0x1f, 0xa4, 0x1e, 0x66, 0x1b, 0x28, 0x1a, 0xea, 0x18, 0xac, 0x19, 0x6e,
0x12, 0x30, 0x13, 0xf2, 0x11, 0xb4, 0x10, 0x76, 0x15, 0x38, 0x14, 0xfa, 0x16, 0xbc, 0x17, 0x7e,
0x38, 0x40, 0x39, 0x82, 0x3b, 0xc4, 0x3a, 0x06, 0x3f, 0x48, 0x3e, 0x8a, 0x3c, 0xcc, 0x3d, 0x0e,
0x36, 0x50, 0x37, 0x92, 0x35, 0xd4, 0x34, 0x16, 0x31, 0x58, 0x30, 0x9a, 0x32, 0xdc, 0x33, 0x1e,
0x24, 0x60, 0x25, 0xa2, 0x27, 0xe4, 0x26, 0x26, 0x23, 0x68, 0x22, 0xaa, 0x20, 0xec, 0x21, 0x2e,
0x2a, 0x70, 0x2b, 0xb2, 0x29, 0xf4, 0x28, 0x36, 0x2d, 0x78, 0x2c, 0xba, 0x2e, 0xfc, 0x2f, 0x3e,
0x70, 0x80, 0x71, 0x42, 0x73, 0x04, 0x72, 0xc6, 0x77, 0x88, 0x76, 0x4a, 0x74, 0x0c, 0x75, 0xce,
0x7e, 0x90, 0x7f, 0x52, 0x7d, 0x14, 0x7c, 0xd6, 0x79, 0x98, 0x78, 0x5a, 0x7a, 0x1c, 0x7b, 0xde,
0x6c, 0xa0, 0x6d, 0x62, 0x6f, 0x24, 0x6e, 0xe6, 0x6b, 0xa8, 0x6a, 0x6a, 0x68, 0x2c, 0x69, 0xee,
0x62, 0xb0, 0x63, 0x72, 0x61, 0x34, 0x60, 0xf6, 0x65, 0xb8, 0x64, 0x7a, 0x66, 0x3c, 0x67, 0xfe,
0x48, 0xc0, 0x49, 0x02, 0x4b, 0x44, 0x4a, 0x86, 0x4f, 0xc8, 0x4e, 0x0a, 0x4c, 0x4c, 0x4d, 0x8e,
0x46, 0xd0, 0x47, 0x12, 0x45, 0x54, 0x44, 0x96, 0x41, 0xd8, 0x40, 0x1a, 0x42, 0x5c, 0x43, 0x9e,
0x54, 0xe0, 0x55, 0x22, 0x57, 0x64, 0x56, 0xa6, 0x53, 0xe8, 0x52, 0x2a, 0x50, 0x6c, 0x51, 0xae,
0x5a, 0xf0, 0x5b, 0x32, 0x59, 0x74, 0x58, 0xb6, 0x5d, 0xf8, 0x5c, 0x3a, 0x5e, 0x7c, 0x5f, 0xbe,
0xe1, 0x00, 0xe0, 0xc2, 0xe2, 0x84, 0xe3, 0x46, 0xe6, 0x08, 0xe7, 0xca, 0xe5, 0x8c, 0xe4, 0x4e,
0xef, 0x10, 0xee, 0xd2, 0xec, 0x94, 0xed, 0x56, 0xe8, 0x18, 0xe9, 0xda, 0xeb, 0x9c, 0xea, 0x5e,
0xfd, 0x20, 0xfc, 0xe2, 0xfe, 0xa4, 0xff, 0x66, 0xfa, 0x28, 0xfb, 0xea, 0xf9, 0xac, 0xf8, 0x6e,
0xf3, 0x30, 0xf2, 0xf2, 0xf0, 0xb4, 0xf1, 0x76, 0xf4, 0x38, 0xf5, 0xfa, 0xf7, 0xbc, 0xf6, 0x7e,
0xd9, 0x40, 0xd8, 0x82, 0xda, 0xc4, 0xdb, 0x06, 0xde, 0x48, 0xdf, 0x8a, 0xdd, 0xcc, 0xdc, 0x0e,
0xd7, 0x50, 0xd6, 0x92, 0xd4, 0xd4, 0xd5, 0x16, 0xd0, 0x58, 0xd1, 0x9a, 0xd3, 0xdc, 0xd2, 0x1e,
0xc5, 0x60, 0xc4, 0xa2, 0xc6, 0xe4, 0xc7, 0x26, 0xc2, 0x68, 0xc3, 0xaa, 0xc1, 0xec, 0xc0, 0x2e,
0xcb, 0x70, 0xca, 0xb2, 0xc8, 0xf4, 0xc9, 0x36, 0xcc, 0x78, 0xcd, 0xba, 0xcf, 0xfc, 0xce, 0x3e,
0x91, 0x80, 0x90, 0x42, 0x92, 0x04, 0x93, 0xc6, 0x96, 0x88, 0x97, 0x4a, 0x95, 0x0c, 0x94, 0xce,
0x9f, 0x90, 0x9e, 0x52, 0x9c, 0x14, 0x9d, 0xd6, 0x98, 0x98, 0x99, 0x5a, 0x9b, 0x1c, 0x9a, 0xde,
0x8d, 0xa0, 0x8c, 0x62, 0x8e, 0x24, 0x8f, 0xe6, 0x8a, 0xa8, 0x8b, 0x6a, 0x89, 0x2c, 0x88, 0xee,
0x83, 0xb0, 0x82, 0x72, 0x80, 0x34, 0x81, 0xf6, 0x84, 0xb8, 0x85, 0x7a, 0x87, 0x3c, 0x86, 0xfe,
0xa9, 0xc0, 0xa8, 0x02, 0xaa, 0x44, 0xab, 0x86, 0xae, 0xc8, 0xaf, 0x0a, 0xad, 0x4c, 0xac, 0x8e,
0xa7, 0xd0, 0xa6, 0x12, 0xa4, 0x54, 0xa5, 0x96, 0xa0, 0xd8, 0xa1, 0x1a, 0xa3, 0x5c, 0xa2, 0x9e,
0xb5, 0xe0, 0xb4, 0x22, 0xb6, 0x64, 0xb7, 0xa6, 0xb2, 0xe8, 0xb3, 0x2a, 0xb1, 0x6c, 0xb0, 0xae,
0xbb, 0xf0, 0xba, 0x32, 0xb8, 0x74, 0xb9, 0xb6, 0xbc, 0xf8, 0xbd, 0x3a, 0xbf, 0x7c, 0xbe, 0xbe };

#endif


#if defined(LTC_GCM_MODE) || defined(LRW_MODE)

#ifndef LTC_FAST
/* right shift */
static void gcm_rightshift(unsigned char *a)
{
   int x;
   for (x = 15; x > 0; x--) {
       a[x] = (a[x]>>1) | ((a[x-1]<<7)&0x80);
   }
   a[0] >>= 1;
}

/* c = b*a */
static const unsigned char mask[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
static const unsigned char poly[] = { 0x00, 0xE1 };


/**
  GCM GF multiplier (internal use only)  bitserial
  @param a   First value
  @param b   Second value
  @param c   Destination for a * b
 */
void gcm_gf_mult(const unsigned char *a, const unsigned char *b, unsigned char *c)
{
   unsigned char Z[16], V[16];
   unsigned char x, y, z;

   zeromem(Z, 16);
   XMEMCPY(V, a, 16);
   for (x = 0; x < 128; x++) {
       if (b[x>>3] & mask[x&7]) {
          for (y = 0; y < 16; y++) {
              Z[y] ^= V[y];
          }
       }
       z     = V[15] & 0x01;
       gcm_rightshift(V);
       V[0] ^= poly[z];
   }
   XMEMCPY(c, Z, 16);
}

#else

/* map normal numbers to "ieee" way ... e.g. bit reversed */
#define M(x) ( ((x&8)>>3) | ((x&4)>>1) | ((x&2)<<1) | ((x&1)<<3) )

#define BPD (sizeof(LTC_FAST_TYPE) * 8)
#define WPV (1 + (16 / sizeof(LTC_FAST_TYPE)))

/**
  GCM GF multiplier (internal use only)  word oriented
  @param a   First value
  @param b   Second value
  @param c   Destination for a * b
 */
void gcm_gf_mult(const unsigned char *a, const unsigned char *b, unsigned char *c)
{
   int i, j, k, u;
   LTC_FAST_TYPE B[16][WPV], tmp[32 / sizeof(LTC_FAST_TYPE)], pB[16 / sizeof(LTC_FAST_TYPE)], zz, z;
   unsigned char pTmp[32];

   /* create simple tables */
   zeromem(B[0],       sizeof(B[0]));
   zeromem(B[M(1)],    sizeof(B[M(1)]));

#ifdef ENDIAN_32BITWORD
   for (i = 0; i < 4; i++) {
       LOAD32H(B[M(1)][i], a + (i<<2));
       LOAD32L(pB[i],      b + (i<<2));
   }
#else
   for (i = 0; i < 2; i++) {
       LOAD64H(B[M(1)][i], a + (i<<3));
       LOAD64L(pB[i],      b + (i<<3));
   }
#endif

   /* now create 2, 4 and 8 */
   B[M(2)][0] = B[M(1)][0] >> 1;
   B[M(4)][0] = B[M(1)][0] >> 2;
   B[M(8)][0] = B[M(1)][0] >> 3;
   for (i = 1; i < (int)WPV; i++) {
      B[M(2)][i] = (B[M(1)][i-1] << (BPD-1)) | (B[M(1)][i] >> 1);
      B[M(4)][i] = (B[M(1)][i-1] << (BPD-2)) | (B[M(1)][i] >> 2);
      B[M(8)][i] = (B[M(1)][i-1] << (BPD-3)) | (B[M(1)][i] >> 3);
   }

   /*  now all values with two bits which are 3, 5, 6, 9, 10, 12 */
   for (i = 0; i < (int)WPV; i++) {
      B[M(3)][i]  = B[M(1)][i] ^ B[M(2)][i];
      B[M(5)][i]  = B[M(1)][i] ^ B[M(4)][i];
      B[M(6)][i]  = B[M(2)][i] ^ B[M(4)][i];
      B[M(9)][i]  = B[M(1)][i] ^ B[M(8)][i];
      B[M(10)][i] = B[M(2)][i] ^ B[M(8)][i];
      B[M(12)][i] = B[M(8)][i] ^ B[M(4)][i];

   /*  now all 3 bit values and the only 4 bit value: 7, 11, 13, 14, 15 */
      B[M(7)][i]  = B[M(3)][i] ^ B[M(4)][i];
      B[M(11)][i] = B[M(3)][i] ^ B[M(8)][i];
      B[M(13)][i] = B[M(1)][i] ^ B[M(12)][i];
      B[M(14)][i] = B[M(6)][i] ^ B[M(8)][i];
      B[M(15)][i] = B[M(7)][i] ^ B[M(8)][i];
   }

   zeromem(tmp, sizeof(tmp));

   /* compute product four bits of each word at a time */
   /* for each nibble */
   for (i = (BPD/4)-1; i >= 0; i--) {
       /* for each word */
       for (j = 0; j < (int)(WPV-1); j++) {
        /* grab the 4 bits recall the nibbles are backwards so it's a shift by (i^1)*4 */
           u = (pB[j] >> ((i^1)<<2)) & 15;

        /* add offset by the word count the table looked up value to the result */
           for (k = 0; k < (int)WPV; k++) {
               tmp[k+j] ^= B[u][k];
           }
       }
     /* shift result up by 4 bits */
       if (i != 0) {
          for (z = j = 0; j < (int)(32 / sizeof(LTC_FAST_TYPE)); j++) {
              zz = tmp[j] << (BPD-4);
              tmp[j] = (tmp[j] >> 4) | z;
              z = zz;
          }
       }
   }

   /* store product */
#ifdef ENDIAN_32BITWORD
   for (i = 0; i < 8; i++) {
       STORE32H(tmp[i], pTmp + (i<<2));
   }
#else
   for (i = 0; i < 4; i++) {
       STORE64H(tmp[i], pTmp + (i<<3));
   }
#endif

   /* reduce by taking most significant byte and adding the appropriate two byte sequence 16 bytes down */
   for (i = 31; i >= 16; i--) {
       pTmp[i-16] ^= gcm_shift_table[((unsigned)pTmp[i]<<1)];
       pTmp[i-15] ^= gcm_shift_table[((unsigned)pTmp[i]<<1)+1];
   }

   for (i = 0; i < 16; i++) {
       c[i] = pTmp[i];
   }

}

#endif

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/encauth/gcm/gcm_gf_mult.c,v $ */
/* $Revision: 1.25 $ */
/* $Date: 2007/05/12 14:32:35 $ */



/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/**
   @file gcm_add_aad.c
   GCM implementation, Add AAD data to the stream, by Tom St Denis
*/


#ifdef LTC_GCM_MODE

/**
  Add AAD to the GCM state
  @param gcm       The GCM state
  @param adata     The additional authentication data to add to the GCM state
  @param adatalen  The length of the AAD data.
  @return CRYPT_OK on success
 */
int gcm_add_aad(gcm_state *gcm,
               const unsigned char *adata,  unsigned long adatalen)
{
   unsigned long x;
   int           err;
#ifdef LTC_FAST
   unsigned long y;
#endif

   LTC_ARGCHK(gcm    != NULL);
   if (adatalen > 0) {
      LTC_ARGCHK(adata  != NULL);
   }

   if (gcm->buflen > 16 || gcm->buflen < 0) {
      return CRYPT_INVALID_ARG;
   }

   if ((err = cipher_is_valid(gcm->cipher)) != CRYPT_OK) {
      return err;
   }

   /* in IV mode? */
   if (gcm->mode == LTC_GCM_MODE_IV) {
      /* let's process the IV */
      if (gcm->ivmode || gcm->buflen != 12) {
         for (x = 0; x < (unsigned long)gcm->buflen; x++) {
             gcm->X[x] ^= gcm->buf[x];
         }
         if (gcm->buflen) {
            gcm->totlen += gcm->buflen * CONST64(8);
            gcm_mult_h(gcm, gcm->X);
         }

         /* mix in the length */
         zeromem(gcm->buf, 8);
         STORE64H(gcm->totlen, gcm->buf+8);
         for (x = 0; x < 16; x++) {
             gcm->X[x] ^= gcm->buf[x];
         }
         gcm_mult_h(gcm, gcm->X);

         /* copy counter out */
         XMEMCPY(gcm->Y, gcm->X, 16);
         zeromem(gcm->X, 16);
      } else {
         XMEMCPY(gcm->Y, gcm->buf, 12);
         gcm->Y[12] = 0;
         gcm->Y[13] = 0;
         gcm->Y[14] = 0;
         gcm->Y[15] = 1;
      }
      XMEMCPY(gcm->Y_0, gcm->Y, 16);
      zeromem(gcm->buf, 16);
      gcm->buflen = 0;
      gcm->totlen = 0;
      gcm->mode   = LTC_GCM_MODE_AAD;
   }

   if (gcm->mode != LTC_GCM_MODE_AAD || gcm->buflen >= 16) {
      return CRYPT_INVALID_ARG;
   }

   x = 0;
#ifdef LTC_FAST
   if (gcm->buflen == 0) {
      for (x = 0; x < (adatalen & ~15); x += 16) {
          for (y = 0; y < 16; y += sizeof(LTC_FAST_TYPE)) {
              *((LTC_FAST_TYPE*)(&gcm->X[y])) ^= *((LTC_FAST_TYPE*)(&adata[x + y]));
          }
          gcm_mult_h(gcm, gcm->X);
          gcm->totlen += 128;
      }
      adata += x;
   }
#endif


   /* start adding AAD data to the state */
   for (; x < adatalen; x++) {
       gcm->X[gcm->buflen++] ^= *adata++;

       if (gcm->buflen == 16) {
         /* GF mult it */
         gcm_mult_h(gcm, gcm->X);
         gcm->buflen = 0;
         gcm->totlen += 128;
      }
   }

   return CRYPT_OK;
}
#endif


/* $Source$ */
/* $Revision$ */
/* $Date$ */


/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/**
   @file gcm_reset.c
   GCM implementation, reset a used state so it can accept IV data, by Tom St Denis
*/


#ifdef LTC_GCM_MODE

/**
  Reset a GCM state to as if you just called gcm_init().  This saves the initialization time.
  @param gcm   The GCM state to reset
  @return CRYPT_OK on success
*/
int gcm_reset(gcm_state *gcm)
{
   LTC_ARGCHK(gcm != NULL);

   zeromem(gcm->buf, sizeof(gcm->buf));
   zeromem(gcm->X,   sizeof(gcm->X));
   gcm->mode     = LTC_GCM_MODE_IV;
   gcm->ivmode   = 0;
   gcm->buflen   = 0;
   gcm->totlen   = 0;
   gcm->pttotlen = 0;

   return CRYPT_OK;
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */

/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */



/**
  @file md5.c
  LTC_MD5 hash function by Tom St Denis
*/

#ifdef LTC_MD5

const struct ltc_hash_descriptor md5_desc =
{
    "md5",
    3,
    16,
    64,

    /* OID */
   { 1, 2, 840, 113549, 2, 5,  },
   6,

    &md5_init,
    &md5_process,
    &md5_done,
    &md5_test,
    NULL
};

#define F(x,y,z)  (z ^ (x & (y ^ z)))
#define G(x,y,z)  (y ^ (z & (y ^ x)))
#define H(x,y,z)  (x^y^z)
#define I(x,y,z)  (y^(x|(~z)))

#ifdef LTC_SMALL_CODE

#define FF(a,b,c,d,M,s,t) \
    a = (a + F(b,c,d) + M + t); a = ROL(a, s) + b;

#define GG(a,b,c,d,M,s,t) \
    a = (a + G(b,c,d) + M + t); a = ROL(a, s) + b;

#define HH(a,b,c,d,M,s,t) \
    a = (a + H(b,c,d) + M + t); a = ROL(a, s) + b;

#define II(a,b,c,d,M,s,t) \
    a = (a + I(b,c,d) + M + t); a = ROL(a, s) + b;

static const unsigned char Worder[64] = {
   0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
   1,6,11,0,5,10,15,4,9,14,3,8,13,2,7,12,
   5,8,11,14,1,4,7,10,13,0,3,6,9,12,15,2,
   0,7,14,5,12,3,10,1,8,15,6,13,4,11,2,9
};

static const unsigned char Rorder[64] = {
   7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
   5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
   4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
   6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21
};

static const ulong32 Korder[64] = {
0xd76aa478UL, 0xe8c7b756UL, 0x242070dbUL, 0xc1bdceeeUL, 0xf57c0fafUL, 0x4787c62aUL, 0xa8304613UL, 0xfd469501UL,
0x698098d8UL, 0x8b44f7afUL, 0xffff5bb1UL, 0x895cd7beUL, 0x6b901122UL, 0xfd987193UL, 0xa679438eUL, 0x49b40821UL,
0xf61e2562UL, 0xc040b340UL, 0x265e5a51UL, 0xe9b6c7aaUL, 0xd62f105dUL, 0x02441453UL, 0xd8a1e681UL, 0xe7d3fbc8UL,
0x21e1cde6UL, 0xc33707d6UL, 0xf4d50d87UL, 0x455a14edUL, 0xa9e3e905UL, 0xfcefa3f8UL, 0x676f02d9UL, 0x8d2a4c8aUL,
0xfffa3942UL, 0x8771f681UL, 0x6d9d6122UL, 0xfde5380cUL, 0xa4beea44UL, 0x4bdecfa9UL, 0xf6bb4b60UL, 0xbebfbc70UL,
0x289b7ec6UL, 0xeaa127faUL, 0xd4ef3085UL, 0x04881d05UL, 0xd9d4d039UL, 0xe6db99e5UL, 0x1fa27cf8UL, 0xc4ac5665UL,
0xf4292244UL, 0x432aff97UL, 0xab9423a7UL, 0xfc93a039UL, 0x655b59c3UL, 0x8f0ccc92UL, 0xffeff47dUL, 0x85845dd1UL,
0x6fa87e4fUL, 0xfe2ce6e0UL, 0xa3014314UL, 0x4e0811a1UL, 0xf7537e82UL, 0xbd3af235UL, 0x2ad7d2bbUL, 0xeb86d391UL
};

#else

#define FF(a,b,c,d,M,s,t) \
    a = (a + F(b,c,d) + M + t); a = ROLc(a, s) + b;

#define GG(a,b,c,d,M,s,t) \
    a = (a + G(b,c,d) + M + t); a = ROLc(a, s) + b;

#define HH(a,b,c,d,M,s,t) \
    a = (a + H(b,c,d) + M + t); a = ROLc(a, s) + b;

#define II(a,b,c,d,M,s,t) \
    a = (a + I(b,c,d) + M + t); a = ROLc(a, s) + b;


#endif

#ifdef LTC_CLEAN_STACK
static int _md5_compress(hash_state *md, unsigned char *buf)
#else
static int  md5_compress(hash_state *md, unsigned char *buf)
#endif
{
    ulong32 i, W[16], a, b, c, d;
#ifdef LTC_SMALL_CODE
    ulong32 t;
#endif

    /* copy the state into 512-bits into W[0..15] */
    for (i = 0; i < 16; i++) {
        LOAD32L(W[i], buf + (4*i));
    }

    /* copy state */
    a = md->md5.state[0];
    b = md->md5.state[1];
    c = md->md5.state[2];
    d = md->md5.state[3];

#ifdef LTC_SMALL_CODE
    for (i = 0; i < 16; ++i) {
        FF(a,b,c,d,W[Worder[i]],Rorder[i],Korder[i]);
        t = d; d = c; c = b; b = a; a = t;
    }

    for (; i < 32; ++i) {
        GG(a,b,c,d,W[Worder[i]],Rorder[i],Korder[i]);
        t = d; d = c; c = b; b = a; a = t;
    }

    for (; i < 48; ++i) {
        HH(a,b,c,d,W[Worder[i]],Rorder[i],Korder[i]);
        t = d; d = c; c = b; b = a; a = t;
    }

    for (; i < 64; ++i) {
        II(a,b,c,d,W[Worder[i]],Rorder[i],Korder[i]);
        t = d; d = c; c = b; b = a; a = t;
    }

#else
    FF(a,b,c,d,W[0],7,0xd76aa478UL)
    FF(d,a,b,c,W[1],12,0xe8c7b756UL)
    FF(c,d,a,b,W[2],17,0x242070dbUL)
    FF(b,c,d,a,W[3],22,0xc1bdceeeUL)
    FF(a,b,c,d,W[4],7,0xf57c0fafUL)
    FF(d,a,b,c,W[5],12,0x4787c62aUL)
    FF(c,d,a,b,W[6],17,0xa8304613UL)
    FF(b,c,d,a,W[7],22,0xfd469501UL)
    FF(a,b,c,d,W[8],7,0x698098d8UL)
    FF(d,a,b,c,W[9],12,0x8b44f7afUL)
    FF(c,d,a,b,W[10],17,0xffff5bb1UL)
    FF(b,c,d,a,W[11],22,0x895cd7beUL)
    FF(a,b,c,d,W[12],7,0x6b901122UL)
    FF(d,a,b,c,W[13],12,0xfd987193UL)
    FF(c,d,a,b,W[14],17,0xa679438eUL)
    FF(b,c,d,a,W[15],22,0x49b40821UL)
    GG(a,b,c,d,W[1],5,0xf61e2562UL)
    GG(d,a,b,c,W[6],9,0xc040b340UL)
    GG(c,d,a,b,W[11],14,0x265e5a51UL)
    GG(b,c,d,a,W[0],20,0xe9b6c7aaUL)
    GG(a,b,c,d,W[5],5,0xd62f105dUL)
    GG(d,a,b,c,W[10],9,0x02441453UL)
    GG(c,d,a,b,W[15],14,0xd8a1e681UL)
    GG(b,c,d,a,W[4],20,0xe7d3fbc8UL)
    GG(a,b,c,d,W[9],5,0x21e1cde6UL)
    GG(d,a,b,c,W[14],9,0xc33707d6UL)
    GG(c,d,a,b,W[3],14,0xf4d50d87UL)
    GG(b,c,d,a,W[8],20,0x455a14edUL)
    GG(a,b,c,d,W[13],5,0xa9e3e905UL)
    GG(d,a,b,c,W[2],9,0xfcefa3f8UL)
    GG(c,d,a,b,W[7],14,0x676f02d9UL)
    GG(b,c,d,a,W[12],20,0x8d2a4c8aUL)
    HH(a,b,c,d,W[5],4,0xfffa3942UL)
    HH(d,a,b,c,W[8],11,0x8771f681UL)
    HH(c,d,a,b,W[11],16,0x6d9d6122UL)
    HH(b,c,d,a,W[14],23,0xfde5380cUL)
    HH(a,b,c,d,W[1],4,0xa4beea44UL)
    HH(d,a,b,c,W[4],11,0x4bdecfa9UL)
    HH(c,d,a,b,W[7],16,0xf6bb4b60UL)
    HH(b,c,d,a,W[10],23,0xbebfbc70UL)
    HH(a,b,c,d,W[13],4,0x289b7ec6UL)
    HH(d,a,b,c,W[0],11,0xeaa127faUL)
    HH(c,d,a,b,W[3],16,0xd4ef3085UL)
    HH(b,c,d,a,W[6],23,0x04881d05UL)
    HH(a,b,c,d,W[9],4,0xd9d4d039UL)
    HH(d,a,b,c,W[12],11,0xe6db99e5UL)
    HH(c,d,a,b,W[15],16,0x1fa27cf8UL)
    HH(b,c,d,a,W[2],23,0xc4ac5665UL)
    II(a,b,c,d,W[0],6,0xf4292244UL)
    II(d,a,b,c,W[7],10,0x432aff97UL)
    II(c,d,a,b,W[14],15,0xab9423a7UL)
    II(b,c,d,a,W[5],21,0xfc93a039UL)
    II(a,b,c,d,W[12],6,0x655b59c3UL)
    II(d,a,b,c,W[3],10,0x8f0ccc92UL)
    II(c,d,a,b,W[10],15,0xffeff47dUL)
    II(b,c,d,a,W[1],21,0x85845dd1UL)
    II(a,b,c,d,W[8],6,0x6fa87e4fUL)
    II(d,a,b,c,W[15],10,0xfe2ce6e0UL)
    II(c,d,a,b,W[6],15,0xa3014314UL)
    II(b,c,d,a,W[13],21,0x4e0811a1UL)
    II(a,b,c,d,W[4],6,0xf7537e82UL)
    II(d,a,b,c,W[11],10,0xbd3af235UL)
    II(c,d,a,b,W[2],15,0x2ad7d2bbUL)
    II(b,c,d,a,W[9],21,0xeb86d391UL)
#endif

    md->md5.state[0] = md->md5.state[0] + a;
    md->md5.state[1] = md->md5.state[1] + b;
    md->md5.state[2] = md->md5.state[2] + c;
    md->md5.state[3] = md->md5.state[3] + d;

    return CRYPT_OK;
}

#ifdef LTC_CLEAN_STACK
static int md5_compress(hash_state *md, unsigned char *buf)
{
   int err;
   err = _md5_compress(md, buf);
   burn_stack(sizeof(ulong32) * 21);
   return err;
}
#endif

/**
   Initialize the hash state
   @param md   The hash state you wish to initialize
   @return CRYPT_OK if successful
*/
int md5_init(hash_state * md)
{
   LTC_ARGCHK(md != NULL);
   md->md5.state[0] = 0x67452301UL;
   md->md5.state[1] = 0xefcdab89UL;
   md->md5.state[2] = 0x98badcfeUL;
   md->md5.state[3] = 0x10325476UL;
   md->md5.curlen = 0;
   md->md5.length = 0;
   return CRYPT_OK;
}

/**
   Process a block of memory though the hash
   @param md     The hash state
   @param in     The data to hash
   @param inlen  The length of the data (octets)
   @return CRYPT_OK if successful
*/
HASH_PROCESS(md5_process, md5_compress, md5, 64)

/**
   Terminate the hash to get the digest
   @param md  The hash state
   @param out [out] The destination of the hash (16 bytes)
   @return CRYPT_OK if successful
*/
int md5_done(hash_state * md, unsigned char *out)
{
    int i;

    LTC_ARGCHK(md  != NULL);
    LTC_ARGCHK(out != NULL);

    if (md->md5.curlen >= sizeof(md->md5.buf)) {
       return CRYPT_INVALID_ARG;
    }


    /* increase the length of the message */
    md->md5.length += md->md5.curlen * 8;

    /* append the '1' bit */
    md->md5.buf[md->md5.curlen++] = (unsigned char)0x80;

    /* if the length is currently above 56 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (md->md5.curlen > 56) {
        while (md->md5.curlen < 64) {
            md->md5.buf[md->md5.curlen++] = (unsigned char)0;
        }
        md5_compress(md, md->md5.buf);
        md->md5.curlen = 0;
    }

    /* pad upto 56 bytes of zeroes */
    while (md->md5.curlen < 56) {
        md->md5.buf[md->md5.curlen++] = (unsigned char)0;
    }

    /* store length */
    STORE64L(md->md5.length, md->md5.buf+56);
    md5_compress(md, md->md5.buf);

    /* copy output */
    for (i = 0; i < 4; i++) {
        STORE32L(md->md5.state[i], out+(4*i));
    }
#ifdef LTC_CLEAN_STACK
    zeromem(md, sizeof(hash_state));
#endif
    return CRYPT_OK;
}

/**
  Self-test the hash
  @return CRYPT_OK if successful, CRYPT_NOP if self-tests have been disabled
*/
int  md5_test(void)
{
 #ifndef LTC_TEST
    return CRYPT_NOP;
 #else
  static const struct {
      char *msg;
      unsigned char hash[16];
  } tests[] = {
    { "",
      { 0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
        0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e } },
    { "a",
      {0x0c, 0xc1, 0x75, 0xb9, 0xc0, 0xf1, 0xb6, 0xa8,
       0x31, 0xc3, 0x99, 0xe2, 0x69, 0x77, 0x26, 0x61 } },
    { "abc",
      { 0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
        0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72 } },
    { "message digest",
      { 0xf9, 0x6b, 0x69, 0x7d, 0x7c, 0xb7, 0x93, 0x8d,
        0x52, 0x5a, 0x2f, 0x31, 0xaa, 0xf1, 0x61, 0xd0 } },
    { "abcdefghijklmnopqrstuvwxyz",
      { 0xc3, 0xfc, 0xd3, 0xd7, 0x61, 0x92, 0xe4, 0x00,
        0x7d, 0xfb, 0x49, 0x6c, 0xca, 0x67, 0xe1, 0x3b } },
    { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
      { 0xd1, 0x74, 0xab, 0x98, 0xd2, 0x77, 0xd9, 0xf5,
        0xa5, 0x61, 0x1c, 0x2c, 0x9f, 0x41, 0x9d, 0x9f } },
    { "12345678901234567890123456789012345678901234567890123456789012345678901234567890",
      { 0x57, 0xed, 0xf4, 0xa2, 0x2b, 0xe3, 0xc9, 0x55,
        0xac, 0x49, 0xda, 0x2e, 0x21, 0x07, 0xb6, 0x7a } },
    { NULL, { 0 } }
  };

  int i;
  unsigned char tmp[16];
  hash_state md;

  for (i = 0; tests[i].msg != NULL; i++) {
      md5_init(&md);
      md5_process(&md, (unsigned char *)tests[i].msg, (unsigned long)strlen(tests[i].msg));
      md5_done(&md, tmp);
      if (XMEMCMP(tmp, tests[i].hash, 16) != 0) {
         return CRYPT_FAIL_TESTVECTOR;
      }
  }
  return CRYPT_OK;
 #endif
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */

/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/**
  @file ctr_encrypt.c
  CTR implementation, encrypt data, Tom St Denis
*/


#ifdef LTC_CTR_MODE

/**
  CTR encrypt
  @param pt     Plaintext
  @param ct     [out] Ciphertext
  @param len    Length of plaintext (octets)
  @param ctr    CTR state
  @return CRYPT_OK if successful
*/
int ctr_encrypt(const unsigned char *pt, unsigned char *ct, unsigned long len, symmetric_CTR *ctr)
{
   int x, err;

   LTC_ARGCHK(pt != NULL);
   LTC_ARGCHK(ct != NULL);
   LTC_ARGCHK(ctr != NULL);

   if ((err = cipher_is_valid(ctr->cipher)) != CRYPT_OK) {
       return err;
   }

   /* is blocklen/padlen valid? */
   if (ctr->blocklen < 1 || ctr->blocklen > (int)sizeof(ctr->ctr) ||
       ctr->padlen   < 0 || ctr->padlen   > (int)sizeof(ctr->pad)) {
      return CRYPT_INVALID_ARG;
   }

#ifdef LTC_FAST
   if (ctr->blocklen % sizeof(LTC_FAST_TYPE)) {
      return CRYPT_INVALID_ARG;
   }
#endif

   /* handle acceleration only if pad is empty, accelerator is present and length is >= a block size */
   if ((ctr->padlen == ctr->blocklen) && cipher_descriptor[ctr->cipher].accel_ctr_encrypt != NULL && (len >= (unsigned long)ctr->blocklen)) {
      if ((err = cipher_descriptor[ctr->cipher].accel_ctr_encrypt(pt, ct, len/ctr->blocklen, ctr->ctr, ctr->mode, &ctr->key)) != CRYPT_OK) {
         return err;
      }
      len %= ctr->blocklen;
   }

   while (len) {
      /* is the pad empty? */
      if (ctr->padlen == ctr->blocklen) {
         /* increment counter */
         if (ctr->mode == CTR_COUNTER_LITTLE_ENDIAN) {
            /* little-endian */
            for (x = 0; x < ctr->ctrlen; x++) {
               ctr->ctr[x] = (ctr->ctr[x] + (unsigned char)1) & (unsigned char)255;
               if (ctr->ctr[x] != (unsigned char)0) {
                  break;
               }
            }
         } else {
            /* big-endian */
            for (x = ctr->blocklen-1; x >= ctr->ctrlen; x--) {
               ctr->ctr[x] = (ctr->ctr[x] + (unsigned char)1) & (unsigned char)255;
               if (ctr->ctr[x] != (unsigned char)0) {
                  break;
               }
            }
         }

         /* encrypt it */
         if ((err = cipher_descriptor[ctr->cipher].ecb_encrypt(ctr->ctr, ctr->pad, &ctr->key)) != CRYPT_OK) {
            return err;
         }
         ctr->padlen = 0;
      }
#ifdef LTC_FAST
      if (ctr->padlen == 0 && len >= (unsigned long)ctr->blocklen) {
         for (x = 0; x < ctr->blocklen; x += sizeof(LTC_FAST_TYPE)) {
            *((LTC_FAST_TYPE*)((unsigned char *)ct + x)) = *((LTC_FAST_TYPE*)((unsigned char *)pt + x)) ^
                                                           *((LTC_FAST_TYPE*)((unsigned char *)ctr->pad + x));
         }
       pt         += ctr->blocklen;
       ct         += ctr->blocklen;
       len        -= ctr->blocklen;
       ctr->padlen = ctr->blocklen;
       continue;
      }
#endif
      *ct++ = *pt++ ^ ctr->pad[ctr->padlen++];
      --len;
   }
   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/modes/ctr/ctr_encrypt.c,v $ */
/* $Revision: 1.22 $ */
/* $Date: 2007/02/22 20:26:05 $ */

/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/**
   @file ctr_done.c
   CTR implementation, finish chain, Tom St Denis
*/

#ifdef LTC_CTR_MODE

/** Terminate the chain
  @param ctr    The CTR chain to terminate
  @return CRYPT_OK on success
*/
int ctr_done(symmetric_CTR *ctr)
{
   int err;
   LTC_ARGCHK(ctr != NULL);

   if ((err = cipher_is_valid(ctr->cipher)) != CRYPT_OK) {
      return err;
   }
   cipher_descriptor[ctr->cipher].done(&ctr->key);
   return CRYPT_OK;
}



#endif

/* $Source: /cvs/libtom/libtomcrypt/src/modes/ctr/ctr_done.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */

/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/**
  @file ctr_decrypt.c
  CTR implementation, decrypt data, Tom St Denis
*/

#ifdef LTC_CTR_MODE

/**
   CTR decrypt
   @param ct      Ciphertext
   @param pt      [out] Plaintext
   @param len     Length of ciphertext (octets)
   @param ctr     CTR state
   @return CRYPT_OK if successful
*/
int ctr_decrypt(const unsigned char *ct, unsigned char *pt, unsigned long len, symmetric_CTR *ctr)
{
   LTC_ARGCHK(pt != NULL);
   LTC_ARGCHK(ct != NULL);
   LTC_ARGCHK(ctr != NULL);

   return ctr_encrypt(ct, pt, len, ctr);
}

#endif


/* $Source: /cvs/libtom/libtomcrypt/src/modes/ctr/ctr_decrypt.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:24 $ */

/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/**
   @file ctr_start.c
   CTR implementation, start chain, Tom St Denis
*/


#ifdef LTC_CTR_MODE

/**
   Initialize a CTR context
   @param cipher      The index of the cipher desired
   @param IV          The initial vector
   @param key         The secret key
   @param keylen      The length of the secret key (octets)
   @param num_rounds  Number of rounds in the cipher desired (0 for default)
   @param ctr_mode    The counter mode (CTR_COUNTER_LITTLE_ENDIAN or CTR_COUNTER_BIG_ENDIAN)
   @param ctr         The CTR state to initialize
   @return CRYPT_OK if successful
*/
int ctr_start(               int   cipher,
              const unsigned char *IV,
              const unsigned char *key,       int keylen,
                             int  num_rounds, int ctr_mode,
                   symmetric_CTR *ctr)
{
   int x, err;

   LTC_ARGCHK(IV  != NULL);
   LTC_ARGCHK(key != NULL);
   LTC_ARGCHK(ctr != NULL);

   /* bad param? */
   if ((err = cipher_is_valid(cipher)) != CRYPT_OK) {
      return err;
   }

   /* ctrlen == counter width */
   ctr->ctrlen   = (ctr_mode & 255) ? (ctr_mode & 255) : cipher_descriptor[cipher].block_length;
   if (ctr->ctrlen > cipher_descriptor[cipher].block_length) {
      return CRYPT_INVALID_ARG;
   }

   if ((ctr_mode & 0x1000) == CTR_COUNTER_BIG_ENDIAN) {
      ctr->ctrlen = cipher_descriptor[cipher].block_length - ctr->ctrlen;
   }

   /* setup cipher */
   if ((err = cipher_descriptor[cipher].setup(key, keylen, num_rounds, &ctr->key)) != CRYPT_OK) {
      return err;
   }

   /* copy ctr */
   ctr->blocklen = cipher_descriptor[cipher].block_length;
   ctr->cipher   = cipher;
   ctr->padlen   = 0;
   ctr->mode     = ctr_mode & 0x1000;
   for (x = 0; x < ctr->blocklen; x++) {
       ctr->ctr[x] = IV[x];
   }

   if (ctr_mode & LTC_CTR_RFC3686) {
      /* increment the IV as per RFC 3686 */
      if (ctr->mode == CTR_COUNTER_LITTLE_ENDIAN) {
         /* little-endian */
         for (x = 0; x < ctr->ctrlen; x++) {
             ctr->ctr[x] = (ctr->ctr[x] + (unsigned char)1) & (unsigned char)255;
             if (ctr->ctr[x] != (unsigned char)0) {
                break;
             }
         }
      } else {
         /* big-endian */
         for (x = ctr->blocklen-1; x >= ctr->ctrlen; x--) {
             ctr->ctr[x] = (ctr->ctr[x] + (unsigned char)1) & (unsigned char)255;
             if (ctr->ctr[x] != (unsigned char)0) {
                break;
             }
         }
      }
   }

   return cipher_descriptor[ctr->cipher].ecb_encrypt(ctr->ctr, ctr->pad, &ctr->key);
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/modes/ctr/ctr_start.c,v $ */
/* $Revision: 1.15 $ */
/* $Date: 2007/02/23 14:18:37 $ */

/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/**
  @file ctr_setiv.c
  CTR implementation, set IV, Tom St Denis
*/

#ifdef LTC_CTR_MODE

/**
   Set an initial vector
   @param IV   The initial vector
   @param len  The length of the vector (in octets)
   @param ctr  The CTR state
   @return CRYPT_OK if successful
*/
int ctr_setiv(const unsigned char *IV, unsigned long len, symmetric_CTR *ctr)
{
   int err;

   LTC_ARGCHK(IV  != NULL);
   LTC_ARGCHK(ctr != NULL);

   /* bad param? */
   if ((err = cipher_is_valid(ctr->cipher)) != CRYPT_OK) {
      return err;
   }

   if (len != (unsigned long)ctr->blocklen) {
      return CRYPT_INVALID_ARG;
   }

   /* set IV */
   XMEMCPY(ctr->ctr, IV, len);

   /* force next block */
   ctr->padlen = 0;
   return cipher_descriptor[ctr->cipher].ecb_encrypt(IV, ctr->pad, &ctr->key);
}

#endif


/* $Source: /cvs/libtom/libtomcrypt/src/modes/ctr/ctr_setiv.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */

/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/**
   @file ctr_getiv.c
   CTR implementation, get IV, Tom St Denis
*/

#ifdef LTC_CTR_MODE

/**
   Get the current initial vector
   @param IV   [out] The destination of the initial vector
   @param len  [in/out]  The max size and resulting size of the initial vector
   @param ctr  The CTR state
   @return CRYPT_OK if successful
*/
int ctr_getiv(unsigned char *IV, unsigned long *len, symmetric_CTR *ctr)
{
   LTC_ARGCHK(IV  != NULL);
   LTC_ARGCHK(len != NULL);
   LTC_ARGCHK(ctr != NULL);
   if ((unsigned long)ctr->blocklen > *len) {
      *len = ctr->blocklen;
      return CRYPT_BUFFER_OVERFLOW;
   }
   XMEMCPY(IV, ctr->ctr, ctr->blocklen);
   *len = ctr->blocklen;

   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/modes/ctr/ctr_getiv.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */
