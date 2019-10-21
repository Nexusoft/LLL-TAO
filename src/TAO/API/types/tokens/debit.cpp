/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/conditions.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

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

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            uint256_t nSession = users->GetSession(params);

            /* Check for credit parameter. */
            if(params.find("amount") == params.end())
                throw APIException(-46, "Missing amount.");

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Lock the signature chain. */
            LOCK(users->CREATE_MUTEX);

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(user, strPIN, tx))
                throw APIException(-17, "Failed to create transaction");

            /* Submit the transaction payload. */
            TAO::Register::Address hashTo;

            /* If name_to is provided then use this to deduce the register address,
             * otherwise try to find the raw hex encoded address. */
            if(params.find("name_to") != params.end())
                hashTo = Names::ResolveAddress(params, params["name_to"].get<std::string>());
            else if(params.find("address_to") != params.end())
                hashTo.SetBase58(params["address_to"].get<std::string>());
            else
                throw APIException(-64, "Missing recipient account name_to / address_to");

            /* Get the transaction id. */
            TAO::Register::Address hashFrom;

            /* If name is provided then use this to deduce the register address,
             * otherwise try to find the raw hex encoded address. */
            if(params.find("name") != params.end())
                hashFrom = Names::ResolveAddress(params, params["name"].get<std::string>());
            else if(params.find("address") != params.end())
                hashFrom.SetBase58(params["address"].get<std::string>());
            else
                throw APIException(-33, "Missing name / address");

            /* Get the token / account object. */
            TAO::Register::Object object;
            if(!LLD::Register->ReadState(hashFrom, object, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-122, "Token/account not found");

            /* Parse the object register. */
            if(!object.Parse())
                throw APIException(-14, "Object failed to parse");

            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            uint8_t nDecimals = 0;
            uint64_t nCurrentBalance = 0;

            /* Check the object standard. */
            if(nStandard == TAO::Register::OBJECTS::TOKEN
            || nStandard == TAO::Register::OBJECTS::ACCOUNT
            || nStandard == TAO::Register::OBJECTS::TRUST)
            {
                /* If the user requested a particular object type then check it is that type */
                std::string strType = params.find("type") != params.end() ? params["type"].get<std::string>() : "";
                if(strType == "token" && (nStandard == TAO::Register::OBJECTS::ACCOUNT || nStandard == TAO::Register::OBJECTS::TRUST))
                    throw APIException(-123, "Object is not a token");
                else if(strType == "account" && nStandard == TAO::Register::OBJECTS::TOKEN)
                    throw APIException(-65, "Object is not an account");

                nCurrentBalance = object.get<uint64_t>("balance");
                nDecimals = GetDecimals(object);
            }
            else
            {
                throw APIException(-124, "Unknown token / account.");
            }


            /* Get the recipent token / account object. */
            TAO::Register::Object recipient;
            if(!LLD::Register->ReadState(hashTo, recipient, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-209, "Recipient is not a valid account");

            /* Parse the object register. */
            if(!recipient.Parse())
                throw APIException(-14, "Object failed to parse");

            /* Check recipient account type */
            nStandard = recipient.Base();
            if(nStandard != TAO::Register::OBJECTS::ACCOUNT
            || (recipient.get<uint256_t>("token") != object.get<uint256_t>("token")))
            {
                throw APIException(-209, "Recipient is not a valid account.");
            }

            /* Get the amount to debit. */
            uint64_t nAmount = std::stod(params["amount"].get<std::string>()) * pow(10, nDecimals);

            /* Check the amount is not too small once converted by the token Decimals */
            if(nAmount == 0)
                throw APIException(-68, "Amount too small");

            /* Check they have the required funds */
            if(nAmount > nCurrentBalance)
                throw APIException(-69, "Insufficient funds");

            /* The optional payment reference */
            uint64_t nReference = 0;
            if(params.find("reference") != params.end())
                nReference = stoull(params["reference"].get<std::string>());

            /* Submit the payload object. */
            tx[0] << (uint8_t)TAO::Operation::OP::DEBIT << hashFrom << hashTo << nAmount << nReference;

            /* Add expiration condition unless sending to self */
            if(recipient.hashOwner != object.hashOwner)
                AddExpires( params, user->Genesis(), tx[0], false);

            /* Add the fee */
            AddFee(tx);

            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-30, "Operations failed to execute");

            /* Sign the transaction. */
            if(!tx.Sign(users->GetKey(tx.nSequence, strPIN, nSession)))
                throw APIException(-31, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-32, "Failed to accept");

            /* Build a JSON response object. */
            ret["txid"] = tx.GetHash().ToString();

            return ret;
        }
    }
}
