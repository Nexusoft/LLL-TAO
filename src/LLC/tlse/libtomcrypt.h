/* $Source$ */
/* $Revision$ */
/* $Date$ */


#include "libtommath.h"


#ifndef TOMCRYPT_H_
#define TOMCRYPT_H_
#define USE_LTM
#define LTM_DESC
#define LTC_SHA1
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

/* use configuration data */
#ifndef TOMCRYPT_CUSTOM_H_
#define TOMCRYPT_CUSTOM_H_

/* macros for various libc functions you can change for embedded targets */
#ifndef XMALLOC
 #ifdef malloc
  #define LTC_NO_PROTOTYPES
 #endif
 #define XMALLOC    malloc
#endif
#ifndef XREALLOC
 #ifdef realloc
  #define LTC_NO_PROTOTYPES
 #endif
 #define XREALLOC    realloc
#endif
#ifndef XCALLOC
 #ifdef calloc
  #define LTC_NO_PROTOTYPES
 #endif
 #define XCALLOC    calloc
#endif
#ifndef XFREE
 #ifdef free
  #define LTC_NO_PROTOTYPES
 #endif
 #define XFREE    free
#endif

#ifndef XMEMSET
 #ifdef memset
  #define LTC_NO_PROTOTYPES
 #endif
 #define XMEMSET    memset
#endif
#ifndef XMEMCPY
 #ifdef memcpy
  #define LTC_NO_PROTOTYPES
 #endif
 #define XMEMCPY    memcpy
#endif
#ifndef XMEMCMP
 #ifdef memcmp
  #define LTC_NO_PROTOTYPES
 #endif
 #define XMEMCMP    memcmp
#endif
#ifndef XSTRCMP
 #ifdef strcmp
  #define LTC_NO_PROTOTYPES
 #endif
 #define XSTRCMP    strcmp
#endif

#ifndef XCLOCK
 #define XCLOCK             clock
#endif
#ifndef XCLOCKS_PER_SEC
 #define XCLOCKS_PER_SEC    CLOCKS_PER_SEC
#endif

#ifndef XQSORT
 #ifdef qsort
  #define LTC_NO_PROTOTYPES
 #endif
 #define XQSORT    qsort
#endif

/* Easy button? */
#ifdef LTC_EASY
 #define LTC_NO_CIPHERS
 #define LTC_RIJNDAEL
 #define LTC_BLOWFISH
 #define LTC_DES
 #define LTC_CAST5

 #define LTC_NO_MODES
 #define LTC_ECB_MODE
 #define LTC_CBC_MODE
 #define LTC_CTR_MODE

 #define LTC_NO_HASHES
 #define LTC_SHA1
 #define LTC_SHA512
 #define LTC_SHA384
 #define LTC_SHA256
 #define LTC_SHA224

 #define LTC_NO_MACS
 #define LTC_HMAC
 #define LTC_OMAC
 #define LTC_CCM_MODE

 #define LTC_NO_PRNGS
 #define LTC_SPRNG
 #define LTC_YARROW
 #define LTC_DEVRANDOM
 #define TRY_URANDOM_FIRST

 #define LTC_NO_PK
 #define LTC_MRSA
 #define LTC_MECC
#endif

/* Use small code where possible */
/* #define LTC_SMALL_CODE */

/* Enable self-test test vector checking */
#ifndef LTC_NO_TEST
 #define LTC_TEST
#endif

/* clean the stack of functions which put private information on stack */
/* #define LTC_CLEAN_STACK */

/* disable all file related functions */
/* #define LTC_NO_FILE */

/* disable all forms of ASM */
/* #define LTC_NO_ASM */

/* disable FAST mode */
/* #define LTC_NO_FAST */

/* disable BSWAP on x86 */
/* #define LTC_NO_BSWAP */

/* ---> Symmetric Block Ciphers <--- */
#ifndef LTC_NO_CIPHERS

 #define LTC_BLOWFISH
 #define LTC_RC2
 #define LTC_RC5
 #define LTC_RC6
 #define LTC_SAFERP
 #define LTC_RIJNDAEL
 #define LTC_XTEA

/* _TABLES tells it to use tables during setup, _SMALL means to use the smaller scheduled key format
 * (saves 4KB of ram), _ALL_TABLES enables all tables during setup */
 #define LTC_TWOFISH
 #ifndef LTC_NO_TABLES
  #define LTC_TWOFISH_TABLES
/* #define LTC_TWOFISH_ALL_TABLES */
 #else
  #define LTC_TWOFISH_SMALL
 #endif
/* #define LTC_TWOFISH_SMALL */
/* LTC_DES includes EDE triple-LTC_DES */
 #define LTC_DES
 #define LTC_CAST5
 #define LTC_NOEKEON
 #define LTC_SKIPJACK
 #define LTC_SAFER
 #define LTC_KHAZAD
 #define LTC_ANUBIS
 #define LTC_ANUBIS_TWEAK
 #define LTC_KSEED
 #define LTC_KASUMI
#endif /* LTC_NO_CIPHERS */


/* ---> Block Cipher Modes of Operation <--- */
#ifndef LTC_NO_MODES

 #define LTC_CFB_MODE
 #define LTC_OFB_MODE
 #define LTC_ECB_MODE
 #define LTC_CBC_MODE
 #define LTC_CTR_MODE

/* F8 chaining mode */
 #define LTC_F8_MODE

/* LRW mode */
 #define LTC_LRW_MODE
 #ifndef LTC_NO_TABLES

/* like GCM mode this will enable 16 8x128 tables [64KB] that make
 * seeking very fast.
 */
  #define LRW_TABLES
 #endif

/* XTS mode */
 #define LTC_XTS_MODE
#endif /* LTC_NO_MODES */

/* ---> One-Way Hash Functions <--- */
#ifndef LTC_NO_HASHES

 #define LTC_CHC_HASH
 #define LTC_WHIRLPOOL
 #define LTC_SHA512
 #define LTC_SHA384
 #define LTC_SHA256
 #define LTC_SHA224
 #define LTC_TIGER
 #define LTC_SHA1
 #define LTC_MD5
 #define LTC_MD4
 #define LTC_MD2
 #define LTC_RIPEMD128
 #define LTC_RIPEMD160
 #define LTC_RIPEMD256
 #define LTC_RIPEMD320
#endif /* LTC_NO_HASHES */

/* ---> MAC functions <--- */
#ifndef LTC_NO_MACS

 #define LTC_HMAC
 #define LTC_OMAC
 #define LTC_PMAC
 #define LTC_XCBC
 #define LTC_F9_MODE
 #define LTC_PELICAN

 #if defined(LTC_PELICAN) && !defined(LTC_RIJNDAEL)
  #error Pelican-MAC requires LTC_RIJNDAEL
 #endif

/* ---> Encrypt + Authenticate Modes <--- */

 #define LTC_EAX_MODE
 #if defined(LTC_EAX_MODE) && !(defined(LTC_CTR_MODE) && defined(LTC_OMAC))
  #error LTC_EAX_MODE requires CTR and LTC_OMAC mode
 #endif

 #define LTC_OCB_MODE
 #define LTC_CCM_MODE
 #define LTC_GCM_MODE

/* Use 64KiB tables */
 #ifndef LTC_NO_TABLES
  #define LTC_GCM_TABLES
 #endif

/* USE SSE2? requires GCC works on x86_32 and x86_64*/
 #ifdef LTC_GCM_TABLES
/* #define LTC_GCM_TABLES_SSE2 */
 #endif
#endif /* LTC_NO_MACS */

/* Various tidbits of modern neatoness */
#define LTC_BASE64

/* --> Pseudo Random Number Generators <--- */
#ifndef LTC_NO_PRNGS

/* Yarrow */
 #define LTC_YARROW
/* which descriptor of AES to use?  */
/* 0 = rijndael_enc 1 = aes_enc, 2 = rijndael [full], 3 = aes [full] */
 #define LTC_YARROW_AES    0

 #if defined(LTC_YARROW) && !defined(LTC_CTR_MODE)
  #error LTC_YARROW requires LTC_CTR_MODE chaining mode to be defined!
 #endif

/* a PRNG that simply reads from an available system source */
 #define LTC_SPRNG

/* The LTC_RC4 stream cipher */
 #define LTC_RC4

/* Fortuna PRNG */
 #define LTC_FORTUNA
/* reseed every N calls to the read function */
 #define LTC_FORTUNA_WD       10
/* number of pools (4..32) can save a bit of ram by lowering the count */
 #define LTC_FORTUNA_POOLS    32

/* Greg's LTC_SOBER128 PRNG ;-0 */
 #define LTC_SOBER128

/* the *nix style /dev/random device */
 #define LTC_DEVRANDOM
/* try /dev/urandom before trying /dev/random */
 #define TRY_URANDOM_FIRST
#endif /* LTC_NO_PRNGS */

/* ---> math provider? <--- */
#ifndef LTC_NO_MATH

/* LibTomMath */
/* #define LTM_LTC_DESC */

/* TomsFastMath */
/* #define TFM_LTC_DESC */
#endif /* LTC_NO_MATH */

/* ---> Public Key Crypto <--- */
#ifndef LTC_NO_PK

/* Include RSA support */
 #define LTC_MRSA

/* Include Katja (a Rabin variant like RSA) */
/* #define MKAT */

/* Digital Signature Algorithm */
 #define LTC_MDSA

/* ECC */
 #define LTC_MECC

/* use Shamir's trick for point mul (speeds up signature verification) */
 #define LTC_ECC_SHAMIR

 #if defined(TFM_LTC_DESC) && defined(LTC_MECC)
  #define LTC_MECC_ACCEL
 #endif

/* do we want fixed point ECC */
/* #define LTC_MECC_FP */

/* Timing Resistant? */
/* #define LTC_ECC_TIMING_RESISTANT */
#endif /* LTC_NO_PK */

/* LTC_PKCS #1 (RSA) and #5 (Password Handling) stuff */
#ifndef LTC_NO_PKCS

 #define LTC_PKCS_1
 #define LTC_PKCS_5

/* Include ASN.1 DER (required by DSA/RSA) */
 #define LTC_DER
#endif /* LTC_NO_PKCS */

/* cleanup */

#ifdef LTC_MECC
/* Supported ECC Key Sizes */
 #ifndef LTC_NO_CURVES
  #define ECC112
  #define ECC128
  #define ECC160
  #define ECC192
  #define ECC224
  #define ECC256
  #define ECC384
  #define ECC521
 #endif
#endif

#if defined(LTC_MECC) || defined(LTC_MRSA) || defined(LTC_MDSA) || defined(MKATJA)
/* Include the MPI functionality?  (required by the PK algorithms) */
 #define MPI
#endif

#ifdef LTC_MRSA
 #define LTC_PKCS_1
#endif

#if defined(LTC_DER) && !defined(MPI)
 #error ASN.1 DER requires MPI functionality
#endif

#if (defined(LTC_MDSA) || defined(LTC_MRSA) || defined(LTC_MECC) || defined(MKATJA)) && !defined(LTC_DER)
 #error PK requires ASN.1 DER functionality, make sure LTC_DER is enabled
#endif

/* THREAD management */
#ifdef LTC_PTHREAD

 #include <pthread.h>

 #define LTC_MUTEX_GLOBAL(x)    pthread_mutex_t x = PTHREAD_MUTEX_INITIALIZER;
 #define LTC_MUTEX_PROTO(x)     extern pthread_mutex_t x;
 #define LTC_MUTEX_TYPE(x)      pthread_mutex_t x;
 #define LTC_MUTEX_INIT(x)      pthread_mutex_init(x, NULL);
 #define LTC_MUTEX_LOCK(x)      pthread_mutex_lock(x);
 #define LTC_MUTEX_UNLOCK(x)    pthread_mutex_unlock(x);

#else

/* default no functions */
 #define LTC_MUTEX_GLOBAL(x)
 #define LTC_MUTEX_PROTO(x)
 #define LTC_MUTEX_TYPE(x)
 #define LTC_MUTEX_INIT(x)
 #define LTC_MUTEX_LOCK(x)
 #define LTC_MUTEX_UNLOCK(x)
#endif

/* Debuggers */

/* define this if you use Valgrind, note: it CHANGES the way SOBER-128 and LTC_RC4 work (see the code) */
/* #define LTC_VALGRIND */
#endif



/* $Source: /cvs/libtom/libtomcrypt/src/headers/tomcrypt_custom.h,v $ */
/* $Revision: 1.73 $ */
/* $Date: 2007/05/12 14:37:41 $ */

#ifdef __cplusplus
extern "C" {
#endif

/* version */
#define CRYPT           0x0117
#define SCRYPT          "1.17"

/* max size of either a cipher/hash block or symmetric key [largest of the two] */
#define MAXBLOCKSIZE    128

/* descriptor table size */

/* error codes [will be expanded in future releases] */
enum {
    CRYPT_OK=0,               /* Result OK */
    CRYPT_ERROR,              /* Generic Error */
    CRYPT_NOP,                /* Not a failure but no operation was performed */

    CRYPT_INVALID_KEYSIZE,    /* Invalid key size given */
    CRYPT_INVALID_ROUNDS,     /* Invalid number of rounds */
    CRYPT_FAIL_TESTVECTOR,    /* Algorithm failed test vectors */

    CRYPT_BUFFER_OVERFLOW,    /* Not enough space for output */
    CRYPT_INVALID_PACKET,     /* Invalid input packet given */

    CRYPT_INVALID_PRNGSIZE,   /* Invalid number of bits for a PRNG */
    CRYPT_ERROR_READPRNG,     /* Could not read enough from PRNG */

    CRYPT_INVALID_CIPHER,     /* Invalid cipher specified */
    CRYPT_INVALID_HASH,       /* Invalid hash specified */
    CRYPT_INVALID_PRNG,       /* Invalid PRNG specified */

    CRYPT_MEM,                /* Out of memory */

    CRYPT_PK_TYPE_MISMATCH,   /* Not equivalent types of PK keys */
    CRYPT_PK_NOT_PRIVATE,     /* Requires a private PK key */

    CRYPT_INVALID_ARG,        /* Generic invalid argument */
    CRYPT_FILE_NOTFOUND,      /* File Not Found */

    CRYPT_PK_INVALID_TYPE,    /* Invalid type of PK key */
    CRYPT_PK_INVALID_SYSTEM,  /* Invalid PK system specified */
    CRYPT_PK_DUP,             /* Duplicate key already in key ring */
    CRYPT_PK_NOT_FOUND,       /* Key not found in keyring */
    CRYPT_PK_INVALID_SIZE,    /* Invalid size input for PK parameters */

    CRYPT_INVALID_PRIME_SIZE, /* Invalid size of prime requested */
    CRYPT_PK_INVALID_PADDING  /* Invalid padding on input */
};

/* This is the build config file.
 *
 * With this you can setup what to inlcude/exclude automatically during any build.  Just comment
 * out the line that #define's the word for the thing you want to remove.  phew!
 */

#ifndef TOMCRYPT_CFG_H
#define TOMCRYPT_CFG_H

#if defined(_WIN32) || defined(_MSC_VER)
 #define LTC_CALL    __cdecl
#else
 #ifndef LTC_CALL
  #define LTC_CALL
 #endif
#endif

#ifndef LTC_EXPORT
 #define LTC_EXPORT
#endif

/* certain platforms use macros for these, making the prototypes broken */
#ifndef LTC_NO_PROTOTYPES

/* you can change how memory allocation works ... */
LTC_EXPORT void *LTC_CALL XMALLOC(size_t n);
LTC_EXPORT void *LTC_CALL XREALLOC(void *p, size_t n);
LTC_EXPORT void *LTC_CALL XCALLOC(size_t n, size_t s);
LTC_EXPORT void LTC_CALL XFREE(void *p);

LTC_EXPORT void LTC_CALL XQSORT(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));


/* change the clock function too */
LTC_EXPORT clock_t LTC_CALL XCLOCK(void);

/* various other functions */
LTC_EXPORT void *LTC_CALL XMEMCPY(void *dest, const void *src, size_t n);
LTC_EXPORT int LTC_CALL XMEMCMP(const void *s1, const void *s2, size_t n);
LTC_EXPORT void *LTC_CALL XMEMSET(void *s, int c, size_t n);

LTC_EXPORT int LTC_CALL XSTRCMP(const char *s1, const char *s2);
#endif

/* type of argument checking, 0=default, 1=fatal and 2=error+continue, 3=nothing */
#ifndef ARGTYPE
 #define ARGTYPE    0
#endif

/* Controls endianess and size of registers.  Leave uncommented to get platform neutral [slower] code
 *
 * Note: in order to use the optimized macros your platform must support unaligned 32 and 64 bit read/writes.
 * The x86 platforms allow this but some others [ARM for instance] do not.  On those platforms you **MUST**
 * use the portable [slower] macros.
 */

/* detect x86-32 machines somewhat */
#if !defined(__STRICT_ANSI__) && (defined(INTEL_CC) || (defined(_MSC_VER) && defined(WIN32)) || (defined(__GNUC__) && (defined(__DJGPP__) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__i386__))))
 #define ENDIAN_LITTLE
 #define ENDIAN_32BITWORD
 #define LTC_FAST
 #define LTC_FAST_TYPE    unsigned long
#endif

/* detects MIPS R5900 processors (PS2) */
#if (defined(__R5900) || defined(R5900) || defined(__R5900__)) && (defined(_mips) || defined(__mips__) || defined(mips))
 #define ENDIAN_LITTLE
 #define ENDIAN_64BITWORD
#endif

/* detect amd64 */
#if !defined(__STRICT_ANSI__) && defined(__x86_64__)
 #define ENDIAN_LITTLE
 #define ENDIAN_64BITWORD
 #define LTC_FAST
 #define LTC_FAST_TYPE    unsigned long
#endif

/* detect PPC32 */
#if !defined(__STRICT_ANSI__) && defined(LTC_PPC32)
 #define ENDIAN_BIG
 #define ENDIAN_32BITWORD
 #define LTC_FAST
 #define LTC_FAST_TYPE    unsigned long
#endif

/* detect sparc and sparc64 */
#if defined(__sparc__)
 #define ENDIAN_BIG
 #if defined(__arch64__)
  #define ENDIAN_64BITWORD
 #else
  #define ENDIAN_32BITWORD
 #endif
#endif


#ifdef LTC_NO_FAST
 #ifdef LTC_FAST
  #undef LTC_FAST
 #endif
#endif

/* No asm is a quick way to disable anything "not portable" */
#ifdef LTC_NO_ASM
 #undef ENDIAN_LITTLE
 #undef ENDIAN_BIG
 #undef ENDIAN_32BITWORD
 #undef ENDIAN_64BITWORD
 #undef LTC_FAST
 #undef LTC_FAST_TYPE
 #define LTC_NO_ROLC
 #define LTC_NO_BSWAP
#endif

/* #define ENDIAN_LITTLE */
/* #define ENDIAN_BIG */

/* #define ENDIAN_32BITWORD */
/* #define ENDIAN_64BITWORD */

#if (defined(ENDIAN_BIG) || defined(ENDIAN_LITTLE)) && !(defined(ENDIAN_32BITWORD) || defined(ENDIAN_64BITWORD))
 #error You must specify a word size as well as endianess in tomcrypt_cfg.h
#endif

#if !(defined(ENDIAN_BIG) || defined(ENDIAN_LITTLE))
 #define ENDIAN_NEUTRAL
#endif
#endif


/* $Source: /cvs/libtom/libtomcrypt/src/headers/tomcrypt_cfg.h,v $ */
/* $Revision: 1.19 $ */
/* $Date: 2006/12/04 02:19:48 $ */

/* fix for MSVC ...evil! */
#ifdef _MSC_VER
 #define CONST64(n)    n ## ui64
typedef unsigned __int64     ulong64;
#else
 #define CONST64(n)    n ## ULL
typedef unsigned long long   ulong64;
#endif

/* this is the "32-bit at least" data type
 * Re-define it to suit your platform but it must be at least 32-bits
 */
#if defined(__x86_64__) || (defined(__sparc__) && defined(__arch64__))
typedef unsigned             ulong32;
#else
typedef unsigned long        ulong32;
#endif

/* ---- HELPER MACROS ---- */
#ifdef ENDIAN_NEUTRAL

 #define STORE32L(x, y)                                                                         \
    { (y)[3] = (unsigned char)(((x) >> 24) & 255); (y)[2] = (unsigned char)(((x) >> 16) & 255); \
      (y)[1] = (unsigned char)(((x) >> 8) & 255); (y)[0] = (unsigned char)((x) & 255); }

 #define LOAD32L(x, y)                            \
    { x = ((unsigned long)((y)[3] & 255) << 24) | \
          ((unsigned long)((y)[2] & 255) << 16) | \
          ((unsigned long)((y)[1] & 255) << 8) |  \
          ((unsigned long)((y)[0] & 255)); }

 #define STORE64L(x, y)                                                                         \
    { (y)[7] = (unsigned char)(((x) >> 56) & 255); (y)[6] = (unsigned char)(((x) >> 48) & 255); \
      (y)[5] = (unsigned char)(((x) >> 40) & 255); (y)[4] = (unsigned char)(((x) >> 32) & 255); \
      (y)[3] = (unsigned char)(((x) >> 24) & 255); (y)[2] = (unsigned char)(((x) >> 16) & 255); \
      (y)[1] = (unsigned char)(((x) >> 8) & 255); (y)[0] = (unsigned char)((x) & 255); }

 #define LOAD64L(x, y)                                                            \
    { x = (((ulong64)((y)[7] & 255)) << 56) | (((ulong64)((y)[6] & 255)) << 48) | \
          (((ulong64)((y)[5] & 255)) << 40) | (((ulong64)((y)[4] & 255)) << 32) | \
          (((ulong64)((y)[3] & 255)) << 24) | (((ulong64)((y)[2] & 255)) << 16) | \
          (((ulong64)((y)[1] & 255)) << 8) | (((ulong64)((y)[0] & 255))); }

 #define STORE32H(x, y)                                                                         \
    { (y)[0] = (unsigned char)(((x) >> 24) & 255); (y)[1] = (unsigned char)(((x) >> 16) & 255); \
      (y)[2] = (unsigned char)(((x) >> 8) & 255); (y)[3] = (unsigned char)((x) & 255); }

 #define LOAD32H(x, y)                            \
    { x = ((unsigned long)((y)[0] & 255) << 24) | \
          ((unsigned long)((y)[1] & 255) << 16) | \
          ((unsigned long)((y)[2] & 255) << 8) |  \
          ((unsigned long)((y)[3] & 255)); }

 #define STORE64H(x, y)                                                                         \
    { (y)[0] = (unsigned char)(((x) >> 56) & 255); (y)[1] = (unsigned char)(((x) >> 48) & 255); \
      (y)[2] = (unsigned char)(((x) >> 40) & 255); (y)[3] = (unsigned char)(((x) >> 32) & 255); \
      (y)[4] = (unsigned char)(((x) >> 24) & 255); (y)[5] = (unsigned char)(((x) >> 16) & 255); \
      (y)[6] = (unsigned char)(((x) >> 8) & 255); (y)[7] = (unsigned char)((x) & 255); }

 #define LOAD64H(x, y)                                                            \
    { x = (((ulong64)((y)[0] & 255)) << 56) | (((ulong64)((y)[1] & 255)) << 48) | \
          (((ulong64)((y)[2] & 255)) << 40) | (((ulong64)((y)[3] & 255)) << 32) | \
          (((ulong64)((y)[4] & 255)) << 24) | (((ulong64)((y)[5] & 255)) << 16) | \
          (((ulong64)((y)[6] & 255)) << 8) | (((ulong64)((y)[7] & 255))); }
#endif /* ENDIAN_NEUTRAL */

#ifdef ENDIAN_LITTLE

 #if !defined(LTC_NO_BSWAP) && (defined(INTEL_CC) || (defined(__GNUC__) && (defined(__DJGPP__) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__i386__) || defined(__x86_64__))))

  #define STORE32H(x, y)     \
    asm __volatile__ (       \
        "bswapl %0     \n\t" \
        "movl   %0,(%1)\n\t" \
        "bswapl %0     \n\t" \
           ::"r" (x), "r" (y));

  #define LOAD32H(x, y)    \
    asm __volatile__ (     \
        "movl (%1),%0\n\t" \
        "bswapl %0\n\t"    \
        : "=r" (x) : "r" (y));

 #else

  #define STORE32H(x, y)                                                                        \
    { (y)[0] = (unsigned char)(((x) >> 24) & 255); (y)[1] = (unsigned char)(((x) >> 16) & 255); \
      (y)[2] = (unsigned char)(((x) >> 8) & 255); (y)[3] = (unsigned char)((x) & 255); }

  #define LOAD32H(x, y)                           \
    { x = ((unsigned long)((y)[0] & 255) << 24) | \
          ((unsigned long)((y)[1] & 255) << 16) | \
          ((unsigned long)((y)[2] & 255) << 8) |  \
          ((unsigned long)((y)[3] & 255)); }
 #endif


/* x86_64 processor */
 #if !defined(LTC_NO_BSWAP) && (defined(__GNUC__) && defined(__x86_64__))

  #define STORE64H(x, y)     \
    asm __volatile__ (       \
        "bswapq %0     \n\t" \
        "movq   %0,(%1)\n\t" \
        "bswapq %0     \n\t" \
           ::"r" (x), "r" (y));

  #define LOAD64H(x, y)    \
    asm __volatile__ (     \
        "movq (%1),%0\n\t" \
        "bswapq %0\n\t"    \
        : "=r" (x) : "r" (y));

 #else

  #define STORE64H(x, y)                                                                        \
    { (y)[0] = (unsigned char)(((x) >> 56) & 255); (y)[1] = (unsigned char)(((x) >> 48) & 255); \
      (y)[2] = (unsigned char)(((x) >> 40) & 255); (y)[3] = (unsigned char)(((x) >> 32) & 255); \
      (y)[4] = (unsigned char)(((x) >> 24) & 255); (y)[5] = (unsigned char)(((x) >> 16) & 255); \
      (y)[6] = (unsigned char)(((x) >> 8) & 255); (y)[7] = (unsigned char)((x) & 255); }

  #define LOAD64H(x, y)                                                           \
    { x = (((ulong64)((y)[0] & 255)) << 56) | (((ulong64)((y)[1] & 255)) << 48) | \
          (((ulong64)((y)[2] & 255)) << 40) | (((ulong64)((y)[3] & 255)) << 32) | \
          (((ulong64)((y)[4] & 255)) << 24) | (((ulong64)((y)[5] & 255)) << 16) | \
          (((ulong64)((y)[6] & 255)) << 8) | (((ulong64)((y)[7] & 255))); }
 #endif

 #ifdef ENDIAN_32BITWORD

  #define STORE32L(x, y) \
    { ulong32 __t = (x); XMEMCPY(y, &__t, 4); }

  #define LOAD32L(x, y) \
    XMEMCPY(&(x), y, 4);

  #define STORE64L(x, y)                                                                        \
    { (y)[7] = (unsigned char)(((x) >> 56) & 255); (y)[6] = (unsigned char)(((x) >> 48) & 255); \
      (y)[5] = (unsigned char)(((x) >> 40) & 255); (y)[4] = (unsigned char)(((x) >> 32) & 255); \
      (y)[3] = (unsigned char)(((x) >> 24) & 255); (y)[2] = (unsigned char)(((x) >> 16) & 255); \
      (y)[1] = (unsigned char)(((x) >> 8) & 255); (y)[0] = (unsigned char)((x) & 255); }

  #define LOAD64L(x, y)                                                           \
    { x = (((ulong64)((y)[7] & 255)) << 56) | (((ulong64)((y)[6] & 255)) << 48) | \
          (((ulong64)((y)[5] & 255)) << 40) | (((ulong64)((y)[4] & 255)) << 32) | \
          (((ulong64)((y)[3] & 255)) << 24) | (((ulong64)((y)[2] & 255)) << 16) | \
          (((ulong64)((y)[1] & 255)) << 8) | (((ulong64)((y)[0] & 255))); }

 #else /* 64-bit words then  */

  #define STORE32L(x, y) \
    { ulong32 __t = (x); XMEMCPY(y, &__t, 4); }

  #define LOAD32L(x, y) \
    { XMEMCPY(&(x), y, 4); x &= 0xFFFFFFFF; }

  #define STORE64L(x, y) \
    { ulong64 __t = (x); XMEMCPY(y, &__t, 8); }

  #define LOAD64L(x, y) \
    { XMEMCPY(&(x), y, 8); }
 #endif /* ENDIAN_64BITWORD */
#endif  /* ENDIAN_LITTLE */

#ifdef ENDIAN_BIG
 #define STORE32L(x, y)                                                                         \
    { (y)[3] = (unsigned char)(((x) >> 24) & 255); (y)[2] = (unsigned char)(((x) >> 16) & 255); \
      (y)[1] = (unsigned char)(((x) >> 8) & 255); (y)[0] = (unsigned char)((x) & 255); }

 #define LOAD32L(x, y)                            \
    { x = ((unsigned long)((y)[3] & 255) << 24) | \
          ((unsigned long)((y)[2] & 255) << 16) | \
          ((unsigned long)((y)[1] & 255) << 8) |  \
          ((unsigned long)((y)[0] & 255)); }

 #define STORE64L(x, y)                                                                         \
    { (y)[7] = (unsigned char)(((x) >> 56) & 255); (y)[6] = (unsigned char)(((x) >> 48) & 255); \
      (y)[5] = (unsigned char)(((x) >> 40) & 255); (y)[4] = (unsigned char)(((x) >> 32) & 255); \
      (y)[3] = (unsigned char)(((x) >> 24) & 255); (y)[2] = (unsigned char)(((x) >> 16) & 255); \
      (y)[1] = (unsigned char)(((x) >> 8) & 255); (y)[0] = (unsigned char)((x) & 255); }

 #define LOAD64L(x, y)                                                            \
    { x = (((ulong64)((y)[7] & 255)) << 56) | (((ulong64)((y)[6] & 255)) << 48) | \
          (((ulong64)((y)[5] & 255)) << 40) | (((ulong64)((y)[4] & 255)) << 32) | \
          (((ulong64)((y)[3] & 255)) << 24) | (((ulong64)((y)[2] & 255)) << 16) | \
          (((ulong64)((y)[1] & 255)) << 8) | (((ulong64)((y)[0] & 255))); }

 #ifdef ENDIAN_32BITWORD

  #define STORE32H(x, y) \
    { ulong32 __t = (x); XMEMCPY(y, &__t, 4); }

  #define LOAD32H(x, y) \
    XMEMCPY(&(x), y, 4);

  #define STORE64H(x, y)                                                                        \
    { (y)[0] = (unsigned char)(((x) >> 56) & 255); (y)[1] = (unsigned char)(((x) >> 48) & 255); \
      (y)[2] = (unsigned char)(((x) >> 40) & 255); (y)[3] = (unsigned char)(((x) >> 32) & 255); \
      (y)[4] = (unsigned char)(((x) >> 24) & 255); (y)[5] = (unsigned char)(((x) >> 16) & 255); \
      (y)[6] = (unsigned char)(((x) >> 8) & 255);  (y)[7] = (unsigned char)((x) & 255); }

  #define LOAD64H(x, y)                                                           \
    { x = (((ulong64)((y)[0] & 255)) << 56) | (((ulong64)((y)[1] & 255)) << 48) | \
          (((ulong64)((y)[2] & 255)) << 40) | (((ulong64)((y)[3] & 255)) << 32) | \
          (((ulong64)((y)[4] & 255)) << 24) | (((ulong64)((y)[5] & 255)) << 16) | \
          (((ulong64)((y)[6] & 255)) << 8) | (((ulong64)((y)[7] & 255))); }

 #else /* 64-bit words then  */

  #define STORE32H(x, y) \
    { ulong32 __t = (x); XMEMCPY(y, &__t, 4); }

  #define LOAD32H(x, y) \
    { XMEMCPY(&(x), y, 4); x &= 0xFFFFFFFF; }

  #define STORE64H(x, y) \
    { ulong64 __t = (x); XMEMCPY(y, &__t, 8); }

  #define LOAD64H(x, y) \
    { XMEMCPY(&(x), y, 8); }
 #endif /* ENDIAN_64BITWORD */
#endif  /* ENDIAN_BIG */

#define BSWAP(x)                                               \
    (((x >> 24) & 0x000000FFUL) | ((x << 24) & 0xFF000000UL) | \
     ((x >> 8) & 0x0000FF00UL) | ((x << 8) & 0x00FF0000UL))


/* 32-bit Rotates */
#if defined(_MSC_VER)

/* instrinsic rotate */
 #include <stdlib.h>
 #pragma intrinsic(_lrotr,_lrotl)
 #define ROR(x, n)     _lrotr(x, n)
 #define ROL(x, n)     _lrotl(x, n)
 #define RORc(x, n)    _lrotr(x, n)
 #define ROLc(x, n)    _lrotl(x, n)

#elif !defined(__STRICT_ANSI__) && defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)) && !defined(INTEL_CC) && !defined(LTC_NO_ASM)

static inline unsigned ROL(unsigned word, int i) {
    asm ("roll %%cl,%0"
         : "=r" (word)
         : "0" (word), "c" (i));
    return word;
}

static inline unsigned ROR(unsigned word, int i) {
    asm ("rorl %%cl,%0"
         : "=r" (word)
         : "0" (word), "c" (i));
    return word;
}

 #ifndef LTC_NO_ROLC

static inline unsigned ROLc(unsigned word, const int i) {
    asm ("roll %2,%0"
         : "=r" (word)
         : "0" (word), "I" (i));
    return word;
}

static inline unsigned RORc(unsigned word, const int i) {
    asm ("rorl %2,%0"
         : "=r" (word)
         : "0" (word), "I" (i));
    return word;
}

 #else

  #define ROLc    ROL
  #define RORc    ROR
 #endif

#elif !defined(__STRICT_ANSI__) && defined(LTC_PPC32)

static inline unsigned ROL(unsigned word, int i) {
    asm ("rotlw %0,%0,%2"
         : "=r" (word)
         : "0" (word), "r" (i));
    return word;
}

static inline unsigned ROR(unsigned word, int i) {
    asm ("rotlw %0,%0,%2"
         : "=r" (word)
         : "0" (word), "r" (32 - i));
    return word;
}

 #ifndef LTC_NO_ROLC

static inline unsigned ROLc(unsigned word, const int i) {
    asm ("rotlwi %0,%0,%2"
         : "=r" (word)
         : "0" (word), "I" (i));
    return word;
}

static inline unsigned RORc(unsigned word, const int i) {
    asm ("rotrwi %0,%0,%2"
         : "=r" (word)
         : "0" (word), "I" (i));
    return word;
}

 #else

  #define ROLc    ROL
  #define RORc    ROR
 #endif


#else

/* rotates the hard way */
 #define ROL(x, y)     ((((unsigned long)(x) << (unsigned long)((y) & 31)) | (((unsigned long)(x) & 0xFFFFFFFFUL) >> (unsigned long)(32 - ((y) & 31)))) & 0xFFFFFFFFUL)
 #define ROR(x, y)     (((((unsigned long)(x) & 0xFFFFFFFFUL) >> (unsigned long)((y) & 31)) | ((unsigned long)(x) << (unsigned long)(32 - ((y) & 31)))) & 0xFFFFFFFFUL)
 #define ROLc(x, y)    ((((unsigned long)(x) << (unsigned long)((y) & 31)) | (((unsigned long)(x) & 0xFFFFFFFFUL) >> (unsigned long)(32 - ((y) & 31)))) & 0xFFFFFFFFUL)
 #define RORc(x, y)    (((((unsigned long)(x) & 0xFFFFFFFFUL) >> (unsigned long)((y) & 31)) | ((unsigned long)(x) << (unsigned long)(32 - ((y) & 31)))) & 0xFFFFFFFFUL)
#endif


/* 64-bit Rotates */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__) && defined(__x86_64__) && !defined(LTC_NO_ASM)

static inline unsigned long ROL64(unsigned long word, int i) {
    asm ("rolq %%cl,%0"
         : "=r" (word)
         : "0" (word), "c" (i));
    return word;
}

static inline unsigned long ROR64(unsigned long word, int i) {
    asm ("rorq %%cl,%0"
         : "=r" (word)
         : "0" (word), "c" (i));
    return word;
}

 #ifndef LTC_NO_ROLC

static inline unsigned long ROL64c(unsigned long word, const int i) {
    asm ("rolq %2,%0"
         : "=r" (word)
         : "0" (word), "J" (i));
    return word;
}

static inline unsigned long ROR64c(unsigned long word, const int i) {
    asm ("rorq %2,%0"
         : "=r" (word)
         : "0" (word), "J" (i));
    return word;
}

 #else /* LTC_NO_ROLC */

  #define ROL64c    ROL64
  #define ROR64c    ROR64
 #endif

#else /* Not x86_64  */

 #define ROL64(x, y)                 \
    ((((x) << ((ulong64)(y) & 63)) | \
      (((x) & CONST64(0xFFFFFFFFFFFFFFFF)) >> ((ulong64)64 - ((y) & 63)))) & CONST64(0xFFFFFFFFFFFFFFFF))

 #define ROR64(x, y)                                                          \
    (((((x) & CONST64(0xFFFFFFFFFFFFFFFF)) >> ((ulong64)(y) & CONST64(63))) | \
      ((x) << ((ulong64)(64 - ((y) & CONST64(63)))))) & CONST64(0xFFFFFFFFFFFFFFFF))

 #define ROL64c(x, y)                \
    ((((x) << ((ulong64)(y) & 63)) | \
      (((x) & CONST64(0xFFFFFFFFFFFFFFFF)) >> ((ulong64)64 - ((y) & 63)))) & CONST64(0xFFFFFFFFFFFFFFFF))

 #define ROR64c(x, y)                                                         \
    (((((x) & CONST64(0xFFFFFFFFFFFFFFFF)) >> ((ulong64)(y) & CONST64(63))) | \
      ((x) << ((ulong64)(64 - ((y) & CONST64(63)))))) & CONST64(0xFFFFFFFFFFFFFFFF))
#endif

#ifndef MAX
 #define MAX(x, y)    (((x) > (y)) ? (x) : (y))
#endif

#ifndef MIN
 #define MIN(x, y)    (((x) < (y)) ? (x) : (y))
#endif

/* extract a byte portably */
#ifdef _MSC_VER
 #define byte(x, n)    ((unsigned char)((x) >> (8 * (n))))
#else
 #define byte(x, n)    (((x) >> (8 * (n))) & 255)
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/headers/tomcrypt_macros.h,v $ */
/* $Revision: 1.15 $ */
/* $Date: 2006/11/29 23:43:57 $ */

/* ---- SYMMETRIC KEY STUFF -----
 *
 * We put each of the ciphers scheduled keys in their own structs then we put all of
 * the key formats in one union.  This makes the function prototypes easier to use.
 */
#ifdef LTC_BLOWFISH
struct blowfish_key {
    ulong32 S[4][256];
    ulong32 K[18];
};
#endif

#ifdef LTC_RC5
struct rc5_key {
    int     rounds;
    ulong32 K[50];
};
#endif

#ifdef LTC_RC6
struct rc6_key {
    ulong32 K[44];
};
#endif

#ifdef LTC_SAFERP
struct saferp_key {
    unsigned char K[33][16];
    long          rounds;
};
#endif

#ifdef LTC_RIJNDAEL
struct rijndael_key {
    ulong32 eK[60], dK[60];
    int     Nr;
};
#endif

#ifdef LTC_KSEED
struct kseed_key {
    ulong32 K[32], dK[32];
};
#endif

#ifdef LTC_KASUMI
struct kasumi_key {
    ulong32 KLi1[8], KLi2[8],
            KOi1[8], KOi2[8], KOi3[8],
            KIi1[8], KIi2[8], KIi3[8];
};
#endif

#ifdef LTC_XTEA
struct xtea_key {
    unsigned long A[32], B[32];
};
#endif

#ifdef LTC_TWOFISH
 #ifndef LTC_TWOFISH_SMALL
struct twofish_key {
    ulong32 S[4][256], K[40];
};
 #else
struct twofish_key {
    ulong32       K[40];
    unsigned char S[32], start;
};
 #endif
#endif

#ifdef LTC_SAFER
 #define LTC_SAFER_K64_DEFAULT_NOF_ROUNDS      6
 #define LTC_SAFER_K128_DEFAULT_NOF_ROUNDS     10
 #define LTC_SAFER_SK64_DEFAULT_NOF_ROUNDS     8
 #define LTC_SAFER_SK128_DEFAULT_NOF_ROUNDS    10
 #define LTC_SAFER_MAX_NOF_ROUNDS              13
 #define LTC_SAFER_BLOCK_LEN                   8
 #define LTC_SAFER_KEY_LEN                     (1 + LTC_SAFER_BLOCK_LEN * (1 + 2 * LTC_SAFER_MAX_NOF_ROUNDS))
typedef unsigned char   safer_block_t[LTC_SAFER_BLOCK_LEN];
typedef unsigned char   safer_key_t[LTC_SAFER_KEY_LEN];
struct safer_key {
    safer_key_t key;
};
#endif

#ifdef LTC_RC2
struct rc2_key {
    unsigned xkey[64];
};
#endif

#ifdef LTC_DES
struct des_key {
    ulong32 ek[32], dk[32];
};

struct des3_key {
    ulong32 ek[3][32], dk[3][32];
};
#endif

#ifdef LTC_CAST5
struct cast5_key {
    ulong32 K[32], keylen;
};
#endif

#ifdef LTC_NOEKEON
struct noekeon_key {
    ulong32 K[4], dK[4];
};
#endif

#ifdef LTC_SKIPJACK
struct skipjack_key {
    unsigned char key[10];
};
#endif

#ifdef LTC_KHAZAD
struct khazad_key {
    ulong64 roundKeyEnc[8 + 1];
    ulong64 roundKeyDec[8 + 1];
};
#endif

#ifdef LTC_ANUBIS
struct anubis_key {
    int     keyBits;
    int     R;
    ulong32 roundKeyEnc[18 + 1][4];
    ulong32 roundKeyDec[18 + 1][4];
};
#endif

#ifdef LTC_MULTI2
struct multi2_key {
    int     N;
    ulong32 uk[8];
};
#endif

typedef union Symmetric_key {
#ifdef LTC_DES
    struct des_key      des;
    struct des3_key     des3;
#endif
#ifdef LTC_RC2
    struct rc2_key      rc2;
#endif
#ifdef LTC_SAFER
    struct safer_key    safer;
#endif
#ifdef LTC_TWOFISH
    struct twofish_key  twofish;
#endif
#ifdef LTC_BLOWFISH
    struct blowfish_key blowfish;
#endif
#ifdef LTC_RC5
    struct rc5_key      rc5;
#endif
#ifdef LTC_RC6
    struct rc6_key      rc6;
#endif
#ifdef LTC_SAFERP
    struct saferp_key   saferp;
#endif
#ifdef LTC_RIJNDAEL
    struct rijndael_key rijndael;
#endif
#ifdef LTC_XTEA
    struct xtea_key     xtea;
#endif
#ifdef LTC_CAST5
    struct cast5_key    cast5;
#endif
#ifdef LTC_NOEKEON
    struct noekeon_key  noekeon;
#endif
#ifdef LTC_SKIPJACK
    struct skipjack_key skipjack;
#endif
#ifdef LTC_KHAZAD
    struct khazad_key   khazad;
#endif
#ifdef LTC_ANUBIS
    struct anubis_key   anubis;
#endif
#ifdef LTC_KSEED
    struct kseed_key    kseed;
#endif
#ifdef LTC_KASUMI
    struct kasumi_key   kasumi;
#endif
#ifdef LTC_MULTI2
    struct multi2_key   multi2;
#endif
    void                *data;
} symmetric_key;

#ifdef LTC_ECB_MODE
/** A block cipher ECB structure */
typedef struct {
    /** The index of the cipher chosen */
    int           cipher,
    /** The block size of the given cipher */
                  blocklen;
    /** The scheduled key */
    symmetric_key key;
} symmetric_ECB;
#endif

#ifdef LTC_CFB_MODE
/** A block cipher CFB structure */
typedef struct {
    /** The index of the cipher chosen */
    int           cipher,
    /** The block size of the given cipher */
                  blocklen,
    /** The padding offset */
                  padlen;
    /** The current IV */
    unsigned char IV[MAXBLOCKSIZE],
    /** The pad used to encrypt/decrypt */
                  pad[MAXBLOCKSIZE];
    /** The scheduled key */
    symmetric_key key;
} symmetric_CFB;
#endif

#ifdef LTC_OFB_MODE
/** A block cipher OFB structure */
typedef struct {
    /** The index of the cipher chosen */
    int           cipher,
    /** The block size of the given cipher */
                  blocklen,
    /** The padding offset */
                  padlen;
    /** The current IV */
    unsigned char IV[MAXBLOCKSIZE];
    /** The scheduled key */
    symmetric_key key;
} symmetric_OFB;
#endif

#ifdef LTC_CBC_MODE
/** A block cipher CBC structure */
typedef struct {
    /** The index of the cipher chosen */
    int           cipher,
    /** The block size of the given cipher */
                  blocklen;
    /** The current IV */
    unsigned char IV[MAXBLOCKSIZE];
    /** The scheduled key */
    symmetric_key key;
} symmetric_CBC;
#endif


#ifdef LTC_CTR_MODE
/** A block cipher CTR structure */
typedef struct {
    /** The index of the cipher chosen */
    int           cipher,
    /** The block size of the given cipher */
                  blocklen,
    /** The padding offset */
                  padlen,
    /** The mode (endianess) of the CTR, 0==little, 1==big */
                  mode,
    /** counter width */
                  ctrlen;

    /** The counter */
    unsigned char ctr[MAXBLOCKSIZE],
    /** The pad used to encrypt/decrypt */
                  pad[MAXBLOCKSIZE];
    /** The scheduled key */
    symmetric_key key;
} symmetric_CTR;
#endif


#ifdef LTC_LRW_MODE
/** A LRW structure */
typedef struct {
    /** The index of the cipher chosen (must be a 128-bit block cipher) */
    int           cipher;

    /** The current IV */
    unsigned char IV[16],

    /** the tweak key */
                  tweak[16],

    /** The current pad, it's the product of the first 15 bytes against the tweak key */
                  pad[16];

    /** The scheduled symmetric key */
    symmetric_key key;

 #ifdef LRW_TABLES
    /** The pre-computed multiplication table */
    unsigned char PC[16][256][16];
 #endif
} symmetric_LRW;
#endif

#ifdef LTC_F8_MODE
/** A block cipher F8 structure */
typedef struct {
    /** The index of the cipher chosen */
    int           cipher,
    /** The block size of the given cipher */
                  blocklen,
    /** The padding offset */
                  padlen;
    /** The current IV */
    unsigned char IV[MAXBLOCKSIZE],
                  MIV[MAXBLOCKSIZE];
    /** Current block count */
    ulong32       blockcnt;
    /** The scheduled key */
    symmetric_key key;
} symmetric_F8;
#endif


/** cipher descriptor table, last entry has "name == NULL" to mark the end of table */
extern struct ltc_cipher_descriptor {
    /** name of cipher */
    char          *name;
    /** internal ID */
    unsigned char ID;
    /** min keysize (octets) */
    int           min_key_length,
    /** max keysize (octets) */
                  max_key_length,
    /** block size (octets) */
                  block_length,
    /** default number of rounds */
                  default_rounds;

    /** Setup the cipher
       @param key         The input symmetric key
       @param keylen      The length of the input key (octets)
       @param num_rounds  The requested number of rounds (0==default)
       @param skey        [out] The destination of the scheduled key
       @return CRYPT_OK if successful
     */
    int           (*setup)(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);

    /** Encrypt a block
       @param pt      The plaintext
       @param ct      [out] The ciphertext
       @param skey    The scheduled key
       @return CRYPT_OK if successful
     */
    int           (*ecb_encrypt)(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);

    /** Decrypt a block
       @param ct      The ciphertext
       @param pt      [out] The plaintext
       @param skey    The scheduled key
       @return CRYPT_OK if successful
     */
    int           (*ecb_decrypt)(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);

    /** Test the block cipher
        @return CRYPT_OK if successful, CRYPT_NOP if self-testing has been disabled
     */
    int           (*test)(void);

    /** Terminate the context
       @param skey    The scheduled key
     */
    void          (*done)(symmetric_key *skey);

    /** Determine a key size
        @param keysize    [in/out] The size of the key desired and the suggested size
        @return CRYPT_OK if successful
     */
    int           (*keysize)(int *keysize);

/** Accelerators **/

    /** Accelerated ECB encryption
        @param pt      Plaintext
        @param ct      Ciphertext
        @param blocks  The number of complete blocks to process
        @param skey    The scheduled key context
        @return CRYPT_OK if successful
     */
    int           (*accel_ecb_encrypt)(const unsigned char *pt, unsigned char *ct, unsigned long blocks, symmetric_key *skey);

    /** Accelerated ECB decryption
        @param pt      Plaintext
        @param ct      Ciphertext
        @param blocks  The number of complete blocks to process
        @param skey    The scheduled key context
        @return CRYPT_OK if successful
     */
    int           (*accel_ecb_decrypt)(const unsigned char *ct, unsigned char *pt, unsigned long blocks, symmetric_key *skey);

    /** Accelerated CBC encryption
        @param pt      Plaintext
        @param ct      Ciphertext
        @param blocks  The number of complete blocks to process
        @param IV      The initial value (input/output)
        @param skey    The scheduled key context
        @return CRYPT_OK if successful
     */
    int           (*accel_cbc_encrypt)(const unsigned char *pt, unsigned char *ct, unsigned long blocks, unsigned char *IV, symmetric_key *skey);

    /** Accelerated CBC decryption
        @param pt      Plaintext
        @param ct      Ciphertext
        @param blocks  The number of complete blocks to process
        @param IV      The initial value (input/output)
        @param skey    The scheduled key context
        @return CRYPT_OK if successful
     */
    int           (*accel_cbc_decrypt)(const unsigned char *ct, unsigned char *pt, unsigned long blocks, unsigned char *IV, symmetric_key *skey);

    /** Accelerated CTR encryption
        @param pt      Plaintext
        @param ct      Ciphertext
        @param blocks  The number of complete blocks to process
        @param IV      The initial value (input/output)
        @param mode    little or big endian counter (mode=0 or mode=1)
        @param skey    The scheduled key context
        @return CRYPT_OK if successful
     */
    int           (*accel_ctr_encrypt)(const unsigned char *pt, unsigned char *ct, unsigned long blocks, unsigned char *IV, int mode, symmetric_key *skey);

    /** Accelerated LRW
        @param pt      Plaintext
        @param ct      Ciphertext
        @param blocks  The number of complete blocks to process
        @param IV      The initial value (input/output)
        @param tweak   The LRW tweak
        @param skey    The scheduled key context
        @return CRYPT_OK if successful
     */
    int           (*accel_lrw_encrypt)(const unsigned char *pt, unsigned char *ct, unsigned long blocks, unsigned char *IV, const unsigned char *tweak, symmetric_key *skey);

    /** Accelerated LRW
        @param ct      Ciphertext
        @param pt      Plaintext
        @param blocks  The number of complete blocks to process
        @param IV      The initial value (input/output)
        @param tweak   The LRW tweak
        @param skey    The scheduled key context
        @return CRYPT_OK if successful
     */
    int           (*accel_lrw_decrypt)(const unsigned char *ct, unsigned char *pt, unsigned long blocks, unsigned char *IV, const unsigned char *tweak, symmetric_key *skey);

    /** Accelerated CCM packet (one-shot)
        @param key        The secret key to use
        @param keylen     The length of the secret key (octets)
        @param uskey      A previously scheduled key [optional can be NULL]
        @param nonce      The session nonce [use once]
        @param noncelen   The length of the nonce
        @param header     The header for the session
        @param headerlen  The length of the header (octets)
        @param pt         [out] The plaintext
        @param ptlen      The length of the plaintext (octets)
        @param ct         [out] The ciphertext
        @param tag        [out] The destination tag
        @param taglen     [in/out] The max size and resulting size of the authentication tag
        @param direction  Encrypt or Decrypt direction (0 or 1)
        @return CRYPT_OK if successful
     */
    int           (*accel_ccm_memory)(
        const unsigned char *key, unsigned long keylen,
        symmetric_key *uskey,
        const unsigned char *nonce, unsigned long noncelen,
        const unsigned char *header, unsigned long headerlen,
        unsigned char *pt, unsigned long ptlen,
        unsigned char *ct,
        unsigned char *tag, unsigned long *taglen,
        int direction);

    /** Accelerated GCM packet (one shot)
        @param key        The secret key
        @param keylen     The length of the secret key
        @param IV         The initial vector
        @param IVlen      The length of the initial vector
        @param adata      The additional authentication data (header)
        @param adatalen   The length of the adata
        @param pt         The plaintext
        @param ptlen      The length of the plaintext (ciphertext length is the same)
        @param ct         The ciphertext
        @param tag        [out] The MAC tag
        @param taglen     [in/out] The MAC tag length
        @param direction  Encrypt or Decrypt mode (GCM_ENCRYPT or GCM_DECRYPT)
        @return CRYPT_OK on success
     */
    int           (*accel_gcm_memory)(
        const unsigned char *key, unsigned long keylen,
        const unsigned char *IV, unsigned long IVlen,
        const unsigned char *adata, unsigned long adatalen,
        unsigned char *pt, unsigned long ptlen,
        unsigned char *ct,
        unsigned char *tag, unsigned long *taglen,
        int direction);

    /** Accelerated one shot LTC_OMAC
        @param key            The secret key
        @param keylen         The key length (octets)
        @param in             The message
        @param inlen          Length of message (octets)
        @param out            [out] Destination for tag
        @param outlen         [in/out] Initial and final size of out
        @return CRYPT_OK on success
     */
    int           (*omac_memory)(
        const unsigned char *key, unsigned long keylen,
        const unsigned char *in, unsigned long inlen,
        unsigned char *out, unsigned long *outlen);

    /** Accelerated one shot XCBC
        @param key            The secret key
        @param keylen         The key length (octets)
        @param in             The message
        @param inlen          Length of message (octets)
        @param out            [out] Destination for tag
        @param outlen         [in/out] Initial and final size of out
        @return CRYPT_OK on success
     */
    int           (*xcbc_memory)(
        const unsigned char *key, unsigned long keylen,
        const unsigned char *in, unsigned long inlen,
        unsigned char *out, unsigned long *outlen);

    /** Accelerated one shot F9
        @param key            The secret key
        @param keylen         The key length (octets)
        @param in             The message
        @param inlen          Length of message (octets)
        @param out            [out] Destination for tag
        @param outlen         [in/out] Initial and final size of out
        @return CRYPT_OK on success
        @remark Requires manual padding
     */
    int           (*f9_memory)(
        const unsigned char *key, unsigned long keylen,
        const unsigned char *in, unsigned long inlen,
        unsigned char *out, unsigned long *outlen);
} cipher_descriptor[];

#ifdef LTC_BLOWFISH
int blowfish_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int blowfish_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int blowfish_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int blowfish_test(void);
void blowfish_done(symmetric_key *skey);
int blowfish_keysize(int *keysize);

extern const struct ltc_cipher_descriptor blowfish_desc;
#endif

#ifdef LTC_RC5
int rc5_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int rc5_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int rc5_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int rc5_test(void);
void rc5_done(symmetric_key *skey);
int rc5_keysize(int *keysize);

extern const struct ltc_cipher_descriptor rc5_desc;
#endif

#ifdef LTC_RC6
int rc6_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int rc6_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int rc6_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int rc6_test(void);
void rc6_done(symmetric_key *skey);
int rc6_keysize(int *keysize);

extern const struct ltc_cipher_descriptor rc6_desc;
#endif

#ifdef LTC_RC2
int rc2_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int rc2_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int rc2_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int rc2_test(void);
void rc2_done(symmetric_key *skey);
int rc2_keysize(int *keysize);

extern const struct ltc_cipher_descriptor rc2_desc;
#endif

#ifdef LTC_SAFERP
int saferp_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int saferp_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int saferp_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int saferp_test(void);
void saferp_done(symmetric_key *skey);
int saferp_keysize(int *keysize);

extern const struct ltc_cipher_descriptor saferp_desc;
#endif

#ifdef LTC_SAFER
int safer_k64_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int safer_sk64_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int safer_k128_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int safer_sk128_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int safer_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *key);
int safer_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *key);
int safer_k64_test(void);
int safer_sk64_test(void);
int safer_sk128_test(void);
void safer_done(symmetric_key *skey);
int safer_64_keysize(int *keysize);
int safer_128_keysize(int *keysize);

extern const struct ltc_cipher_descriptor safer_k64_desc, safer_k128_desc, safer_sk64_desc, safer_sk128_desc;
#endif

#ifdef LTC_RIJNDAEL

/* make aes an alias */
 #define aes_setup              rijndael_setup
 #define aes_ecb_encrypt        rijndael_ecb_encrypt
 #define aes_ecb_decrypt        rijndael_ecb_decrypt
 #define aes_test               rijndael_test
 #define aes_done               rijndael_done
 #define aes_keysize            rijndael_keysize

 #define aes_enc_setup          rijndael_enc_setup
 #define aes_enc_ecb_encrypt    rijndael_enc_ecb_encrypt
 #define aes_enc_keysize        rijndael_enc_keysize

int rijndael_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int rijndael_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int rijndael_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int rijndael_test(void);
void rijndael_done(symmetric_key *skey);
int rijndael_keysize(int *keysize);
int rijndael_enc_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int rijndael_enc_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
void rijndael_enc_done(symmetric_key *skey);
int rijndael_enc_keysize(int *keysize);

extern const struct ltc_cipher_descriptor rijndael_desc, aes_desc;
extern const struct ltc_cipher_descriptor rijndael_enc_desc, aes_enc_desc;
#endif

#ifdef LTC_XTEA
int xtea_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int xtea_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int xtea_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int xtea_test(void);
void xtea_done(symmetric_key *skey);
int xtea_keysize(int *keysize);

extern const struct ltc_cipher_descriptor xtea_desc;
#endif

#ifdef LTC_TWOFISH
int twofish_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int twofish_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int twofish_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int twofish_test(void);
void twofish_done(symmetric_key *skey);
int twofish_keysize(int *keysize);

extern const struct ltc_cipher_descriptor twofish_desc;
#endif

#ifdef LTC_DES
int des_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int des_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int des_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int des_test(void);
void des_done(symmetric_key *skey);
int des_keysize(int *keysize);
int des3_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int des3_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int des3_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int des3_test(void);
void des3_done(symmetric_key *skey);
int des3_keysize(int *keysize);

extern const struct ltc_cipher_descriptor des_desc, des3_desc;
#endif

#ifdef LTC_CAST5
int cast5_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int cast5_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int cast5_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int cast5_test(void);
void cast5_done(symmetric_key *skey);
int cast5_keysize(int *keysize);

extern const struct ltc_cipher_descriptor cast5_desc;
#endif

#ifdef LTC_NOEKEON
int noekeon_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int noekeon_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int noekeon_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int noekeon_test(void);
void noekeon_done(symmetric_key *skey);
int noekeon_keysize(int *keysize);

extern const struct ltc_cipher_descriptor noekeon_desc;
#endif

#ifdef LTC_SKIPJACK
int skipjack_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int skipjack_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int skipjack_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int skipjack_test(void);
void skipjack_done(symmetric_key *skey);
int skipjack_keysize(int *keysize);

extern const struct ltc_cipher_descriptor skipjack_desc;
#endif

#ifdef LTC_KHAZAD
int khazad_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int khazad_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int khazad_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int khazad_test(void);
void khazad_done(symmetric_key *skey);
int khazad_keysize(int *keysize);

extern const struct ltc_cipher_descriptor khazad_desc;
#endif

#ifdef LTC_ANUBIS
int anubis_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int anubis_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int anubis_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int anubis_test(void);
void anubis_done(symmetric_key *skey);
int anubis_keysize(int *keysize);

extern const struct ltc_cipher_descriptor anubis_desc;
#endif

#ifdef LTC_KSEED
int kseed_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int kseed_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int kseed_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int kseed_test(void);
void kseed_done(symmetric_key *skey);
int kseed_keysize(int *keysize);

extern const struct ltc_cipher_descriptor kseed_desc;
#endif

#ifdef LTC_KASUMI
int kasumi_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int kasumi_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int kasumi_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int kasumi_test(void);
void kasumi_done(symmetric_key *skey);
int kasumi_keysize(int *keysize);

extern const struct ltc_cipher_descriptor kasumi_desc;
#endif


#ifdef LTC_MULTI2
int multi2_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey);
int multi2_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey);
int multi2_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey);
int multi2_test(void);
void multi2_done(symmetric_key *skey);
int multi2_keysize(int *keysize);

extern const struct ltc_cipher_descriptor multi2_desc;
#endif

#ifdef LTC_ECB_MODE
int ecb_start(int cipher, const unsigned char *key,
              int keylen, int num_rounds, symmetric_ECB *ecb);
int ecb_encrypt(const unsigned char *pt, unsigned char *ct, unsigned long len, symmetric_ECB *ecb);
int ecb_decrypt(const unsigned char *ct, unsigned char *pt, unsigned long len, symmetric_ECB *ecb);
int ecb_done(symmetric_ECB *ecb);
#endif

#ifdef LTC_CFB_MODE
int cfb_start(int cipher, const unsigned char *IV, const unsigned char *key,
              int keylen, int num_rounds, symmetric_CFB *cfb);
int cfb_encrypt(const unsigned char *pt, unsigned char *ct, unsigned long len, symmetric_CFB *cfb);
int cfb_decrypt(const unsigned char *ct, unsigned char *pt, unsigned long len, symmetric_CFB *cfb);
int cfb_getiv(unsigned char *IV, unsigned long *len, symmetric_CFB *cfb);
int cfb_setiv(const unsigned char *IV, unsigned long len, symmetric_CFB *cfb);
int cfb_done(symmetric_CFB *cfb);
#endif

#ifdef LTC_OFB_MODE
int ofb_start(int cipher, const unsigned char *IV, const unsigned char *key,
              int keylen, int num_rounds, symmetric_OFB *ofb);
int ofb_encrypt(const unsigned char *pt, unsigned char *ct, unsigned long len, symmetric_OFB *ofb);
int ofb_decrypt(const unsigned char *ct, unsigned char *pt, unsigned long len, symmetric_OFB *ofb);
int ofb_getiv(unsigned char *IV, unsigned long *len, symmetric_OFB *ofb);
int ofb_setiv(const unsigned char *IV, unsigned long len, symmetric_OFB *ofb);
int ofb_done(symmetric_OFB *ofb);
#endif

#ifdef LTC_CBC_MODE
int cbc_start(int cipher, const unsigned char *IV, const unsigned char *key,
              int keylen, int num_rounds, symmetric_CBC *cbc);
int cbc_encrypt(const unsigned char *pt, unsigned char *ct, unsigned long len, symmetric_CBC *cbc);
int cbc_decrypt(const unsigned char *ct, unsigned char *pt, unsigned long len, symmetric_CBC *cbc);
int cbc_getiv(unsigned char *IV, unsigned long *len, symmetric_CBC *cbc);
int cbc_setiv(const unsigned char *IV, unsigned long len, symmetric_CBC *cbc);
int cbc_done(symmetric_CBC *cbc);
#endif

#ifdef LTC_CTR_MODE

 #define CTR_COUNTER_LITTLE_ENDIAN    0x0000
 #define CTR_COUNTER_BIG_ENDIAN       0x1000
 #define LTC_CTR_RFC3686              0x2000

int ctr_start(int cipher,
              const unsigned char *IV,
              const unsigned char *key, int keylen,
              int num_rounds, int ctr_mode,
              symmetric_CTR *ctr);
int ctr_encrypt(const unsigned char *pt, unsigned char *ct, unsigned long len, symmetric_CTR *ctr);
int ctr_decrypt(const unsigned char *ct, unsigned char *pt, unsigned long len, symmetric_CTR *ctr);
int ctr_getiv(unsigned char *IV, unsigned long *len, symmetric_CTR *ctr);
int ctr_setiv(const unsigned char *IV, unsigned long len, symmetric_CTR *ctr);
int ctr_done(symmetric_CTR *ctr);
int ctr_test(void);
#endif

#ifdef LTC_LRW_MODE

 #define LRW_ENCRYPT    0
 #define LRW_DECRYPT    1

int lrw_start(int cipher,
              const unsigned char *IV,
              const unsigned char *key, int keylen,
              const unsigned char *tweak,
              int num_rounds,
              symmetric_LRW *lrw);
int lrw_encrypt(const unsigned char *pt, unsigned char *ct, unsigned long len, symmetric_LRW *lrw);
int lrw_decrypt(const unsigned char *ct, unsigned char *pt, unsigned long len, symmetric_LRW *lrw);
int lrw_getiv(unsigned char *IV, unsigned long *len, symmetric_LRW *lrw);
int lrw_setiv(const unsigned char *IV, unsigned long len, symmetric_LRW *lrw);
int lrw_done(symmetric_LRW *lrw);
int lrw_test(void);

/* don't call */
int lrw_process(const unsigned char *pt, unsigned char *ct, unsigned long len, int mode, symmetric_LRW *lrw);
#endif

#ifdef LTC_F8_MODE
int f8_start(int cipher, const unsigned char *IV,
             const unsigned char *key, int keylen,
             const unsigned char *salt_key, int skeylen,
             int num_rounds, symmetric_F8 *f8);
int f8_encrypt(const unsigned char *pt, unsigned char *ct, unsigned long len, symmetric_F8 *f8);
int f8_decrypt(const unsigned char *ct, unsigned char *pt, unsigned long len, symmetric_F8 *f8);
int f8_getiv(unsigned char *IV, unsigned long *len, symmetric_F8 *f8);
int f8_setiv(const unsigned char *IV, unsigned long len, symmetric_F8 *f8);
int f8_done(symmetric_F8 *f8);
int f8_test_mode(void);
#endif

#ifdef LTC_XTS_MODE
typedef struct {
    symmetric_key key1, key2;
    int           cipher;
} symmetric_xts;

int xts_start(int                 cipher,
              const unsigned char *key1,
              const unsigned char *key2,
              unsigned long       keylen,
              int                 num_rounds,
              symmetric_xts       *xts);

int xts_encrypt(
    const unsigned char *pt, unsigned long ptlen,
    unsigned char *ct,
    const unsigned char *tweak,
    symmetric_xts *xts);
int xts_decrypt(
    const unsigned char *ct, unsigned long ptlen,
    unsigned char *pt,
    const unsigned char *tweak,
    symmetric_xts *xts);

void xts_done(symmetric_xts *xts);
int  xts_test(void);
void xts_mult_x(unsigned char *I);
#endif

int find_cipher(const char *name);
int find_cipher_any(const char *name, int blocklen, int keylen);
int find_cipher_id(unsigned char ID);
int register_cipher(const struct ltc_cipher_descriptor *cipher);
int unregister_cipher(const struct ltc_cipher_descriptor *cipher);
int cipher_is_valid(int idx);

LTC_MUTEX_PROTO(ltc_cipher_mutex)

/* $Source: /cvs/libtom/libtomcrypt/src/headers/tomcrypt_cipher.h,v $ */
/* $Revision: 1.54 $ */
/* $Date: 2007/05/12 14:37:41 $ */

#define LTC_SHA1
/* ---- HASH FUNCTIONS ---- */
#ifdef LTC_SHA512
struct sha512_state {
    ulong64       length, state[8];
    unsigned long curlen;
    unsigned char buf[128];
};
#endif

#ifdef LTC_SHA256
struct sha256_state {
    ulong64       length;
    ulong32       state[8], curlen;
    unsigned char buf[64];
};
#endif

#ifdef LTC_SHA1
struct sha1_state {
    ulong64       length;
    ulong32       state[5], curlen;
    unsigned char buf[64];
};
#endif

#ifdef LTC_MD5
struct md5_state {
    ulong64       length;
    ulong32       state[4], curlen;
    unsigned char buf[64];
};
#endif

#ifdef LTC_MD4
struct md4_state {
    ulong64       length;
    ulong32       state[4], curlen;
    unsigned char buf[64];
};
#endif

#ifdef LTC_TIGER
struct tiger_state {
    ulong64       state[3], length;
    unsigned long curlen;
    unsigned char buf[64];
};
#endif

#ifdef LTC_MD2
struct md2_state {
    unsigned char chksum[16], X[48], buf[16];
    unsigned long curlen;
};
#endif

#ifdef LTC_RIPEMD128
struct rmd128_state {
    ulong64       length;
    unsigned char buf[64];
    ulong32       curlen, state[4];
};
#endif

#ifdef LTC_RIPEMD160
struct rmd160_state {
    ulong64       length;
    unsigned char buf[64];
    ulong32       curlen, state[5];
};
#endif

#ifdef LTC_RIPEMD256
struct rmd256_state {
    ulong64       length;
    unsigned char buf[64];
    ulong32       curlen, state[8];
};
#endif

#ifdef LTC_RIPEMD320
struct rmd320_state {
    ulong64       length;
    unsigned char buf[64];
    ulong32       curlen, state[10];
};
#endif

#ifdef LTC_WHIRLPOOL
struct whirlpool_state {
    ulong64       length, state[8];
    unsigned char buf[64];
    ulong32       curlen;
};
#endif

#ifdef LTC_CHC_HASH
struct chc_state {
    ulong64       length;
    unsigned char state[MAXBLOCKSIZE], buf[MAXBLOCKSIZE];
    ulong32       curlen;
};
#endif

typedef union Hash_state {
    char                   dummy[1];
#ifdef LTC_CHC_HASH
    struct chc_state       chc;
#endif
#ifdef LTC_WHIRLPOOL
    struct whirlpool_state whirlpool;
#endif
#ifdef LTC_SHA512
    struct sha512_state    sha512;
#endif
#ifdef LTC_SHA256
    struct sha256_state    sha256;
#endif
#ifdef LTC_SHA1
    struct sha1_state      sha1;
#endif
#ifdef LTC_MD5
    struct md5_state       md5;
#endif
#ifdef LTC_MD4
    struct md4_state       md4;
#endif
#ifdef LTC_MD2
    struct md2_state       md2;
#endif
#ifdef LTC_TIGER
    struct tiger_state     tiger;
#endif
#ifdef LTC_RIPEMD128
    struct rmd128_state    rmd128;
#endif
#ifdef LTC_RIPEMD160
    struct rmd160_state    rmd160;
#endif
#ifdef LTC_RIPEMD256
    struct rmd256_state    rmd256;
#endif
#ifdef LTC_RIPEMD320
    struct rmd320_state    rmd320;
#endif
    void                   *data;
} hash_state;

/** hash descriptor */
extern struct ltc_hash_descriptor {
    /** name of hash */
    char          *name;
    /** internal ID */
    unsigned char ID;
    /** Size of digest in octets */
    unsigned long hashsize;
    /** Input block size in octets */
    unsigned long blocksize;
    /** ASN.1 OID */
    unsigned long OID[16];
    /** Length of DER encoding */
    unsigned long OIDlen;

    /** Init a hash state
       @param hash   The hash to initialize
       @return CRYPT_OK if successful
     */
    int           (*init)(hash_state *hash);

    /** Process a block of data
       @param hash   The hash state
       @param in     The data to hash
       @param inlen  The length of the data (octets)
       @return CRYPT_OK if successful
     */
    int           (*process)(hash_state *hash, const unsigned char *in, unsigned long inlen);

    /** Produce the digest and store it
       @param hash   The hash state
       @param out    [out] The destination of the digest
       @return CRYPT_OK if successful
     */
    int           (*done)(hash_state *hash, unsigned char *out);

    /** Self-test
       @return CRYPT_OK if successful, CRYPT_NOP if self-tests have been disabled
     */
    int           (*test)(void);

    /* accelerated hmac callback: if you need to-do multiple packets just use the generic hmac_memory and provide a hash callback */
    int           (*hmac_block)(const unsigned char *key, unsigned long keylen,
                                const unsigned char *in, unsigned long inlen,
                                unsigned char *out, unsigned long *outlen);
} hash_descriptor[];

#ifdef LTC_CHC_HASH
int chc_register(int cipher);
int chc_init(hash_state *md);
int chc_process(hash_state *md, const unsigned char *in, unsigned long inlen);
int chc_done(hash_state *md, unsigned char *hash);
int chc_test(void);

extern const struct ltc_hash_descriptor chc_desc;
#endif

#ifdef LTC_WHIRLPOOL
int whirlpool_init(hash_state *md);
int whirlpool_process(hash_state *md, const unsigned char *in, unsigned long inlen);
int whirlpool_done(hash_state *md, unsigned char *hash);
int whirlpool_test(void);

extern const struct ltc_hash_descriptor whirlpool_desc;
#endif

#ifdef LTC_SHA512
int sha512_init(hash_state *md);
int sha512_process(hash_state *md, const unsigned char *in, unsigned long inlen);
int sha512_done(hash_state *md, unsigned char *hash);
int sha512_test(void);

extern const struct ltc_hash_descriptor sha512_desc;
#endif

#ifdef LTC_SHA384
 #ifndef LTC_SHA512
  #error LTC_SHA512 is required for LTC_SHA384
 #endif
int sha384_init(hash_state *md);

 #define sha384_process    sha512_process
int sha384_done(hash_state *md, unsigned char *hash);
int sha384_test(void);

extern const struct ltc_hash_descriptor sha384_desc;
#endif

#ifdef LTC_SHA256
int sha256_init(hash_state *md);
int sha256_process(hash_state *md, const unsigned char *in, unsigned long inlen);
int sha256_done(hash_state *md, unsigned char *hash);
int sha256_test(void);

extern const struct ltc_hash_descriptor sha256_desc;

 #ifdef LTC_SHA224
  #ifndef LTC_SHA256
   #error LTC_SHA256 is required for LTC_SHA224
  #endif
int sha224_init(hash_state *md);

  #define sha224_process    sha256_process
int sha224_done(hash_state *md, unsigned char *hash);
int sha224_test(void);

extern const struct ltc_hash_descriptor sha224_desc;
 #endif
#endif

#ifdef LTC_SHA1
int sha1_init(hash_state *md);
int sha1_process(hash_state *md, const unsigned char *in, unsigned long inlen);
int sha1_done(hash_state *md, unsigned char *hash);
int sha1_test(void);

extern const struct ltc_hash_descriptor sha1_desc;
#endif

#ifdef LTC_MD5
int md5_init(hash_state *md);
int md5_process(hash_state *md, const unsigned char *in, unsigned long inlen);
int md5_done(hash_state *md, unsigned char *hash);
int md5_test(void);

extern const struct ltc_hash_descriptor md5_desc;
#endif

#ifdef LTC_MD4
int md4_init(hash_state *md);
int md4_process(hash_state *md, const unsigned char *in, unsigned long inlen);
int md4_done(hash_state *md, unsigned char *hash);
int md4_test(void);

extern const struct ltc_hash_descriptor md4_desc;
#endif

#ifdef LTC_MD2
int md2_init(hash_state *md);
int md2_process(hash_state *md, const unsigned char *in, unsigned long inlen);
int md2_done(hash_state *md, unsigned char *hash);
int md2_test(void);

extern const struct ltc_hash_descriptor md2_desc;
#endif

#ifdef LTC_TIGER
int tiger_init(hash_state *md);
int tiger_process(hash_state *md, const unsigned char *in, unsigned long inlen);
int tiger_done(hash_state *md, unsigned char *hash);
int tiger_test(void);

extern const struct ltc_hash_descriptor tiger_desc;
#endif

#ifdef LTC_RIPEMD128
int rmd128_init(hash_state *md);
int rmd128_process(hash_state *md, const unsigned char *in, unsigned long inlen);
int rmd128_done(hash_state *md, unsigned char *hash);
int rmd128_test(void);

extern const struct ltc_hash_descriptor rmd128_desc;
#endif

#ifdef LTC_RIPEMD160
int rmd160_init(hash_state *md);
int rmd160_process(hash_state *md, const unsigned char *in, unsigned long inlen);
int rmd160_done(hash_state *md, unsigned char *hash);
int rmd160_test(void);

extern const struct ltc_hash_descriptor rmd160_desc;
#endif

#ifdef LTC_RIPEMD256
int rmd256_init(hash_state *md);
int rmd256_process(hash_state *md, const unsigned char *in, unsigned long inlen);
int rmd256_done(hash_state *md, unsigned char *hash);
int rmd256_test(void);

extern const struct ltc_hash_descriptor rmd256_desc;
#endif

#ifdef LTC_RIPEMD320
int rmd320_init(hash_state *md);
int rmd320_process(hash_state *md, const unsigned char *in, unsigned long inlen);
int rmd320_done(hash_state *md, unsigned char *hash);
int rmd320_test(void);

extern const struct ltc_hash_descriptor rmd320_desc;
#endif


int find_hash(const char *name);
int find_hash_id(unsigned char ID);
int find_hash_oid(const unsigned long *ID, unsigned long IDlen);
int find_hash_any(const char *name, int digestlen);
int register_hash(const struct ltc_hash_descriptor *hash);
int unregister_hash(const struct ltc_hash_descriptor *hash);
int hash_is_valid(int idx);

LTC_MUTEX_PROTO(ltc_hash_mutex)

int hash_memory(int hash,
                const unsigned char *in, unsigned long inlen,
                unsigned char *out, unsigned long *outlen);
int hash_memory_multi(int hash, unsigned char *out, unsigned long *outlen,
                      const unsigned char *in, unsigned long inlen, ...);
int hash_filehandle(int hash, FILE *in, unsigned char *out, unsigned long *outlen);
int hash_file(int hash, const char *fname, unsigned char *out, unsigned long *outlen);

/* a simple macro for making hash "process" functions */
#define HASH_PROCESS(func_name, compress_name, state_var, block_size)               \
    int func_name(hash_state * md, const unsigned char *in, unsigned long inlen)    \
    {                                                                               \
        unsigned long n;                                                            \
        int           err;                                                          \
        LTC_ARGCHK(md != NULL);                                                     \
        LTC_ARGCHK(in != NULL);                                                     \
        if (md->state_var.curlen > sizeof(md->state_var.buf)) {                     \
            return CRYPT_INVALID_ARG;                                               \
        }                                                                           \
        while (inlen > 0) {                                                         \
            if (md->state_var.curlen == 0 && inlen >= block_size) {                 \
                if ((err = compress_name(md, (unsigned char *)in)) != CRYPT_OK) {   \
                    return err;                                                     \
                }                                                                   \
                md->state_var.length += block_size * 8;                             \
                in    += block_size;                                                \
                inlen -= block_size;                                                \
            } else {                                                                \
                n = MIN(inlen, (block_size - md->state_var.curlen));                \
                memcpy(md->state_var.buf + md->state_var.curlen, in, (size_t)n);    \
                md->state_var.curlen += n;                                          \
                in    += n;                                                         \
                inlen -= n;                                                         \
                if (md->state_var.curlen == block_size) {                           \
                    if ((err = compress_name(md, md->state_var.buf)) != CRYPT_OK) { \
                        return err;                                                 \
                    }                                                               \
                    md->state_var.length += 8 * block_size;                         \
                    md->state_var.curlen  = 0;                                      \
                }                                                                   \
            }                                                                       \
        }                                                                           \
        return CRYPT_OK;                                                            \
    }

/* $Source: /cvs/libtom/libtomcrypt/src/headers/tomcrypt_hash.h,v $ */
/* $Revision: 1.22 $ */
/* $Date: 2007/05/12 14:32:35 $ */

#ifdef LTC_HMAC
typedef struct Hmac_state {
    hash_state    md;
    int           hash;
    hash_state    hashstate;
    unsigned char *key;
} hmac_state;

int hmac_init(hmac_state *hmac, int hash, const unsigned char *key, unsigned long keylen);
int hmac_process(hmac_state *hmac, const unsigned char *in, unsigned long inlen);
int hmac_done(hmac_state *hmac, unsigned char *out, unsigned long *outlen);
int hmac_test(void);
int hmac_memory(int hash,
                const unsigned char *key, unsigned long keylen,
                const unsigned char *in, unsigned long inlen,
                unsigned char *out, unsigned long *outlen);
int hmac_memory_multi(int hash,
                      const unsigned char *key, unsigned long keylen,
                      unsigned char *out, unsigned long *outlen,
                      const unsigned char *in, unsigned long inlen, ...);
int hmac_file(int hash, const char *fname, const unsigned char *key,
              unsigned long keylen,
              unsigned char *dst, unsigned long *dstlen);
#endif

#ifdef LTC_OMAC

typedef struct {
    int           cipher_idx,
                  buflen,
                  blklen;
    unsigned char block[MAXBLOCKSIZE],
                  prev[MAXBLOCKSIZE],
                  Lu[2][MAXBLOCKSIZE];
    symmetric_key key;
} omac_state;

int omac_init(omac_state *omac, int cipher, const unsigned char *key, unsigned long keylen);
int omac_process(omac_state *omac, const unsigned char *in, unsigned long inlen);
int omac_done(omac_state *omac, unsigned char *out, unsigned long *outlen);
int omac_memory(int cipher,
                const unsigned char *key, unsigned long keylen,
                const unsigned char *in, unsigned long inlen,
                unsigned char *out, unsigned long *outlen);
int omac_memory_multi(int cipher,
                      const unsigned char *key, unsigned long keylen,
                      unsigned char *out, unsigned long *outlen,
                      const unsigned char *in, unsigned long inlen, ...);
int omac_file(int cipher,
              const unsigned char *key, unsigned long keylen,
              const char *filename,
              unsigned char *out, unsigned long *outlen);
int omac_test(void);
#endif /* LTC_OMAC */

#ifdef LTC_PMAC

typedef struct {
    unsigned char Ls[32][MAXBLOCKSIZE],       /* L shifted by i bits to the left */
                  Li[MAXBLOCKSIZE],           /* value of Li [current value, we calc from previous recall] */
                  Lr[MAXBLOCKSIZE],           /* L * x^-1 */
                  block[MAXBLOCKSIZE],        /* currently accumulated block */
                  checksum[MAXBLOCKSIZE];     /* current checksum */

    symmetric_key key;                        /* scheduled key for cipher */
    unsigned long block_index;                /* index # for current block */
    int           cipher_idx,                 /* cipher idx */
                  block_len,                  /* length of block */
                  buflen;                     /* number of bytes in the buffer */
} pmac_state;

int pmac_init(pmac_state *pmac, int cipher, const unsigned char *key, unsigned long keylen);
int pmac_process(pmac_state *pmac, const unsigned char *in, unsigned long inlen);
int pmac_done(pmac_state *pmac, unsigned char *out, unsigned long *outlen);

int pmac_memory(int cipher,
                const unsigned char *key, unsigned long keylen,
                const unsigned char *msg, unsigned long msglen,
                unsigned char *out, unsigned long *outlen);

int pmac_memory_multi(int cipher,
                      const unsigned char *key, unsigned long keylen,
                      unsigned char *out, unsigned long *outlen,
                      const unsigned char *in, unsigned long inlen, ...);

int pmac_file(int cipher,
              const unsigned char *key, unsigned long keylen,
              const char *filename,
              unsigned char *out, unsigned long *outlen);

int pmac_test(void);

/* internal functions */
int pmac_ntz(unsigned long x);
void pmac_shift_xor(pmac_state *pmac);
#endif /* PMAC */

#ifdef LTC_EAX_MODE

 #if !(defined(LTC_OMAC) && defined(LTC_CTR_MODE))
  #error LTC_EAX_MODE requires LTC_OMAC and CTR
 #endif

typedef struct {
    unsigned char N[MAXBLOCKSIZE];
    symmetric_CTR ctr;
    omac_state    headeromac, ctomac;
} eax_state;

int eax_init(eax_state *eax, int cipher, const unsigned char *key, unsigned long keylen,
             const unsigned char *nonce, unsigned long noncelen,
             const unsigned char *header, unsigned long headerlen);

int eax_encrypt(eax_state *eax, const unsigned char *pt, unsigned char *ct, unsigned long length);
int eax_decrypt(eax_state *eax, const unsigned char *ct, unsigned char *pt, unsigned long length);
int eax_addheader(eax_state *eax, const unsigned char *header, unsigned long length);
int eax_done(eax_state *eax, unsigned char *tag, unsigned long *taglen);

int eax_encrypt_authenticate_memory(int cipher,
                                    const unsigned char *key, unsigned long keylen,
                                    const unsigned char *nonce, unsigned long noncelen,
                                    const unsigned char *header, unsigned long headerlen,
                                    const unsigned char *pt, unsigned long ptlen,
                                    unsigned char *ct,
                                    unsigned char *tag, unsigned long *taglen);

int eax_decrypt_verify_memory(int cipher,
                              const unsigned char *key, unsigned long keylen,
                              const unsigned char *nonce, unsigned long noncelen,
                              const unsigned char *header, unsigned long headerlen,
                              const unsigned char *ct, unsigned long ctlen,
                              unsigned char *pt,
                              unsigned char *tag, unsigned long taglen,
                              int *stat);

int eax_test(void);
#endif /* EAX MODE */

#ifdef LTC_OCB_MODE
typedef struct {
    unsigned char L[MAXBLOCKSIZE],            /* L value */
                  Ls[32][MAXBLOCKSIZE],       /* L shifted by i bits to the left */
                  Li[MAXBLOCKSIZE],           /* value of Li [current value, we calc from previous recall] */
                  Lr[MAXBLOCKSIZE],           /* L * x^-1 */
                  R[MAXBLOCKSIZE],            /* R value */
                  checksum[MAXBLOCKSIZE];     /* current checksum */

    symmetric_key key;                        /* scheduled key for cipher */
    unsigned long block_index;                /* index # for current block */
    int           cipher,                     /* cipher idx */
                  block_len;                  /* length of block */
} ocb_state;

int ocb_init(ocb_state *ocb, int cipher,
             const unsigned char *key, unsigned long keylen, const unsigned char *nonce);

int ocb_encrypt(ocb_state *ocb, const unsigned char *pt, unsigned char *ct);
int ocb_decrypt(ocb_state *ocb, const unsigned char *ct, unsigned char *pt);

int ocb_done_encrypt(ocb_state *ocb,
                     const unsigned char *pt, unsigned long ptlen,
                     unsigned char *ct,
                     unsigned char *tag, unsigned long *taglen);

int ocb_done_decrypt(ocb_state *ocb,
                     const unsigned char *ct, unsigned long ctlen,
                     unsigned char *pt,
                     const unsigned char *tag, unsigned long taglen, int *stat);

int ocb_encrypt_authenticate_memory(int cipher,
                                    const unsigned char *key, unsigned long keylen,
                                    const unsigned char *nonce,
                                    const unsigned char *pt, unsigned long ptlen,
                                    unsigned char *ct,
                                    unsigned char *tag, unsigned long *taglen);

int ocb_decrypt_verify_memory(int cipher,
                              const unsigned char *key, unsigned long keylen,
                              const unsigned char *nonce,
                              const unsigned char *ct, unsigned long ctlen,
                              unsigned char *pt,
                              const unsigned char *tag, unsigned long taglen,
                              int *stat);

int ocb_test(void);

/* internal functions */
void ocb_shift_xor(ocb_state *ocb, unsigned char *Z);
int ocb_ntz(unsigned long x);
int s_ocb_done(ocb_state *ocb, const unsigned char *pt, unsigned long ptlen,
               unsigned char *ct, unsigned char *tag, unsigned long *taglen, int mode);
#endif /* LTC_OCB_MODE */

#ifdef LTC_CCM_MODE

 #define CCM_ENCRYPT    0
 #define CCM_DECRYPT    1

int ccm_memory(int cipher,
               const unsigned char *key, unsigned long keylen,
               symmetric_key *uskey,
               const unsigned char *nonce, unsigned long noncelen,
               const unsigned char *header, unsigned long headerlen,
               unsigned char *pt, unsigned long ptlen,
               unsigned char *ct,
               unsigned char *tag, unsigned long *taglen,
               int direction);

int ccm_test(void);
#endif /* LTC_CCM_MODE */

#if defined(LRW_MODE) || defined(LTC_GCM_MODE)
void gcm_gf_mult(const unsigned char *a, const unsigned char *b, unsigned char *c);
#endif


/* table shared between GCM and LRW */
#if defined(LTC_GCM_TABLES) || defined(LRW_TABLES) || ((defined(LTC_GCM_MODE) || defined(LTC_GCM_MODE)) && defined(LTC_FAST))
extern const unsigned char gcm_shift_table[];
#endif

#ifdef LTC_GCM_MODE

 #define GCM_ENCRYPT          0
 #define GCM_DECRYPT          1

 #define LTC_GCM_MODE_IV      0
 #define LTC_GCM_MODE_AAD     1
 #define LTC_GCM_MODE_TEXT    2

typedef struct {
    symmetric_key K;
    unsigned char H[16],             /* multiplier */
                  X[16],             /* accumulator */
                  Y[16],             /* counter */
                  Y_0[16],           /* initial counter */
                  buf[16];           /* buffer for stuff */

    int           cipher,            /* which cipher */
                  ivmode,            /* Which mode is the IV in? */
                  mode,              /* mode the GCM code is in */
                  buflen;            /* length of data in buf */

    ulong64       totlen,            /* 64-bit counter used for IV and AAD */
                  pttotlen;          /* 64-bit counter for the PT */

 #ifdef LTC_GCM_TABLES
    unsigned char PC[16][256][16]       /* 16 tables of 8x128 */
  #ifdef LTC_GCM_TABLES_SSE2
    __attribute__ ((aligned(16)))
  #endif
    ;
 #endif
} gcm_state;

void gcm_mult_h(gcm_state *gcm, unsigned char *I);

int gcm_init(gcm_state *gcm, int cipher,
             const unsigned char *key, int keylen);

int gcm_reset(gcm_state *gcm);

int gcm_add_iv(gcm_state *gcm,
               const unsigned char *IV, unsigned long IVlen);

int gcm_add_aad(gcm_state *gcm,
                const unsigned char *adata, unsigned long adatalen);

int gcm_process(gcm_state *gcm,
                unsigned char *pt, unsigned long ptlen,
                unsigned char *ct,
                int direction);

int gcm_done(gcm_state *gcm,
             unsigned char *tag, unsigned long *taglen);

int gcm_memory(int cipher,
               const unsigned char *key, unsigned long keylen,
               const unsigned char *IV, unsigned long IVlen,
               const unsigned char *adata, unsigned long adatalen,
               unsigned char *pt, unsigned long ptlen,
               unsigned char *ct,
               unsigned char *tag, unsigned long *taglen,
               int direction);
int gcm_test(void);
#endif /* LTC_GCM_MODE */

#ifdef LTC_PELICAN

typedef struct pelican_state {
    symmetric_key K;
    unsigned char state[16];
    int           buflen;
} pelican_state;

int pelican_init(pelican_state *pelmac, const unsigned char *key, unsigned long keylen);
int pelican_process(pelican_state *pelmac, const unsigned char *in, unsigned long inlen);
int pelican_done(pelican_state *pelmac, unsigned char *out);
int pelican_test(void);

int pelican_memory(const unsigned char *key, unsigned long keylen,
                   const unsigned char *in, unsigned long inlen,
                   unsigned char *out);
#endif

#ifdef LTC_XCBC

/* add this to "keylen" to xcbc_init to use a pure three-key XCBC MAC */
 #define LTC_XCBC_PURE    0x8000UL

typedef struct {
    unsigned char K[3][MAXBLOCKSIZE],
                  IV[MAXBLOCKSIZE];

    symmetric_key key;

    int           cipher,
                  buflen,
                  blocksize;
} xcbc_state;

int xcbc_init(xcbc_state *xcbc, int cipher, const unsigned char *key, unsigned long keylen);
int xcbc_process(xcbc_state *xcbc, const unsigned char *in, unsigned long inlen);
int xcbc_done(xcbc_state *xcbc, unsigned char *out, unsigned long *outlen);
int xcbc_memory(int cipher,
                const unsigned char *key, unsigned long keylen,
                const unsigned char *in, unsigned long inlen,
                unsigned char *out, unsigned long *outlen);
int xcbc_memory_multi(int cipher,
                      const unsigned char *key, unsigned long keylen,
                      unsigned char *out, unsigned long *outlen,
                      const unsigned char *in, unsigned long inlen, ...);
int xcbc_file(int cipher,
              const unsigned char *key, unsigned long keylen,
              const char *filename,
              unsigned char *out, unsigned long *outlen);
int xcbc_test(void);
#endif

#ifdef LTC_F9_MODE

typedef struct {
    unsigned char akey[MAXBLOCKSIZE],
                  ACC[MAXBLOCKSIZE],
                  IV[MAXBLOCKSIZE];

    symmetric_key key;

    int           cipher,
                  buflen,
                  keylen,
                  blocksize;
} f9_state;

int f9_init(f9_state *f9, int cipher, const unsigned char *key, unsigned long keylen);
int f9_process(f9_state *f9, const unsigned char *in, unsigned long inlen);
int f9_done(f9_state *f9, unsigned char *out, unsigned long *outlen);
int f9_memory(int cipher,
              const unsigned char *key, unsigned long keylen,
              const unsigned char *in, unsigned long inlen,
              unsigned char *out, unsigned long *outlen);
int f9_memory_multi(int cipher,
                    const unsigned char *key, unsigned long keylen,
                    unsigned char *out, unsigned long *outlen,
                    const unsigned char *in, unsigned long inlen, ...);
int f9_file(int cipher,
            const unsigned char *key, unsigned long keylen,
            const char *filename,
            unsigned char *out, unsigned long *outlen);
int f9_test(void);
#endif


/* $Source: /cvs/libtom/libtomcrypt/src/headers/tomcrypt_mac.h,v $ */
/* $Revision: 1.23 $ */
/* $Date: 2007/05/12 14:37:41 $ */

/* ---- PRNG Stuff ---- */
#ifdef LTC_YARROW
struct yarrow_prng {
    int           cipher, hash;
    unsigned char pool[MAXBLOCKSIZE];
    symmetric_CTR ctr;
    LTC_MUTEX_TYPE(prng_lock)
};
#endif

#ifdef LTC_RC4
struct rc4_prng {
    int           x, y;
    unsigned char buf[256];
};
#endif

#ifdef LTC_FORTUNA
struct fortuna_prng {
    hash_state    pool[LTC_FORTUNA_POOLS];  /* the  pools */

    symmetric_key skey;

    unsigned char K[32],      /* the current key */
                  IV[16];     /* IV for CTR mode */

    unsigned long pool_idx,   /* current pool we will add to */
                  pool0_len,  /* length of 0'th pool */
                  wd;

    ulong64       reset_cnt;  /* number of times we have reset */
    LTC_MUTEX_TYPE(prng_lock)
};
#endif

#ifdef LTC_SOBER128
struct sober128_prng {
    ulong32 R[17],               /* Working storage for the shift register */
            initR[17],           /* saved register contents */
            konst,               /* key dependent constant */
            sbuf;                /* partial word encryption buffer */

    int     nbuf,                /* number of part-word stream bits buffered */
            flag,                /* first add_entropy call or not? */
            set;                 /* did we call add_entropy to set key? */
};
#endif

typedef union Prng_state {
    char                 dummy[1];
#ifdef LTC_YARROW
    struct yarrow_prng   yarrow;
#endif
#ifdef LTC_RC4
    struct rc4_prng      rc4;
#endif
#ifdef LTC_FORTUNA
    struct fortuna_prng  fortuna;
#endif
#ifdef LTC_SOBER128
    struct sober128_prng sober128;
#endif
} prng_state;

/** PRNG descriptor */
extern struct ltc_prng_descriptor {
    /** Name of the PRNG */
    char          *name;
    /** size in bytes of exported state */
    int           export_size;

    /** Start a PRNG state
        @param prng   [out] The state to initialize
        @return CRYPT_OK if successful
     */
    int           (*start)(prng_state *prng);

    /** Add entropy to the PRNG
        @param in         The entropy
        @param inlen      Length of the entropy (octets)\
        @param prng       The PRNG state
        @return CRYPT_OK if successful
     */
    int           (*add_entropy)(const unsigned char *in, unsigned long inlen, prng_state *prng);

    /** Ready a PRNG state to read from
        @param prng       The PRNG state to ready
        @return CRYPT_OK if successful
     */
    int           (*ready)(prng_state *prng);

    /** Read from the PRNG
        @param out     [out] Where to store the data
        @param outlen  Length of data desired (octets)
        @param prng    The PRNG state to read from
        @return Number of octets read
     */
    unsigned long (*read)(unsigned char *out, unsigned long outlen, prng_state *prng);

    /** Terminate a PRNG state
        @param prng   The PRNG state to terminate
        @return CRYPT_OK if successful
     */
    int           (*done)(prng_state *prng);

    /** Export a PRNG state
        @param out     [out] The destination for the state
        @param outlen  [in/out] The max size and resulting size of the PRNG state
        @param prng    The PRNG to export
        @return CRYPT_OK if successful
     */
    int           (*pexport)(unsigned char *out, unsigned long *outlen, prng_state *prng);

    /** Import a PRNG state
        @param in      The data to import
        @param inlen   The length of the data to import (octets)
        @param prng    The PRNG to initialize/import
        @return CRYPT_OK if successful
     */
    int           (*pimport)(const unsigned char *in, unsigned long inlen, prng_state *prng);

    /** Self-test the PRNG
        @return CRYPT_OK if successful, CRYPT_NOP if self-testing has been disabled
     */
    int           (*test)(void);
} prng_descriptor[];

#ifdef LTC_YARROW
int yarrow_start(prng_state *prng);
int yarrow_add_entropy(const unsigned char *in, unsigned long inlen, prng_state *prng);
int yarrow_ready(prng_state *prng);
unsigned long yarrow_read(unsigned char *out, unsigned long outlen, prng_state *prng);
int yarrow_done(prng_state *prng);
int  yarrow_export(unsigned char *out, unsigned long *outlen, prng_state *prng);
int  yarrow_import(const unsigned char *in, unsigned long inlen, prng_state *prng);
int  yarrow_test(void);

extern const struct ltc_prng_descriptor yarrow_desc;
#endif

#ifdef LTC_FORTUNA
int fortuna_start(prng_state *prng);
int fortuna_add_entropy(const unsigned char *in, unsigned long inlen, prng_state *prng);
int fortuna_ready(prng_state *prng);
unsigned long fortuna_read(unsigned char *out, unsigned long outlen, prng_state *prng);
int fortuna_done(prng_state *prng);
int  fortuna_export(unsigned char *out, unsigned long *outlen, prng_state *prng);
int  fortuna_import(const unsigned char *in, unsigned long inlen, prng_state *prng);
int  fortuna_test(void);

extern const struct ltc_prng_descriptor fortuna_desc;
#endif

#ifdef LTC_RC4
int rc4_start(prng_state *prng);
int rc4_add_entropy(const unsigned char *in, unsigned long inlen, prng_state *prng);
int rc4_ready(prng_state *prng);
unsigned long rc4_read(unsigned char *out, unsigned long outlen, prng_state *prng);
int  rc4_done(prng_state *prng);
int  rc4_export(unsigned char *out, unsigned long *outlen, prng_state *prng);
int  rc4_import(const unsigned char *in, unsigned long inlen, prng_state *prng);
int  rc4_test(void);

extern const struct ltc_prng_descriptor rc4_desc;
#endif

#ifdef LTC_SPRNG
int sprng_start(prng_state *prng);
int sprng_add_entropy(const unsigned char *in, unsigned long inlen, prng_state *prng);
int sprng_ready(prng_state *prng);
unsigned long sprng_read(unsigned char *out, unsigned long outlen, prng_state *prng);
int sprng_done(prng_state *prng);
int  sprng_export(unsigned char *out, unsigned long *outlen, prng_state *prng);
int  sprng_import(const unsigned char *in, unsigned long inlen, prng_state *prng);
int  sprng_test(void);

extern const struct ltc_prng_descriptor sprng_desc;
#endif

#ifdef LTC_SOBER128
int sober128_start(prng_state *prng);
int sober128_add_entropy(const unsigned char *in, unsigned long inlen, prng_state *prng);
int sober128_ready(prng_state *prng);
unsigned long sober128_read(unsigned char *out, unsigned long outlen, prng_state *prng);
int sober128_done(prng_state *prng);
int  sober128_export(unsigned char *out, unsigned long *outlen, prng_state *prng);
int  sober128_import(const unsigned char *in, unsigned long inlen, prng_state *prng);
int  sober128_test(void);

extern const struct ltc_prng_descriptor sober128_desc;
#endif

int find_prng(const char *name);
int register_prng(const struct ltc_prng_descriptor *prng);
int unregister_prng(const struct ltc_prng_descriptor *prng);
int prng_is_valid(int idx);

LTC_MUTEX_PROTO(ltc_prng_mutex)

/* Slow RNG you **might** be able to use to seed a PRNG with.  Be careful as this
 * might not work on all platforms as planned
 */
unsigned long rng_get_bytes(unsigned char *out,
                            unsigned long outlen,
                            void (        *callback)(void));

int rng_make_prng(int bits, int wprng, prng_state *prng, void (*callback)(void));


/* $Source: /cvs/libtom/libtomcrypt/src/headers/tomcrypt_prng.h,v $ */
/* $Revision: 1.9 $ */
/* $Date: 2007/05/12 14:32:35 $ */

/* ---- NUMBER THEORY ---- */

enum {
    PK_PUBLIC =0,
    PK_PRIVATE=1
};

int rand_prime(void *N, long len, prng_state *prng, int wprng);

/* ---- RSA ---- */
#ifdef LTC_MRSA

/* Min and Max RSA key sizes (in bits) */
 #define MIN_RSA_SIZE    1024
 #define MAX_RSA_SIZE    4096

/** RSA LTC_PKCS style key */
typedef struct Rsa_key {
    /** Type of key, PK_PRIVATE or PK_PUBLIC */
    int  type;
    /** The public exponent */
    void *e;
    /** The private exponent */
    void *d;
    /** The modulus */
    void *N;
    /** The p factor of N */
    void *p;
    /** The q factor of N */
    void *q;
    /** The 1/q mod p CRT param */
    void *qP;
    /** The d mod (p - 1) CRT param */
    void *dP;
    /** The d mod (q - 1) CRT param */
    void *dQ;
} rsa_key;

int rsa_make_key(prng_state *prng, int wprng, int size, long e, rsa_key *key);

int rsa_exptmod(const unsigned char *in, unsigned long inlen,
                unsigned char *out, unsigned long *outlen, int which,
                rsa_key *key);

void rsa_free(rsa_key *key);

/* These use LTC_PKCS #1 v2.0 padding */
 #define rsa_encrypt_key(_in, _inlen, _out, _outlen, _lparam, _lparamlen, _prng, _prng_idx, _hash_idx, _key) \
    rsa_encrypt_key_ex(_in, _inlen, _out, _outlen, _lparam, _lparamlen, _prng, _prng_idx, _hash_idx, LTC_LTC_PKCS_1_OAEP, _key)

 #define rsa_decrypt_key(_in, _inlen, _out, _outlen, _lparam, _lparamlen, _hash_idx, _stat, _key) \
    rsa_decrypt_key_ex(_in, _inlen, _out, _outlen, _lparam, _lparamlen, _hash_idx, LTC_LTC_PKCS_1_OAEP, _stat, _key)

 #define rsa_sign_hash(_in, _inlen, _out, _outlen, _prng, _prng_idx, _hash_idx, _saltlen, _key) \
    rsa_sign_hash_ex(_in, _inlen, _out, _outlen, LTC_LTC_PKCS_1_PSS, _prng, _prng_idx, _hash_idx, _saltlen, _key)

 #define rsa_verify_hash(_sig, _siglen, _hash, _hashlen, _hash_idx, _saltlen, _stat, _key) \
    rsa_verify_hash_ex(_sig, _siglen, _hash, _hashlen, LTC_LTC_PKCS_1_PSS, _hash_idx, _saltlen, _stat, _key)

/* These can be switched between LTC_PKCS #1 v2.x and LTC_PKCS #1 v1.5 paddings */
int rsa_encrypt_key_ex(const unsigned char *in, unsigned long inlen,
                       unsigned char *out, unsigned long *outlen,
                       const unsigned char *lparam, unsigned long lparamlen,
                       prng_state *prng, int prng_idx, int hash_idx, int padding, rsa_key *key);

int rsa_decrypt_key_ex(const unsigned char *in, unsigned long inlen,
                       unsigned char *out, unsigned long *outlen,
                       const unsigned char *lparam, unsigned long lparamlen,
                       int hash_idx, int padding,
                       int *stat, rsa_key *key);

int rsa_sign_hash_ex(const unsigned char *in, unsigned long inlen,
                     unsigned char *out, unsigned long *outlen,
                     int padding,
                     prng_state *prng, int prng_idx,
                     int hash_idx, unsigned long saltlen,
                     rsa_key *key);

int rsa_verify_hash_ex(const unsigned char *sig, unsigned long siglen,
                       const unsigned char *hash, unsigned long hashlen,
                       int padding,
                       int hash_idx, unsigned long saltlen,
                       int *stat, rsa_key *key);

/* LTC_PKCS #1 import/export */
int rsa_export(unsigned char *out, unsigned long *outlen, int type, rsa_key *key);
int rsa_import(const unsigned char *in, unsigned long inlen, rsa_key *key);
#endif

/* ---- Katja ---- */
#ifdef MKAT

/* Min and Max KAT key sizes (in bits) */
 #define MIN_KAT_SIZE    1024
 #define MAX_KAT_SIZE    4096

/** Katja LTC_PKCS style key */
typedef struct KAT_key {
    /** Type of key, PK_PRIVATE or PK_PUBLIC */
    int  type;
    /** The private exponent */
    void *d;
    /** The modulus */
    void *N;
    /** The p factor of N */
    void *p;
    /** The q factor of N */
    void *q;
    /** The 1/q mod p CRT param */
    void *qP;
    /** The d mod (p - 1) CRT param */
    void *dP;
    /** The d mod (q - 1) CRT param */
    void *dQ;
    /** The pq param */
    void *pq;
} katja_key;

int katja_make_key(prng_state *prng, int wprng, int size, katja_key *key);

int katja_exptmod(const unsigned char *in, unsigned long inlen,
                  unsigned char *out, unsigned long *outlen, int which,
                  katja_key *key);

void katja_free(katja_key *key);

/* These use LTC_PKCS #1 v2.0 padding */
int katja_encrypt_key(const unsigned char *in, unsigned long inlen,
                      unsigned char *out, unsigned long *outlen,
                      const unsigned char *lparam, unsigned long lparamlen,
                      prng_state *prng, int prng_idx, int hash_idx, katja_key *key);

int katja_decrypt_key(const unsigned char *in, unsigned long inlen,
                      unsigned char *out, unsigned long *outlen,
                      const unsigned char *lparam, unsigned long lparamlen,
                      int hash_idx, int *stat,
                      katja_key *key);

/* LTC_PKCS #1 import/export */
int katja_export(unsigned char *out, unsigned long *outlen, int type, katja_key *key);
int katja_import(const unsigned char *in, unsigned long inlen, katja_key *key);
#endif

/* ---- ECC Routines ---- */
#ifdef LTC_MECC

/* size of our temp buffers for exported keys */
 #define ECC_BUF_SIZE    256

/* max private key size */
 #define ECC_MAXSIZE     66

/** Structure defines a NIST GF(p) curve */
typedef struct {
    /** The size of the curve in octets */
    int  size;

    /** name of curve */
    char *name;

    /** The prime that defines the field the curve is in (encoded in hex) */
    char *prime;

    /** The fields B param (hex) */
    char *B;

    /** The order of the curve (hex) */
    char *order;

    /** The x co-ordinate of the base point on the curve (hex) */
    char *Gx;

    /** The y co-ordinate of the base point on the curve (hex) */
    char *Gy;
} ltc_ecc_set_type;

/** A point on a ECC curve, stored in Jacbobian format such that (x,y,z) => (x/z^2, y/z^3, 1) when interpretted as affine */
typedef struct {
    /** The x co-ordinate */
    void *x;

    /** The y co-ordinate */
    void *y;

    /** The z co-ordinate */
    void *z;
} ecc_point;

/** An ECC key */
typedef struct {
    /** Type of key, PK_PRIVATE or PK_PUBLIC */
    int                    type;

    /** Index into the ltc_ecc_sets[] for the parameters of this curve; if -1, then this key is using user supplied curve in dp */
    int                    idx;

    /** pointer to domain parameters; either points to NIST curves (identified by idx >= 0) or user supplied curve */
    const ltc_ecc_set_type *dp;

    /** The public key */
    ecc_point              pubkey;

    /** The private key */
    void                   *k;
} ecc_key;

/** the ECC params provided */
extern const ltc_ecc_set_type ltc_ecc_sets[];

int  ecc_test(void);
void ecc_sizes(int *low, int *high);
int  ecc_get_size(ecc_key *key);

int  ecc_make_key(prng_state *prng, int wprng, int keysize, ecc_key *key);
int  ecc_make_key_ex(prng_state *prng, int wprng, ecc_key *key, const ltc_ecc_set_type *dp);
void ecc_free(ecc_key *key);

int  ecc_export(unsigned char *out, unsigned long *outlen, int type, ecc_key *key);
int  ecc_import(const unsigned char *in, unsigned long inlen, ecc_key *key);
int  ecc_import_ex(const unsigned char *in, unsigned long inlen, ecc_key *key, const ltc_ecc_set_type *dp);

int ecc_ansi_x963_export(ecc_key *key, unsigned char *out, unsigned long *outlen);
int ecc_ansi_x963_import(const unsigned char *in, unsigned long inlen, ecc_key *key);
int ecc_ansi_x963_import_ex(const unsigned char *in, unsigned long inlen, ecc_key *key, ltc_ecc_set_type *dp);

int  ecc_shared_secret(ecc_key *private_key, ecc_key *public_key,
                       unsigned char *out, unsigned long *outlen);

int  ecc_encrypt_key(const unsigned char *in, unsigned long inlen,
                     unsigned char *out, unsigned long *outlen,
                     prng_state *prng, int wprng, int hash,
                     ecc_key *key);

int  ecc_decrypt_key(const unsigned char *in, unsigned long inlen,
                     unsigned char *out, unsigned long *outlen,
                     ecc_key *key);

int  ecc_sign_hash(const unsigned char *in, unsigned long inlen,
                   unsigned char *out, unsigned long *outlen,
                   prng_state *prng, int wprng, ecc_key *key);

int  ecc_verify_hash(const unsigned char *sig, unsigned long siglen,
                     const unsigned char *hash, unsigned long hashlen,
                     int *stat, ecc_key *key);

/* low level functions */
ecc_point *ltc_ecc_new_point(void);
void       ltc_ecc_del_point(ecc_point *p);
int        ltc_ecc_is_valid_idx(int n);

/* point ops (mp == montgomery digit) */
 #if !defined(LTC_MECC_ACCEL) || defined(LTM_LTC_DESC) || defined(GMP_LTC_DESC)
/* R = 2P */
int ltc_ecc_projective_dbl_point(ecc_point *P, ecc_point *R, void *modulus, void *mp);

/* R = P + Q */
int ltc_ecc_projective_add_point(ecc_point *P, ecc_point *Q, ecc_point *R, void *modulus, void *mp);
 #endif

 #if defined(LTC_MECC_FP)
/* optimized point multiplication using fixed point cache (HAC algorithm 14.117) */
int ltc_ecc_fp_mulmod(void *k, ecc_point *G, ecc_point *R, void *modulus, int map);

/* functions for saving/loading/freeing/adding to fixed point cache */
int ltc_ecc_fp_save_state(unsigned char **out, unsigned long *outlen);
int ltc_ecc_fp_restore_state(unsigned char *in, unsigned long inlen);
void ltc_ecc_fp_free(void);
int ltc_ecc_fp_add_point(ecc_point *g, void *modulus, int lock);

/* lock/unlock all points currently in fixed point cache */
void ltc_ecc_fp_tablelock(int lock);
 #endif

/* R = kG */
int ltc_ecc_mulmod(void *k, ecc_point *G, ecc_point *R, void *modulus, int map);

 #ifdef LTC_ECC_SHAMIR
/* kA*A + kB*B = C */
int ltc_ecc_mul2add(ecc_point *A, void *kA,
                    ecc_point *B, void *kB,
                    ecc_point *C,
                    void *modulus);

  #ifdef LTC_MECC_FP
/* Shamir's trick with optimized point multiplication using fixed point cache */
int ltc_ecc_fp_mul2add(ecc_point *A, void *kA,
                       ecc_point *B, void *kB,
                       ecc_point *C, void *modulus);
  #endif
 #endif


/* map P to affine from projective */
int ltc_ecc_map(ecc_point *P, void *modulus, void *mp);
#endif

#ifdef LTC_MDSA

/* Max diff between group and modulus size in bytes */
 #define LTC_MDSA_DELTA        512

/* Max DSA group size in bytes (default allows 4k-bit groups) */
 #define LTC_MDSA_MAX_GROUP    512

/** DSA key structure */
typedef struct {
    /** The key type, PK_PRIVATE or PK_PUBLIC */
    int  type;

    /** The order of the sub-group used in octets */
    int  qord;

    /** The generator  */
    void *g;

    /** The prime used to generate the sub-group */
    void *q;

    /** The large prime that generats the field the contains the sub-group */
    void *p;

    /** The private key */
    void *x;

    /** The public key */
    void *y;
} dsa_key;

int dsa_make_key(prng_state *prng, int wprng, int group_size, int modulus_size, dsa_key *key);
void dsa_free(dsa_key *key);

int dsa_sign_hash_raw(const unsigned char *in, unsigned long inlen,
                      void *r, void *s,
                      prng_state *prng, int wprng, dsa_key *key);

int dsa_sign_hash(const unsigned char *in, unsigned long inlen,
                  unsigned char *out, unsigned long *outlen,
                  prng_state *prng, int wprng, dsa_key *key);

int dsa_verify_hash_raw(void *r, void *s,
                        const unsigned char *hash, unsigned long hashlen,
                        int *stat, dsa_key *key);

int dsa_verify_hash(const unsigned char *sig, unsigned long siglen,
                    const unsigned char *hash, unsigned long hashlen,
                    int *stat, dsa_key *key);

int dsa_encrypt_key(const unsigned char *in, unsigned long inlen,
                    unsigned char *out, unsigned long *outlen,
                    prng_state *prng, int wprng, int hash,
                    dsa_key *key);

int dsa_decrypt_key(const unsigned char *in, unsigned long inlen,
                    unsigned char *out, unsigned long *outlen,
                    dsa_key *key);

int dsa_import(const unsigned char *in, unsigned long inlen, dsa_key *key);
int dsa_export(unsigned char *out, unsigned long *outlen, int type, dsa_key *key);
int dsa_verify_key(dsa_key *key, int *stat);

int dsa_shared_secret(void *private_key, void *base,
                      dsa_key *public_key,
                      unsigned char *out, unsigned long *outlen);
#endif

#ifdef LTC_DER
/* DER handling */

enum {
    LTC_ASN1_EOL,
    LTC_ASN1_BOOLEAN,
    LTC_ASN1_INTEGER,
    LTC_ASN1_SHORT_INTEGER,
    LTC_ASN1_BIT_STRING,
    LTC_ASN1_OCTET_STRING,
    LTC_ASN1_NULL,
    LTC_ASN1_OBJECT_IDENTIFIER,
    LTC_ASN1_IA5_STRING,
    LTC_ASN1_PRINTABLE_STRING,
    LTC_ASN1_UTF8_STRING,
    LTC_ASN1_UTCTIME,
    LTC_ASN1_CHOICE,
    LTC_ASN1_SEQUENCE,
    LTC_ASN1_SET,
    LTC_ASN1_SETOF
};

/** A LTC ASN.1 list type */
typedef struct ltc_asn1_list_ {
    /** The LTC ASN.1 enumerated type identifier */
    int                   type;
    /** The data to encode or place for decoding */
    void                  *data;
    /** The size of the input or resulting output */
    unsigned long         size;
    /** The used flag, this is used by the CHOICE ASN.1 type to indicate which choice was made */
    int                   used;
    /** prev/next entry in the list */
    struct ltc_asn1_list_ *prev, *next, *child, *parent;
} ltc_asn1_list;

 #define LTC_SET_ASN1(list, index, Type, Data, Size)          \
    do {                                                      \
        int           LTC_MACRO_temp  = (index);              \
        ltc_asn1_list *LTC_MACRO_list = (list);               \
        LTC_MACRO_list[LTC_MACRO_temp].type = (Type);         \
        LTC_MACRO_list[LTC_MACRO_temp].data = (void *)(Data); \
        LTC_MACRO_list[LTC_MACRO_temp].size = (Size);         \
        LTC_MACRO_list[LTC_MACRO_temp].used = 0;              \
    } while (0);

/* SEQUENCE */
int der_encode_sequence_ex(ltc_asn1_list *list, unsigned long inlen,
                           unsigned char *out, unsigned long *outlen, int type_of);

 #define der_encode_sequence(list, inlen, out, outlen)    der_encode_sequence_ex(list, inlen, out, outlen, LTC_ASN1_SEQUENCE)

int der_decode_sequence_ex(const unsigned char *in, unsigned long inlen,
                           ltc_asn1_list *list, unsigned long outlen, int ordered);

 #define der_decode_sequence(in, inlen, list, outlen)    der_decode_sequence_ex(in, inlen, list, outlen, 1)

int der_length_sequence(ltc_asn1_list *list, unsigned long inlen,
                        unsigned long *outlen);

/* SET */
 #define der_decode_set(in, inlen, list, outlen)    der_decode_sequence_ex(in, inlen, list, outlen, 0)
 #define der_length_set    der_length_sequence
int der_encode_set(ltc_asn1_list *list, unsigned long inlen,
                   unsigned char *out, unsigned long *outlen);

int der_encode_setof(ltc_asn1_list *list, unsigned long inlen,
                     unsigned char *out, unsigned long *outlen);

/* VA list handy helpers with triplets of <type, size, data> */
int der_encode_sequence_multi(unsigned char *out, unsigned long *outlen, ...);
int der_decode_sequence_multi(const unsigned char *in, unsigned long inlen, ...);

/* FLEXI DECODER handle unknown list decoder */
int  der_decode_sequence_flexi(const unsigned char *in, unsigned long *inlen, ltc_asn1_list **out);
void der_free_sequence_flexi(ltc_asn1_list *list);
void der_sequence_free(ltc_asn1_list *in);

/* BOOLEAN */
int der_length_boolean(unsigned long *outlen);
int der_encode_boolean(int in,
                       unsigned char *out, unsigned long *outlen);
int der_decode_boolean(const unsigned char *in, unsigned long inlen,
                       int *out);

/* INTEGER */
int der_encode_integer(void *num, unsigned char *out, unsigned long *outlen);
int der_decode_integer(const unsigned char *in, unsigned long inlen, void *num);
int der_length_integer(void *num, unsigned long *len);

/* INTEGER -- handy for 0..2^32-1 values */
int der_decode_short_integer(const unsigned char *in, unsigned long inlen, unsigned long *num);
int der_encode_short_integer(unsigned long num, unsigned char *out, unsigned long *outlen);
int der_length_short_integer(unsigned long num, unsigned long *outlen);

/* BIT STRING */
int der_encode_bit_string(const unsigned char *in, unsigned long inlen,
                          unsigned char *out, unsigned long *outlen);
int der_decode_bit_string(const unsigned char *in, unsigned long inlen,
                          unsigned char *out, unsigned long *outlen);
int der_length_bit_string(unsigned long nbits, unsigned long *outlen);

/* OCTET STRING */
int der_encode_octet_string(const unsigned char *in, unsigned long inlen,
                            unsigned char *out, unsigned long *outlen);
int der_decode_octet_string(const unsigned char *in, unsigned long inlen,
                            unsigned char *out, unsigned long *outlen);
int der_length_octet_string(unsigned long noctets, unsigned long *outlen);

/* OBJECT IDENTIFIER */
int der_encode_object_identifier(unsigned long *words, unsigned long nwords,
                                 unsigned char *out, unsigned long *outlen);
int der_decode_object_identifier(const unsigned char *in, unsigned long inlen,
                                 unsigned long *words, unsigned long *outlen);
int der_length_object_identifier(unsigned long *words, unsigned long nwords, unsigned long *outlen);
unsigned long der_object_identifier_bits(unsigned long x);

/* IA5 STRING */
int der_encode_ia5_string(const unsigned char *in, unsigned long inlen,
                          unsigned char *out, unsigned long *outlen);
int der_decode_ia5_string(const unsigned char *in, unsigned long inlen,
                          unsigned char *out, unsigned long *outlen);
int der_length_ia5_string(const unsigned char *octets, unsigned long noctets, unsigned long *outlen);

int der_ia5_char_encode(int c);
int der_ia5_value_decode(int v);

/* Printable STRING */
int der_encode_printable_string(const unsigned char *in, unsigned long inlen,
                                unsigned char *out, unsigned long *outlen);
int der_decode_printable_string(const unsigned char *in, unsigned long inlen,
                                unsigned char *out, unsigned long *outlen);
int der_length_printable_string(const unsigned char *octets, unsigned long noctets, unsigned long *outlen);

int der_printable_char_encode(int c);
int der_printable_value_decode(int v);

/* UTF-8 */
 #if (defined(SIZE_MAX) || __STDC_VERSION__ >= 199901L || defined(WCHAR_MAX) || defined(_WCHAR_T) || defined(_WCHAR_T_DEFINED) || defined (__WCHAR_TYPE__)) && !defined(LTC_NO_WCHAR)
  #include <wchar.h>
 #else
typedef ulong32   wchar_t;
 #endif

int der_encode_utf8_string(const wchar_t *in, unsigned long inlen,
                           unsigned char *out, unsigned long *outlen);

int der_decode_utf8_string(const unsigned char *in, unsigned long inlen,
                           wchar_t *out, unsigned long *outlen);
unsigned long der_utf8_charsize(const wchar_t c);
int der_length_utf8_string(const wchar_t *in, unsigned long noctets, unsigned long *outlen);


/* CHOICE */
int der_decode_choice(const unsigned char *in, unsigned long *inlen,
                      ltc_asn1_list *list, unsigned long outlen);

/* UTCTime */
typedef struct {
    unsigned YY,      /* year */
             MM,      /* month */
             DD,      /* day */
             hh,      /* hour */
             mm,      /* minute */
             ss,      /* second */
             off_dir, /* timezone offset direction 0 == +, 1 == - */
             off_hh,  /* timezone offset hours */
             off_mm;  /* timezone offset minutes */
} ltc_utctime;

int der_encode_utctime(ltc_utctime *utctime,
                       unsigned char *out, unsigned long *outlen);

int der_decode_utctime(const unsigned char *in, unsigned long *inlen,
                       ltc_utctime *out);

int der_length_utctime(ltc_utctime *utctime, unsigned long *outlen);
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/headers/tomcrypt_pk.h,v $ */
/* $Revision: 1.81 $ */
/* $Date: 2007/05/12 14:32:35 $ */

/** math functions **/
#define LTC_SOURCE
#define LTC_MP_LT     -1
#define LTC_MP_EQ     0
#define LTC_MP_GT     1

#define LTC_MP_NO     0
#define LTC_MP_YES    1

#ifndef LTC_MECC
typedef void   ecc_point;
#endif

#ifndef LTC_MRSA
typedef void   rsa_key;
#endif

/** math descriptor */
typedef struct {
    /** Name of the math provider */
    char *name;

    /** Bits per digit, amount of bits must fit in an unsigned long */
    int  bits_per_digit;

/* ---- init/deinit functions ---- */

    /** initialize a bignum
       @param   a     The number to initialize
       @return  CRYPT_OK on success
     */
    int (*init)(void **a);

    /** init copy
       @param  dst    The number to initialize and write to
       @param  src    The number to copy from
       @return CRYPT_OK on success
     */
    int (*init_copy)(void **dst, void *src);

    /** deinit
       @param   a    The number to free
       @return CRYPT_OK on success
     */
    void (*deinit)(void *a);

/* ---- data movement ---- */

    /** negate
       @param   src   The number to negate
       @param   dst   The destination
       @return CRYPT_OK on success
     */
    int (*neg)(void *src, void *dst);

    /** copy
       @param   src   The number to copy from
       @param   dst   The number to write to
       @return CRYPT_OK on success
     */
    int (*copy)(void *src, void *dst);

/* ---- trivial low level functions ---- */

    /** set small constant
       @param a    Number to write to
       @param n    Source upto bits_per_digit (actually meant for very small constants)
       @return CRYPT_OK on succcess
     */
    int (*set_int)(void *a, unsigned long n);

    /** get small constant
       @param a    Number to read, only fetches upto bits_per_digit from the number
       @return  The lower bits_per_digit of the integer (unsigned)
     */
    unsigned long (*get_int)(void *a);

    /** get digit n
       @param a  The number to read from
       @param n  The number of the digit to fetch
       @return  The bits_per_digit  sized n'th digit of a
     */
    unsigned long (*get_digit)(void *a, int n);

    /** Get the number of digits that represent the number
       @param a   The number to count
       @return The number of digits used to represent the number
     */
    int (*get_digit_count)(void *a);

    /** compare two integers
       @param a   The left side integer
       @param b   The right side integer
       @return LTC_MP_LT if a < b, LTC_MP_GT if a > b and LTC_MP_EQ otherwise.  (signed comparison)
     */
    int (*compare)(void *a, void *b);

    /** compare against int
       @param a   The left side integer
       @param b   The right side integer (upto bits_per_digit)
       @return LTC_MP_LT if a < b, LTC_MP_GT if a > b and LTC_MP_EQ otherwise.  (signed comparison)
     */
    int (*compare_d)(void *a, unsigned long n);

    /** Count the number of bits used to represent the integer
       @param a   The integer to count
       @return The number of bits required to represent the integer
     */
    int (*count_bits)(void *a);

    /** Count the number of LSB bits which are zero
       @param a   The integer to count
       @return The number of contiguous zero LSB bits
     */
    int (*count_lsb_bits)(void *a);

    /** Compute a power of two
       @param a  The integer to store the power in
       @param n  The power of two you want to store (a = 2^n)
       @return CRYPT_OK on success
     */
    int (*twoexpt)(void *a, int n);

/* ---- radix conversions ---- */

    /** read ascii string
       @param a     The integer to store into
       @param str   The string to read
       @param radix The radix the integer has been represented in (2-64)
       @return CRYPT_OK on success
     */
    int (*read_radix)(void *a, const char *str, int radix);

    /** write number to string
       @param a     The integer to store
       @param str   The destination for the string
       @param radix The radix the integer is to be represented in (2-64)
       @return CRYPT_OK on success
     */
    int (*write_radix)(void *a, char *str, int radix);

    /** get size as unsigned char string
       @param a     The integer to get the size (when stored in array of octets)
       @return The length of the integer
     */
    unsigned long (*unsigned_size)(void *a);

    /** store an integer as an array of octets
       @param src   The integer to store
       @param dst   The buffer to store the integer in
       @return CRYPT_OK on success
     */
    int (*unsigned_write)(void *src, unsigned char *dst);

    /** read an array of octets and store as integer
       @param dst   The integer to load
       @param src   The array of octets
       @param len   The number of octets
       @return CRYPT_OK on success
     */
    int (*unsigned_read)(void *dst, unsigned char *src, unsigned long len);

/* ---- basic math ---- */

    /** add two integers
       @param a   The first source integer
       @param b   The second source integer
       @param c   The destination of "a + b"
       @return CRYPT_OK on success
     */
    int (*add)(void *a, void *b, void *c);


    /** add two integers
       @param a   The first source integer
       @param b   The second source integer (single digit of upto bits_per_digit in length)
       @param c   The destination of "a + b"
       @return CRYPT_OK on success
     */
    int (*addi)(void *a, unsigned long b, void *c);

    /** subtract two integers
       @param a   The first source integer
       @param b   The second source integer
       @param c   The destination of "a - b"
       @return CRYPT_OK on success
     */
    int (*sub)(void *a, void *b, void *c);

    /** subtract two integers
       @param a   The first source integer
       @param b   The second source integer (single digit of upto bits_per_digit in length)
       @param c   The destination of "a - b"
       @return CRYPT_OK on success
     */
    int (*subi)(void *a, unsigned long b, void *c);

    /** multiply two integers
       @param a   The first source integer
       @param b   The second source integer (single digit of upto bits_per_digit in length)
       @param c   The destination of "a * b"
       @return CRYPT_OK on success
     */
    int (*mul)(void *a, void *b, void *c);

    /** multiply two integers
       @param a   The first source integer
       @param b   The second source integer (single digit of upto bits_per_digit in length)
       @param c   The destination of "a * b"
       @return CRYPT_OK on success
     */
    int (*muli)(void *a, unsigned long b, void *c);

    /** Square an integer
       @param a    The integer to square
       @param b    The destination
       @return CRYPT_OK on success
     */
    int (*sqr)(void *a, void *b);

    /** Divide an integer
       @param a    The dividend
       @param b    The divisor
       @param c    The quotient (can be NULL to signify don't care)
       @param d    The remainder (can be NULL to signify don't care)
       @return CRYPT_OK on success
     */
    int (*mpdiv)(void *a, void *b, void *c, void *d);

    /** divide by two
       @param  a   The integer to divide (shift right)
       @param  b   The destination
       @return CRYPT_OK on success
     */
    int (*div_2)(void *a, void *b);

    /** Get remainder (small value)
       @param  a    The integer to reduce
       @param  b    The modulus (upto bits_per_digit in length)
       @param  c    The destination for the residue
       @return CRYPT_OK on success
     */
    int (*modi)(void *a, unsigned long b, unsigned long *c);

    /** gcd
       @param  a     The first integer
       @param  b     The second integer
       @param  c     The destination for (a, b)
       @return CRYPT_OK on success
     */
    int (*gcd)(void *a, void *b, void *c);

    /** lcm
       @param  a     The first integer
       @param  b     The second integer
       @param  c     The destination for [a, b]
       @return CRYPT_OK on success
     */
    int (*lcm)(void *a, void *b, void *c);

    /** Modular multiplication
       @param  a     The first source
       @param  b     The second source
       @param  c     The modulus
       @param  d     The destination (a*b mod c)
       @return CRYPT_OK on success
     */
    int (*mulmod)(void *a, void *b, void *c, void *d);

    /** Modular squaring
       @param  a     The first source
       @param  b     The modulus
       @param  c     The destination (a*a mod b)
       @return CRYPT_OK on success
     */
    int (*sqrmod)(void *a, void *b, void *c);

    /** Modular inversion
       @param  a     The value to invert
       @param  b     The modulus
       @param  c     The destination (1/a mod b)
       @return CRYPT_OK on success
     */
    int (*invmod)(void *, void *, void *);

/* ---- reduction ---- */

    /** setup montgomery
        @param a  The modulus
        @param b  The destination for the reduction digit
        @return CRYPT_OK on success
     */
    int (*montgomery_setup)(void *a, void **b);

    /** get normalization value
        @param a   The destination for the normalization value
        @param b   The modulus
        @return  CRYPT_OK on success
     */
    int (*montgomery_normalization)(void *a, void *b);

    /** reduce a number
        @param a   The number [and dest] to reduce
        @param b   The modulus
        @param c   The value "b" from montgomery_setup()
        @return CRYPT_OK on success
     */
    int (*montgomery_reduce)(void *a, void *b, void *c);

    /** clean up  (frees memory)
        @param a   The value "b" from montgomery_setup()
        @return CRYPT_OK on success
     */
    void (*montgomery_deinit)(void *a);

/* ---- exponentiation ---- */

    /** Modular exponentiation
        @param a    The base integer
        @param b    The power (can be negative) integer
        @param c    The modulus integer
        @param d    The destination
        @return CRYPT_OK on success
     */
    int (*exptmod)(void *a, void *b, void *c, void *d);

    /** Primality testing
        @param a     The integer to test
        @param b     The destination of the result (FP_YES if prime)
        @return CRYPT_OK on success
     */
    int (*isprime)(void *a, int *b);

/* ----  (optional) ecc point math ---- */

    /** ECC GF(p) point multiplication (from the NIST curves)
        @param k   The integer to multiply the point by
        @param G   The point to multiply
        @param R   The destination for kG
        @param modulus  The modulus for the field
        @param map Boolean indicated whether to map back to affine or not (can be ignored if you work in affine only)
        @return CRYPT_OK on success
     */
    int (*ecc_ptmul)(void *k, ecc_point *G, ecc_point *R, void *modulus, int map);

    /** ECC GF(p) point addition
        @param P    The first point
        @param Q    The second point
        @param R    The destination of P + Q
        @param modulus  The modulus
        @param mp   The "b" value from montgomery_setup()
        @return CRYPT_OK on success
     */
    int (*ecc_ptadd)(ecc_point *P, ecc_point *Q, ecc_point *R, void *modulus, void *mp);

    /** ECC GF(p) point double
        @param P    The first point
        @param R    The destination of 2P
        @param modulus  The modulus
        @param mp   The "b" value from montgomery_setup()
        @return CRYPT_OK on success
     */
    int (*ecc_ptdbl)(ecc_point *P, ecc_point *R, void *modulus, void *mp);

    /** ECC mapping from projective to affine, currently uses (x,y,z) => (x/z^2, y/z^3, 1)
        @param P     The point to map
        @param modulus The modulus
        @param mp    The "b" value from montgomery_setup()
        @return CRYPT_OK on success
        @remark  The mapping can be different but keep in mind a ecc_point only has three
                 integers (x,y,z) so if you use a different mapping you have to make it fit.
     */
    int (*ecc_map)(ecc_point *P, void *modulus, void *mp);

    /** Computes kA*A + kB*B = C using Shamir's Trick
        @param A        First point to multiply
        @param kA       What to multiple A by
        @param B        Second point to multiply
        @param kB       What to multiple B by
        @param C        [out] Destination point (can overlap with A or B
        @param modulus  Modulus for curve
        @return CRYPT_OK on success
     */
    int (*ecc_mul2add)(ecc_point *A, void *kA,
                       ecc_point *B, void *kB,
                       ecc_point *C,
                       void *modulus);

/* ---- (optional) rsa optimized math (for internal CRT) ---- */

    /** RSA Key Generation
        @param prng     An active PRNG state
        @param wprng    The index of the PRNG desired
        @param size     The size of the modulus (key size) desired (octets)
        @param e        The "e" value (public key).  e==65537 is a good choice
        @param key      [out] Destination of a newly created private key pair
        @return CRYPT_OK if successful, upon error all allocated ram is freed
     */
    int (*rsa_keygen)(prng_state *prng, int wprng, int size, long e, rsa_key *key);


    /** RSA exponentiation
       @param in       The octet array representing the base
       @param inlen    The length of the input
       @param out      The destination (to be stored in an octet array format)
       @param outlen   The length of the output buffer and the resulting size (zero padded to the size of the modulus)
       @param which    PK_PUBLIC for public RSA and PK_PRIVATE for private RSA
       @param key      The RSA key to use
       @return CRYPT_OK on success
     */
    int (*rsa_me)(const unsigned char *in, unsigned long inlen,
                  unsigned char *out, unsigned long *outlen, int which,
                  rsa_key *key);
} ltc_math_descriptor;

extern ltc_math_descriptor ltc_mp;

int ltc_init_multi(void **a, ...);
void ltc_deinit_multi(void *a, ...);

#ifdef LTM_DESC
extern const ltc_math_descriptor ltm_desc;
#endif

#ifdef TFM_DESC
extern const ltc_math_descriptor tfm_desc;
#endif

#ifdef GMP_DESC
extern const ltc_math_descriptor gmp_desc;
#endif

#if !defined(DESC_DEF_ONLY) && defined(LTC_SOURCE)
 #undef MP_DIGIT_BIT
 #undef mp_iszero
 #undef mp_isodd
 #undef mp_tohex

 #define MP_DIGIT_BIT    ltc_mp.bits_per_digit

/* some handy macros */
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

/* $Source: /cvs/libtom/libtomcrypt/src/headers/tomcrypt_math.h,v $ */
/* $Revision: 1.44 $ */
/* $Date: 2007/05/12 14:32:35 $ */

/* ---- LTC_BASE64 Routines ---- */
#ifdef LTC_BASE64
int base64_encode(const unsigned char *in, unsigned long len,
                  unsigned char *out, unsigned long *outlen);

int base64_decode(const unsigned char *in, unsigned long len,
                  unsigned char *out, unsigned long *outlen);
#endif

/* ---- MEM routines ---- */
void zeromem(void *dst, size_t len);
void burn_stack(unsigned long len);

const char *error_to_string(int err);

extern const char *crypt_build_settings;

/* ---- HMM ---- */
int crypt_fsa(void *mp, ...);

/* $Source: /cvs/libtom/libtomcrypt/src/headers/tomcrypt_misc.h,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2007/05/12 14:32:35 $ */

/* Defines the LTC_ARGCHK macro used within the library */
/* ARGTYPE is defined in mycrypt_cfg.h */
#if ARGTYPE == 0

 #include <signal.h>

/* this is the default LibTomCrypt macro  */
void crypt_argchk(char *v, char *s, int d);

 #define LTC_ARGCHK(x)      if (!(x)) { crypt_argchk(#x, __FILE__, __LINE__); }
 #define LTC_ARGCHKVD(x)    LTC_ARGCHK(x)

#elif ARGTYPE == 1

/* fatal type of error */
 #define LTC_ARGCHK(x)      assert((x))
 #define LTC_ARGCHKVD(x)    LTC_ARGCHK(x)

#elif ARGTYPE == 2

 #define LTC_ARGCHK(x)      if (!(x)) { fprintf(stderr, "\nwarning: ARGCHK failed at %s:%d\n", __FILE__, __LINE__); }
 #define LTC_ARGCHKVD(x)    LTC_ARGCHK(x)

#elif ARGTYPE == 3

 #define LTC_ARGCHK(x)
 #define LTC_ARGCHKVD(x)    LTC_ARGCHK(x)

#elif ARGTYPE == 4

 #define LTC_ARGCHK(x)      if (!(x)) return CRYPT_INVALID_ARG;
 #define LTC_ARGCHKVD(x)    if (!(x)) return;
#endif


/* $Source: /cvs/libtom/libtomcrypt/src/headers/tomcrypt_argchk.h,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/08/27 20:50:21 $ */

/* LTC_PKCS Header Info */

/* ===> LTC_PKCS #1 -- RSA Cryptography <=== */
#ifdef LTC_PKCS_1

enum ltc_pkcs_1_v1_5_blocks {
    LTC_LTC_PKCS_1_EMSA = 1,        /* Block type 1 (LTC_PKCS #1 v1.5 signature padding) */
    LTC_LTC_PKCS_1_EME  = 2         /* Block type 2 (LTC_PKCS #1 v1.5 encryption padding) */
};

enum ltc_pkcs_1_paddings {
    LTC_LTC_PKCS_1_V1_5 = 1,        /* LTC_PKCS #1 v1.5 padding (\sa ltc_pkcs_1_v1_5_blocks) */
    LTC_LTC_PKCS_1_OAEP = 2,        /* LTC_PKCS #1 v2.0 encryption padding */
    LTC_LTC_PKCS_1_PSS  = 3         /* LTC_PKCS #1 v2.1 signature padding */
};

int pkcs_1_mgf1(int hash_idx,
                const unsigned char *seed, unsigned long seedlen,
                unsigned char *mask, unsigned long masklen);

int pkcs_1_i2osp(void *n, unsigned long modulus_len, unsigned char *out);
int pkcs_1_os2ip(void *n, unsigned char *in, unsigned long inlen);

/* *** v1.5 padding */
int pkcs_1_v1_5_encode(const unsigned char *msg,
                       unsigned long       msglen,
                       int                 block_type,
                       unsigned long       modulus_bitlen,
                       prng_state          *prng,
                       int                 prng_idx,
                       unsigned char       *out,
                       unsigned long       *outlen);

int pkcs_1_v1_5_decode(const unsigned char *msg,
                       unsigned long       msglen,
                       int                 block_type,
                       unsigned long       modulus_bitlen,
                       unsigned char       *out,
                       unsigned long       *outlen,
                       int                 *is_valid);

/* *** v2.1 padding */
int pkcs_1_oaep_encode(const unsigned char *msg, unsigned long msglen,
                       const unsigned char *lparam, unsigned long lparamlen,
                       unsigned long modulus_bitlen, prng_state *prng,
                       int prng_idx, int hash_idx,
                       unsigned char *out, unsigned long *outlen);

int pkcs_1_oaep_decode(const unsigned char *msg, unsigned long msglen,
                       const unsigned char *lparam, unsigned long lparamlen,
                       unsigned long modulus_bitlen, int hash_idx,
                       unsigned char *out, unsigned long *outlen,
                       int *res);

int pkcs_1_pss_encode(const unsigned char *msghash, unsigned long msghashlen,
                      unsigned long saltlen, prng_state *prng,
                      int prng_idx, int hash_idx,
                      unsigned long modulus_bitlen,
                      unsigned char *out, unsigned long *outlen);

int pkcs_1_pss_decode(const unsigned char *msghash, unsigned long msghashlen,
                      const unsigned char *sig, unsigned long siglen,
                      unsigned long saltlen, int hash_idx,
                      unsigned long modulus_bitlen, int *res);
#endif /* LTC_PKCS_1 */

/* ===> LTC_PKCS #5 -- Password Based Cryptography <=== */
#ifdef LTC_PKCS_5

/* Algorithm #1 (old) */
int pkcs_5_alg1(const unsigned char *password, unsigned long password_len,
                const unsigned char *salt,
                int iteration_count, int hash_idx,
                unsigned char *out, unsigned long *outlen);

/* Algorithm #2 (new) */
int pkcs_5_alg2(const unsigned char *password, unsigned long password_len,
                const unsigned char *salt, unsigned long salt_len,
                int iteration_count, int hash_idx,
                unsigned char *out, unsigned long *outlen);
#endif  /* LTC_PKCS_5 */

/* $Source: /cvs/libtom/libtomcrypt/src/headers/tomcrypt_pkcs.h,v $ */
/* $Revision: 1.8 $ */
/* $Date: 2007/05/12 14:32:35 $ */

#ifdef __cplusplus
}
#endif
#endif /* TOMCRYPT_H_ */
