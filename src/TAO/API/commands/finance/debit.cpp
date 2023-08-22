/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

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
#include <TAO/API/types/accounts.h>
#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands.h>
#include <TAO/API/types/commands/finance.h>

//this is our binary boilerplate contracts
#include <TAO/API/types/contracts/expiring.h>

#include <TAO/API/include/conditions.h>
#include <TAO/API/include/contracts/build.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/constants.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/credentials.h>

#include <Util/templates/datastream.h>
#include <Util/include/string.h>
#include <Util/include/math.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Debit an account for NXS or any token. */
    encoding::json Finance::Debit(const encoding::json& jParams, const bool fHelp)
    {
        /* Get our genesis-id for this call. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Let's keep our working accounts in a nice tidy multimap, mapped by token-id. */
        std::map<uint256_t, Accounts> mapAccounts;

        /* Check for the existence of recipients array */
        std::vector<encoding::json> vRecipients;
        if(jParams.find("recipients") != jParams.end())
        {
            /* Grab a reference to work on. */
            const encoding::json& jRecipients = jParams["recipients"];

            /* Check for correct JSON types. */
            if(!jRecipients.is_array())
                throw Exception(-216, "recipients field must be an array.");

            /* Check that there are recipient objects in the array */
            if(jRecipients.size() == 0)
                throw Exception(-217, "recipients array is empty");

            /* We need to copy session here to get the name. */
            const bool fSession = (jParams.find("session") != jParams.end());

            /* Add them to to the vector for processing */
            for(const auto& jRecipient : jRecipients)
            {
                /* Add our session-id for now XXX: name lookup spagetti shouldn't require session to lookup. */
                //we are passing these parameters around to functions without having a clear and consistent design
                encoding::json jAdjusted = jRecipient;
                if(fSession)
                    jAdjusted["session"] = jParams["session"];

                vRecipients.push_back(jAdjusted);
            }
        }

        /* Sending to only one recipient */
        else
        {
            /* Push this to our recipients vector now. */
            vRecipients.push_back(jParams); //XXX: we want to optimize this
        }

        /* The sending account or token. */
        const TAO::Register::Address hashFrom = ExtractAddress(jParams, "from");
        if(hashFrom == TAO::API::ADDRESS_ALL)
        {
            /* Extract a token name if debit from ALL. */
            const uint256_t hashToken = ExtractToken(jParams);

            /* Get the token / account object. */
            if(hashToken != TOKEN::NXS)
            {
                /* Check for correct token type if not for NXS. */
                TAO::Register::Object objToken;
                if(!LLD::Register->ReadObject(hashToken, objToken))
                    throw Exception(-48, "Token not found");
            }

            /* Let's now push our account to vector. */
            std::vector<TAO::Register::Address> vAccounts;
            ListAccounts(hashGenesis, vAccounts, false, false); //XXX: we need to be able to pass in FLAGS::BLOCK here

            /* Iterate through our accounts and add to our map. */
            for(const auto& hashRegister : vAccounts)
            {
                /* Get the token / account object. */
                TAO::Register::Object objFrom;
                if(!LLD::Register->ReadObject(hashRegister, objFrom, TAO::Ledger::FLAGS::MEMPOOL))
                     continue;

                /* Check for a valid token, otherwise skip it. */
                if(objFrom.get<uint256_t>("token") != hashToken)
                    continue;

                /* Ensure we have balance, since any is for DEBIT. */
                if(objFrom.get<uint64_t>("balance") == 0)
                    continue;

                /* Check our standard fuction that will filter out balances. */
                if(!CheckStandard(jParams, objFrom))
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
            if(vRecipients.size() < 1)
                throw Exception(-55, "Must have at least one recipient to debit from any");

            /* Loop through our recipients to get the tokens that we are sending to. */
            for(const auto& jRecipient : vRecipients)
            {
                /* Handle for legacy output. */
                Legacy::NexusAddress addrLegacy;
                if(ExtractLegacy(jRecipient, addrLegacy, "to"))
                {
                    /* Add our NXS token accounts here. */
                    if(!mapAccounts.count(TOKEN::NXS))
                        mapAccounts[TOKEN::NXS] = Accounts(TAO::Ledger::NXS_DIGITS);

                    continue;
                }

                /* The register address of the recipient acccount. */
                const TAO::Register::Address hashTo =
                    ExtractAddress(jRecipient, "to"); //we use suffix 'to' here

                /* Get the token / account object. */
                TAO::Register::Object objTo;
                if(!LLD::Register->ReadObject(hashTo, objTo))
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
                TAO::Register::Object objFrom;
                if(!LLD::Register->ReadObject(hashRegister, objFrom, TAO::Ledger::FLAGS::MEMPOOL))
                    continue;

                /* Ensure we have balance, since any is for DEBIT. */
                if(objFrom.get<uint64_t>("balance") == 0)
                    continue;

                 /* Check our standard fuction that will filter out balances. */
                 if(!CheckStandard(jParams, objFrom))
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
            if(!LLD::Register->ReadObject(hashFrom, objFrom))
                throw Exception(-13, "Object not found");

            /* Ensure we have balance, since any is for DEBIT. */
            if(objFrom.get<uint64_t>("balance") == 0)
                throw Exception(-69, "Insufficient funds");

            /* Now lets check our expected types match. */
            if(!CheckStandard(jParams, objFrom))
                throw Exception(-49, "Unsupported type for name/address");

            /* Extract a token name from our from parameter. */
            const uint256_t hashToken = objFrom.get<uint256_t>("token");
            if(!mapAccounts.count(hashToken))
                mapAccounts[hashToken] = Accounts(GetDecimals(objFrom));

            /* Add our new value to our map now. */
            mapAccounts[hashToken].Insert(hashFrom, objFrom.get<uint64_t>("balance"));
        }

        /* Build our list of contracts. */
        std::vector<TAO::Operation::Contract> vContracts;
        for(const auto& jRecipient : vRecipients)
        {
            /* Check for amount parameter. */
            if(jRecipient.find("amount") == jRecipient.end())
                throw Exception(-46, "Missing amount");

            /* Check for a legacy output here. */
            Legacy::NexusAddress addrLegacy;
            if(ExtractLegacy(jRecipient, addrLegacy, "to"))
            {
                /* Check that we have an available account to debit. */
                if(!mapAccounts.count(0))
                    throw Exception(-51, "No available accounts for recipient");

                /* Grab a reference of our token struct. */
                Accounts& tAccounts = mapAccounts[0];

                /* Loop until we have depleted the amount for this recipient. */
                uint64_t nAmount = ExtractAmount(jRecipient, tAccounts.GetFigures());
                while(nAmount > 0)
                {
                    /* Grab the balance of our current account. */
                    uint64_t nDebit = nAmount;

                    /* Check they have the required funds */
                    if(nDebit > tAccounts.GetBalance())
                    {
                        /* Check if we have another account on hand. */
                        if(!tAccounts.HasNext())
                            throw Exception(-69, "Insufficient funds");

                        /* Adjust our debit amount. */
                        nDebit = tAccounts.GetBalance();
                    }

                    /* Build our script payload */
                    Legacy::Script script;
                    script.SetNexusAddress(addrLegacy);

                    /* Build our legacy contract. */
                    TAO::Operation::Contract tContract;
                    tContract << uint8_t(TAO::Operation::OP::LEGACY) << tAccounts.GetAddress() << nDebit << script;

                    /* Add this contract to our processing queue. */
                    vContracts.push_back(tContract);

                    /* Reduce the current balances. */
                    tAccounts -= nDebit;
                    nAmount   -= nDebit;

                    /* Iterate to next if needed. */
                    if(nAmount > 0)
                        tAccounts++; //iterate to next account
                }

                break; //we break if we detected legacy address so we don't process as tritium
            }

            /* Extract as a register address if legacy checks failed. */
            const TAO::Register::Address hashTo =
                ExtractAddress(jRecipient, "to"); //we use suffix 'to' here

            /* Get the recipent token / account object. */
            TAO::Register::Object objTo;
            if(!LLD::Register->ReadObject(hashTo, objTo, TAO::Ledger::FLAGS::LOOKUP))
                throw Exception(-209, "Recipient account doesn't exist");

            /* Check that we are not debiting to tokenized asset. */
            uint256_t hashToken = ~uint256_t(0); //default value should fail
            if(objTo.Base() == TAO::Register::OBJECTS::NONSTANDARD)
            {
                /* Check that we don't have multiple accounts that would come from any/all. */
                if(mapAccounts.size() != 1)
                    throw Exception(-65, "Cannot debit/any to tokenized asset");

                /* Get the token-id we will work with. */
                hashToken = mapAccounts.begin()->first;
            }
            else //otherwise check by account token
                hashToken = objTo.get<uint256_t>("token");

            /* Check that we have an available account to debit. */
            if(!mapAccounts.count(hashToken))
                throw Exception(-51, "No available token accounts for recipient");

            /* Grab a reference of our token struct. */
            Accounts& tAccounts = mapAccounts[hashToken];

            /* Loop until we have depleted the amount for this recipient. */
            uint64_t nAmount = ExtractAmount(jRecipient, tAccounts.GetFigures());
            while(nAmount > 0)
            {
                /* Grab the balance of our current account. */
                uint64_t nDebit = nAmount;

                /* Check they have the required funds */
                if(nDebit > tAccounts.GetBalance())
                {
                    /* Check if we have another account on hand. */
                    if(!tAccounts.HasNext())
                        throw Exception(-69, "Insufficient funds");

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
                            throw Exception(-211, "Recipient object has not been tokenized.");

                        /* Set the flag for this debit. */
                        fTokenizedDebit = true;

                        break;
                    }

                    default:
                        throw Exception(-209, "Recipient is not a valid account.");
                }

                /* The optional payment reference */
                const uint64_t nReference =
                    ExtractInteger<uint64_t>(jRecipient, "reference", 0); //0: default reference of 0

                /* Submit the payload object. */
                TAO::Operation::Contract tContract;
                tContract << uint8_t(TAO::Operation::OP::DEBIT) << tAccounts.GetAddress() << hashTo << nDebit << nReference;

                /* Add expiration condition unless sending to self */
                if(!fSendToSelf)
                    AddExpires(jParams, hashGenesis, tContract, fTokenizedDebit);

                /* Add this contract to our processing queue. */
                vContracts.push_back(tContract);

                /* Reduce the current balances. */
                tAccounts -= nDebit;
                nAmount   -= nDebit;

                /* Iterate to next if needed. */
                if(nAmount > 0)
                    tAccounts++; //iterate to next account
            }
        }

        /* Build response JSON boilerplate. */
        return BuildResponse(jParams, vContracts);
    }
}
