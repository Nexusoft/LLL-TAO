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

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/templates/datastream.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Debit a NXS account. */
        json::json Finance::Debit(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params);

            /* Get the session to be used for this API call */
            uint64_t nSession = users->GetSession(params);

            /* Check for credit parameter. */
            if(params.find("amount") == params.end())
                throw APIException(-25, "Missing Amount. (<amount>)");

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-25, "Invalid session ID");

            /* Check that the account is unlocked for creating transactions */
            if(!users->CanTransact())
                throw APIException(-25, "Account has not been unlocked for transactions.");

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-25, "Failed to create transaction.");

            /* Submit the transaction payload. */
            uint256_t hashTo = 0;

            /* If name_to is provided then use this to deduce the register address,
             * otherwise try to find the raw hex encoded address. */
            if(params.find("name_to") != params.end())
                hashTo = RegisterAddressFromName(params, params["name_to"].get<std::string>());
            else if(params.find("address_to") != params.end())
                hashTo.SetHex(params["address_to"].get<std::string>());
            else
                throw APIException(-22, "Missing recipient account name_to or address_to");

            /* Get the transaction id. */
            uint256_t hashFrom = 0;

            /* If name is provided then use this to deduce the register address,
             * otherwise try to find the raw hex encoded address. */
            if(params.find("name") != params.end())
                hashFrom = RegisterAddressFromName(params, params["name"].get<std::string>());
            else if(params.find("address") != params.end())
                hashFrom.SetHex(params["address"].get<std::string>());
            else
                throw APIException(-22, "Missing account name or address)");

            /* Get the account object. */
            TAO::Register::Object object;
            if(!LLD::regDB->ReadState(hashFrom, object))
                throw APIException(-24, "Account not found");

            /* Parse the object register. */
            if(!object.Parse())
                throw APIException(-24, "Object failed to parse");

            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            /* Check the object standard. */
            if( nStandard != TAO::Register::OBJECTS::ACCOUNT && nStandard != TAO::Register::OBJECTS::TRUST)
                throw APIException(-24, "Object is not an account");

            /* Check the account is a NXS account */
            if( object.get<uint256_t>("token_address") != 0)
                throw APIException(-24, "Account is not a NXS account.  Please use the tokens API for debiting non-NXS token accounts.");

            
            uint64_t nDigits = 1000000;
            uint64_t nCurrentBalance = object.get<uint64_t>("balance");;

            /* Get the amount to debit. */
            uint64_t nAmount = std::stod(params["amount"].get<std::string>()) * pow(10, nDigits);

            /* Check the amount is not too small once converted by the token digits */
            if(nAmount == 0)
                throw APIException(-25, "Amount too small");

            /* Check they have the required funds */
            if(nAmount > nCurrentBalance)
                throw APIException(-25, "Insufficient funds");

            /* Submit the payload object. */
            tx << (uint8_t)TAO::Operation::OP::DEBIT << hashFrom << hashTo << nAmount;

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
            ret["txid"] = tx.GetHash().ToString();

            return ret;
        }
    }
}
