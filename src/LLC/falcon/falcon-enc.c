/*
 * Encoding/decoding of keys and signatures.
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

#include "internal.h"

/*
 * Encoding rules
 * ==============
 *
 * In this description we number bits left-to-right, starting at 0. In
 * an octet (a "byte"), there are eight bits, the most significant being
 * leftmost (i.e. bit 0 here).
 *
 *
 * Ring elements, q = 12289
 * ------------------------
 *
 * A point coordinate is an integer between 0 and 12288 (inclusive), which
 * is encoded as a 14-bit sequence, again numbered 0 to 13, 0 being most
 * significant.
 *
 * An arbitrary point (e.g. public key H) is a sequence of values modulo
 * q. These are encoded in due order as sequences of 14 bits. These are
 * then concatenated, then split over bytes in left-to-right order. For
 * instance, if the first two coordinates are 11911 (10111010000111 in
 * binary) and 3446 (00110101110110 in binary), then the bit sequence
 * will be:
 *
 *   10111010000111 00110101110110 ...
 *
 * which gets resplit into eight-bit chunks as:
 *
 *   10111010 00011100 11010111 0110...
 *
 * so the three first bytes of the encoding will have values 186, 28
 * and 215, in that order.
 *
 * If the total bit length is not a multiple of 8, then extra padding
 * bits are added to complete the final bytes. These padding bits MUST
 * have value 0.
 *
 *
 * Ring elements, q = 18433
 * ------------------------
 *
 * For the ternary case, modulus q is 18433. Encoding is similar to
 * the case q = 12289, except that each value uses 15 bits instead
 * of 14.
 *
 * Also, array length is 1.5*2^logn in that case.
 *
 *
 * Small vectors
 * -------------
 *
 * "Small vectors" are private keys and signatures. They consist in
 * _signed_ integers with values close to 0. They are encoded into bytes
 * with a static code that approximates Huffman codes.
 */

/* see falcon-internal.h */
size_t
falcon_encode_12289(void *out, size_t max_out_len,
	const uint16_t *x, unsigned logn)
{
	unsigned char *buf;
	size_t n, u;
	uint32_t acc;
	int acc_len;

	n = (size_t)1 << logn;
	buf = out;
	u = 0;
	acc = 0;
	acc_len = 0;
	while(n > 0) {
		acc = (acc << 14) | (*x ++);
		n --;
		acc_len += 14;
		while(acc_len >= 8) {
			acc_len -= 8;
			if(out != NULL) {
				if(u >= max_out_len) {
					return 0;
				}
				buf[u] = (unsigned char)(acc >> acc_len);
			}
			u ++;
			acc &= (1U << acc_len) - 1U;
		}
	}
	if(acc_len > 0) {
		if(out != NULL) {
			if(u >= max_out_len) {
				return 0;
			}
			buf[u] = (unsigned char)(acc << (8 - acc_len));
		}
		u ++;
	}
	return u;
}

/* see falcon-internal.h */
size_t
falcon_decode_12289(uint16_t *x, unsigned logn, const void *data, size_t len)
{
	const unsigned char *buf;
	size_t n, u;
	uint32_t acc;
	int acc_len;

	n = (size_t)1 << logn;
	buf = data;
	u = 0;
	acc = 0;
	acc_len = 0;
	while(n > 0) {
		if(u >= len) {
			return 0;
		}
		acc = (acc << 8) | buf[u ++];
		acc_len += 8;
		if(acc_len >= 14) {
			uint32_t w;

			acc_len -= 14;
			w = acc >> acc_len;
			if(w >= 12289) {
				return 0;
			}
			*x ++ = (uint16_t)w;
			n --;
			acc &= (1U << acc_len) - 1U;
		}
	}
	if(acc != 0) {
		return 0;
	}
	return len;
}

/* see falcon-internal.h */
size_t
falcon_encode_18433(void *out, size_t max_out_len,
	const uint16_t *x, unsigned logn)
{
	unsigned char *buf;
	size_t n, u;
	uint32_t acc;
	int acc_len;

	n = (size_t)3 << (logn - 1);
	buf = out;
	u = 0;
	acc = 0;
	acc_len = 0;
	while(n > 0) {
		acc = (acc << 15) | (*x ++);
		n --;
		acc_len += 15;
		while(acc_len >= 8) {
			acc_len -= 8;
			if(out != NULL) {
				if(u >= max_out_len) {
					return 0;
				}
				buf[u] = (unsigned char)(acc >> acc_len);
			}
			u ++;
			acc &= (1U << acc_len) - 1U;
		}
	}
	if(acc_len > 0) {
		if(out != NULL) {
			if(u >= max_out_len) {
				return 0;
			}
			buf[u] = (unsigned char)(acc << (8 - acc_len));
		}
		u ++;
	}
	return u;
}

/* see falcon-internal.h */
size_t
falcon_decode_18433(uint16_t *x, unsigned logn, const void *data, size_t len)
{
	const unsigned char *buf;
	size_t n, u;
	uint32_t acc;
	int acc_len;

	n = (size_t)3 << (logn - 1);
	buf = data;
	u = 0;
	acc = 0;
	acc_len = 0;
	while(n > 0) {
		if(u >= len) {
			return 0;
		}
		acc = (acc << 8) | buf[u ++];
		acc_len += 8;
		if(acc_len >= 15) {
			uint32_t w;

			acc_len -= 15;
			w = acc >> acc_len;
			if(w >= 18433) {
				return 0;
			}
			*x ++ = (uint16_t)w;
			n --;
			acc &= (1U << acc_len) - 1U;
		}
	}
	if(acc != 0) {
		return 0;
	}
	return len;
}

/*
 * Encoding of a small vector, no compression, 16 bits per value.
 */
static size_t
compress_none(void *out, size_t max_out_len,
	unsigned q, const int16_t *x, unsigned logn)
{
	size_t len, u;
	unsigned char *buf;

	if(q == 12289) {
		len = (size_t)2 << logn;
	} else {
		len = (size_t)3 << logn;
	}
	if(out == NULL) {
		return len;
	}
	if(max_out_len < len) {
		return 0;
	}
	buf = out;
	for(u = 0; u < len; u += 2) {
		unsigned w;

		w = *x ++;
		buf[u + 0] = (unsigned char)(w >> 8);
		buf[u + 1] = (unsigned char)w;
	}
	return len;
}

/*
 * Encoding of a small vector, using (sort-of) Huffman codes.
 */
static size_t
compress_static(void *out, size_t max_out_len,
	unsigned q, const int16_t *x, unsigned logn)
{
	unsigned char *buf;
	size_t n, u;
	unsigned acc, mask;
	int acc_len, j;

	/*
	 * Let x be a value to encode. We first encode its sign as 1 bit
	 * (1 = negative, 0 = positive), and replace x with |x|.
	 * The low j bits are encoded as-is in the stream (number j depends
	 * on the modulus q); then we follow with x/(2^j) bits of value 0,
	 * and a final bit of value 1.
	 *
	 * We use j = 7 for q = 12289, j = 8 for q = 18433.
	 */

	if(q == 12289) {
		n = (size_t)1 << logn;
		j = 7;
	} else {
		n = (size_t)3 << (logn - 1);
		j = 8;
	}
	mask = (1U << j) - 1U;
	buf = out;
	u = 0;
	acc = 0;
	acc_len = 0;
	while(n > 0) {
		int w;
		unsigned lo;
		int ne;

		w = *x ++;
		n --;

		/*
		 * First part: 1 bit for sign, and then the 7 low bits of
		 * the absolute value of the integer.
		 */
		if(w < 0) {
			w = -w;
			lo = 1U << j;
		} else {
			lo = 0;
		}
		lo |= w & mask;
		ne = w >> j;
		acc = (acc << (j + 1)) | lo;
		acc_len += j + 1;
		while(acc_len >= 8) {
			acc_len -= 8;
			if(buf != NULL) {
				if(u >= max_out_len) {
					return 0;
				}
				buf[u] = (unsigned char)(acc >> acc_len);
			}
			u ++;
		}

		/*
		 * Second part: 'ne' bits of value 0, and one bit of value 1.
		 */
		while(ne -- >= 0) {
			acc <<= 1;
			acc += ((unsigned)ne >> 15) & 1;
			if(++ acc_len == 8) {
				if(buf != NULL) {
					if(u >= max_out_len) {
						return 0;
					}
					buf[u] = (unsigned char)acc;
				}
				u ++;
				acc_len = 0;
			}
		}
	}
	if(acc_len > 0) {
		if(buf != NULL) {
			if(u >= max_out_len) {
				return 0;
			}
			buf[u] = (unsigned char)(acc << (8 - acc_len));
		}
		u ++;
	}
	return u;
}

/* see falcon-internal.h */
size_t
falcon_encode_small(void *out, size_t max_out_len,
	int comp, unsigned q, const int16_t *x, unsigned logn)
{
	switch (comp) {
	case FALCON_COMP_NONE:
		return compress_none(out, max_out_len, q, x, logn);
	case FALCON_COMP_STATIC:
		return compress_static(out, max_out_len, q, x, logn);
	default:
		return 0;
	}
}

/*
 * Decoding of a small vector, no compression.
 */
static size_t
uncompress_none(int16_t *x, unsigned logn,
	unsigned q, const void *data, size_t len)
{
	size_t n, u;
	const unsigned char *buf;
	uint32_t hq, tq;

	if(q == 12289) {
		n = (size_t)1 << logn;
	} else {
		n = (size_t)3 << (logn - 1);
	}
	if(len < (n << 1)) {
		return 0;
	}

	/*
	 * hq = floor(q/2)
	 * tq = ceil(3q/2)
	 * (Remember that q is odd).
	 */
	hq = q >> 1;
	tq = hq + q + 1;

	buf = data;
	for(u = 0; u < n; u ++) {
		uint32_t w;
		long y;

		/*
		 * Decode the value as a 32-bit signed value (held in an
		 * unsigned integer).
		 */
		w = ((uint32_t)buf[(u << 1) + 0] << 8)
			| (uint32_t)buf[(u << 1) + 1];
		w |= -(w & 0x8000);

		/*
		 * Add q. The result must be greater than q/2, and lower
		 * than 3q/2.
		 */
		w += q;
		if(!(((hq - w) & (w - tq)) >> 31)) {
			return 0;
		}

		/*
		 * Assemble the signed value in a 'long'. Thanks to the
		 * verifications above, we know that the result will fit
		 * in an int16_t, so the cast is valid.
		 */
		y = (long)w - (long)q;
		x[u] = (int16_t)y;
	}
	return n << 1;
}

/*
 * Decoding of a small vector, compressed with fixed (sort-of) Huffman codes.
 */
static size_t
uncompress_static(int16_t *x, unsigned logn,
	unsigned q, const void *data, size_t len)
{
	const unsigned char *buf;
	size_t n, u, v;
	unsigned db, mask;
	unsigned db_len, j;

	if(q == 12289) {
		n = (size_t)1 << logn;
		j = 7;
	} else {
		n = (size_t)3 << (logn - 1);
		j = 8;
	}
	mask = (1U << j) - 1U;
	buf = data;
	u = 0;
	v = 0;
	db = 0;
	db_len = 0;
	for(;;) {
		unsigned sign;
		unsigned lo;
		unsigned ne;

		/*
		 * Read next byte(s) for the sign bit and the low j bits
		 * of the absolute value of the next integer.
		 */
		while(db_len <= j) {
			if(v >= len) {
				return 0;
			}
			db = (db << 8) + buf[v ++];
			db_len += 8;
		}
		sign = (db >> (db_len - 1)) & 1;
		db_len -= j + 1;
		lo = (db >> db_len) & mask;

		/*
		 * Read subsequent bits until next '1' (inclusive).
		 */
		for(ne = 0;; ne ++) {
			unsigned bit;

			if(db_len == 0) {
				if(v >= len) {
					return 0;
				}
				db = buf[v ++];
				db_len = 8;
			}
			db_len --;
			bit = (db >> db_len) & 1;
			if(bit) {
				break;
			}
		}

		/*
		 * Write value.
		 */
		if(ne > 255) {
			return 0;
		}
		lo += (ne << j);
		if(sign) {
			x[u] = -(int16_t)lo;
		} else {
			x[u] = (int16_t)lo;
		}
		if(++ u >= n) {
			/*
			 * Check that the remaining bits are all zero.
			 */
			if((db & ((1U << db_len) - 1U)) != 0) {
				return 0;
			}
			return v;
		}
	}
}

/* see falcon-internal.h */
size_t
falcon_decode_small(int16_t *x, unsigned logn,
	int comp, unsigned q, const void *data, size_t len)
{
	switch (comp) {
	case FALCON_COMP_NONE:
		return uncompress_none(x, logn, q, data, len);
	case FALCON_COMP_STATIC:
		return uncompress_static(x, logn, q, data, len);
	default:
		return 0;
	}
}

/* see falcon-internal.h */
void
falcon_hash_to_point(shake_context *sc, unsigned q, uint16_t *x, unsigned logn)
{
	/*
	 * TODO: make a constant-time version of this process, in case
	 * the signed data is confidential. We could generate twice as
	 * many values as necessary, then move proper values into place
	 * with logn passes that perform conditional swaps.
	 */

	size_t n;
	uint32_t lim;

	if(q == 12289) {
		n = (size_t)1 << logn;
	} else {
		n = (size_t)3 << (logn - 1);
	}
	lim = (uint32_t)65536 - ((uint32_t)65536 % q);
	while(n > 0) {
		unsigned char buf[2];
		uint32_t w;

		shake_extract(sc, buf, sizeof buf);
		w = (buf[0] << 8) | buf[1];
		if(w < lim) {
			*x ++ = (uint16_t)(w % q);
			n --;
		}
	}
}

/* see falcon-internal.h */
int
falcon_is_short(const int16_t *s1, const int16_t *s2,
	unsigned logn, unsigned ter)
{
	if(ter) {
		/*
		 * In the ternary case, we must compute the norm in
		 * the FFT embedding. Fortunately, this can be done
		 * without computing the FFT.
		 */
		size_t n, hn, u;
		int64_t s;

		n = (size_t)3 << (logn - 1);
		hn = n >> 1;
		s = 0;
		for(u = 0; u < n; u ++) {
			int32_t z;

			z = s1[u];
			s += z * z;
			z = s2[u];
			s += z * z;
		}
		for(u = 0; u < hn; u ++) {
			s += (int32_t)s1[u] * (int32_t)s1[u + hn];
			s += (int32_t)s2[u] * (int32_t)s2[u + hn];
		}

		/*
		 * If the norm is v, then we computed (v^2)/N (in s).
		 */

		/*
		 * Acceptance bound on the embedding norm is:
		 *   b = 1.2*1.32*2*N*sqrt(q/sqrt(2))
		 *
		 * Since we computed (v^2)/N, we must compare it with:
		 *   (b^2)/N = ((1.2*1.32*2)^2/sqrt(2))*q*N
		 *           = (3*(1.2*1.32*2)^2/sqrt(2))*q*2^(logn-1)
		 *
		 * We use 100464491 = floor((b^2)/N) when N = 768, and
		 * scale it down for lower dimensions.
		 */
		return s < (int64_t)((uint32_t)100464491 >> (9 - logn));
	} else {
		/*
		 * In the binary case, we use the l2-norm. Code below
		 * uses only 32-bit operations to compute the square
		 * of the norm with saturation to 2^32-1 if the value
		 * exceeds 2^31-1.
		 */
		size_t n, u;
		uint32_t s, ng;

		n = (size_t)1 << logn;
		s = 0;
		ng = 0;
		for(u = 0; u < n; u ++) {
			int32_t z;

			z = s1[u];
			s += (uint32_t)(z * z);
			ng |= s;
			z = s2[u];
			s += (uint32_t)(z * z);
			ng |= s;
		}
		s |= -(ng >> 31);

		/*
		 * Acceptance bound on the l2-norm is:
		 *   1.2*1.55*sqrt(q)*sqrt(2*N)
		 * Value 7085 is floor((1.2^2)*(1.55^2)*2*1024).
		 */
		return s < (((uint32_t)7085 * (uint32_t)12289) >> (10 - logn));
	}
}
