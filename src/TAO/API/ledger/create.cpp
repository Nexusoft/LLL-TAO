/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/ledger.h>
#include <TAO/API/include/accounts.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/state.h>

#include <LLC/include/eckey.h>

#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** List of accounts in API. **/
        Ledger ledger;


        /* Standard initialization function. */
        void Ledger::Initialize()
        {
            mapFunctions["createblock"] = Function(std::bind(&Ledger::CreateBlock, this, std::placeholders::_1, std::placeholders::_2));
        }


        /* Creates a register with given RAW state. */
        json::json Ledger::CreateBlock(const json::json& params, bool fHelp)
        {
            /* Check for username parameter. */
            if(params.find("session") == params.end())
                throw APIException(-25, "Missing Session ID");

            /* Check for pin parameter. */
            if(params.find("pin") == params.end())
                throw APIException(-25, "Missing PIN");

            /* Get the session. */
            uint64_t nSession = std::stoull(params["session"].get<std::string>());

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = accounts.GetAccount(nSession);
            if(!user)
                throw APIException(-25, "Invalid session ID");

            /* Create the block object. */
            TAO::Ledger::TritiumBlock block;
            if(!TAO::Ledger::CreateBlock(user, params["pin"].get<std::string>().c_str(), 2, block))
                throw APIException(-26, "Failed to create block");

            /* Get the secret from new key. */
            std::vector<uint8_t> vBytes = accounts.GetKey(block.producer.nSequence, params["pin"].get<std::string>().c_str(), nSession).GetBytes();
            LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

            /* Generate the EC Key. */
            #if defined USE_FALCON
            LLC::FLKey key;
            #else
            LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);
            #endif
            if(!key.SetSecret(vchSecret, true))
                throw APIException(-26, "Failed to set secret key");

            /* Generate new block signature. */
            block.GenerateSignature(key);

            /* Verify the block object. */
            if(!block.Check())
                throw APIException(-26, "Block is invalid");

            /* Create the state object. */
            if(!block.Accept())
                throw APIException(-26, "Block failed accept");

            json::json ret;
            ret["block"] = block.GetHash().ToString();

            return ret;
        }
    }
}
