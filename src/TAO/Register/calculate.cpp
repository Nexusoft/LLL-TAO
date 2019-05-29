/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/stream.h>
#include <TAO/Register/include/calculate.h>

#include <new> //std::bad_alloc

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /* Verify the pre-states of a register to current network state. */
        bool Calculate(Contract& contract)
        {
            /* Reset the contract streams. */
            contract.Reset();

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
                        /* Get the Address of the Register. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the state data. */
                        std::vector<uint8_t> vchData;
                        contract >> vchData;

                        /* Serialize the pre-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::PRESTATE);

                        /* Read the register from database. */
                        State state;
                        if(!LLD::regDB->ReadState(hashAddress, state, TAO::Register::FLAGS::PRESTATE))
                            return debug::error(FUNCTION, "OP::WRITE: register pre-state doesn't exist");

                        /* Check the ownership. */
                        if(state.hashOwner != contract.hashCaller)
                            return debug::error(FUNCTION, "OP::WRITE: cannot generate pre-state if not owner");

                        /* Serialize the pre-state into contract. */
                        contract <<= state;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Write::Execute(state, vchData, contract.nTimestamp))
                            return debug::error(FUNCTION, "OP::WRITE: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= state.GetHash();

                        break;
                    }


                    /* Generate pre-state to database. */
                    case TAO::Operation::OP::APPEND:
                    {
                        /* Get the Address of the Register. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the state data. */
                        std::vector<uint8_t> vchData;
                        contract >> vchData;

                        /* Serialize the pre-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::PRESTATE);

                        /* Read the register from database. */
                        State state;
                        if(!LLD::regDB->ReadState(hashAddress, state, TAO::Register::FLAGS::PRESTATE))
                            return debug::error(FUNCTION, "OP::APPEND: register pre-state doesn't exist");

                        /* Check the ownership. */
                        if(state.hashOwner != contract.hashCaller)
                            return debug::error(FUNCTION, "OP::APPEND: cannot generate pre-state if not owner");

                        /* Serialize the pre-state into contract. */
                        contract <<= state;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Append::Execute(state, vchData, contract.nTimestamp))
                            return debug::error(FUNCTION, "OP::APPEND: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= state.GetHash();

                        break;
                    }


                    /*
                     * This does not contain any prestates.
                     */
                    case TAO::Operation::OP::CREATE:
                    {
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
                        State state;
                        state.nVersion   = 1;
                        state.nType      = nType;
                        state.hashOwner  = contract.hashCaller;

                        /* Generate the post-state. */
                        if(!TAO::Operation::Create::Execute(state, vchData, contract.nTimestamp))
                            return debug::error(FUNCTION, "OP::CREATE: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= state.GetHash();                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= state.GetHash();

                        break;
                    }


                    /* Transfer ownership of a register to another signature chain. */
                    case TAO::Operation::OP::TRANSFER:
                    {
                        /* Extract the address from the tx.ssOperation. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Read the register transfer recipient. */
                        uint256_t hashTransfer = 0;
                        contract >> hashTransfer;

                        /* Serialize the pre-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::PRESTATE);

                        /* Read the register from database. */
                        State state;
                        if(!LLD::regDB->ReadState(hashAddress, state, TAO::Register::FLAGS::PRESTATE))
                            return debug::error(FUNCTION, "OP::TRANSFER: register pre-state doesn't exist");

                        /* Check the ownership. */
                        if(state.hashOwner != contract.hashCaller)
                            return debug::error(FUNCTION, "OP::TRANSFER: cannot generate pre-state if not owner");

                        /* Serialize the pre-state into contract. */
                        contract <<= state;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Transfer::Execute(state, hashTransfer, contract.nTimestamp))
                            return debug::error(FUNCTION, "OP::TRANSFER: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= state.GetHash();

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

                        /* Get the previous tx. */
                        TAO::Ledger::Transaction tx;
                        if(!LLD::legDB->ReadTx(hashTx, tx))
                            return debug::error(FUNCTION, "OP::CLAIM: cannot read prev tx ", hashTx.SubString());

                        /* Get the previous contract. */
                        const TAO::Operation::Contract& claim = tx[nContract];

                        /* Get the state byte. */
                        uint8_t nState = 0; //RESERVED
                        claim >>= nState;

                        /* Check for the pre-state. */
                        if(nState != TAO::Register::STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::CLAIM: register script not in pre-state");

                        /* Get the pre-state. */
                        TAO::Register::State state;
                        claim >>= state;

                        /* Set state to proper transfer position. */
                        state.hashOwner = 0;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Claim::Execute(state, contract.hashCaller, contract.nTimestamp))
                            return debug::error(FUNCTION, "OP::CLAIM: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= state.GetHash();

                        break;
                    }

                    /* Coinbase operation. Creates an account if none exists. */
                    case TAO::Operation::OP::COINBASE:
                    {
                        break;
                    }


                    /* Coinstake operation. Requires an account. */
                    case TAO::Operation::OP::TRUST:
                    {
                        /* Seek to scores. */
                        contract.Seek(64);

                        /* Get the trust score. */
                        uint64_t nScore = 0;
                        contract >> nScore;

                        /* Get the stake reward. */
                        uint64_t nReward = 0;
                        contract >> nReward;

                        /* Serialize the pre-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::PRESTATE);

                        /* Read the register from database. */
                        State state;
                        if(!LLD::regDB->ReadTrust(contract.hashCaller, state))
                            return debug::error(FUNCTION, "OP::TRUST: register pre-state doesn't exist");

                        /* Serialize the pre-state into contract. */
                        contract <<= state;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Trust::Execute(state, nReward, nScore, contract.nTimestamp))
                            return debug::error(FUNCTION, "OP::TRUST: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= state.GetHash();

                        break;
                    }


                    /* Coinstake operation. Requires an account. */
                    case TAO::Operation::OP::GENESIS:
                    {
                        /* Get last trust block. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the stake reward. */
                        uint64_t nReward = 0;
                        contract >> nReward;

                        /* Serialize the pre-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::PRESTATE);

                        /* Read the register from database. */
                        State state;
                        if(!LLD::regDB->ReadState(hashAddress, state))
                            return debug::error(FUNCTION, "OP::GENESIS: register pre-state doesn't exist");

                        /* Serialize the pre-state into contract. */
                        contract <<= state;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Genesis::Execute(state, nReward, contract.nTimestamp))
                            return debug::error(FUNCTION, "OP::GENESIS: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= state.GetHash();

                        break;
                    }


                    /* Debit tokens from an account you own. */
                    case TAO::Operation::OP::DEBIT:
                    {
                        uint256_t hashAddress;
                        tx.ssOperation >> hashAddress;

                        /* Verify the first register code. */
                        uint8_t nState;
                        tx.ssRegister  >> nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        tx.ssRegister  >> prestate;

                        /* Read the register from database. */
                        State dbstate;
                        if(!LLD::regDB->ReadState(hashAddress, dbstate, nFlags))
                            return debug::error(FUNCTION, "register pre-state doesn't exist");

                        /* Check the ownership. */
                        if(dbstate.hashOwner != tx.hashGenesis)
                            return debug::error(FUNCTION, "cannot generate pre-state if not owner");

                        /* Check the prestate to the dbstate. */
                        if(prestate != dbstate)
                            return debug::error(FUNCTION, "prestate and dbstate mismatch");

                        /* Seek to the next operation. */
                        tx.ssOperation.seek(40);

                        /* Seek register past the post state */
                        tx.ssRegister.seek(9);

                        break;
                    }


                    /* Credit tokens to an account you own. */
                    case TAO::Operation::OP::CREDIT:
                    {
                        /* The transaction that this credit is claiming. */
                        tx.ssOperation.seek(96);

                        /* The account that is being credited. */
                        uint256_t hashAddress;
                        tx.ssOperation >> hashAddress;

                        /* Verify the first register code. */
                        uint8_t nState;
                        tx.ssRegister  >> nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        tx.ssRegister  >> prestate;

                        /* Read the register from database. */
                        State dbstate;
                        if(!LLD::regDB->ReadState(hashAddress, dbstate, nFlags))
                            return debug::error(FUNCTION, "register pre-state doesn't exist");

                        /* Check the ownership. */
                        if(dbstate.hashOwner != tx.hashGenesis)
                            return debug::error(FUNCTION, "cannot generate pre-state if not owner");

                        /* Check the prestate to the dbstate. */
                        if(prestate != dbstate)
                            return debug::error(FUNCTION, "prestate and dbstate mismatch");

                        /* Seek to the next operation. */
                        tx.ssOperation.seek(8);

                        break;
                    }


                    /* Authorize is enabled in private mode only. */
                    case TAO::Operation::OP::AUTHORIZE:
                    {
                        /* Seek through the stream. */
                        tx.ssOperation.seek(64);

                        /* Extract the genesis. */
                        uint256_t hashGenesis;
                        tx.ssOperation >> hashGenesis;

                        /* Check and enforce private mode. */
                        if(!config::GetBoolArg("-private"))
                            return debug::error(FUNCTION, "cannot use authorize when not in private mode");

                        /* Check genesis. */
                        if(hashGenesis != uint256_t("0xb5a74c14508bd09e104eff93d86cbbdc5c9556ae68546895d964d8374a0e9a41"))
                            return debug::error(FUNCTION, "invalid genesis generated");

                        break;
                    }


                    /* Claim doesn't need register verification. */
                    case TAO::Operation::OP::CLAIM:
                    {
                        /* Seek through the stream. */
                        tx.ssOperation.seek(64);

                        break;
                    }

                    default:
                        return debug::error(FUNCTION, "invalid code for register verification");
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
