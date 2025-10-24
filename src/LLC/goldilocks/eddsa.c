/**
 * @file eddsa.c
 * @author Mike Hamburg
 *
 * @copyright
 *   Copyright (c) 2015-2016 Cryptography Research, Inc.  \n
 *   Copyright (c) 2018 the libgoldilocks contributors.  \n
 *   Released under the MIT License.  See LICENSE.txt for license information.
 *
 * @cond internal
 * @brief EdDSA routines.
 */
#include "word.h"
#include <goldilocks/ed448.h>
#include <goldilocks/shake.h>
#include <string.h>
#include "api.h"

#define hash_ctx_p   goldilocks_shake256_ctx_p
#define hash_init    goldilocks_shake256_init
#define hash_update  goldilocks_shake256_update
#define hash_final   goldilocks_shake256_final
#define hash_destroy goldilocks_shake256_destroy
#define hash_hash    goldilocks_shake256_hash

#define NO_CONTEXT GOLDILOCKS_EDDSA_448_SUPPORTS_CONTEXTLESS_SIGS
#define EDDSA_PREHASH_BYTES 64

#if NO_CONTEXT
const uint8_t NO_CONTEXT_POINTS_HERE = 0;
const uint8_t * const GOLDILOCKS_ED448_NO_CONTEXT = &NO_CONTEXT_POINTS_HERE;
#endif

static void clamp (
    uint8_t secret_scalar_ser[GOLDILOCKS_EDDSA_448_PRIVATE_BYTES]
) {
    uint8_t hibit;
    /* Blarg */
    secret_scalar_ser[0] &= -COFACTOR;
    hibit = (1<<0)>>1;
    if (hibit == 0) {
        secret_scalar_ser[GOLDILOCKS_EDDSA_448_PRIVATE_BYTES - 1] = 0;
        secret_scalar_ser[GOLDILOCKS_EDDSA_448_PRIVATE_BYTES - 2] |= 0x80;
    } else {
        secret_scalar_ser[GOLDILOCKS_EDDSA_448_PRIVATE_BYTES - 1] &= hibit-1;
        secret_scalar_ser[GOLDILOCKS_EDDSA_448_PRIVATE_BYTES - 1] |= hibit;
    }
}

/* is ed448 by default with no context? */
static void hash_init_with_dom(
    hash_ctx_p hash,
    uint8_t prehashed,
    uint8_t for_prehash,
    const uint8_t *context,
    uint8_t context_len
) {
    const char *dom_s =  "SigEd448";
    const uint8_t dom[2] = {2+word_is_zero(prehashed)+word_is_zero(for_prehash), context_len};
    hash_init(hash);

#if NO_CONTEXT
    if (context_len == 0 && context == GOLDILOCKS_ED448_NO_CONTEXT) {
        (void)prehashed;
        (void)for_prehash;
        (void)context;
        (void)context_len;
        return;
    }
#endif
    hash_update(hash,(const unsigned char *)dom_s, strlen(dom_s));
    hash_update(hash,dom,2);
    hash_update(hash,context,context_len);
}

void goldilocks_ed448_prehash_init (
    hash_ctx_p hash
) {
    hash_init(hash);
}

/* In this file because it uses the hash */
void goldilocks_ed448_convert_private_key_to_x448 (
    uint8_t x[GOLDILOCKS_X448_PRIVATE_BYTES],
    const uint8_t ed[GOLDILOCKS_EDDSA_448_PRIVATE_BYTES]
) {
    /* pass the private key through hash_hash function */
    /* and keep the first GOLDILOCKS_X448_PRIVATE_BYTES bytes */
    hash_hash(
        x,
        GOLDILOCKS_X448_PRIVATE_BYTES,
        ed,
        GOLDILOCKS_EDDSA_448_PRIVATE_BYTES
    );
}

/* Specially for libotrv4 */
void goldilocks_ed448_derive_secret_scalar (
    API_NS(scalar_p) secret,
    const uint8_t privkey[GOLDILOCKS_EDDSA_448_PRIVATE_BYTES]
) {
    unsigned int c;
    /* only this much used for keygen */
    uint8_t secret_scalar_ser[GOLDILOCKS_EDDSA_448_PRIVATE_BYTES];

    hash_hash(
        secret_scalar_ser,
        sizeof(secret_scalar_ser),
        privkey,
        GOLDILOCKS_EDDSA_448_PRIVATE_BYTES
    );
    clamp(secret_scalar_ser);

    API_NS(scalar_decode_long)(secret, secret_scalar_ser, sizeof(secret_scalar_ser));

    /* Since we are going to mul_by_cofactor during encoding, divide by it here.
     * However, the EdDSA base point is not the same as the decaf base point if
     * the sigma isogeny is in use: the EdDSA base point is on Etwist_d/(1-d) and
     * the decaf base point is on Etwist_d, and when converted it effectively
     * picks up a factor of 2 from the isogenies.  So we might start at 2 instead of 1.
     */
    for (c=1; c < GOLDILOCKS_448_EDDSA_ENCODE_RATIO; c <<= 1) {
        API_NS(scalar_halve)(secret,secret);
    }

    goldilocks_bzero(secret_scalar_ser, sizeof(secret_scalar_ser));
}

void goldilocks_ed448_derive_public_key (
    uint8_t pubkey[GOLDILOCKS_EDDSA_448_PUBLIC_BYTES],
    const uint8_t privkey[GOLDILOCKS_EDDSA_448_PRIVATE_BYTES]
) {
    API_NS(scalar_p) secret_scalar;
    API_NS(point_p) p;
    goldilocks_ed448_derive_secret_scalar(secret_scalar, privkey);

    API_NS(precomputed_scalarmul)(p,API_NS(precomputed_base),secret_scalar);

    API_NS(point_mul_by_ratio_and_encode_like_eddsa)(pubkey, p);

    /* Cleanup */
    API_NS(scalar_destroy)(secret_scalar);
    API_NS(point_destroy)(p);
}

void goldilocks_ed448_sign (
    uint8_t signature[GOLDILOCKS_EDDSA_448_SIGNATURE_BYTES],
    const uint8_t privkey[GOLDILOCKS_EDDSA_448_PRIVATE_BYTES],
    const uint8_t pubkey[GOLDILOCKS_EDDSA_448_PUBLIC_BYTES],
    const uint8_t *message,
    size_t message_len,
    uint8_t prehashed,
    const uint8_t *context,
    uint8_t context_len
) {
    API_NS(scalar_p) secret_scalar;
    hash_ctx_p hash;
    API_NS(scalar_p) nonce_scalar;
    uint8_t nonce_point[GOLDILOCKS_EDDSA_448_PUBLIC_BYTES] = {0};
    API_NS(scalar_p) challenge_scalar;
    {
        /* Schedule the secret key */
        struct {
            uint8_t secret_scalar_ser[GOLDILOCKS_EDDSA_448_PRIVATE_BYTES];
            uint8_t seed[GOLDILOCKS_EDDSA_448_PRIVATE_BYTES];
        } __attribute__((packed)) expanded;
        hash_hash(
            (uint8_t *)&expanded,
            sizeof(expanded),
            privkey,
            GOLDILOCKS_EDDSA_448_PRIVATE_BYTES
        );
        clamp(expanded.secret_scalar_ser);
        API_NS(scalar_decode_long)(secret_scalar, expanded.secret_scalar_ser, sizeof(expanded.secret_scalar_ser));

        /* Hash to create the nonce */
        hash_init_with_dom(hash,prehashed,0,context,context_len);
        hash_update(hash,expanded.seed,sizeof(expanded.seed));
        hash_update(hash,message,message_len);
        goldilocks_bzero(&expanded, sizeof(expanded));
    }

    /* Decode the nonce */
    {
        uint8_t nonce[2*GOLDILOCKS_EDDSA_448_PRIVATE_BYTES];
        hash_final(hash,nonce,sizeof(nonce));
        API_NS(scalar_decode_long)(nonce_scalar, nonce, sizeof(nonce));
        goldilocks_bzero(nonce, sizeof(nonce));
    }

    {
        unsigned int c;
        API_NS(point_p) p;
        /* Scalarmul to create the nonce-point */
        API_NS(scalar_p) nonce_scalar_2;
        API_NS(scalar_halve)(nonce_scalar_2,nonce_scalar);
        for (c = 2; c < GOLDILOCKS_448_EDDSA_ENCODE_RATIO; c <<= 1) {
            API_NS(scalar_halve)(nonce_scalar_2,nonce_scalar_2);
        }

        API_NS(precomputed_scalarmul)(p,API_NS(precomputed_base),nonce_scalar_2);
        API_NS(point_mul_by_ratio_and_encode_like_eddsa)(nonce_point, p);
        API_NS(point_destroy)(p);
        API_NS(scalar_destroy)(nonce_scalar_2);
    }

    {
        uint8_t challenge[2*GOLDILOCKS_EDDSA_448_PRIVATE_BYTES];
        /* Compute the challenge */
        hash_init_with_dom(hash,prehashed,0,context,context_len);
        hash_update(hash,nonce_point,sizeof(nonce_point));
        hash_update(hash,pubkey,GOLDILOCKS_EDDSA_448_PUBLIC_BYTES);
        hash_update(hash,message,message_len);
        hash_final(hash,challenge,sizeof(challenge));
        hash_destroy(hash);
        API_NS(scalar_decode_long)(challenge_scalar,challenge,sizeof(challenge));
        goldilocks_bzero(challenge,sizeof(challenge));
    }

    API_NS(scalar_mul)(challenge_scalar,challenge_scalar,secret_scalar);
    API_NS(scalar_add)(challenge_scalar,challenge_scalar,nonce_scalar);

    goldilocks_bzero(signature,GOLDILOCKS_EDDSA_448_SIGNATURE_BYTES);
    memcpy(signature,nonce_point,sizeof(nonce_point));
    API_NS(scalar_encode)(&signature[GOLDILOCKS_EDDSA_448_PUBLIC_BYTES],challenge_scalar);

    API_NS(scalar_destroy)(secret_scalar);
    API_NS(scalar_destroy)(nonce_scalar);
    API_NS(scalar_destroy)(challenge_scalar);
}


void goldilocks_ed448_sign_prehash (
    uint8_t signature[GOLDILOCKS_EDDSA_448_SIGNATURE_BYTES],
    const uint8_t privkey[GOLDILOCKS_EDDSA_448_PRIVATE_BYTES],
    const uint8_t pubkey[GOLDILOCKS_EDDSA_448_PUBLIC_BYTES],
    const goldilocks_ed448_prehash_ctx_p hash,
    const uint8_t *context,
    uint8_t context_len
) {
    uint8_t hash_output[EDDSA_PREHASH_BYTES];
    {
        goldilocks_ed448_prehash_ctx_p hash_too;
        memcpy(hash_too,hash,sizeof(hash_too));
        hash_final(hash_too,hash_output,sizeof(hash_output));
        hash_destroy(hash_too);
    }

    goldilocks_ed448_sign(signature,privkey,pubkey,hash_output,sizeof(hash_output),1,context,context_len);
    goldilocks_bzero(hash_output,sizeof(hash_output));
}

goldilocks_error_t goldilocks_ed448_verify (
    const uint8_t signature[GOLDILOCKS_EDDSA_448_SIGNATURE_BYTES],
    const uint8_t pubkey[GOLDILOCKS_EDDSA_448_PUBLIC_BYTES],
    const uint8_t *message,
    size_t message_len,
    uint8_t prehashed,
    const uint8_t *context,
    uint8_t context_len
) {
    API_NS(point_p) pk_point, r_point;
    API_NS(scalar_p) challenge_scalar;
    API_NS(scalar_p) response_scalar;
    unsigned  int c;
    goldilocks_error_t error = API_NS(point_decode_like_eddsa_and_mul_by_ratio)(pk_point,pubkey);
    if (GOLDILOCKS_SUCCESS != error) { return error; }

    error = API_NS(point_decode_like_eddsa_and_mul_by_ratio)(r_point,signature);
    if (GOLDILOCKS_SUCCESS != error) { return error; }

    {
        /* Compute the challenge */
        hash_ctx_p hash;
        uint8_t challenge[2*GOLDILOCKS_EDDSA_448_PRIVATE_BYTES];
        hash_init_with_dom(hash,prehashed,0,context,context_len);
        hash_update(hash,signature,GOLDILOCKS_EDDSA_448_PUBLIC_BYTES);
        hash_update(hash,pubkey,GOLDILOCKS_EDDSA_448_PUBLIC_BYTES);
        hash_update(hash,message,message_len);
        hash_final(hash,challenge,sizeof(challenge));
        hash_destroy(hash);
        API_NS(scalar_decode_long)(challenge_scalar,challenge,sizeof(challenge));
        goldilocks_bzero(challenge,sizeof(challenge));
    }
    API_NS(scalar_sub)(challenge_scalar, API_NS(scalar_zero), challenge_scalar);

    API_NS(scalar_decode_long)(
        response_scalar,
        &signature[GOLDILOCKS_EDDSA_448_PUBLIC_BYTES],
        GOLDILOCKS_EDDSA_448_PRIVATE_BYTES
    );

    for (c=1; c<GOLDILOCKS_448_EDDSA_DECODE_RATIO; c<<=1) {
        API_NS(scalar_add)(response_scalar,response_scalar,response_scalar);
    }


    /* pk_point = -c(x(P)) + (cx + k)G = kG */
    API_NS(base_double_scalarmul_non_secret)(
        pk_point,
        response_scalar,
        pk_point,
        challenge_scalar
    );
    return goldilocks_succeed_if(API_NS(point_eq(pk_point,r_point)));
}


goldilocks_error_t goldilocks_ed448_verify_prehash (
    const uint8_t signature[GOLDILOCKS_EDDSA_448_SIGNATURE_BYTES],
    const uint8_t pubkey[GOLDILOCKS_EDDSA_448_PUBLIC_BYTES],
    const goldilocks_ed448_prehash_ctx_p hash,
    const uint8_t *context,
    uint8_t context_len
) {
    goldilocks_error_t ret;

    uint8_t hash_output[EDDSA_PREHASH_BYTES];
    {
        goldilocks_ed448_prehash_ctx_p hash_too;
        memcpy(hash_too,hash,sizeof(hash_too));
        hash_final(hash_too,hash_output,sizeof(hash_output));
        hash_destroy(hash_too);
    }

    ret = goldilocks_ed448_verify(signature,pubkey,hash_output,sizeof(hash_output),1,context,context_len);

    return ret;
}
