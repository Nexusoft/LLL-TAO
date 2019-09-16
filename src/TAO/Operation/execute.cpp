/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Man often becomes what he believes himself to be." - Mahatma Gandhi

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <Legacy/include/trust.h>
#include <Legacy/types/script.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/append.h>
#include <TAO/Operation/include/claim.h>
#include <TAO/Operation/include/coinbase.h>
#include <TAO/Operation/include/constants.h>
#include <TAO/Operation/include/create.h>
#include <TAO/Operation/include/credit.h>
#include <TAO/Operation/include/debit.h>
#include <TAO/Operation/include/fee.h>
#include <TAO/Operation/include/genesis.h>
#include <TAO/Operation/include/legacy.h>
#include <TAO/Operation/include/migrate.h>
#include <TAO/Operation/include/stake.h>
#include <TAO/Operation/include/transfer.h>
#include <TAO/Operation/include/trust.h>
#include <TAO/Operation/include/unstake.h>
#include <TAO/Operation/include/validate.h>
#include <TAO/Operation/include/write.h>

#include <TAO/Operation/types/contract.h>
#include <TAO/Operation/types/condition.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/constants.h>
#include <TAO/Register/types/object.h>
#include <TAO/Register/types/address.h>

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
            bool fValidate = false;
            try
            {
                /* Get the contract OP. */
                uint8_t nOP = 0;
                contract >> nOP;

                /* Check the current opcode. */
                switch(nOP)
                {
                    /* Condition that allows a validation to occur. */
                    case OP::CONDITION:
                    {
                        /* Condition has no parameters. */
                        contract >> nOP;

                        /* Check for valid primitives that can have a condition. */
                        switch(nOP)
                        {
                            /* Transfer and debit are the only permitted. */
                            case OP::TRANSFER:
                            case OP::DEBIT:
                            {
                                //transfer and debit are permitted
                                break;
                            }

                            default:
                                return debug::error(FUNCTION, "unsupported OP for condition");
                        }

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

                        /* Get next OP. */
                        contract >> nOP;

                        /* Set validate flag. */
                        fValidate = true;

                        break;
                    }
                }


                /* Check the current opcode after checking for conditions or validation. */
                switch(nOP)
                {

                    /* Generate pre-state to database. */
                    case OP::WRITE:
                    {
                        /* Make sure there are no conditions. */
                        if(!contract.Empty(Contract::CONDITIONS))
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
                        if(!contract.Empty(Contract::CONDITIONS))
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

                        /* Check for maximum register size. */
                        if(state.GetState().size() > TAO::Register::MAX_REGISTER_SIZE)
                            return debug::error(FUNCTION, "OP::APPEND: register size out of bounds ", vchData.size());

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


                    /* This does not contain any prestates. */
                    case OP::CREATE:
                    {
                        /* Make sure there are no conditions. */
                        if(!contract.Empty(Contract::CONDITIONS))
                            return debug::error(FUNCTION, "OP::CREATE: conditions not allowed on create");

                        /* Verify the operation rules. */
                        if(!Create::Verify(contract))
                            return false;

                        /* Get the Address of the Register. */
                        TAO::Register::Address hashAddress;
                        contract >> hashAddress;

                        /* Get the Register Type. */
                        uint8_t nType = 0;
                        contract >> nType;

                        /* Get the register data. */
                        std::vector<uint8_t> vchData;
                        contract >> vchData;

                        /* Check for maximum register size. */
                        if(vchData.size() > TAO::Register::MAX_REGISTER_SIZE)
                            return debug::error(FUNCTION, "OP::CREATE: register size out of bounds ", vchData.size());

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

                        /* Check for the fees. */
                        uint64_t nCost = 0;

                        /* Commit the register to disk. */
                        if(!Create::Commit(state, hashAddress, nCost, nFlags))
                            return false;

                        /* Set the fee cost to the contract. */
                        contract.AddCost(nCost);

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

                        /* Read the force transfer flag */
                        uint8_t nType = 0;
                        contract >> nType;

                        /* Register custody in SYSTEM ownership until claimed, unless the ForceTransfer flag has been set */
                        uint256_t hashNewOwner = (nType == TRANSFER::FORCE ? hashTransfer : 0);

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
                        if(!Transfer::Execute(state, hashNewOwner, contract.Timestamp()))
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
                        if(!contract.Empty(Contract::CONDITIONS))
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
                        if(!transfer.Empty(Contract::CONDITIONS))
                        {
                            /* Get the condition. */
                            uint8_t nType = 0;
                            transfer >> nType;

                            /* Check for condition. */
                            Condition conditions = Condition(transfer, contract);
                            if(nType == OP::CONDITION)
                            {
                                /* Read the contract database. */
                                uint256_t hashValidator = 0;
                                if(LLD::Contract->ReadContract(std::make_pair(hashTx, nContract), hashValidator, nFlags))
                                {
                                    /* Check that the caller is the claimant. */
                                    if(hashValidator != contract.Caller())
                                        return debug::error(FUNCTION, "OP::CLAIM: caller is not authorized to claim validation");
                                }

                                /* If no validate fulfilled, try to exeucte conditions. */
                                else if(!conditions.Execute())
                                    return debug::error(FUNCTION, "OP::CLAIM: conditions not satisfied");
                            }
                            else if(!conditions.Execute())
                                return debug::error(FUNCTION, "OP::CLAIM: conditions not satisfied");

                            /* Assess the fees for the computation limits. */
                            if(conditions.nCost > CONDITION_LIMIT_FREE)
                                contract.AddCost(conditions.nCost - CONDITION_LIMIT_FREE);
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
                        /* Check for validate. */
                        if(fValidate)
                            return debug::error(FUNCTION, "OP::COINBASE: cannot use OP::VALIDATE with coinbase");

                        /* Check for valid coinbase. */
                        if(!Coinbase::Verify(contract))
                            return false;

                        /* Get the genesis. */
                        uint256_t hashGenesis;
                        contract >> hashGenesis;

                        /* Seek to end. */
                        contract.Seek(16);

                        /* Commit to disk. */
                        if(contract.Caller() != hashGenesis && !Coinbase::Commit(hashGenesis, contract.Hash(), nFlags))
                            return false;

                        break;
                    }


                    /* Coinstake operation. Requires an account. */
                    case OP::TRUST:
                    {
                        /* Check for validate. */
                        if(fValidate)
                            return debug::error(FUNCTION, "OP::TRUST: cannot use OP::VALIDATE with trust");

                        /* Make sure there are no conditions. */
                        if(!contract.Empty(Contract::CONDITIONS))
                            return debug::error(FUNCTION, "OP::TRUST: conditions not allowed on trust");

                        /* Verify the operation rules. */
                        if(!Trust::Verify(contract))
                            return false;

                        /* Seek to scores. */
                        contract.Seek(64);

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
                        /* Check for validate. */
                        if(fValidate)
                            return debug::error(FUNCTION, "OP::GENESIS: cannot use OP::VALIDATE with genesis");

                        /* Make sure there are no conditions. */
                        if(!contract.Empty(Contract::CONDITIONS))
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
                        /* Check for validate. */
                        if(fValidate)
                            return debug::error(FUNCTION, "OP::STAKE: cannot use OP::VALIDATE with stake");

                        /* Make sure there are no conditions. */
                        if(!contract.Empty(Contract::CONDITIONS))
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
                        /* Check for validate. */
                        if(fValidate)
                            return debug::error(FUNCTION, "OP::UNSTAKE: cannot use OP::VALIDATE with unstake");

                        /* Make sure there are no conditions. */
                        if(!contract.Empty(Contract::CONDITIONS))
                            return debug::error(FUNCTION, "OP::UNSTAKE: conditions not allowed on unstake");

                        /* Verify the operation rules. */
                        if(!Unstake::Verify(contract))
                            return false;

                        contract.Reset();

                        contract.Seek(1);

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

                        /* Get the reference. */
                        uint64_t nReference = 0;
                        contract >> nReference;

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
                        if(!contract.Empty(Contract::CONDITIONS))
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
                        if(!debit.Empty(Contract::CONDITIONS))
                        {
                            /* Get the condition. */
                            uint8_t nType = 0;
                            debit >> nType;

                            /* Check for condition. */
                            Condition conditions = Condition(debit, contract);
                            if(nType == OP::CONDITION)
                            {
                                /* Read the contract database. */
                                uint256_t hashValidator = 0;
                                if(LLD::Contract->ReadContract(std::make_pair(hashTx, nContract), hashValidator, nFlags))
                                {
                                    /* Check that the caller is the claimant. */
                                    if(hashValidator != contract.Caller())
                                        return debug::error(FUNCTION, "OP::CREDIT: caller is not authorized to claim validation");
                                }

                                /* If no validate fulfilled, try to exeucte conditions. */
                                else if(!conditions.Execute())
                                    return debug::error(FUNCTION, "OP::CREDIT: conditions not satisfied");
                            }
                            else if(!conditions.Execute())
                                return debug::error(FUNCTION, "OP::CREDIT: conditions not satisfied");

                            /* Assess the fees for the computation limits. */
                            if(conditions.nCost > CONDITION_LIMIT_FREE)
                                contract.AddCost(conditions.nCost - CONDITION_LIMIT_FREE);
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


                    /* Migrate a trust key to a trust account register. */
                    case OP::MIGRATE:
                    {
                        /* Make sure there are no conditions. */
                        if(!contract.Empty(Contract::CONDITIONS))
                            return debug::error(FUNCTION, "OP::MIGRATE: conditions not allowed on migrate");

                        /* Extract the transaction from contract. */
                        uint512_t hashTx = 0;
                        contract >> hashTx;

                        /* Retrieve a debit for the Legacy tx output. Migrate tx will only have one output (index 0) */
                        Contract debit = LLD::Ledger->ReadContract(hashTx, 0);

                        /* Add migrate data from Legacy tx to debit (base ReadContract returns generic Legacy send to register) */
                        if(!::Legacy::BuildMigrateDebit(debit, hashTx))
                            return false;

                        /* Verify the operation rules. */
                        if(!Migrate::Verify(contract, debit))
                            return false;

                        /* After Verify, reset streams */
                        contract.Reset();
                        contract.Seek(65);

                        /* Get the trust register address. (hash to) */
                        TAO::Register::Address hashAccount;
                        contract >> hashAccount;

                        /* Get the Legacy trust key hash (hash from) */
                        uint512_t hashKey = 0;
                        contract >> hashKey;

                        /* Get the amount to migrate. */
                        uint64_t nAmount = 0;
                        contract >> nAmount;

                        /* Get the trust score to migrate. */
                        uint32_t nScore = 0;
                        contract >> nScore;

                        /* Get the hash last stake. */
                        uint512_t hashLast = 0;
                        contract >> hashLast;

                        /* Deserialize the pre-state byte from the contract. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::MIGRATE: register pre-state doesn't exist");

                        /* Retrieve the register pre-state. */
                        TAO::Register::Object object;
                        contract >>= object;

                        /* Calculate the new operation. */
                        if(!Migrate::Execute(object, nAmount, nScore, contract.Timestamp()))
                            return false;

                        /* Deserialize the post-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::POSTSTATE)
                            return debug::error(FUNCTION, "OP::MIGRATE: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != object.GetHash())
                            return debug::error(FUNCTION, "OP::MIGRATE: invalid register post-state");

                        /* Commit the migration updates to disk. */
                        if(!Migrate::Commit(object, hashAccount, contract.Caller(), hashTx, hashKey, hashLast, nFlags))
                            return false;

                        break;
                    }


                    /* Debit tokens from an account you own. */
                    case OP::FEE:
                    {
                        /* Make sure there are no conditions. */
                        if(!contract.Empty(Contract::CONDITIONS))
                            return debug::error(FUNCTION, "OP::FEE: conditions not allowed on fees");

                        /* Verify the operation rules. */
                        if(!Fee::Verify(contract))
                            return false;

                        /* Get the register address. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the fee amount. */
                        uint64_t  nFees = 0;
                        contract >> nFees;

                        /* Deserialize the pre-state byte from the contract. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::FEE: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::Object object;
                        contract >>= object;

                        /* Calculate the new operation. */
                        if(!Fee::Execute(object, nFees, contract.Timestamp()))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::STATES::POSTSTATE)
                            return debug::error(FUNCTION, "OP::FEE: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != object.GetHash())
                            return debug::error(FUNCTION, "OP::FEE: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!Fee::Commit(object, hashAddress, nFlags))
                            return false;

                        break;
                    }


                    /* Authorize is enabled in private mode only. */
                    case OP::AUTHORIZE:
                    {
                        //TODO: decide more details on the use of authorize.

                        /* Seek to address. */
                        contract.Seek(96);

                        break;
                    }


                    /* Create unspendable legacy script, that acts to debit from the account and make this unspendable. */
                    case OP::LEGACY:
                    {
                        /* Make sure there are no conditions. */
                        if(!contract.Empty(Contract::CONDITIONS))
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
                    return debug::error(FUNCTION, "contract can only have one PRIMITIVE");
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
