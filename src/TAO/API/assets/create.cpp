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
#include <TAO/API/include/assets.h>

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
        json::json Assets::Create(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Check for pin parameter. */
            if(params.find("pin") == params.end())
                throw APIException(-25, "Missing PIN");

            /* Check for username parameter. */
            if(params.find("session") == params.end())
                throw APIException(-25, "Missing Session ID");

            /* Check for data parameter. */
            if(params.find("data") == params.end())
                throw APIException(-25, "Missing data");

            /* Get the session. */
            uint64_t nSession = std::stoull(params["session"].get<std::string>());

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = accounts.GetAccount(nSession);
            if(!user)
                throw APIException(-25, "Invalid session ID");

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, params["pin"].get<std::string>().c_str(), tx))
                throw APIException(-25, "Failed to create transaction");

            /* Submit the transaction payload. */
            uint256_t hashRegister = 0;

            /* Check for data parameter. */
            if(params.find("name") != params.end())
            {
                /* Get the address from the name. */
                std::string strName = GetName() + ":" + params["name"].get<std::string>();

                /* Build the address from an SK256 hash of API:NAME. */
                hashRegister = LLC::SK256(std::vector<uint8_t>(strName.begin(), strName.end()));
            }
            else
                hashRegister = LLC::GetRand256();

            /* Test the payload feature. */
            DataStream ssData(SER_REGISTER, 1);
            ssData << params["data"].get<std::string>();

            /* Submit the payload object. */
            tx << (uint8_t)TAO::Operation::OP::REGISTER << hashRegister << (uint8_t)TAO::Register::OBJECT::READONLY << ssData.Bytes();

            /* Execute the operations layer. */
            if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE))
                throw APIException(-26, "Operations failed to execute");

            /* Sign the transaction. */
            if(!tx.Sign(accounts.GetKey(tx.nSequence, params["pin"].get<std::string>().c_str(), nSession)))
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
