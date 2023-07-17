/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/x509_cert.h>
#include <LLC/include/eckey.h>
#include <LLC/include/key_error.h>
#include <LLC/hash/SK.h>

#include <Util/include/debug.h>
#include <Util/include/filesystem.h>
#include <Util/templates/datastream.h>

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
    , strCertBundle()
    {
        init();
    }


    /** Default Destructor **/
    X509Cert::~X509Cert()
    {
        shutdown();
    }

    bool X509Cert::Read(const std::string& strCert, const std::string& strKey, const std::string& strCABundle)
    {
        //bool ret = 0;

        /* Store the CA Bundle path as this is only used for verification */
        strCertBundle = strCABundle;

        /* Read the private key PEM file from the key path. */
        FILE *pFile = fopen(strKey.c_str(), "rb");
        if(!pFile)
            return debug::error(FUNCTION, "X509Cert : Unable to open certificate key file: ", strKey);

        PEM_read_PrivateKey(pFile, &pkey, nullptr, nullptr);

        fclose(pFile);
        //if(!ret)
        //    return debug::error(FUNCTION, "X509Cert : Unable to read key file.");

        debug::log(0, "Loading external SSL certificate: ", strCert);

        /* Read the certificate PEM file from the certificate path. */
        pFile = fopen(strCert.c_str(), "rb");
        if(!pFile)
            return debug::error(FUNCTION, "X509Cert : Unable to open cert file: ", strCert);

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
            return debug::error(FUNCTION, "Uninitialized EVP_PKEY");
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


    /* Generate an RSA keypair and corresponding certificate signed with the key.  This method is useful for creating
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

        /* Perform the sign. */
        X509_sign(px509, pkey, EVP_sha1());


        return true;
    }


    /* Generate a certificate using EC signature scheme, signed with the specified private key.  This method is useful when
       creating and regenerating self-signed certificates where the private key is persistent.
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

        /* Perform the sign. */
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

        /* Add ca-bundle if configured */
        if(!strCertBundle.empty())
        {
            debug::log(0, "Loading external SSL CA certificate bundle: ", strCertBundle);

            if(!SSL_CTX_load_verify_locations(ssl_ctx, strCertBundle.c_str(), NULL)) // cafile: CA PEM certs file
                return debug::error(FUNCTION, "SSL_CTX_load_verify_locations failed to load CA bundle file: ", strCertBundle);
        }

        /* Assign the certificate to the SSL object. */
        if(SSL_CTX_use_certificate(ssl_ctx, px509) != 1)
            return debug::error(FUNCTION, "Failed to initialize SSL Context with certificate.");


        /* Assign the private key to the SSL object. */
        if(SSL_CTX_use_PrivateKey(ssl_ctx, pkey) != 1)
            return debug::error(FUNCTION, "Failed to initialize SSL Context with private key.");

        return true;
    }


    /*  Check that the private key matches the public key in the x509 certificate. */
    bool X509Cert::Verify(bool fCheckPrivate)
    {
        if(fCheckPrivate && X509_check_private_key(px509, pkey) != 1)
            return debug::error(FUNCTION, "X509_check_private_key() : Private key does not match the certificate public key.");

        if(!verify())
            return debug::error(FUNCTION, "X509 certificate is not valid.");

        return true;
    }


    /* Check the authenticity of the public key in the certificate and the private key paired with it. */
    bool X509Cert::Verify(SSL *ssl, bool fCheckPrivate)
    {
        if(fCheckPrivate && SSL_check_private_key(ssl) != 1)
            return debug::error(FUNCTION, "SSL_check_private_key() : Private key does not match the certificate public key.");

        if(!verify())
            return debug::error(FUNCTION, "X509 certificate is not valid.");

        return true;
    }


    /* Check the authenticity of the public key in the certificate and the private key paired with it. */
    bool X509Cert::Verify(SSL_CTX *ssl_ctx, bool fCheckPrivate)
    {
        if(fCheckPrivate && SSL_CTX_check_private_key(ssl_ctx) != 1)
            return debug::error(FUNCTION, "SSL_CTX_check_private_key() : Private key does not match the certificate public key.");

        if(!verify())
            return debug::error(FUNCTION, "X509 certificate is not valid.");

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
    bool X509Cert::GetPEM(std::vector<uint8_t>& vchCertificate) const
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
        vchCertificate.resize(nSize);

        /* Read the PEM data from the BIO memory stream into the return vector */
        BIO_read(bio, &vchCertificate[0], nSize);

        /* Clean up and return */
        BIO_free(bio);
        return true;
    }


    /* Loads the certificate data, which is passed to the method in base64 encoded PEM format*/
    bool X509Cert::Load(const std::vector<uint8_t>& vchCertificate)
    {
        /* Flag indicating load was successful  */
        bool fSuccess = false;

        /* IO memory stream to load with the certificate data */
        BIO *bio;

        /* instantiate the BIO memory steam */
        bio = BIO_new(BIO_s_mem());

        if(bio == NULL)
            fSuccess = debug::error("BIO_new failed..");

        /* Write the certificate bytes into the BIO */
        BIO_write(bio, &vchCertificate[0], vchCertificate.size());

        /* Write the x509 certificate data to the BIO stream in PEM format */
        if(PEM_read_bio_X509(bio, &px509, nullptr, nullptr) == 0)
            fSuccess = debug::error("PEM_read_bio_X509() failed..");

        /* Get the EVP_PKEY from the certificate */
        pkey = X509_get_pubkey(px509);

        /* PEM successfully read */
        fSuccess = true;

        /* Clean up and return */
        BIO_free(bio);

        return fSuccess;

    }


    /* Verify the data and signature of the x509 certificate */
    bool X509Cert::verify()
    {
        /* Flag indicating it is valid */
        bool fValid = false;

        /* Create certificate store containing the x509 certificate to verify */
        X509_STORE_CTX *ctx;
        ctx = X509_STORE_CTX_new();
        X509_STORE *store = X509_STORE_new();

        /* If a CA bundle has been provided then load it before verification */
        if(strCertBundle.empty())
            return true;

        debug::log(0, "Loading external SSL CA certificate bundle: ", strCertBundle);
        if(!X509_STORE_load_locations(store, strCertBundle.c_str(), NULL))
            debug::log(3, "Certificate verification failed: ", X509_verify_cert_error_string(X509_STORE_CTX_get_error(ctx)));

        X509_STORE_add_cert(store, px509);

        X509_STORE_CTX_init(ctx, store, px509, NULL);


        /* Verify the cert */
        fValid = (X509_verify_cert(ctx) == 1);

        /* Log verification failure message*/
        if(!fValid)
            debug::log(3, "Certificate verification failed: ", X509_verify_cert_error_string(X509_STORE_CTX_get_error(ctx)));

        /* Clean up and return */
        X509_STORE_free(store);
        X509_STORE_CTX_free(ctx);

        return fValid;
    }


    /* Gets the public key used to sign this certificate. */
    bool X509Cert::GetPublicKey(std::vector<uint8_t>& vchKey) const
    {

        if(EVP_PKEY_base_id(pkey) != EVP_PKEY_EC)
            return debug::error("GetPublicKey only supported for EC keys");

        /* Get EC key from the EVP key. */
        EC_KEY* ecKey = EVP_PKEY_get1_EC_KEY(pkey);

        /* Check the EC key was obtained */
        if(ecKey == nullptr)
            return debug::error(FUNCTION, "Unable to obtain EC key.");

        /* The EC key wrapper class */
        ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64, ecKey);

        /* Get the public key bytes */
        std::vector<uint8_t> vchKeyBytes = key.GetPubKey();

        /* Copy the bytes into the return vector */
        vchKey.insert(vchKey.begin(), vchKeyBytes.begin(), vchKeyBytes.end());

        return true;
    }

    /* Returns a 256-bit hash of the certificate data, with the exception of the signature. */
    uint256_t X509Cert::Hash()
    {

        /* Cert version */
        uint8_t nVersion = X509_get_version(px509);

        /* Get the certificate serial number */
        uint8_t nSerial = *X509_get_serialNumber(px509)->data;

        /* Get the subject line */
        char *str = X509_NAME_oneline(X509_get_subject_name(px509), 0, 0);
        std::string strSubject(str);
        OPENSSL_free(str);

        /* Get the issuer */
        str = X509_NAME_oneline(X509_get_issuer_name(px509), 0, 0);
        std::string strIssuer(str);
        OPENSSL_free(str);

        /* Signature scheme */
        uint8_t nSigScheme = X509_get_signature_nid(px509);

        /* Not before time */
        std::string strNotBefore((char*)X509_get_notBefore(px509)->data);

        /* Not after time */
        std::string strNotAfter((char*)X509_get_notAfter(px509)->data);

        /* Public key */
        std::vector<uint8_t> vchKey;
        GetPublicKey(vchKey);

        /* Data stream used to help serialize the certificate data ready for hashing. */
        DataStream ssData(SER_GETHASH, 1);

        /* Serialize the data */
        ssData << nVersion << nSerial << strSubject << strIssuer << strNotBefore << strNotAfter << nSigScheme << vchKey;

        /* Generate the hash */
        uint256_t hashCert = LLC::SK256(ssData.Bytes());

        return hashCert;
    }


    /* Returns the content of the Common Name (CN) field from the certificate. */
    std::string X509Cert::GetCN()
    {
        /* Temp buffer to receive the data */
        char strCN[256];

        /* Get the common name from the subject */
        X509_NAME_get_text_by_NID(X509_get_subject_name(px509), NID_commonName, strCN, 256);

        return std::string(strCN);

    }

}
