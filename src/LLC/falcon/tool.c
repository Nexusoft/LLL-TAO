/*
 * Command-line tool for Falcon.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "falcon.h"

static void *
xmalloc(size_t len)
{
	void *buf;

	if (len == 0) {
		return NULL;
	}
	buf = malloc(len);
	if (buf == NULL) {
		fprintf(stderr, "memory allocation error\n");
		exit(EXIT_FAILURE);
	}
	return buf;
}

static void
xfree(void *buf)
{
	if (buf != NULL) {
		free(buf);
	}
}

static unsigned char *
read_file(const char *fname, size_t *data_len)
{
	FILE *f;
	unsigned char *buf;
	size_t ptr, len;

	f = fopen(fname, "rb");
	if (f == NULL) {
		fprintf(stderr, "could not open file '%s'\n", fname);
		exit(EXIT_FAILURE);
	}
	buf = NULL;
	ptr = 0;
	len = 0;
	for (;;) {
		unsigned char tmp[4096];
		size_t rlen;

		rlen = fread(tmp, 1, sizeof tmp, f);
		if (rlen == 0) {
			break;
		}
		if (buf == NULL) {
			buf = xmalloc(rlen);
			memcpy(buf, tmp, rlen);
			ptr = rlen;
			len = rlen;
		} else {
			if ((len - ptr) < rlen) {
				unsigned char *nbuf;

				len <<= 1;
				if ((len - ptr) < rlen) {
					len = ptr + rlen;
				}
				nbuf = xmalloc(len);
				memcpy(nbuf, buf, ptr);
				xfree(buf);
				buf = nbuf;
			}
			memcpy(buf + ptr, tmp, rlen);
			ptr += rlen;
		}
	}
	fclose(f);
	*data_len = ptr;
	return buf;
}

static void
write_file(const char *fname, const void *data, size_t len)
{
	const unsigned char *buf;
	FILE *f;

	f = fopen(fname, "wb");
	if (f == NULL) {
		fprintf(stderr, "could not open file '%s'\n", fname);
		exit(EXIT_FAILURE);
	}
	buf = data;
	while (len > 0) {
		size_t wlen;

		wlen = fwrite(buf, 1, len, f);
		if (wlen == 0) {
			fprintf(stderr, "write error in '%s'\n", fname);
			exit(EXIT_FAILURE);
		}
		buf += wlen;
		len -= wlen;
	}
	fclose(f);
}

static int
eqstr(const char *s1, const char *s2)
{
	for (;;) {
		int c1, c2;

		c1 = *s1 ++;
		c2 = *s2 ++;
		if (c1 >= 'A' && c1 <= 'Z') {
			c1 += 'a' - 'A';
		}
		if (c2 >= 'A' && c2 <= 'Z') {
			c2 += 'a' - 'A';
		}
		if (c1 != c2) {
			return 0;
		}
		if (c1 == 0) {
			return 1;
		}
	}
}

static unsigned char *
hextobin(const char *hex, size_t *len)
{
	unsigned char *buf;

	buf = NULL;
	for (;;) {
		const char *p;
		size_t u;
		int acc;
		int z;

		u = 0;
		acc = 0;
		z = 0;
		for (p = hex; *p; p ++) {
			int c;

			c = *p;
			if (c >= '0' && c <= '9') {
				c -= '0';
			} else if (c >= 'A' && c <= 'F') {
				c -= ('A' - 10);
			} else if (c >= 'a' && c <= 'f') {
				c -= ('a' - 10);
			} else if (c == ' ' || c == ':') {
				continue;
			} else {
				fprintf(stderr, "not an hexdigit: 0x%02X\n", c);
				exit(EXIT_FAILURE);
			}
			if (z) {
				if (buf != NULL) {
					buf[u] = (acc << 4) + c;
				}
				u ++;
			} else {
				acc = c;
			}
			z = !z;
		}
		if (z) {
			fprintf(stderr, "odd number of hexdigits\n");
			exit(EXIT_FAILURE);
		}
		if (buf == NULL) {
			buf = xmalloc(u);
		} else {
			*len = u;
			return buf;
		}
	}
}

static void
usage(void)
{
	fprintf(stderr,
"usage: falcon command [ options ]\n");
	fprintf(stderr,
"commands:\n");
	fprintf(stderr,
"   sign     compute a signature\n");
	fprintf(stderr,
"   verify   verify a signature\n");
	fprintf(stderr,
"   keygen   generate a key pair\n");
	exit(EXIT_FAILURE);
}

static void
usage_sign(void)
{
	fprintf(stderr,
"usage: falcon sign [ options ]\n"
"options:\n"
"   -key fname      read private key from file 'fname'\n"
"   -in fname       read data from file 'fname' ('-' for standard input)\n"
"   -out fname      write signature in file 'fname'\n"
"   -nonce hex      use specific nonce (provided in hexadecimal)\n"
"Normal output is the nonce (lowercase hexadecimal, one line) then the\n"
"signature (lowercase hexadecimal, one line). If '-out' is used, only the\n"
"nonce is printed on output, and the signature will be written out in\n"
"binary. If '-nonce' is used, then the nonce output is suppressed.\n");
	exit(EXIT_FAILURE);
}

static void
do_sign(int argc, char *argv[])
{
	int i;
	const char *fname_key;
	const char *fname_in;
	const char *fname_out;
	const char *hexnonce;
	unsigned char *skey;
	size_t skey_len;
	unsigned char *nonce;
	size_t nonce_len;
	falcon_sign *fs;
	FILE *df;
	unsigned char sig[2049];
	size_t sig_len;

	fname_key = NULL;
	fname_in = NULL;
	fname_out = NULL;
	hexnonce = NULL;
	for (i = 0; i < argc; i ++) {
		const char *opt;

		opt = argv[i];
		if (eqstr(opt, "-key")) {
			if (++ i >= argc) {
				fprintf(stderr, "missing arg for '-key'\n");
				usage_sign();
			}
			if (fname_key != NULL) {
				fprintf(stderr, "duplicate '-key'\n");
				usage_sign();
			}
			fname_key = argv[i];
		} else if (eqstr(opt, "-in")) {
			if (++ i >= argc) {
				fprintf(stderr, "missing arg for '-in'\n");
				usage_sign();
			}
			if (fname_in != NULL) {
				fprintf(stderr, "duplicate '-in'\n");
				usage_sign();
			}
			fname_in = argv[i];
		} else if (eqstr(opt, "-out")) {
			if (++ i >= argc) {
				fprintf(stderr, "missing arg for '-out'\n");
				usage_sign();
			}
			if (fname_out != NULL) {
				fprintf(stderr, "duplicate '-out'\n");
				usage_sign();
			}
			fname_out = argv[i];
		} else if (eqstr(opt, "-nonce")) {
			if (++ i >= argc) {
				fprintf(stderr, "missing arg for '-nonce'\n");
				usage_sign();
			}
			if (hexnonce != NULL) {
				fprintf(stderr, "duplicate '-nonce'\n");
				usage_sign();
			}
			hexnonce = argv[i];
		} else {
			fprintf(stderr, "unknown option: '%s'\n", opt);
			usage_sign();
		}
	}

	if (fname_key == NULL) {
		fprintf(stderr, "no private key specified\n");
		usage_sign();
	}
	skey = read_file(fname_key, &skey_len);
	if (hexnonce != NULL) {
		nonce = hextobin(hexnonce, &nonce_len);
	} else {
		nonce = NULL;
	}

	fs = falcon_sign_new();
	if (fs == NULL) {
		fprintf(stderr, "memory allocation error\n");
		exit(EXIT_FAILURE);
	}

	if (!falcon_sign_set_private_key(fs, skey, skey_len)) {
		fprintf(stderr, "error loading private key\n");
		exit(EXIT_FAILURE);
	}

	/*
	 * We get the degree from the first key byte (this is for the
	 * 'recording' test feature).
	 */
	xfree(skey);

	if (nonce == NULL) {
		unsigned char r[40];
		size_t u;

		if (!falcon_sign_start(fs, r)) {
			fprintf(stderr, "RNG seeding failed\n");
			exit(EXIT_FAILURE);
		}
		for (u = 0; u < sizeof r; u ++) {
			printf("%02x", r[u]);
		}
		printf("\n");
	} else {
		falcon_sign_start_external_nonce(fs, nonce, nonce_len);
		xfree(nonce);
	}

	/*
	 * Read and hash data to sign.
	 */
	if (fname_in != NULL) {
		df = fopen(fname_in, "rb");
		if (df == NULL) {
			fprintf(stderr, "error opening file '%s'\n", fname_in);
			exit(EXIT_FAILURE);
		}
	} else {
		df = stdin;
	}
	for (;;) {
		unsigned char tmp[4096];
		size_t rlen;

		rlen = fread(tmp, 1, sizeof tmp, df);
		if (rlen == 0) {
			break;
		}
		falcon_sign_update(fs, tmp, rlen);
	}
	if (df != stdin) {
		fclose(df);
	}

	/*
	 * Obtain signature.
	 */
	sig_len = falcon_sign_generate(fs,
		sig, sizeof sig, FALCON_COMP_STATIC);
	if (sig_len == 0) {
		fprintf(stderr, "signature failure\n");
		exit(EXIT_FAILURE);
	}

	if (fname_out == NULL) {
		size_t u;

		for (u = 0; u < sig_len; u ++) {
			printf("%02x", sig[u]);
		}
		printf("\n");
	} else {
		write_file(fname_out, sig, sig_len);
	}

	falcon_sign_free(fs);
}

static void
usage_vrfy(void)
{
	fprintf(stderr,
"usage: falcon verify [ options ]\n"
"options:\n"
"   -key fname      read public key from file 'fname'\n"
"   -in fname       read data from file 'fname' ('-' for standard input)\n"
"   -nonce hex      set nonce (provided in hexadecimal)\n"
"   -sig fname      read signature from file 'fname'\n"
"   -sighex hex     set signature (provided in hexadecimal)\n"
"Output will be 'OK' for a valid signature, 'INVALID' for an invalid\n"
"signature. The key, signature and nonce MUST be provided.\n");
	exit(EXIT_FAILURE);
}

static void
do_vrfy(int argc, char *argv[])
{
	int i;
	const char *fname_key;
	const char *fname_in;
	const char *fname_sig;
	const char *hexnonce;
	const char *hexsig;
	unsigned char *pkey;
	size_t pkey_len;
	unsigned char *nonce;
	size_t nonce_len;
	unsigned char *sig;
	size_t sig_len;
	falcon_vrfy *fv;
	FILE *df;
	int z;

	fname_key = NULL;
	fname_in = NULL;
	fname_sig = NULL;
	hexnonce = NULL;
	hexsig = NULL;
	for (i = 0; i < argc; i ++) {
		const char *opt;

		opt = argv[i];
		if (eqstr(opt, "-key")) {
			if (++ i >= argc) {
				fprintf(stderr, "missing arg for '-key'\n");
				usage_vrfy();
			}
			if (fname_key != NULL) {
				fprintf(stderr, "duplicate '-key'\n");
				usage_vrfy();
			}
			fname_key = argv[i];
		} else if (eqstr(opt, "-in")) {
			if (++ i >= argc) {
				fprintf(stderr, "missing arg for '-in'\n");
				usage_vrfy();
			}
			if (fname_in != NULL) {
				fprintf(stderr, "duplicate '-in'\n");
				usage_vrfy();
			}
			fname_in = argv[i];
		} else if (eqstr(opt, "-sig")) {
			if (++ i >= argc) {
				fprintf(stderr, "missing arg for '-sig'\n");
				usage_vrfy();
			}
			if (fname_sig != NULL) {
				fprintf(stderr, "duplicate '-sig'\n");
				usage_vrfy();
			}
			fname_sig = argv[i];
		} else if (eqstr(opt, "-nonce")) {
			if (++ i >= argc) {
				fprintf(stderr, "missing arg for '-nonce'\n");
				usage_vrfy();
			}
			if (hexnonce != NULL) {
				fprintf(stderr, "duplicate '-nonce'\n");
				usage_vrfy();
			}
			hexnonce = argv[i];
		} else if (eqstr(opt, "-sighex")) {
			if (++ i >= argc) {
				fprintf(stderr, "missing arg for '-sighex'\n");
				usage_vrfy();
			}
			if (hexsig != NULL) {
				fprintf(stderr, "duplicate '-sighex'\n");
				usage_vrfy();
			}
			hexsig = argv[i];
		} else {
			fprintf(stderr, "unknown option: '%s'\n", opt);
			usage_vrfy();
		}
	}

	if (fname_key == NULL) {
		fprintf(stderr, "no public key specified\n");
		usage_vrfy();
	}
	if (hexnonce == NULL) {
		fprintf(stderr, "no nonce provided\n");
		usage_vrfy();
	}
	if (fname_sig == NULL && hexsig == NULL) {
		fprintf(stderr, "no signature provided\n");
		usage_vrfy();
	}
	if (fname_sig != NULL && hexsig != NULL) {
		fprintf(stderr, "two signatures provided\n");
		usage_vrfy();
	}

	pkey = read_file(fname_key, &pkey_len);
	nonce = hextobin(hexnonce, &nonce_len);
	if (fname_sig != NULL) {
		sig = read_file(fname_sig, &sig_len);
	} else {
		sig = hextobin(hexsig, &sig_len);
	}

	fv = falcon_vrfy_new();
	if (fv == NULL) {
		fprintf(stderr, "memory allocation error\n");
		exit(EXIT_FAILURE);
	}

	if (!falcon_vrfy_set_public_key(fv, pkey, pkey_len)) {
		fprintf(stderr, "error loading public key\n");
		exit(EXIT_FAILURE);
	}
	xfree(pkey);

	falcon_vrfy_start(fv, nonce, nonce_len);
	xfree(nonce);

	/*
	 * Read and hash data to sign.
	 */
	if (fname_in != NULL) {
		df = fopen(fname_in, "rb");
		if (df == NULL) {
			fprintf(stderr, "error opening file '%s'\n", fname_in);
			exit(EXIT_FAILURE);
		}
	} else {
		df = stdin;
	}
	for (;;) {
		unsigned char tmp[4096];
		size_t rlen;

		rlen = fread(tmp, 1, sizeof tmp, df);
		if (rlen == 0) {
			break;
		}
		falcon_vrfy_update(fv, tmp, rlen);
	}
	if (df != stdin) {
		fclose(df);
	}

	/*
	 * Verify signature.
	 */
	z = falcon_vrfy_verify(fv, sig, sig_len);
	xfree(sig);
	falcon_vrfy_free(fv);

	if (z > 0) {
		printf("OK\n");
	} else {
		printf("INVALID: %d\n", z);
		exit(EXIT_FAILURE);
	}
}

static void
usage_keygen(void)
{
	fprintf(stderr,
"usage: falcon keygen [ options ]\n"
"options:\n"
"   -priv fname     write private key into file 'fname'\n"
"   -pub fname      write public key into file 'fname'\n"
"   -logn logn      use degree 2^logn (default is 512 = 2^9)\n"
"   -logt logn      use degree 1.5*2^logn\n"
"Only one of -logn and -logt may be specified.\n");
	exit(EXIT_FAILURE);
}

static void
do_keygen(int argc, char *argv[])
{
	int i;
	const char *fname_priv;
	const char *fname_pub;
	unsigned logn;
	int ternary;
	falcon_keygen *fk;
	unsigned char *privkey;
	size_t privkey_len;
	unsigned char *pubkey;
	size_t pubkey_len;
	int ret;

	fname_priv = NULL;
	fname_pub = NULL;
	logn = 0;
	ternary = 0;
	for (i = 0; i < argc; i ++) {
		const char *opt;

		opt = argv[i];
		if (eqstr(opt, "-priv")) {
			if (++ i >= argc) {
				fprintf(stderr, "missing arg for '-priv'\n");
				usage_keygen();
			}
			if (fname_priv != NULL) {
				fprintf(stderr, "duplicate '-priv'\n");
				usage_keygen();
			}
			fname_priv = argv[i];
		} else if (eqstr(opt, "-pub")) {
			if (++ i >= argc) {
				fprintf(stderr, "missing arg for '-pub'\n");
				usage_keygen();
			}
			if (fname_pub != NULL) {
				fprintf(stderr, "duplicate '-pub'\n");
				usage_keygen();
			}
			fname_pub = argv[i];
		} else if (eqstr(opt, "-logn")) {
			int x;

			if (++ i >= argc) {
				fprintf(stderr, "missing arg for '-logn'\n");
				usage_keygen();
			}
			if (logn != 0) {
				fprintf(stderr, "duplicate '-logn'/'-logt'\n");
				usage_keygen();
			}
			x = atoi(argv[i]);
			if (x < 1 || x > 10) {
				fprintf(stderr, "unsupported degree\n");
				usage_keygen();
			}
			logn = (unsigned)x;
		} else if (eqstr(opt, "-logt")) {
			int x;

			if (++ i >= argc) {
				fprintf(stderr, "missing arg for '-logt'\n");
				usage_keygen();
			}
			if (logn != 0) {
				fprintf(stderr, "duplicate '-logn'/'-logt'\n");
				usage_keygen();
			}
			x = atoi(argv[i]);
			if (x < 2 || x > 9) {
				fprintf(stderr, "unsupported degree\n");
				usage_keygen();
			}
			logn = (unsigned)x;
			ternary = 1;
		} else {
			fprintf(stderr, "unknown option: '%s'\n", opt);
			usage_keygen();
		}
	}

	if (fname_priv == NULL) {
		fprintf(stderr, "no output file for private key\n");
		usage_keygen();
	}

	if (logn == 0) {
		logn = 9;
	}

	fk = NULL;
	privkey = NULL;
	pubkey = NULL;

	fk = falcon_keygen_new(logn, ternary);
	if (fk == NULL) {
		fprintf(stderr, "memory allocation error\n");
		exit(EXIT_FAILURE);
	}

	privkey_len = falcon_keygen_max_privkey_size(fk);
	privkey = xmalloc(privkey_len);
	pubkey_len = falcon_keygen_max_pubkey_size(fk);
	pubkey = xmalloc(pubkey_len);

	ret = falcon_keygen_make(fk, FALCON_COMP_STATIC,
		privkey, &privkey_len, pubkey, &pubkey_len);
	if (ret) {
		write_file(fname_priv, privkey, privkey_len);
		if (fname_pub != NULL) {
			write_file(fname_pub, pubkey, pubkey_len);
		}
	} else {
		fprintf(stderr, "key generation failed\n");
	}

	falcon_keygen_free(fk);
	xfree(privkey);
	xfree(pubkey);

	if (!ret) {
		exit(EXIT_FAILURE);
	}
}

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		usage();
	}
	if (eqstr(argv[1], "sign")) {
		do_sign(argc - 2, argv + 2);
	} else if (eqstr(argv[1], "vrfy") || eqstr(argv[1], "verify")) {
		do_vrfy(argc - 2, argv + 2);
	} else if (eqstr(argv[1], "keygen")) {
		do_keygen(argc - 2, argv + 2);
	} else {
		usage();
	}
	return 0;
}
