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
#include <TAO/API/include/global.h>
#include <TAO/API/include/get.h>
#include <TAO/API/include/list.h>

#include <TAO/API/include/conditions.h>

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
    /** Token struct to manage token related data. **/
    struct TokenAccounts
    {
        /* Vector of addresses and balances to check against. */
        std::vector<std::pair<uint256_t, uint64_t>> vAddresses;


        /* Iterator for our vector to get current account. */
        uint32_t nIterator;


        /* The decimals for this specific token. */
        uint8_t nDecimals;


        /* Default constructor. */
        TokenAccounts()
        : vAddresses ( )
        , nIterator  (0)
        , nDecimals  (0)
        {
        }


        /* Copy constructor. */
        TokenAccounts(const TokenAccounts& in)
        : vAddresses (in.vAddresses)
        , nIterator  (in.nIterator)
        , nDecimals  (in.nDecimals)
        {
        }


        /* Move constructor. */
        TokenAccounts(TokenAccounts&& in)
        : vAddresses (std::move(in.vAddresses))
        , nIterator  (std::move(in.nIterator))
        , nDecimals  (std::move(in.nDecimals))
        {
        }


        /** Copy assignment. **/
        TokenAccounts& operator=(const TokenAccounts& in)
        {
            vAddresses = in.vAddresses;
            nIterator  = in.nIterator;
            nDecimals  = in.nDecimals;

            return *this;
        }


        /** Move assignment. **/
        TokenAccounts& operator=(TokenAccounts&& in)
        {
            vAddresses = std::move(in.vAddresses);
            nIterator  = std::move(in.nIterator);
            nDecimals  = std::move(in.nDecimals);

            return *this;
        }


        /** Default destructor. **/
        ~TokenAccounts()
        {
        }


        /** Constructor for decimals. **/
        TokenAccounts(const uint8_t nDecimalsIn)
        : vAddresses ( )
        , nIterator  (0)
        , nDecimals  (nDecimalsIn)
        {
        }


        /** Operator-= overload
         *
         *  Adjusts the balance for the currently loaded account.
         *
         *  @param[in] nBalance The balance to deduct by.
         *
         **/
        TokenAccounts& operator-=(const uint32_t nBalance)
        {
            /* If we have exhausted our list of address, otherwise throw here. */
            if(nIterator >= vAddresses.size())
                throw APIException(-52, "No more available accounts for debit");

            /* Check that we don't underflow here. */
            if(vAddresses[nIterator].second < nBalance)
                throw APIException(-69, "Insufficient funds");

            /* Adjust the balance for designated account. */
            vAddresses[nIterator].second -= nBalance;

            return *this;
        }


        /** Operator++ overload
         *
         *  Adjusts the iterator for the currently loaded account.
         *
         **/
        TokenAccounts& operator++(int)
        {
            /* If we have exhausted our list of address, otherwise throw here. */
            if(++nIterator >= vAddresses.size())
                throw APIException(-52, "No more available accounts for debit");

            return *this;
        }


        /** HasNext
         *
         *  Checks if we have another account to iterate to.
         *
         **/
        bool HasNext() const
        {
            return (nIterator < (vAddresses.size() - 1));
        }


        /** GetBalance
         *
         *  Get the balance of current token account being iterated.
         *
         **/
        uint64_t GetBalance() const
        {
            /* If we have exhausted our list of address, otherwise throw here. */
            if(nIterator >= vAddresses.size())
                throw APIException(-52, "No more available accounts for debit");

            return vAddresses[nIterator].second;
        }


        /** GetAddress
         *
         *  Gets the current address of the account we are operating on.
         *
         **/
        uint256_t GetAddress() const
        {
            /* If we have exhausted our list of address, otherwise throw here. */
            if(nIterator >= vAddresses.size())
                throw APIException(-52, "No more available accounts for debit");

            return vAddresses[nIterator].first;
        }
    };


    /* Debit an account for NXS or any token. */
    json::json Finance::Debit(const json::json& jParams, bool fHelp)
    {
        /* Get our genesis-id for this call. */
        const uint256_t hashGenesis =
            users->GetSession(jParams).GetAccount()->Genesis();

        /* Let's keep our working accounts in a nice tidy multimap, mapped by token-id. */
        std::map<uint256_t, TokenAccounts> mapAccounts;

        /* The sending account or token. */
        const TAO::Register::Address hashFrom = ExtractAddress(jParams);
        if(hashFrom == TAO::Register::WILDCARD_ADDRESS)
        {
            /* Extract a token name if debit from ALL. */
            const uint256_t hashToken = ExtractToken(jParams);

            /* Get the token / account object. */
            TAO::Register::Object objToken;
            if(!LLD::Register->ReadObject(hashToken, objToken, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-48, "Token not found");

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

                /* Check for a valid token, otherwise skip it. */
                if(objFrom.get<uint256_t>("token") != hashToken)
                    continue;

                /* Check for an available balance, otherwise skip it. */
                if(objFrom.get<uint64_t>("balance") == 0)
                    continue;

                /* Initialize our map if required. */
                if(!mapAccounts.count(hashToken))
                    mapAccounts[hashToken] = TokenAccounts(GetDecimals(objFrom));

                /* Add our new value to our map now. */
                mapAccounts[hashToken].vAddresses.push_back(std::make_pair(hashRegister, objFrom.get<uint64_t>("balance")));
            }
        }

        /* Handle a sending from ANY which allows a mix-and-match of different token types. */
        else if(hashFrom == 0)
        {
            throw APIException(-3, "Unspecified API error");

            /* debit from ANY
             1. Get all accounts
             2. Map accounts by token, with a pair of address and balance.
             3. Build contracts based on account for account values, so recipients can be a mix of tokens
             */
        }

        /* Regular send from name or address, use same map for managing accounts, but only have a singl entry. */
        else
        {
            /* Get the token / account object. */
            TAO::Register::Object objFrom;
            if(!LLD::Register->ReadObject(hashFrom, objFrom, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-13, "Account not found");

            /* Extract a token name from our from parameter. */
            const uint256_t hashToken = objFrom.get<uint256_t>("token");
            //if(objFrom.get<uint64_t>("balance") == 0) //catch balance issue early
            //    throw APIException(-69, "Insufficient funds");

            /* Now lets check our expected types match. */
            CheckType(jParams, objFrom);

            /* Initialize our map if required. */
            if(!mapAccounts.count(hashToken))
                mapAccounts[hashToken] = TokenAccounts(GetDecimals(objFrom));

            /* Add our new value to our map now. */
            mapAccounts[hashToken].vAddresses.push_back(std::make_pair(hashFrom, objFrom.get<uint64_t>("balance")));
        }

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
                json::json jAdjusted = jRecipient;
                if(fSession)
                    jAdjusted["session"] = jParams["session"];

                vRecipients.push_back(jRecipient);
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

            /* We need to copy session here to get the name (XXX: this is spagetti, geez, need to cleanup names next). */
            if(jParams.find("session") != jParams.end())
                jRecipient["session"] = jParams["session"];

            /* Push this to our recipients vector now. */
            vRecipients.push_back(jRecipient);
        }

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
                TokenAccounts& tAccounts = mapAccounts[0];

                /* Check the amount is not too small once converted by the token Decimals */
                const uint64_t nAmount = std::stod(jRecipient["amount"].get<std::string>()) * math::pow(10, tAccounts.nDecimals);
                if(nAmount == 0)
                    throw APIException(-68, "Amount too small");

                /* Check they have the required funds */
                if(nAmount > tAccounts.GetBalance())
                    throw APIException(-69, "Insufficient funds");

                //XXX: maybe we find a way to have these two share their logic?

                //XXX: handle legacy contract here
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
                TokenAccounts& tAccounts = mapAccounts[hashToken];

                /* Check the amount is not too small once converted by the token Decimals */
                uint64_t nAmount = std::stod(jRecipient["amount"].get<std::string>()) * math::pow(10, tAccounts.nDecimals);
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

                    /* Reduce the current balance and iterate to next account. */
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






    /* Debit a NXS account. */
    json::json DebitOLD(const json::json& jParams, bool fHelp)
    {
        using namespace TAO::Operation;

        json::json ret;

        /* Authenticate the users credentials */
        if(!users->Authenticate(jParams))
            throw APIException(-139, "Invalid credentials");

        /* Get the PIN to be used for this API call */
        SecureString strPIN = users->GetPin(jParams, TAO::Ledger::PinUnlock::TRANSACTIONS);

        /* Get the session to be used for this API call */
        Session& session = users->GetSession(jParams);

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
        if(jParams.find("name") != jParams.end() && !jParams["name"].get<std::string>().empty())
            hashFrom = Names::ResolveAddress(jParams, jParams["name"].get<std::string>());
        else if(jParams.find("address") != jParams.end())
            hashFrom.SetBase58(jParams["address"].get<std::string>());
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

        /* Check the account is a NXS account */
        //if(accountFrom.get<uint256_t>("token") != 0)
        //    throw APIException(-66, "Account is not a NXS account.  Please use the tokens API for debiting non-NXS token accounts.");

        /* Get current balance and significant figures. */
        uint8_t nDecimals = GetDecimals(accountFrom);
        uint64_t nCurrentBalance = accountFrom.get<uint64_t>("balance");

        /* vector of recipient addresses and amounts */
        std::vector<json::json> vRecipients;

        /* Check for the existence of recipients array */
        if(jParams.find("recipients") != jParams.end() && !jParams["recipients"].is_null())
        {
            if(!jParams["recipients"].is_array())
                throw APIException(-216, "recipients field must be an array.");

            /* Get the recipients json array */
            json::json jsonRecipients = jParams["recipients"];


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
            vRecipients.push_back(jParams);
        }

        /* Check that there are not too many recipients to fit into one transaction */
        if(vRecipients.size() > 99)
            throw APIException(-215, "Max number of recipients (99) exceeded");

        /* Flag that there is at least one legacy recipient */
        bool fHasLegacy = false;

        /* Current contract ID */
        uint8_t nContract = 0;

        /* Process the recipients */
        for(const auto& jsonRecipient : vRecipients)
        {
            /* Double check that there are not too many recipients to fit into one transaction */
            if(nContract >= 99)
                throw APIException(-215, "Max number of recipients (99) exceeded");

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
                hashTo = Names::ResolveAddress(jParams, jsonRecipient["name_to"].get<std::string>());
            else if(jsonRecipient.find("address_to") != jsonRecipient.end())
            {
                strAddressTo = jsonRecipient["address_to"].get<std::string>();

                /* Decode the base58 register address */
                if(CheckAddress(strAddressTo))
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
                    AddExpires( jsonRecipient, session.GetAccount()->Genesis(), tx[nContract], fTokenizedDebit);

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
}
