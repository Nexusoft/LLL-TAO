/**
 * @file goldilocks/spongerng.h
 * @copyright
 *   Copyright (c) 2015-2016 Cryptography Research, Inc.  \n
 *   Copyright (c) 2018 the libgoldilocks contributors.  \n
 *   Released under the MIT License.  See LICENSE.txt for license information.
 * @author Mike Hamburg
 * @brief Sponge-based RNGs.
 * @warning This construction isn't final.  In particular,
 * the outputs of deterministic RNGs from this mechanism might change in future versions.
 */

#ifndef __GOLDILOCKS_SPONGERNG_H__
#define __GOLDILOCKS_SPONGERNG_H__

#include <goldilocks/shake.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Keccak CSPRNG structure as struct. */
typedef struct {
    goldilocks_keccak_sponge_p sponge;  /**< Internal sponge object. */
} goldilocks_keccak_prng_s;

/** Keccak CSPRNG structure as one-element array */
typedef goldilocks_keccak_prng_s goldilocks_keccak_prng_p[1];

/** Initialize a sponge-based CSPRNG from a buffer. */
void goldilocks_spongerng_init_from_buffer (
    goldilocks_keccak_prng_p prng,             /**< [out] The PRNG object. */
    const uint8_t *__restrict__ in, /**< [in]  The initialization data. */
    size_t len,                     /**< [in]  The length of the initialization data. */
    int deterministic               /**< [in]  If zero, allow RNG to stir in nondeterministic data from RDRAND or RDTSC.*/
) GOLDILOCKS_NONNULL GOLDILOCKS_API_VIS;

/**
 * @brief Initialize a sponge-based CSPRNG from a file.
 * @retval GOLDILOCKS_SUCCESS success.
 * @retval GOLDILOCKS_FAILURE failure.
 * @note On failure, errno can be used to determine the cause.
 */
goldilocks_error_t goldilocks_spongerng_init_from_file (
    goldilocks_keccak_prng_p prng, /**< [out] The PRNG object. */
    const char *file,   /**< [in]  A name of a file containing initial data. */
    size_t len,         /**< [in]  The length of the initial data.  Must be positive. */
    int deterministic   /**< [in]  If zero, allow RNG to stir in nondeterministic data from RDRAND or RDTSC. */
) GOLDILOCKS_NONNULL GOLDILOCKS_API_VIS GOLDILOCKS_WARN_UNUSED;

/**
 * @brief Initialize a nondeterministic sponge-based CSPRNG from /dev/urandom.
 * @retval GOLDILOCKS_SUCCESS success.
 * @retval GOLDILOCKS_FAILURE failure.
 * @note On failure, errno can be used to determine the cause.
 */
goldilocks_error_t goldilocks_spongerng_init_from_dev_urandom (
    goldilocks_keccak_prng_p prng /**< [out] sponge The sponge object. */
) GOLDILOCKS_API_VIS GOLDILOCKS_WARN_UNUSED;

/** Output bytes from a sponge-based CSPRNG. */
void goldilocks_spongerng_next (
    goldilocks_keccak_prng_p prng,         /**< [inout] The PRNG object. */
    uint8_t * __restrict__ out, /**< [out]   Output buffer. */
    size_t len                  /**< [in]    Number of bytes to output. */
) GOLDILOCKS_API_VIS;

/** Stir entropy data into a sponge-based CSPRNG from a buffer.  */
void goldilocks_spongerng_stir (
    goldilocks_keccak_prng_p prng,              /**< [out] The PRNG object. */
    const uint8_t * __restrict__ in, /**< [in]  The entropy data. */
    size_t len                       /**< [in]  The length of the initial data. */
) GOLDILOCKS_NONNULL GOLDILOCKS_API_VIS;

/** Securely destroy a sponge RNG object by overwriting it. */
static GOLDILOCKS_INLINE void
goldilocks_spongerng_destroy (
    goldilocks_keccak_prng_p doomed /**< [in] The object to destroy. */
);

/** @cond internal */
/***************************************/
/* Implementations of inline functions */
/***************************************/
void goldilocks_spongerng_destroy (goldilocks_keccak_prng_p doomed) {
    goldilocks_sha3_destroy(doomed->sponge);
}
/** @endcond */ /* internal */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __GOLDILOCKS_SPONGERNG_H__ */
