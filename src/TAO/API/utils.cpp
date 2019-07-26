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

#include <TAO/Operation/types/stream.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/create.h>
#include <TAO/Register/include/names.h>

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
                if(!LLD::Register->ReadState(hashFeeAccount, object))
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


    } // End API namespace
} // End TAO namespace
