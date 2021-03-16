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
#include <TAO/API/include/json.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/conditions.h>

#include <TAO/API/finance/types/recipient.h>
#include <TAO/API/finance/types/running_balance.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Ledger/include/chainstate.h>
#include <Legacy/wallet/wallet.h>

#include <Util/templates/datastream.h>
#include <Util/include/string.h>

using namespace TAO::Operation;

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Debit tokens from an account or token object register. */
        json::json Finance::Debit(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Authenticate the users credentials */
            if(!users->Authenticate(params))
                throw APIException(-139, "Invalid credentials");

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            Session& session = users->GetSession(params);

            /* Lock the signature chain. */
            LOCK(session.CREATE_MUTEX);

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(session.GetAccount(), strPIN, tx))
                throw APIException(-17, "Failed to create transaction");

            /* The account to debit from. */
            TAO::Register::Address hashFrom;

            /* If name is provided then use this to deduce the register address,
             * otherwise try to find the raw hex encoded address. */
            if(params.find("name") != params.end() && !params["name"].get<std::string>().empty())
                hashFrom = Names::ResolveAddress(params, params["name"].get<std::string>());
            else if(params.find("address") != params.end())
                hashFrom.SetBase58(params["address"].get<std::string>());
            else
                throw APIException(-33, "Missing name or address");

            /* Get the account object. */
            TAO::Register::Object accountFrom;
            if(!LLD::Register->ReadState(hashFrom, accountFrom))
                throw APIException(-13, "Account not found");

            /* Parse the object register. */
            if(!accountFrom.Parse())
                throw APIException(-14, "Object failed to parse");

            /* Get the object standard. */
            uint8_t nStandard = accountFrom.Standard();

            /* Check the object standard. */
            if(nStandard != TAO::Register::OBJECTS::ACCOUNT && nStandard != TAO::Register::OBJECTS::TRUST)
                throw APIException(-65, "Object is not an account");

            /* Get current balance and significant figures. */
            uint8_t nDecimals = GetDecimals(accountFrom);
            uint64_t nCurrentBalance = accountFrom.get<uint64_t>("balance");

            /* vector of recipient addresses and amounts */
            std::vector<json::json> vRecipients;

            /* Check for the existence of recipients array */
            if( params.find("recipients") != params.end() && !params["recipients"].is_null())
            {
                if(!params["recipients"].is_array())
                    throw APIException(-216, "recipients field must be an array.");

                /* Get the recipients json array */
                json::json jsonRecipients = params["recipients"];


                /* Check that there are recipient objects in the array */
                if(jsonRecipients.size() == 0)
                    throw APIException(-217, "recipients array is empty");

                /* Add them to to the vector for processing */
                for(const auto& jsonRecipient : jsonRecipients)
                    vRecipients.push_back(jsonRecipient);
            }
            else
            {
                /* Sending to only one recipient */
                vRecipients.push_back(params);
            }

            /* Check that there are not too many recipients to fit into one transaction, leaving one slot for the fee contract */
            if(vRecipients.size() > (TAO::Ledger::MAX_TRANSACTION_CONTRACTS - 1))
                throw APIException(-215, debug::safe_printstr("Max number of recipients (", (TAO::Ledger::MAX_TRANSACTION_CONTRACTS - 1), ") exceeded"));

            /* Flag that there is at least one legacy recipient */
            bool fHasLegacy = false;

            /* Current contract ID */
            uint8_t nContract = 0;

            /* Process the recipients */
            for(const auto& jsonRecipient : vRecipients)
            {
                /* Double check that there are not too many recipients to fit into one transaction */
                if(nContract >= (TAO::Ledger::MAX_TRANSACTION_CONTRACTS - 1))
                    throw APIException(-215, debug::safe_printstr("Max number of recipients (", (TAO::Ledger::MAX_TRANSACTION_CONTRACTS - 1), ") exceeded"));

                /* Check for amount parameter. */
                if(jsonRecipient.find("amount") == jsonRecipient.end())
                    throw APIException(-46, "Missing amount");

                /* Get the amount to debit. */
                uint64_t nAmount = std::stod(jsonRecipient["amount"].get<std::string>()) * pow(10, nDecimals);

                /* Check the amount is not too small once converted by the token Decimals */
                if(nAmount == 0)
                    throw APIException(-68, "Amount too small");

                /* Check they have the required funds */
                if(nAmount > nCurrentBalance)
                    throw APIException(-69, "Insufficient funds");

                /* The register address of the recipient acccount. */
                TAO::Register::Address hashTo;
                std::string strAddressTo;

                /* Check to see whether caller has provided name_to or address_to */
                if(jsonRecipient.find("name_to") != jsonRecipient.end())
                    hashTo = Names::ResolveAddress(params, jsonRecipient["name_to"].get<std::string>());
                else if(jsonRecipient.find("address_to") != jsonRecipient.end())
                {
                    strAddressTo = jsonRecipient["address_to"].get<std::string>();

                    /* Decode the base58 register address */
                    if(IsRegisterAddress(strAddressTo))
                        hashTo.SetBase58(strAddressTo);

                    /* Check that it is valid */
                    if(!hashTo.IsValid())
                        throw APIException(-165, "Invalid address_to");
                }
                else
                    throw APIException(-64, "Missing recipient account name_to / address_to");



                /* Build the transaction payload object. */
                if(hashTo.IsLegacy())
                {
                    Legacy::NexusAddress legacyAddress;
                    legacyAddress.SetString(strAddressTo);

                    /* legacy payload */
                    Legacy::Script script;
                    script.SetNexusAddress(legacyAddress);

                    tx[nContract] << (uint8_t)OP::LEGACY << hashFrom << nAmount << script;

                    /* Set the legacy flag */
                    fHasLegacy = true;

                    /* Increment the contract ID */
                    nContract++;
                }
                else
                {
                    /* flag indicating if the contract is a debit to a tokenized asset  */
                    bool fTokenizedDebit = false;

                    /* flag indicating that this is a send to self */
                    bool fSendToSelf = false;

                    /* Get the recipent account object. */
                    TAO::Register::Object recipient;

                    /* If we successfully read the register then valiate it */
                    if(LLD::Register->ReadState(hashTo, recipient, TAO::Ledger::FLAGS::LOOKUP))
                    {
                        /* Parse the object register. */
                        if(!recipient.Parse())
                            throw APIException(-14, "Object failed to parse");

                        /* Check recipient account type */
                        switch(recipient.Base())
                        {
                            case TAO::Register::OBJECTS::ACCOUNT:
                            {
                                if(recipient.get<uint256_t>("token") != accountFrom.get<uint256_t>("token"))
                                    throw APIException(-209, "Recipient account is for a different token.");

                                fSendToSelf = recipient.hashOwner == accountFrom.hashOwner;

                                break;
                            }
                            case TAO::Register::OBJECTS::NONSTANDARD :
                            {
                                fTokenizedDebit = true;

                                /* For payments to objects, they must be owned by a token */
                                if(recipient.hashOwner.GetType() != TAO::Register::Address::TOKEN)
                                    throw APIException(-211, "Recipient object has not been tokenized.");

                                break;
                            }
                            default :
                                throw APIException(-209, "Recipient is not a valid account.");
                        }
                    }

                    /* If in client mode we won't have the recipient register, so we have to loosen the checks to only look at
                       the receiving register address type, rather than check that the account/asset exists */
                    else if(config::fClient.load())
                    {
                        if(hashTo.IsObject())
                            fTokenizedDebit = true;
                        else if(!hashTo.IsAccount())
                            throw APIException(-209, "Recipient is not a valid account");
                    }

                    else
                    {
                        throw APIException(-209, "Recipient is not a valid account");
                    }


                    /* The optional payment reference */
                    uint64_t nReference = 0;
                    if(jsonRecipient.find("reference") != jsonRecipient.end())
                    {
                        /* The reference as a string */
                        std::string strReference = jsonRecipient["reference"].get<std::string>();

                        /* Check that the reference contains only numeric characters before attempting to convert it */
                        if(!IsAllDigit(strReference) || !IsUINT64(strReference))
                            throw APIException(-167, "Invalid reference");

                        /* Convert the reference to uint64 */
                        nReference = stoull(strReference);
                    }

                    /* Build the OP:DEBIT */
                    tx[nContract] << (uint8_t)OP::DEBIT << hashFrom << hashTo << nAmount << nReference;

                    /* Add expiration condition unless sending to self */
                    if(!fSendToSelf)
                        AddExpires( params, session.GetAccount()->Genesis(), tx[nContract], fTokenizedDebit);

                    /* Increment the contract ID */
                    nContract++;

                    /* Reduce the current balance by the amount for this recipient */
                    nCurrentBalance -= nAmount;
                }
            }

            /* Add the fee. If the sending account is a NXS account then we take the fee from that, otherwise we will leave it 
            to the AddFee method to take the fee from the default NXS account */
            if(accountFrom.get<uint256_t>("token") == 0)
                AddFee(tx, hashFrom);
            else
                AddFee(tx);

            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-44, "Transaction failed to build");

            /* Sign the transaction. */
            if(!tx.Sign(session.GetAccount()->Generate(tx.nSequence, strPIN)))
                throw APIException(-31, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-32, "Failed to accept");

            /* If this has a legacy transaction and not in client mode  then we need to make sure it shows in the legacy wallet */
            if(fHasLegacy && !config::fClient.load())
            {
                #ifndef NO_WALLET

                TAO::Ledger::BlockState state;
                Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(tx, state, true);

                #endif
            }
            /* Build a JSON response object. */
            ret["txid"] = tx.GetHash().ToString();

            return ret;
        }


        /* Debit tokens from any account for the given token type.  If multiple accounts exist, the selection algorithm will 
           attempt to fill up the transaction with as many DEBIT contracts as possible. */
        json::json Finance::DebitAny(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* The token type being debited, default to 0 (NXS). */
            TAO::Register::Address hashToken;

            /* Authenticate the users credentials */
            if(!users->Authenticate(params))
                throw APIException(-139, "Invalid credentials");

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            Session& session = users->GetSession(params);

            /* Lock the signature chain. */
            LOCK(session.CREATE_MUTEX);

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(session.GetAccount(), strPIN, tx))
                throw APIException(-17, "Failed to create transaction");

            /* If token name is provided then use this to deduce the register address, otherwise use the hex encoded address. */
            if(params.find("token_name") != params.end() && !params["token_name"].get<std::string>().empty())
                hashToken = Names::ResolveAddress(params, params["token_name"].get<std::string>());
            else if(params.find("token") != params.end() && IsRegisterAddress(params["token"]))
                hashToken.SetBase58(params["token"]);

            /* The significant figures of the token, used to convert incoming token amounts into token units */
            uint8_t nDecimals = TAO::Ledger::NXS_DIGITS;

            /* Validate the token address unless it is NXS */
            if(hashToken != 0)
            {
                if(hashToken.GetType() != TAO::Register::Address::TOKEN)
                    throw APIException(-212, "Invalid token");

                /* Retrieve the token */
                TAO::Register::Object token;
                if(!LLD::Register->ReadState(hashToken, token, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-125, "Token not found");

                /* Parse the object register. */
                if(!token.Parse())
                    throw APIException(-14, "Object failed to parse");

                /* Check the standard */
                if(token.Standard() != TAO::Register::OBJECTS::TOKEN)
                    throw APIException(-212, "Invalid token");

                /* Get the significant figures of the token */
                nDecimals = GetDecimals(token);
            }

            /* Vector of accounts owned by this sig chain */
            std::vector<std::pair<TAO::Register::Address, TAO::Register::Object>> vAccounts;

            /* Get all of the account objects */
            if(!ListAccounts(session.GetAccount()->Genesis(), false, true, vAccounts))
                throw APIException(-74, "No registers found");

            /* Filter the list of accounts by the requested token type.  Also filter out any with 0 balance */
            vAccounts.erase(std::remove_if(vAccounts.begin(), vAccounts.end(), 
                                            [hashToken](const std::pair<TAO::Register::Address, TAO::Register::Object>& entry)
                                            {
                                                return entry.second.get<uint256_t>("token") != hashToken ||
                                                       entry.second.get<uint64_t>("balance") == 0;
                                            }), 
                                            vAccounts.end());

            /* From this point forward we only need the account address, balance, and a sum of the X balances below it in the list,
               where X is the target number of contracts we want to create for each receiving address. */
            std::vector<RunningBalance> vRunningBalances;

            /* Add the filtered accounts to the running balances vector */
            for(const auto& account : vAccounts)
                vRunningBalances.push_back(RunningBalance(account.first, account.second.get<uint64_t>("balance")));

            /* Order the accounts by balance with smallest first*/
            std::sort(vRunningBalances.begin(), vRunningBalances.end(),
                [](const RunningBalance &a,const RunningBalance &b) {return a.nBalance < b.nBalance;});

            /* vector of recipient addresses and amounts in JSON*/
            std::vector<json::json> vJSONRecipients;

            /* Check for the existence of recipients array */
            if( params.find("recipients") != params.end() && !params["recipients"].is_null())
            {
                if(!params["recipients"].is_array())
                    throw APIException(-216, "recipients field must be an array.");

                /* Get the recipients json array */
                json::json jsonRecipients = params["recipients"];

                /* Check that there are recipient objects in the array */
                if(jsonRecipients.size() == 0)
                    throw APIException(-217, "recipients array is empty");

                /* Add them to to the vector for processing */
                for(const auto& jsonRecipient : jsonRecipients)
                    vJSONRecipients.push_back(jsonRecipient);
            }
            else
            {
                /* Sending to only one recipient */
                vJSONRecipients.push_back(params);
            }

            /* vector of recipients */
            std::vector<Recipient> vRecipients;

            /* Validate each recipient and add them to the recipient map*/
            for(const auto& jsonRecipient : vJSONRecipients)
            {
                /* The register address of the recipient acccount. */
                TAO::Register::Address hashTo;

                /* Check to see whether caller has provided name_to or address_to */
                if(jsonRecipient.find("name_to") != jsonRecipient.end())
                    hashTo = Names::ResolveAddress(params, jsonRecipient["name_to"].get<std::string>());
                else if(jsonRecipient.find("address_to") != jsonRecipient.end())
                {
                    std::string strAddressTo = jsonRecipient["address_to"].get<std::string>();

                    /* Decode the base58 register address */
                    if(IsRegisterAddress(strAddressTo))
                        hashTo.SetBase58(strAddressTo);

                    /* Check that it is valid */
                    if(!hashTo.IsValid())
                        throw APIException(-165, "Invalid address_to");
                }
                else
                    throw APIException(-64, "Missing recipient account name_to / address_to");

                /* Check for amount parameter. */
                if(jsonRecipient.find("amount") == jsonRecipient.end())
                    throw APIException(-46, "Missing amount");

                /* Get the amount to debit. */
                uint64_t nAmount = std::stod(jsonRecipient["amount"].get<std::string>()) * pow(10, nDecimals);

                /* Check the amount is not too small once converted by the token Decimals */
                if(nAmount == 0)
                    throw APIException(-68, "Amount too small");

                /* The optional payment reference */
                uint64_t nReference = 0;
                if(jsonRecipient.find("reference") != jsonRecipient.end())
                {
                    /* The reference as a string */
                    std::string strReference = jsonRecipient["reference"].get<std::string>();

                    /* Check that the reference contains only numeric characters before attempting to convert it */
                    if(!IsAllDigit(strReference) || !IsUINT64(strReference))
                        throw APIException(-167, "Invalid reference");

                    /* Convert the reference to uint64 */
                    nReference = stoull(strReference);
                }

                /* Add the recipient to the list. */
                vRecipients.push_back(Recipient(hashTo, nAmount, nReference));
            }

            /* Remove any recipient accounts from the running balances, in case of send to self */
            vRunningBalances.erase(std::remove_if(vRunningBalances.begin(), vRunningBalances.end(), 
                                    [vRecipients](const RunningBalance& balance) 
                                    {
                                        /* Search for recipients matching the running balance account */
                                        const auto& itt = std::find_if(vRecipients.begin(), vRecipients.end(), 
                                                                        [balance](const Recipient& recipient) 
                                                                        {return recipient.hashAddress == balance.hashAddress; });
                                        
                                        /* Return true if we found a match so that the account is removed */
                                        return itt != vRecipients.end();
                                    }), vRunningBalances.end());

            /* The total number of recipients */
            uint8_t nRecipients = vRecipients.size();

            /* Check that there are not too many recipients to fit into one transaction */
            if(nRecipients > (TAO::Ledger::MAX_TRANSACTION_CONTRACTS - 1))
                throw APIException(-215, debug::safe_printstr("Max number of recipients (", (TAO::Ledger::MAX_TRANSACTION_CONTRACTS - 1), ") exceeded"));

            /* The total balance remaining across all accounts */
            uint64_t nBalance = 0;

            /* Flag that there is at least one legacy recipient */
            bool fHasLegacy = false;

            /* Current contract ID */
            uint8_t nContract = 0;

            /* The number of recipients processed */
            uint8_t nProcessed = 0;

            /* The target number of contracts to use for each recipient */
            uint8_t nTarget = 0;

            /* Process the recipients */
            for(const auto& recipient : vRecipients)
            {
                /* The address to send to */
                TAO::Register::Address hashTo = recipient.hashAddress;

                /* Get the total amount to debit. */
                uint64_t nAmount = recipient.nAmount;

                /* For this iteration, remove any accounts that have been depleted */
                vRunningBalances.erase(std::remove_if(vRunningBalances.begin(), vRunningBalances.end(), 
                                       [](const RunningBalance& balance) {return balance.nBalance == 0;}), vRunningBalances.end());

                /* Calculate the optimal target number of contracts to use = contract slots left / number of recipients left */
                nTarget = std::floor(((TAO::Ledger::MAX_TRANSACTION_CONTRACTS - 1) - nContract) / (nRecipients - nProcessed)) ;

                /* Reduce the max contracts if there are less accounts than contract spaces to fill */
                nTarget = std::min(nTarget, (uint8_t)vRunningBalances.size());

                /* Get the total balance across all accounts */
                nBalance = GetTotalBalance(vRunningBalances);  

                debug::log(4, FUNCTION, "Total Balance: ", nBalance, " Recipient Amount: ", nAmount);              

                /* Initial check of required funds by checking the total balance across all accounts */
                if(nAmount > nBalance)
                    throw APIException(-69, "Insufficient funds");
              
                /* The index of the last account to be used to fulfill the amount */
                uint8_t nLast = 0;

                /* Flag indicating we found an optimal number of contracts to fulfill this recipient amount */
                bool fFound = false;
                
                /* Iterate through the running balances.  On the first pass see if there is a point where sum(nTarget) is enough to 
                   fulfill this recipient amount. If so, use all nTarget accounts, debiting the full balance from each and the 
                   remainder from the highest balance account.  If we cannot fulfill the required amount with the optimal number of 
                   contracts, we increase nTarget by 1 (include one more contract) and iterate again.  Continue this process until
                   we have found the minimum number of contracts > nTarget to fulfill the recipient. */ 
                while(!fFound)
                {
                    /* Calculate the running balances for the new target number of contracts */
                    CalcRunningBalances(vRunningBalances, nTarget); 

                    /* Start at the smallest balance and work up */
                    for(nLast=0; nLast < vRunningBalances.size() && !fFound; nLast++)
                    {
                        /* If the sum(x) balance at this point is sufficient to fulfill the amount then break out */
                        if(vRunningBalances.at(nLast).nSumBalances >= nAmount)
                        {
                            fFound = true;
                            break;
                        }
                    }

                    /* If we found a satisfactory index then break */
                    if(fFound)
                        break;

                    /* If no match was found, increment the target number of contracts to check and try again */
                    nTarget++;

                    /* Additional check that we are not attempting more contracts than we have accounts (insufficient funds) */
                    if(nTarget > vRunningBalances.size())
                        throw APIException(-69, "Insufficient funds");

                    /* Check that we are leaving at least one contract for each of the remaining recipients */
                    if((nContract + nTarget) + (nRecipients - nProcessed - 1) > (TAO::Ledger::MAX_TRANSACTION_CONTRACTS - 1) )
                        throw APIException(-306, debug::safe_printstr("Max number of contracts (", (TAO::Ledger::MAX_TRANSACTION_CONTRACTS - 1), ") exceeded.  Please try again with a smaller amount or fewer recipients"));
                }

                /* If the index of the last account is less than nTarget number of contracts then there are not enough accounts to
                   fill nTarget contracts.  Therefore we reduce the target to the number of accounts we do have. */
                if(nLast < nTarget -1)
                    nTarget = nLast +1;
                
                /* The amount remaining for this recipient */
                uint64_t nRemaining = nAmount;

                /* Index of first account to add a transaction for */
                uint8_t nStart = nLast - (nTarget -1);

                debug::log(4, FUNCTION, "Using ", (int)nTarget, " accounts from ", (int)nStart, " to ", (int)nLast); 

                /* Add the contracts to the transaction. */
                if(hashTo.IsLegacy())
                {
                    /* Set the legacy flag */
                    fHasLegacy = true;

                    /* Get the legacy address */
                    Legacy::NexusAddress legacyAddress;
                    legacyAddress.SetString(hashTo.ToString());

                    /* legacy payload */
                    Legacy::Script script;
                    script.SetNexusAddress(legacyAddress);

                    /* Add the contracts */
                    for(size_t nIndex = nStart; nIndex <= nLast; nIndex++)
                    {
                        /* The amount to add for this contract */
                        uint64_t nContractAmount = 0;
                        
                        if(vRunningBalances.at(nIndex).nBalance <= nRemaining)
                            nContractAmount = vRunningBalances.at(nIndex).nBalance;
                        else
                            nContractAmount = nRemaining;

                        /* Add the contract */
                        tx[nContract] << (uint8_t)OP::LEGACY << vRunningBalances.at(nIndex).hashAddress << nContractAmount << script;

                        /* Reduce the remaining amount to be added for this recipient */
                        nRemaining -= nContractAmount;

                        /* Update the running balance for the account added */
                        vRunningBalances.at(nIndex).nBalance -= nContractAmount;

                        /* Increment the contract ID */
                        nContract++;
                    }
                }
                else
                {
                    /* flag indicating if the contract is a debit to a tokenized asset  */
                    bool fTokenizedDebit = false;

                    /* flag indicating that this is a send to self */
                    bool fSendToSelf = false;

                    /* Get the recipient account object. */
                    TAO::Register::Object account;

                    /* If we successfully read the register then valiate it */
                    if(LLD::Register->ReadState(hashTo, account, TAO::Ledger::FLAGS::LOOKUP))
                    {
                        /* Parse the object register. */
                        if(!account.Parse())
                            throw APIException(-14, "Object failed to parse");

                        /* Check recipient account type */
                        switch(account.Base())
                        {
                            case TAO::Register::OBJECTS::ACCOUNT:
                            {
                                if(account.get<uint256_t>("token") != hashToken)
                                    throw APIException(-209, "Recipient account is for a different token.");

                                fSendToSelf = account.hashOwner == session.GetAccount()->Genesis();

                                break;
                            }
                            case TAO::Register::OBJECTS::NONSTANDARD :
                            {
                                fTokenizedDebit = true;

                                /* For payments to objects, they must be owned by a token */
                                if(account.hashOwner.GetType() != TAO::Register::Address::TOKEN)
                                    throw APIException(-211, "Recipient object has not been tokenized.");

                                break;
                            }
                            default :
                                throw APIException(-209, "Recipient is not a valid account.");
                        }
                    }
                    else
                    {
                        throw APIException(-209, "Recipient is not a valid account");
                    }

                    /* The optional payment reference */
                    uint64_t nReference = recipient.nReference;
                    
                    /* Add the contracts */
                    for(size_t nIndex = nStart; nIndex <= nLast; nIndex++)
                    {
                        /* The amount to add for this contract */
                        uint64_t nContractAmount = 0;
                        
                        if(vRunningBalances.at(nIndex).nBalance <= nRemaining)
                            nContractAmount = vRunningBalances.at(nIndex).nBalance;
                        else
                            nContractAmount = nRemaining;

                        /* Add the contract */
                        tx[nContract] << (uint8_t)OP::DEBIT << vRunningBalances.at(nIndex).hashAddress << hashTo << nContractAmount << nReference;
                        
                        /* Add expiration condition unless sending to self */
                        if(!fSendToSelf)
                            AddExpires( params, session.GetAccount()->Genesis(), tx[nContract], fTokenizedDebit);

                        /* Reduce the remaining amount to be added for this recipient */
                        nRemaining -= nContractAmount;

                        /* Update the running balance for the account added */
                        vRunningBalances.at(nIndex).nBalance -= nContractAmount;

                        /* Increment the contract ID */
                        nContract++;
                    }
                }

                /* Reduce the overall balance across all accounts by the amount for this recipient */
                nBalance -= nAmount;

                /* Increment number of recipients processed */
                nProcessed++;
            }

            debug::log(4, FUNCTION, "Final Balance: ", GetTotalBalance(vRunningBalances));

            /* Add the fee. If the sending account is a NXS account then we take the fee from that, otherwise we will leave it 
            to the AddFee method to take the fee from the default NXS account */
            AddFee(tx);

            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-44, "Transaction failed to build");

            /* Sign the transaction. */
            if(!tx.Sign(session.GetAccount()->Generate(tx.nSequence, strPIN)))
                throw APIException(-31, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
             if(!TAO::Ledger::mempool.Accept(tx))
                 throw APIException(-32, "Failed to accept");

            /* If this has a legacy transaction and not in client mode  then we need to make sure it shows in the legacy wallet */
            if(fHasLegacy && !config::fClient.load())
            {
                #ifndef NO_WALLET

                TAO::Ledger::BlockState state;
                Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(tx, state, true);

                #endif
            }
            /* Build a JSON response object. */
            ret["txid"] = tx.GetHash().ToString();

            return ret;
        }
    }
}
