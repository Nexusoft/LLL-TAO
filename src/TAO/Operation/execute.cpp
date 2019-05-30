/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Man often becomes what he believes himself to be." - Mahatma Gandhi

____________________________________________________________________________________________*/


#include <LLC/types/uint1024.h>

#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/types/stream.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/operations.h>

#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/types/transaction.h>

#include <Util/include/hex.h>

#include <new> /* std::bad_alloc */


namespace TAO
{

    namespace Operation
    {

        /* Executes a given operation byte sequence. */
        bool Execute(const Contract& contract, uint8_t nFlags)
        {
            /* Make sure no exceptions are thrown. */
            try
            {
                /* Get the contract OP. */
                uint8_t OP = 0;
                contract >> OP;

                /* Check the current opcode. */
                switch(OP)
                {

                    /* Generate pre-state to database. */
                    case TAO::Operation::OP::WRITE:
                    {
                        /* Verify the operation rules. */
                        if(!TAO::Operation::Write::Verify(contract))
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
                        if(nState != TAO::Register::FLAGS::PRESTATE)
                            return debug::error(FUNCTION, "OP::WRITE: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::State state;
                        contract >>= state;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Write::Execute(state, vchData, contract.nTimestamp))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::FLAGS::POSTSTATE)
                            return debug::error(FUNCTION, "OP::WRITE: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != state.GetHash())
                            return debug::error(FUNCTION, "OP::WRITE: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!TAO::Operation::Write::Commit(state, hashAddress, nFlags))
                            return debug::error(FUNCTION, "OP::WRITE: failed to write final state");

                        break;
                    }


                    /* Generate pre-state to database. */
                    case TAO::Operation::OP::APPEND:
                    {
                        /* Verify the operation rules. */
                        if(!TAO::Operation::Append::Verify(contract))
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
                        if(nState != TAO::Register::FLAGS::PRESTATE)
                            return debug::error(FUNCTION, "OP::APPEND: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::State state;
                        contract >>= state;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Append::Execute(state, vchData, contract.nTimestamp))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::FLAGS::POSTSTATE)
                            return debug::error(FUNCTION, "OP::APPEND: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != state.GetHash())
                            return debug::error(FUNCTION, "OP::APPEND: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!TAO::Operation::Append::Commit(state, hashAddress, nFlags))
                            return debug::error(FUNCTION, "OP::APPEND: failed to write final state");

                        break;
                    }


                    /*
                     * This does not contain any prestates.
                     */
                    case TAO::Operation::OP::CREATE:
                    {
                        /* Verify the operation rules. */
                        if(!TAO::Operation::Create::Verify(contract))
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
                        state.hashOwner  = contract.hashCaller;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Create::Execute(state, vchData, contract.nTimestamp))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::FLAGS::POSTSTATE)
                            return debug::error(FUNCTION, "OP::CREATE: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != state.GetHash())
                            return debug::error(FUNCTION, "OP::CREATE: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!TAO::Operation::Create::Commit(state, hashAddress, nFlags))
                            return debug::error(FUNCTION, "OP::CREATE: failed to write final state");

                        break;
                    }


                    /* Transfer ownership of a register to another signature chain. */
                    case TAO::Operation::OP::TRANSFER:
                    {
                        /* Verify the operation rules. */
                        if(!TAO::Operation::Transfer::Verify(contract))
                            return false;

                        /* Extract the address from the tx.ssOperation. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Read the register transfer recipient. */
                        uint256_t hashTransfer = 0;
                        contract >> hashTransfer;

                        /* Deserialize the pre-state byte from the contract. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::FLAGS::PRESTATE)
                            return debug::error(FUNCTION, "OP::TRANSFER: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::State state;
                        contract >>= state;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Transfer::Execute(state, hashTransfer, contract.nTimestamp))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::FLAGS::POSTSTATE)
                            return debug::error(FUNCTION, "OP::TRANSFER: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != state.GetHash())
                            return debug::error(FUNCTION, "OP::TRANSFER: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!TAO::Operation::Transfer::Commit(state, hashAddress, nFlags))
                            return debug::error(FUNCTION, "OP::TRANSFER: failed to write final state");

                        break;
                    }


                    /* Transfer ownership of a register to another signature chain. */
                    case TAO::Operation::OP::CLAIM:
                    {
                        /* Extract the transaction from contract. */
                        uint512_t hashTx = 0;
                        contract >> hashTx;

                        /* Extract the contract-id. */
                        uint32_t nContract = 0;
                        contract >> nContract;

                        /* Extract the address from the tx.ssOperation. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the previous tx. */
                        TAO::Ledger::Transaction tx;
                        if(!LLD::legDB->ReadTx(hashTx, tx))
                            return debug::error(FUNCTION, "OP::CLAIM: cannot read prev tx ", hashTx.SubString());

                        /* Verify the operation rules. */
                        if(!TAO::Operation::Claim::Verify(contract, tx[nContract]))
                            return false;

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
                        if(!TAO::Operation::Claim::Execute(state, contract.hashCaller, contract.nTimestamp))
                            return debug::error(FUNCTION, "OP::CLAIM: cannot generate post-state");

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::FLAGS::POSTSTATE)
                            return debug::error(FUNCTION, "OP::CLAIM: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != state.GetHash())
                            return debug::error(FUNCTION, "OP::CLAIM: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!TAO::Operation::Transfer::Commit(state, hashAddress, hashTx, nContract, nFlags))
                            return debug::error(FUNCTION, "OP::CLAIM: failed to write final state");

                        break;
                    }

                    /* Coinbase operation. Creates an account if none exists. */
                    case TAO::Operation::OP::COINBASE:
                    {
                        /* Seek to end. */
                        contract.Seek(40);

                        break;
                    }


                    /* Coinstake operation. Requires an account. */
                    case TAO::Operation::OP::TRUST:
                    {
                        /* Verify the operation rules. */
                        if(!TAO::Operation::Trust::Verify(contract))
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
                        if(nState != TAO::Register::FLAGS::PRESTATE)
                            return debug::error(FUNCTION, "OP::TRUST: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::State state;
                        contract >>= state;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Trust::Execute(state, nReward, nScore, contract.nTimestamp))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::FLAGS::POSTSTATE)
                            return debug::error(FUNCTION, "OP::TRUST: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != state.GetHash())
                            return debug::error(FUNCTION, "OP::TRUST: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!TAO::Operation::Trust::Commit(state, nFlags))
                            return debug::error(FUNCTION, "OP::TRUST: failed to write final state");

                        break;
                    }


                    /* Coinstake operation. Requires an account. */
                    case TAO::Operation::OP::GENESIS:
                    {
                        /* Verify the operation rules. */
                        if(!TAO::Operation::Genesis::Verify(contract))
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
                        if(nState != TAO::Register::FLAGS::PRESTATE)
                            return debug::error(FUNCTION, "OP::GENESIS: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::State state;
                        contract >>= state;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Genesis::Execute(state, nReward, contract.nTimestamp))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::FLAGS::POSTSTATE)
                            return debug::error(FUNCTION, "OP::GENESIS: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != state.GetHash())
                            return debug::error(FUNCTION, "OP::GENESIS: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!TAO::Operation::Genesis::Commit(state, hashAddress, nFlags))
                            return debug::error(FUNCTION, "OP::GENESIS: failed to write final state");

                        break;
                    }


                    /* Debit tokens from an account you own. */
                    case TAO::Operation::OP::DEBIT:
                    {
                        /* Verify the operation rules. */
                        if(!TAO::Operation::Debit::Verify(contract))
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
                        if(nState != TAO::Register::FLAGS::PRESTATE)
                            return debug::error(FUNCTION, "OP::DEBIT: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::Object object;
                        contract >>= object;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Debit::Execute(object, nAmount, contract.nTimestamp))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::FLAGS::POSTSTATE)
                            return debug::error(FUNCTION, "OP::DEBIT: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != object.GetHash())
                            return debug::error(FUNCTION, "OP::DEBIT: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!TAO::Operation::Debit::Commit(object, hashAddress, nFlags))
                            return debug::error(FUNCTION, "OP::DEBIT: failed to write final state");

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

                        /* Get the debit contract. */
                        const Contract debit = LLD::legDB->ReadContract(hashTx, nContract);

                        /* Verify the operation rules. */
                        if(!TAO::Operation::Credit::Verify(contract, debit, nFlags))
                            return false;

                        /* Seek past transaction-id. */
                        contract.Seek(68);

                        /* Get the transfer address. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the transfer address. */
                        uint256_t hashProof = 0;
                        contract >> hashProof;

                        /* Get the transfer amount. */
                        uint64_t  nAmount = 0;
                        contract >> nAmount;

                        /* Deserialize the pre-state byte from the contract. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::FLAGS::PRESTATE)
                            return debug::error(FUNCTION, "OP::CREDIT: register pre-state doesn't exist");

                        /* Read the register from database. */
                        TAO::Register::Object object;
                        contract >>= object;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Credit::Execute(object, nAmount, contract.nTimestamp))
                            return false;

                        /* Deserialize the pre-state byte from contract. */
                        nState = 0;
                        contract >>= nState;

                        /* Check for pre-state. */
                        if(nState != TAO::Register::FLAGS::POSTSTATE)
                            return debug::error(FUNCTION, "OP::CREDIT: register post-state doesn't exist");

                        /* Deserialize the checksum from contract. */
                        uint64_t nChecksum = 0;
                        contract >>= nChecksum;

                        /* Check the post-state to register state. */
                        if(nChecksum != object.GetHash())
                            return debug::error(FUNCTION, "OP::CREDIT: invalid register post-state");

                        /* Commit the register to disk. */
                        if(!TAO::Operation::Credit::Commit(object, hashAddress, hashProof, hashTx, nContract, nAmount, nFlags))
                            return debug::error(FUNCTION, "OP::CREDIT: failed to write final state");

                        break;
                    }


                    /* Authorize is enabled in private mode only. */
                    case TAO::Operation::OP::AUTHORIZE:
                    {
                        /* Seek to address. */
                        contract.Seek(96);

                        break;
                    }

                    default:
                        return debug::error(FUNCTION, "invalid code for register verification");
                }


                /* Check for end of stream. */
                if(!contract.End())
                {
                    /* Get the contract OP. */
                    OP = 0;
                    contract >> OP;

                    /* Check for OP::REQUIRE. */
                    if(OP != TAO::Operation::OP::REQUIRE && OP != TAO::Operation::OP::VALIDATE)
                        return debug::error(FUNCTION, "contract cannot contain second OP beyond REQUIRE or VALIDATE");
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
