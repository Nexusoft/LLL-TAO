/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/x509_cert.h>
#include <LLC/include/key_error.h>

#include <Util/include/debug.h>
#include <Util/include/filesystem.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
#include <openssl/bn.h>
#include <openssl/ssl.h>

#include <cstdio>

namespace LLC
{

    /** Default Constructor **/
    X509Cert::X509Cert(uint32_t bits)
    : px509(nullptr)
    , pkey(nullptr)
    , pRSA(nullptr)
    , nBits(bits)
    {
        if(!init_cert())
            throw key_error("X509Cert::X509Cert : Certificate initialization failed.");
    }


    /** Default Destructor **/
    X509Cert::~X509Cert()
    {
        free_cert();
    }


    /*  Writes the certificate and private key PEM files to an ssl folder located in the default directory path.
     *  The ssl folder will be created if it doesn't exist. */
    bool X509Cert::Write()
    {
        /* Check for null data. */
        if(!pkey)
            return debug::error(FUNCTION, "Unitialized EVP_PKEY");
        if(!px509)
            return debug::error(FUNCTION, "Uninitialized certificate.");

        /* Identify the ssl subfolder. */
        std::string strFolder = config::GetDataDir() + "ssl/";

        /* Create the ssl subfolder if it doesn't exist. */
        if(!filesystem::exists(strFolder))
            filesystem::create_directory(strFolder);

        /* Identify the certificate and key paths. */
        std::string strKeyPath = strFolder + "key.pem";
        std::string strCertPath = strFolder + "cert.pem";

        /* Write the private key PEM file to the key path. */
        FILE *pFile = fopen(strKeyPath.c_str(), "wb");
        if(!pFile)
            return debug::error(FUNCTION, "X509Cert : Unable to open key file.");
        bool ret = PEM_write_PrivateKey(pFile, pkey, nullptr, nullptr, 0, nullptr, nullptr);
        fclose(pFile);
        if(!ret)
            return debug::error(FUNCTION, "X509Cert : Unable to write key file.");

        /* Write the certificate PEM file to the certificate path. */
        pFile = fopen(strCertPath.c_str(), "wb");
        if(!pFile)
            return debug::error(FUNCTION, "X509Cert : Unable to open cert file.");
        ret = PEM_write_X509(pFile, px509);
        fclose(pFile);
        if(!ret)
            return debug::error(FUNCTION, "X509Cert : Unable to write cert file.");

        return true;
    }


    /*  Modifies the SSL internal state with certificate and key information. */
    bool X509Cert::Init_SSL(SSL *ssl)
    {
        /* Check for null data. */
        if(ssl == nullptr)
            return debug::error(FUNCTION, "SSL object is null.");
        if(px509 == nullptr)
            return debug::error(FUNCTION, "certificate is null.");
        if(pkey == nullptr)
            return debug::error(FUNCTION, "private key is null.");

        /* Assign the certiicate to the SSL object. */
        if(SSL_use_certificate(ssl, px509) != 1)
            return debug::error(FUNCTION, "Failed to initialize SSL with certificate.");

        /* Assign the private key to the SSL object. */
        if(SSL_use_PrivateKey(ssl, pkey) != 1)
            return debug::error(FUNCTION, "Failed to initialize SSL with private key.");

        return true;
    }


    /*  Prints out information about the certificate with human readable format. */
    void X509Cert::Print()
    {
        char *str = X509_NAME_oneline(X509_get_subject_name(px509), 0, 0);
        debug::log(0, "subject: ", str);
        OPENSSL_free(str);
        str = X509_NAME_oneline(X509_get_issuer_name(px509), 0, 0);
        debug::log(0, "issuer: ", str);
        OPENSSL_free(str);
    }


    /* Initializes and creates a new certificate signed with a unique RSA private key. */
    bool X509Cert::init_cert()
    {
        int32_t ret = 0;

        /* Create the EVP_PKEY structure. */
        pkey = EVP_PKEY_new();
        if(pkey == nullptr)
            return debug::error(FUNCTION, "EVP_PKEY_new() failed");

        /* Create the RSA structure. */
        pRSA = RSA_new();
        if(pRSA == nullptr)
            return debug::error(FUNCTION, "RSA_new() failed");

        /* Generate the exponent and the RSA key. */
        BIGNUM *pBNE = BN_new();
        BN_set_word(pBNE, RSA_F4);
        ret = RSA_generate_key_ex(pRSA, nBits, pBNE, nullptr);
        BN_clear_free(pBNE);
        if(ret != 1)
            return debug::error(FUNCTION, "Unable to generate ", nBits, "-Bit RSA key.");

        /* Assign RSA to the EVP key. */
        if(!EVP_PKEY_assign_RSA(pkey, pRSA))
            return debug::error(FUNCTION, "Unable to assign ", nBits, "-Bit RSA key.");

        /* Create the X509 structure. */
        px509 = X509_new();
        if(px509 == nullptr)
            return debug::error(FUNCTION, "X509_new() failed");

        /* Set the serial number of certificate to '1'. Some open-source HTTP servers refuse to accept a certificate with a
           serial number of '0', which is the default. */
        ASN1_INTEGER_set(X509_get_serialNumber(px509), 1);

        /* Set the notBefore property to the current time and notAfter property to current time plus 365 days.*/
        X509_gmtime_adj(X509_get_notBefore(px509), 0);
        X509_gmtime_adj(X509_get_notAfter(px509), 31536000L);

        /*Set the public key for the certificate. */
        X509_set_pubkey(px509, pkey);

        /* Self-signed certificate, set the name of issuer to the name of the subject. */
        X509_NAME *name = X509_get_subject_name(px509);

        /* Provide country code "C", organization "O", and common name "CN" */
        X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (uint8_t *)"US", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (uint8_t *)"Nexus", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (uint8_t *)"localhost", -1, -1, 0);

        /* Set the issuer name. */
        X509_set_issuer_name(px509, name);

        /* Peform the sign. */
        X509_sign(px509, pkey, EVP_sha1());

        return true;
    }


    /*  Frees memory associated with the certificate and key. */
    void X509Cert::free_cert()
    {
        if(px509)
            X509_free(px509);

        /* No need to call RSA_free if it is assigned to EVP_PKEY, it will free it automatically. */
        if(pkey)
            EVP_PKEY_free(pkey);
    }



}
