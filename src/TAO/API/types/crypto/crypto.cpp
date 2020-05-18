/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLC/hash/argon2.h>

#include <TAO/API/types/objects.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/include/create.h>

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>
#include <Util/include/encoding.h>
#include <Util/include/base58.h>
#include <Util/include/base64.h>
#include <Util/include/string.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Determines the scheme (key type) to use for constructing keys. */
        uint8_t Crypto::get_scheme(const json::json& params )
        {
            /* The scheme to return. Default to brainpool*/
            uint8_t nKeyType = TAO::Ledger::SIGNATURE::BRAINPOOL;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            uint256_t nSession = users->GetSession(params);

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Check the caller included the key name */
            if(params.find("name") == params.end() || params["name"].get<std::string>().empty())
                throw APIException(-88, "Missing name.");
            
            /* Get the requested key name */
            std::string strName = params["name"].get<std::string>();

            /* The logged in sig chain genesis hash */
            uint256_t hashGenesis = user->Genesis();


            /* Check the caller included a specific scheme */
            if(params.find("scheme") != params.end() )
            {
                std::string strScheme = ToLower(params["scheme"].get<std::string>()); 

                if(strScheme == "falcon")
                    nKeyType = TAO::Ledger::SIGNATURE::FALCON;
                else if(strScheme == "brainpool")
                    nKeyType = TAO::Ledger::SIGNATURE::BRAINPOOL;
                else
                    throw APIException(-262, "Invalid scheme");
            }
            else if(hashGenesis != 0)
            {
                /* If the caller has not provided the scheme parameter then we need to determins what it should be. */

                /* The address of the crypto object register, which is deterministic based on the genesis */
                TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);
                
                /* Read the crypto object register */
                TAO::Register::Object crypto;
                if(!LLD::Register->ReadState(hashCrypto, crypto, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-259, "Could not read crypto object register");

                /* Parse the object. */
                if(!crypto.Parse())
                    throw APIException(-36, "Failed to parse object register");

                uint256_t hashKey = 0;
                
                /* Check to see if the key name exists */
                if(crypto.CheckName(strName))
                {
                    /* Get the hashkey from the register */
                    hashKey = crypto.get<uint256_t>(strName);
                }

                /* First check to see if the name exists in the crypto object register, if so we take the type from that */
                if(hashKey != 0)
                    nKeyType = hashKey.GetType();
                else
                {
                    /* If not then we use the key type based on the node config, which is embedded in CreateTransaction */
                    TAO::Ledger::Transaction tx;
                    TAO::Ledger::CreateTransaction(user, strPIN, tx);

                    nKeyType = tx.nKeyType;
                }
            }
            else
            {
                throw APIException(-275, "Missing scheme");
            }

            return nKeyType;
        }

    }
}
