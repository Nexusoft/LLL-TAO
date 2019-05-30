/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <LLC/include/random.h>
#include <LLD/include/global.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Transfer an asset or digital item. */
        json::json Assets::Transfer(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params);

            /* Get the session to be used for this API call */
            uint64_t nSession = users->GetSession(params);

            /* Watch for destination genesis. */
            uint256_t hashTo = 0;
            if(params.find("destination") != params.end())
                hashTo.SetHex(params["destination"].get<std::string>());
            else if(params.find("username") != params.end())
                hashTo = TAO::Ledger::SignatureChain::Genesis(params["username"].get<std::string>().c_str());
            else
                throw APIException(-25, "Missing Destination");

            /* Check that the destination exists. */
            if(!LLD::legDB->HasGenesis(hashTo))
                throw APIException(-25, "Destination doesn't exist");

            /* Get the register address. */
            uint256_t hashRegister = 0;

            /* Check whether the caller has provided the asset name parameter. */
            if(params.find("name") != params.end())
                /* If name is provided then use this to deduce the register address */
                hashRegister = RegisterAddressFromName(params, params["name"].get<std::string>());
            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetHex(params["address"]);
            /* Fail if no required parameters supplied. */
            else
                throw APIException(-23, "Missing memory address");

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-25, "Invalid session ID");

            /* Check that the account is unlocked for creating transactions */
            if(!users->CanTransact())
                throw APIException(-25, "Account has not been unlocked for transactions");

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-25, "Failed to create transaction");

            /* Submit the payload object. */
            tx << (uint8_t)TAO::Operation::OP::TRANSFER << hashRegister << hashTo;

            /* Execute the operations layer. */
            if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE))
                throw APIException(-26, "Operations failed to execute");

            /* Sign the transaction. */
            if(!tx.Sign(users->GetKey(tx.nSequence, strPIN, nSession)))
                throw APIException(-26, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-26, "Failed to accept");

            /* Build a JSON response object. */
            ret["txid"]  = tx.GetHash().ToString();
            ret["address"] = hashRegister.ToString();

            return ret;
        }
    }
}
