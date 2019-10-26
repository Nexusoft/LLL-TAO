/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/constants.h>
#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/rollback.h>
#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /* Verify the pre-states of a register to current network state. */
        bool Rollback(const TAO::Operation::Contract& contract, const uint8_t nFlags)
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

                    /* Condition that allows a validation to occur. */
                    case TAO::Operation::OP::CONDITION:
                    {
                        /* Condition has no parameters. */
                        contract >> nOP;

                        /* Condition has no parameters. */
                        break;
                    }


                    /* Validate a previous contract's conditions */
                    case TAO::Operation::OP::VALIDATE:
                    {
                        /* Extract the transaction from contract. */
                        uint512_t hashTx = 0;
                        contract >> hashTx;

                        /* Extract the contract-id. */
                        uint32_t nContract = 0;
                        contract >> nContract;

                        /* Erase the contract validation record. */
                        if(!LLD::Contract->EraseContract(std::make_pair(hashTx, nContract)))
                            return debug::error(FUNCTION, "failed to erase validation contract");

                        /* Condition has no parameters. */
                        contract >> nOP;

                        break;
                    }
                }

                /* Check the current opcode. */
                switch(nOP)
                {

                    /* Check pre-state to database. */
                    case TAO::Operation::OP::WRITE:
                    {
                        /* Get the Address of the Register. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the object data size. */
                        uint32_t nSize = contract.ReadCompactSize(TAO::Operation::Contract::OPERATIONS);

                        /* Seek past state data. */
                        contract.Seek(nSize);

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::WRITE: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State state;
                        contract >>= state;

                        /* Write the register from database. */
                        if(!LLD::Register->WriteState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::WRITE: failed to rollback to pre-state");

                        return true;
                    }


                    /* Check pre-state to database. */
                    case TAO::Operation::OP::APPEND:
                    {
                        /* Get the Address of the Register. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the object data size. */
                        uint32_t nSize = contract.ReadCompactSize(TAO::Operation::Contract::OPERATIONS);

                        /* Seek past state data. */
                        contract.Seek(nSize);

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::APPEND: egister state not in pre-state");

                        /* Verify the register's prestate. */
                        State state;
                        contract >>= state;

                        /* Write the register from database. */
                        if(!LLD::Register->WriteState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::APPEND: failed to rollback to pre-state");

                        return true;
                    }


                    /* Delete the state on rollback. */
                    case TAO::Operation::OP::CREATE:
                    {
                        /* Get the address of the Register. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Unpack the register type. */
                        uint8_t nType = 0;
                        contract >> nType;

                        /* Get the object data size. */
                        uint32_t nSize = contract.ReadCompactSize(TAO::Operation::Contract::OPERATIONS);

                        /* Seek past state data. */
                        contract.Seek(nSize);

                        /* Erase the register from database. */
                        if(!LLD::Register->EraseState(hashAddress, nFlags))
                            return debug::error(FUNCTION, "OP::CREATE: failed to erase post-state");

                        break;
                    }


                    /* Transfer ownership of a register to another signature chain. */
                    case TAO::Operation::OP::TRANSFER:
                    {
                        /* Get the Address of the Register. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Seek to end. */
                        uint256_t hashTransfer = 0;
                        contract >> hashTransfer;

                        /* Seek past flag. */
                        contract.Seek(1);

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::TRANSFER: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State state;
                        contract >>= state;

                        /* Write the register from database. */
                        if(!LLD::Register->WriteState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::TRANSFER: failed to rollback to pre-state");

                        /* Write the event to the ledger database. */
                        if(nFlags == TAO::Ledger::FLAGS::BLOCK && hashTransfer != WILDCARD_ADDRESS && !LLD::Ledger->EraseEvent(hashTransfer))
                            return debug::error(FUNCTION, "OP::TRANSFER: failed to rollback event");

                        break;
                    }


                    /* Transfer ownership of a register to another signature chain. */
                    case TAO::Operation::OP::CLAIM:
                    {
                        /* The transaction that this transfer is claiming. */
                        uint512_t hashTx = 0;
                        contract >> hashTx;

                        /* Get the contract number. */
                        uint32_t nContract = 0;
                        contract >> nContract;

                        /* Get the address to update. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Erase the proof. */
                        if(!LLD::Ledger->EraseProof(hashAddress, hashTx, nContract, nFlags))
                            return debug::error(FUNCTION, "OP::CLAIM: failed to erase claim proof");

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::CLAIM: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State state;
                        contract >>= state;

                        /* Write the register from database. */
                        if(!LLD::Register->WriteState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::CLAIM: failed to rollback to pre-state");

                        break;
                    }


                    /* Coinbase operation. Creates an account if none exists. */
                    case TAO::Operation::OP::COINBASE:
                    {
                        /* Get the genesis. */
                        uint256_t hashGenesis;
                        contract >> hashGenesis;

                        /* Seek to end. */
                        contract.Seek(16);

                        /* Commit to disk. */
                        if(nFlags == TAO::Ledger::FLAGS::BLOCK && contract.Caller() != hashGenesis && !LLD::Ledger->EraseEvent(hashGenesis))
                            return false;

                        break;
                    }


                    /* Coinstake operation. Requires an account. */
                    case TAO::Operation::OP::TRUST:
                    {
                        /* Seek to end. */
                        contract.Seek(88);

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::TRUST: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State state;
                        contract >>= state;

                        /* Write the register prestate to database. */
                        if(!LLD::Register->WriteTrust(state.hashOwner, state))
                            return debug::error(FUNCTION, "OP::TRUST: failed to rollback to pre-state");

                        break;
                    }


                    /* Coinstake operation. Requires an account. */
                    case TAO::Operation::OP::GENESIS:
                    {
                        /* Seek to end. */
                        contract.Seek(8);

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::GENESIS: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State state;
                        contract >>= state;

                        /* Get trust account addresses for owner (caller = hashOwner was verified by op, can use either) */
                        uint256_t hashAddress =
                            TAO::Register::Address(std::string("trust"), state.hashOwner, TAO::Register::Address::TRUST);

                        /* Write the register prestate to database. */
                        if(!LLD::Register->WriteState(hashAddress, state))
                            return debug::error(FUNCTION, "OP::GENESIS: failed to rollback to pre-state");

                        /* Erase the trust index. */
                        if(!LLD::Register->EraseTrust(contract.Caller()))
                            return debug::error(FUNCTION, "OP::GENESIS: failed to erase trust index");

                        break;
                    }


                    /* Debit tokens from an account you own. */
                    case TAO::Operation::OP::DEBIT:
                    {
                        /* Get last trust block. */
                        uint256_t hashFrom = 0;
                        contract >> hashFrom;

                        /* Get last trust block. */
                        uint256_t hashTo = 0;
                        contract >> hashTo;

                        /* Seek to end. */
                        contract.Seek(16);

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::DEBIT: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State state;
                        contract >>= state;

                        /* Write the register from database. */
                        if(!LLD::Register->WriteState(hashFrom, state, nFlags))
                            return debug::error(FUNCTION, "OP::DEBIT: failed to rollback to pre-state");

                        /* Read the owner of register. */
                        TAO::Register::State stateTo;
                        if(!LLD::Register->ReadState(hashTo, stateTo, nFlags))
                            return debug::error(FUNCTION, "failed to read register to");

                        /* Write the event to the ledger database. */
                        if(nFlags == TAO::Ledger::FLAGS::BLOCK && hashTo != WILDCARD_ADDRESS && !LLD::Ledger->EraseEvent(stateTo.hashOwner))
                            return debug::error(FUNCTION, "OP::DEBIT: failed to rollback event");

                        break;
                    }


                    /* Credit tokens to an account you own. */
                    case TAO::Operation::OP::CREDIT:
                    {
                        /* Extract the transaction from contract. */
                        uint512_t hashTx = 0;
                        contract >> hashTx;

                        /* Extract the contract-id. */
                        uint32_t nContract = 0;
                        contract >> nContract;

                        /* Get the transfer address. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the proof address. */
                        uint256_t hashProof = 0;
                        contract >> hashProof;

                        /* Get the transfer amount. */
                        uint64_t  nAmount = 0;
                        contract >> nAmount;

                        debug::log(0, FUNCTION, "OP::CREDIT: Erasing Proof ", hashProof.SubString(), " txid ", hashTx.SubString(), " contract ", nContract);

                        /* Write the claimed proof. */
                        if(!LLD::Ledger->EraseProof(hashProof, hashTx, nContract, nFlags))
                            return debug::error(FUNCTION, "OP::CREDIT: failed to erase credit proof");

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::CREDIT: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State state;
                        contract >>= state;

                        /* Write the register from database. */
                        if(!LLD::Register->WriteState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::CREDIT: failed to rollback to pre-state");

                        /* Read the debit. */
                        const TAO::Operation::Contract debit = LLD::Ledger->ReadContract(hashTx, nContract);

                        /* Check for non coinbase. */
                        uint8_t nDebit = 0;
                        debit >> nDebit;
                        if(nDebit == TAO::Operation::OP::DEBIT)
                        {
                            /* Get address from. */
                            uint256_t hashFrom = 0;
                            debit  >> hashFrom;

                            /* Get the to address */
                            TAO::Register::Address hashTo;
                            debit >> hashTo;

                            /* If the debit is to a tokenized asset, it could have partial credits. Therefore we need check the
                               credit amount with respect to the other claimed amounts and then subtract this credit from the
                               current claimed amount. */
                            if(hashTo.IsObject())
                            {
                                /* Get the partial amount. */
                                uint64_t nClaimed = 0;
                                if(!LLD::Ledger->ReadClaimed(hashTx, nContract, nClaimed, nFlags))
                                    return debug::error(FUNCTION, "OP::CREDIT: failed to read claimed amount");

                                /* Sanity check for claimed overflow. */
                                if(nClaimed < nAmount)
                                    return debug::error(FUNCTION, "OP::CREDIT: amount larger than claimed (overflow)");

                                debug::log(0, FUNCTION, "OP::CREDIT: Writing Partial ", (nClaimed - nAmount), " ",
                                    hashProof.SubString(), " txid ", hashTx.SubString(), " contract ", nContract);

                                /* Write the new claimed amount. */
                                if(!LLD::Ledger->WriteClaimed(hashTx, nContract, (nClaimed - nAmount), nFlags))
                                    return debug::error(FUNCTION, "OP::CREDIT: failed to rollback claimed amount");
                            }
                        }

                        break;
                    }

                    /* Migrate a trust key to a trust account register. */
                    case TAO::Operation::OP::MIGRATE:
                    {
                        /* Extract the transaction from contract. */
                        uint512_t hashTx;
                        contract >> hashTx;

                        /* Get the trust register address. (hash to) */
                        TAO::Register::Address hashAccount;
                        contract >> hashAccount;

                        /* Get the Legacy trust key hash (hash from) */
                        uint576_t hashTrust;
                        contract >> hashTrust;

                        /* Seek to end. */
                        contract.Seek(76);

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::MIGRATE: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State state;
                        contract >>= state;

                        /* Write the register prestate to database. */
                        if(!LLD::Register->WriteState(hashAccount, state, nFlags))
                            return debug::error(FUNCTION, "OP::MIGRATE: failed to rollback to pre-state");

                        /* Erase the migrate proof. */
                        if(!LLD::Ledger->EraseProof(TAO::Register::WILDCARD_ADDRESS, hashTx, 0, nFlags))
                            return debug::error(FUNCTION, "OP::MIGRATE: failed to erase migrate proof");

                        /* Erase the trust index. */
                        if(!LLD::Register->EraseTrust(contract.Caller()))
                            return debug::error(FUNCTION, "OP::MIGRATE: failed to erase trust index");

                        /* Erase the last stake. */
                        if(!LLD::Ledger->EraseStake(contract.Caller()))
                            return debug::error(FUNCTION, "OP::MIGRATE: failed to erase last stake");

                        /* Erase the trust key conversion. */
                        if(!LLD::Legacy->EraseTrustConversion(hashTrust))
                            return debug::error(FUNCTION, "OP::MIGRATE: failed to erase trust key migration from disk");

                        break;
                    }

                    /* Debit tokens from an account you own for fees. */
                    case TAO::Operation::OP::FEE:
                    {
                        /* Get account from block. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Seek to end. */
                        contract.Seek(8);

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::FEE: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State state;
                        contract >>= state;

                        /* Write the register from database. */
                        if(!LLD::Register->WriteState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::FEE: failed to rollback to pre-state");

                        break;
                    }


                    /* Authorize is enabled in private mode only. */
                    case TAO::Operation::OP::AUTHORIZE:
                    {
                        /* Seek to address. */
                        contract.Seek(96);

                        break;
                    }


                    /* Debit tokens from an account you own. */
                    case TAO::Operation::OP::LEGACY:
                    {
                        /* Get account from block. */
                        uint256_t hashFrom = 0;
                        contract >> hashFrom;

                        /* Skip amount */
                        contract.Seek(8);

                        /* Extract the script data. (move to end) */
                        ::Legacy::Script script;
                        contract >> script;

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::LEGACY: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State state;
                        contract >>= state;

                        /* Write the register from database. */
                        if(!LLD::Register->WriteState(hashFrom, state, nFlags))
                            return debug::error(FUNCTION, "OP::LEGACY: failed to rollback to pre-state");

                        break;
                    }

                    default:
                        return debug::error(FUNCTION, "invalid code for contract execution");

                }

                /* Check for end of stream. */
                if(!contract.End())
                    return debug::error(FUNCTION, "can only have one PRIMITIVE per contract");
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
