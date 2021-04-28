/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/get.h>

#include <TAO/API/include/conditions.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/templates/datastream.h>
#include <Util/include/math.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Create an asset or digital item. */
        json::json Tokens::Debit(const json::json& params, bool fHelp)
        {
            /* The sending account or token. */
            const TAO::Register::Address hashFrom = ExtractAddress(params);

            /* Get the token / account object. */
            TAO::Register::Object objectFrom;
            if(!LLD::Register->ReadObject(hashFrom, objectFrom, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-122, "Token/account not found");

            /* Get the object standard. */
            uint8_t nDecimals = 0;
            uint64_t nBalance = 0;

            /* Check the object standard. */
            const uint8_t nStandard = objectFrom.Standard();
            if(nStandard == TAO::Register::OBJECTS::TOKEN)
            {
                nBalance = objectFrom.get<uint64_t>("balance");
                nDecimals = GetDecimals(objectFrom);
            }
            else
                throw APIException(-124, "Unknown token / account.");

            /* Check for the existence of recipients array */
            std::vector<json::json> vRecipients;
            if(params.find("recipients") != params.end() && !params["recipients"].is_null())
            {
                /* Grab a reference to work on. */
                const json::json& jRecipients = params["recipients"];

                /* Check for correct JSON types. */
                if(!jRecipients.is_array())
                    throw APIException(-216, "recipients field must be an array.");

                /* Check that there are recipient objects in the array */
                if(jRecipients.size() == 0)
                    throw APIException(-217, "recipients array is empty");

                /* Add them to to the vector for processing */
                for(const auto& jRecipient : jRecipients)
                    vRecipients.push_back(jRecipient);
            }

            /* Sending to only one recipient */
            else
                vRecipients.push_back(params); //XXX: this is hacky to push the entire json here

            /* Check that there are not too many recipients to fit into one transaction */
            if(vRecipients.size() > 99)
                throw APIException(-215, "Max number of recipients (99) exceeded");

            /* Build our list of contracts. */
            std::vector<TAO::Operation::Contract> vContracts;
            for(const auto& jRecipient : vRecipients)
            {
                /* Double check that there are not too many recipients to fit into one transaction */
                if(vContracts.size() == 99)
                    throw APIException(-215, "Max number of recipients (99) exceeded");

                /* Check for amount parameter. */
                if(jRecipient.find("amount") == jRecipient.end())
                    throw APIException(-46, "Missing amount");

                /* Check the amount is not too small once converted by the token Decimals */
                const uint64_t nAmount = std::stod(jRecipient["amount"].get<std::string>()) * math::pow(10, nDecimals);
                if(nAmount == 0)
                    throw APIException(-68, "Amount too small");

                /* Check they have the required funds */
                if(nAmount > nBalance)
                    throw APIException(-69, "Insufficient funds");

                /* The register address of the recipient acccount. */
                const TAO::Register::Address hashTo = ExtractAddress(jRecipient, true); //true for sending to

                /* Get the recipent token / account object. */
                TAO::Register::Object objectTo;
                if(!LLD::Register->ReadObject(hashTo, objectTo, TAO::Ledger::FLAGS::LOOKUP))
                    throw APIException(-209, "Recipient is not a valid account");

                /* Flags to track for final adjustments in our expiration contract.  */
                bool fTokenizedDebit = false, fSendToSelf = false;

                /* Check recipient account type */
                switch(objectTo.Base())
                {
                    /* Case if sending to an account. */
                    case TAO::Register::OBJECTS::ACCOUNT:
                    {
                        /* Check for a valid token type compared to where we are debiting from. */
                        if(objectTo.get<uint256_t>("token") != objectFrom.get<uint256_t>("token"))
                            throw APIException(-209, "Recipient account is for a different token.");

                        /* Check if this is a send to self. */
                        fSendToSelf = (objectTo.hashOwner == objectFrom.hashOwner);

                        break;
                    }

                    /* Case if sending to an asset (this is a tokenized debit) */
                    case TAO::Register::OBJECTS::NONSTANDARD :
                    {
                        /* For payments to objects, they must be owned by a token */
                        if(objectTo.hashOwner.GetType() != TAO::Register::Address::TOKEN)
                            throw APIException(-211, "Recipient object has not been tokenized.");

                        /* Set the flag for this debit. */
                        fTokenizedDebit = true;

                        break;
                    }

                    default :
                        throw APIException(-209, "Recipient is not a valid account.");
                }

                /* The optional payment reference */
                uint64_t nReference = 0;
                if(jRecipient.find("reference") != jRecipient.end())
                    nReference = stoull(jRecipient["reference"].get<std::string>());

                /* Submit the payload object. */
                TAO::Operation::Contract contract;
                contract << (uint8_t)TAO::Operation::OP::DEBIT << hashFrom << hashTo << nAmount << nReference;

                /* Add expiration condition unless sending to self */
                if(!fSendToSelf)
                    AddExpires(jRecipient, users->GetSession(params).GetAccount()->Genesis(), contract, fTokenizedDebit);

                /* Reduce the current balance by the amount for this recipient */
                nBalance -= nAmount;

                /* Add this contract to our processing queue. */
                vContracts.push_back(contract);
            }

            /* Build a JSON response object. */
            json::json ret;
            ret["txid"] = BuildAndAccept(params, vContracts).ToString();

            return ret;
        }
    }
}
