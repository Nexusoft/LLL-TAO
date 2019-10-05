/*
 * Wrapper for implementing the NIST API for the PQC competition.
 */

#include <stddef.h>
#include <string.h>

#include "api.h"
#include "falcon.h"

#define PARAM_LOGN      9
#define PARAM_TERNARY   0
#define PARAM_NONCE     40

void randombytes_init(unsigned char *entropy_input,
	unsigned char *personalization_string,
	int security_strength);
int randombytes(unsigned char *x, unsigned long long xlen);

int
crypto_sign_keypair(unsigned char *pk, unsigned char *sk)
{
	falcon_keygen *fk;
	unsigned char seed[48];
	size_t pklen, sklen;
	int r;

	fk = falcon_keygen_new(PARAM_LOGN, PARAM_TERNARY);
	if (fk == NULL) {
		return -1;
	}
	randombytes(seed, sizeof seed);
	falcon_keygen_set_seed(fk, seed, sizeof seed, 1);
	pklen = CRYPTO_PUBLICKEYBYTES;
	sklen = CRYPTO_SECRETKEYBYTES;
	r = falcon_keygen_make(fk, FALCON_COMP_NONE, sk, &sklen, pk, &pklen);
	if (pklen != CRYPTO_PUBLICKEYBYTES || sklen != CRYPTO_SECRETKEYBYTES) {
		r = 0;
	}
	falcon_keygen_free(fk);
	return r > 0 ? 0 : -1;
}

int
crypto_sign(unsigned char *sm, unsigned long long *smlen,
	const unsigned char *m, unsigned long long mlen,
	const unsigned char *sk)
{
	falcon_sign *fs;
	unsigned char seed[48];
	unsigned char nonce[PARAM_NONCE];
	unsigned char sig[CRYPTO_BYTES - 2 - PARAM_NONCE];
	size_t sig_len, off;
	int r;

	r = -1;
	fs = falcon_sign_new();
	if (fs == NULL) {
		goto exit_crypto_sign;
	}
	randombytes(seed, sizeof seed);
	falcon_sign_set_seed(fs, seed, sizeof seed, 1);
	if (!falcon_sign_set_private_key(fs, sk, CRYPTO_SECRETKEYBYTES)) {
		goto exit_crypto_sign;
	}
	if (!falcon_sign_start(fs, nonce)) {
		goto exit_crypto_sign;
	}
	falcon_sign_update(fs, m, mlen);
	sig_len = falcon_sign_generate(fs,
		sig, sizeof sig, FALCON_COMP_STATIC);
	if (sig_len == 0) {
		goto exit_crypto_sign;
	}

	/*
	 * Because the API insists on encoding the message and the
	 * signature in a single blob, and does not offer much support
	 * for variable-length signatures, we need to do a custom
	 * encoding:
	 *
	 *   sig_len   signature length (in bytes): 2 bytes, big-endian
	 *   nonce     40 bytes
	 *   message   the message itself
	 *   sig       signature
	 *
	 * For streamed processing, the nonce would have to be known before
	 * processing the bulk of the message, but the signature length
	 * would be known only at the end.
	 *
	 * FIXME: when signatures with recovery are implemented, modify
	 * the format so as to leverage them (lower size overhead).
	 */
	off = 0;
	sm[off ++] = (unsigned char)(sig_len >> 8);
	sm[off ++] = (unsigned char)sig_len;
	memcpy(sm + off, nonce, sizeof nonce);
	off += sizeof nonce;
	memcpy(sm + off, m, mlen);
	off += mlen;
	memcpy(sm + off, sig, sig_len);
	off += sig_len;
	*smlen = off;
	r = 0;

exit_crypto_sign:
	falcon_sign_free(fs);
	return r;
}

int
crypto_sign_open(unsigned char *m, unsigned long long *mlen,
	const unsigned char *sm, unsigned long long smlen,
	const unsigned char *pk)
{
	falcon_vrfy *fv;
	int r;
	const unsigned char *sig, *msg;
	size_t sig_len, msg_len;

	r = -1;
	fv = falcon_vrfy_new();
	if (fv == NULL) {
		goto exit_crypto_sign_open;
	}
	if (!falcon_vrfy_set_public_key(fv, pk, CRYPTO_PUBLICKEYBYTES)) {
		goto exit_crypto_sign_open;
	}
	if (smlen < (2 + PARAM_NONCE)) {
		goto exit_crypto_sign_open;
	}
	sig_len = ((size_t)sm[0] << 8) + sm[1];
	if (sig_len > (smlen - (2 + PARAM_NONCE))) {
		goto exit_crypto_sign_open;
	}
	msg = sm + 2 + PARAM_NONCE;
	msg_len = smlen - (2 + PARAM_NONCE) - sig_len;
	sig = msg + msg_len;
	falcon_vrfy_start(fv, sm + 2, PARAM_NONCE);
	falcon_vrfy_update(fv, msg, msg_len);
	if (falcon_vrfy_verify(fv, sig, sig_len) > 0) {
		r = 0;
		memcpy(m, msg, msg_len);
		*mlen = msg_len;
	}

exit_crypto_sign_open:
	falcon_vrfy_free(fv);
	return r;
}
