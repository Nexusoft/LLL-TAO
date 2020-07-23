/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/x509_cert.h>
#include <LLC/include/eckey.h>
#include <LLC/include/key_error.h>

#include <Util/include/debug.h>
#include <Util/include/filesystem.h>
#include <openssl/ec.h> 
#include <openssl/pem.h>
#include <openssl/x509v3.h>
#include <openssl/bn.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>

#include <cstdio>

namespace LLC
{

    /** Default Constructor **/
    X509Cert::X509Cert(uint32_t bits)
    : px509(nullptr)
    , pkey(nullptr)
    , nBits(bits)
    {
        init();
    }


    /** Default Destructor **/
    X509Cert::~X509Cert()
    {
        shutdown();
    }

    bool X509Cert::Read()
    {
        //bool ret = 0;

        /* Identify the ssl subfolder. */
        std::string strFolder = config::GetDataDir() + "ssl/";

        /* Check if the ssl subfolder exists. */
        if(!filesystem::exists(strFolder))
            return debug::error(FUNCTION, "Folder doesn't exist: ", strFolder);


        /* Identify the certificate and key paths. */
        std::string strCertPath = strFolder + "cert.pem";

        /* Read the private key PEM file from the key path. */
        FILE *pFile = fopen(strCertPath.c_str(), "rb");
        if(!pFile)
            return debug::error(FUNCTION, "X509Cert : Unable to open key file.");

        PEM_read_PrivateKey(pFile, &pkey, nullptr, nullptr);

        fclose(pFile);
        //if(!ret)
        //    return debug::error(FUNCTION, "X509Cert : Unable to read key file.");

        /* Read the certificate PEM file from the certificate path. */
        pFile = fopen(strCertPath.c_str(), "rb");
        if(!pFile)
            return debug::error(FUNCTION, "X509Cert : Unable to open cert file.");

        PEM_read_X509(pFile, &px509, nullptr, nullptr);

        fclose(pFile);
        //if(!ret)
        //    return debug::error(FUNCTION, "X509Cert : Unable to read cert file.");

        return true;
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


    /* Generate an RSA keypair and correspoding certificate signed with the key.  This method is useful for creating
       ad-hoc one-off self-signed certificates where the private key is ephemeral.  The certificate validity is set to 1 year . */
    bool X509Cert::GenerateRSA(const std::string& strCN, const uint64_t nValidFrom)
    {
        int32_t ret = 0;

        /* Create the RSA structure. */
        RSA *pRSA = RSA_new();

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
        {
            if(pRSA)
                RSA_free(pRSA);

            return debug::error(FUNCTION, "Unable to assign ", nBits, "-Bit RSA key.");
        }


        /* Set the serial number of certificate to '1'. Some open-source HTTP servers refuse to accept a certificate with a
           serial number of '0', which is the default. */
        ASN1_INTEGER_set(X509_get_serialNumber(px509), 1);

        /* Set the notBefore property to the current time and notAfter property to current time plus 365 days.*/
        X509_set_notBefore(px509, ASN1_TIME_set( nullptr, nValidFrom));
        X509_set_notAfter(px509, ASN1_TIME_set( nullptr, nValidFrom +  31536000L));
        //X509_gmtime_adj(X509_get_notBefore(px509), 0);
        //X509_gmtime_adj(X509_get_notAfter(px509), 31536000L);

        /*Set the public key for the certificate. */
        X509_set_pubkey(px509, pkey);

        /* Self-signed certificate, set the name of issuer to the name of the subject. */
        X509_NAME *name = X509_get_subject_name(px509);

        /* Provide country code "C", organization "O", and common name "CN" */
        X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (uint8_t *)"ORG", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (uint8_t *)"Nexus", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (uint8_t *)strCN.c_str(), -1, -1, 0);

        /* Set the issuer name. */
        X509_set_issuer_name(px509, name);

        /* Peform the sign. */
        X509_sign(px509, pkey, EVP_sha1());


        return true;
    }


    /* Generate a certificate using EC signature scheme, signed with the specified prigate key.  This method is useful when
       creating and regenerating self-signed certificates where the private key is persistant. 
       The certificate validity is set to 1 year. */
    bool X509Cert::GenerateEC(const uint512_t& hashSecret, const std::string& strCN, const uint64_t nValidFrom)
    {
        /* Generate CSecret from the hashSecret */
        std::vector<uint8_t> vBytes = hashSecret.GetBytes();
        LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

        /* The EC key wrapper class */
        ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

        /* Set the secret key. */
        if(!key.SetSecret(vchSecret, true))
            return debug::error(FUNCTION, "failed to set brainpool secret key");

        /* Assign EC key to the EVP key. */
        if(!EVP_PKEY_set1_EC_KEY(pkey, key.GetEC()))
            return debug::error(FUNCTION, "Unable to assign EC key.");

        /* Set the serial number of certificate to '1'. Some open-source HTTP servers refuse to accept a certificate with a
           serial number of '0', which is the default. */
        ASN1_INTEGER_set(X509_get_serialNumber(px509), 1);

        /* Set the notBefore property to the current time and notAfter property to current time plus 365 days.*/
        X509_set_notBefore(px509, ASN1_TIME_set( nullptr, nValidFrom));
        X509_set_notAfter(px509, ASN1_TIME_set( nullptr, nValidFrom +  31536000L));
        //X509_gmtime_adj(X509_get_notBefore(px509), 0);
        //X509_gmtime_adj(X509_get_notAfter(px509), 31536000L);

        /*Set the public key for the certificate. */
        X509_set_pubkey(px509, pkey);

        /* Self-signed certificate, set the name of issuer to the name of the subject. */
        X509_NAME *name = X509_get_subject_name(px509);

        /* Provide country code "C", organization "O", and common name "CN" */
        X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (uint8_t *)"ORG", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (uint8_t *)"Nexus", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (uint8_t *)strCN.c_str(), -1, -1, 0);

        /* Set the issuer name. */
        X509_set_issuer_name(px509, name);

        /* Peform the sign. */
        X509_sign(px509, pkey, EVP_sha1());


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

        /* Assign the certificate to the SSL object. */
        if(SSL_use_certificate(ssl, px509) != 1)
            return debug::error(FUNCTION, "Failed to initialize SSL with certificate.");

        /* Assign the private key to the SSL object. */
        if(SSL_use_PrivateKey(ssl, pkey) != 1)
            return debug::error(FUNCTION, "Failed to initialize SSL with private key.");

        return true;
    }

    /*  Modifies the SSL internal state with certificate and key information. */
    bool X509Cert::Init_SSL(SSL_CTX *ssl_ctx)
    {
        /* Check for null data. */
        if(ssl_ctx == nullptr)
            return debug::error(FUNCTION, "SSL object is null.");
        if(px509 == nullptr)
            return debug::error(FUNCTION, "certificate is null.");
        if(pkey == nullptr)
            return debug::error(FUNCTION, "private key is null.");

        /* Assign the certificate to the SSL object. */
        if(SSL_CTX_use_certificate(ssl_ctx, px509) != 1)
            return debug::error(FUNCTION, "Failed to initialize SSL Context with certificate.");

        /* Assign the private key to the SSL object. */
        if(SSL_CTX_use_PrivateKey(ssl_ctx, pkey) != 1)
            return debug::error(FUNCTION, "Failed to initialize SSL Context with private key.");

        return true;
    }


    /*  Check that the private key matches the public key in the x509 certificate. */
    bool X509Cert::Verify()
    {
        if(X509_check_private_key(px509, pkey) != 1)
            return debug::error(FUNCTION, "X509_check_private_key() : Private key does not match the certificate public key.");

        return true;
    }


    /* Check the authenticity of the public key in the certificate and the private key paired with it. */
    bool X509Cert::Verify(SSL *ssl)
    {
        if(SSL_check_private_key(ssl) != 1)
            return debug::error(FUNCTION, "SSL_check_private_key() : Private key does not match the certificate public key.");

        return true;
    }


    /* Check the authenticity of the public key in the certificate and the private key paired with it. */
    bool X509Cert::Verify(SSL_CTX *ssl_ctx)
    {
        if(SSL_CTX_check_private_key(ssl_ctx) != 1)
            return debug::error(FUNCTION, "SSL_CTX_check_private_key() : Private key does not match the certificate public key.");

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


    /* Initializes a new certificate and private key. */
    void X509Cert::init()
    {
        /* Create the EVP_PKEY structure. */
        pkey = EVP_PKEY_new();
        if(pkey == nullptr)
            debug::error(FUNCTION, "EVP_PKEY_new() failed");

        /* Create the X509 structure. */
        px509 = X509_new();
        if(px509 == nullptr)
            debug::error(FUNCTION, "X509_new() failed");
    }


    /*  Frees memory associated with the certificate and key. */
    void X509Cert::shutdown()
    {
        if(px509)
            X509_free(px509);

        /* No need to call RSA_free if it is assigned to EVP_PKEY, it will free it automatically. */
        if(pkey)
            EVP_PKEY_free(pkey);
    }


    /*  Print information about peer certificate. */
    void PrintCert(X509 *cert)
    {
        if(cert != nullptr)
        {
            char *str = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
            debug::log(0, "\tsubject: ", str);
            OPENSSL_free(str);
            str = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
            debug::log(0, "\tissuer: ", str);
            OPENSSL_free(str);
        }
        else
        {
            debug::log(0, "\tsubject: (NONE)");
            debug::log(0, "\tissuer: (NONE)");
        }
    }


    /*  Print information about peer certificate. */
    void PeerCertificateInfo(SSL *pSSL)
    {
        X509 *pPeerCert = SSL_get_peer_certificate(pSSL);

        debug::log(0, "Peer certificate:");

        if(pPeerCert)
        {
            int nRet = SSL_get_verify_result(pSSL);
            switch(nRet)
            {
                case X509_V_OK:
                {
                    break;
                }
                case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
                {
                    debug::error("self-signed cert: X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT");
                    break;
                }
                case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
                {
                    debug::error("self-signed cert: X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN");
                    break;
                }
                default:
                {
                    debug::error("certificate verification error: ", nRet);
                    break;
                }
            }
        }


        PrintCert(pPeerCert);

        /* Does nothing if null */
        X509_free(pPeerCert);
    }


    /* Always returns true. Callback function. */
    int always_true_callback(int ok, X509_STORE_CTX *ctx)
    {
        return 1;
    }


    /* Gets the x509 certificate binary data in base64 encoded PEM format. */
    bool X509Cert::GetPEM(std::vector<uint8_t>& vCertificate) const
    {
        /* The io memory stream to help us access the certificate data */
        BIO *bio;
        
        /* instantiate the BIO memory steam */
        bio = BIO_new(BIO_s_mem());

        if(bio == NULL) 
            return debug::error("BIO_new failed..");
        
        /* Write the x509 certificate data to the BIO stream in PEM format */
        if(PEM_write_bio_X509(bio, px509) == 0) 
        {
            BIO_free(bio);
            return debug::error("PEM_write_bio_x509() failed..");
        }

        /* Determine the buffer size needed by getting the BIO internal memory pointer */
        BUF_MEM *bptr;
        BIO_get_mem_ptr(bio, &bptr);

        /* Get the buffer size */
        int nSize = bptr->length;

        /* Set the return vector buffer size */
        vCertificate.resize(nSize);

        /* Read the PEM data from the BIO memory stream into the return vector */
        BIO_read(bio, &vCertificate[0], nSize);

        /* Clean up and return */
        BIO_free(bio);
        return true;
    }

}
