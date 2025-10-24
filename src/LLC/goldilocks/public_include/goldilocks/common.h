/**
 * @file goldilocks/common.h
 * @author Mike Hamburg
 *
 * @copyright
 *   Copyright (c) 2015 Cryptography Research, Inc.  \n
 *   Copyright (c) 2018 the libgoldilocks contributors.  \n
 *   Released under the MIT License.  See LICENSE.txt for license information.
 *
 * @brief Common utility headers for Goldilocks library.
 */

#ifndef __GOLDILOCKS_COMMON_H__
#define __GOLDILOCKS_COMMON_H__ 1

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Goldilocks' build flags default to hidden and stripping executables. */
/** @cond internal */
#if (defined(DOXYGEN) && DOXYGEN) || defined(__attribute__)
#define __attribute__(x)
#define NOINLINE
#endif
#define GOLDILOCKS_API_VIS __attribute__((visibility("default")))
#define GOLDILOCKS_NOINLINE  __attribute__((noinline))
#define GOLDILOCKS_WARN_UNUSED __attribute__((warn_unused_result))
#define GOLDILOCKS_NONNULL __attribute__((nonnull))
#define GOLDILOCKS_INLINE inline __attribute__((always_inline,unused))
// Cribbed from libnotmuch
#if defined (__clang_major__) && __clang_major__ >= 3 \
    || defined (__GNUC__) && __GNUC__ >= 5 \
    || defined (__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ >= 5
#define GOLDILOCKS_DEPRECATED(msg) __attribute__ ((deprecated(msg)))
#else
#define GOLDILOCKS_DEPRECATED(msg) __attribute__ ((deprecated))
#endif
/** @endcond */

/* Internal word types.
 *
 * Somewhat tricky.  This could be decided separately per platform.  However,
 * the structs do need to be all the same size and alignment on a given
 * platform to support dynamic linking, since even if you header was built
 * with eg arch_neon, you might end up linking a library built with arch_arm32.
 */
#ifndef GOLDILOCKS_WORD_BITS
    #if (defined(__ILP64__) || defined(__amd64__) || defined(__x86_64__) || (((__UINT_FAST32_MAX__)>>30)>>30))
        #define GOLDILOCKS_WORD_BITS 64 /**< The number of bits in a word */
    #else
        #define GOLDILOCKS_WORD_BITS 32 /**< The number of bits in a word */
    #endif
#endif

#if GOLDILOCKS_WORD_BITS == 64
typedef uint64_t goldilocks_word_t;      /**< Word size for internal computations */
typedef int64_t goldilocks_sword_t;      /**< Signed word size for internal computations */
typedef uint64_t goldilocks_bool_t;      /**< "Boolean" type, will be set to all-zero or all-one (i.e. -1u) */
typedef __uint128_t goldilocks_dword_t;  /**< Double-word size for internal computations */
typedef __int128_t goldilocks_dsword_t;  /**< Signed double-word size for internal computations */
#elif GOLDILOCKS_WORD_BITS == 32         /**< The number of bits in a word */
typedef uint32_t goldilocks_word_t;      /**< Word size for internal computations */
typedef int32_t goldilocks_sword_t;      /**< Signed word size for internal computations */
typedef uint32_t goldilocks_bool_t;      /**< "Boolean" type, will be set to all-zero or all-one (i.e. -1u) */
typedef uint64_t goldilocks_dword_t;     /**< Double-word size for internal computations */
typedef int64_t goldilocks_dsword_t;     /**< Signed double-word size for internal computations */
#else
#error "Only supporting GOLDILOCKS_WORD_BITS = 32 or 64 for now"
#endif

/** GOLDILOCKS_TRUE = -1 so that GOLDILOCKS_TRUE & x = x */
static const goldilocks_bool_t GOLDILOCKS_TRUE = -(goldilocks_bool_t)1;

/** GOLDILOCKS_FALSE = 0 so that GOLDILOCKS_FALSE & x = 0 */
static const goldilocks_bool_t GOLDILOCKS_FALSE = 0;

/** Another boolean type used to indicate success or failure. */
typedef enum {
    GOLDILOCKS_SUCCESS = -1, /**< The operation succeeded. */
    GOLDILOCKS_FAILURE = 0   /**< The operation failed. */
} goldilocks_error_t;


/** Return success if x is true */
static GOLDILOCKS_INLINE goldilocks_error_t
goldilocks_succeed_if(goldilocks_bool_t x) {
    return (goldilocks_error_t)x;
}

/** Return GOLDILOCKS_TRUE iff x == GOLDILOCKS_SUCCESS */
static GOLDILOCKS_INLINE goldilocks_bool_t
goldilocks_successful(goldilocks_error_t e) {
    goldilocks_word_t succ = GOLDILOCKS_SUCCESS;
    goldilocks_dword_t w = ((goldilocks_word_t)e) ^  succ;
    return (w-1)>>GOLDILOCKS_WORD_BITS;
}

/** Overwrite data with zeros.  Uses memset_s if available. */
void goldilocks_bzero (
    void *data,
    size_t size
) GOLDILOCKS_NONNULL GOLDILOCKS_API_VIS;

/** Compare two buffers, returning GOLDILOCKS_TRUE if they are equal. */
goldilocks_bool_t goldilocks_memeq (
    const void *data1,
    const void *data2,
    size_t size
) GOLDILOCKS_NONNULL GOLDILOCKS_WARN_UNUSED GOLDILOCKS_API_VIS;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __GOLDILOCKS_COMMON_H__ */
