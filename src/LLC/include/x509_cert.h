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

#include <Util/include/runtime.h>

#include <LLC/types/uint1024.h>

#include <cstdint>
#include <string>
#include <vector>

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
         *  @param[in] strPEM Path and filename of the certificate to load (in PEM format)
         *  @param[in] strKey Path and filename of the private key file
         *  @param[in] strCABundle Path and filename of the certificate bundle for the Certificate Authority
         * 
         *  @return Returns true if loaded successfully, false otherwise.
         *
         **/
        bool Read(const std::string& strCert, const std::string& strKey, const std::string& strCABundle);


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
         *  Check that the private key matches the public key in the x509 certificate and then checks the signature
         *
         *  @param[in] fCheckPrivate Flag indicating the private key should be checked
         * 
         *  @return Returns true if public and private key are consistent, false otherwise.
         *
         **/
        bool Verify(bool fCheckPrivate = true);


        /** Verify
         *
         *  Check that the private key matches the public key in the x509 certificate and then checks the signature
         *
         *  @param[in] ssl The ssl object to check.
         *  @param[in] fCheckPrivate Flag indicating the private key should be checked
         *
         *  @return Returns true if verified, false otherwise.
         *
         **/
        bool Verify(SSL *ssl, bool fCheckPrivate = true);


        /** Verify
         *
         *  Check that the private key matches the public key in the x509 certificate and then checks the signature
         *
         *  @param[in] ssl_ctx The ssl context object to check.
         *  @param[in] fCheckPrivate Flag indicating the private key should be checked
         *
         *  @return Returns true if verified, false otherwise.
         *
         **/
        bool Verify(SSL_CTX *ssl_ctx, bool fCheckPrivate = true);


        /** Print
         *
         *  Prints out information about the certificate with human readable format.
         *
         **/
        void Print();


        /** GenerateRSA
         *
         *  Generate an RSA keypair and correspoding certificate signed with the key.  This method is useful for creating
         *  ad-hoc one-off self-signed certificates where the private key is ephemeral.  The certificate validity is set to 1 year 
         * 
         *  @param[in] strCN The common name to set.
         *  @param[in] nValidFrom The timestamp to set the certificate validity from.
         *
         *  @return Returns true if successful, false otherwise.
         *
         **/
        bool GenerateRSA(const std::string& strCN, const uint64_t nValidFrom = runtime::unifiedtimestamp());


        /** GenerateEC
         *
         *  Generate a certificate using EC signature scheme, signed with the specified prigate key.  This method is useful when
         *  creating and regenerating self-signed certificates where the private key is persistant. 
         *  The certificate validity is set to 1 year 
         * 
         *  @param[in] hashSecret The private key to use .
         *  @param[in] strCN The common name to set.
         *  @param[in] nValidFrom The timestamp to set the certificate validity from.
         *
         *  @return Returns true if successful, false otherwise.
         *
         **/
        bool GenerateEC(const uint512_t& hashSecret, const std::string& strCN, const uint64_t nValidFrom = runtime::unifiedtimestamp());


        /** GetPEM
         *
         *  Gets the x509 certificate binary data in base64 encoded PEM format.
         * 
         *  @param[out] vchCertificate Vector to be populated with the certificate bytes .
         *
         *  @return Returns true if successful, false otherwise.
         *
         **/
        bool GetPEM(std::vector<uint8_t>& vchCertificate) const;


        /** GetPublicKey
         *
         *  Gets the public key used to sign this certificate.
         * 
         *  @param[out] vchKey Vector to be populated with the public key bytes .
         *
         *  @return Returns true if successful, false otherwise.
         *
         **/
        bool GetPublicKey(std::vector<uint8_t>& vchKey) const;


        /** Load
         *
         *  Loads the certificate data, which is passed to the method in base64 encoded PEM format.
         * 
         *  @param[in] vchCertificate Vector of bytes containing the PEM data .
         *
         *  @return Returns true if successful, false otherwise.
         *
         **/
        bool Load(const std::vector<uint8_t>& vchCertificate);


        /** Hash()
         *
         *  Returns a 256-bit hash of the certificate data, with the exception of the signature. 
         * 
         *  @return The 256-bit hash of the certificate data.
         *
         **/
        uint256_t Hash();


        /** GetCN()
         *
         *  Returns the content of the Common Name (CN) field from the certificate. 
         * 
         *  @return The content of the Common Name (CN) field from the certificate.
         *
         **/
        std::string GetCN();


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


        /** verify
         *
         *  Verify the data and signature of the x509 certificate.
         *
         *  @return Returns true if verification, false otherwise.
         *
         **/
        bool verify();


        /* The OpenSSL x509 certificate object. */
        X509 *px509;

        /* The OpenSSL key object. */
        EVP_PKEY *pkey;


        /* The number of bits for the RSA key generation. */
        uint32_t nBits;

        /* The path to the Certificate Authority cert bundle */
        std::string strCertBundle;

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
