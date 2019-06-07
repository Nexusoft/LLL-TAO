/*
 * Internal functions for Falcon. These function are not meant to be
 * called by applications directly.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2017  Falcon Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @author   Thomas Pornin <thomas.pornin@nccgroup.trust>
 */

#ifndef FALCON_INTERNAL_H__
#define FALCON_INTERNAL_H__

#include <stdlib.h>
#include <string.h>

#include "shake.h"
#include "falcon.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * On MSVC, disable warning about applying unary minus on an unsigned
 * type: this is perfectly defined standard behaviour and we do it
 * quite often.
 */
#if _MSC_VER
#pragma warning( disable : 4146)
#endif

/*
 * This macro is defined to a non-zero value on systems which are known
 * to use little-endian encoding and to tolerate unaligned accesses.
 */
#ifndef FALCON_LE_U
#if __i386__ || _M_IX86 \
	|| __x86_64__ || _M_X64 || \
	(_ARCH_PWR8 && (__LITTLE_ENDIAN || __LITTLE_ENDIAN__))
#define FALCON_LE_U   1
#else
#define FALCON_LE_U   0
#endif
#endif

/* ==================================================================== */
/*
 * Encoding/decoding functions (falcon-enc.c).
 */

/*
 * Encode a ring element into bytes. This function is used when q = 12289;
 * each coordinate is encoded over 14 bits.
 *
 * Returned value is the encoded length, in bytes. If 'out' is NULL, then
 * the encoded length is still computed and returned, but no output is
 * produced; in that case, 'max_out_len' is ignored. Otherwise, bytes
 * are written into the buffer pointed to by 'out', up to 'max_out_len'
 * bytes; if the output buffer is too small (total encoded length would
 * exceed the value of 'max_out_len') then 0 is returned.
 */
size_t falcon_encode_12289(void *out, size_t max_out_len,
	const uint16_t *x, unsigned logn);

/*
 * Decode a ring element, using q = 12289. The encoded length (in bytes)
 * is returned. If the source value is incorrect (too short or otherwise
 * invalid) then this function returns 0.
 */
size_t falcon_decode_12289(uint16_t *x, unsigned logn,
	const void *data, size_t len);

/*
 * Encode a ring element into bytes. This function is used when q = 18433;
 * each coordinate is encoded over 15 bits.
 *
 * Note that length of x[] is 1.5*2^logn.
 *
 * Returned value is the encoded length, in bytes. If 'out' is NULL, then
 * the encoded length is still computed and returned, but no output is
 * produced; in that case, 'max_out_len' is ignored. Otherwise, bytes
 * are written into the buffer pointed to by 'out', up to 'max_out_len'
 * bytes; if the output buffer is too small (total encoded length would
 * exceed the value of 'max_out_len') then 0 is returned.
 */
size_t falcon_encode_18433(void *out, size_t max_out_len,
	const uint16_t *x, unsigned logn);

/*
 * Decode a ring element, using q = 18433. The encoded length (in bytes)
 * is returned. If the source value is incorrect (too short or otherwise
 * invalid) then this function returns 0.
 *
 * Note that length of x[] is 1.5*2^logn.
 */
size_t falcon_decode_18433(uint16_t *x, unsigned logn,
	const void *data, size_t len);

/*
 * Encode a "small vector" into bytes.
 *
 * 'comp' is the compression type:
 *    0   uncompressed
 *    1   compressed with sort-of Huffman codes
 * Other values are reserved and trigger an error.
 *
 * Returned value is the encoded length, in bytes. If 'out' is NULL, then
 * the encoded length is still computed and returned, but no output is
 * produced; in that case, 'max_out_len' is ignored.
 *
 * Returned value is 0 on error. Possible error conditions are:
 *
 *  - At least one of the source values cannot be encoded (not in the
 *    supported range for the specified encoding type).
 *
 *  - Output buffer is too small (this may happen only if 'out' is not NULL).
 */
size_t falcon_encode_small(void *out, size_t max_out_len,
	int comp, unsigned q, const int16_t *x, unsigned logn);

/*
 * Decode a "small vector". The encoded length (in bytes) is returned. If
 * the source value is incorrect (too short or otherwise invalid), then
 * this function returns 0.
 */
size_t falcon_decode_small(int16_t *x, unsigned logn,
	int comp, unsigned q, const void *data, size_t len);

/*
 * From a SHAKE context (must be already flipped), produce a new point.
 */
void falcon_hash_to_point(shake_context *sc, unsigned q,
	uint16_t *x, unsigned logn);

/*
 * Tell whether a given vector (2N coordinates, in two halves) is
 * acceptable as a signature. This compares the appropriate norm of the
 * vector with the acceptance bound. Returned value is 1 on success
 * (vector is short enough to be acceptable), 0 otherwise.
 */
int falcon_is_short(const int16_t *s1, const int16_t *s2,
	unsigned logn, unsigned ter);

/* ==================================================================== */
/*
 * Signature verification functions (falcon-vrfy.c).
 */

/*
 * Internal signature verification code:
 *   c0[]      contains the hashed message
 *   s2[]      is the decoded signature
 *   h[]       contains the public key, in NTT + Montgomery format
 *   logn      is the degree log
 *   ternary   1 for the ternary case, 0 for binary
 * Returned value is 1 on success, 0 on error.
 */
int falcon_vrfy_verify_raw(const uint16_t *c0, const int16_t *s2,
	const uint16_t *h, unsigned logn, int ternary);

/*
 * Compute the public key h[], given the private key elements f[] and
 * g[]. This computes h = g/f mod phi mod q, where phi is the polynomial
 * modulus. This function returns 1 on success, 0 on error (an error is
 * reported if f is not invertible mod phi mod q).
 */
int falcon_compute_public(uint16_t *h,
	const int16_t *f, const int16_t *g, unsigned logn, int ternary);

/*
 * Recompute the fourth private key element. Private key consists in
 * four polynomials with small coefficients f, g, F and G, which are
 * such that fG - gF = q mod phi; further more, f is invertible modulo
 * phi and modulo q. This function recomputes G from f, g and F.
 *
 * Returned value is 1 in success, 0 on error (f not invertible).
 */
int falcon_complete_private(int16_t *G,
	const int16_t *f, const int16_t *g, const int16_t *F,
	unsigned logn, int ternary);

/* ==================================================================== */
/*
 * Implementation of floating-point real numbers.
 */

/*
 * Real numbers are implemented by an extra header file, included below.
 * This is meant to support pluggable implementations. The default
 * implementation relies on the C type 'double'.
 *
 * The included file must define the following types and inline functions:
 *
 *   fpr
 *         type for a real number
 *
 *   fpr fpr_of(int64_t i)
 *         cast an integer into a real number
 *
 *   fpr fpr_scaled(int64_t i, int sc)
 *         compute i*2^sc as a real number
 *
 *   fpr fpr_inverse_of(long i)
 *         compute 1/i, as a real number
 *
 *   fpr fpr_log2
 *         constant value, equal to the logarithm of 2
 *
 *   fpr fpr_p55
 *         constant value, equal to 2 to the power 55
 *
 *   int64_t fpr_rint(fpr x)
 *         round x to the nearest integer
 *
 *   long fpr_floor(fpr x)
 *         round down to the previous integer
 *
 *   fpr fpr_add(fpr x, fpr y)
 *         compute x + y
 *
 *   fpr fpr_sub(fpr x, fpr y)
 *         compute x - y
 *
 *   fpr fpr_neg(fpr x)
 *         compute -x
 *
 *   fpr fpr_half(fpr x)
 *         compute x/2
 *
 *   fpr fpr_double(fpr x)
 *         compute x*2
 *
 *   fpr fpr_mul(fpr x, fpr y)
 *         compute x * y
 *
 *   fpr fpr_sqr(fpr x)
 *         compute x * x
 *
 *   fpr fpr_inv(fpr x)
 *         compute 1/x
 *
 *   fpr fpr_div(fpr x, fpr y)
 *         compute x/y
 *
 *   fpr fpr_sqrt(fpr x)
 *         compute the square root of x
 *
 *   fpr fpr_max(fpr x, fpr y)
 *         compute the greater of x and y
 *
 *   int fpr_lt(fpr x, fpr y)
 *         return 1 if x < y, 0 otherwise
 *
 *   fpr fpr_exp_small(fpr x)
 *         return exp(x), assuming that |x| <= log(2). This function must
 *         have a precision of at least 50 bits.
 *
 *   void fpr_gauss(fpr *re, fpr *im, fpr sigma, uint32_t a, uint32_t b)
 *         compute re and im such that:
 *            re+i*im = sigma*sqrt(-2*ln(x)) * exp(i*2*pi*y)
 *         where x = (a + 1) / 2^32 and y = (b + 1) / 2^32.
 */
#ifndef FPR_IMPL
#define FPR_IMPL   "fpr-double.h"
#endif
#include FPR_IMPL

/* ==================================================================== */
/*
 * FFT (falcon-fft.c).
 *
 * A real polynomial is represented as an array of N 'fpr' elements.
 * The FFT representation of a real polynomial contains N/2 complex
 * elements; each is stored as two real numbers, for the real and
 * imaginary parts, respectively. See falcon-fft.c for details on the
 * internal representation.
 */

/*
 * Compute FFT in-place: the source array should contain a real
 * polynomial (N coefficients); its storage area is reused to store
 * the FFT representation of that polynomial (N/2 complex numbers).
 *
 * 'logn' MUST lie between 1 and 10 (inclusive).
 */
void falcon_FFT(fpr *f, unsigned logn);

/*
 * Compute the inverse FFT in-place: the source array should contain the
 * FFT representation of a real polynomial (N/2 elements); the resulting
 * real polynomial (N coefficients of type 'fpr') is written over the
 * array.
 *
 * 'logn' MUST lie between 1 and 10 (inclusive).
 */
void falcon_iFFT(fpr *f, unsigned logn);

/*
 * Add polynomial b to polynomial a. a and b MUST NOT overlap. This
 * function works in normal representation.
 */
void falcon_poly_add(fpr *restrict a, const fpr *restrict b, unsigned logn);

/*
 * Add polynomial b to polynomial a. a and b MUST NOT overlap. This
 * function works in FFT representation.
 */
#define falcon_poly_add_fft   falcon_poly_add

/*
 * Add a constant value to a polynomial. This function works in normal
 * representation.
 */
void falcon_poly_addconst(fpr *a, fpr x, unsigned logn);

/*
 * Add a constant value to a polynomial. This function works in FFT
 * representation.
 */
void falcon_poly_addconst_fft(fpr *a, fpr x, unsigned logn);

/*
 * Subtract polynomial b from polynomial a. a and b MUST NOT overlap. This
 * function works in normal representation.
 */
void falcon_poly_sub(fpr *restrict a, const fpr *restrict b, unsigned logn);

/*
 * Subtract polynomial b from polynomial a. a and b MUST NOT overlap. This
 * function works in FFT representation.
 */
#define falcon_poly_sub_fft   falcon_poly_sub

/*
 * Negate polynomial a. This function works in normal representation.
 */
void falcon_poly_neg(fpr *a, unsigned logn);

/*
 * Negate polynomial a. This function works in FFT representation.
 */
#define falcon_poly_neg_fft   falcon_poly_neg

/*
 * Compute adjoint of polynomial a. This function works in normal
 * representation.
 */
void falcon_poly_adj(fpr *a, unsigned logn);

/*
 * Compute adjoint of polynomial a. This function works in FFT representation.
 */
void falcon_poly_adj_fft(fpr *a, unsigned logn);

/*
 * We do not define falcon_poly_mul() because implementation must use
 * extra temporary storage.
 *
void falcon_poly_mul(fpr *restrict a, const fpr *restrict b, unsigned logn);
 */

/*
 * Multiply polynomial a with polynomial b. a and b MUST NOT overlap.
 * This function works in FFT representation.
 */
void falcon_poly_mul_fft(fpr *restrict a, const fpr *restrict b, unsigned logn);

/*
 * We do not define falcon_poly_sqr() because implementation must use
 * extra temporary storage.
 *
void falcon_poly_sqr(fpr *restrict a, const fpr *restrict b, unsigned logn);
 */

/*
 * Square a polynomial. This function works in FFT representation.
 */
void falcon_poly_sqr_fft(fpr *a, unsigned logn);

/*
 * Multiply polynomial a with the adjoint of polynomial b. a and b MUST NOT
 * overlap. This function works in FFT representation.
 */
void falcon_poly_muladj_fft(fpr *restrict a,
	const fpr *restrict b, unsigned logn);

/*
 * Multiply polynomial with its own adjoint. This function works in FFT
 * representation.
 */
void falcon_poly_mulselfadj_fft(fpr *a, unsigned logn);

/*
 * Multiply polynomial with a real constant. This function works in normal
 * representation.
 */
void falcon_poly_mulconst(fpr *a, fpr x, unsigned logn);

/*
 * Multiply polynomial with a real constant. This function works in FFT
 * representation.
 */
#define falcon_poly_mulconst_fft   falcon_poly_mulconst

/*
 * Invert a polynomial modulo X^N+1 (FFT representation).
 */
void falcon_poly_inv_fft(fpr *a, unsigned logn);

/*
 * Divide polynomial a by polynomial b, modulo X^N+1 (FFT representation).
 * a and b MUST NOT overlap.
 */
void falcon_poly_div_fft(fpr *restrict a, const fpr *restrict b, unsigned logn);

/*
 * Divide polynomial a by polynomial adj(b), modulo X^N+1 (FFT
 * representation). a and b MUST NOT overlap.
 */
void falcon_poly_divadj_fft(fpr *restrict a,
	const fpr *restrict b, unsigned logn);

/*
 * Given f and g (in FFT representation), compute 1/(f*adj(f)+g*adj(g))
 * (also in FFT representation). Since the result is auto-adjoint, all its
 * coordinates in FFT representation are real; as such, only the first N/2
 * values of d[] are filled (the imaginary parts are skipped).
 *
 * Array d MUST NOT overlap with either a or b.
 */
void falcon_poly_invnorm2_fft(fpr *restrict d,
	const fpr *restrict a, const fpr *restrict b, unsigned logn);

/*
 * Given F, G, f and g (in FFT representation), compute F*adj(f)+G*adj(g)
 * (also in FFT representation). Destination d MUST NOT overlap with
 * any of the source arrays.
 */
void falcon_poly_add_muladj_fft(fpr *restrict d,
	const fpr *restrict F, const fpr *restrict G,
	const fpr *restrict f, const fpr *restrict g, unsigned logn);

/*
 * Multiply polynomial a by polynomial b, where b is autoadjoint. Both
 * a and b are in FFT representation. Since b is autoadjoint, all its
 * FFT coefficients are real, and the array b contains only N/2 elements.
 * a and b MUST NOT overlap.
 */
void falcon_poly_mul_autoadj_fft(fpr *restrict a,
	const fpr *restrict b, unsigned logn);

/*
 * Divide polynomial a by polynomial b, where b is autoadjoint. Both
 * a and b are in FFT representation. Since b is autoadjoint, all its
 * FFT coefficients are real, and the array b contains only N/2 elements.
 * a and b MUST NOT overlap.
 */
void falcon_poly_div_autoadj_fft(fpr *restrict a,
	const fpr *restrict b, unsigned logn);

/*
 * Apply "split" operation on a polynomial in FFT representation:
 * f = f0(x^2) + x*f1(x^2), for half-size polynomials f0 and f1
 * (polynomials modulo X^(N/2)+1). f0, f1 and f MUST NOT overlap.
 */
void falcon_poly_split_fft(fpr *restrict t0, fpr *restrict t1,
	const fpr *restrict f, unsigned logn);

/*
 * Apply "merge" operation on two polynomials in FFT representation:
 * given f0 and f1, polynomials moduo X^(N/2)+1, this function computes
 * f = f0(x^2) + x*f1(x^2), in FFT representation modulo X^N+1.
 * f MUST NOT overlap with either f0 or f1.
 */
void falcon_poly_merge_fft(fpr *restrict f,
	const fpr *restrict f0, const fpr *restrict f1, unsigned logn);

/*
 * FFT3 (falcon-fft.c).
 *
 * The 'FFT3' functions are for polynomials modulo X^N - X^(N/2) + 1.
 * The degree N is provided as two parameters: 'logn' and 'full'. If
 * 'full' is 0, then the degree is 2^logn. If 'full' is 1, then
 * the degree is 1.5*2^logn. The 'full' value MUST be 0 or 1 (other
 * non-zero values are not tolerated).
 *
 * When full = 0, logn must be between 1 and 8 (degree 2 to 256).
 * When full = 1, logn must be between 2 and 9 (degree 6 to 768).
 */

/*
 * Compute FFT3 in-place: the source array should contain a real
 * polynomial (N coefficients); its storage area is reused to store
 * the FFT3 representation of that polynomial (N/2 complex numbers).
 *
 * 'logn' MUST lie between 2 and 9 (inclusive).
 */
void falcon_FFT3(fpr *f, unsigned logn, unsigned full);

/*
 * Compute the inverse FFT3 in-place: the source array should contain the
 * FFT3 representation of a real polynomial (N/2 elements); the resulting
 * real polynomial (N coefficients of type 'fpr') is written over the
 * array.
 *
 * 'logn' MUST lie between 2 and 9 (inclusive).
 */
void falcon_iFFT3(fpr *f, unsigned logn, unsigned full);

/*
 * Add polynomial b to polynomial a. a and b MUST NOT overlap. This
 * function works in normal representation.
 */
void falcon_poly_add3(fpr *restrict a, const fpr *restrict b,
	unsigned logn, unsigned full);

/*
 * Add polynomial b to polynomial a. a and b MUST NOT overlap. This
 * function works in FFT3 representation.
 */
#define falcon_poly_add_fft3   falcon_poly_add3

/*
 * Add a constant value to a polynomial. This function works in normal
 * representation.
 */
void falcon_poly_addconst3(fpr *a, fpr x, unsigned logn, unsigned full);

/*
 * Add a constant value to a polynomial. This function works in FFT3
 * representation.
 */
void falcon_poly_addconst_fft3(fpr *a, fpr x, unsigned logn, unsigned full);

/*
 * Subtract polynomial b from polynomial a. a and b MUST NOT overlap. This
 * function works in normal representation.
 */
void falcon_poly_sub3(fpr *restrict a, const fpr *restrict b,
	unsigned logn, unsigned full);

/*
 * Subtract polynomial b from polynomial a. a and b MUST NOT overlap. This
 * function works in FFT3 representation.
 */
#define falcon_poly_sub_fft3   falcon_poly_sub3

/*
 * Negate polynomial a. This function works in normal representation.
 */
void falcon_poly_neg3(fpr *a, unsigned logn, unsigned full);

/*
 * Negate polynomial a. This function works in FFT3 representation.
 */
#define falcon_poly_neg_fft3   falcon_poly_neg3

/*
 * Compute adjoint of polynomial a. This function works in FFT3 representation.
 */
void falcon_poly_adj_fft3(fpr *a, unsigned logn, unsigned full);

/*
 * We do not define falcon_poly_mul3() because implementation must use
 * extra temporary storage.
 *
void falcon_poly_mul3(fpr *restrict a, const fpr *restrict b,
	unsigned logn, unsigned full);
 */

/*
 * Multiply polynomial a with polynomial b. a and b MUST NOT overlap.
 * This function works in FFT3 representation.
 */
void falcon_poly_mul_fft3(fpr *restrict a,
	const fpr *restrict b, unsigned logn, unsigned full);

/*
 * We do not define falcon_poly_sqr() because implementation must use
 * extra temporary storage.
 *
void falcon_poly_sqr3(fpr *restrict a, const fpr *restrict b,
	unsigned logn, unsigned full);
 */

/*
 * Square a polynomial. This function works in FFT3 representation.
 */
void falcon_poly_sqr_fft3(fpr *a, unsigned logn, unsigned full);

/*
 * Multiply polynomial a with the adjoint of polynomial b. a and b MUST NOT
 * overlap. This function works in FFT3 representation.
 */
void falcon_poly_muladj_fft3(fpr *restrict a,
	const fpr *restrict b, unsigned logn, unsigned full);

/*
 * Multiply polynomial with its own adjoint. This function works in FFT3
 * representation.
 */
void falcon_poly_mulselfadj_fft3(fpr *a, unsigned logn, unsigned full);

/*
 * Multiply polynomial with a real constant. This function works in normal
 * representation.
 */
void falcon_poly_mulconst3(fpr *a, fpr x, unsigned logn, unsigned full);

/*
 * Multiply polynomial with a real constant. This function works in FFT3
 * representation.
 */
#define falcon_poly_mulconst_fft3   falcon_poly_mulconst3

/*
 * Invert a polynomial modulo X^N-X^(N/2)+1 (FFT3 representation).
 */
void falcon_poly_inv_fft3(fpr *a, unsigned logn, unsigned full);

/*
 * Divide polynomial a by polynomial b, modulo X^N-X^(N/2)+1 (FFT3
 * representation). a and b MUST NOT overlap.
 */
void falcon_poly_div_fft3(fpr *restrict a,
	const fpr *restrict b, unsigned logn, unsigned full);

/*
 * Divide polynomial a by polynomial adj(b), modulo X^N-X^(N/2)+1 (FFT3
 * representation). a and b MUST NOT overlap.
 */
void falcon_poly_divadj_fft3(fpr *restrict a,
	const fpr *restrict b, unsigned logn, unsigned full);

/*
 * Given f and g (in FFT representation), compute 1/(f*adj(f)+g*adj(g))
 * (also in FFT3 representation). Since the result is auto-adjoint, all its
 * coordinates in FFT3 representation are real; as such, only the first N/2
 * values of d[] are filled (the imaginary parts are skipped).
 *
 * Array d MUST NOT overlap with either a or b.
 */
void falcon_poly_invnorm2_fft3(fpr *restrict d,
	const fpr *restrict a, const fpr *restrict b,
	unsigned logn, unsigned full);

/*
 * Given F, G, f and g (in FFT3 representation), compute F*adj(f)+G*adj(g)
 * (also in FFT3 representation). Destination d MUST NOT overlap with
 * any of the source arrays.
 */
void falcon_poly_add_muladj_fft3(fpr *restrict d,
	const fpr *restrict F, const fpr *restrict G,
	const fpr *restrict f, const fpr *restrict g,
	unsigned logn, unsigned full);

/*
 * Multiply polynomial a by polynomial b, where b is autoadjoint. Both
 * a and b are in FFT3 representation. Since b is autoadjoint, all its
 * FFT3 coefficients are real, and the array b contains only N/2 elements.
 * a and b MUST NOT overlap.
 */
void falcon_poly_mul_autoadj_fft3(fpr *restrict a,
	const fpr *restrict b, unsigned logn, unsigned full);

/*
 * Divide polynomial a by polynomial b, where b is autoadjoint. Both
 * a and b are in FFT3 representation. Since b is autoadjoint, all its
 * FFT3 coefficients are real, and the array b contains only N/2 elements.
 * a and b MUST NOT overlap.
 */
void falcon_poly_div_autoadj_fft3(fpr *restrict a,
	const fpr *restrict b, unsigned logn, unsigned full);

/*
 * Apply "split" operation on a polynomial in FFT3 representation:
 * f = f0(x^3) + x*f1(x^3) + x^2*f2(x^3), for half-size polynomials f0,
 * f1 and f2. This function is for a f of degree N = 1.5*2^logn,
 * modulo X^N-X^(N/2)+1. f0, f1, f2 and f MUST NOT overlap. This function
 * requires that logn >= 2.
 */
void falcon_poly_split_top_fft3(
	fpr *restrict f0, fpr *restrict f1, fpr *restrict f2,
	const fpr *restrict f, unsigned logn);

/*
 * Apply "split" operation on a polynomial in FFT3 representation:
 * f = f0(x^2) + x*f1(x^2), for half-size polynomials f0 and f1.
 * This function is for a f of degree N = 2^logn, modulo X^N-X^(N/2)+1.
 * f0, f1 and f MUST NOT overlap. This function requires that logn >= 1.
 * If logn == 1, then *f0 and *f1 are filled with a single real value
 * each.
 */
void falcon_poly_split_deep_fft3(fpr *restrict f0, fpr *restrict f1,
	const fpr *restrict f, unsigned logn);

/*
 * Apply "merge" operation on three polynomials in FFT3 representation.
 * This is the opposite of falcon_poly_split_top_fft3().
 */
void falcon_poly_merge_top_fft3(fpr *restrict f,
	const fpr *restrict f0, const fpr *restrict f1, const fpr *restrict f2,
	unsigned logn);

/*
 * Apply "merge" operation on two polynomials in FFT3 representation.
 * This is the opposite of falcon_poly_split_deep_fft3().
 */
void falcon_poly_merge_deep_fft3(fpr *restrict f,
	const fpr *restrict f0, const fpr *restrict f1, unsigned logn);

/* ==================================================================== */
/*
 * RNG stuff. This is a generic API to get a random seed from the
 * underlying hardware and/or operating system, and to use it to power
 * an efficient PRNG. The PRNG algorithm itself will be selected based
 * on what the current platform can do.
 */

/*
 * Obtain a random seed from the system RNG.
 *
 * Returned value is 1 on success, 0 on error.
 */
int falcon_get_seed(void *seed, size_t seed_len);

/*
 * Structure for a PRNG. This includes a large buffer so that values
 * get generated in advance. The 'state' is used to keep the current
 * PRNG algorithm state (contents depend on the selected algorithm).
 *
 * The unions with 'dummy_u64' are there to ensure proper alignment for
 * 64-bit direct access.
 */
typedef struct {
	union {
		unsigned char d[4096];
		uint64_t dummy_u64;
	} buf;
	size_t ptr;
	union {
		unsigned char d[256];
		uint64_t dummy_u64;
	} state;
	int type;
} prng;

/*
 * PRNG types. Only PRNG_CHACHA20 is currently implemented.
 */
#define PRNG_CHACHA20        1
#define PRNG_CHACHA20_SSE2   2
#define PRNG_AES_X86NI       3

/*
 * Instantiate a PRNG. That PRNG will feed over the provided SHAKE
 * context (in "flipped" state) to obtain its initial state.
 * The 'type' parameter is the requested PRNG type; if set to 0, then
 * a default type (adapted to the current platform) will be used.
 *
 * Returned value is the used RNG type (non-zero) on success, or 0 on
 * error. An error occurs only if the requested PRNG type is not
 * available; this cannot happen if 'type' is 0.
 */
int falcon_prng_init(prng *p, shake_context *src, int type);

/*
 * Refill the PRNG buffer. This is normally invoked automatically, and
 * is declared here only so that falcon_prng_get_u64() may be inlined.
 */
void falcon_prng_refill(prng *p);

/*
 * Get some bytes from a PRNG.
 */
void falcon_prng_get_bytes(prng *p, void *dst, size_t len);

/*
 * Get a 64-bit random value from a PRNG.
 */
static inline uint64_t
falcon_prng_get_u64(prng *p)
{
	size_t u;

	/*
	 * If there are less than 9 bytes in the buffer, we refill it.
	 * This means that we may drop the last few bytes, but this allows
	 * for faster extraction code. Also, it means that we never leave
	 * an empty buffer.
	 */
	u = p->ptr;
	if(u >= (sizeof p->buf.d) - 9) {
		falcon_prng_refill(p);
		u = 0;
	}
	p->ptr = u + 8;

	/*
	 * On systems that use little-endian encoding and allow
	 * unaligned accesses, we can simply read the data where it is.
	 */
#if FALCON_LE_U
	return *(uint64_t *)(p->buf.d + u);
#else
	return (uint64_t)p->buf.d[u + 0]
		| ((uint64_t)p->buf.d[u + 1] << 8)
		| ((uint64_t)p->buf.d[u + 2] << 16)
		| ((uint64_t)p->buf.d[u + 3] << 24)
		| ((uint64_t)p->buf.d[u + 4] << 32)
		| ((uint64_t)p->buf.d[u + 5] << 40)
		| ((uint64_t)p->buf.d[u + 6] << 48)
		| ((uint64_t)p->buf.d[u + 7] << 56);
#endif
}

/*
 * Get an 8-bit random value from a PRNG.
 */
static inline unsigned
falcon_prng_get_u8(prng *p)
{
	unsigned v;

	v = p->buf.d[p->ptr ++];
	if(p->ptr == sizeof p->buf.d) {
		falcon_prng_refill(p);
	}
	return v;
}

/* ==================================================================== */

#ifdef __cplusplus
}
#endif

#endif
