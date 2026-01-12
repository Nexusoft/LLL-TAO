/**
 * @file goldilocks/shake.h
 * @copyright
 *   Based on CC0 code by David Leon Gil, 2015 \n
 *   Copyright (c) 2015 Cryptography Research, Inc.  \n
 *   Copyright (c) 2018 the libgoldilocks contributors.  \n
 *   Released under the MIT License.  See LICENSE.txt for license information.
 * @author Mike Hamburg
 * @brief SHA-3-n and GOLDILOCKS_SHAKE-n instances.
 */

#ifndef __GOLDILOCKS_SHAKE_H__
#define __GOLDILOCKS_SHAKE_H__

#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h> /* for NULL */

#include <goldilocks/common.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef INTERNAL_SPONGE_STRUCT
    /** Sponge container object for the various primitives. */
    typedef struct goldilocks_keccak_sponge_s {
        /** @cond internal */
        uint64_t opaque[26];
        /** @endcond */
    } goldilocks_keccak_sponge_s;

    /** Convenience GMP-style one-element array version */
    typedef struct goldilocks_keccak_sponge_s goldilocks_keccak_sponge_p[1];

    /** Parameters for sponge construction, distinguishing GOLDILOCKS_SHA3 and
     * GOLDILOCKS_SHAKE instances.
     */
    struct goldilocks_kparams_s;
#endif

/**
 * @brief Initialize a sponge context object.
 * @param [out] sponge The object to initialize.
 * @param [in] params The sponge's parameter description.
 */
void goldilocks_sha3_init (
    goldilocks_keccak_sponge_p sponge,
    const struct goldilocks_kparams_s *params
) GOLDILOCKS_API_VIS;

/**
 * @brief Absorb data into a GOLDILOCKS_SHA3 or GOLDILOCKS_SHAKE hash context.
 * @param [inout] sponge The context.
 * @param [in] in The input data.
 * @param [in] len The input data's length in bytes.
 * @return GOLDILOCKS_FAILURE if the sponge has already been used for output.
 * @return GOLDILOCKS_SUCCESS otherwise.
 */
goldilocks_error_t goldilocks_sha3_update (
    struct goldilocks_keccak_sponge_s * __restrict__ sponge,
    const uint8_t *in,
    size_t len
) GOLDILOCKS_API_VIS;

/**
 * @brief Squeeze output data from a GOLDILOCKS_SHA3 or GOLDILOCKS_SHAKE hash context.
 * This does not destroy or re-initialize the hash context, and
 * goldilocks_sha3 output can be called more times.
 *
 * @param [inout] sponge The context.
 * @param [out] out The output data.
 * @param [in] len The requested output data length in bytes.
 * @return GOLDILOCKS_FAILURE if the sponge has exhausted its output capacity.
 * @return GOLDILOCKS_SUCCESS otherwise.
 */
goldilocks_error_t goldilocks_sha3_output (
    goldilocks_keccak_sponge_p sponge,
    uint8_t * __restrict__ out,
    size_t len
) GOLDILOCKS_API_VIS;

/**
 * @brief Squeeze output data from a GOLDILOCKS_SHA3 or GOLDILOCKS_SHAKE hash context.
 * This re-initializes the context to its starting parameters.
 *
 * @param [inout] sponge The context.
 * @param [out] out The output data.
 * @param [in] len The requested output data length in bytes.
 */
goldilocks_error_t goldilocks_sha3_final (
    goldilocks_keccak_sponge_p sponge,
    uint8_t * __restrict__ out,
    size_t len
) GOLDILOCKS_API_VIS;

/**
 * @brief Reset the sponge to the empty string.
 *
 * @param [inout] sponge The context.
 */
void goldilocks_sha3_reset (
    goldilocks_keccak_sponge_p sponge
) GOLDILOCKS_API_VIS;

/**
 * @brief Return the default output length of the sponge construction,
 * for the purpose of C++ default operators.
 *
 * Returns n/8 for GOLDILOCKS_SHA3-n and 2n/8 for GOLDILOCKS_SHAKE-n.
 */
size_t goldilocks_sha3_default_output_bytes (
    const goldilocks_keccak_sponge_p sponge /**< [inout] The context. */
) GOLDILOCKS_API_VIS;

/**
 * @brief Return the default output length of the sponge construction,
 * for the purpose of C++ default operators.
 *
 * Returns n/8 for GOLDILOCKS_SHA3-n and SIZE_MAX for GOLDILOCKS_SHAKE-n.
 */
size_t goldilocks_sha3_max_output_bytes (
    const goldilocks_keccak_sponge_p sponge /**< [inout] The context. */
) GOLDILOCKS_API_VIS;

/**
 * @brief Destroy a GOLDILOCKS_SHA3 or GOLDILOCKS_SHAKE sponge context by overwriting it with 0.
 * @param [out] sponge The context.
 */
void goldilocks_sha3_destroy (
    goldilocks_keccak_sponge_p sponge
) GOLDILOCKS_API_VIS;

/**
 * @brief Hash (in) to (out)
 * @param [in] in The input data.
 * @param [in] inlen The length of the input data.
 * @param [out] out A buffer for the output data.
 * @param [in] outlen The length of the output data.
 * @param [in] params The parameters of the sponge hash.
 */
goldilocks_error_t goldilocks_sha3_hash (
    uint8_t *out,
    size_t outlen,
    const uint8_t *in,
    size_t inlen,
    const struct goldilocks_kparams_s *params
) GOLDILOCKS_API_VIS;

/* FUTURE: expand/doxygenate individual GOLDILOCKS_SHAKE/GOLDILOCKS_SHA3 instances? */

/** @cond internal */
#define GOLDILOCKS_DEC_SHAKE(n) \
    extern const struct goldilocks_kparams_s GOLDILOCKS_SHAKE##n##_params_s GOLDILOCKS_API_VIS; \
    typedef struct goldilocks_shake##n##_ctx_s { goldilocks_keccak_sponge_p s; } goldilocks_shake##n##_ctx_p[1]; \
    static inline void GOLDILOCKS_NONNULL goldilocks_shake##n##_init(goldilocks_shake##n##_ctx_p sponge) { \
        goldilocks_sha3_init(sponge->s, &GOLDILOCKS_SHAKE##n##_params_s); \
    } \
    static inline void GOLDILOCKS_NONNULL goldilocks_shake##n##_gen_init(goldilocks_keccak_sponge_p sponge) { \
        goldilocks_sha3_init(sponge, &GOLDILOCKS_SHAKE##n##_params_s); \
    } \
    static inline goldilocks_error_t GOLDILOCKS_NONNULL goldilocks_shake##n##_update(goldilocks_shake##n##_ctx_p sponge, const uint8_t *in, size_t inlen ) { \
        return goldilocks_sha3_update(sponge->s, in, inlen); \
    } \
    static inline void  GOLDILOCKS_NONNULL goldilocks_shake##n##_final(goldilocks_shake##n##_ctx_p sponge, uint8_t *out, size_t outlen ) { \
        goldilocks_sha3_output(sponge->s, out, outlen); \
        goldilocks_sha3_init(sponge->s, &GOLDILOCKS_SHAKE##n##_params_s); \
    } \
    static inline void  GOLDILOCKS_NONNULL goldilocks_shake##n##_output(goldilocks_shake##n##_ctx_p sponge, uint8_t *out, size_t outlen ) { \
        goldilocks_sha3_output(sponge->s, out, outlen); \
    } \
    static inline void  GOLDILOCKS_NONNULL goldilocks_shake##n##_hash(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen) { \
        goldilocks_sha3_hash(out,outlen,in,inlen,&GOLDILOCKS_SHAKE##n##_params_s); \
    } \
    static inline void  GOLDILOCKS_NONNULL goldilocks_shake##n##_destroy(goldilocks_shake##n##_ctx_p sponge) { \
        goldilocks_sha3_destroy(sponge->s); \
    }

#define GOLDILOCKS_DEC_SHA3(n) \
    extern const struct goldilocks_kparams_s GOLDILOCKS_SHA3_##n##_params_s GOLDILOCKS_API_VIS; \
    typedef struct goldilocks_sha3_##n##_ctx_s { goldilocks_keccak_sponge_p s; } goldilocks_sha3_##n##_ctx_p[1]; \
    static inline void GOLDILOCKS_NONNULL goldilocks_sha3_##n##_init(goldilocks_sha3_##n##_ctx_p sponge) { \
        goldilocks_sha3_init(sponge->s, &GOLDILOCKS_SHA3_##n##_params_s); \
    } \
    static inline void GOLDILOCKS_NONNULL goldilocks_sha3_##n##_gen_init(goldilocks_keccak_sponge_p sponge) { \
        goldilocks_sha3_init(sponge, &GOLDILOCKS_SHA3_##n##_params_s); \
    } \
    static inline goldilocks_error_t GOLDILOCKS_NONNULL goldilocks_sha3_##n##_update(goldilocks_sha3_##n##_ctx_p sponge, const uint8_t *in, size_t inlen ) { \
        return goldilocks_sha3_update(sponge->s, in, inlen); \
    } \
    static inline goldilocks_error_t GOLDILOCKS_NONNULL goldilocks_sha3_##n##_final(goldilocks_sha3_##n##_ctx_p sponge, uint8_t *out, size_t outlen ) { \
        goldilocks_error_t ret = goldilocks_sha3_output(sponge->s, out, outlen); \
        goldilocks_sha3_init(sponge->s, &GOLDILOCKS_SHA3_##n##_params_s); \
        return ret; \
    } \
    static inline goldilocks_error_t GOLDILOCKS_NONNULL goldilocks_sha3_##n##_output(goldilocks_sha3_##n##_ctx_p sponge, uint8_t *out, size_t outlen ) { \
        return goldilocks_sha3_output(sponge->s, out, outlen); \
    } \
    static inline goldilocks_error_t GOLDILOCKS_NONNULL goldilocks_sha3_##n##_hash(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen) { \
        return goldilocks_sha3_hash(out,outlen,in,inlen,&GOLDILOCKS_SHA3_##n##_params_s); \
    } \
    static inline void GOLDILOCKS_NONNULL goldilocks_sha3_##n##_destroy(goldilocks_sha3_##n##_ctx_p sponge) { \
        goldilocks_sha3_destroy(sponge->s); \
    }
/** @endcond */

GOLDILOCKS_DEC_SHAKE(128)
GOLDILOCKS_DEC_SHAKE(256)
GOLDILOCKS_DEC_SHA3(224)
GOLDILOCKS_DEC_SHA3(256)
GOLDILOCKS_DEC_SHA3(384)
GOLDILOCKS_DEC_SHA3(512)
#undef GOLDILOCKS_DEC_SHAKE
#undef GOLDILOCKS_DEC_SHA3

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __GOLDILOCKS_SHAKE_H__ */
