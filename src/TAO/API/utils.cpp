/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#include <unordered_set>

#include <LLD/include/global.h>
#include <LLD/cache/template_lru.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/types/exception.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/mempool.h>

#include <TAO/Operation/types/stream.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/create.h>
#include <TAO/Register/include/names.h>
#include <TAO/Register/include/unpack.h>

#include <Util/include/args.h>
#include <Util/include/hex.h>
#include <Util/include/json.h>
#include <Util/include/base64.h>
#include <Util/include/debug.h>



/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Determines whether a string value is a valid base58 encoded register address.
         *  This only checks to see if the value an be decoded into a valid Register::Address with a valid Type.
         *  It does not check to see whether the register address exists in the database
         */
        bool IsRegisterAddress(const std::string& strValueToCheck)
        {
            /* Decode the incoming string into a register address */
            TAO::Register::Address address;
            address.SetBase58(strValueToCheck);

            /* Check to see whether it is valid */
            return address.IsValid();

        }


        /* Retrieves the number of decimals that applies to amounts for this token or account object.
         *  If the object register passed in is a token account then we need to look at the token definition
         *  in order to get the decimals.  The token is obtained by looking at the identifier field,
         *  which contains the register address of the issuing token
         */
        uint8_t GetDecimals(const TAO::Register::Object& object)
        {
            /* Declare the nDecimals to return */
            uint8_t nDecimals = 0;

            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            /* Check the object standard. */
            switch(nStandard)
            {
                case TAO::Register::OBJECTS::TOKEN:
                {
                    nDecimals = object.get<uint8_t>("decimals");
                    break;
                }

                case TAO::Register::OBJECTS::TRUST:
                {
                    nDecimals = TAO::Ledger::NXS_DIGITS; // NXS token default digits
                    break;
                }

                case TAO::Register::OBJECTS::ACCOUNT:
                {
                    /* If debiting an account we need to look at the token definition in order to get the digits. */
                    TAO::Register::Address nIdentifier = object.get<uint256_t>("token");

                    /* Edge case for NXS token which has identifier 0, so no look up needed */
                    if(nIdentifier == 0)
                        nDecimals = TAO::Ledger::NXS_DIGITS;
                    else
                    {

                        TAO::Register::Object token;
                        if(!LLD::Register->ReadState(nIdentifier, token, TAO::Ledger::FLAGS::MEMPOOL))
                            throw APIException(-125, "Token not found");

                        /* Parse the object register. */
                        if(!token.Parse())
                            throw APIException(-14, "Object failed to parse");

                        nDecimals = token.get<uint8_t>("decimals");
                    }
                    break;
                }

                default:
                {
                    throw APIException(-124, "Unknown token / account.");
                }

            }

            return nDecimals;
        }


        /* In order to work out which registers are currently owned by a particular sig chain
         * we must iterate through all of the transactions for the sig chain and track the history
         * of each register.  By iterating through the transactions from most recent backwards we
         * can make some assumptions about the current owned state.  For example if we find a debit
         * transaction for a register before finding a transfer then we must know we currently own it.
         * Similarly if we find a transfer transaction for a register before any other transaction
         * then we must know we currently to NOT own it.
         */
        bool ListRegisters(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vRegisters)
        {
            /* LRU register cache by genesis hash.  This caches the vector of register addresses along with the last txid of the
               sig chain, so that we can determine whether any new transactions have been added, invalidating the cache.  */
            static LLD::TemplateLRU<uint256_t, std::pair<uint512_t, std::vector<TAO::Register::Address>>> cache(10);

            /* Get the last transaction. */
            uint512_t hashLast = 0;

            /* Get the last transaction for this genesis.  NOTE that we include the mempool here as there may be registers that
               have been created recently but not yet included in a block*/
            if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                return false;

            /* Check the cache to see if we have already cached the registers for this sig chain and it is still valid. */
            if(cache.Has(hashGenesis))
            {
                /* The cached register list */
                std::pair<uint512_t, std::vector<TAO::Register::Address>> cacheEntry;
                
                /* Retrieve the cached register list from the LRU cache */
                cache.Get(hashGenesis, cacheEntry);

                /* Check that the hashlast hasn't changed */
                if(cacheEntry.first == hashLast)
                {
                    /* Poplate vector to return */
                    vRegisters = cacheEntry.second;

                    return true;
                }

            }

            /* Keep a running list of owned and transferred registers. We use a set to store these registers because
             * we are going to be checking them frequently to see if a hash is already in the container,
             * and a set offers us near linear search time
             */
            std::unordered_set<uint256_t> vTransferred;
            std::unordered_set<uint256_t> vOwnedRegisters;

            /* The previous hash in the chain */
            uint512_t hashPrev = hashLast;

            /* Loop until genesis. */
            while(hashPrev != 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashPrev, tx, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-108, "Failed to read transaction");

                /* Set the next last. */
                hashPrev = !tx.IsFirst() ? tx.hashPrevTx : 0;

                /* Iterate through all contracts. */
                for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
                {
                    /* Get the contract output. */
                    const TAO::Operation::Contract& contract = tx[nContract];

                    /* Seek to start of the operation stream in case this a transaction from mempool that has already been read*/
                    contract.Reset(TAO::Operation::Contract::OPERATIONS);

                    /* Deserialize the OP. */
                    uint8_t nOP = 0;
                    contract >> nOP;

                    /* Check the current opcode. */
                    switch(nOP)
                    {
                        /* Condition that allows a validation to occur. */
                        case TAO::Operation::OP::CONDITION:
                        {
                            /* Condition has no parameters. */
                            contract >> nOP;

                            break;
                        }


                        /* Validate a previous contract's conditions */
                        case TAO::Operation::OP::VALIDATE:
                        {
                            /* Skip over validate. */
                            contract.Seek(68);

                            /* Get next OP. */
                            contract >> nOP;

                            break;
                        }
                    }

                    /* Check the current opcode. */
                    switch(nOP)
                    {

                        /* These are the register-based operations that prove ownership if encountered before a transfer*/
                        case TAO::Operation::OP::WRITE:
                        case TAO::Operation::OP::APPEND:
                        case TAO::Operation::OP::CREATE:
                        case TAO::Operation::OP::DEBIT:
                        {
                            /* Extract the address from the contract. */
                            TAO::Register::Address hashAddress;
                            contract >> hashAddress;

                            /* for these operations, if the address is NOT in the transferred list
                               then we know that we must currently own this register */
                           if(vTransferred.find(hashAddress)    == vTransferred.end()
                           && vOwnedRegisters.find(hashAddress) == vOwnedRegisters.end())
                           {
                               /* Add to owned set. */
                               vOwnedRegisters.insert(hashAddress);

                               /* Add to return vector. */
                               vRegisters.push_back(hashAddress);
                           }

                            break;
                        }


                        /* Check for credits here. */
                        case TAO::Operation::OP::CREDIT:
                        {
                            /* Seek past irrelevant data. */
                            contract.Seek(68);

                            /* The account that is being credited. */
                            TAO::Register::Address hashAddress;
                            contract >> hashAddress;

                            /* If we find a credit before a transfer transaction for this register then
                               we can know for certain that we must own it */
                           if(vTransferred.find(hashAddress)    == vTransferred.end()
                           && vOwnedRegisters.find(hashAddress) == vOwnedRegisters.end())
                           {
                               /* Add to owned set. */
                               vOwnedRegisters.insert(hashAddress);

                               /* Add to return vector. */
                               vRegisters.push_back(hashAddress);
                           }

                            break;

                        }


                        /* Check for a transfer here. */
                        case TAO::Operation::OP::TRANSFER:
                        {
                            /* Extract the address from the contract. */
                            TAO::Register::Address hashAddress;
                            contract >> hashAddress;

                            /* Read the register transfer recipient. */
                            TAO::Register::Address hashTransfer;
                            contract >> hashTransfer;

                            /* Read the force transfer flag */
                            uint8_t nType = 0;
                            contract >> nType;

                            /* If we have transferred to a token that we own then we ignore the transfer as we still
                               technically own the register */
                            if(nType == TAO::Operation::TRANSFER::FORCE)
                            {
                                TAO::Register::Object newOwner;
                                if(!LLD::Register->ReadState(hashTransfer, newOwner))
                                    throw APIException(-153, "Transfer recipient object not found");

                                if(newOwner.hashOwner == hashGenesis)
                                    break;
                            }
                            else
                            {
                                /* Retrieve the object so we can see whether it has been claimed or not */
                                TAO::Register::Object object;
                                if(!LLD::Register->ReadState(hashAddress, object, TAO::Ledger::FLAGS::MEMPOOL))
                                    throw APIException(-104, "Object not found");

                                /* If we are transferring to someone else but it has not yet been claimed then we ignore the
                                   transfer and still show it as ours */
                                if(object.hashOwner == 0)
                                    break;
                            }
                            

                            /* If we find a TRANSFER then we can know for certain that we no longer own it */
                            if(vTransferred.find(hashAddress)    == vTransferred.end())
                                vTransferred.insert(hashAddress);

                            break;
                        }


                        /* Check for claims here. */
                        case TAO::Operation::OP::CLAIM:
                        {
                            /* Seek past irrelevant data. */
                            contract.Seek(68);

                            /* Extract the address from the contract. */
                            TAO::Register::Address hashAddress;
                            contract >> hashAddress;

                            /* If we find a CLAIM transaction before a TRANSFER, then we know that we must currently own this register */
                            if(vTransferred.find(hashAddress)    == vTransferred.end()
                            && vOwnedRegisters.find(hashAddress) == vOwnedRegisters.end())
                            {
                                /* Add to owned set. */
                                vOwnedRegisters.insert(hashAddress);

                                /* Add to return vector. */
                                vRegisters.push_back(hashAddress);
                            }

                            break;
                        }

                        default:
                            continue;
                    }
                }
            }

            /* Add the register list to the LRU cache */
            cache.Put(hashGenesis, std::make_pair(hashLast, vRegisters));

            return true;
        }


        /* Scans a signature chain to work out all assets that it owns */
        bool ListObjects(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vObjects)
        {
            /* Get all registers owned by the sig chain */
            std::vector<TAO::Register::Address> vRegisters;
            ListRegisters(hashGenesis, vRegisters);

            /* Filter out only those that are objects */
            for(const auto& address : vRegisters)
            {
                /* Check that the address is for an object */
                if(address.IsObject())
                    vObjects.push_back(address);
            }

            return true;
        }


        /* Scans a signature chain to work out all accounts that it owns */
        bool ListAccounts(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vAccounts, bool fTokens, bool fTrust)
        {
            /* Get all registers owned by the sig chain */
            std::vector<TAO::Register::Address> vRegisters;
            ListRegisters(hashGenesis, vRegisters);

            /* Filter out only those that are accounts */
            for(const auto& address : vRegisters)
            {
                /* Check that the address is for an account or token */
                if(address.IsAccount() || (fTokens && address.IsToken()) || (fTrust && address.IsTrust()))
                    vAccounts.push_back(address);
            }

            return true;
        }


        /* Calculates the required fee for the transaction and adds the OP::FEE contract to the transaction if necessary.
         *  If a specified fee account is not specified, the method will lookup the "default" NXS account and use this account 
         *  to pay the fees.  An exception will be thrownIf there are insufficient funds to pay the fee. */
        bool AddFee(TAO::Ledger::Transaction& tx, const TAO::Register::Address& hashFeeAccount)
        {
            /* First we need to ensure that the transaction is built so that the contracts have their pre states */
            tx.Build();

            /* Obtain the transaction cost */
            uint64_t nCost = tx.Cost();

            /* If a fee needs to be applied then add it */
            if(nCost > 0)
            {
                /* The register adddress of the account to deduct fees from */
                TAO::Register::Address hashRegister;

                /* If the caller has specified a fee account to use then use this */
                if(hashFeeAccount.IsValid() && hashFeeAccount.IsAccount())
                {
                    hashRegister = hashFeeAccount;
                }
                else
                {
                    /* Otherwise we need to look up the default fee account */
                    TAO::Register::Object defaultNameRegister;

                    if(!TAO::Register::GetNameRegister(tx.hashGenesis, std::string("default"), defaultNameRegister))
                        throw TAO::API::APIException(-163, "Could not retrieve default NXS account to debit fees.");

                    /* Get the address of the default account */
                    hashRegister = defaultNameRegister.get<uint256_t>("address");
                }

                /* Retrieve the account */
                TAO::Register::Object object;
                if(!LLD::Register->ReadState(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
                    throw TAO::API::APIException(-13, "Account not found");

                /* Parse the object register. */
                if(!object.Parse())
                    throw TAO::API::APIException(-14, "Object failed to parse");

                /* Get the object standard. */
                uint8_t nStandard = object.Standard();

                /* Check the object standard. */
                if(nStandard != TAO::Register::OBJECTS::ACCOUNT)
                    throw TAO::API::APIException(-65, "Object is not an account");

                /* Check the account is a NXS account */
                if(object.get<uint256_t>("token") != 0)
                    throw TAO::API::APIException(-164, "Fee account is not a NXS account.");

                /* Get the account balance */
                uint64_t nCurrentBalance = object.get<uint64_t>("balance");

                /* Check that there is enough balance to pay the fee */
                if(nCurrentBalance < nCost)
                    throw TAO::API::APIException(-214, "Insufficient funds to pay fee");

                /* Add the fee contract */
                uint32_t nContractPos = tx.Size();
                tx[nContractPos] << uint8_t(TAO::Operation::OP::FEE) << hashRegister << nCost;

                return true;
            }

            return false;
        }

        /* Returns a type string for the register type */
        std::string RegisterType(uint8_t nType)
        {
            std::string strRegisterType = "UNKNOWN";

            switch(nType)
            {
                case TAO::Register::REGISTER::RESERVED :
                    strRegisterType = "RESERVED";
                    break;
                case TAO::Register::REGISTER::READONLY :
                    strRegisterType = "READONLY";
                    break;
                case TAO::Register::REGISTER::APPEND :
                    strRegisterType = "APPEND";
                    break;
                case TAO::Register::REGISTER::RAW :
                    strRegisterType = "RAW";
                    break;
                case TAO::Register::REGISTER::OBJECT :
                    strRegisterType = "OBJECT";
                    break;
                case TAO::Register::REGISTER::SYSTEM :
                    strRegisterType = "SYSTEM";

            }

            return strRegisterType;
        }


        /* Returns a type string for the register object type */
        std::string ObjectType(uint8_t nType)
        {
            std::string strObjectType = "UNKNOWN";

            switch(nType)
            {
                case TAO::Register::OBJECTS::NONSTANDARD :
                    strObjectType = "REGISTER";
                    break;
                case TAO::Register::OBJECTS::ACCOUNT :
                    strObjectType = "ACCOUNT";
                    break;
                case TAO::Register::OBJECTS::NAME :
                    strObjectType = "NAME";
                    break;
                case TAO::Register::OBJECTS::NAMESPACE :
                    strObjectType = "NAMESPACE";
                    break;
                case TAO::Register::OBJECTS::TOKEN :
                    strObjectType = "TOKEN";
                    break;
                case TAO::Register::OBJECTS::TRUST :
                    strObjectType = "TRUST";
                    break;
                case TAO::Register::OBJECTS::CRYPTO :
                    strObjectType = "CRYPTO";
                    break;
            }

            return strObjectType;
        }


        /* Get the sum of all debit notifications for the the specified token */
        uint64_t GetPending(const uint256_t& hashGenesis, const uint256_t& hashToken, const uint256_t& hashAccount)
        {
            /* Th return value */
            uint64_t nPending = 0;

            /* Transaction to check */
            TAO::Ledger::Transaction tx;

            /* Counter of consecutive processed events. */
            uint32_t nConsecutive = 0;

            /* The event sequence number */
            uint32_t nSequence = 0;

            /* Get the last event */
            LLD::Ledger->ReadSequence(hashGenesis, nSequence);

            /* Decrement the current sequence number to get the last event sequence number */
            --nSequence;

            /* Look back through all events to find those that are not yet processed. */
            while(LLD::Ledger->ReadEvent(hashGenesis, nSequence, tx))
            {
                /* Check to see if we have 100 (or the user configured amount) consecutive processed events.  If we do then we 
                   assume all prior events are also processed.  This saves us having to scan the entire chain of events */
                if(nConsecutive >= config::GetArg("-eventsdepth", 100))
                    break;

                /* Loop through transaction contracts. */
                uint32_t nContracts = tx.Size();
                for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                {
                    /* The proof to check for this contract */
                    TAO::Register::Address hashProof;

                    /* Reset the op stream */
                    tx[nContract].Reset();

                    /* The operation */
                    uint8_t nOp;
                    tx[nContract] >> nOp;

                    /* Check for that the debit is meant for us. */
                    if(nOp == TAO::Operation::OP::DEBIT)
                    {
                        /* Get the source address which is the proof for the debit */
                        tx[nContract] >> hashProof;

                        /* Get the recipient account */
                        TAO::Register::Address hashTo;
                        tx[nContract] >> hashTo;

                        /* Retrieve the account. */
                        TAO::Register::Object account;
                        if(!LLD::Register->ReadState(hashTo, account))
                            continue;

                        /* Parse the object register. */
                        if(!account.Parse())
                            continue;

                        /* Check that this is an account */
                        if(account.Base() != TAO::Register::OBJECTS::ACCOUNT )
                            continue;

                        /* Get the token address */
                        TAO::Register::Address token = account.get<uint256_t>("token");

                        /* Check the account token matches the one passed in*/
                        if(token != hashToken)
                            continue;

                        /* Check owner that we are the owner of the recipient account  */
                        if(account.hashOwner != hashGenesis)
                            continue;

                        /* Check to see if we have already credited this debit. NOTE we do this before checking whether the account 
                           for this event matches the account we are getting the pending balance for, as we are making the 
                           assumption that if the last X number of events have been processed then there are no others pending 
                           for any account*/
                        if(LLD::Ledger->HasProof(hashProof, tx.GetHash(), nContract, TAO::Ledger::FLAGS::MEMPOOL))
                        {
                            nConsecutive++;
                            continue;
                        }

                        /* Check the account filter */
                        if(hashAccount != 0 && hashAccount != hashTo)
                            continue;

                    }
                    else if(hashToken == 0 // only include coinbase for NXS token
                        && hashAccount == 0 // only include coinbase if no account is specified
                        && nOp == TAO::Operation::OP::COINBASE)
                    {
                        /* Unpack the miners genesis from the contract */
                        if(!TAO::Register::Unpack(tx[nContract], hashProof))
                            continue;

                        /* Check that it is meant for our sig chain */
                        if(hashGenesis != hashProof)
                            continue;

                        /* Check to see if we have already credited this coinbase. */
                        if(LLD::Ledger->HasProof(hashProof, tx.GetHash(), nContract, TAO::Ledger::FLAGS::MEMPOOL))
                        {
                            nConsecutive++;
                            continue;
                        }
                    }
                    else
                        continue;

                    /* Get the amount */
                    uint64_t nAmount = 0;
                    TAO::Register::Unpack(tx[nContract], nAmount);

                    /* Add it onto our pending amount */
                    nPending += nAmount;

                    /* Reset the consecutive counter since this has not been processed */
                    nConsecutive = 0;
                }

                /* Iterate the sequence id backwards. */
                --nSequence;
            }

            /* Next we need to include mature coinbase transactions.  We can skip this if a token as been specified as coinbase
               only apply to NXS accounts.  We can also skip if an account has been specified as coinbases can be credited to
               any account */
            if(hashToken == 0 && hashAccount == 0)
            {
                /* Get the last transaction. */
                uint512_t hashLast = 0;
                LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL);

                /* Get the mature coinbase transactions */
                std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> vContracts;
                Users::get_coinbases(hashGenesis, hashLast, vContracts);

                /* Iterate through all un-credited debits and see whether they apply to this token/account */
                for(const auto& contract : vContracts)
                {
                    /* Get a reference to the contract */
                    const TAO::Operation::Contract& refContract = std::get<0>(contract);

                    /* Reset the contract operation stream. */
                    refContract.Reset();

                    /* Get the opcode. */
                    uint8_t OPERATION;
                    refContract >> OPERATION;

                    /* Get the genesis hash */
                    TAO::Register::Address hashFrom;
                    refContract >> hashFrom;

                    /* Get the amount */
                    uint64_t nAmount = 0;
                    refContract >> nAmount;

                    /* Add this to the pending amount */
                    nPending += nAmount;
                }
            }


            /* Now we need to look at tokenized debits that are coming in.  We can skip this if an account filter has been passed
               in as tokenized debits are not made to a specific account. */
            if(hashAccount == 0)
            {
                std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> vContracts;
                Users::get_tokenized_debits(hashGenesis, vContracts);

                /* Iterate through all un-credited debits and see whether they apply to this token/account */
                for(const auto& contract : vContracts)
                {
                    /* Get a reference to the contract */
                    const TAO::Operation::Contract& refContract = std::get<0>(contract);

                    /* Reset the contract operation stream. */
                    refContract.Reset();

                    /* Get the opcode. */
                    uint8_t OPERATION;
                    refContract >> OPERATION;

                    /* Get the token/account we are debiting from */
                    TAO::Register::Address hashFrom;
                    refContract >> hashFrom;

                    /* REtrieve the account/token the debit was from */
                    TAO::Register::Object from;
                    if(!LLD::Register->ReadState(hashFrom, from))
                        continue;

                    /* Parse the object register. */
                    if(!from.Parse())
                        continue;

                    /* Check the token type */
                    if(from.get<uint256_t>("token") != hashToken)
                        continue;

                    /* Retrieve the proof account so we can work out the partial claim amount */
                    TAO::Register::Address hashProof = std::get<2>(contract);

                    /* Read the object register, which is the proof account . */
                    TAO::Register::Object account;
                    if(!LLD::Register->ReadState(hashProof, account, TAO::Ledger::FLAGS::MEMPOOL))
                        continue;

                    /* Parse the object register. */
                    if(!account.Parse())
                        continue;

                    /* Check that this is an account */
                    if(account.Standard() != TAO::Register::OBJECTS::ACCOUNT )
                        continue;

                    /* Get the token address */
                    TAO::Register::Address hashProofToken = account.get<uint256_t>("token");

                    /* Read the token register. */
                    TAO::Register::Object token;
                    if(!LLD::Register->ReadState(hashProofToken, token, TAO::Ledger::FLAGS::MEMPOOL))
                        continue;

                    /* Parse the object register. */
                    if(!token.Parse())
                        continue;

                    /* Get the token supply so that we an determine our share */
                    uint64_t nSupply = token.get<uint64_t>("supply");

                    /* Get the balance of our token account */
                    uint64_t nBalance = account.get<uint64_t>("balance");

                    /* Get the amount from the debit contract*/
                    uint64_t nAmount = 0;
                    TAO::Register::Unpack(refContract, nAmount);

                    /* Calculate the partial debit amount that this token holder is entitled to. */
                    uint64_t nPartial = (nAmount * nBalance) / nSupply;

                    /* Add this to our pending balance */
                    nPending += nPartial;
                }

            }

            return nPending;
        }


        /* Get the sum of all debit transactions in the mempool for the the specified token */
        uint64_t GetUnconfirmed(const uint256_t& hashGenesis, const uint256_t& hashToken, bool fOutgoing, const uint256_t& hashAccount)
        {
            /* The return value */
            uint64_t nUnconfirmed = 0;

            /* Get all transactions in the mempool */
            std::vector<uint512_t> vMempool;
            if(TAO::Ledger::mempool.List(vMempool))
            {
                /* Loop through the list of transactions. */
                for(const auto& hash : vMempool)
                {
                    /* Get the transaction from the memory pool. */
                    TAO::Ledger::Transaction tx;
                    if(!TAO::Ledger::mempool.Get(hash, tx))
                        continue;

                    /* Loop through transaction contracts. */
                    uint32_t nContracts = tx.Size();
                    for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                    {
                        /* Reset the op stream */
                        tx[nContract].Reset();

                        /* The operation */
                        uint8_t nOp;
                        tx[nContract] >> nOp;

                        /* Check for that the debit is meant for us. */
                        if(nOp == TAO::Operation::OP::DEBIT)
                        {
                            /* Get the source address which is the proof for the debit */
                            TAO::Register::Address hashFrom;
                            tx[nContract] >> hashFrom;

                            /* Get the recipient account */
                            TAO::Register::Address hashTo;
                            tx[nContract] >> hashTo;

                            /* Get the amount */
                            uint64_t nAmount = 0;
                            tx[nContract] >> nAmount;

                            /* If caller wants outgoing transactions then check that we made the transaction */
                            if(fOutgoing)
                            {
                                /* Check we made the transaction */
                                if(tx.hashGenesis != hashGenesis)
                                    continue;

                                /* Check the account filter based on the originating account*/
                                if(hashAccount != 0 && hashAccount != hashFrom)
                                    continue;

                                /* Retrieve the account. */
                                TAO::Register::Object account;
                                if(!LLD::Register->ReadState(hashFrom, account))
                                    continue;

                                /* Parse the object register. */
                                if(!account.Parse())
                                    continue;

                                /* Check that this is an account */
                                if(account.Base() != TAO::Register::OBJECTS::ACCOUNT )
                                    continue;

                                /* Get the token address */
                                TAO::Register::Address token = account.get<uint256_t>("token");

                                /* Check the account token matches the one passed in*/
                                if(token != hashToken)
                                    continue;
                            }
                            else
                            {
                                /* Check the account filter */
                                if(hashAccount != 0 && hashAccount != hashTo)
                                    continue;

                                /* Retrieve the account. */
                                TAO::Register::Object account;
                                if(!LLD::Register->ReadState(hashTo, account))
                                    continue;

                                /* Parse the object register. */
                                if(!account.Parse())
                                    continue;

                                /* Check that this is an account */
                                if(account.Base() != TAO::Register::OBJECTS::ACCOUNT )
                                    continue;

                                /* Get the token address */
                                TAO::Register::Address token = account.get<uint256_t>("token");

                                /* Check the account token matches the one passed in*/
                                if(token != hashToken)
                                    continue;

                                /* Check owner that we are the owner of the recipient account  */
                                if(account.hashOwner != hashGenesis)
                                    continue;
                            }

                            /* Add this amount to our total */
                            nUnconfirmed += nAmount;

                        }
                        /* Check for that the credits we made . */
                        else if(!fOutgoing && nOp == TAO::Operation::OP::CREDIT)
                        {
                            /* For credits first make sure we made the transaction */
                            if(tx.hashGenesis != hashGenesis)
                                continue;

                            /* Extract the transaction from contract. */
                            uint512_t hashTx = 0;
                            tx[nContract] >> hashTx;

                            /* Extract the contract-id. */
                            uint32_t nID = 0;
                            tx[nContract] >> nID;

                            /* Get the account address. */
                            TAO::Register::Address hashTo;
                            tx[nContract] >> hashTo;

                            /* Get the proof address. */
                            TAO::Register::Address hashProof;
                            tx[nContract] >> hashProof;

                            /* Get the credit amount. */
                            uint64_t nCredit = 0;
                            tx[nContract] >> nCredit;

                            /* Check the account filter */
                            if(hashAccount != 0 && hashAccount != hashTo)
                                continue;

                            /* Retrieve the account. */
                            TAO::Register::Object account;
                            if(!LLD::Register->ReadState(hashTo, account))
                                continue;

                            /* Parse the object register. */
                            if(!account.Parse())
                                continue;

                            /* Check that this is an account */
                            if(account.Base() != TAO::Register::OBJECTS::ACCOUNT )
                                continue;

                            /* Get the token address */
                            TAO::Register::Address token = account.get<uint256_t>("token");

                            /* Check the account token matches the one passed in*/
                            if(token != hashToken)
                                continue;

                            /* Check owner that we are the owner of the recipient account  */
                            if(account.hashOwner != hashGenesis)
                                continue;

                            /* Add this amount to our total */
                            nUnconfirmed += nCredit;
                        }
                        /* Check for outgoing OP::LEGACY */
                        else if(fOutgoing && nOp == TAO::Operation::OP::LEGACY)
                        {
                            /* Check the token filter is 0 as OP::LEGACY are only for NXS accounts*/
                            if(hashToken != 0)
                                continue;

                            /* Get the source address which is the proof for the debit */
                            TAO::Register::Address hashFrom;
                            tx[nContract] >> hashFrom;

                            /* Get the amount */
                            uint64_t nAmount = 0;
                            tx[nContract] >> nAmount;

                            /* Check we made the transaction */
                            if(tx.hashGenesis != hashGenesis)
                                continue;

                            /* Check the account filter based on the originating account*/
                            if(hashAccount != 0 && hashAccount != hashFrom)
                                continue;

                            /* Add this amount to our total */
                            nUnconfirmed += nAmount;
                        }
                    }
                }
            }

            return nUnconfirmed;
        }


        /*  Get the outstanding coinbases. */
        uint64_t GetImmature(const uint256_t& hashGenesis)
        {
            /* Return amount */
            uint64_t nImmature = 0;

            /* Get the last transaction. */
            uint512_t hashLast = 0;
            if(LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
            {
                /* Reverse iterate until genesis (newest to oldest). */
                while(hashLast != 0)
                {
                    /* Get the transaction from disk. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                        debug::error(FUNCTION, "Failed to read transaction");

                    /* Skip this transaction if it is mature. */
                    if(LLD::Ledger->ReadMature(hashLast))
                    {
                        /* Set the next last. */
                        hashLast = !tx.IsFirst() ? tx.hashPrevTx : 0;
                        continue;
                    }

                    /* Loop through all contracts and check that it is a coinbase. */
                    uint32_t nContracts = tx.Size();
                    for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                    {
                        uint64_t nAmount = 0;

                        /* The operation */
                        uint8_t nOp = 0;

                        /* Reset the contract operation stream */
                        tx[nContract].Reset();

                        /* Get the operation */
                        tx[nContract] >> nOp;

                        /* Check for coinbase or coinstake opcode */
                        if(nOp == Operation::OP::COINBASE)
                        {
                            /* Get the amount */
                            TAO::Register::Unpack(tx[nContract], nAmount);

                            /* Get the genesis of the coinbase recipient*/
                            TAO::Register::Address hashRecipient;
                            TAO::Register::Unpack(tx[nContract], hashRecipient);

                            /* Check that the contract was for us */
                            if(hashRecipient == hashGenesis)
                                /* Add it to our return values */
                                nImmature += nAmount;

                        }
                    }

                    /* Set the next last. */
                    hashLast = !tx.IsFirst() ? tx.hashPrevTx : 0;
                }
            }

            return nImmature;
        }


        /* Calculates the percentage of tokens owned from the total supply. */
        double GetTokenOwnership(const TAO::Register::Address& hashToken, const uint256_t& hashGenesis)
        {
            /* Find all token accounts owned by the caller for the token */
            std::vector<TAO::Register::Address> vAccounts;
            ListAccounts(hashGenesis, vAccounts, true, false);

            /* The balance of tokens owned for this asset */
            uint64_t nBalance = 0;

            for(const auto& hashAccount : vAccounts)
            {
                /* Make sure it is an account or the token itself (in case not all supply has been distributed)*/
                if(!hashAccount.IsAccount() && !hashAccount.IsToken())
                    continue;

                /* Get the account from the register DB. */
                TAO::Register::Object object;
                if(!LLD::Register->ReadState(hashAccount, object, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-13, "Account not found");

                /* Check that this is a non-standard object type so that we can parse it and check the type*/
                if(object.nType != TAO::Register::REGISTER::OBJECT)
                    continue;

                /* parse object so that the data fields can be accessed */
                if(!object.Parse())
                    throw APIException(-36, "Failed to parse object register");

                /* Check that this is an account or token */
                if(object.Base() != TAO::Register::OBJECTS::ACCOUNT)
                    continue;

                /* Check the token*/
                if(object.get<uint256_t>("token") != hashToken)
                    continue;

                /* Get the balance */
                nBalance += object.get<uint64_t>("balance");
            }

            /* If we have a balance > 0 then we have some ownership, so work out the % */
            if(nBalance > 0)
            {
                /* Retrieve the token itself so we can get the supply */
                TAO::Register::Object token;
                if(!LLD::Register->ReadState(hashToken, token, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-125, "Token not found");

                /* Parse the object register. */
                if(!token.Parse())
                    throw APIException(-14, "Object failed to parse");

                /* Get the total supply */
                uint64_t nSupply = token.get<uint64_t>("supply");

                /* Calculate the ownership % */
                return (double)nBalance / (double)nSupply * 100.0;
            }
            else
            {
                return 0.0;
            }

        }


        /* Lists all object registers partially owned by way of tokens that the sig chain owns  */
        bool ListPartial(const uint256_t& hashGenesis, std::vector<TAO::Register::Address>& vRegisters)
        {
            /* Find all token accounts owned by the caller */
            std::vector<TAO::Register::Address> vAccounts;
            ListAccounts(hashGenesis, vAccounts, true, false);

            for(const auto& hashAccount : vAccounts)
            {
                /* Make sure it is an account or the token itself (in case not all supply has been distributed)*/
                if(!hashAccount.IsAccount() && !hashAccount.IsToken())
                    continue;

                /* Get the account from the register DB. */
                TAO::Register::Object object;
                if(!LLD::Register->ReadState(hashAccount, object, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-13, "Account not found");

                /* Check that this is a non-standard object type so that we can parse it and check the type*/
                if(object.nType != TAO::Register::REGISTER::OBJECT)
                    continue;

                /* parse object so that the data fields can be accessed */
                if(!object.Parse())
                    throw APIException(-36, "Failed to parse object register");

                /* Check that this is an account or token */
                if(object.Base() != TAO::Register::OBJECTS::ACCOUNT)
                    continue;

                /* Get the token*/
                TAO::Register::Address hashToken = object.get<uint256_t>("token") ;

                /* NXS can't be used to tokenize an asset so if this is a NXS account we can skip it */
                if(hashToken == 0)
                    continue;

                /* Get all objects owned by this token */
                std::vector<TAO::Register::Address> vTokenizedObjects;
                ListTokenizedObjects(hashToken, vTokenizedObjects);

                /* Add them to the list if they are not already in there */
                for(const auto& address : vTokenizedObjects)
                {
                    if(std::find(vRegisters.begin(), vRegisters.end(), address) == vRegisters.end())
                        vRegisters.push_back(address);
                }
            }

            return vRegisters.size() > 0;
        }


        /* Finds all objects that have been tokenized and therefore owned by hashToken */
        bool ListTokenizedObjects(const TAO::Register::Address& hashToken,
                                  std::vector<TAO::Register::Address>& vObjects)
        {
            /* There is no index of the assets owned by a token.  Therefore, to determine which assets the token owns, we can scan
               through the events for the token itself to find all object transfers where the new owner is the token. */

            /* Transaction for the event. */
            TAO::Ledger::Transaction tx;

            /* Iterate all events in the sig chain */
            uint32_t nSequence = 0;
            while(LLD::Ledger->ReadEvent(hashToken, nSequence, tx))
            {
                /* Loop through transaction contracts. */
                uint32_t nContracts = tx.Size();
                for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                {
                    /* Reset the op stream */
                    tx[nContract].Reset();

                    /* The operation */
                    uint8_t nOp;
                    tx[nContract] >> nOp;

                    if(nOp == TAO::Operation::OP::TRANSFER)
                    {
                        /* The register address being transferred */
                        TAO::Register::Address hashRegister;
                        tx[nContract] >> hashRegister;

                        /* Get the new owner hash */
                        TAO::Register::Address hashTo;
                        tx[nContract] >> hashTo;

                        /* Read the force transfer flag */
                        uint8_t nType = 0;
                        tx[nContract] >> nType;

                        /* Ensure this was a forced transfer (which tokenized asset transfers must be) */
                        if(nType != TAO::Operation::TRANSFER::FORCE)
                            continue;

                        /* Check that the recipient of the transfer is the token */
                        if(hashToken != hashTo)
                            continue;

                        vObjects.push_back(hashRegister);
                    }

                    else
                        continue;
                }

                /* Iterate the sequence id forward. */
                ++nSequence;
            }

            /* Return true if we found any objects owned by the token */
            return vObjects.size() > 0;
        }


        /* Utilty method that checks that the signature chain is mature and can therefore create new transactions.
        *  Throws an appropriate APIException if it is not mature. */
        void CheckMature(const uint256_t& hashGenesis)
        {
            /* No need to check this in private mode as there is no PoS/Pow */
            if(!config::GetBoolArg("-private"))
            {
                /* Get the number of blocks to maturity for this sig chain */
                uint32_t nBlocksToMaturity = users->BlocksToMaturity(hashGenesis);

                if(nBlocksToMaturity > 0)
                    throw APIException(-202, debug::safe_printstr( "Signature chain not mature after your previous mined/stake block. ", nBlocksToMaturity, " more confirmation(s) required."));
            }
        }


        /* Reads a batch of states registers from the Register DB */
        bool GetRegisters(const std::vector<TAO::Register::Address>& vAddresses, 
                          std::vector<std::pair<TAO::Register::Address, TAO::Register::State>>& vStates)
        {
            for(const auto& hashRegister : vAddresses)
            {
                /* Get the state from the register DB. */
                TAO::Register::State state;
                if(!LLD::Register->ReadState(hashRegister, state, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-104, "Object not found");
                
                vStates.push_back(std::make_pair(hashRegister, state));
            }

            /* Now sort the states based on the creation time */
            std::sort(vStates.begin(), vStates.end(), 
                [](const std::pair<TAO::Register::Address, TAO::Register::State> &a,  
                const std::pair<TAO::Register::Address, TAO::Register::State> &b)
                { 
                    return ( a.second.nCreated < b.second.nCreated );
                });

            return vStates.size() > 0;
        }


        /* Creates a void contract for the specified transaction  */
        bool VoidContract(const TAO::Operation::Contract& contract, const uint32_t nContract, TAO::Operation::Contract &voidContract)
        {
            /* The return flag indicating the contract was voided */
            bool bVoided = false;

            /* Get the transaction hash */
            uint512_t hashTx = contract.Hash();

            /* Reset the operation stream position in case it was loaded from mempool and therefore still in previous state */
            contract.Reset();

            /* Get the operation byte. */
            uint8_t nType = 0;
            contract >> nType;

            /* Check for conditional OP */
            if(nType == TAO::Operation::OP::CONDITION)
                contract >> nType;

            /* Ensure that it is a debit or transfer */
            if(nType != TAO::Operation::OP::DEBIT && nType != TAO::Operation::OP::TRANSFER)
                return false;

            /* Process crediting a debit */
            if(nType == TAO::Operation::OP::DEBIT)
            {
                /* Get the hashFrom from the debit transaction. This is the account we are going to return the credit to*/
                TAO::Register::Address hashFrom;
                contract >> hashFrom;

                /* Get the hashTo from the debit transaction. */
                TAO::Register::Address hashTo;
                contract >> hashTo;

                /* Get the amount to respond to. */
                uint64_t nAmount = 0;
                contract >> nAmount;

                /* Get the token / account object that the debit was made to. */
                TAO::Register::Object debit;
                if(!LLD::Register->ReadState(hashTo, debit))
                    return false;

                /* Parse the object register. */
                if(!debit.Parse())
                    throw APIException(-41, "Failed to parse object from debit transaction");

                /* Check to see whether there are any partial credits already claimed against the debit */
                uint64_t nClaimed = 0;
                if(!LLD::Ledger->ReadClaimed(hashTx, nContract, nClaimed, TAO::Ledger::FLAGS::MEMPOOL))
                    nClaimed = 0; 

                /* Check that there is something to be claimed */
                if(nClaimed == nAmount)
                    throw APIException(-173, "Cannot void debit transaction as it has already been fully credited by all recipients");

                /* Reduce the amount to credit by the amount already claimed */
                nAmount -= nClaimed;

                /* Create the credit contract  */
                voidContract<< uint8_t(TAO::Operation::OP::CREDIT) << hashTx << uint32_t(nContract) << hashFrom <<  hashFrom << nAmount;
                
                bVoided = true;   
            }
            /* Process voiding a transfer */
            else if(nType == TAO::Operation::OP::TRANSFER)
            {
                /* Get the address of the asset being transferred from the transaction. */
                TAO::Register::Address hashAddress;
                contract >> hashAddress;

                /* Get the genesis hash (recipient) of the transfer*/
                uint256_t hashGenesis = 0;
                contract >> hashGenesis;

                /* Read the force transfer flag */
                uint8_t nForceFlag = 0;
                contract >> nForceFlag;

                /* Ensure this wasn't a forced transfer (which requires no Claim) */
                if(nForceFlag == TAO::Operation::TRANSFER::FORCE)
                    return false;

                /* Create the claim contract  */
                voidContract << (uint8_t)TAO::Operation::OP::CLAIM << hashTx << uint32_t(nContract) << hashAddress;
                
                bVoided = true;
            }

            return bVoided;
        }


        /* Get the number of transactions in the sig chain relating to the register. */
        uint32_t GetTxCount(const uint256_t& hashGenesis, const TAO::Register::Object& object, const TAO::Register::Address& hashRegister)
        {
            /* return value. */
            uint32_t nCount = 0;;

            /* Flag indicating that the register is a trust account, as we need to process trust-related transactions differently */
            bool fTrust = false;

           
            /* We need to check if this register is a trust account as stake/genesis/trust/stake/unstake
               transactions don't include the account register address like all other transactions do */
            if(object.Standard() == TAO::Register::OBJECTS::TRUST)
                fTrust = true;

            /* Get the last transaction. */
            uint512_t hashLast = 0;
            if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-144, "No transactions found");

            /* Loop until genesis. */
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-108, "Failed to read transaction");

                /* Flag indicating this transaction is trust related */
                bool fTrustRelated = false;

                /* The contracts to include that relate to the supplied register */
                std::vector<std::pair<TAO::Operation::Contract, uint32_t>> vContracts;

                /* Check all contracts in the transaction to see if any of them relates to the requested object register. */
                uint32_t nContracts = tx.Size();
                for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                {
                    /* reset trust related flag */
                    fTrustRelated = false;

                    /* The register address that this contract relates to, if any  */
                    TAO::Register::Address hashAddress;

                    /* Retrieve the contract from the transaction for easier processing */
                    const TAO::Operation::Contract& contract = tx[nContract];

                    /* Start the stream at the beginning. */
                    contract.Reset();

                    /* Get the contract operation. */
                    uint8_t OPERATION = 0;
                    contract >> OPERATION;

                    /* Check for conditional OP */
                    switch(OPERATION)
                    {
                        case TAO::Operation::OP::VALIDATE:
                        {
                            /* Seek through validate. */
                            contract.Seek(68);
                            contract >> OPERATION;

                            break;
                        }

                        case TAO::Operation::OP::CONDITION:
                        {
                            /* Get new operation. */
                            contract >> OPERATION;
                        }
                    }

                    /* Check the current opcode. */
                    switch(OPERATION)
                    {
                        case TAO::Operation::OP::WRITE:
                        case TAO::Operation::OP::APPEND:
                        case TAO::Operation::OP::CREATE:
                        case TAO::Operation::OP::TRANSFER:
                        case TAO::Operation::OP::DEBIT:
                        case TAO::Operation::OP::FEE:
                        case TAO::Operation::OP::LEGACY:
                        {
                            /* Get the Address of the Register. */
                            contract >> hashAddress;
                            break;
                        }

                        case TAO::Operation::OP::CLAIM:
                        {
                            /* Extract the transaction from contract. */
                            uint512_t hashTx = 0;
                            contract >> hashTx;

                            /* Extract the contract-id. */
                            uint32_t nContract = 0;
                            contract >> nContract;

                            /* Extract the address from the contract. */
                            contract >> hashAddress;
                            break;
                        }

                        case TAO::Operation::OP::CREDIT:
                        {
                            /* Extract the transaction from contract. */
                            uint512_t hashTx = 0;
                            contract >> hashTx;

                            /* Extract the contract-id. */
                            uint32_t nID = 0;
                            contract >> nID;

                            /* Get the receiving account address. */
                            contract >> hashAddress;
                            break;
                        }

                        case TAO::Operation::OP::TRUST:
                        case TAO::Operation::OP::GENESIS:
                        case TAO::Operation::OP::MIGRATE:
                        {
                            fTrustRelated = true;
                            break;
                        }
                    }

                    /* If the register from the contract is the same as the requested register OR if the register is a trust account
                       and the contract is a trust related operation, then include this tx in the count*/
                    if((hashAddress == hashRegister) || (fTrust && fTrustRelated))
                    {
                        /* increment the tx count */
                        ++nCount;

                        /* break out of processing this transaction as we don't need to look at any more contracts in it */
                        break;
                    }
                }

                /* Set the next last. */
                hashLast = !tx.IsFirst() ? tx.hashPrevTx : 0;
                
            }

            return nCount;
        }




    } // End API namespace
} // End TAO namespace
