/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/wallet/wallet.h>

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/constants.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/get.h>
#include <TAO/API/include/list.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/finance/types/finance.h>

#include <TAO/API/include/conditions.h>

#include <TAO/API/finance/types/accounts.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/constants.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/templates/datastream.h>
#include <Util/include/string.h>
#include <Util/include/math.h>

/* Global TAO namespace. */
namespace TAO::API
{

    /* Debit an account for NXS or any token. */
    json::json Finance::Debit(const json::json& jParams, const bool fHelp)
    {
        /* Get our genesis-id for this call. */
        const uint256_t hashGenesis =
            Commands::Get<Users>()->GetSession(jParams).GetAccount()->Genesis();

        /* Let's keep our working accounts in a nice tidy multimap, mapped by token-id. */
        std::map<uint256_t, Accounts> mapAccounts;

        /* Check for the existence of recipients array */
        std::vector<json::json> vRecipients;
        if(jParams.find("recipients") != jParams.end())
        {
            /* Grab a reference to work on. */
            const json::json& jRecipients = jParams["recipients"];

            /* Check for correct JSON types. */
            if(!jRecipients.is_array())
                throw APIException(-216, "recipients field must be an array.");

            /* Check that there are recipient objects in the array */
            if(jRecipients.size() == 0)
                throw APIException(-217, "recipients array is empty");

            /* We need to copy session here to get the name. */
            const bool fSession = (jParams.find("session") != jParams.end());

            /* Add them to to the vector for processing */
            for(const auto& jRecipient : jRecipients)
            {
                /* Add our session-id for now XXX: name lookup spagetti shouldn't require session to lookup. */
                //we are passing these parameters around to functions without having a clear and consistent design
                json::json jAdjusted = jRecipient;
                if(fSession)
                    jAdjusted["session"] = jParams["session"];

                vRecipients.push_back(jAdjusted);
            }
        }

        /* Sending to only one recipient */
        else
        {
            /* Check for amount parameter. */
            if(jParams.find("amount") == jParams.end())
                throw APIException(-46, "Missing amount");

            /* Build a recipeint object from parameters. */
            json::json jRecipient;
            jRecipient["amount"] = jParams["amount"];

            /* Check for a name_to parameter. */
            if(jParams.find("name_to") != jParams.end())
                jRecipient["name_to"] = jParams["name_to"];

            /* Check for a address_to parameter. */
            if(jParams.find("address_to") != jParams.end())
                jRecipient["address_to"] = jParams["address_to"];

            /* Check for a reference parameter. */
            if(jParams.find("reference") != jParams.end())
                jRecipient["reference"] = jParams["reference"];

            /* Check for a reference parameter. */
            if(jParams.find("expires") != jParams.end())
                jRecipient["expires"] = jParams["expires"];

            /* We need to copy session here to get the name. */
            if(jParams.find("session") != jParams.end())
                jRecipient["session"] = jParams["session"];

            /* Push this to our recipients vector now. */
            vRecipients.push_back(jRecipient);
        }

        /* The sending account or token. */
        const TAO::Register::Address hashFrom = ExtractAddress(jParams);
        if(hashFrom == TAO::API::ADDRESS_ALL)
        {
            /* Extract a token name if debit from ALL. */
            const uint256_t hashToken = ExtractToken(jParams);

            /* Get the token / account object. */
            TAO::Register::Object objToken;
            if(!LLD::Register->ReadObject(hashToken, objToken, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-48, "Token not found");

            /* Let's now push our account to vector. */
            std::vector<TAO::Register::Address> vAccounts;
            ListAccounts(hashGenesis, vAccounts, false, false); //XXX: we need to be able to pass in FLAGS::BLOCK here

            /* Iterate through our accounts and add to our map. */
            for(const auto& hashRegister : vAccounts)
            {
                /* Get the token / account object. */
                TAO::Register::Object objFrom; //XXX: we may not want to use MEMPOOL flag for debits
                if(!LLD::Register->ReadObject(hashRegister, objFrom, TAO::Ledger::FLAGS::MEMPOOL))
                     continue;

                /* Check for a valid token, otherwise skip it. */
                if(objFrom.get<uint256_t>("token") != hashToken)
                    continue;

                /* Check for an available balance, otherwise skip it. */
                if(objFrom.get<uint64_t>("balance") == 0)
                    continue;

                /* Initialize our map if required. */
                if(!mapAccounts.count(hashToken))
                    mapAccounts[hashToken] = Accounts(GetDecimals(objFrom));

                /* Add our new value to our map now. */
                mapAccounts[hashToken].Insert(hashRegister, objFrom.get<uint64_t>("balance"));
            }
        }

        /* Handle a sending from ANY which allows a mix-and-match of different token types. */
        else if(hashFrom == TAO::API::ADDRESS_ANY)
        {
            /* To send to ANY we need to have more than one recipient. */
            if(vRecipients.size() <= 1)
                throw APIException(-55, "Must have at least two recipients to debit from any");

            /* Loop through our recipients to get the tokens that we are sending to. */
            for(const auto& jRecipient : vRecipients)
            {
                /* The register address of the recipient acccount. */
                const TAO::Register::Address hashTo =
                    ExtractAddress(jRecipient, "to"); //we use suffix 'to' here

                /* Get the token / account object. */
                TAO::Register::Object objTo;
                if(!LLD::Register->ReadObject(hashTo, objTo, TAO::Ledger::FLAGS::MEMPOOL))
                     continue;

                 /* Initialize our map if required. */
                 const uint256_t hashToken = objTo.get<uint256_t>("token");
                 if(!mapAccounts.count(hashToken))
                     mapAccounts[hashToken] = Accounts(GetDecimals(objTo));
            }

            /* Let's now push our account to vector. */
            std::vector<TAO::Register::Address> vAccounts;
            ListAccounts(hashGenesis, vAccounts, false, false);

            /* Iterate through our accounts and add to our map. */
            for(const auto& hashRegister : vAccounts)
            {
                /* Get the token / account object. */
                TAO::Register::Object objFrom; //XXX: we may not want to use MEMPOOL flag for debits
                if(!LLD::Register->ReadObject(hashRegister, objFrom, TAO::Ledger::FLAGS::MEMPOOL))
                     continue;

                /* Check for an available balance, otherwise skip it. */
                if(objFrom.get<uint64_t>("balance") == 0)
                    continue;

                /* Check for a valid token, otherwise skip it. */
                const uint256_t hashToken = objFrom.get<uint256_t>("token");
                if(!mapAccounts.count(hashToken))
                    continue;

                /* Add our new value to our map now. */
                mapAccounts[hashToken].Insert(hashRegister, objFrom.get<uint64_t>("balance"));
            }
        }

        /* Regular send from name or address, use same map for managing accounts, but only have a singl entry. */
        else
        {
            /* Get the token / account object. */
            TAO::Register::Object objFrom;
            if(!LLD::Register->ReadObject(hashFrom, objFrom, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-13, "Object not found");

            /* Now lets check our expected types match. */
            if(!CheckType(jParams, objFrom))
                throw APIException(-49, "Unexpected type for name / address");

            /* Extract a token name from our from parameter. */
            const uint256_t hashToken = objFrom.get<uint256_t>("token");
            if(!mapAccounts.count(hashToken))
                mapAccounts[hashToken] = Accounts(GetDecimals(objFrom));

            /* Add our new value to our map now. */
            mapAccounts[hashToken].Insert(hashFrom, objFrom.get<uint64_t>("balance"));
        }

        /* Check that there are not too many recipients to fit into one transaction */
        if(vRecipients.size() > 99)
            throw APIException(-215, "Max number of recipients (99) exceeded");

        /* Build our list of contracts. */
        std::vector<TAO::Operation::Contract> vContracts;
        for(const auto& jRecipient : vRecipients)
        {
            /* Double check that there are not too many recipients to fit into one transaction */
            if(vContracts.size() >= 99)
                throw APIException(-215, "Max number of recipients (99) exceeded");

            /* Check for amount parameter. */
            if(jRecipient.find("amount") == jRecipient.end())
                throw APIException(-46, "Missing amount");

            /* The register address of the recipient acccount. */
            const TAO::Register::Address hashTo =
                ExtractAddress(jRecipient, "to"); //we use suffix 'to' here

            /* Check for a legacy output here. */
            if(hashTo.IsLegacy())
            {
                /* Check that we have an available account to debit. */
                if(!mapAccounts.count(0))
                    throw APIException(-51, "No available accounts for recipient");

                /* Grab a reference of our token struct. */
                Accounts& tAccounts = mapAccounts[0];

                /* Check the amount is not too small once converted by the token Decimals */
                uint64_t nAmount = std::stod(jRecipient["amount"].get<std::string>()) * tAccounts.GetFigures();
                if(nAmount == 0)
                    throw APIException(-68, "Amount too small");

                /* Loop until we have depleted the amount for this recipient. */
                while(nAmount > 0)
                {
                    /* Grab the balance of our current account. */
                    uint64_t nDebit = nAmount;

                    /* Check they have the required funds */
                    if(nDebit > tAccounts.GetBalance())
                    {
                        /* Check if we have another account on hand. */
                        if(!tAccounts.HasNext())
                            throw APIException(-69, "Insufficient funds");

                        /* Adjust our debit amount. */
                        nDebit = tAccounts.GetBalance();
                    }

                    /* Build our legacy address. */
                    Legacy::NexusAddress addrLegacy = Legacy::NexusAddress(hashTo);

                    /* Build our script payload */
                    Legacy::Script script;
                    script.SetNexusAddress(addrLegacy);

                    /* Build our legacy contract. */
                    TAO::Operation::Contract contract;
                    contract << uint8_t(TAO::Operation::OP::LEGACY) << tAccounts.GetAddress() << nDebit << script;

                    /* Add this contract to our processing queue. */
                    vContracts.push_back(contract);

                    /* Reduce the current balances. */
                    tAccounts -= nDebit;
                    nAmount   -= nDebit;

                    /* Iterate to next if needed. */
                    if(nAmount > 0)
                        tAccounts++; //iterate to next account
                }


                //XXX: maybe we find a way to have these two share their logic?
            }
            else
            {
                /* Get the recipent token / account object. */
                TAO::Register::Object objTo;
                if(!LLD::Register->ReadObject(hashTo, objTo, TAO::Ledger::FLAGS::LOOKUP))
                    throw APIException(-209, "Recipient is not a valid account");

                /* Track our token we will be using. */
                const uint256_t hashToken = objTo.get<uint256_t>("token");

                /* Check that we have an available account to debit. */
                if(!mapAccounts.count(hashToken))
                    throw APIException(-51, "No available token accounts for recipient");

                /* Grab a reference of our token struct. */
                Accounts& tAccounts = mapAccounts[hashToken];

                /* Check the amount is not too small once converted by the token Decimals */
                uint64_t nAmount = std::stod(jRecipient["amount"].get<std::string>()) * tAccounts.GetFigures();
                if(nAmount == 0)
                    throw APIException(-68, "Amount too small");

                /* Loop until we have depleted the amount for this recipient. */
                while(nAmount > 0)
                {
                    /* Grab the balance of our current account. */
                    uint64_t nDebit = nAmount;

                    /* Check they have the required funds */
                    if(nDebit > tAccounts.GetBalance())
                    {
                        /* Check if we have another account on hand. */
                        if(!tAccounts.HasNext())
                            throw APIException(-69, "Insufficient funds");

                        /* Adjust our debit amount. */
                        nDebit = tAccounts.GetBalance();
                    }

                    /* Flags to track for final adjustments in our expiration contract.  */
                    bool fTokenizedDebit = false, fSendToSelf = false;

                    /* Check recipient account type */
                    switch(objTo.Base())
                    {
                        /* Case if sending to an account. */
                        case TAO::Register::OBJECTS::ACCOUNT:
                        {
                            /* Check if this is a send to self. */
                            fSendToSelf = (objTo.hashOwner == hashGenesis);

                            break;
                        }

                        /* Case if sending to an asset (this is a tokenized debit) */
                        case TAO::Register::OBJECTS::NONSTANDARD:
                        {
                            /* For payments to objects, they must be owned by a token */
                            if(objTo.hashOwner.GetType() != TAO::Register::Address::TOKEN)
                                throw APIException(-211, "Recipient object has not been tokenized.");

                            /* Set the flag for this debit. */
                            fTokenizedDebit = true;

                            break;
                        }

                        default:
                            throw APIException(-209, "Recipient is not a valid account.");
                    }

                    /* The optional payment reference */
                    uint64_t nReference = 0;
                    if(jRecipient.find("reference") != jRecipient.end())
                        nReference = stoull(jRecipient["reference"].get<std::string>());

                    /* Submit the payload object. */
                    TAO::Operation::Contract contract;
                    contract << uint8_t(TAO::Operation::OP::DEBIT) << tAccounts.GetAddress() << hashTo << nDebit << nReference;

                    /* Add expiration condition unless sending to self */
                    if(!fSendToSelf)
                        AddExpires(jRecipient, hashGenesis, contract, fTokenizedDebit);

                    /* Add this contract to our processing queue. */
                    vContracts.push_back(contract);

                    /* Reduce the current balances. */
                    tAccounts -= nDebit;
                    nAmount   -= nDebit;

                    /* Iterate to next if needed. */
                    if(nAmount > 0)
                        tAccounts++; //iterate to next account
                }
            }
        }

        /* Build response JSON boilerplate. */
        return BuildResponse(jParams, vContracts);
    }
}
