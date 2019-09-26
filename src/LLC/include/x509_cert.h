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
typedef struct ssl_st SSL;
typedef struct rsa_st RSA;
typedef struct evp_pkey_st EVP_PKEY;
typedef struct x509_st X509;


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
         *  Writes the certificate and private key PEM files to an ssl folder located in the default directory path.
         *  The ssl folder will be created if it doesn't exist.
         *
         *  @return Returns true if file writes are successful, false otherwise.
         *
         **/
        bool Write();


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


        /** Print
         *
         *  Prints out information about the certificate with human readable format.
         *
         **/
        void Print();


    private:

        /** init_cert
         *
         *  Initializes and creates a new certificate signed with a unique RSA private key.
         *
         *  @return Returns true if certificate was successfully created, false otherwise.
         *
         **/
        bool init_cert();


        /** free_cert
         *
         *  Frees memory associated with the certificate and key.
         *
         **/
        void free_cert();


        /* The OpenSSL x509 certificate object. */
        X509 *px509;

        /* The OpenSSL key object. */
        EVP_PKEY *pkey;

        /* The OpenSSL RSA key object. */
        RSA *pRSA;

        /* The number of bits for the RSA key generation. */
        uint32_t nBits;

    };




}

#endif
