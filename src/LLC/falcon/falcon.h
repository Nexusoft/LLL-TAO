/*
 * Public API for Falcon.
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

#ifndef FALCON_H__
#define FALCON_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================================================================== */
/*
 * Small vector compression.
 *
 * Signatures and private keys contain small vectors. They are optionally
 * compressed. Several compression algorithms are defined.
 */

/* No compression. */
#define FALCON_COMP_NONE      0

/* Compression with static codes (approximation of Huffman codes). */
#define FALCON_COMP_STATIC    1

/* ==================================================================== */
/*
 * Signature verifier.
 *
 * An opaque falcon_vrfy context structure must be allocated with
 * falcon_vrfy_new(). That context must be ultimately released with
 * falcon_vrfy_free().
 *
 * The public key is set with falcon_vrfy_set_public_key(). The public
 * key must be set at some point before calling falcon_vrfy_verify(),
 * but part or all of the signed message may be injected before setting
 * the public key. Several successive signature verifications can be
 * performed with the same context; the public key is preserved, unless
 * changed explicitly.
 *
 * To start a new verification, call falcon_vrfy_start(). Then inject
 * the signed message data with zero, one or more calls to
 * falcon_vrfy_update(). Finally, falcon_vrfy_verify() checks the
 * signature against the injected message and the set public key.
 *
 * Falcon signatures include an explicit nonce element, which must be
 * provided as parameter to falcon_vrfy_start(). The nonce should be
 * generated randomly by the signer.
 */

/*
 * Signature verifier context.
 */
typedef struct falcon_vrfy_ falcon_vrfy;

/*
 * Allocate a new falcon_vrfy structure. It will have to be released later
 * on with falcon_vrfy_free().
 *
 * This function returns NULL on memory allocation failure.
 */
falcon_vrfy *falcon_vrfy_new(void);

/*
 * Release a falcon_vrfy structure and all corresponding resources. This
 * function may be called at any time, cancelling the currently ongoing
 * signature verification. If 'fv' is NULL, then this function does
 * nothing.
 */
void falcon_vrfy_free(falcon_vrfy *fv);

/*
 * Set the public key against which signatures are to be verified.
 * The encoded public key is provided as pointer 'pkey', with encoded
 * length 'len' bytes.
 *
 * Returned value is 1 on success, 0 on error (invalid public key encoding).
 */
int falcon_vrfy_set_public_key(falcon_vrfy *fv,
	const void *pkey, size_t len);

/*
 * Reset the hashing mechanism for a new message. The 'r' value is the
 * random nonce which was used when generating the signature; it has
 * length 'rlen' bytes.
 */
void falcon_vrfy_start(falcon_vrfy *fv, const void *r, size_t rlen);

/*
 * Inject some message bytes. These bytes are added to the current message
 * to verify. The message bytes are pointed to by 'data' and have length
 * 'len' (in bytes). If 'len' is 0, then this call does nothing.
 */
void falcon_vrfy_update(falcon_vrfy *fv, const void *data, size_t len);

/*
 * Verify the provided signature against the currently injected message and
 * public key. The signature is in 'sig', of length 'len' bytes.
 *
 * Returned value is:
 *   1   signature is valid
 *   0   signature does not match the message + public key
 *  -1   signature decoding error (invalid / wrong modulus or degree)
 *  -2   public key was not set
 */
int falcon_vrfy_verify(falcon_vrfy *fv, const void *sig, size_t len);

/* ==================================================================== */
/*
 * Signature generator.
 *
 * An opaque falcon_sign context structure must be allocated with
 * falcon_sign_new(). That context must be ultimately released with
 * falcon_sign_free().
 *
 * The private key is set with falcon_sign_set_private_key(). The public
 * key must be set at some point before calling falcon_sign_generate(),
 * but part or all of the signed message may be injected before setting
 * the private key. Several successive signature generations can be
 * performed with the same context; the private key is preserved, unless
 * changed explicitly.
 *
 * To start a new signature generation, call falcon_sign_start(). Then
 * inject the signed message data with zero, one or more calls to
 * falcon_sign_update(). Finally, falcon_sign_verify() checks the
 * signature against the injected message and the set public key.
 *
 * Falcon signatures include an explicit nonce element. Normally, the
 * signature generator produces it as a sequence of 40 random bytes;
 * this is done in falcon_sign_start(). Alternatively,
 * falcon_sign_start_external_nonce() can be called instead of
 * falcon_sign_start(), to provide the nonce explicitly. In that case,
 * the caller is responsible for nonce generation. Nonces shall be
 * generated with a cryptographically strong RNG.
 */

/*
 * Signature generation context.
 */
typedef struct falcon_sign_ falcon_sign;

/*
 * Allocate a new falcon_sign structure. It will have to be released later
 * on with falcon_sign_free().
 *
 * This function returns NULL on memory allocation failure.
 */
falcon_sign *falcon_sign_new(void);

/*
 * Release a falcon_sign structure and all corresponding resources. This
 * function may be called at any time, cancelling the currently ongoing
 * signature generation. If 'fs' is NULL, then this function does
 * nothing.
 */
void falcon_sign_free(falcon_sign *fs);

/*
 * Set the random seed for signature generation. If this function is called,
 * then the internal RNG used for this specific signature will be seeded
 * with the provided value.
 *
 * If 'replace' is non-zero, then the provided seed _replaces_ any other
 * seed currently set in the RNG. This can be used for test purposes, to
 * ensure reproducible behaviour. Otherwise, the provided seed is added
 * to the internal pool, which will also use the system RNG.
 *
 * When falcon_sign_start() is called, the following happens:
 *
 *  - If falcon_sign_set_seed() was called with a non-zero 'replace'
 *    parameter, then that value is used as seed, along with all seeds
 *    injected with subsequent falcon_sign_set_seed() with a zero
 *    'replace'.
 *
 *  - Otherwise, the system RNG is used to obtain fresh randomness. If
 *    the system RNG fails, then a failure is reported and the engine
 *    is not started. If the system RNG succeeds, then the fresh randomness
 *    is added to all seeds provided with falcon_sign_set_seed() calls.
 */
void falcon_sign_set_seed(falcon_sign *fs,
	const void *seed, size_t len, int replace);

/*
 * Set the private key for signature generation. The encoded private key
 * is provided as pointer 'skey', with encoded length 'len' bytes.
 *
 * Returned value is 1 on success, 0 on error (invalid private key encoding
 * or memory allocation failure).
 */
int falcon_sign_set_private_key(falcon_sign *fs,
	const void *skey, size_t len);

/*
 * Reset the hashing mechanism for a new message. The "r" value shall
 * point to a 40-byte buffer, which is filled with a newly-generated
 * random nonce for this signature. That nonce value shall be conveyed
 * to verifiers along with the signature (it is required for proper
 * verification).
 *
 * Returned value is 1 on success, 0 on error. An error may be reported
 * if no system RNG could be successfully used to produce the nonce.
 */
int falcon_sign_start(falcon_sign *fs, void *r /* 40 bytes */);

/*
 * Reset the hashing mechanism for a new message. The provided nonce 'r'
 * (of length 'rlen' bytes) is used for computing the signature, and
 * shall be used by verifiers to verify the signature. The caller is
 * responsible for proper nonce generation: for security, that nonce
 * shall be produced with a cryptographically strong RNG and have
 * sufficient length to be unpredictable (40 bytes are recommended).
 */
void falcon_sign_start_external_nonce(falcon_sign *fs,
	const void *r, size_t rlen);

/*
 * Inject some message bytes. These bytes are added to the current message
 * to sign. The message bytes are pointed to by 'data' and have length
 * 'len' (in bytes). If 'len' is 0, then this call does nothing.
 */
void falcon_sign_update(falcon_sign *fs, const void *data, size_t len);

/*
 * Produce the signature. The signature being a short vector, it can
 * optionally be compressed; the compression algorithm to use is set
 * in the 'comp' parameter, and shall be one of the FALCON_COMP_*
 * constants.
 *
 * Returned value is the signature length, in bytes.
 *
 * On error, 0 is returned. Error conditions include the following:
 *
 *  - Private key was not set.
 *  - The system RNG could not be located, or failed.
 *  - The signature length would exceed the output buffer length (sig_max_len).
 *
 * If the private key has degree N, then an uncompressed signature
 * (FALCON_COMP_NONE) has length exactly 2*N+1 bytes. With Huffman
 * compression, the signature is shorter with overwhelming probability,
 * but its exact length may vary. Degree N is normally 512 or 1024,
 * depending on key parameters.
 */
size_t falcon_sign_generate(falcon_sign *fs,
	void *sig, size_t sig_max_len, int comp);

/* ==================================================================== */
/*
 * Key generator.
 *
 * Key pair generation uses an allocated context. That context keeps
 * track of the internal PRNG (this allows external seeding for
 * reproducible key generation) and contains some tables that can be
 * reused for producing several key pairs.
 */

/*
 * Key generation context.
 */
typedef struct falcon_keygen_ falcon_keygen;

/*
 * Create a new falcon key generation context.
 *
 * In the binary case (ternary = 0), the 'logn' parameter is the base-2
 * logarithm of the degree; it must be between 1 and 10 (normal Falcon
 * parameters use logn = 9 or 10; lower values are for reduced test-only
 * versions).
 *
 * In the ternary case (ternary = 1), the 'logn' parameter is the base-2
 * logarithm of 2/3rd of the degree (e.g. logn is 9 for degree 768). In
 * that case, 'logn' must lie between 2 and 9 (normal value is 9, lower
 * values are for reduced test-only versions).
 *
 * Returned value is the new context, or NULL on error. Errors include
 * out-of-range parameters, and memory allocation errors.
 */
falcon_keygen *falcon_keygen_new(unsigned logn, int ternary);

/*
 * Release a previously allocated key generation context, and all
 * corresponding resources. If 'fk' is NULL then this function does
 * nothing.
 */
void falcon_keygen_free(falcon_keygen *fk);

/*
 * Get the maximum encoded size (in bytes) of a private key that can be
 * generated with the provided context. When using no compression
 * (FALCON_COMP_NONE), this is the exact size; with compression,
 * private key will be shorter.
 */
size_t falcon_keygen_max_privkey_size(falcon_keygen *fk);

/*
 * Get the maximum encoded size (in bytes) of a public key that can be
 * generated with the provided context. Since public keys are uncompressed,
 * the returned size is always exact.
 */
size_t falcon_keygen_max_pubkey_size(falcon_keygen *fk);

/*
 * Set the random seed for key pair generation. If this function is called,
 * then the internal RNG used for this specific signature will be seeded
 * with the provided value.
 *
 * If 'replace' is non-zero, then the provided seed _replaces_ any other
 * seed currently set in the RNG. This can be used for test purposes, to
 * ensure reproducible behaviour. Otherwise, the provided seed is added
 * to the internal pool, which will also use the system RNG.
 *
 * When falcon_keygen_make() is called, the following happens:
 *
 *  - If falcon_keygen_set_seed() was called with a non-zero 'replace'
 *    parameter, then that value is used as seed, along with all seeds
 *    injected with subsequent falcon_keygen_set_seed() with a zero
 *    'replace'.
 *
 *  - Otherwise, the system RNG is used to obtain fresh randomness. If
 *    the system RNG fails, then a failure is reported and the engine
 *    is not started. If the system RNG succeeds, then the fresh randomness
 *    is added to all seeds provided with falcon_keygen_set_seed() calls.
 */
void falcon_keygen_set_seed(falcon_keygen *fk,
	const void *seed, size_t len, int replace);

/*
 * Generate a new key pair.
 *
 * The private key is written in 'privkey', using the compression algorithm
 * specified by 'comp'. The public key is written in 'pubkey'.
 *
 * The '*privkey_len' and '*pubkey_len' must initially be set to the
 * maximum lengths of the corresponding buffers; on output, they are
 * modified to characterize the actual private and public key lengths.
 *
 * Returned value is 1 on success, 0 on error. An error is reported in the
 * following cases:
 *
 *  - The system RNG failed to provide appropriate seeding.
 *
 *  - Some memory allocation failed.
 *
 *  - A new key pair could be internally produced, but could not be encoded
 *    because one of the output buffers is too small.
 */
int falcon_keygen_make(falcon_keygen *fk, int comp,
	void *privkey, size_t *privkey_len,
	void *pubkey, size_t *pubkey_len);

/* ==================================================================== */

#ifdef __cplusplus
}
#endif

#endif
