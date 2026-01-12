/**
 * @file elligator.c
 * @author Mike Hamburg
 *
 * @copyright
 *   Copyright (c) 2015-2016 Cryptography Research, Inc.  \n
 *   Copyright (c) 2018 the libgoldilocks contributors.  \n
 *   Released under the MIT License.  See LICENSE.txt for license information.
 *
 * @brief Elligator high-level functions.
 */
#include "word.h"
#include "field.h"
#include <goldilocks.h>
#include "api.h"

/* Template stuff */
#define point_p API_NS(point_p)
static const int EDWARDS_D = -39081;

/* End of template stuff */
extern mask_t API_NS(deisogenize) (
    gf_s *__restrict__ s,
    gf_s *__restrict__ inv_el_sum,
    gf_s *__restrict__ inv_el_m1,
    const point_p p,
    mask_t toggle_hibit_s,
    mask_t toggle_altx,
    mask_t toggle_rotation
);

void API_NS(point_from_hash_nonuniform) (
    point_p p,
    const unsigned char ser[SER_BYTES]
) {
    gf r0,r,a,b,c,N,e;
    const uint8_t mask = (uint8_t)(0xFE<<(7));
    mask_t square;
    ignore_result(gf_deserialize(r0,ser,mask));
    gf_strong_reduce(r0);
    gf_sqr(a,r0);
    gf_mul_qnr(r,a);

    /* Compute D@c := (dr+a-d)(dr-ar-d) with a=1 */
    gf_sub(a,r,ONE);
    gf_mulw(b,a,EDWARDS_D); /* dr-d */
    gf_add(a,b,ONE);
    gf_sub(b,b,r);
    gf_mul(c,a,b);

    /* compute N := (r+1)(a-2d) */
    gf_add(a,r,ONE);
    gf_mulw(N,a,1-2*EDWARDS_D);

    /* e = +-sqrt(1/ND) or +-r0 * sqrt(qnr/ND) */
    gf_mul(a,c,N);
    square = gf_isr(b,a);
    gf_cond_sel(c,r0,ONE,square); /* r? = square ? 1 : r0 */
    gf_mul(e,b,c);

    /* s@a = +-|N.e| */
    gf_mul(a,N,e);
    gf_cond_neg(a,gf_lobit(a) ^ ~square);

    /* t@b = -+ cN(r-1)((a-2d)e)^2 - 1 */
    gf_mulw(c,e,1-2*EDWARDS_D); /* (a-2d)e */
    gf_sqr(b,c);
    gf_sub(e,r,ONE);
    gf_mul(c,b,e);
    gf_mul(b,c,N);
    gf_cond_neg(b,square);
    gf_sub(b,b,ONE);

    gf_sqr(c,a); /* s^2 */
    gf_add(a,a,a); /* 2s */
    gf_add(e,c,ONE);
    gf_mul(p->t,a,e); /* 2s(1+s^2) */
    gf_mul(p->x,a,b); /* 2st */
    gf_sub(a,ONE,c);
    gf_mul(p->y,e,a); /* (1+s^2)(1-s^2) */
    gf_mul(p->z,a,b); /* (1-s^2)t */

    assert(API_NS(point_valid)(p));
}

void API_NS(point_from_hash_uniform) (
    point_p pt,
    const unsigned char hashed_data[2*SER_BYTES]
) {
    point_p pt2;
    API_NS(point_from_hash_nonuniform)(pt,hashed_data);
    API_NS(point_from_hash_nonuniform)(pt2,&hashed_data[SER_BYTES]);
    API_NS(point_add)(pt,pt,pt2);
}

/* Elligator_onto:
 * Make elligator-inverse onto at the cost of roughly halving the success probability.
 * Currently no effect for curves with field size 1 bit mod 8 (where the top bit
 * is chopped off).  FUTURE MAGIC: automatic at least for brainpool-style curves; support
 * log p == 1 mod 8 brainpool curves maybe?
 */
#define MAX(A,B) (((A)>(B)) ? (A) : (B))

goldilocks_error_t
API_NS(invert_elligator_nonuniform) (
    unsigned char recovered_hash[SER_BYTES],
    const point_p p,
    uint32_t hint_
) {
    mask_t hint = hint_;
    mask_t sgn_s = -(hint & 1),
        sgn_altx = -(hint>>1 & 1),
        sgn_r0 = -(hint>>2 & 1),
        /* FUTURE MAGIC: eventually if there's a curve which needs sgn_ed_T but not sgn_r0,
         * change this mask extraction.
         */
        sgn_ed_T = -(hint>>3 & 1);
    gf a,b,c;
    mask_t is_identity;
    mask_t succ;
    API_NS(deisogenize)(a,b,c,p,sgn_s,sgn_altx,sgn_ed_T);

    is_identity = gf_eq(p->t,ZERO);
    gf_cond_sel(b,b,ONE,is_identity & sgn_altx);
    gf_cond_sel(c,c,ONE,is_identity & sgn_s &~ sgn_altx);
    gf_mulw(a,b,EDWARDS_D-1);
    gf_add(b,a,b);
    gf_sub(a,a,c);
    gf_add(b,b,c);
    gf_cond_swap(a,b,sgn_s);
    gf_mul_qnr(c,b);
    gf_mul(b,c,a);
    succ = gf_isr(c,b);
    succ |= gf_eq(b,ZERO);
    gf_mul(b,c,a);

    gf_cond_neg(b, sgn_r0^gf_lobit(b));
    /* Eliminate duplicate values for identity ... */
    succ &= ~(gf_eq(b,ZERO) & (sgn_r0 | sgn_s));
    gf_serialize(recovered_hash,b);
// TODO: ??!
#if 0
        recovered_hash[SER_BYTES-1] ^= (hint>>3)<<0;
#endif
    return goldilocks_succeed_if(mask_to_bool(succ));
}

goldilocks_error_t
API_NS(invert_elligator_uniform) (
    unsigned char partial_hash[2*SER_BYTES],
    const point_p p,
    uint32_t hint
) {
    point_p pt2;
    API_NS(point_from_hash_nonuniform)(pt2,&partial_hash[SER_BYTES]);
    API_NS(point_sub)(pt2,p,pt2);
    return API_NS(invert_elligator_nonuniform)(partial_hash,pt2,hint);
}
