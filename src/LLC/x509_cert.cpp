/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

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

#include <LLC/include/mbedtls/pk.h>


namespace LLC
{

    /** Default Constructor **/
    X509Cert::X509Cert(uint32_t bits)
    : pCert (nullptr)
    , nBits (bits)
    {
        //TODO: make sure we init our entropy here
    }


    /** Default Destructor **/
    X509Cert::~X509Cert()
    {
        if(pCert)
            mbedtls_x509_crt_free(pCert);
    }

    bool X509Cert::Read(const std::string& strCert, const std::string& strKey, const std::string& strCABundle)
    {
        //TODO: use ssl_client1.c to get this correct for certificate reading

        return true;
    }


    /*  Writes the certificate and private key PEM files to an ssl folder located in the default directory path.
     *  The ssl folder will be created if it doesn't exist. */
    bool X509Cert::Write()
    {
        //TODO: use cert_write.c code to get this correct

        return true;
    }


    /* Generate an RSA keypair and correspoding certificate signed with the key.  This method is useful for creating
       ad-hoc one-off self-signed certificates where the private key is ephemeral.  The certificate validity is set to 1 year . */
    bool X509Cert::GenerateRSA(const std::string& strCN, const uint64_t nValidFrom)
    {
        /* Create our public key context. */
        mbedtls_pk_context pKey;

        //TODO: use gen_key.c to get this correct
    }


    /*  Modifies the SSL internal state with certificate and key information. */
    bool X509Cert::Init_SSL(mbedtls_ssl_context *ssl)
    {
        //TODO: finish this

        return true;
    }


    /* Check the authenticity of the public key in the certificate and the private key paired with it. */
    bool X509Cert::Verify(bool fCheckPrivate)
    {
        //TODO: finish this

        return true;
    }


    /* Loads the certificate data, which is passed to the method in base64 encoded PEM format*/
    bool X509Cert::Load(const std::vector<uint8_t>& vchCertificate)
    {
        /* Flag indicating load was successful  */
        bool fSuccess = false;

        //TODO: finish this

        return fSuccess;
    }


    /* Verify the data and signature of the x509 certificate */
    bool X509Cert::verify()
    {
        /* Flag indicating it is valid */
        bool fValid = false;

        //TODO: finish this

        return fValid;
    }


    /* Gets the public key used to sign this certificate. */
    bool X509Cert::GetPublicKey(std::vector<uint8_t>& vchKey) const
    {
        //TODO: finish this

        return true;
    }

    /* Returns a 256-bit hash of the certificate data, with the exception of the signature. */
    uint256_t X509Cert::Hash()
    {
        //TODO: populate data from our x509 context

        /* Data stream used to help serialize the certificate data ready for hashing. */
        DataStream ssData(SER_GETHASH, 1);

        /* Serialize the data */
        //ssData << nVersion << nSerial << strSubject << strIssuer << strNotBefore << strNotAfter << nSigScheme << vchKey;

        /* Generate the hash */
        uint256_t hashCert = LLC::SK256(ssData.Bytes());

        return hashCert;
    }
}
