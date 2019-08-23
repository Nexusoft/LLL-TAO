/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#include <unordered_set>

#include <LLD/include/global.h>

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
        /* Determins whether a string value is a register address.
        *  This only checks to see if the value is 64 characters in length and all hex characters (i.e. can be converted to a uint256).
        *  It does not check to see whether the register address exists in the database
        */
        bool IsRegisterAddress(const std::string& strValueToCheck)
        {
            return strValueToCheck.length() == 64 && strValueToCheck.find_first_not_of("0123456789abcdefABCDEF", 0) == std::string::npos;
        }


        /* Retrieves the number of digits that applies to amounts for this token or account object.
        *  If the object register passed in is a token account then we need to look at the token definition
        *  in order to get the digits.  The token is obtained by looking at the identifier field,
        *  which contains the register address of the issuing token
        */
        uint64_t GetDigits(const TAO::Register::Object& object)
        {
            /* Declare the nDigits to return */
            uint64_t nDigits = 0;

            /* Get the object standard. */
            uint8_t nStandard = object.Standard();

            /* Check the object standard. */
            switch(nStandard)
            {
                case TAO::Register::OBJECTS::TOKEN:
                {
                    nDigits = object.get<uint64_t>("digits");
                    break;
                }

                case TAO::Register::OBJECTS::TRUST:
                {
                    nDigits = TAO::Ledger::NXS_DIGITS; // NXS token default digits
                    break;
                }

                case TAO::Register::OBJECTS::ACCOUNT:
                {
                    /* If debiting an account we need to look at the token definition in order to get the digits. */
                    uint256_t nIdentifier = object.get<uint256_t>("token");

                    /* Edge case for NXS token which has identifier 0, so no look up needed */
                    if(nIdentifier == 0)
                        nDigits = TAO::Ledger::NXS_DIGITS;
                    else
                    {

                        TAO::Register::Object token;
                        if(!LLD::Register->ReadState(nIdentifier, token, TAO::Ledger::FLAGS::MEMPOOL))
                            throw APIException(-125, "Token not found");

                        /* Parse the object register. */
                        if(!token.Parse())
                            throw APIException(-14, "Object failed to parse");

                        nDigits = token.get<uint64_t>("digits");
                    }
                    break;
                }

                default:
                {
                    throw APIException(-124, "Unknown token / account.");
                }

            }

            return nDigits;
        }


        /* In order to work out which registers are currently owned by a particular sig chain
         * we must iterate through all of the transactions for the sig chain and track the history
         * of each register.  By iterating through the transactions from most recent backwards we
         * can make some assumptions about the current owned state.  For example if we find a debit
         * transaction for a register before finding a transfer then we must know we currently own it.
         * Similarly if we find a transfer transaction for a register before any other transaction
         * then we must know we currently to NOT own it.
         */
        bool ListRegisters(const uint256_t& hashGenesis, std::vector<uint256_t>& vRegisters)
        {
            /* Get the last transaction. */
            uint512_t hashLast = 0;

            /* Get the last transaction for this genesis.  NOTE that we include the mempool here as there may be registers that
               have been created recently but not yet included in a block*/
            if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                return false;

            /* Keep a running list of owned and transferred registers. We use a set to store these registers because
             * we are going to be checking them frequently to see if a hash is already in the container,
             * and a set offers us near linear search time
             */
            std::unordered_set<uint256_t> vTransferred;
            std::unordered_set<uint256_t> vOwnedRegisters;

            /* Loop until genesis. */
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-108, "Failed to read transaction");

                /* Set the next last. */
                hashLast = tx.hashPrevTx;

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
                            uint256_t hashAddress = 0;
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
                            uint256_t hashAddress = 0;
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
                            uint256_t hashAddress = 0;
                            contract >> hashAddress;

                            /* Read the register transfer recipient. */
                            uint256_t hashTransfer = 0;
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
                            uint256_t hashAddress = 0;
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

            return true;
        }


        /* Calculates the required fee for the transaction and adds the OP::FEE contract to the transaction if necessary.
        *  The method will lookup the "default" NXS account and use this account to pay the fees.  An exception will be thrown
        *  If there are insufficient funds to pay the fee. */
        bool AddFee(TAO::Ledger::Transaction& tx)
        {
            uint64_t nCost = tx.Cost();
            if(nCost > 0)
            {
                TAO::Register::Object defaultNameRegister;

                if(!TAO::Register::GetNameRegister(tx.hashGenesis, std::string("default"), defaultNameRegister))
                    throw TAO::API::APIException(-163, "Could not retrieve default NXS account to debit fees.");

                /* Get the address of the default account */
                uint256_t hashFeeAccount = defaultNameRegister.get<uint256_t>("address");

                /* Retrieve the account */
                TAO::Register::Object object;
                if(!LLD::Register->ReadState(hashFeeAccount, object, TAO::Ledger::FLAGS::MEMPOOL))
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
                    throw TAO::API::APIException(-164, "Account 'default' is not a NXS account.");
                
                /* Get the account balance */
                uint64_t nCurrentBalance = object.get<uint64_t>("balance");

                /* Check that there is enough balance to pay the fee */
                if(nCurrentBalance < nCost)
                    throw TAO::API::APIException(-69, "Insufficient funds");

                /* Add the fee contract */
                uint32_t nContractPos = tx.Size();
                tx[nContractPos] << uint8_t(TAO::Operation::OP::FEE) << hashFeeAccount << nCost; 

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

            /* Iterate through all events */
            uint32_t nSequence = 0;
            while(LLD::Ledger->ReadEvent(hashGenesis, nSequence, tx))
            {
                /* Loop through transaction contracts. */
                uint32_t nContracts = tx.Size();
                for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                {
                    /* The proof to check for this contract */
                    uint256_t hashProof = 0;

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
                        uint256_t hashTo = 0;
                        tx[nContract] >> hashTo; 

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
                        uint256_t token = account.get<uint256_t>("token");
                        
                        /* Check the account token matches the one passed in*/
                        if(token != hashToken)
                            continue;

                        /* Check owner that we are the owner of the recipient account  */
                        if(account.hashOwner != hashGenesis)
                            continue;

                        /* Check to see if we have already credited this debit. */
                        if(LLD::Ledger->HasProof(hashProof, tx.GetHash(), nContract, TAO::Ledger::FLAGS::MEMPOOL))
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
                            continue;
                    }
                    else 
                        continue;

                    /* Get the amount */
                    uint64_t nAmount = 0;
                    TAO::Register::Unpack(tx[nContract], nAmount);

                    /* Add it onto our pending amount */
                    nPending += nAmount;
                }

                /* Iterate the sequence id forward. */
                ++nSequence;
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
                    uint256_t hashFrom = 0;
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
                    uint256_t hashFrom = 0;
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
                    uint256_t hashProof = std::get<2>(contract);

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
                    uint256_t hashProofToken = account.get<uint256_t>("token");                    

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
                            /* The proof to check for this contract */
                            uint256_t hashFrom = 0;
                            
                            /* Get the source address which is the proof for the debit */
                            tx[nContract] >> hashFrom;
                            
                            /* Get the recipient account */
                            uint256_t hashTo = 0;
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
                                uint256_t token = account.get<uint256_t>("token");
                                
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
                                uint256_t token = account.get<uint256_t>("token");
                                
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
                            uint256_t hashTo = 0;
                            tx[nContract] >> hashTo;

                            /* Get the proof address. */
                            uint256_t hashProof = 0;
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
                            uint256_t token = account.get<uint256_t>("token");
                            
                            /* Check the account token matches the one passed in*/
                            if(token != hashToken)
                                continue;

                            /* Check owner that we are the owner of the recipient account  */
                            if(account.hashOwner != hashGenesis)
                                continue;

                            /* Add this amount to our total */
                            nUnconfirmed += nCredit;
                        } 
                    }
                }
            }

            return nUnconfirmed;
        }


        /*  Get the outstanding coinbases. */
        void GetImmature(const uint256_t& hashGenesis, uint64_t& nCoinbase, uint64_t& nCoinstake)
        {
            /* Reset amounts */
            nCoinbase = 0;
            nCoinstake = 0;

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
                        hashLast = tx.hashPrevTx;
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

                            /* Add it to our return values */
                            nCoinbase += nAmount;

                        }
                        else if(nOp == Operation::OP::TRUST || nOp == Operation::OP::GENESIS)
                        {
                            /* Get the amount */
                            TAO::Register::Unpack(tx[nContract], nAmount);

                            /* Add it to our return values */
                            nCoinstake += nAmount;
                        }
                    }

                    /* Set the next last. */
                    hashLast = tx.hashPrevTx;
                }
            }
        }



    } // End API namespace
} // End TAO namespace
