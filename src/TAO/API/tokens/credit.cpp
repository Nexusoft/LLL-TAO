/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>
#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/include/users.h>
#include <TAO/API/include/tokens.h>
#include <TAO/API/include/utils.h>

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
        json::json Tokens::Credit(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users.GetPin(params);

            /* Get the session to be used for this API call */
            uint64_t nSession = users.GetSession(params);

            /* Check for txid parameter. */
            if(params.find("txid") == params.end())
                throw APIException(-25, "Missing TXID");

            /* Check for credit parameter. */
            if(params.find("amount") == params.end())
                throw APIException(-25, "Missing Amount");

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users.GetAccount(nSession);
            if(!user)
                throw APIException(-25, "Invalid session ID");

            /* Check that the account is unlocked for creating transactions */
            if(!users.CanTransact())
                throw APIException(-25, "Account has not been unlocked for transactions");

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-25, "Failed to create transaction");

            /* Submit the transaction payload. */
            uint256_t hashTo = 0;

            /* Check for data parameter. */
            if(params.find("name_to") != params.end())
            {
                /* If name_to is provided then use this to deduce the register address */
                hashTo = RegisterAddressFromName( params, "token", params["name_to"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address_to") != params.end())
                hashTo.SetHex(params["address_to"].get<std::string>());
            else
                throw APIException(-22, "Missing to account");

            /* Get the transaction id. */
            uint512_t hashTx;
            hashTx.SetHex(params["txid"].get<std::string>());

            /* Check for data parameter. */
            uint256_t hashProof;
            if(params.find("name_proof") != params.end())
            {
                /* If name_proof is provided then use this to deduce the register address */
                hashProof = RegisterAddressFromName( params, "token", params["name_proof"].get<std::string>());
            }
            else if(params.find("proof") != params.end())
                hashProof.SetHex(params["proof"].get<std::string>());
            else
            {
                /* Read the previous transaction. */
                TAO::Ledger::Transaction txPrev;
                if(!LLD::legDB->ReadTx(hashTx, txPrev))
                    throw APIException(-23, "Previous transaction not found");

                /* Read the type from previous transaction */
                uint8_t nType;
                txPrev.ssOperation >> nType;

                /* Check type. */
                if(nType != TAO::Operation::OP::DEBIT)
                    throw APIException(-32, "Previous transaction not debit");

                /* Get the hashFrom from the previous transaction. */
                uint256_t hashFrom;
                txPrev.ssOperation >> hashFrom;

                /* Assign hash proof to hash to. */
                hashProof = hashFrom;
            }

            /* Get the credit. */
            uint64_t nAmount = std::stoull(params["amount"].get<std::string>());

            /* Submit the payload object. */
            tx << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << hashProof << hashTo << nAmount;

            /* Execute the operations layer. */
            if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE))
                throw APIException(-26, "Operations failed to execute");

            /* Sign the transaction. */
            if(!tx.Sign(users.GetKey(tx.nSequence, strPIN, nSession)))
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
