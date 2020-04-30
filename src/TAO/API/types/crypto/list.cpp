/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/objects.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>
#include <Util/include/encoding.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* List the public keys stored in the crypto object register. */
        json::json Crypto::List(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret = json::json::array();

            /* Get the Genesis ID. */
            uint256_t hashGenesis = 0;

            /* Check for specified username / genesis. If no specific genesis or username
             * have been provided then fall back to the active sigchain. */
            if(params.find("genesis") != params.end())
                hashGenesis.SetHex(params["genesis"].get<std::string>());

            /* Check for username. */
            else if(params.find("username") != params.end())
                hashGenesis = TAO::Ledger::SignatureChain::Genesis(params["username"].get<std::string>().c_str());
            
            /* use logged in session. */
            else 
                hashGenesis = users->GetGenesis(users->GetSession(params));
           
            /* Prevent foreign data lookup in client mode */
            if(config::fClient.load() && hashGenesis != users->GetCallersGenesis(params))
                throw APIException(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");

            /* Check genesis exists */
            if(!LLD::Ledger->HasGenesis(hashGenesis))
                throw APIException(-258, "Unknown genesis");

            /* The address of the crypto object register, which is deterministic based on the genesis */
            TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);
            
            /* Read the crypto object register */
            TAO::Register::Object crypto;
            if(!LLD::Register->ReadState(hashCrypto, crypto))
                throw APIException(-259, "Could not read crypto object register");

            /* Parse the object. */
            if(!crypto.Parse())
                throw APIException(-36, "Failed to parse object register");
            
            /* Get List of key names in the crypto object */
            std::vector<std::string> vKeys = crypto.GetFieldNames();

            /* Iterate through keys */
            for(const auto& strName : vKeys)
            {
                /* JSON entry for this key */
                json::json jsonKey;

                /* Get the public key */
                uint256_t hashPublic = crypto.get<uint256_t>(strName);

                /* Populate response JSON */
                jsonKey["name"] = strName;

                /* convert the scheme type to a string */
                switch(hashPublic.GetType())
                {
                    case TAO::Ledger::SIGNATURE::FALCON:
                        jsonKey["scheme"] = "FALCON";
                        break;
                    case TAO::Ledger::SIGNATURE::BRAINPOOL:
                        jsonKey["scheme"] = "BRAINPOOL";
                        break;
                    default:
                        jsonKey["scheme"] = "";

                }
                
                /* Populate the key, base58 encoded */
                jsonKey["key"] = hashPublic == 0 ? "" : encoding::EncodeBase58(hashPublic.GetBytes());

                /* add key to the response array */
                ret.push_back(jsonKey);
            }

            return ret;
        }
    }
}
