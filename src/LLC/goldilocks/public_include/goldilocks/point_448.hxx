/**
 * @file goldilocks/point_448.hxx
 * @author Mike Hamburg
 *
 * @copyright
 *   Copyright (c) 2015-2016 Cryptography Research, Inc.  \n
 *   Copyright (c) 2018 the libgoldilocks contributors.  \n
 *   Released under the MIT License.  See LICENSE.txt for license information.
 *
 * A group of prime order, C++ wrapper.
 *
 * The Goldilocks library implements cryptographic operations on a an elliptic curve
 * group of prime order. It accomplishes this by using a twisted Edwards
 * curve (isogenous to Ed448-Goldilocks) and wiping out the cofactor.
 *
 * Most of the functions in this file run in constant time, can't fail
 * except for ubiquitous reasons like memory exhaustion, and contain no
 * data-dependend branches, timing or memory accesses.  There are some
 * exceptions, which should be noted.  Typically, decoding functions can
 * fail.
 */

#ifndef __GOLDILOCKS_POINT_448_HXX__
#define __GOLDILOCKS_POINT_448_HXX__ 1

/** This code uses posix_memalign. */
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif
#include <stdlib.h>
#include <string.h> /* for memcpy */

#include <goldilocks/point_448.h>
#include <goldilocks/ed448.h>
#include <goldilocks/secure_buffer.hxx>
#include <string>
#include <sys/types.h>
#include <limits.h>

/** @cond internal */
#if __cplusplus >= 201103L
#define GOLDILOCKS_NOEXCEPT noexcept
#else
#define GOLDILOCKS_NOEXCEPT throw()
#endif
/** @endcond */

namespace goldilocks {

/**
 * Ed448-Goldilocks/Goldilocks instantiation of group.
 */
struct Ed448Goldilocks {

/** The name of the curve */
static inline const char *name() { return "Ed448-Goldilocks"; }

/** The name of the curve */
static inline int bits() { return 448; }

/** The curve's cofactor (removed, but useful for testing) */
static const int REMOVED_COFACTOR = 4;

/** Residue class of field modulus: p == this mod 2*(this-1) */
static const int FIELD_MODULUS_TYPE = 3;

/** @cond internal */
class Point;
class Precomputed;
/** @endcond */

/**
 * A scalar modulo the curve order.
 * Supports the usual arithmetic operations, all in constant time.
 */
class Scalar : public Serializable<Scalar> {
public:
    /** wrapped C type */
    typedef goldilocks_448_scalar_p Wrapped;

    /** Size of a serialized element */
    static const size_t SER_BYTES = GOLDILOCKS_448_SCALAR_BYTES;

    /** access to the underlying scalar object */
    Wrapped s;

    /** @cond internal */
    /** Don't initialize. */
    inline Scalar(const NOINIT &) GOLDILOCKS_NOEXCEPT {}
    /** @endcond */

    /** Set to an unsigned word */
    inline Scalar(uint64_t w) GOLDILOCKS_NOEXCEPT { *this = w; }

    /** Set to a signed word */
    inline Scalar(int64_t w) GOLDILOCKS_NOEXCEPT { *this = w; }

    /** Set to an unsigned word */
    inline Scalar(unsigned int w) GOLDILOCKS_NOEXCEPT { *this = w; }

    /** Set to a signed word */
    inline Scalar(int w) GOLDILOCKS_NOEXCEPT { *this = w; }

    /** Construct from RNG */
    inline explicit Scalar(Rng &rng) GOLDILOCKS_NOEXCEPT {
        FixedArrayBuffer<SER_BYTES + 16> sb(rng);
        *this = sb;
    }

    /** Construct from goldilocks_scalar_p object. */
    inline Scalar(const Wrapped &t = goldilocks_448_scalar_zero) GOLDILOCKS_NOEXCEPT { goldilocks_448_scalar_copy(s,t); }

    /** Copy constructor. */
    inline Scalar(const Scalar &x) GOLDILOCKS_NOEXCEPT { *this = x; }

    /** Construct from arbitrary-length little-endian byte sequence. */
    inline Scalar(const Block &buffer) GOLDILOCKS_NOEXCEPT { *this = buffer; }

    /** Serializable instance */
    inline size_t ser_size() const GOLDILOCKS_NOEXCEPT { return SER_BYTES; }

    /** Serializable instance */
    inline void serialize_into(unsigned char *buffer) const GOLDILOCKS_NOEXCEPT {
        goldilocks_448_scalar_encode(buffer, s);
    }

    /** Assignment. */
    inline Scalar& operator=(const Scalar &x) GOLDILOCKS_NOEXCEPT { goldilocks_448_scalar_copy(s,x.s); return *this; }

    /** Assign from unsigned 64-bit integer. */
    inline Scalar& operator=(uint64_t w) GOLDILOCKS_NOEXCEPT { goldilocks_448_scalar_set_unsigned(s,w); return *this; }


    /** Assign from signed int. */
    inline Scalar& operator=(int64_t w) GOLDILOCKS_NOEXCEPT {
        Scalar t(-(uint64_t)INT_MIN);
        goldilocks_448_scalar_set_unsigned(s,(uint64_t)w - (uint64_t)INT_MIN);
        *this -= t;
        return *this;
    }

    /** Assign from unsigned int. */
    inline Scalar& operator=(unsigned int w) GOLDILOCKS_NOEXCEPT { return *this = (uint64_t)w; }

    /** Assign from signed int. */
    inline Scalar& operator=(int w) GOLDILOCKS_NOEXCEPT { return *this = (int64_t)w; }

    /** Destructor securely zeorizes the scalar. */
    inline ~Scalar() GOLDILOCKS_NOEXCEPT { goldilocks_448_scalar_destroy(s); }

    /** Assign from arbitrary-length little-endian byte sequence in a Block. */
    inline Scalar &operator=(const Block &bl) GOLDILOCKS_NOEXCEPT {
        goldilocks_448_scalar_decode_long(s,bl.data(),bl.size()); return *this;
    }

    /**
     * Decode from correct-length little-endian byte sequence.
     * @return GOLDILOCKS_FAILURE if the scalar is greater than or equal to the group order q.
     */
    static inline goldilocks_error_t GOLDILOCKS_WARN_UNUSED decode (
        Scalar &sc, const FixedBlock<SER_BYTES> buffer
    ) GOLDILOCKS_NOEXCEPT {
        return goldilocks_448_scalar_decode(sc.s,buffer.data());
    }

    /** Add. */
    inline Scalar operator+ (const Scalar &q) const GOLDILOCKS_NOEXCEPT { Scalar r((NOINIT())); goldilocks_448_scalar_add(r.s,s,q.s); return r; }

    /** Add to this. */
    inline Scalar &operator+=(const Scalar &q) GOLDILOCKS_NOEXCEPT { goldilocks_448_scalar_add(s,s,q.s); return *this; }

    /** Subtract. */
    inline Scalar operator- (const Scalar &q) const GOLDILOCKS_NOEXCEPT { Scalar r((NOINIT())); goldilocks_448_scalar_sub(r.s,s,q.s); return r; }

    /** Subtract from this. */
    inline Scalar &operator-=(const Scalar &q) GOLDILOCKS_NOEXCEPT { goldilocks_448_scalar_sub(s,s,q.s); return *this; }

    /** Multiply */
    inline Scalar operator* (const Scalar &q) const GOLDILOCKS_NOEXCEPT { Scalar r((NOINIT())); goldilocks_448_scalar_mul(r.s,s,q.s); return r; }

    /** Multiply into this. */
    inline Scalar &operator*=(const Scalar &q) GOLDILOCKS_NOEXCEPT { goldilocks_448_scalar_mul(s,s,q.s); return *this; }

    /** Negate */
    inline Scalar operator- () const GOLDILOCKS_NOEXCEPT { Scalar r((NOINIT())); goldilocks_448_scalar_sub(r.s,goldilocks_448_scalar_zero,s); return r; }

    /** Return 1/this.
     * @throw CryptoException if this is 0.
     */
    inline Scalar inverse() const /*throw(CryptoException)*/ {
        Scalar r;
        if (GOLDILOCKS_SUCCESS != goldilocks_448_scalar_invert(r.s,s)) {
            throw CryptoException();
        }
        return r;
    }

    /** Invert with Fermat's Little Theorem (slow!). If *this == 0, set r=0
     * and return GOLDILOCKS_FAILURE. */
    inline goldilocks_error_t GOLDILOCKS_WARN_UNUSED
    inverse_noexcept(Scalar &r) const GOLDILOCKS_NOEXCEPT {
        return goldilocks_448_scalar_invert(r.s,s);
    }

    /** Return this/q. @throw CryptoException if q == 0. */
    inline Scalar operator/ (const Scalar &q) const /*throw(CryptoException)*/ { return *this * q.inverse(); }

    /** Set this to this/q. @throw CryptoException if q == 0. */
    inline Scalar &operator/=(const Scalar &q) /*throw(CryptoException)*/ { return *this *= q.inverse(); }

    /** Return half this scalar.  Much faster than /2. */
    inline Scalar half() const { Scalar out; goldilocks_448_scalar_halve(out.s,s); return out; }

    /** Compare in constant time */
    inline bool operator!=(const Scalar &q) const GOLDILOCKS_NOEXCEPT { return !(*this == q); }

    /** Compare in constant time */
    inline bool operator==(const Scalar &q) const GOLDILOCKS_NOEXCEPT { return !!goldilocks_448_scalar_eq(s,q.s); }

    /** Scalarmul with scalar on left. */
    inline Point operator* (const Point &q) const GOLDILOCKS_NOEXCEPT { return q * (*this); }

    /** Scalarmul-precomputed with scalar on left. */
    inline Point operator* (const Precomputed &q) const GOLDILOCKS_NOEXCEPT { return q * (*this); }

    /** Direct scalar multiplication.
     * @throw CryptoException if the input didn't decode.
     */
    inline SecureBuffer direct_scalarmul (
        const FixedBlock<SER_BYTES> &in,
        goldilocks_bool_t allow_identity=GOLDILOCKS_FALSE,
        goldilocks_bool_t short_circuit=GOLDILOCKS_TRUE
    ) const /*throw(CryptoException)*/;

    /** Direct scalar multiplication. */
    inline goldilocks_error_t GOLDILOCKS_WARN_UNUSED direct_scalarmul_noexcept(
        FixedBuffer<SER_BYTES> &out,
        const FixedBlock<SER_BYTES> &in,
        goldilocks_bool_t allow_identity=GOLDILOCKS_FALSE,
        goldilocks_bool_t short_circuit=GOLDILOCKS_TRUE
    ) const GOLDILOCKS_NOEXCEPT;
};

/** Element of prime-order elliptic curve group. */
class Point : public Serializable<Point> {
public:
    /** Wrapped C type */
    typedef goldilocks_448_point_p Wrapped;

    /** Size of a serialized element */
    static const size_t SER_BYTES = GOLDILOCKS_448_SER_BYTES;

    /** Bytes required for hash */
    static const size_t HASH_BYTES = GOLDILOCKS_448_HASH_BYTES;

    /** Bytes required for EdDSA encoding */
    static const size_t EDDSA_BYTES = GOLDILOCKS_EDDSA_448_PUBLIC_BYTES;

    /** Bytes required for EdDSA encoding */
    static const size_t LADDER_BYTES = GOLDILOCKS_X448_PUBLIC_BYTES;

    /** Ratio due to EdDSA encoding */
    static const int EDDSA_ENCODE_RATIO = GOLDILOCKS_448_EDDSA_ENCODE_RATIO;

    /** Ratio due to EdDSA decoding */
    static const int EDDSA_DECODE_RATIO = GOLDILOCKS_448_EDDSA_DECODE_RATIO;

    /** Ratio due to ladder decoding */
    static const int LADDER_ENCODE_RATIO = GOLDILOCKS_X448_ENCODE_RATIO;

    /** Size of a steganographically-encoded curve element.  If the point is random, the encoding
     * should look statistically close to a uniformly-random sequnece of STEG_BYTES bytes.
     */
    static const size_t STEG_BYTES = HASH_BYTES * 2;

    /** Number of bits in invert_elligator which are actually used. */
    static const unsigned int INVERT_ELLIGATOR_WHICH_BITS = GOLDILOCKS_448_INVERT_ELLIGATOR_WHICH_BITS;

    /** The c-level object. */
    Wrapped p;

    /** @cond internal */
    /** Don't initialize. */
    inline Point(const NOINIT &) GOLDILOCKS_NOEXCEPT {}
    /** @endcond */

    /** Constructor sets to identity by default. */
    inline Point(const Wrapped &q = goldilocks_448_point_identity) GOLDILOCKS_NOEXCEPT { goldilocks_448_point_copy(p,q); }

    /** Copy constructor. */
    inline Point(const Point &q) GOLDILOCKS_NOEXCEPT { *this = q; }

    /** Assignment. */
    inline Point& operator=(const Point &q) GOLDILOCKS_NOEXCEPT { goldilocks_448_point_copy(p,q.p); return *this; }

    /** Destructor securely zeorizes the point. */
    inline ~Point() GOLDILOCKS_NOEXCEPT { goldilocks_448_point_destroy(p); }

    /** Construct from RNG */
    inline explicit Point(Rng &rng, bool uniform = true) GOLDILOCKS_NOEXCEPT {
        if (uniform) {
            FixedArrayBuffer<2*HASH_BYTES> b(rng);
            set_to_hash(b);
        } else {
            FixedArrayBuffer<HASH_BYTES> b(rng);
            set_to_hash(b);
        }
    }

   /**
    * Initialize from a fixed-length byte string.
    * The all-zero string maps to the identity.
    *
    * @throw CryptoException the string was the wrong length, or wasn't the encoding of a point,
    * or was the identity and allow_identity was GOLDILOCKS_FALSE.
    */
    inline explicit Point(const FixedBlock<SER_BYTES> &buffer, bool allow_identity=true)
        /*throw(CryptoException)*/ {
        if (GOLDILOCKS_SUCCESS != decode(buffer,allow_identity)) {
            throw CryptoException();
        }
    }

    /**
     * Initialize from C++ fixed-length byte string.
     * The all-zero string maps to the identity.
     *
     * @retval GOLDILOCKS_SUCCESS the string was successfully decoded.
     * @return GOLDILOCKS_FAILURE the string was the wrong length, or wasn't the encoding of a point,
     * or was the identity and allow_identity was GOLDILOCKS_FALSE. Contents of the buffer are undefined.
     */
    inline goldilocks_error_t GOLDILOCKS_WARN_UNUSED decode (
        const FixedBlock<SER_BYTES> &buffer, bool allow_identity=true
    ) GOLDILOCKS_NOEXCEPT {
        return goldilocks_448_point_decode(p,buffer.data(),allow_identity ? GOLDILOCKS_TRUE : GOLDILOCKS_FALSE);
    }

    /**
     * Initialize from C++ fixed-length byte string, like EdDSA.
     * The all-zero string maps to the identity.
     *
     * @retval GOLDILOCKS_SUCCESS the string was successfully decoded.
     * @return GOLDILOCKS_FAILURE the string was the wrong length, or wasn't the encoding of a point.
     * Contents of the point are undefined.
     */
    inline goldilocks_error_t GOLDILOCKS_WARN_UNUSED decode_like_eddsa_and_mul_by_ratio_noexcept (
        const FixedBlock<GOLDILOCKS_EDDSA_448_PUBLIC_BYTES> &buffer
    ) GOLDILOCKS_NOEXCEPT {
        return goldilocks_448_point_decode_like_eddsa_and_mul_by_ratio(p,buffer.data());
    }

    /**
     * Decode from EDDSA, multiply by EDDSA_DECODE_RATIO, and ignore any
     * remaining cofactor information.
     * @throw CryptoException if the input point was invalid.
     */
    inline void decode_like_eddsa_and_mul_by_ratio(
        const FixedBlock<GOLDILOCKS_EDDSA_448_PUBLIC_BYTES> &buffer
    ) /*throw(CryptoException)*/ {
        if (GOLDILOCKS_SUCCESS != decode_like_eddsa_and_mul_by_ratio_noexcept(buffer)) throw(CryptoException());
    }

    /** Multiply by EDDSA_ENCODE_RATIO and encode like EdDSA. */
    inline SecureBuffer mul_by_ratio_and_encode_like_eddsa() const {
        SecureBuffer ret(GOLDILOCKS_EDDSA_448_PUBLIC_BYTES);
        goldilocks_448_point_mul_by_ratio_and_encode_like_eddsa(ret.data(),p);
        return ret;
    }

    /** Multiply by EDDSA_ENCODE_RATIO and encode like EdDSA. */
    inline void mul_by_ratio_and_encode_like_eddsa(
        FixedBuffer<GOLDILOCKS_EDDSA_448_PUBLIC_BYTES> &out
    ) const {
        goldilocks_448_point_mul_by_ratio_and_encode_like_eddsa(out.data(),p);
    }

    /** Multiply by LADDER_ENCODE_RATIO and encode like X448. */
    inline SecureBuffer mul_by_ratio_and_encode_like_ladder() const {
        SecureBuffer ret(LADDER_BYTES);
        goldilocks_448_point_mul_by_ratio_and_encode_like_x448(ret.data(),p);
        return ret;
    }

    /** Multiply by LADDER_ENCODE_RATIO and encode like X448. */
    inline void mul_by_ratio_and_encode_like_ladder(FixedBuffer<LADDER_BYTES> &out) const {
        goldilocks_448_point_mul_by_ratio_and_encode_like_x448(out.data(),p);
    }

    /**
     * Map uniformly to the curve from a hash buffer.
     * The empty or all-zero string maps to the identity, as does the string "\\x01".
     * If the buffer is shorter than 2*HASH_BYTES, well, it won't be as uniform,
     * but the buffer will be zero-padded on the right.
     */
    static inline Point from_hash ( const Block &s ) GOLDILOCKS_NOEXCEPT {
        Point p((NOINIT())); p.set_to_hash(s); return p;
    }

    /**
     * Map to the curve from a hash buffer.
     * The empty or all-zero string maps to the identity, as does the string "\\x01".
     * If the buffer is shorter than 2*HASH_BYTES, well, it won't be as uniform,
     * but the buffer will be zero-padded on the right.
     */
    inline void set_to_hash( const Block &s ) GOLDILOCKS_NOEXCEPT {
        if (s.size() < HASH_BYTES) {
            SecureBuffer b(HASH_BYTES);
            memcpy(b.data(), s.data(), s.size());
            goldilocks_448_point_from_hash_nonuniform(p,b.data());
        } else if (s.size() == HASH_BYTES) {
            goldilocks_448_point_from_hash_nonuniform(p,s.data());
        } else if (s.size() < 2*HASH_BYTES) {
            SecureBuffer b(2*HASH_BYTES);
            memcpy(b.data(), s.data(), s.size());
            goldilocks_448_point_from_hash_uniform(p,b.data());
        } else {
            goldilocks_448_point_from_hash_uniform(p,s.data());
        }
    }

    /** Encode to string. The identity encodes to the all-zero string. */
    inline operator SecureBuffer() const {
        SecureBuffer buffer(SER_BYTES);
        goldilocks_448_point_encode(buffer.data(), p);
        return buffer;
    }

    /** Serializable instance */
    inline size_t ser_size() const GOLDILOCKS_NOEXCEPT { return SER_BYTES; }

    /** Serializable instance */
    inline void serialize_into(unsigned char *buffer) const GOLDILOCKS_NOEXCEPT {
        goldilocks_448_point_encode(buffer, p);
    }

    /** Point add. */
    inline Point operator+ (const Point &q) const GOLDILOCKS_NOEXCEPT { Point r((NOINIT())); goldilocks_448_point_add(r.p,p,q.p); return r; }

    /** Point add. */
    inline Point &operator+=(const Point &q) GOLDILOCKS_NOEXCEPT { goldilocks_448_point_add(p,p,q.p); return *this; }

    /** Point subtract. */
    inline Point operator- (const Point &q) const GOLDILOCKS_NOEXCEPT { Point r((NOINIT())); goldilocks_448_point_sub(r.p,p,q.p); return r; }

    /** Point subtract. */
    inline Point &operator-=(const Point &q) GOLDILOCKS_NOEXCEPT { goldilocks_448_point_sub(p,p,q.p); return *this; }

    /** Point negate. */
    inline Point operator- () const GOLDILOCKS_NOEXCEPT { Point r((NOINIT())); goldilocks_448_point_negate(r.p,p); return r; }

    /** Double the point out of place. */
    inline Point times_two () const GOLDILOCKS_NOEXCEPT { Point r((NOINIT())); goldilocks_448_point_double(r.p,p); return r; }

    /** Double the point in place. */
    inline Point &double_in_place() GOLDILOCKS_NOEXCEPT { goldilocks_448_point_double(p,p); return *this; }

    /** Constant-time compare. */
    inline bool operator!=(const Point &q) const GOLDILOCKS_NOEXCEPT { return ! goldilocks_448_point_eq(p,q.p); }

    /** Constant-time compare. */
    inline bool operator==(const Point &q) const GOLDILOCKS_NOEXCEPT { return !!goldilocks_448_point_eq(p,q.p); }

    /** Scalar multiply. */
    inline Point operator* (const Scalar &s) const GOLDILOCKS_NOEXCEPT { Point r((NOINIT())); goldilocks_448_point_scalarmul(r.p,p,s.s); return r; }

    /** Scalar multiply in place. */
    inline Point &operator*=(const Scalar &s) GOLDILOCKS_NOEXCEPT { goldilocks_448_point_scalarmul(p,p,s.s); return *this; }

    /** Multiply by s.inverse(). If s=0, maps to the identity. */
    inline Point operator/ (const Scalar &s) const /*throw(CryptoException)*/ { return (*this) * s.inverse(); }

    /** Multiply by s.inverse(). If s=0, maps to the identity. */
    inline Point &operator/=(const Scalar &s) /*throw(CryptoException)*/ { return (*this) *= s.inverse(); }

    /** Validate / sanity check */
    inline bool validate() const GOLDILOCKS_NOEXCEPT { return goldilocks_448_point_valid(p); }

    /** Double-scalar multiply, equivalent to q*qs + r*rs but faster. */
    static inline Point double_scalarmul (
        const Point &q, const Scalar &qs, const Point &r, const Scalar &rs
    ) GOLDILOCKS_NOEXCEPT {
        Point p((NOINIT())); goldilocks_448_point_double_scalarmul(p.p,q.p,qs.s,r.p,rs.s); return p;
    }

    /** Dual-scalar multiply, equivalent to this*r1, this*r2 but faster. */
    inline void dual_scalarmul (
        Point &q1, Point &q2, const Scalar &r1, const Scalar &r2
    ) const GOLDILOCKS_NOEXCEPT {
        goldilocks_448_point_dual_scalarmul(q1.p,q2.p,p,r1.s,r2.s);
    }

    /**
     * Double-scalar multiply, equivalent to q*qs + r*rs but faster.
     * For those who like their scalars before the point.
     */
    static inline Point double_scalarmul (
        const Scalar &qs, const Point &q, const Scalar &rs, const Point &r
    ) GOLDILOCKS_NOEXCEPT {
        return double_scalarmul(q,qs,r,rs);
    }

    /**
     * Double-scalar multiply: this point by the first scalar and base by the second scalar.
     * @warning This function takes variable time, and may leak the scalars (or points, but currently
     * it doesn't).
     */
    inline Point non_secret_combo_with_base(const Scalar &s, const Scalar &s_base) GOLDILOCKS_NOEXCEPT {
        Point r((NOINIT())); goldilocks_448_base_double_scalarmul_non_secret(r.p,s_base.s,p,s.s); return r;
    }

    /** Return a point equal to *this, whose internal data is rotated by a torsion element. */
    inline Point debugging_torque() const GOLDILOCKS_NOEXCEPT {
        Point q;
        goldilocks_448_point_debugging_torque(q.p,p);
        return q;
    }

    /** Return a point equal to *this, whose internal data has a modified representation. */
    inline Point debugging_pscale(const FixedBlock<SER_BYTES> factor) const GOLDILOCKS_NOEXCEPT {
        Point q;
        goldilocks_448_point_debugging_pscale(q.p,p,factor.data());
        return q;
    }

    /** Return a point equal to *this, whose internal data has a randomized representation. */
    inline Point debugging_pscale(Rng &r) const GOLDILOCKS_NOEXCEPT {
        FixedArrayBuffer<SER_BYTES> sb(r);
        return debugging_pscale(sb);
    }

    /**
     * Modify buffer so that Point::from_hash(Buffer) == *this, and return GOLDILOCKS_SUCCESS;
     * or leave buf unmodified and return GOLDILOCKS_FAILURE.
     */
    inline goldilocks_error_t invert_elligator (
        Buffer buf, uint32_t hint
    ) const GOLDILOCKS_NOEXCEPT {
        unsigned char buf2[2*HASH_BYTES];
        memset(buf2,0,sizeof(buf2));
        memcpy(buf2,buf.data(),(buf.size() > 2*HASH_BYTES) ? 2*HASH_BYTES : buf.size());
        goldilocks_bool_t ret;
        if (buf.size() > HASH_BYTES) {
            ret = goldilocks_successful(goldilocks_448_invert_elligator_uniform(buf2, p, hint));
        } else {
            ret = goldilocks_successful(goldilocks_448_invert_elligator_nonuniform(buf2, p, hint));
        }
        if (buf.size() < HASH_BYTES) {
            ret &= goldilocks_memeq(&buf2[buf.size()], &buf2[HASH_BYTES], HASH_BYTES - buf.size());
        }
        for (size_t i=0; i<buf.size() && i<HASH_BYTES; i++) {
            buf[i] = (buf[i] & ~ret) | (buf2[i] &ret);
        }
        goldilocks_bzero(buf2,sizeof(buf2));
        return goldilocks_succeed_if(ret);
    }

    /** Steganographically encode this */
    inline SecureBuffer steg_encode(Rng &rng, size_t size=STEG_BYTES) const /*throw(std::bad_alloc, LengthException)*/ {
        if (size <= HASH_BYTES + 4 || size > 2*HASH_BYTES) throw LengthException();
        SecureBuffer out(STEG_BYTES);
        goldilocks_error_t done;
        do {
            rng.read(Buffer(out).slice(HASH_BYTES-4,STEG_BYTES-HASH_BYTES+1));
            uint32_t hint = 0;
            for (int i=0; i<4; i++) { hint |= uint32_t(out[HASH_BYTES-4+i])<<(8*i); }
            done = invert_elligator(out, hint);
        } while (!goldilocks_successful(done));
        return out;
    }

    /** Return the base point of the curve. */
    static inline const Point base() GOLDILOCKS_NOEXCEPT { return Point(goldilocks_448_point_base); }

    /** Return the identity point of the curve. */
    static inline const Point identity() GOLDILOCKS_NOEXCEPT { return Point(goldilocks_448_point_identity); }
};

/**
 * Precomputed table of points.
 * Minor difficulties arise here because the goldilocks API doesn't expose, as a constant, how big such an object is.
 * Therefore we have to call malloc() or friends, but that's probably for the best, because you don't want to
 * stack-allocate a 15kiB object anyway.
 */

/** @cond internal */
typedef goldilocks_448_precomputed_s Precomputed_U;
/** @endcond */
class Precomputed
    /** @cond internal */
    : protected OwnedOrUnowned<Precomputed,Precomputed_U>
    /** @endcond */
{
public:

    /** Destructor securely zeorizes the memory. */
    inline ~Precomputed() GOLDILOCKS_NOEXCEPT { clear(); }

    /**
     * Initialize from underlying type, declared as a reference to prevent
     * it from being called with 0, thereby breaking override.
     *
     * The underlying object must remain valid throughout the lifetime of this one.
     *
     * By default, initializes to the table for the base point.
     *
     * @warning The empty initializer makes this equal to base, unlike the empty
     * initializer for points which makes this equal to the identity.
     */
    inline Precomputed (
        const Precomputed_U &yours = *goldilocks_448_precomputed_base
    ) GOLDILOCKS_NOEXCEPT : OwnedOrUnowned<Precomputed,Precomputed_U>(yours) {}


#if __cplusplus >= 201103L
    /** Move-assign operator */
    inline Precomputed &operator=(Precomputed &&it) GOLDILOCKS_NOEXCEPT {
        OwnedOrUnowned<Precomputed,Precomputed_U>::operator= (it);
        return *this;
    }

    /** Move constructor */
    inline Precomputed(Precomputed &&it) GOLDILOCKS_NOEXCEPT : OwnedOrUnowned<Precomputed,Precomputed_U>() {
        *this = it;
    }

    /** Undelete copy operator */
    inline Precomputed &operator=(const Precomputed &it) GOLDILOCKS_NOEXCEPT {
        OwnedOrUnowned<Precomputed,Precomputed_U>::operator= (it);
        return *this;
    }
#endif

    /**
     * Initilaize from point. Must allocate memory, and may throw.
     */
    inline Precomputed &operator=(const Point &it) /*throw(std::bad_alloc)*/ {
        alloc();
        goldilocks_448_precompute(ours.mine,it.p);
        return *this;
    }

    /**
     * Copy constructor.
     */
    inline Precomputed(const Precomputed &it) /*throw(std::bad_alloc)*/
        : OwnedOrUnowned<Precomputed,Precomputed_U>() { *this = it; }

    /**
     * Constructor which initializes from point.
     */
    inline explicit Precomputed(const Point &it) /*throw(std::bad_alloc)*/
        : OwnedOrUnowned<Precomputed,Precomputed_U>() { *this = it; }

    /** Fixed base scalarmul. */
    inline Point operator* (const Scalar &s) const GOLDILOCKS_NOEXCEPT { Point r; goldilocks_448_precomputed_scalarmul(r.p,get(),s.s); return r; }

    /** Multiply by s.inverse(). If s=0, maps to the identity. */
    inline Point operator/ (const Scalar &s) const /*throw(CryptoException)*/ { return (*this) * s.inverse(); }

    /** Return the table for the base point. */
    static inline const Precomputed base() GOLDILOCKS_NOEXCEPT { return Precomputed(); }

public:
    /** @cond internal */
    friend class OwnedOrUnowned<Precomputed,Precomputed_U>;
    static inline size_t size() GOLDILOCKS_NOEXCEPT { return goldilocks_448_sizeof_precomputed_s; }
    static inline size_t alignment() GOLDILOCKS_NOEXCEPT { return goldilocks_448_alignof_precomputed_s; }
    static inline const Precomputed_U * default_value() GOLDILOCKS_NOEXCEPT { return goldilocks_448_precomputed_base; }
    /** @endcond */
};

/** X-only Diffie-Hellman ladder functions */
struct DhLadder {
public:
    /** Bytes in an X448 public key. */
    static const size_t PUBLIC_BYTES = GOLDILOCKS_X448_PUBLIC_BYTES;

    /** Bytes in an X448 private key. */
    static const size_t PRIVATE_BYTES = GOLDILOCKS_X448_PRIVATE_BYTES;

    /** Base point for a scalar multiplication. */
    static const FixedBlock<PUBLIC_BYTES> base_point() GOLDILOCKS_NOEXCEPT {
        return FixedBlock<PUBLIC_BYTES>(goldilocks_x448_base_point);
    }

    /** Calculate and return a shared secret with public key.  */
    static inline SecureBuffer shared_secret(
        const FixedBlock<PUBLIC_BYTES> &pk,
        const FixedBlock<PRIVATE_BYTES> &scalar
    ) /*throw(std::bad_alloc,CryptoException)*/ {
        SecureBuffer out(PUBLIC_BYTES);
        if (GOLDILOCKS_SUCCESS != goldilocks_x448(out.data(), pk.data(), scalar.data())) {
            throw CryptoException();
        }
        return out;
    }

    /** Calculate and write into out a shared secret with public key, noexcept version.  */
    static inline goldilocks_error_t GOLDILOCKS_WARN_UNUSED
    shared_secret_noexcept (
        FixedBuffer<PUBLIC_BYTES> &out,
        const FixedBlock<PUBLIC_BYTES> &pk,
        const FixedBlock<PRIVATE_BYTES> &scalar
    ) GOLDILOCKS_NOEXCEPT {
       return goldilocks_x448(out.data(), pk.data(), scalar.data());
    }

    /** Calculate and return a public key; equivalent to shared_secret(base_point(),scalar)
     * but possibly faster.
     */
    static inline SecureBuffer derive_public_key(
        const FixedBlock<PRIVATE_BYTES> &scalar
    ) /*throw(std::bad_alloc)*/ {
        SecureBuffer out(PUBLIC_BYTES);
        goldilocks_x448_derive_public_key(out.data(), scalar.data());
        return out;
    }

    /** Calculate and return a public key into a fixed buffer;
     * equivalent to shared_secret(base_point(),scalar) but possibly faster.
     */
    static inline void
    derive_public_key_noexcept (
        FixedBuffer<PUBLIC_BYTES> &out,
        const FixedBlock<PRIVATE_BYTES> &scalar
    ) GOLDILOCKS_NOEXCEPT {
        goldilocks_x448_derive_public_key(out.data(), scalar.data());
    }
};

}; /* struct Ed448Goldilocks */

/** @cond internal */
inline SecureBuffer Ed448Goldilocks::Scalar::direct_scalarmul (
    const FixedBlock<Ed448Goldilocks::Point::SER_BYTES> &in,
    goldilocks_bool_t allow_identity,
    goldilocks_bool_t short_circuit
) const /*throw(CryptoException)*/ {
    SecureBuffer out(Ed448Goldilocks::Point::SER_BYTES);
    if (GOLDILOCKS_SUCCESS !=
        goldilocks_448_direct_scalarmul(out.data(), in.data(), s, allow_identity, short_circuit)
    ) {
        throw CryptoException();
    }
    return out;
}

inline goldilocks_error_t Ed448Goldilocks::Scalar::direct_scalarmul_noexcept (
    FixedBuffer<Ed448Goldilocks::Point::SER_BYTES> &out,
    const FixedBlock<Ed448Goldilocks::Point::SER_BYTES> &in,
    goldilocks_bool_t allow_identity,
    goldilocks_bool_t short_circuit
) const GOLDILOCKS_NOEXCEPT {
    return goldilocks_448_direct_scalarmul(out.data(), in.data(), s, allow_identity, short_circuit);
}
/** @endcond */



#undef GOLDILOCKS_NOEXCEPT
} /* namespace goldilocks */

#endif /* __GOLDILOCKS_POINT_448_HXX__ */
