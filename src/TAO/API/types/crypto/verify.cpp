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

#include <TAO/API/types/objects.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>
#include <Util/include/encoding.h>
#include <Util/include/base64.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Verifies the signature is correct for the specified public key and data. */
        json::json Crypto::Verify(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

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
            std::string strScheme = params["scheme"].get<std::string>();

            /* Convert to scheme type */
            if(strScheme == "FALCON")
                nScheme = TAO::Ledger::SIGNATURE::FALCON;
            else if(strScheme == "BRAINPOOL")
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
            std::vector<uint8_t> vchData;
            try
            {
                vchData = encoding::DecodeBase64(params["data"].get<std::string>().c_str());
            }
            catch(const std::exception& e)
            {
                throw APIException(-27, "Malformed base64 encoding");
            }

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
    }
}
