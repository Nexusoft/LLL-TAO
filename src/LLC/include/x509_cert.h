/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLC_INCLUDE_X509_CERT_H
#define NEXUS_LLC_INCLUDE_X509_CERT_H

#include <cstdint>

/** Forward declarations. **/
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_st SSL;
typedef struct rsa_st RSA;
typedef struct evp_pkey_st EVP_PKEY;
typedef struct x509_st X509;
typedef struct x509_store_ctx_st X509_STORE_CTX;


namespace LLC
{
    class X509Cert
    {
    public:

        /** Default Constructor
         *
         *  @param[in] bits The number of bits used for RSA key generation.
         *
         **/
        X509Cert(uint32_t bits = 2048);


        /** Default Destructor **/
        ~X509Cert();


        /** Write
         *
         *  Writes the certificate and private key PEM files to an ssl folder located in the default data path.
         *  The ssl folder will be created if it doesn't exist.
         *
         *  @return Returns true if file writes are successful, false otherwise.
         *
         **/
        bool Write();


        /** Read
         *
         *  Reads the certificate and private key PEM files from an ssl folder located in the default data path
         *
         *  @return Returns true if loaded successfully, false otherwise.
         *
         **/
        bool Read();


        /** Init_SSL
         *
         *  Modifies the SSL internal state with certificate and key information.
         *
         *  @param[in/out] ssl The ssl object to load certificate and private key information into.
         *
         *  @return Returns true if successfully applied, false otherwise.
         *
         **/
        bool Init_SSL(SSL *ssl);


        /** Init_SSL_CTX
         *
         *  Modifies the SSL context internal state with certificate and key information. Every SSL object created with
         *  this context will share the same certification info.
         *
         *  @param[in/out] ssl_ctx The ssl context to load certificate and private key information into.
         *
         *  @return Returns true if successfully applied, false otherwise.
         *
         **/
        bool Init_SSL(SSL_CTX *ssl_ctx);


        /** Verify
         *
         *  Check that the private key matches the public key in the x509 certificate.
         *
         *  @return Returns true if public and private key are consistent, false otherwise.
         *
         **/
        bool Verify();


        /** Verify
         *
         *  Check the authenticity of the public key in the certificate and the private key paired with it.
         *
         *  @param[in] ssl The ssl object to check.
         *
         *  @return Returns true if verified, false otherwise.
         *
         **/
        bool Verify(SSL *ssl);


        /** Verify
         *
         *  Check the authenticity of the public key in the certificate and the private key paired with it.
         *
         *  @param[in] ssl_ctx The ssl context object to check.
         *
         *  @return Returns true if verified, false otherwise.
         *
         **/
        bool Verify(SSL_CTX *ssl_ctx);


        /** Print
         *
         *  Prints out information about the certificate with human readable format.
         *
         **/
        void Print();


        /** Generate
         *
         *  Generate the private key and certificate.
         *
         *  @return Returns true if successful, false otherwise.
         *
         **/
        bool Generate();


    private:

        /** init
         *
         *  Initializes a new certificate and private key.
         *
         **/
        void init();


        /** shutdown
         *
         *  Frees memory associated with the certificate and private key.
         *
         **/
        void shutdown();


        /* The OpenSSL x509 certificate object. */
        X509 *px509;

        /* The OpenSSL key object. */
        EVP_PKEY *pkey;


        /* The number of bits for the RSA key generation. */
        uint32_t nBits;

    };


    /** PeerCertificateInfo
     *
     *  Print information about peer certificate.
     *
     **/
    void PeerCertificateInfo(SSL *ssl);


    /** PrintCert
     *
     *  Print information about certificate.
     *
     **/
    void PrintCert(X509 *cert);



    /** always_true_callback
     *
     * Always returns true. Callback function.
     *
     **/
    int always_true_callback(int ok, X509_STORE_CTX *ctx);

}

#endif
