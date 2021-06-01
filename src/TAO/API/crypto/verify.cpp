/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLC/include/eckey.h>
#include <LLC/include/flkey.h>
#include <LLC/include/x509_cert.h>

#include <TAO/API/objects/types/objects.h>

#include <TAO/API/include/json.h>
#include <TAO/API/crypto/types/crypto.h>

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>
#include <Util/include/encoding.h>
#include <Util/include/base64.h>
#include <Util/include/string.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Verifies the signature is correct for the specified public key and data. */
        encoding::json Crypto::Verify(const encoding::json& params, const bool fHelp)
        {
            /* JSON return value. */
            encoding::json ret;

            /* The key scheme */
            uint8_t nScheme = 0;

            /* Check if public key has been provided */
            if(params.find("publickey") == params.end())
                throw APIException(-265, "Missing public key");

            /* Check that scheme has been provided */
            if(params.find("scheme") == params.end())
                throw APIException(-275, "Missing scheme");


            /* Decode the public key into a vector of bytes */
            std::vector<uint8_t> vchPubKey;
            if(!encoding::DecodeBase58(params["publickey"].get<std::string>(), vchPubKey))
                throw APIException(-266, "Malformed public key.");

            /* Get the scheme string */
            std::string strScheme = ToLower(params["scheme"].get<std::string>());

            /* Convert to scheme type */
            if(strScheme == "falcon")
                nScheme = TAO::Ledger::SIGNATURE::FALCON;
            else if(strScheme == "brainpool")
                nScheme = TAO::Ledger::SIGNATURE::BRAINPOOL;
            else
                throw APIException(-262, "Invalid scheme.");


            /* Check the caller included the signature */
            if(params.find("signature") == params.end() || params["signature"].get<std::string>().empty())
                throw APIException(-274, "Missing signature");

            /* Decode the data into a vector of bytes */
            std::vector<uint8_t> vchSig;
            try
            {
                vchSig = encoding::DecodeBase64(params["signature"].get<std::string>().c_str());
            }
            catch(const std::exception& e)
            {
                throw APIException(-27, "Malformed base64 encoding");
            }

            /* Check the caller included the data */
            if(params.find("data") == params.end() || params["data"].get<std::string>().empty())
                throw APIException(-18, "Missing data");

            /* Decode the data into a vector of bytes */
            std::string strData = params["data"].get<std::string>();
            std::vector<uint8_t> vchData(strData.begin(), strData.end());

            /* flag indicating the signature is verified */
            bool fVerified = false;

            /* Switch based on signature type. */
            switch(nScheme)
            {
                /* Support for the FALCON signature scheeme. */
                case TAO::Ledger::SIGNATURE::FALCON:
                {
                    /* Create the FL Key object. */
                    LLC::FLKey key;

                    /* Set the public key and verify. */
                    key.SetPubKey(vchPubKey);
                    if(key.Verify(vchData, vchSig))
                        fVerified = true;

                    break;
                }

                /* Support for the BRAINPOOL signature scheme. */
                case TAO::Ledger::SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the public key and verify. */
                    key.SetPubKey(vchPubKey);
                    if(key.Verify(vchData, vchSig))
                        fVerified = true;

                    break;
                }
            }

            /* Set the flag in the json to return */
            ret["verified"] = fVerified;

            return ret;
        }


        /* Verifies the x509 certificate. */
        encoding::json Crypto::VerifyCertificate(const encoding::json& params, const bool fHelp)
        {
            /* JSON return value. */
            encoding::json ret;

            /* Check the caller included the certificate cert */
            if(params.find("certificate") == params.end() || params["certificate"].get<std::string>().empty())
                throw APIException(-296, "Missing certificate");

            /* Decode the certificate data into a vector of bytes */
            std::vector<uint8_t> vchCert;
            try
            {
                vchCert = encoding::DecodeBase64(params["certificate"].get<std::string>().c_str());
            }
            catch(const std::exception& e)
            {
                throw APIException(-27, "Malformed base64 encoding");
            }

            /* flag indicating the certificate is verified */
            bool fVerified = false;

            /* X509 certificate to load with the pem data and verify */
            LLC::X509Cert x509;

            /* Load the certificate data */
            if(!x509.Load(vchCert))
                throw APIException(-296, "Invalid certificate data");

            /* Verify the certificate signature*/
            if(x509.Verify(false))
            {
                /* Get the genesis hash of the certificate owner */
                uint256_t hashGenesis = 0;
                hashGenesis.SetHex(x509.GetCN());

                /* The address of the crypto object register, which is deterministic based on the genesis */
                TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

                /* Read the crypto object register */
                TAO::Register::Object crypto;
                if(!LLD::Register->ReadState(hashCrypto, crypto, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-259, "Could not read crypto object register");

                /* Parse the object. */
                if(!crypto.Parse())
                    throw APIException(-36, "Failed to parse object register");

                /* Get the cert key from their crypto register */
                uint256_t hashCert = crypto.get<uint256_t>("cert");

                /* Check that the certificate has been created */
                if(hashCert == 0)
                    throw APIException(-294, "Certificate has not yet been created for this signature chain.  Please use crypto/create/key to create the certificate first.");

                /* Compare this to a hash of the cert */
                fVerified = hashCert == x509.Hash();
            }

            /* Set the flag in the json to return */
            ret["verified"] = fVerified;

            return ret;
        }
    }
}
