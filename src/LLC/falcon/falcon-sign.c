/*
 * Falcon signature generation.
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
 * If CLEANSE is non-zero, then temporary areas obtained with malloc()
 * and used to contain secret values are explicitly cleared before
 * deallocation with free(). This is the default behaviour; use
 * -DCLEANSE=0 to disable cleansing.
 */
#ifndef CLEANSE
#define CLEANSE   1
#endif

/*
 * If SAMPLER_CODF is non-zero, then the discrete Gaussian sampler will
 * use a CoDF table and a variable number of PRNG invocations; use
 * -DSAMPLER_CODF (or -DSAMPLER_CODF=1) to enable this code.
 */
#ifndef SAMPLER_CODF
#define SAMPLER_CODF   0
#endif

/*
 * If SAMPLER_CDF is non-zero, then the discrete Gaussian sampler will
 * use a tabulated distribution with 128 bits of precision and a constant 
 * number of PRNG invocations; use -DSAMPLER_CDF (or -DSAMPLER_CDF=1)
 * to enable this code. The default Gaussian sampler uses a tabulated 
 * distribution with 136 bits of precision and a variable number of PRNG
 * invocations.
 */
#ifndef SAMPLER_CDF
#define SAMPLER_CDF   0
#endif

/*
 * If CT_BEREXP is non-zero, then the rejection phase on the discrete
 * Gaussian sampler will use an algorithm with a constant number of PRNG
 * invocations (which should help with achieving constant-time code);
 * use -DCT_BEREXP (or -DCT_BEREXP=1) to enable this code. The default
 * rejection algorithm uses a lazy method with a variable number of PRNG
 * invocations, which help reducing the PRNG overall cost.
 */
#ifndef CT_BEREXP
#define CT_BEREXP   0
#endif

/* =================================================================== */

/*
 * Compute degree N from logarithm 'logn', and ternary flag 'ter'.
 * 'ter' MUST be 0 or 1.
 */
#define MKN(logn, ter)   ((size_t)(1 + ((ter) << 1)) << ((logn) - (ter)))

/* =================================================================== */
/*
 * Binary case:
 *   N = 2^logn
 *   phi = X^N+1
 */

/*
 * Compute LDL decomposition on an auto-adjoint 2x2 matrix G. Matrix
 * elements are real polynomials modulo X^n+1:
 *   g00   top-left element
 *   g01   top-right element
 *   g10   bottom-left element
 *   g11   bottom-right element
 *
 * The matrix being auto-adjoint means that G = G*. Thus, adj(g00) == g00,
 * adj(g11) == g11, adj(g10) == g01, and adj(g01) == g10.
 * Since g10 and g01 are redundant, g10 is not provided as parameter.
 *
 * LDL decomposition is:
 *   G = L·D·L*
 * where L is lower-triangular with 1 on the diagonal, and D is diagonal.
 * The top-left element of D is equal to g00, so it is not returned;
 * outputs are thus d11 (lower-right element of D) and l10 (lower-left
 * element of L).
 *
 * The tmp[] buffer must be able to hold at least one polynomial.
 *
 * All operands are in FFT representation. No overlap allowed, except
 * between the constant inputs (g00, g01 and g11).
 */
static void
LDL_fft(fpr *restrict d11, fpr *restrict l10,
	const fpr *restrict g00, const fpr *restrict g01,
	const fpr *restrict g11, unsigned logn, fpr *restrict tmp)
{
	size_t n;

	n = MKN(logn, 0);

	/* Let tmp = mu = G[0,1] / G[0,0]. */
	memcpy(tmp, g01, n * sizeof *g01);
	falcon_poly_div_fft(tmp, g00, logn);

	/* Let L[1,0] = adj(mu) and tmp = aux = mu * adj(mu). */
	memcpy(l10, tmp, n * sizeof *tmp);
	falcon_poly_adj_fft(l10, logn);
	falcon_poly_mul_fft(tmp, l10, logn);

	/* D[1,1] = G[1,1] - aux * G[0][0]. */
	falcon_poly_mul_fft(tmp, g00, logn);
	memcpy(d11, g11, n * sizeof *g11);
	falcon_poly_sub_fft(d11, tmp, logn);
}

/*
 * Special case of LDL when G is quasicyclic, i.e. g11 == g00.
 */
static inline void
LDLqc_fft(fpr *restrict d11, fpr *restrict l10,
	const fpr *restrict g00, const fpr *restrict g01, unsigned logn,
	fpr *restrict tmp)
{
	LDL_fft(d11, l10, g00, g01, g00, logn, tmp);
}

/*
 * Get the size of the LDL tree for an input with polynomials of size
 * 2^logn. The size is expressed in the number of elements.
 */
static inline unsigned
ffLDL_treesize(unsigned logn)
{
	/*
	 * For logn = 0 (polynomials are constant), the "tree" is a
	 * single element. Otherwise, the tree node has size 2^logn, and
	 * has two child trees for size logn-1 each. Thus, treesize s()
	 * must fulfill these two relations:
	 *
	 *   s(0) = 1
	 *   s(logn) = (2^logn) + 2*s(logn-1)
	 */
	return (logn + 1) << logn;
}

/*
 * Inner function for ffLDL_fft(). It expects the matrix to be both
 * auto-adjoint and quasicyclic; also, it uses the source operands
 * as modifiable temporaries.
 *
 * tmp[] must have room for at least two polynomials.
 */
static void
ffLDL_fft_inner(fpr *restrict tree,
	fpr *restrict g0, fpr *restrict g1, unsigned logn, fpr *restrict tmp)
{
	size_t n, hn;

	n = MKN(logn, 0);
	if (n == 1) {
		tree[0] = g0[0];
		return;
	}
	hn = n >> 1;

	/*
	 * The LDL decomposition yields L (which is written in the tree)
	 * and the diagonal of D. Since d00 = g0, we just write d11
	 * into tmp.
	 */
	LDLqc_fft(tmp, tree, g0, g1, logn, tmp + n);

	/*
	 * Split d00 (currently in g0) and d11 (currently in tmp). We
	 * reuse g0 and g1 as temporary storage spaces:
	 *   d00 splits into g1, g1+hn
	 *   d11 splits into g0, g0+hn
	 */
	falcon_poly_split_fft(g1, g1 + hn, g0, logn);
	falcon_poly_split_fft(g0, g0 + hn, tmp, logn);

	/*
	 * Each split result is the first row of a new auto-adjoint
	 * quasicyclic matrix for the next recursive step.
	 */
	ffLDL_fft_inner(tree + n,
		g1, g1 + hn, logn - 1, tmp);
	ffLDL_fft_inner(tree + n + ffLDL_treesize(logn - 1),
		g0, g0 + hn, logn - 1, tmp);
}

/*
 * Compute the ffLDL tree of an auto-adjoint matrix G. The matrix
 * is provided as three polynomials (FFT representation).
 *
 * The "tree" array is filled with the computed tree, of size
 * (logn+1)*(2^logn) elements (see ffLDL_treesize()).
 *
 * Input arrays MUST NOT overlap, except possibly the three unmodified
 * arrays g00, g01 and g11. tmp[] should have room for at least four
 * polynomials of 2^logn elements each.
 */
static void
ffLDL_fft(fpr *restrict tree, const fpr *restrict g00,
	const fpr *restrict g01, const fpr *restrict g11,
	unsigned logn, fpr *restrict tmp)
{
	size_t n, hn;
	fpr *d00, *d11;

	n = MKN(logn, 0);
	if (n == 1) {
		tree[0] = g00[0];
		return;
	}
	hn = n >> 1;
	d00 = tmp;
	d11 = tmp + n;
	tmp += n << 1;

	memcpy(d00, g00, n * sizeof *g00);
	LDL_fft(d11, tree, g00, g01, g11, logn, tmp);

	falcon_poly_split_fft(tmp, tmp + hn, d00, logn);
	falcon_poly_split_fft(d00, d00 + hn, d11, logn);
	memcpy(d11, tmp, n * sizeof *tmp);
	ffLDL_fft_inner(tree + n,
		d11, d11 + hn, logn - 1, tmp);
	ffLDL_fft_inner(tree + n + ffLDL_treesize(logn - 1),
		d00, d00 + hn, logn - 1, tmp);
}

/*
 * Normalize an ffLDL tree: each leaf of value x is replaced with
 * sigma / sqrt(x).
 */
static void
ffLDL_binary_normalize(fpr *tree, fpr sigma, unsigned logn)
{
	/*
	 * TODO: make an iterative version.
	 */
	size_t n;

	n = MKN(logn, 0);
	if (n == 1) {
		tree[0] = fpr_div(sigma, fpr_sqrt(tree[0]));
	} else {
		ffLDL_binary_normalize(tree + n,
			sigma, logn - 1);
		ffLDL_binary_normalize(tree + n + ffLDL_treesize(logn - 1),
			sigma, logn - 1);
	}
}

/* =================================================================== */
/*
 * Ternary case:
 *   N = 1.5*2^logn
 *   phi = X^N-X^(N/2)+1
 *
 * When doing operations along the splitting tree, the "top" operation
 * divides the degree by 3, while "deep" operations halve the degree.
 *
 * At the top-level, we perform a trisection:
 *
 *  - Input 2x2 Gram matrix is decomposed into its LDL representation:
 *    G = L·D·L*, where D is diagonal and L is low-triangular with
 *    only ones on its diagonal. Thus, there is one non-trivial element
 *    in L, and two non-trivial elements in D.
 *
 *  - Elements of D are each split into three elements of degree N/3.
 *
 *  - Two recursive invocations on 3x3 Gram matrices are performed.
 *
 * At the level immediately below:
 *
 *  - Input 3x3 Gram matrix is decomposed into LDL. We get three non-trivial
 *    elements in D (the diagonal), and three others in L (the lower
 *    triangle, excluding the diagonal). From these, we performs splits
 *    (that halve the degree) and build three 2x2 matrices for the recursive
 *    invocation.
 *
 * Lower levels receive a 2x2 Gram matrix, and perform 2-way splits.
 *
 * At the lowest level, polynomials are modulo X^2-X+1.
 */

/*
 * Perform LDL decomposition of a 2x2 Gram matrix.
 *
 * Input: matrix G = [[g00, g01], [g10, g11]] such that G* = G
 * (hence: adj(g00) = g00, adj(g11) = g11, adj(g01) = g10).
 *
 * Output: L = [[1, 0], [l10, 1]] and D = [[d00, 0], [0, d11]] such
 * that G = L·D·L*.
 *
 * We have d00 = g00, thus that value is omitted from the outputs.
 *
 * All inputs and outputs are in FFT3 representation.
 * Overlap allowed only between the constant inputs (g00, g10, g11).
 */
static void
LDL_dim2_fft3(fpr *restrict d11, fpr *restrict l10,
	const fpr *restrict g00, const fpr *restrict g10,
	const fpr *restrict g11, unsigned logn, unsigned full)
{
	size_t n;

	n = MKN(logn, full);

	/*
	 * Set l10 = g10/g00.
	 * Since g00 = adj(g00), FFT representation of g00 contains only
	 * real numbers.
	 */
	memcpy(l10, g10, n * sizeof *g10);
	falcon_poly_div_autoadj_fft3(l10, g00, logn, full);

	/*
	 * Set d11 = g11 - g10*adj(g10/g00).
	 * TODO: g11 = adj(g11), so it should be half-sized (only real
	 * numbers in FFT representation).
	 */
	memcpy(d11, g10, n * sizeof *g10);
	falcon_poly_muladj_fft3(d11, l10, logn, full);
	falcon_poly_neg_fft3(d11, logn, full);
	falcon_poly_add_fft3(d11, g11, logn, full);
}

/*
 * Perform LDL decomposition of a Gram 3x3 matrix.
 *
 * Input: matrix G = [[g00, g01, g02], [g10, g11, g12], [g20, g21, g22]]
 * such that G* = G.
 *
 * Output: L = [[1, 0, 0], [l10, 1, 0], [l20, l21, 1]], and
 * D = [[d00, 0, 0], [0, d11, 0], [0, 0, d22]] such that G = L·D·L*.
 *
 * We have d00 = g00, thus that value is omitted from the outputs.
 *
 * All inputs and outputs are in FFT3 representation.
 * Overlap allowed only between the constant inputs (g00, g10, g11).
 * tmp[] must have room for one polynomial.
 */
static void
LDL_dim3_fft3(fpr *restrict d11, fpr *restrict d22,
	fpr *restrict l10, fpr *restrict l20, fpr *restrict l21,
	const fpr *restrict g00, const fpr *restrict g10,
	const fpr *restrict g11, const fpr *restrict g20,
	const fpr *restrict g21, const fpr *restrict g22,
	unsigned logn, unsigned full, fpr *restrict tmp)
{
	size_t n;

	n = MKN(logn, full);

	/*
	 * l10 = g10/g00
	 * d11 = g11 - g10*adj(g10/g00)
	 * These are the same formulas as the LDL decomposition of a 2x2
	 * matrix.
	 */
	LDL_dim2_fft3(d11, l10, g00, g10, g11, logn, full);

	/*
	 * l20 = g20/g00
	 */
	memcpy(l20, g20, n * sizeof *g20);
	falcon_poly_div_autoadj_fft3(l20, g00, logn, full);

	/*
	 * l21 = (g21 - g20*adj(g10)/g00) / d11
	 * Note that d11 = adj(d11)
	 */
	memcpy(l21, g20, n * sizeof *g20);
	falcon_poly_muladj_fft3(l21, g10, logn, full);
	falcon_poly_div_autoadj_fft3(l21, g00, logn, full);
	falcon_poly_neg_fft3(l21, logn, full);
	falcon_poly_add_fft3(l21, g21, logn, full);
	falcon_poly_div_autoadj_fft3(l21, d11, logn, full);

	/*
	 * d22 = g22 - l20*adj(g20) - l21*adj(l21) / d11
	 */
	memcpy(d22, l20, n * sizeof *l20);
	falcon_poly_muladj_fft3(d22, g20, logn, full);
	falcon_poly_neg_fft3(d22, logn, full);
	falcon_poly_add_fft3(d22, g22, logn, full);
	memcpy(tmp, l21, n * sizeof *l21);
	falcon_poly_mulselfadj_fft3(tmp, logn, full);
	falcon_poly_mul_autoadj_fft3(tmp, d11, logn, full);
	falcon_poly_sub_fft3(d22, tmp, logn, full);
}

static size_t
ffLDL_inner_fft3(fpr *restrict tree, const fpr *restrict g00,
	const fpr *restrict g10, const fpr *restrict g11,
	unsigned logn, fpr *restrict tmp)
{
	size_t n, hn, s;
	fpr *t0, *t1, *t2;

	n = (size_t)1 << logn;
	hn = n >> 1;

	if (logn == 1) {
		/*
		 * When N = 2, diagonal elements (of D in the LDL
		 * decomposition) are real numbers (since they are
		 * auto-adjoint), and there is no need for further
		 * recursion.
		 *
		 * LDL_dim2_fft3() returns d11 (in tmp) and l10
		 * (two slots, in tree). It will be followed by two
		 * leaves, corresponding to d00 (which is g00) and d11.
		 * The two leaves are real numbers (one slot each).
		 */
		LDL_dim2_fft3(tmp, tree, g00, g10, g11, logn, 0);

		tree[2] = g00[0];
		tree[3] = tmp[0];
		return 4;
	}

	/*
	 * Do the LDL, split diagonal elements, and recurse.
	 * Since d00 = g00, we can do the first recursion
	 * before the LDL.
	 */
	s = n;
	t0 = tmp;
	t1 = tmp + hn;
	t2 = t1 + hn;

	falcon_poly_split_deep_fft3(t0, t1, g00, logn);
	falcon_poly_adj_fft3(t1, logn - 1, 0);
	s += ffLDL_inner_fft3(tree + s, t0, t1, t0, logn - 1, t2);

	LDL_dim2_fft3(t2, tree, g00, g10, g11, logn, 0);

	falcon_poly_split_deep_fft3(t0, t1, t2, logn);
	falcon_poly_adj_fft3(t1, logn - 1, 0);
	s += ffLDL_inner_fft3(tree + s, t0, t1, t0, logn - 1, t2);

	return s;
}

static size_t
ffLDL_depth1_fft3(fpr *restrict tree, const fpr *restrict g00,
	const fpr *restrict g10, const fpr *restrict g11,
	const fpr *restrict g20, const fpr *restrict g21,
	const fpr *restrict g22, unsigned logn, fpr *restrict tmp)
{
	/*
	 * At depth 1, we perform a bisection on the elements of the
	 * input 3x3 matrix.
	 */

	size_t n, hn, s;
	fpr *l10, *l20, *l21, *d11, *d22;
	fpr *t0, *t1, *t2;

	n = (size_t)1 << logn;
	hn = n >> 1;
	l10 = tree;
	l20 = l10 + n;
	l21 = l20 + n;
	d11 = tmp;
	d22 = d11 + n;
	t0 = d22 + n;
	t1 = t0 + hn;
	t2 = t1 + hn;
	s = 3 * n;

	/*
	 * LDL decomposition.
	 */
	LDL_dim3_fft3(d11, d22, l10, l20, l21,
		g00, g10, g11, g20, g21, g22, logn, 0, t2);

	/*
	 * Splits and recursive invocations.
	 *
	 * TODO: for N = 6, this would need special code. Right now,
	 * we simply refuse to handle it, because N = 6 is way too weak
	 * to have any value anyway.
	 */
	falcon_poly_split_deep_fft3(t0, t1, g00, logn);
	falcon_poly_adj_fft3(t1, logn - 1, 0);
	s += ffLDL_inner_fft3(tree + s, t0, t1, t0, logn - 1, t2);

	falcon_poly_split_deep_fft3(t0, t1, d11, logn);
	falcon_poly_adj_fft3(t1, logn - 1, 0);
	s += ffLDL_inner_fft3(tree + s, t0, t1, t0, logn - 1, t2);

	falcon_poly_split_deep_fft3(t0, t1, d22, logn);
	falcon_poly_adj_fft3(t1, logn - 1, 0);
	s += ffLDL_inner_fft3(tree + s, t0, t1, t0, logn - 1, t2);

	return s;
}

static size_t
ffLDL_fft3(fpr *restrict tree, const fpr *restrict g00,
	const fpr *restrict g10, const fpr *restrict g11,
	unsigned logn, fpr *restrict tmp)
{
	size_t n, tn, s;
	fpr *l10, *d11, *t0, *t1, *t2, *t3;

	n = (size_t)3 << (logn - 1);
	tn = (size_t)1 << (logn - 1);
	l10 = tree;
	s = n;
	t0 = tmp;
	t1 = t0 + tn;
	t2 = t1 + tn;
	t3 = t2 + tn;

	/*
	 * Formally, we perform the LDL decomposition, _then_ do
	 * the recursion on split versions of the diagonal elements.
	 * However, d00 = g00, so we can perform the first recursion
	 * _before_ computing the LDL; this saves RAM.
	 */

	/*
	 * Trisection of d00 for first recursion.
	 */
	falcon_poly_split_top_fft3(t0, t1, t2, g00, logn);
	falcon_poly_adj_fft3(t1, logn - 1, 0);
	falcon_poly_adj_fft3(t2, logn - 1, 0);
	s += ffLDL_depth1_fft3(tree + s, t0, t1, t0, t2, t1, t0, logn - 1, t3);

	/*
	 * Compute LDL decomposition of the top matrix.
	 */
	d11 = t3;
	LDL_dim2_fft3(d11, l10, g00, g10, g11, logn, 1);

	/*
	 * Trisection of d11 for second recursion.
	 */
	falcon_poly_split_top_fft3(t0, t1, t2, d11, logn);
	falcon_poly_adj_fft3(t1, logn - 1, 0);
	falcon_poly_adj_fft3(t2, logn - 1, 0);
	s += ffLDL_depth1_fft3(tree + s, t0, t1, t0, t2, t1, t0, logn - 1, t3);

	return s;
}

/*
 * Get the size of the LDL tree for an input with polynomials of size
 * 2^logn. The size is expressed in the number of elements.
 */
static inline unsigned
ffLDL_ternary_treesize(unsigned logn)
{
	return 3 * ((logn + 2) << (logn - 1));
}

static size_t
ffLDL_ternary_normalize_inner(fpr *tree, fpr sigma, unsigned logn)
{
	size_t s;

	if (logn == 1) {
		/*
		 * At logn = 1, tree consists in three polynomials,
		 * one parent node and two leaves. We normalize the
		 * leaves.
		 */
		tree[2] = fpr_div(sigma, fpr_sqrt(tree[2]));
		tree[3] = fpr_div(sigma, fpr_sqrt(tree[3]));
		return 4;
	}

	s = (size_t)1 << logn;
	s += ffLDL_ternary_normalize_inner(tree + s, sigma, logn - 1);
	s += ffLDL_ternary_normalize_inner(tree + s, sigma, logn - 1);
	return s;
}

static size_t
ffLDL_ternary_normalize_depth1(fpr *tree, fpr sigma, unsigned logn)
{
	size_t s;

	s = (size_t)3 << logn;
	s += ffLDL_ternary_normalize_inner(tree + s, sigma, logn - 1);
	s += ffLDL_ternary_normalize_inner(tree + s, sigma, logn - 1);
	s += ffLDL_ternary_normalize_inner(tree + s, sigma, logn - 1);
	return s;
}

static size_t
ffLDL_ternary_normalize(fpr *tree, fpr sigma, unsigned logn)
{
	size_t s;

	s = (size_t)3 << (logn - 1);
	s += ffLDL_ternary_normalize_depth1(tree + s, sigma, logn - 1);
	s += ffLDL_ternary_normalize_depth1(tree + s, sigma, logn - 1);
	return s;
}

/* =================================================================== */

/*
 * Convert an integer polynomial (with small values) into the
 * representation with complex numbers.
 */
static void
smallints_to_fpr(fpr *r, const int16_t *t, unsigned logn, unsigned ter)
{
	size_t n, u;

	n = MKN(logn, ter);
	for (u = 0; u < n; u ++) {
		r[u] = fpr_of(t[u]);
	}
}

/*
 * The expanded private key contains:
 *  - The B0 matrix (four elements)
 *  - The ffLDL tree
 */

static inline size_t
skoff_b00(unsigned logn, unsigned ter)
{
	(void)logn;
	(void)ter;
	return 0;
}

static inline size_t
skoff_b01(unsigned logn, unsigned ter)
{
	return MKN(logn, ter);
}

static inline size_t
skoff_b10(unsigned logn, unsigned ter)
{
	return 2 * MKN(logn, ter);
}

static inline size_t
skoff_b11(unsigned logn, unsigned ter)
{
	return 3 * MKN(logn, ter);
}

static inline size_t
skoff_tree(unsigned logn, unsigned ter)
{
	return 4 * MKN(logn, ter);
}

/*
 * Load a private key and perform precomputations for signing.
 *
 * Number of elements in sk[]:
 *
 *  - binary case: (logn+5) * 2^logn
 *
 *  - ternary case: 3*(logn+6) * 2^(logn-1)
 *
 * tmp[] must have room for at least seven polynomials.
 */
static void
load_skey(fpr *restrict sk, unsigned q,
	const int16_t *f_src, const int16_t *g_src,
	const int16_t *F_src, const int16_t *G_src,
	unsigned logn, unsigned ter, fpr *restrict tmp)
{
	size_t n;
	fpr *f, *g, *F, *G;
	fpr *b00, *b01, *b10, *b11;
	fpr *tree;
	fpr sigma;

	n = MKN(logn, ter);
	b00 = sk + skoff_b00(logn, ter);
	b01 = sk + skoff_b01(logn, ter);
	b10 = sk + skoff_b10(logn, ter);
	b11 = sk + skoff_b11(logn, ter);
	tree = sk + skoff_tree(logn, ter);

	/*
	 * We load the private key elements directly into the B0 matrix,
	 * since B0 = [[g, -f], [G, -F]].
	 */
	f = b01;
	g = b00;
	F = b11;
	G = b10;

	smallints_to_fpr(f, f_src, logn, ter);
	smallints_to_fpr(g, g_src, logn, ter);
	smallints_to_fpr(F, F_src, logn, ter);
	smallints_to_fpr(G, G_src, logn, ter);

	/*
	 * Compute the FFT for the key elements, and negate f and F.
	 */
	if (ter) {
		falcon_FFT3(f, logn, 1);
		falcon_FFT3(g, logn, 1);
		falcon_FFT3(F, logn, 1);
		falcon_FFT3(G, logn, 1);
		falcon_poly_neg_fft3(f, logn, 1);
		falcon_poly_neg_fft3(F, logn, 1);
	} else {
		falcon_FFT(f, logn);
		falcon_FFT(g, logn);
		falcon_FFT(F, logn);
		falcon_FFT(G, logn);
		falcon_poly_neg_fft(f, logn);
		falcon_poly_neg_fft(F, logn);
	}

	/*
	 * The Gram matrix is G = B·B*. Formulas are:
	 *   g00 = b00*adj(b00) + b01*adj(b01)
	 *   g01 = b00*adj(b10) + b01*adj(b11)
	 *   g10 = b10*adj(b00) + b11*adj(b01)
	 *   g11 = b10*adj(b10) + b11*adj(b11)
	 */
	if (ter) {
		fpr *g00, *g10, *g11, *gxx;

		g00 = tmp;
		g10 = g00 + n;
		g11 = g10 + n;
		gxx = g11 + n;

		memcpy(g00, b00, n * sizeof *b00);
		falcon_poly_mulselfadj_fft3(g00, logn, 1);
		memcpy(gxx, b01, n * sizeof *b01);
		falcon_poly_mulselfadj_fft3(gxx, logn, 1);
		falcon_poly_add_fft3(g00, gxx, logn, 1);

		memcpy(g10, b10, n * sizeof *b10);
		falcon_poly_muladj_fft3(g10, b00, logn, 1);
		memcpy(gxx, b11, n * sizeof *b11);
		falcon_poly_muladj_fft3(gxx, b01, logn, 1);
		falcon_poly_add_fft3(g10, gxx, logn, 1);

		memcpy(g11, b10, n * sizeof *b10);
		falcon_poly_mulselfadj_fft3(g11, logn, 1);
		memcpy(gxx, b11, n * sizeof *b11);
		falcon_poly_mulselfadj_fft3(gxx, logn, 1);
		falcon_poly_add_fft3(g11, gxx, logn, 1);

		/*
		 * Compute the Falcon tree.
		 */
		ffLDL_fft3(tree, g00, g10, g11, logn, gxx);

		/*
		 * Tree normalization:
		 *   sigma = 1.32 * sqrt(q/sqrt(2))
		 */
		sigma = fpr_mul(
			fpr_sqrt(fpr_mul(fpr_of(q), fpr_sqrt(fpr_of(2)))),
			fpr_div(fpr_of(132), fpr_of(100)));

		/*
		 * Normalize tree with sigma.
		 */
		ffLDL_ternary_normalize(tree, sigma, logn);
	} else {
		/*
		 * For historical reasons, this implementation uses
		 * g00, g01 and g11 (upper triangle).
		 */
		fpr *g00, *g01, *g11, *gxx;

		g00 = tmp;
		g01 = g00 + n;
		g11 = g01 + n;
		gxx = g11 + n;

		memcpy(g00, b00, n * sizeof *b00);
		falcon_poly_mulselfadj_fft(g00, logn);
		memcpy(gxx, b01, n * sizeof *b01);
		falcon_poly_mulselfadj_fft(gxx, logn);
		falcon_poly_add_fft(g00, gxx, logn);

		memcpy(g01, b00, n * sizeof *b00);
		falcon_poly_muladj_fft(g01, b10, logn);
		memcpy(gxx, b01, n * sizeof *b01);
		falcon_poly_muladj_fft(gxx, b11, logn);
		falcon_poly_add_fft(g01, gxx, logn);

		memcpy(g11, b10, n * sizeof *b10);
		falcon_poly_mulselfadj_fft(g11, logn);
		memcpy(gxx, b11, n * sizeof *b11);
		falcon_poly_mulselfadj_fft(gxx, logn);
		falcon_poly_add_fft(g11, gxx, logn);

		/*
		 * Compute the Falcon tree.
		 */
		ffLDL_fft(tree, g00, g01, g11, logn, gxx);

		/*
		 * Tree normalization:
		 *   sigma = 1.55 * sqrt(q)
		 */
		sigma = fpr_mul(fpr_sqrt(fpr_of(q)),
			fpr_div(fpr_of(155), fpr_of(100)));

		/*
		 * Normalize tree with sigma.
		 */
		ffLDL_binary_normalize(tree, sigma, logn);
	}
}

typedef int (*samplerZ)(void *ctx, fpr mu, fpr sigma);

/*
 * Perform Fast Fourier Sampling for target vector t and LDL tree T.
 * tmp[] must have size for at least two polynomials of size 2^logn.
 */
static void
ffSampling_fft(samplerZ samp, void *samp_ctx,
	fpr *restrict z0, fpr *restrict z1,
	const fpr *restrict tree,
	const fpr *restrict t0, const fpr *restrict t1, unsigned logn,
	fpr *restrict tmp)
{
	size_t n, hn;
	const fpr *tree0, *tree1;

	n = (size_t)1 << logn;
	if (n == 1) {
		fpr x0, x1, sigma;

		x0 = t0[0];
		x1 = t1[0];
		sigma = tree[0];
		z0[0] = fpr_of(samp(samp_ctx, x0, sigma));
		z1[0] = fpr_of(samp(samp_ctx, x1, sigma));
		return;
	}

	hn = n >> 1;
	tree0 = tree + n;
	tree1 = tree + n + ffLDL_treesize(logn - 1);

	/*
	 * We split t1 into z1 (reused as temporary storage), then do
	 * the recursive invocation, with output in tmp. We finally
	 * merge back into z1.
	 */
	falcon_poly_split_fft(z1, z1 + hn, t1, logn);
	ffSampling_fft(samp, samp_ctx, tmp, tmp + hn,
		tree1, z1, z1 + hn, logn - 1, tmp + n);
	falcon_poly_merge_fft(z1, tmp, tmp + hn, logn);

	/*
	 * Compute tb0 = t0 + (t1 - z1) * L. Value tb0 ends up in tmp[].
	 */
	memcpy(tmp, t1, n * sizeof *t1);
	falcon_poly_sub_fft(tmp, z1, logn);
	falcon_poly_mul_fft(tmp, tree, logn);
	falcon_poly_add_fft(tmp, t0, logn);

	/*
	 * Second recursive invocation.
	 */
	falcon_poly_split_fft(z0, z0 + hn, tmp, logn);
	ffSampling_fft(samp, samp_ctx, tmp, tmp + hn,
		tree0, z0, z0 + hn, logn - 1, tmp + n);
	falcon_poly_merge_fft(z0, tmp, tmp + hn, logn);
}

static void
ffSampling_inner_fft3(samplerZ samp, void *samp_ctx,
	fpr *restrict z0, fpr *restrict z1,
	const fpr *restrict tree,
	const fpr *restrict t0, const fpr *restrict t1,
	unsigned logn, fpr *restrict tmp)
{
	size_t n, hn;
	fpr *x0, *x1, *y0, *y1;
	const fpr *tree0, *tree1;

	/*
	 * For tree construction, recursion stopped at n = 2, but it
	 * produced a tree which also covers n = 1. For sampling, we use
	 * the fact that the split() and merge() function
	 * implementations actually supports logn = 1.
	 */
	if (logn == 0) {
		fpr r0, r1, rx;
		fpr sigma;

		sigma = tree[0];
		r1 = *t1;
		r0 = *t0;
		r1 = fpr_sub(r1, fpr_of(
			samp(samp_ctx, r1, fpr_mul(fpr_IW1I, sigma))));
		rx = fpr_half(r1);
		r0 = fpr_add(r0, rx);
		r0 = fpr_sub(r0, fpr_of(
			samp(samp_ctx, r0, sigma)));
		r0 = fpr_sub(r0, rx);
		*z0 = r0;
		*z1 = r1;
		return;
	}

	n = (size_t)1 << logn;
	hn = n >> 1;
	y0 = tmp;
	y1 = y0 + hn;

	tree0 = tree + n;
	tree1 = tree + n + (logn << (logn - 1));

	/*
	 * Split t1, recurse, merge into z1.
	 */
	x0 = z1;
	x1 = x0 + hn;
	falcon_poly_split_deep_fft3(x0, x1, t1, logn);
	ffSampling_inner_fft3(samp, samp_ctx,
		y0, y1, tree1, x0, x1, logn - 1, tmp + n);
	falcon_poly_merge_deep_fft3(z1, y0, y1, logn);

	/*
	 * Compute t0b = t0 + z1 * l10 (into tmp[]).
	 * FIXME: save z1 * l10 instead of recomputing it later on.
	 */
	memcpy(tmp, z1, n * sizeof *t1);
	falcon_poly_mul_fft3(tmp, tree, logn, 0);
	falcon_poly_add_fft3(tmp, t0, logn, 0);

	/*
	 * Split t0b, recurse, merge into z0.
	 */
	x0 = z0;
	x1 = x0 + hn;
	falcon_poly_split_deep_fft3(x0, x1, tmp, logn);
	ffSampling_inner_fft3(samp, samp_ctx,
		y0, y1, tree0, x0, x1, logn - 1, tmp + n);
	falcon_poly_merge_deep_fft3(z0, y0, y1, logn);

	/*
	 * Subtract z1 * l10 from z0.
	 */
	memcpy(tmp, z1, n * sizeof *z1);
	falcon_poly_mul_fft3(tmp, tree, logn, 0);
	falcon_poly_sub_fft3(z0, tmp, logn, 0);
}

static void
ffSampling_depth1_fft3(samplerZ samp, void *samp_ctx,
	fpr *restrict z0, fpr *restrict z1, fpr *restrict z2,
	const fpr *restrict tree,
	const fpr *restrict t0, const fpr *restrict t1, const fpr *restrict t2,
	unsigned logn, fpr *restrict tmp)
{
	size_t n, hn;
	fpr *x0, *x1, *y0, *y1;
	const fpr *tree0, *tree1, *tree2;

	n = (size_t)1 << logn;
	hn = n >> 1;

	tree0 = tree + 3 * n;
	tree1 = tree0 + (logn << (logn - 1));
	tree2 = tree1 + (logn << (logn - 1));
	y0 = tmp;
	y1 = y0 + hn;

	/*
	 * Split t2, recurse, merge into z2.
	 */
	x0 = z2;
	x1 = x0 + hn;
	falcon_poly_split_deep_fft3(x0, x1, t2, logn);
	ffSampling_inner_fft3(samp, samp_ctx,
		y0, y1, tree2, x0, x1, logn - 1, tmp + n);
	falcon_poly_merge_deep_fft3(z2, y0, y1, logn);

	/*
	 * Compute t1b = t1 + z2 * l21 (into tmp[]).
	 * FIXME: save z2 * l21 instead of recomputing it later on.
	 */
	memcpy(tmp, z2, n * sizeof *z2);
	falcon_poly_mul_fft3(tmp, tree + 2 * n, logn, 0);
	falcon_poly_add_fft3(tmp, t1, logn, 0);

	/*
	 * Split t1b, recurse, merge into z1, and subtract z2 * l21.
	 */
	x0 = z1;
	x1 = x0 + hn;
	falcon_poly_split_deep_fft3(x0, x1, tmp, logn);
	ffSampling_inner_fft3(samp, samp_ctx,
		y0, y1, tree1, x0, x1, logn - 1, tmp + n);
	falcon_poly_merge_deep_fft3(z1, y0, y1, logn);
	memcpy(tmp, z2, n * sizeof *z2);
	falcon_poly_mul_fft3(tmp, tree + 2 * n, logn, 0);
	falcon_poly_sub_fft3(z1, tmp, logn, 0);

	/*
	 * Compute t0b = t0 + z1 * l10 + z2 * l20 (into z0).
	 * We use z0 as extra temporary.
	 * FIXME: save z1 * l10 + z2 * l20 instead of recomputing it later on.
	 */
	memcpy(z0, t0, n * sizeof *t0);
	memcpy(tmp, z1, n * sizeof *z1);
	falcon_poly_mul_fft3(tmp, tree, logn, 0);
	falcon_poly_add_fft3(z0, tmp, logn, 0);
	memcpy(tmp, z2, n * sizeof *z1);
	falcon_poly_mul_fft3(tmp, tree + n, logn, 0);
	falcon_poly_add_fft3(z0, tmp, logn, 0);

	/*
	 * Split t0b, recurse, merge into z0.
	 */
	x0 = z0;
	x1 = x0 + hn;
	memcpy(tmp, z0, n * sizeof *z0);
	falcon_poly_split_deep_fft3(x0, x1, tmp, logn);
	ffSampling_inner_fft3(samp, samp_ctx,
		y0, y1, tree0, x0, x1, logn - 1, tmp + n);
	falcon_poly_merge_deep_fft3(z0, y0, y1, logn);

	/*
	 * Subtract z1 * l10 and z2 * l20 from z0.
	 */
	memcpy(tmp, z1, n * sizeof *z1);
	falcon_poly_mul_fft3(tmp, tree, logn, 0);
	falcon_poly_sub_fft3(z0, tmp, logn, 0);
	memcpy(tmp, z2, n * sizeof *z1);
	falcon_poly_mul_fft3(tmp, tree + n, logn, 0);
	falcon_poly_sub_fft3(z0, tmp, logn, 0);
}

static void
ffSampling_fft3(samplerZ samp, void *samp_ctx,
	fpr *restrict z0, fpr *restrict z1,
	const fpr *restrict tree,
	const fpr *restrict t0, const fpr *restrict t1, unsigned logn,
	fpr *restrict tmp)
{
	size_t n, tn;
	fpr *x0, *x1, *x2, *y0, *y1, *y2;
	const fpr *tree0, *tree1;

	n = (size_t)3 << (logn - 1);
	tn = (size_t)1 << (logn - 1);

	tree0 = tree + n;
	tree1 = tree0 + 3 * ((logn + 1) << (logn - 2));
	y0 = tmp;
	y1 = y0 + tn;
	y2 = y1 + tn;

	/*
	 * We split t1 three ways for recursive invocation. We use
	 * z0 and z1 as temporary storage areas; final merge yields z1.
	 */
	x0 = z1;
	x1 = x0 + tn;
	x2 = x1 + tn;
	falcon_poly_split_top_fft3(x0, x1, x2, t1, logn);
	ffSampling_depth1_fft3(samp, samp_ctx,
		y0, y1, y2, tree1, x0, x1, x2, logn - 1, tmp + n);
	falcon_poly_merge_top_fft3(z1, y0, y1, y2, logn);

	/*
	 * Compute t0b = t0 + z1 * L (in tmp[]).
	 * FIXME: save z1 * L instead of recomputing it later on.
	 */
	memcpy(tmp, z1, n * sizeof *z1);
	falcon_poly_mul_fft3(tmp, tree, logn, 1);
	falcon_poly_add_fft3(tmp, t0, logn, 1);

	/*
	 * Split t0b, recurse, and merge into z0.
	 */
	x0 = z0;
	x1 = x0 + tn;
	x2 = x1 + tn;
	falcon_poly_split_top_fft3(x0, x1, x2, tmp, logn);
	ffSampling_depth1_fft3(samp, samp_ctx,
		y0, y1, y2, tree0, x0, x1, x2, logn - 1, tmp + n);
	falcon_poly_merge_top_fft3(z0, y0, y1, y2, logn);

	/*
	 * Subtract z1 * L from z0.
	 */
	memcpy(tmp, z1, n * sizeof *z1);
	falcon_poly_mul_fft3(tmp, tree, logn, 1);
	falcon_poly_sub_fft3(z0, tmp, logn, 1);
}

/*
 * Compute a signature: the signature contains two vectors, s1 and s2;
 * the caller must still check that they comply with the signature bound,
 * and try again if that is not the case.
 *
 * tmp[] must have room for at least six polynomials.
 */
static void
do_sign(samplerZ samp, void *samp_ctx,
	int16_t *restrict s1, int16_t *restrict s2,
	unsigned q, const fpr *restrict sk, const uint16_t *restrict hm,
	unsigned logn, unsigned ter, fpr *restrict tmp)
{
	size_t n, u;
	fpr *t0, *t1, *tx, *ty, *tz;
	const fpr *b00, *b01, *b10, *b11, *tree;
	fpr ni;

	n = MKN(logn, ter);
	t0 = tmp;
	t1 = t0 + n;
	tx = t1 + n;
	ty = tx + n;
	tz = ty + n;
	b00 = sk + skoff_b00(logn, ter);
	b01 = sk + skoff_b01(logn, ter);
	b10 = sk + skoff_b10(logn, ter);
	b11 = sk + skoff_b11(logn, ter);
	tree = sk + skoff_tree(logn, ter);

	/*
	 * Set the target vector to [hm, 0] (hm is the hashed message).
	 */
	for (u = 0; u < n; u ++) {
		t0[u] = fpr_of(hm[u]);
		/* This is implicit.
		t1[u] = fpr_of(0);
		*/
	}

	if (ter) {
		/*
		 * Apply the lattice basis to obtain the real target
		 * vector (after normalization with regards to modulus).
		 */
		falcon_FFT3(t0, logn, 1);
		ni = fpr_inverse_of(q);
		memcpy(t1, t0, n * sizeof *t0);
		falcon_poly_mul_fft3(t1, b01, logn, 1);
		falcon_poly_mulconst_fft3(t1, fpr_neg(ni), logn, 1);
		falcon_poly_mul_fft3(t0, b11, logn, 1);
		falcon_poly_mulconst_fft3(t0, ni, logn, 1);

		/*
		 * Apply sampling. Output is written back in [tx, ty].
		 */
		ffSampling_fft3(samp, samp_ctx, tx, ty, tree, t0, t1, logn, tz);

		/*
		 * Get the lattice point corresponding to that tiny vector.
		 */
		memcpy(t0, tx, n * sizeof *tx);
		memcpy(t1, ty, n * sizeof *ty);
		falcon_poly_mul_fft3(tx, b00, logn, 1);
		falcon_poly_mul_fft3(ty, b10, logn, 1);
		falcon_poly_add_fft3(tx, ty, logn, 1);
		memcpy(ty, t0, n * sizeof *t0);
		falcon_poly_mul_fft3(ty, b01, logn, 1);

		memcpy(t0, tx, n * sizeof *tx);
		falcon_poly_mul_fft3(t1, b11, logn, 1);
		falcon_poly_add_fft3(t1, ty, logn, 1);

		falcon_iFFT3(t0, logn, 1);
		falcon_iFFT3(t1, logn, 1);

		/*
		 * Compute the signature.
		 */
		for (u = 0; u < n; u ++) {
			s1[u] = (int16_t)fpr_rint(t0[u]);
			s2[u] = (int16_t)fpr_rint(t1[u]);
		}
	} else {
		/*
		 * Apply the lattice basis to obtain the real target
		 * vector (after normalization with regards to modulus).
		 */
		falcon_FFT(t0, logn);
		ni = fpr_inverse_of(q);
		memcpy(t1, t0, n * sizeof *t0);
		falcon_poly_mul_fft(t1, b01, logn);
		falcon_poly_mulconst_fft(t1, fpr_neg(ni), logn);
		falcon_poly_mul_fft(t0, b11, logn);
		falcon_poly_mulconst_fft(t0, ni, logn);

		/*
		 * Apply sampling. Output is written back in [tx, ty].
		 */
		ffSampling_fft(samp, samp_ctx, tx, ty, tree, t0, t1, logn, tz);

		/*
		 * Get the lattice point corresponding to that tiny vector.
		 */
		memcpy(t0, tx, n * sizeof *tx);
		memcpy(t1, ty, n * sizeof *ty);
		falcon_poly_mul_fft(tx, b00, logn);
		falcon_poly_mul_fft(ty, b10, logn);
		falcon_poly_add(tx, ty, logn);
		memcpy(ty, t0, n * sizeof *t0);
		falcon_poly_mul_fft(ty, b01, logn);

		memcpy(t0, tx, n * sizeof *tx);
		falcon_poly_mul_fft(t1, b11, logn);
		falcon_poly_add_fft(t1, ty, logn);

		falcon_iFFT(t0, logn);
		falcon_iFFT(t1, logn);

		/*
		 * Compute the signature.
		 */
		for (u = 0; u < n; u ++) {
			s1[u] = (int16_t)(hm[u] - fpr_rint(t0[u]));
			s2[u] = (int16_t)-fpr_rint(t1[u]);
		}
	}
}

/*
 * We have here three versions of gaussian0_sampler().
 *
 * The CoDF version maintains the to-be-returned value z as an integer,
 * and makes repeated samplings of 64-bit values; each of them is checked
 * against a tabulated value to decide whether z should be returned as
 * is, or z should be incremented and the process repeated.
 *
 * The CDF version obtains a random 128-bit value and finds it in the
 * tabulated distribution to directly return the sampled value.
 *
 * Since we follow a discrete Gaussian of standard deviation sigma0 = 2,
 * the average number of PRNG invocations in the CoDF variant should be
 * 2. The CDF variant always makes exactly 2 invocations of the PRNG.
 * Thus, the two variants should offer about the same performance.
 * Neither is constant-time, as implemented here, but the CDF variant
 * _could_ be made constant-time relatively easily by reading up the
 * whole table systematically.
 *
 * The third version (which is the default) is a CDF variant with a
 * fast test on the top bytes; this reduces the average number of bytes
 * obtained from the RNG.
 */

#if SAMPLER_CODF

/*
 * Precomputed CoDF table, scaled to 2^64.
 *
 * We use distribution D(z) = exp(-(z^2)/(2*sigma0^2)), for integer z >= 0
 * and sigma0 = 2. CoDF is defined as:
 *   CoDF[z] = D(z) / (\sum_{i>=z} D(i))
 * i.e. CoDF[z] is the probability of getting exactly z, conditioned by
 * the probability of getting _at least_ z.
 *
 * Values below have been computed with 200 bits of precision, then
 * scaled up to 2^64, and rounded to the nearest integer. An extra
 * terminator value has been added with value 2^64-1, to ensure that the
 * sampling will always terminate (but it is unreachable with
 * overwhelming probability anyway).
 */
static const uint64_t CoDF[] = {
	 6135359076264090556u,  8112710616924306458u,  9953032297470027236u,
	11570271901782068036u, 12938678630255467417u, 14067899748132476476u,
	14984234773976257854u, 15719358179957714060u, 16304436317268820589u,
	16767483091484223253u, 17132471096555539072u, 17419317572418304948u,
	17644260465667274210u, 17820371633146694049u, 17958082469163820764u,
	18065665744948529650u, 18149652895292277433u, 18215183554218485224u,
	18266292212831004768u, 18306140009815638184u, 18337200327237292561u,
	18361406363042628418u, 18380267875334172737u, 18394963192522594010u,
	18406411526561969749u, 18415329685754092522u, 18422276481254688480u,
	18427687455017083046u, 18431902013094434230u, 18435184609813683248u,
	18446744073709551615u
};

/*
 * This function samples a positive integer z along the distribution
 * D(z) = exp(-(z^2)/(2*sigma0^2)) (with sigma0 = 2).
 */
static int
gaussian0_sampler(prng *p)
{
	int z;

	for (z = 0;; z ++) {
		if (falcon_prng_get_u64(p) <= CoDF[z]) {
			return z;
		}
	}
}

/*
 * The "large" Gaussian sampler, using sigma0 = sqrt(5) instead of
 * sigma0 = 2. See above for comments.
 */

static const uint64_t CoDF_large[] = {
	 5585698290684357146u,  7249223068825568181u,  8847100402462867882u,
	10311416391296380421u, 11610658709007993603u, 12738082895937970315u,
	13701420522128960375u, 14515655832074621060u, 15198539954879572289u,
	15768038078814656243u, 16241001905848789539u, 16632570846605441455u,
	16955985738791465744u, 17222623757559529836u, 17442142477961602124u,
	17622669281608141903u, 17771000949527216889u, 17892795030360415835u,
	17992744215115961471u, 18074730391283377394u, 18141958023865435089u,
	18197068047683742685u, 18242234163582632318u, 18279243671444027479u,
	18309564958002955235u, 18334403612118750997u, 18354748936728452469u,
	18371412406174455800u, 18385059402287341093u, 18396235363810085460u,
	18446744073709551615u
};

static int
gaussian0_sampler_large(prng *p)
{
	int z;

	for (z = 0;; z ++) {
		if (falcon_prng_get_u64(p) <= CoDF_large[z]) {
			return z;
		}
	}
}

#elif SAMPLER_CDF

/*
 * Precomputed CDF table, scaled to 2^128.
 *
 * We use distribution D(z) = exp(-(z^2)/(2*sigma0^2)), for integer z >= 0
 * and sigma0 = 2. CDF is defined as:
 *   CDF[z - 1] = (\sum_{i>=z} D(i)) / (\sum_{i>=0} D(i))
 * i.e. CDF[z - 1] is the probability of getting at least z. Since
 * probability of getting at least 0 is 1, that value is omitted (which
 * is why we use 'CDF[z - 1]' and not 'CDF[z]').
 *
 * Values below have been computed with 250 bits of precision, then
 * scaled up to 2^128, and rounded to the nearest integer.
 */
typedef struct {
	uint64_t hi, lo;
} z128;

static const z128 CDF[] = {
	{ 12311384997445461060u,  1164166488924346079u },
	{  6896949616398116699u, 12764548512970285063u },
	{  3175666228297764653u,  3821523381717377694u },
	{  1183806766059182243u,  9659191409195894253u },
	{   353476207714659138u, 11568890547115288740u },
	{    83907343225073545u,  8467089641975205687u },
	{    15749660485982248u,  2564667606477951368u },
	{     2328616999791799u,  9676146345336721991u },
	{      270433320942720u,  2935538500327714828u },
	{       24618334939657u, 15160816743013190466u },
	{        1753979576256u, 10774725962424838880u },
	{          97691228987u,   260573317676966082u },
	{           4249834528u, 17953183039089978228u },
	{            144306182u, 18293121091792460987u },
	{              3822728u,  5847176767326442802u },
	{                78971u,  1092095764737208072u },
	{                 1271u, 15793321684841757645u },
	{                   15u, 17810511485592413461u },
	{                    0u,  2881005946479451579u },
	{                    0u,    21959492827510209u },
	{                    0u,      130403777780196u },
	{                    0u,         603269596717u },
	{                    0u,           2173991748u },
	{                    0u,              6102497u },
	{                    0u,                13343u },
	{                    0u,                   23u },
	{                    0u,                    0u }
};

/*
 * This function samples a positive integer z along the distribution
 * D(z) = exp(-(z^2)/(2*sigma0^2)) (with sigma0 = 2).
 */
static int
gaussian0_sampler(prng *p)
{
	uint64_t hi, lo;
	int z;

	hi = falcon_prng_get_u64(p);
	lo = falcon_prng_get_u64(p);

	/*
	 * Loop below MUST exit, since the last CDF[] table entry is 0.
	 */
	for (z = 0;; z ++) {
		if (hi > CDF[z].hi || (hi == CDF[z].hi && lo >= CDF[z].lo)) {
			return z;
		}
	}
}

/*
 * The "large" Gaussian sampler, using sigma0 = sqrt(5) instead of
 * sigma0 = 2. See above for comments.
 */

static const z128 CDF_large[] = {
	{ 12861045783025194470u,  4172822031082392599u },
	{  7806896963754487971u,  1791502523474411737u },
	{  4062691428401757936u,  7604423452952674944u },
	{  1791715974944572781u, 12746882011603970427u },
	{   663982939486702512u, 10691794681225833799u },
	{   205480902982363203u, 16583804506623933557u },
	{    52858833213387347u,  3579657461630629701u },
	{    11264466882686187u,  5640638197337281079u },
	{     1983509262044379u, 16761989531132089487u },
	{      288031217321451u,  6230729019263372079u },
	{       34440907249949u, 15311621317364317869u },
	{        3387143639027u, 10510168041118337409u },
	{         273729206155u,  4159623830376585760u },
	{          18164586717u,  5026128773602959958u },
	{            989235429u, 14864651885815683859u },
	{             44192296u,  6814956854290206119u },
	{              1618856u, 18089287580937416157u },
	{                48613u, 12706079520459332296u },
	{                 1196u,  8301927825924258072u },
	{                   24u,  2373930578664405075u },
	{                    0u,  7354088428325342096u },
	{                    0u,    99537325746473565u },
	{                    0u,     1103521004104852u },
	{                    0u,       10020207975859u },
	{                    0u,          74515224141u },
	{                    0u,            453796867u },
	{                    0u,              2263115u },
	{                    0u,                 9242u },
	{                    0u,                   31u },
	{                    0u,                    0u }
};

static int
gaussian0_sampler_large(prng *p)
{
	uint64_t hi, lo;
	int z;

	hi = falcon_prng_get_u64(p);
	lo = falcon_prng_get_u64(p);
	for (z = 0;; z ++) {
		if (hi > CDF_large[z].hi
			|| (hi == CDF_large[z].hi && lo >= CDF_large[z].lo))
		{
			return z;
		}
	}
}

#else

/*
 * Partial precomputed CDF tables:
 *  - CDF8 holds the 8 MSBs of each CDF image not starting with 0x00;
 *  - CDFs holds the next 128 MSBs of the same CDF images as CDF8;
 *  - CDF0 holds the next 128 MSBs of each CDF image starting with 0x00.
 *
 * We use the same distribution and indexation as above (i.e. CDF8[z] and 
 * CDFs[z] correspond to CDF[z]) exept that D(z) is scaled to 2^136 and CDF0 
 * is offset by 6 slots (i.e. CDF0[z] corresponds to CDF[z + 6]).
 *
 * Values below have been computed with 256 bits of precision, then
 * scaled up to 2^136, and rounded to the nearest integer.
 */
typedef struct {
	uint64_t hi, lo;
} z128;

static const uint8_t CDF8[] = {
	170u, 95u, 44u, 16u, 4u, 1u
};

static const z128 CDFs[] = {
	{ 15768066815414256656u,  2878715985279770247u },
	{ 13178414795510471601u,  2650718273802340096u },
	{  1313815201007480117u,   632549813042453946u },
	{  7906626931797828486u,   889294877069012273u },
	{ 16702932880114533024u, 10156928267985658938u },
	{  3033535791909276021u,  9305891721635116763u }
};

static const z128 CDF0[] = {
	{  4031913084411455523u, 10918864678521243583u },
	{   596125951946700678u,  5229758529120913067u },
	{    69230930161336360u, 13628093135512931395u },
	{     6302293744552402u,  7352830732370919967u },
	{      449018771521685u,  9764979398035562428u },
	{       25008954620675u, 11366537104174662165u },
	{        1087957639417u,  2775583653356073882u },
	{          36942382845u, 16012748850353453704u },
	{            978618449u,  2690982465095676317u },
	{             20216591u,  2875354667081992134u },
	{               325595u,  3253399177098153241u },
	{                 4087u,  3145154105398596933u },
	{                   39u, 18114503424067091158u },
	{                    0u,  5621630163842613476u },
	{                    0u,    33383367111730198u },
	{                    0u,      154437016759436u },
	{                    0u,         556541887369u },
	{                    0u,           1562239343u },
	{                    0u,              3415730u },
	{                    0u,                 5817u },
	{                    0u,                    8u },
	{                    0u,                    0u }
};

/*
 * This function samples a positive integer z along the distribution
 * D(z) = exp(-(z^2)/(2*sigma0^2)) (with sigma0 = 2).
 */
static int
gaussian0_sampler(prng *p)
{
	uint8_t msb;
	uint64_t hi, lo;
	int z;

	msb = falcon_prng_get_u8(p);
	if (msb != 0x00) {
		/*
		 * The loop below return the sample when the byte drawn is 
		 * equal to the MSBs of at most one CDF image (i.e. msb != 0x00
		 * by construction of CDF8 and CDFs).
		 */
		for (z = 0; z < (int)sizeof CDF8; z ++) {
			/*
			 * We conclude directly when the byte drawn differs
			 * from all CDF images, since 8 bits are enough to 
			 * compare it to any CDF image.
			 */
			if (msb > CDF8[z]) {
				return z;
			}

			/*
			 * We draw 128 bits more when the byte drawn is equal
			 * to the MSBs of a CDF image to compare these two.
			 */
			if (msb == CDF8[z]) {
				hi = falcon_prng_get_u64(p);
				lo = falcon_prng_get_u64(p);
				if (hi > CDFs[z].hi
					|| (hi == CDFs[z].hi
					&& lo >= CDFs[z].lo))
				{
					return z;
				}
				return z + 1;
			}
		}
	}

	/*
	 * Otherwise (case msb == 0x00), we draw 128 bits more and compare 
	 * it with the CDF images whose the 8 MSBs are equal to 0x00.
	 */
	hi = falcon_prng_get_u64(p);
	lo = falcon_prng_get_u64(p);
	for (z = 0;; z ++) {
		if (hi > CDF0[z].hi || (hi == CDF0[z].hi && lo >= CDF0[z].lo)) {
			return z + (int)sizeof CDF8;
		}
	}
}

/*
 * The "large" Gaussian sampler, using sigma0 = sqrt(5) instead of
 * sigma0 = 2. See above for comments.
 */

static const uint8_t CDF8_large[] = {
	178u, 108u, 56u, 24u, 9u, 2u
};

static const z128 CDFs_large[] = {
	{  8907275334149596729u, 16778027755648063129u },
	{  6317262760517346072u, 15902788240420165876u },
	{  7031337543115141225u,  9824276216381865951u },
	{ 15957431816781393328u, 16574837997735344921u },
	{  3958935845209878676u,  6981315484799813327u },
	{ 15709623016065876966u,  2702816742530118898u }
};

static const z128 CDF0_large[] = {
	{ 13531861302627160881u, 12501850565673174323u },
	{  2883703521967663950u,  5157340768998930255u },
	{   507778371083361256u, 11424694869198933797u },
	{    73735991634291542u,  8646638592401813304u },
	{     8816872255987156u,  9065313618840431804u },
	{      867108771591057u, 15825127838409392498u },
	{       70074676775737u, 13399288374961512449u },
	{        4650134199621u, 13863624956398687773u },
	{         253244270030u,  5321603584647435131u },
	{          11313227870u, 10635011769594914448u },
	{            414427387u,   724858218881080488u },
	{             12445104u,  6129400264707983377u },
	{               306291u,  3917954960011630607u },
	{                 6176u, 17430417779382047418u },
	{                  102u,  1078742132913311815u },
	{                    1u,  7034811317387681053u },
	{                    0u,   282501377050842205u },
	{                    0u,     2565173241819955u },
	{                    0u,       19075897380102u },
	{                    0u,         116171998071u },
	{                    0u,            579357465u },
	{                    0u,              2365944u },
	{                    0u,                 7912u },
	{                    0u,                   22u },
	{                    0u,                    0u }
};

static int
gaussian0_sampler_large(prng *p)
{
	uint8_t msb;
	uint64_t hi, lo;
	int z;

	msb = falcon_prng_get_u8(p);
	if (msb != 0x00) {
		for (z = 0; z < (int)sizeof CDF8_large; z ++) {
			if (msb > CDF8_large[z]) {
				return z;
			}
			if (msb == CDF8_large[z]) {
				hi = falcon_prng_get_u64(p);
				lo = falcon_prng_get_u64(p);
				if (hi > CDFs_large[z].hi
					|| (hi == CDFs_large[z].hi
					&& lo >= CDFs_large[z].lo))
				{
					return z;
				}
				return z + 1;
			}
		}
	}

	hi = falcon_prng_get_u64(p);
	lo = falcon_prng_get_u64(p);
	for (z = 0;; z ++) {
		if (hi > CDF0_large[z].hi
			|| (hi == CDF0_large[z].hi && lo >= CDF0_large[z].lo))
		{
			return z + (int)sizeof CDF8_large;
		}
	}
}

#endif

#if CT_BEREXP

/*
 * Sample a bit with probability exp(-x) for some x >= 0.
 */
static int
BerExp(prng *p, fpr x)
{
	int s;
	fpr r;
	uint64_t w, z;
	int b;
	uint32_t sw;

	/*
	 * Reduce x modulo log(2): x = s*log(2) + r, with s an integer,
	 * and 0 <= r < log(2).
	 */
	s = fpr_floor(fpr_div(x, fpr_log2));
	r = fpr_sub(x, fpr_mul(fpr_of(s), fpr_log2));

	/*
	 * It may happen (quite rarely) that s >= 64; if sigma = 1.2
	 * (the minimum value for sigma), r = 0 and b = 1, then we get
	 * s >= 64 if the half-Gaussian produced a z >= 13, which happens
	 * with probability about 0.000000000230383991, which is
	 * approximatively equal to 2^(-32). In any case, if s >= 64,
	 * then BerExp will be non-zero with probability less than
	 * 2^(-64), so we can simply saturate s at 63.
	 */
	sw = s;
	sw ^= (sw ^ 63) & -((63 - sw) >> 31);
	s = (int)sw;

	/*
	 * Sample a bit with probability 2^(-s):
	 *  - generate a random 64-bit integer
	 *  - keep only s bits
	 *  - bit is 1 if the result is zero
	 */
	w = falcon_prng_get_u64(p);
	w ^= (w >> s) << s;
	b = 1 - (int)((w | -w) >> 63);

	/*
	 * Sample a bit with probability exp(-r). Since |r| < log(2),
	 * we can use fpr_exp_small(). The value is lower than 1; we
	 * scale it to 2^55.
	 * With combine (with AND) that bit with the previous bit, which
	 * yields the expected result.
	 */
	z = (uint64_t)fpr_rint(fpr_mul(fpr_exp_small(fpr_neg(r)), fpr_p55));
	w = falcon_prng_get_u64(p);
	w &= ((uint64_t)1 << 55) - 1;
	b &= (int)((w - z) >> 63);

	return b;
}

#else

/*
 * Sample a bit with probability exp(-x) for some x >= 0.
 */
static int
BerExp(prng *p, fpr x)
{
	int s, i;
	fpr r;
	uint32_t sw;
	uint64_t z, w;

	/*
	 * Reduce x modulo log(2): x = s*log(2) + r, with s an integer,
	 * and 0 <= r < log(2).
	 */
	s = fpr_floor(fpr_div(x, fpr_log2));
	r = fpr_sub(x, fpr_mul(fpr_of(s), fpr_log2));

	/*
	 * It may happen (quite rarely) that s >= 64; if sigma = 1.2
	 * (the minimum value for sigma), r = 0 and b = 1, then we get
	 * s >= 64 if the half-Gaussian produced a z >= 13, which happens
	 * with probability about 0.000000000230383991, which is
	 * approximatively equal to 2^(-32). In any case, if s >= 64,
	 * then BerExp will be non-zero with probability less than
	 * 2^(-64), so we can simply saturate s at 63.
	 */
	sw = s;
	sw ^= (sw ^ 63) & -((63 - sw) >> 31);
	s = (int)sw;

	/*
	 * Compute exp(-r). Since |r| < log(2), we can use fpr_exp_small().
	 * The value is lower than 1; we scale it to 2^64 and store it shifted
	 * to the right by s bits. The '-1' makes sure that we fit in 64 bits
	 * even if r = 0; the bias is negligible since 'r' itself only has
	 * 53 bits of precision.
	 */
	z = (((uint64_t)fpr_rint(fpr_mul(
		fpr_exp_small(fpr_neg(r)), fpr_p63)) << 1) - 1) >> s;

	/*
	 * Sample a bit with probability exp(-x). Since x = s*log(2) + r,
	 * exp(-x) = 2^-s * exp(-r), we compare lazily exp(-x) with the
	 * PRNG output to limit its consumption, the sign of the difference
	 * yields the expected result.
	 */
	i = 64;
	do {
		i -= 8;
		w = falcon_prng_get_u8(p) - ((z >> i) & (uint64_t)0xFF);
	} while (!w && i > 0);
	return (int)(w >> 63);
}

#endif

/*
 * The sampler produces a random integer that follows a discrete Gaussian
 * distribution, centered on mu, and with standard deviation sigma.
 * The value of sigma MUST lie between 1 and 2 (in Falcon, it should
 * always be between 1.2 and 1.9).
 */
static int
sampler(void *ctx, fpr mu, fpr sigma)
{
	prng *p;
	int s;
	fpr r, dss;

	p = ctx;

	/*
	 * The bimodal Gaussian used for rejection sampling uses
	 * sigma0 = 2; we need sigma <= sigma0. Normally, sigma
	 * is always between 1.2 and 1.9.
	 */
	/* assert(fpr_lt(sigma, fpr_of(2))); */

	/*
	 * Center is mu. We compute mu = s + r where s is an integer
	 * and 0 <= r < 1.
	 */
	s = fpr_floor(mu);
	r = fpr_sub(mu, fpr_of(s));

	/*
	 * dss = 1/(2*sigma^2).
	 */
	dss = fpr_inv(fpr_mul(fpr_sqr(sigma), fpr_of(2)));

	/*
	 * We now need to sample on center r.
	 */
	for (;;) {
		int z, b;
		fpr x;

		/*
		 * Sample z for a Gaussian distribution. Then get a
		 * random bit b to turn the sampling into a bimodal
		 * distribution: if b = 1, we use z+1, otherwise we
		 * use -z. We thus have two situations:
		 *
		 *  - b = 1: z >= 1 and sampled against a Gaussian
		 *    centered on 1.
		 *  - b = 0: z <= 0 and sampled against a Gaussian
		 *    centered on 0.
		 */
		z = gaussian0_sampler(p);
		b = falcon_prng_get_u8(p) & 1;
		z = b + ((b << 1) - 1) * z;

		/*
		 * Rejection sampling. We want a Gaussian centered on r;
		 * but we sampled against a Gaussian centered on b (0 or
		 * 1). But we know that z is always in the range where
		 * our sampling distribution is greater than the Gaussian
		 * distribution, so rejection works.
		 *
		 * We got z with distribution:
		 *    G(z) = exp(-((z-b)^2)/(2*sigma0^2))
		 * We target distribution:
		 *    S(z) = exp(-((z-r)^2)/(2*sigma^2))
		 * Rejection sampling works by keeping the value z with
		 * probability S(z)/G(z), and starting again otherwise.
		 * This requires S(z) <= G(z), which is the case here.
		 * Thus, we simply need to keep our z with probability:
		 *    P = exp(-x)
		 * where:
		 *    x = ((z-r)^2)/(2*sigma^2) - ((z-b)^2)/(2*sigma0^2)
		 *
		 * Note that z and b are integer, and we set sigma0 = 2.
		 */
		x = fpr_mul(fpr_sqr(fpr_sub(fpr_of(z), r)), dss);
		x = fpr_sub(x, fpr_div(fpr_of((z - b) * (z - b)), fpr_of(8)));
		if (BerExp(p, x)) {
			/*
			 * Rejection sampling was centered on r, but the
			 * actual center is mu = s + r.
			 */
			return s + z;
		}
	}
}

/*
 * This alternate sampler implementation accepts larger standard
 * deviations, up to sqrt(5) = 2.236...
 * It should be used for the ternary case.
 */
static int
sampler_large(void *ctx, fpr mu, fpr sigma)
{
	/*
	 * See sampler() for comments.
	 * The two only changes here are:
	 *  - gaussian0_sampler_large() instead of gaussian0_sampler()
	 *  - sigma0 = sqrt(5) instead of 2, so 2*sigma0^2 = 10.
	 */
	prng *p;
	int s;
	fpr r, dss;

	p = ctx;

	s = fpr_floor(mu);
	r = fpr_sub(mu, fpr_of(s));
	dss = fpr_inv(fpr_mul(fpr_sqr(sigma), fpr_of(2)));

	for (;;) {
		int z, b;
		fpr x;

		z = gaussian0_sampler_large(p);
		b = falcon_prng_get_u8(p) & 1;
		z = b + ((b << 1) - 1) * z;
		x = fpr_mul(fpr_sqr(fpr_sub(fpr_of(z), r)), dss);
		x = fpr_sub(x, fpr_div(fpr_of((z - b) * (z - b)), fpr_of(10)));
		if (BerExp(p, x)) {
			return s + z;
		}
	}
}

#if CLEANSE
/*
 * Cleanse a memory region by overwriting it with zeros.
 */
static void
cleanse(void *data, size_t len)
{
	volatile unsigned char *p;

	p = (volatile unsigned char *)data;
	while (len -- > 0) {
		*p ++ = 0;
	}
}
#endif

struct falcon_sign_ {
	/* Context for hashing the nonce + message. */
	shake_context sc;

	/* RNG:
	   seeded    non-zero when a 'replace' seed or system RNG was pushed
	   flipped   non-zero when flipped */
	shake_context rng;
	int seeded;
	int flipped;

	/* Private key elements. */
	unsigned q;
	unsigned logn;
	unsigned ternary;
	fpr *sk;
	size_t sk_len;
	fpr *tmp;
	size_t tmp_len;
};

static void
clear_private(falcon_sign *fs)
{
	if (fs->sk != NULL) {
#if CLEANSE
		cleanse(fs->sk, fs->sk_len);
#endif
		free(fs->sk);
		fs->sk = NULL;
		fs->sk_len = 0;
	}
	if (fs->tmp != NULL) {
#if CLEANSE
		cleanse(fs->tmp, fs->tmp_len);
#endif
		free(fs->tmp);
		fs->tmp = NULL;
		fs->tmp_len = 0;
	}
	fs->q = 0;
	fs->logn = 0;
	fs->ternary = 0;
}

/* see falcon.h */
falcon_sign *
falcon_sign_new(void)
{
	falcon_sign *fs;

	fs = malloc(sizeof *fs);
	if (fs == NULL) {
		return NULL;
	}
	fs->seeded = 0;
	fs->flipped = 0;
	fs->q = 0;
	fs->logn = 0;
	fs->ternary = 0;
	fs->sk = NULL;
	fs->sk_len = 0;
	fs->tmp = NULL;
	fs->tmp_len = 0;
	shake_init(&fs->rng, 512);
	return fs;
}

/* see falcon.h */
void
falcon_sign_free(falcon_sign *fs)
{
	if (fs != NULL) {
		clear_private(fs);
		free(fs);
	}
}

/* see falcon.h */
void
falcon_sign_set_seed(falcon_sign *fs,
	const void *seed, size_t len, int replace)
{
	if (replace) {
		shake_init(&fs->rng, 512);
		shake_inject(&fs->rng, seed, len);
		fs->seeded = 1;
		fs->flipped = 0;
		return;
	}
	if (fs->flipped) {
		unsigned char tmp[32];

		shake_extract(&fs->rng, tmp, sizeof tmp);
		shake_init(&fs->rng, 512);
		shake_inject(&fs->rng, tmp, sizeof tmp);
		fs->flipped = 0;
	}
	shake_inject(&fs->rng, seed, len);
}

static int
rng_ready(falcon_sign *fs)
{
	if (!fs->seeded) {
		unsigned char tmp[32];

		if (!falcon_get_seed(tmp, sizeof tmp)) {
			return 0;
		}
		falcon_sign_set_seed(fs, tmp, sizeof tmp, 0);
		fs->seeded = 1;
	}
	if (!fs->flipped) {
		shake_flip(&fs->rng);
		fs->flipped = 1;
	}
	return 1;
}

/* see falcon.h */
int
falcon_sign_set_private_key(falcon_sign *fs,
	const void *skey, size_t len)
{
	const unsigned char *skey_buf;
	int comp;
	int16_t ske[4][1024];
	int i;
	int fb, has_G;

	/*
	 * TODO: when reloading a new private key of the same size as
	 * the currently allocated one, we could reuse the buffers
	 * instead of releasing and reallocating them.
	 */
	clear_private(fs);

	/*
	 * First byte defines modulus, degree and compression:
	 *   t cc g dddd
	 *   ^ ^^ ^ ^^^^
	 *   |  | |   |
	 *   |  | |   +----- degree log, over four bits (1 to 10 only)
	 *   |  | |
	 *   |  | +--------- 0 if G is present, 1 if G is absent
	 *   |  |
	 *   |  +----------- compression type
	 *   |
	 *   +-------------- 1 if ternary, 0 if binary
	 *
	 * Compression is:
	 *   00   uncompressed, 16 bits per integer (signed)
	 *   01   compressed with static codes (fixed tables)
	 * Other compression values are reserved.
	 */
	skey_buf = skey;
	fb = *skey_buf ++;
	len --;
	fs->logn = fb & 0x0F;
	has_G = !(fb & 0x10);
	fs->ternary = fb >> 7;
	if (fs->ternary) {
		fs->q = 18433;
		if (fs->logn < 3 || fs->logn > 9) {
			goto bad_skey;
		}
	} else {
		fs->q = 12289;
		if (fs->logn < 1 || fs->logn > 10) {
			goto bad_skey;
		}
	}
	comp = (fb >> 5) & 0x03;

	/*
	 * The f, g, F (and optionally G) short vectors should follow in
	 * due order.
	 */
	for (i = 0; i < 3 + has_G; i ++) {
		size_t elen;

		elen = falcon_decode_small(ske[i], fs->logn,
			comp, fs->q, skey_buf, len);
		if (elen == 0) {
			goto bad_skey;
		}
		skey_buf += elen;
		len -= elen;
	}
	if (len != 0) {
		goto bad_skey;
	}

	/*
	 * Recompute G if not provided.
	 */
	if (!has_G) {
		if (!falcon_complete_private(ske[3],
			ske[0], ske[1], ske[2], fs->logn, fs->ternary))
		{
			goto bad_skey;
		}
	}

	/*
	 * Perform pre-computations on private key.
	 */
	if (fs->ternary) {
		fs->sk_len = ((size_t)(3 * (fs->logn + 6)) << (fs->logn - 1))
			* sizeof(fpr);
		fs->tmp_len = ((size_t)21 << (fs->logn - 1)) * sizeof(fpr);
	} else {
		fs->sk_len = ((size_t)(fs->logn + 5) << fs->logn)
			* sizeof(fpr);
		fs->tmp_len = ((size_t)7 << fs->logn) * sizeof(fpr);
	}
	fs->sk = malloc(fs->sk_len);
	if (fs->sk == NULL) {
		goto bad_skey;
	}
	fs->tmp = malloc(fs->tmp_len);
	if (fs->tmp == NULL) {
		goto bad_skey;
	}

	load_skey(fs->sk, fs->q, ske[0], ske[1], ske[2], ske[3],
		fs->logn, fs->ternary, fs->tmp);
	return 1;

bad_skey:
	clear_private(fs);
	return 0;
}

/* see falcon.h */
int
falcon_sign_start(falcon_sign *fs, void *r)
{
	if (!rng_ready(fs)) {
		return 0;
	}
	shake_extract(&fs->rng, r, 40);
	falcon_sign_start_external_nonce(fs, r, 40);
	return 1;
}

/* see falcon.h */
void
falcon_sign_start_external_nonce(falcon_sign *fs, const void *r, size_t rlen)
{
	shake_init(&fs->sc, 512);
	shake_inject(&fs->sc, r, rlen);
}

/* see falcon.h */
void
falcon_sign_update(falcon_sign *fs, const void *data, size_t len)
{
	shake_inject(&fs->sc, data, len);
}

/* see falcon.h */
size_t
falcon_sign_generate(falcon_sign *fs, void *sig, size_t sig_max_len, int comp)
{
	uint16_t hm[1024];
	int16_t s1[1024], s2[1024];
	unsigned char *sig_buf;
	size_t sig_len;

	if (fs->sk == NULL) {
		return 0;
	}
	if (!rng_ready(fs)) {
		return 0;
	}
	if (sig_max_len < 2) {
		return 0;
	}
	shake_flip(&fs->sc);
	falcon_hash_to_point(&fs->sc, fs->q, hm, fs->logn);

	for (;;) {
		/*
		 * Signature produces short vectors s1 and s2. The
		 * signature is acceptable only if the aggregate vector
		 * s1,s2 is short; we must use the same bound as the
		 * verifier.
		 *
		 * If the signature is acceptable, then we return only s2
		 * (the verifier recomputes s1 from s2, the hashed message,
		 * and the public key).
		 */
		prng p;
		samplerZ samp;
		void *samp_ctx;

		/*
		 * Normal sampling. We use a fast PRNG seeded from our
		 * SHAKE context ('rng').
		 */
		falcon_prng_init(&p, &fs->rng, 0);
		samp = fs->ternary ? sampler_large : sampler;
		samp_ctx = &p;

		/*
		 * Do the actual signature.
		 */
		do_sign(samp, samp_ctx, s1, s2,
			fs->q, fs->sk, hm, fs->logn, fs->ternary, fs->tmp);

		/*
		 * Check that the norm is correct. With our chosen
		 * acceptance bound, this should almost always be true.
		 * With a tighter bound, it may happen sometimes that we
		 * end up with an invalidly large signature, in which
		 * case we just loop.
		 */
		if (falcon_is_short(s1, s2, fs->logn, fs->ternary)) {
			break;
		}
	}

	sig_buf = sig;
	sig_len = falcon_encode_small(sig_buf + 1, sig_max_len - 1,
		comp, fs->q, s2, fs->logn);
	if (sig_len == 0) {
		return 0;
	}
	sig_buf[0] = (fs->ternary << 7) | (comp << 5) | fs->logn;
	return sig_len + 1;
}
