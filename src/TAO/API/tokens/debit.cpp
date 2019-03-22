/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>
#include <LLC/hash/SK.h>

#include <TAO/API/include/accounts.h>
#include <TAO/API/include/tokens.h>

#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/templates/datastream.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Create an asset or digital item. */
        json::json Tokens::Debit(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Check for pin parameter. */
            SecureString strPIN;
            bool fNeedPin = accounts.Locked();

            if( fNeedPin && params.find("pin") == params.end() )
                throw APIException(-25, "Missing PIN");
            else if( fNeedPin)
                strPIN = params["pin"].get<std::string>().c_str();
            else
                strPIN = accounts.GetActivePin();

            /* Check for session parameter. */
            uint64_t nSession = 0;
            bool fNeedSession = !accounts.LoggedIn();

            if(fNeedSession && params.find("session") == params.end())
                throw APIException(-25, "Missing Session ID");
            else if(fNeedSession)
                nSession = std::stoull(params["session"].get<std::string>());

            /* Check for credit parameter. */
            if(params.find("amount") == params.end())
                throw APIException(-25, "Missing Amount");

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = accounts.GetAccount(nSession);
            if(!user)
                throw APIException(-25, "Invalid session ID");

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-25, "Failed to create transaction");

            /* Submit the transaction payload. */
            uint256_t hashTo = 0;

            /* Check for data parameter. */
            if(params.find("name_to") != params.end())
            {
                /* Get the address from the name. */
                std::string strName = GetName() + ":" + params["name_to"].get<std::string>();

                /* Build the address from an SK256 hash of API:NAME. */
                hashTo = LLC::SK256(std::vector<uint8_t>(strName.begin(), strName.end()));
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address_to") != params.end())
                hashTo.SetHex(params["address_to"].get<std::string>());
            else
                throw APIException(-22, "Missing to recipient");


            /* Get the transaction id. */
            uint256_t hashFrom = 0;

            /* Check for data parameter. */
            if(params.find("name_from") != params.end())
            {
                /* Get the address from the name. */
                std::string strName = GetName() + ":" + params["name_from"].get<std::string>();

                /* Build the address from an SK256 hash of API:NAME. */
                hashFrom = LLC::SK256(std::vector<uint8_t>(strName.begin(), strName.end()));
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address_from") != params.end())
                hashFrom.SetHex(params["address_from"].get<std::string>());
            else
                throw APIException(-22, "Missing from recipient");

            /* Get the credit. */
            uint64_t nAmount = std::stoull(params["amount"].get<std::string>());

            /* Submit the payload object. */
            tx << (uint8_t)TAO::Operation::OP::DEBIT << hashFrom << hashTo << nAmount;

            /* Execute the operations layer. */
            if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE))
                throw APIException(-26, "Operations failed to execute");

            /* Sign the transaction. */
            if(!tx.Sign(accounts.GetKey(tx.nSequence, strPIN, nSession)))
                throw APIException(-26, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-26, "Failed to accept");

            /* Build a JSON response object. */
            ret["txid"]  = tx.GetHash().ToString();

            return ret;
        }
    }
}
