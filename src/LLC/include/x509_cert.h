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


typedef struct rsa_st RSA;
typedef struct evp_pkey_st EVP_PKEY;
typedef struct x509_st X509;


namespace LLC
{

    class X509Cert
    {
    public:
        X509Cert(uint32_t bits = 2048);

        ~X509Cert();

        bool Write();

    private:

        bool init_cert();


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
