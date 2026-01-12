/**
 * @file goldilocks/point_448.h
 * @author Mike Hamburg
 *
 * @copyright
 *   Copyright (c) 2015-2016 Cryptography Research, Inc.  \n
 *   Copyright (c) 2018 the libgoldilocks contributors.  \n
 *   Released under the MIT License.  See LICENSE.txt for license information.
 *
 * @brief A group of prime order p, based on Ed448-Goldilocks.
 */

#ifndef __GOLDILOCKS_POINT_448_H__
#define __GOLDILOCKS_POINT_448_H__ 1

#include <goldilocks/common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond internal */
#define GOLDILOCKS_448_SCALAR_LIMBS ((446-1)/GOLDILOCKS_WORD_BITS+1)
/** @endcond */

/** The number of bits in a scalar */
#define GOLDILOCKS_448_SCALAR_BITS 446

/** @cond internal */
#ifndef __GOLDILOCKS_448_GF_DEFINED__
#define __GOLDILOCKS_448_GF_DEFINED__ 1
/** @brief Galois field element internal structure */
typedef struct gf_448_s {
    goldilocks_word_t limb[512/GOLDILOCKS_WORD_BITS];
} __attribute__((aligned(32))) gf_448_s, gf_448_p[1];
#endif /* __GOLDILOCKS_448_GF_DEFINED__ */
/** @endcond */

/** Number of bytes in a serialized point. */
#define GOLDILOCKS_448_SER_BYTES 56

/** Number of bytes in an elligated point.  For now set the same as SER_BYTES
 * but could be different for other curves.
 */
#define GOLDILOCKS_448_HASH_BYTES 56

/** Number of bytes in a serialized scalar. */
#define GOLDILOCKS_448_SCALAR_BYTES 56

/** Number of bits in the "which" field of an elligator inverse */
#define GOLDILOCKS_448_INVERT_ELLIGATOR_WHICH_BITS 3

/** The cofactor the curve would have, if we hadn't removed it */
#define GOLDILOCKS_448_REMOVED_COFACTOR 4

/** X448 encoding ratio. */
#define GOLDILOCKS_X448_ENCODE_RATIO 2

/** Number of bytes in an x448 public key */
#define GOLDILOCKS_X448_PUBLIC_BYTES 56

/** Number of bytes in an x448 private key */
#define GOLDILOCKS_X448_PRIVATE_BYTES 56

/** Representation of a point on the elliptic curve. */
typedef struct goldilocks_448_point_s {
    /** @cond internal */
    gf_448_p x,y,z,t; /* Twisted extended homogeneous coordinates */
    /** @endcond */
} goldilocks_448_point_s, goldilocks_448_point_p[1];

/** Precomputed table based on a point.  Can be trivial implementation. */
struct goldilocks_448_precomputed_s;

/** Precomputed table based on a point.  Can be trivial implementation. */
typedef struct goldilocks_448_precomputed_s goldilocks_448_precomputed_s;

/** Size and alignment of precomputed point tables. */
extern const size_t goldilocks_448_sizeof_precomputed_s GOLDILOCKS_API_VIS, goldilocks_448_alignof_precomputed_s GOLDILOCKS_API_VIS;

/** Representation of an element of the scalar field. */
typedef struct goldilocks_448_scalar_s {
    /** @cond internal */
    goldilocks_word_t limb[GOLDILOCKS_448_SCALAR_LIMBS];
    /** @endcond */
} goldilocks_448_scalar_p[1];

/** The scalar 1. */
extern const goldilocks_448_scalar_p goldilocks_448_scalar_one GOLDILOCKS_API_VIS;

/** The scalar 0. */
extern const goldilocks_448_scalar_p goldilocks_448_scalar_zero GOLDILOCKS_API_VIS;

/** The identity (zero) point on the curve. */
extern const goldilocks_448_point_p goldilocks_448_point_identity GOLDILOCKS_API_VIS;

/** An arbitrarily-chosen base point on the curve. */
extern const goldilocks_448_point_p goldilocks_448_point_base GOLDILOCKS_API_VIS;

/** Precomputed table of multiples of the base point on the curve. */
extern const struct goldilocks_448_precomputed_s *goldilocks_448_precomputed_base GOLDILOCKS_API_VIS;

/**
 * @brief Read a scalar from wire format or from bytes.
 *
 * @param [in] ser Serialized form of a scalar.
 * @param [out] out Deserialized form.
 *
 * @retval GOLDILOCKS_SUCCESS The scalar was correctly encoded.
 * @retval GOLDILOCKS_FAILURE The scalar was greater than the modulus,
 * and has been reduced modulo that modulus.
 */
goldilocks_error_t goldilocks_448_scalar_decode (
    goldilocks_448_scalar_p out,
    const unsigned char ser[GOLDILOCKS_448_SCALAR_BYTES]
) GOLDILOCKS_API_VIS GOLDILOCKS_WARN_UNUSED GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Read a scalar from wire format or from bytes.  Reduces mod
 * scalar prime.
 *
 * @param [in] ser Serialized form of a scalar.
 * @param [in] ser_len Length of serialized form.
 * @param [out] out Deserialized form.
 */
void goldilocks_448_scalar_decode_long (
    goldilocks_448_scalar_p out,
    const unsigned char *ser,
    size_t ser_len
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Serialize a scalar to wire format.
 *
 * @param [out] ser Serialized form of a scalar.
 * @param [in] s Deserialized scalar.
 */
void goldilocks_448_scalar_encode (
    unsigned char ser[GOLDILOCKS_448_SCALAR_BYTES],
    const goldilocks_448_scalar_p s
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE GOLDILOCKS_NOINLINE;

/**
 * @brief Add two scalars.  The scalars may use the same memory.
 * @param [in] a One scalar.
 * @param [in] b Another scalar.
 * @param [out] out a+b.
 */
void goldilocks_448_scalar_add (
    goldilocks_448_scalar_p out,
    const goldilocks_448_scalar_p a,
    const goldilocks_448_scalar_p b
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Compare two scalars.
 * @param [in] a One scalar.
 * @param [in] b Another scalar.
 * @retval GOLDILOCKS_TRUE The scalars are equal.
 * @retval GOLDILOCKS_FALSE The scalars are not equal.
 */
goldilocks_bool_t goldilocks_448_scalar_eq (
    const goldilocks_448_scalar_p a,
    const goldilocks_448_scalar_p b
) GOLDILOCKS_API_VIS GOLDILOCKS_WARN_UNUSED GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Subtract two scalars.  The scalars may use the same memory.
 * @param [in] a One scalar.
 * @param [in] b Another scalar.
 * @param [out] out a-b.
 */
void goldilocks_448_scalar_sub (
    goldilocks_448_scalar_p out,
    const goldilocks_448_scalar_p a,
    const goldilocks_448_scalar_p b
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Multiply two scalars.  The scalars may use the same memory.
 * @param [in] a One scalar.
 * @param [in] b Another scalar.
 * @param [out] out a*b.
 */
void goldilocks_448_scalar_mul (
    goldilocks_448_scalar_p out,
    const goldilocks_448_scalar_p a,
    const goldilocks_448_scalar_p b
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
* @brief Halve a scalar.  The scalars may use the same memory.
* @param [in] a A scalar.
* @param [out] out a/2.
*/
void goldilocks_448_scalar_halve (
   goldilocks_448_scalar_p out,
   const goldilocks_448_scalar_p a
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Invert a scalar.  When passed zero, return 0.  The input and output may alias.
 * @param [in] a A scalar.
 * @param [out] out 1/a.
 * @return GOLDILOCKS_SUCCESS The input is nonzero.
 */
goldilocks_error_t goldilocks_448_scalar_invert (
    goldilocks_448_scalar_p out,
    const goldilocks_448_scalar_p a
) GOLDILOCKS_API_VIS GOLDILOCKS_WARN_UNUSED GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Copy a scalar.  The scalars may use the same memory, in which
 * case this function does nothing.
 * @param [in] a A scalar.
 * @param [out] out Will become a copy of a.
 */
static inline void GOLDILOCKS_NONNULL goldilocks_448_scalar_copy (
    goldilocks_448_scalar_p out,
    const goldilocks_448_scalar_p a
) {
    *out = *a;
}

/**
 * @brief Set a scalar to an unsigned 64-bit integer.
 * @param [in] a An integer.
 * @param [out] out Will become equal to a.
 */
void goldilocks_448_scalar_set_unsigned (
    goldilocks_448_scalar_p out,
    uint64_t a
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL;

/**
 * @brief Encode a point as a sequence of bytes.
 *
 * @param [out] ser The byte representation of the point.
 * @param [in] pt The point to encode.
 */
void goldilocks_448_point_encode (
    uint8_t ser[GOLDILOCKS_448_SER_BYTES],
    const goldilocks_448_point_p pt
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Decode a point from a sequence of bytes.
 *
 * Every point has a unique encoding, so not every
 * sequence of bytes is a valid encoding.  If an invalid
 * encoding is given, the output is undefined.
 *
 * @param [out] pt The decoded point.
 * @param [in] ser The serialized version of the point.
 * @param [in] allow_identity GOLDILOCKS_TRUE if the identity is a legal input.
 * @retval GOLDILOCKS_SUCCESS The decoding succeeded.
 * @retval GOLDILOCKS_FAILURE The decoding didn't succeed, because
 * ser does not represent a point.
 */
goldilocks_error_t goldilocks_448_point_decode (
    goldilocks_448_point_p pt,
    const uint8_t ser[GOLDILOCKS_448_SER_BYTES],
    goldilocks_bool_t allow_identity
) GOLDILOCKS_API_VIS GOLDILOCKS_WARN_UNUSED GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Copy a point.  The input and output may alias,
 * in which case this function does nothing.
 *
 * @param [out] a A copy of the point.
 * @param [in] b Any point.
 */
static inline void GOLDILOCKS_NONNULL goldilocks_448_point_copy (
    goldilocks_448_point_p a,
    const goldilocks_448_point_p b
) {
    *a=*b;
}

/**
 * @brief Test whether two points are equal.  If yes, return
 * GOLDILOCKS_TRUE, else return GOLDILOCKS_FALSE.
 *
 * @param [in] a A point.
 * @param [in] b Another point.
 * @retval GOLDILOCKS_TRUE The points are equal.
 * @retval GOLDILOCKS_FALSE The points are not equal.
 */
goldilocks_bool_t goldilocks_448_point_eq (
    const goldilocks_448_point_p a,
    const goldilocks_448_point_p b
) GOLDILOCKS_API_VIS GOLDILOCKS_WARN_UNUSED GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Add two points to produce a third point.  The
 * input points and output point can be pointers to the same
 * memory.
 *
 * @param [out] sum The sum a+b.
 * @param [in] a An addend.
 * @param [in] b An addend.
 */
void goldilocks_448_point_add (
    goldilocks_448_point_p sum,
    const goldilocks_448_point_p a,
    const goldilocks_448_point_p b
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL;

/**
 * @brief Double a point.  Equivalent to
 * goldilocks_448_point_add(two_a,a,a), but potentially faster.
 *
 * @param [out] two_a The sum a+a.
 * @param [in] a A point.
 */
void goldilocks_448_point_double (
    goldilocks_448_point_p two_a,
    const goldilocks_448_point_p a
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL;

/**
 * @brief Subtract two points to produce a third point.  The
 * input points and output point can be pointers to the same
 * memory.
 *
 * @param [out] diff The difference a-b.
 * @param [in] a The minuend.
 * @param [in] b The subtrahend.
 */
void goldilocks_448_point_sub (
    goldilocks_448_point_p diff,
    const goldilocks_448_point_p a,
    const goldilocks_448_point_p b
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL;

/**
 * @brief Negate a point to produce another point.  The input
 * and output points can use the same memory.
 *
 * @param [out] nega The negated input point
 * @param [in] a The input point.
 */
void goldilocks_448_point_negate (
   goldilocks_448_point_p nega,
   const goldilocks_448_point_p a
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL;

/**
 * @brief Multiply a base point by a scalar: scaled = scalar*base.
 *
 * @param [out] scaled The scaled point base*scalar
 * @param [in] base The point to be scaled.
 * @param [in] scalar The scalar to multiply by.
 */
void goldilocks_448_point_scalarmul (
    goldilocks_448_point_p scaled,
    const goldilocks_448_point_p base,
    const goldilocks_448_scalar_p scalar
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Multiply a base point by a scalar: scaled = scalar*base.
 * This function operates directly on serialized forms.
 *
 * @warning This function is experimental.  It may not be supported
 * long-term.
 *
 * @param [out] scaled The scaled point base*scalar
 * @param [in] base The point to be scaled.
 * @param [in] scalar The scalar to multiply by.
 * @param [in] allow_identity Allow the input to be the identity.
 * @param [in] short_circuit Allow a fast return if the input is illegal.
 *
 * @retval GOLDILOCKS_SUCCESS The scalarmul succeeded.
 * @retval GOLDILOCKS_FAILURE The scalarmul didn't succeed, because
 * base does not represent a point.
 */
goldilocks_error_t goldilocks_448_direct_scalarmul (
    uint8_t scaled[GOLDILOCKS_448_SER_BYTES],
    const uint8_t base[GOLDILOCKS_448_SER_BYTES],
    const goldilocks_448_scalar_p scalar,
    goldilocks_bool_t allow_identity,
    goldilocks_bool_t short_circuit
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_WARN_UNUSED GOLDILOCKS_NOINLINE;

/**
 * @brief RFC 7748 Diffie-Hellman scalarmul, used to compute shared secrets.
 * This function uses a different (non-Decaf) encoding.
 *
 * @param [out] shared The shared secret base*scalar
 * @param [in] base The other party's public key, used as the base of the scalarmul.
 * @param [in] scalar The private scalar to multiply by.
 *
 * @retval GOLDILOCKS_SUCCESS The scalarmul succeeded.
 * @retval GOLDILOCKS_FAILURE The scalarmul didn't succeed, because the base
 * point is in a small subgroup.
 */
goldilocks_error_t goldilocks_x448 (
    uint8_t shared[GOLDILOCKS_X448_PUBLIC_BYTES],
    const uint8_t base[GOLDILOCKS_X448_PUBLIC_BYTES],
    const uint8_t scalar[GOLDILOCKS_X448_PRIVATE_BYTES]
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_WARN_UNUSED GOLDILOCKS_NOINLINE;

/**
 * @brief Multiply a point by GOLDILOCKS_X448_ENCODE_RATIO,
 * then encode it like RFC 7748.
 *
 * This function is mainly used internally, but is exported in case
 * it will be useful.
 *
 * The ratio is necessary because the internal representation doesn't
 * track the cofactor information, so on output we must clear the cofactor.
 * This would multiply by the cofactor, but in fact internally libgoldilock's
 * points are always even, so it multiplies by half the cofactor instead.
 *
 * As it happens, this aligns with the base point definitions; that is,
 * if you pass the base point to this function, the result
 * will be GOLDILOCKS_X448_ENCODE_RATIO times the X448
 * base point.
 *
 * @param [out] out The scaled and encoded point.
 * @param [in] p The point to be scaled and encoded.
 */
void goldilocks_448_point_mul_by_ratio_and_encode_like_x448 (
    uint8_t out[GOLDILOCKS_X448_PUBLIC_BYTES],
    const goldilocks_448_point_p p
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL;

/** The base point for X448 Diffie-Hellman */
extern const uint8_t
    goldilocks_x448_base_point[GOLDILOCKS_X448_PUBLIC_BYTES]
#ifndef DOXYGEN
    /* For some reason Doxygen chokes on this despite the defense in common.h... */
    GOLDILOCKS_API_VIS
#endif
;

/**
 * @brief RFC 7748 Diffie-Hellman base point scalarmul.  This function uses
 * a different (non-Decaf) encoding.
 *
 * @param [out] out The public key base*scalar
 * @param [in] scalar The private scalar.
 */
void goldilocks_x448_derive_public_key (
    uint8_t out[GOLDILOCKS_X448_PUBLIC_BYTES],
    const uint8_t scalar[GOLDILOCKS_X448_PRIVATE_BYTES]
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/* FUTURE: uint8_t goldilocks_448_encode_like_curve448) */

/**
 * @brief Precompute a table for fast scalar multiplication.
 * Some implementations do not include precomputed points; for
 * those implementations, this implementation simply copies the
 * point.
 *
 * @param [out] a A precomputed table of multiples of the point.
 * @param [in] b Any point.
 */
void goldilocks_448_precompute (
    goldilocks_448_precomputed_s *a,
    const goldilocks_448_point_p b
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Multiply a precomputed base point by a scalar:
 * scaled = scalar*base.
 * Some implementations do not include precomputed points; for
 * those implementations, this function is the same as
 * goldilocks_448_point_scalarmul
 *
 * @param [out] scaled The scaled point base*scalar
 * @param [in] base The point to be scaled.
 * @param [in] scalar The scalar to multiply by.
 */
void goldilocks_448_precomputed_scalarmul (
    goldilocks_448_point_p scaled,
    const goldilocks_448_precomputed_s *base,
    const goldilocks_448_scalar_p scalar
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Multiply two base points by two scalars:
 * scaled = scalar1*base1 + scalar2*base2.
 *
 * Equivalent to two calls to goldilocks_448_point_scalarmul, but may be
 * faster.
 *
 * @param [out] combo The linear combination scalar1*base1 + scalar2*base2.
 * @param [in] base1 A first point to be scaled.
 * @param [in] scalar1 A first scalar to multiply by.
 * @param [in] base2 A second point to be scaled.
 * @param [in] scalar2 A second scalar to multiply by.
 */
void goldilocks_448_point_double_scalarmul (
    goldilocks_448_point_p combo,
    const goldilocks_448_point_p base1,
    const goldilocks_448_scalar_p scalar1,
    const goldilocks_448_point_p base2,
    const goldilocks_448_scalar_p scalar2
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * Multiply one base point by two scalars:
 *
 * a1 = scalar1 * base
 * a2 = scalar2 * base
 *
 * Equivalent to two calls to goldilocks_448_point_scalarmul, but may be
 * faster.
 *
 * @param [out] a1 The first multiple.  It may be the same as the input point.
 * @param [out] a2 The second multiple.  It may be the same as the input point.
 * @param [in] base1 A point to be scaled.
 * @param [in] scalar1 A first scalar to multiply by.
 * @param [in] scalar2 A second scalar to multiply by.
 */
void goldilocks_448_point_dual_scalarmul (
    goldilocks_448_point_p a1,
    goldilocks_448_point_p a2,
    const goldilocks_448_point_p base1,
    const goldilocks_448_scalar_p scalar1,
    const goldilocks_448_scalar_p scalar2
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Multiply two base points by two scalars:
 * scaled = scalar1*goldilocks_448_point_base + scalar2*base2.
 *
 * Otherwise equivalent to goldilocks_448_point_double_scalarmul, but may be
 * faster at the expense of being variable time.
 *
 * @param [out] combo The linear combination scalar1*base + scalar2*base2.
 * @param [in] scalar1 A first scalar to multiply by.
 * @param [in] base2 A second point to be scaled.
 * @param [in] scalar2 A second scalar to multiply by.
 *
 * @warning: This function takes variable time, and may leak the scalars
 * used.  It is designed for signature verification.
 */
void goldilocks_448_base_double_scalarmul_non_secret (
    goldilocks_448_point_p combo,
    const goldilocks_448_scalar_p scalar1,
    const goldilocks_448_point_p base2,
    const goldilocks_448_scalar_p scalar2
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Constant-time decision between two points.  If pick_b
 * is zero, out = a; else out = b.
 *
 * @param [out] out The output.  It may be the same as either input.
 * @param [in] a Any point.
 * @param [in] b Any point.
 * @param [in] pick_b If nonzero, choose point b.
 */
void goldilocks_448_point_cond_sel (
    goldilocks_448_point_p out,
    const goldilocks_448_point_p a,
    const goldilocks_448_point_p b,
    goldilocks_word_t pick_b
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Constant-time decision between two scalars.  If pick_b
 * is zero, out = a; else out = b.
 *
 * @param [out] out The output.  It may be the same as either input.
 * @param [in] a Any scalar.
 * @param [in] b Any scalar.
 * @param [in] pick_b If nonzero, choose scalar b.
 */
void goldilocks_448_scalar_cond_sel (
    goldilocks_448_scalar_p out,
    const goldilocks_448_scalar_p a,
    const goldilocks_448_scalar_p b,
    goldilocks_word_t pick_b
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Test that a point is valid, for debugging purposes.
 *
 * @param [in] to_test The point to test.
 * @retval GOLDILOCKS_TRUE The point is valid.
 * @retval GOLDILOCKS_FALSE The point is invalid.
 */
goldilocks_bool_t goldilocks_448_point_valid (
    const goldilocks_448_point_p to_test
) GOLDILOCKS_API_VIS GOLDILOCKS_WARN_UNUSED GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Torque a point, for debugging purposes.  The output
 * will be equal to the input.
 *
 * @param [out] q The point to torque.
 * @param [in] p The point to torque.
 */
void goldilocks_448_point_debugging_torque (
    goldilocks_448_point_p q,
    const goldilocks_448_point_p p
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Projectively scale a point, for debugging purposes.
 * The output will be equal to the input, and will be valid
 * even if the factor is zero.
 *
 * @param [out] q The point to scale.
 * @param [in] p The point to scale.
 * @param [in] factor Serialized GF factor to scale.
 */
void goldilocks_448_point_debugging_pscale (
    goldilocks_448_point_p q,
    const goldilocks_448_point_p p,
    const unsigned char factor[GOLDILOCKS_448_SER_BYTES]
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Almost-Elligator-like hash to curve.
 *
 * Call this function with the output of a hash to make a hash to the curve.
 *
 * This function runs Elligator2 on the goldilocks_448 Jacobi quartic model.  It then
 * uses the isogeny to put the result in twisted Edwards form.  As a result,
 * it is safe (cannot produce points of order 4), and would be compatible with
 * hypothetical other implementations of Decaf using a Montgomery or untwisted
 * Edwards model.
 *
 * Unlike Elligator, this function may be up to 4:1 on [0,(p-1)/2]:
 *   A factor of 2 due to the isogeny.
 *   A factor of 2 because we quotient out the 2-torsion.
 *
 * This makes it about 8:1 overall, or 16:1 overall on curves with cofactor 8.
 *
 * Negating the input (mod q) results in the same point.  Inverting the input
 * (mod q) results in the negative point.  This is the same as Elligator.
 *
 * This function isn't quite indifferentiable from a random oracle.
 * However, it is suitable for many protocols, including SPEKE and SPAKE2 EE.
 * Furthermore, calling it twice with independent seeds and adding the results
 * is indifferentiable from a random oracle.
 *
 * @param [in] hashed_data Output of some hash function.
 * @param [out] pt The data hashed to the curve.
 */
void
goldilocks_448_point_from_hash_nonuniform (
    goldilocks_448_point_p pt,
    const unsigned char hashed_data[GOLDILOCKS_448_HASH_BYTES]
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Indifferentiable hash function encoding to curve.
 *
 * Equivalent to calling goldilocks_448_point_from_hash_nonuniform twice and adding.
 *
 * @param [in] hashed_data Output of some hash function.
 * @param [out] pt The data hashed to the curve.
 */
void goldilocks_448_point_from_hash_uniform (
    goldilocks_448_point_p pt,
    const unsigned char hashed_data[2*GOLDILOCKS_448_HASH_BYTES]
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE;

/**
 * @brief Inverse of elligator-like hash to curve.
 *
 * This function writes to the buffer, to make it so that
 * goldilocks_448_point_from_hash_nonuniform(buffer) = pt if
 * possible.  Since there may be multiple preimages, the
 * "which" parameter chooses between them.  To ensure uniform
 * inverse sampling, this function succeeds or fails
 * independently for different "which" values.
 *
 * This function isn't guaranteed to find every possible
 * preimage, but it finds all except a small finite number.
 * In particular, when the number of bits in the modulus isn't
 * a multiple of 8 (i.e. for curve25519), it sets the high bits
 * independently, which enables the generated data to be uniform.
 * But it doesn't add p, so you'll never get exactly p from this
 * function.  This might change in the future, especially if
 * we ever support eg Brainpool curves, where this could cause
 * real nonuniformity.
 *
 * @param [out] recovered_hash Encoded data.
 * @param [in] pt The point to encode.
 * @param [in] which A value determining which inverse point
 * to return.
 *
 * @retval GOLDILOCKS_SUCCESS The inverse succeeded.
 * @retval GOLDILOCKS_FAILURE The inverse failed.
 */
goldilocks_error_t
goldilocks_448_invert_elligator_nonuniform (
    unsigned char recovered_hash[GOLDILOCKS_448_HASH_BYTES],
    const goldilocks_448_point_p pt,
    uint32_t which
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE GOLDILOCKS_WARN_UNUSED;

/**
 * @brief Inverse of elligator-like hash to curve.
 *
 * This function writes to the buffer, to make it so that
 * goldilocks_448_point_from_hash_uniform(buffer) = pt if
 * possible.  Since there may be multiple preimages, the
 * "which" parameter chooses between them.  To ensure uniform
 * inverse sampling, this function succeeds or fails
 * independently for different "which" values.
 *
 * @param [out] recovered_hash Encoded data.
 * @param [in] pt The point to encode.
 * @param [in] which A value determining which inverse point
 * to return.
 *
 * @retval GOLDILOCKS_SUCCESS The inverse succeeded.
 * @retval GOLDILOCKS_FAILURE The inverse failed.
 */
goldilocks_error_t
goldilocks_448_invert_elligator_uniform (
    unsigned char recovered_hash[2*GOLDILOCKS_448_HASH_BYTES],
    const goldilocks_448_point_p pt,
    uint32_t which
) GOLDILOCKS_API_VIS GOLDILOCKS_NONNULL GOLDILOCKS_NOINLINE GOLDILOCKS_WARN_UNUSED;

/** Securely erase a scalar. */
void goldilocks_448_scalar_destroy (
    goldilocks_448_scalar_p scalar
) GOLDILOCKS_NONNULL GOLDILOCKS_API_VIS;

/** Securely erase a point by overwriting it with zeros.
 * @warning This causes the point object to become invalid.
 */
void goldilocks_448_point_destroy (
    goldilocks_448_point_p point
) GOLDILOCKS_NONNULL GOLDILOCKS_API_VIS;

/** Securely erase a precomputed table by overwriting it with zeros.
 * @warning This causes the table object to become invalid.
 */
void goldilocks_448_precomputed_destroy (
    goldilocks_448_precomputed_s *pre
) GOLDILOCKS_NONNULL GOLDILOCKS_API_VIS;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __GOLDILOCKS_POINT_448_H__ */
