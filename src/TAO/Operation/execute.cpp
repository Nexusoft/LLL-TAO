/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Man often becomes what he believes himself to be." - Mahatma Gandhi

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <Legacy/types/script.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/append.h>
#include <TAO/Operation/include/claim.h>
#include <TAO/Operation/include/create.h>
#include <TAO/Operation/include/credit.h>
#include <TAO/Operation/include/debit.h>
#include <TAO/Operation/include/genesis.h>
#include <TAO/Operation/include/legacy.h>
#include <TAO/Operation/include/stake.h>
#include <TAO/Operation/include/transfer.h>
#include <TAO/Operation/include/trust.h>
#include <TAO/Operation/include/unstake.h>
#include <TAO/Operation/include/validate.h>
#include <TAO/Operation/include/write.h>

#include <TAO/Operation/types/contract.h>
#include <TAO/Operation/types/condition.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>

namespace TAO
{

    namespace Operation
    {

        /* Executes a given operation byte sequence. */
        bool Execute(const Contract& contract, const uint8_t nFlags)
        {
            /* Reset the contract streams. */
            contract.Reset();

            /* Make sure no exceptions are thrown. */
            try
            {
                /* Get the contract OP. */
                uint8_t nOP = 0;
                contract >> nOP;

                /* Check the current opcode. */
                switch(nOP)
                {

                    /* Generate pre-state to database. */
                    case OP::WRITE:
                    {
                        /* Make sure there are no conditions. */
                        if(contract.Conditions())
                            return debug::error(FUNCTION, "OP::WRITE: conditions not allowed on write");

                        /* Verify the operation rules. */
                        if(!Write::Verify(contract))
                            return false;

                        /* Get the Address of the Register. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the state data. */
                        std::vector<uint8_t> vchData;
                        contract >> vchData;

                        /* Deserialize the pre-state byte from the contract. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::WRITE: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::State state;
                        contract >>= state;

                        /* Calculate the new operation. */
                        if(!Write::Execute(state, vchData, contract.Timestamp()))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::POSTSTATE)
                            return debug::error(FUNCTION, "OP::WRITE: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != state.GetHash())
                            return debug::error(FUNCTION, "OP::WRITE: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!Write::Commit(state, hashAddress, nFlags))
                            return false;

                        break;
                    }


                    /* Generate pre-state to database. */
                    case OP::APPEND:
                    {
                        /* Make sure there are no conditions. */
                        if(contract.Conditions())
                            return debug::error(FUNCTION, "OP::APPEND: conditions not allowed on append");

                        /* Verify the operation rules. */
                        if(!Append::Verify(contract))
                            return false;

                        /* Get the Address of the Register. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the state data. */
                        std::vector<uint8_t> vchData;
                        contract >> vchData;

                        /* Deserialize the pre-state byte from the contract. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::APPEND: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::State state;
                        contract >>= state;

                        /* Calculate the new operation. */
                        if(!Append::Execute(state, vchData, contract.Timestamp()))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::POSTSTATE)
                            return debug::error(FUNCTION, "OP::APPEND: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != state.GetHash())
                            return debug::error(FUNCTION, "OP::APPEND: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!Append::Commit(state, hashAddress, nFlags))
                            return false;

                        break;
                    }


                    /*
                     * This does not contain any prestates.
                     */
                    case OP::CREATE:
                    {
                        /* Make sure there are no conditions. */
                        if(contract.Conditions())
                            return debug::error(FUNCTION, "OP::CREATE: conditions not allowed on create");

                        /* Verify the operation rules. */
                        if(!Create::Verify(contract))
                            return false;

                        /* Get the Address of the Register. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the Register Type. */
                        uint8_t nType = 0;
                        contract >> nType;

                        /* Get the register data. */
                        std::vector<uint8_t> vchData;
                        contract >> vchData;

                        /* Create the register object. */
                        TAO::Register::State state;
                        state.nVersion   = 1;
                        state.nType      = nType;
                        state.hashOwner  = contract.Caller();

                        /* Calculate the new operation. */
                        if(!Create::Execute(state, vchData, contract.Timestamp()))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::POSTSTATE)
                            return debug::error(FUNCTION, "OP::CREATE: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != state.GetHash())
                            return debug::error(FUNCTION, "OP::CREATE: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!Create::Commit(state, hashAddress, nFlags))
                            return false;

                        break;
                    }


                    /* Transfer ownership of a register to another signature chain. */
                    case OP::TRANSFER:
                    {
                        /* Verify the operation rules. */
                        if(!Transfer::Verify(contract))
                            return false;

                        /* Extract the address from the contract. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Read the register transfer recipient. */
                        uint256_t hashTransfer = 0;
                        contract >> hashTransfer;

                        /* Deserialize the pre-state byte from the contract. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::TRANSFER: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::State state;
                        contract >>= state;

                        /* Calculate the new operation. */
                        if(!Transfer::Execute(state, hashTransfer, contract.Timestamp()))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::POSTSTATE)
                            return debug::error(FUNCTION, "OP::TRANSFER: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != state.GetHash())
                            return debug::error(FUNCTION, "OP::TRANSFER: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!Transfer::Commit(state, contract.Hash(), hashAddress, hashTransfer, nFlags))
                            return false;

                        break;
                    }


                    /* Transfer ownership of a register to another signature chain. */
                    case OP::CLAIM:
                    {
                        /* Make sure there are no conditions. */
                        if(contract.Conditions())
                            return debug::error(FUNCTION, "OP::CLAIM: conditions not allowed on claim");

                        /* Extract the transaction from contract. */
                        uint512_t hashTx = 0;
                        contract >> hashTx;

                        /* Extract the contract-id. */
                        uint32_t nContract = 0;
                        contract >> nContract;

                        /* Extract the address from the contract. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Verify the operation rules. */
                        const Contract transfer = LLD::Ledger->ReadContract(hashTx, nContract);
                        if(!Claim::Verify(contract, transfer))
                            return false;

                        /* Check for conditions. */
                        if(transfer.Conditions())
                        {
                            /* Build the validation script for execution. */
                            Condition conditions = Condition(transfer, contract);
                            if(!conditions.Execute())
                            {
                                /* Seek the debit to end. */
                                transfer.Seek(65);

                                /* Check for condition operation. */
                                if(!transfer.End())
                                {
                                    /* Get the condition. */
                                    uint8_t nType = 0;
                                    transfer >> nType;

                                    /* Check for condition. */
                                    if(nType != OP::CONDITION)
                                        return debug::error(FUNCTION, "OP::CLAIM: condition OP missing");

                                    /* Read the contract database. */
                                    uint256_t hashValidator = 0;
                                    if(!LLD::Contract->ReadContract(std::make_pair(hashTx, nContract), hashValidator, nFlags))
                                        return debug::error(FUNCTION, "OP::CLAIM: condition has not been validated");

                                    /* Check the validator to caller. */
                                    if(hashValidator != contract.Caller())
                                        return debug::error(FUNCTION, "OP::CLAIM: caller is not authorized to claim validation");
                                }
                                else
                                    return debug::error(FUNCTION, "OP::CLAIM: conditions not satisfied");
                            }
                        }

                        /* Check for conditions. */
                        if(transfer.Conditions())
                        {
                            /* Build the validation script for execution. */
                            Condition condition = Condition(transfer, contract);
                            if(!condition.Execute())
                                return debug::error(FUNCTION, "OP::CLAIM: conditions not satisfied");
                        }

                        /* Get the state byte. */
                        uint8_t nState = 0; //RESERVED
                        contract >>= nState;

                        /* Check for the pre-state. */
                        if(nState != TAO::Register::STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::CLAIM: register script not in pre-state");

                        /* Get the pre-state. */
                        TAO::Register::State state;
                        contract >>= state;

                        /* Calculate the new operation. */
                        if(!Claim::Execute(state, contract.Caller(), contract.Timestamp()))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::POSTSTATE)
                            return debug::error(FUNCTION, "OP::CLAIM: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != state.GetHash())
                            return debug::error(FUNCTION, "OP::CLAIM: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!Claim::Commit(state, hashAddress, hashTx, nContract, nFlags))
                            return false;

                        break;
                    }


                    /* Coinbase operation. Creates an account if none exists. */
                    case OP::COINBASE:
                    {
                        /* Seek to end. */
                        contract.Seek(49);

                        break;
                    }


                    /* Coinstake operation. Requires an account. */
                    case OP::TRUST:
                    {
                        /* Make sure there are no conditions. */
                        if(contract.Conditions())
                            return debug::error(FUNCTION, "OP::TRUST: conditions not allowed on trust");

                        /* Verify the operation rules. */
                        if(!Trust::Verify(contract))
                            return false;

                        /* Seek to scores. */
                        contract.Seek(65);

                        /* Get the trust score. */
                        uint64_t nScore = 0;
                        contract >> nScore;

                        /* Get the stake reward. */
                        uint64_t nReward = 0;
                        contract >> nReward;

                        /* Deserialize the pre-state byte from the contract. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::TRUST: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::Object object;
                        contract >>= object;

                        /* Calculate the new operation. */
                        if(!Trust::Execute(object, nReward, nScore, contract.Timestamp()))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::POSTSTATE)
                            return debug::error(FUNCTION, "OP::TRUST: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != object.GetHash())
                            return debug::error(FUNCTION, "OP::TRUST: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!Trust::Commit(object, nFlags))
                            return false;

                        break;
                    }


                    /* Coinstake operation. Requires an account. */
                    case OP::GENESIS:
                    {
                        /* Make sure there are no conditions. */
                        if(contract.Conditions())
                            return debug::error(FUNCTION, "OP::GENESIS: conditions not allowed on genesis");

                        /* Verify the operation rules. */
                        if(!Genesis::Verify(contract))
                            return false;

                        /* Get last trust block. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the stake reward. */
                        uint64_t nReward = 0;
                        contract >> nReward;

                        /* Deserialize the pre-state byte from the contract. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::GENESIS: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::Object object;
                        contract >>= object;

                        /* Calculate the new operation. */
                        if(!Genesis::Execute(object, nReward, contract.Timestamp()))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::POSTSTATE)
                            return debug::error(FUNCTION, "OP::GENESIS: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != object.GetHash())
                            return debug::error(FUNCTION, "OP::GENESIS: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!Genesis::Commit(object, hashAddress, nFlags))
                            return false;

                        break;
                    }


                    /* Move funds from trust account balance to stake. */
                    case OP::STAKE:
                    {
                        /* Make sure there are no conditions. */
                        if(contract.Conditions())
                            return debug::error(FUNCTION, "OP::STAKE: conditions not allowed on stake");

                        /* Verify the operation rules. */
                        if(!Stake::Verify(contract))
                            return false;

                        /* Amount to of funds to move. */
                        uint64_t nAmount;
                        contract >> nAmount;

                        /* Deserialize the pre-state byte from the contract. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::STAKE: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::Object object;
                        contract >>= object;

                        /* Calculate the new operation. */
                        if(!Stake::Execute(object, nAmount, contract.Timestamp()))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::POSTSTATE)
                            return debug::error(FUNCTION, "OP::STAKE: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != object.GetHash())
                            return debug::error(FUNCTION, "OP::STAKE: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!Stake::Commit(object, nFlags))
                            return false;

                        break;
                    }


                    /* Move funds from trust account stake to balance. */
                    case OP::UNSTAKE:
                    {
                        /* Make sure there are no conditions. */
                        if(contract.Conditions())
                            return debug::error(FUNCTION, "OP::UNSTAKE: conditions not allowed on unstake");

                        /* Verify the operation rules. */
                        if(!Unstake::Verify(contract))
                            return false;

                        /* Amount of funds to move. */
                        uint64_t nAmount = 0;
                        contract >> nAmount;

                        /* Trust score penalty from unstake. */
                        uint64_t nPenalty = 0;
                        contract >> nPenalty;

                        /* Deserialize the pre-state byte from the contract. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::UNSTAKE: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::Object object;
                        contract >>= object;

                        /* Calculate the new operation. */
                        if(!Unstake::Execute(object, nAmount, nPenalty, contract.Timestamp()))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::POSTSTATE)
                            return debug::error(FUNCTION, "OP::UNSTAKE: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != object.GetHash())
                            return debug::error(FUNCTION, "OP::UNSTAKE: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!Unstake::Commit(object, nFlags))
                            return false;

                        break;
                    }


                    /* Debit tokens from an account you own. */
                    case OP::DEBIT:
                    {
                        /* Verify the operation rules. */
                        if(!Debit::Verify(contract))
                            return false;

                        /* Get the register address. */
                        uint256_t hashFrom = 0;
                        contract >> hashFrom;

                        /* Get the transfer address. */
                        uint256_t hashTo = 0;
                        contract >> hashTo;

                        /* Get the transfer amount. */
                        uint64_t  nAmount = 0;
                        contract >> nAmount;

                        /* Deserialize the pre-state byte from the contract. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::DEBIT: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::Object object;
                        contract >>= object;

                        /* Calculate the new operation. */
                        if(!Debit::Execute(object, nAmount, contract.Timestamp()))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::POSTSTATE)
                            return debug::error(FUNCTION, "OP::DEBIT: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != object.GetHash())
                            return debug::error(FUNCTION, "OP::DEBIT: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!Debit::Commit(object, contract.Hash(), hashFrom, hashTo, nFlags))
                            return false;

                        break;
                    }


                    /* Credit tokens to an account you own. */
                    case OP::CREDIT:
                    {
                        /* Make sure there are no conditions. */
                        if(contract.Conditions())
                            return debug::error(FUNCTION, "OP::CREDIT: conditions not allowed on credit");

                        /* Extract the transaction from contract. */
                        uint512_t hashTx = 0;
                        contract >> hashTx;

                        /* Extract the contract-id. */
                        uint32_t nContract = 0;
                        contract >> nContract;

                        /* Verify the operation rules. */
                        const Contract debit = LLD::Ledger->ReadContract(hashTx, nContract);
                        if(!Credit::Verify(contract, debit, nFlags))
                            return false;

                        /* Seek past transaction-id. */
                        contract.Seek(69);

                        /* Get the transfer address. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the transfer address. */
                        uint256_t hashProof = 0;
                        contract >> hashProof;

                        /* Get the transfer amount. */
                        uint64_t  nAmount = 0;
                        contract >> nAmount;

                        /* Check for conditions. */
                        if(debit.Conditions())
                        {

                            /* Seek the debit to end. */
                            debit.Seek(73);

                            /* Check for condition operation. */
                            Condition conditions = Condition(debit, contract);
                            if(!debit.End())
                            {

                                /* Get the condition. */
                                uint8_t nType = 0;
                                debit >> nType;

                                /* Check for condition. */
                                if(nType != OP::CONDITION)
                                    return debug::error(FUNCTION, "OP::CREDIT: condition OP missing");

                                /* Read the contract database. */
                                uint256_t hashValidator = 0;
                                if(!LLD::Contract->ReadContract(std::make_pair(hashTx, nContract), hashValidator, nFlags))
                                    return debug::error(FUNCTION, "OP::CREDIT: condition has not been validated");

                                /* Check the validator to caller. */
                                if(hashValidator != contract.Caller() && !conditions.Execute())
                                    return debug::error(FUNCTION, "OP::CREDIT: caller is not authorized to claim validation");
                            }
                            else if(!conditions.Execute())
                                return debug::error(FUNCTION, "OP::CREDIT: conditions not satisfied");

                        }

                        /* Deserialize the pre-state byte from the contract. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::CREDIT: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::Object object;
                        contract >>= object;

                        /* Calculate the new operation. */
                        if(!Credit::Execute(object, nAmount, contract.Timestamp()))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::POSTSTATE)
                            return debug::error(FUNCTION, "OP::CREDIT: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != object.GetHash())
                            return debug::error(FUNCTION, "OP::CREDIT: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!Credit::Commit(object, debit, hashAddress, hashProof, hashTx, nContract, nAmount, nFlags))
                            return false;

                        break;
                    }


                    /* Authorize is enabled in private mode only. */
                    case OP::AUTHORIZE:
                    {
                        /* Seek to address. */
                        contract.Seek(97);

                        break;
                    }


                    /* Create unspendable legacy script, that acts to debit from the account and make this unspendable. */
                    case OP::LEGACY:
                    {
                        /* Make sure there are no conditions. */
                        if(contract.Conditions())
                            return debug::error(FUNCTION, "OP::LEGACY: conditions not allowed on legacy");

                        /* Verify the operation rules. */
                        if(!Legacy::Verify(contract))
                            return false;

                        /* Get the register address. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the transfer amount. */
                        uint64_t  nAmount = 0;
                        contract >> nAmount;

                        /* Deserialize the pre-state byte from the contract. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::LEGACY: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::Object object;
                        contract >>= object;

                        /* Calculate the new operation. */
                        if(!Legacy::Execute(object, nAmount, contract.Timestamp()))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::POSTSTATE)
                            return debug::error(FUNCTION, "OP::LEGACY: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != object.GetHash())
                            return debug::error(FUNCTION, "OP::LEGACY: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!Legacy::Commit(object, hashAddress, nFlags))
                            return false;

                        /* Get the script data. */
                        ::Legacy::Script script;
                        contract >> script;

                        /* Check for OP_DUP OP_HASH256 <hash> OP_EQUALVERIFY. */

                        break;
                    }

                    default:
                        return debug::error(FUNCTION, "invalid code for contract execution ", uint32_t(nOP));
                }


                /* Check for end of stream. */
                if(!contract.End())
                {
                    /* Get the contract OP. */
                    uint8_t nLast = 0;
                    contract >> nLast;

                    /* Check the current opcode. */
                    switch(nLast)
                    {

                        /* Condition that allows a validation to occur. */
                        case OP::CONDITION:
                        {

                            /* Condition has no parameters. */
                            break;
                        }


                        /* Validate a previous contract's conditions */
                        case OP::VALIDATE:
                        {
                            /* Extract the transaction from contract. */
                            uint512_t hashTx = 0;
                            contract >> hashTx;

                            /* Extract the contract-id. */
                            uint32_t nContract = 0;
                            contract >> nContract;

                            /* Verify the operation rules. */
                            const Contract condition = LLD::Ledger->ReadContract(hashTx, nContract);
                            if(!Validate::Verify(contract, condition))
                                return false;

                            /* Commit the validation to disk. */
                            if(!Validate::Commit(hashTx, nContract, contract.Caller(), nFlags))
                                return false;

                            break;
                        }

                        default:
                            return debug::error(FUNCTION, "OP::CONDITIONS:  invalid end code for contract execution ", uint32_t(nLast));
                    }

                    /* Ensure that there are no more operations. */
                    if(!contract.End())
                        return debug::error(FUNCTION, "OP::CONDITIONS: contract cannot contain any more primitives");
                }
            }
            catch(const std::exception& e)
            {
                return debug::error(FUNCTION, "exception encountered ", e.what());
            }

            /* If nothing failed, return true for evaluation. */
            return true;
        }

    }
}
