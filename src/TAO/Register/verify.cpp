/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/append.h>
#include <TAO/Operation/include/claim.h>
#include <TAO/Operation/include/create.h>
#include <TAO/Operation/include/credit.h>
#include <TAO/Operation/include/debit.h>
#include <TAO/Operation/include/fee.h>
#include <TAO/Operation/include/genesis.h>
#include <TAO/Operation/include/legacy.h>
#include <TAO/Operation/include/migrate.h>
#include <TAO/Operation/include/transfer.h>
#include <TAO/Operation/include/trust.h>
#include <TAO/Operation/include/write.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /* Verify the pre-states of a register to current network state. */
        bool Verify(const TAO::Operation::Contract& contract,
                    std::map<uint256_t, TAO::Register::State>& mapStates, const uint8_t nFlags)
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
                        contract.Seek(68);

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
                        std::vector<uint8_t> vchData;
                        contract >> vchData;

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::WRITE: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            state = mapStates[hashAddress];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::WRITE: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != state)
                            return debug::error(FUNCTION, "OP::WRITE: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::WRITE: not authorized ", contract.Caller().SubString());

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Write::Execute(state, vchData, contract.Timestamp()))
                            return false;

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = state;

                        return true;
                    }


                    /* Check pre-state to database. */
                    case TAO::Operation::OP::APPEND:
                    {
                        /* Get the Address of the Register. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the object data size. */
                        std::vector<uint8_t> vchData;
                        contract >> vchData;

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::APPEND: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            state = mapStates[hashAddress];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::APPEND: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != state)
                            return debug::error(FUNCTION, "OP::APPEND: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::APPEND: not authorized ", contract.Caller().SubString());

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Append::Execute(state, vchData, contract.Timestamp()))
                            return false;

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = state;

                        return true;
                    }


                    /* Create does not have a pre-state. */
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
                        TAO::Register::State state;
                        state.nVersion   = 1;
                        state.nType      = nType;
                        state.hashOwner  = contract.Caller();

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Create::Execute(state, vchData, contract.Timestamp()))
                            return false;

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = state;

                        break;
                    }


                    /* Transfer ownership of a register to another signature chain. */
                    case TAO::Operation::OP::TRANSFER:
                    {
                        /* Get the Address of the Register. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Read the register transfer recipient. */
                        uint256_t hashTransfer = 0;
                        contract >> hashTransfer;

                        /* Read the force transfer flag */
                        uint8_t nType = 0;
                        contract >> nType;

                        /* Register custody in SYSTEM ownership until claimed, unless the ForceTransfer flag has been set */
                        uint256_t hashNewOwner = (nType == TAO::Operation::TRANSFER::FORCE ? hashTransfer : 0);

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::TRANSFER: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            state = mapStates[hashAddress];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::TRANSFER: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != state)
                            return debug::error(FUNCTION, "OP::TRANSFER: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::TRANSFER: not authorized ", contract.Caller().SubString());

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Transfer::Execute(state, hashNewOwner, contract.Timestamp()))
                            return false;

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = state;

                        break;
                    }


                    /* Transfer ownership of a register to another signature chain. */
                    case TAO::Operation::OP::CLAIM:
                    {
                        /* Seek to address location. */
                        contract.Seek(68);

                        /* Get the address to update. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::CLAIM: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            state = mapStates[hashAddress];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::CLAIM: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != state)
                            return debug::error(FUNCTION, "OP::CLAIM: pre-state verification failed");

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Claim::Execute(state, contract.Caller(), contract.Timestamp()))
                            return false;

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = state;

                        break;
                    }


                    /* Coinbase operation. Creates an account if none exists. */
                    case TAO::Operation::OP::COINBASE:
                    {
                        /* Seek through coinbase data. */
                        contract.Seek(48);

                        break;
                    }


                    /* Coinstake operation. Requires an account. */
                    case TAO::Operation::OP::TRUST:
                    {
                        /* Get trust account address for contract caller */
                        uint256_t hashAddress =
                            TAO::Register::Address(std::string("trust"), contract.Caller(), TAO::Register::Address::TRUST);

                        /* Seek to scores. */
                        contract.Seek(64);

                        /* Get the trust score. */
                        uint64_t nScore = 0;
                        contract >> nScore;

                        /* Get the stake change. */
                        int64_t nStakeChange = 0;
                        contract >> nStakeChange;

                        /* Get the stake reward. */
                        uint64_t nReward = 0;
                        contract >> nReward;

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::TRUST: register state not in pre-state");

                        /* Verify the register's prestate. */
                        Object prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        Object object;
                        if(mapStates.count(hashAddress))
                            object = TAO::Register::Object(mapStates[hashAddress]);

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, object))
                            return debug::error(FUNCTION, "OP::TRUST: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != object)
                            return debug::error(FUNCTION, "OP::TRUST: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::TRUST: not authorized ", contract.Caller().SubString());

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Trust::Execute(object, nReward, nScore, nStakeChange, contract.Timestamp()))
                            return false;

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = TAO::Register::State(object);

                        break;
                    }


                    /* Coinstake operation. Requires an account. */
                    case TAO::Operation::OP::GENESIS:
                    {
                        /* Get trust account address for contract caller */
                        uint256_t hashAddress =
                            TAO::Register::Address(std::string("trust"), contract.Caller(), TAO::Register::Address::TRUST);

                        /* Get the stake reward. */
                        uint64_t nReward = 0;
                        contract >> nReward;

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::GENESIS: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Hard rule: genesis requires to resolve to trust account. */
                        if(prestate.hashOwner != contract.Caller())
                            return debug::error(FUNCTION, "OP::GENESIS: caller is not state owner");

                        /* Check temporary memory states first. */
                        Object object;
                        if(mapStates.count(hashAddress))
                            object = TAO::Register::Object(mapStates[hashAddress]);

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, object, nFlags))
                            return debug::error(FUNCTION, "OP::GENESIS: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != object)
                            return debug::error(FUNCTION, "OP::GENESIS: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::GENESIS: not authorized ", contract.Caller().SubString());

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Genesis::Execute(object, nReward, contract.Timestamp()))
                            return false;

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = TAO::Register::State(object);

                        break;
                    }


                    /* Debit tokens from an account you own. */
                    case TAO::Operation::OP::DEBIT:
                    {
                        /* Get last trust block. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Seek to the end. */
                        contract.Seek(32);

                        /* Get the transfer amount. */
                        uint64_t nAmount = 0;
                        contract >> nAmount;

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Get the reference. */
                        uint64_t nReference = 0;
                        contract >> nReference;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::DEBIT: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        Object object;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            object = TAO::Register::Object(mapStates[hashAddress]);

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, object, nFlags))
                            return debug::error(FUNCTION, "OP::DEBIT: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != object)
                            return debug::error(FUNCTION, "OP::DEBIT: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::DEBIT: not authorized ", contract.Caller().SubString());

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Debit::Execute(object, nAmount, contract.Timestamp()))
                            return false;

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = TAO::Register::State(object);

                        break;
                    }


                    /* Credit tokens to an account you own. */
                    case TAO::Operation::OP::CREDIT:
                    {
                        /* Seek to address. */
                        contract.Seek(68);

                        /* Get the transfer address. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Seek to the end. */
                        contract.Seek(32);

                        /* Get the transfer amount. */
                        uint64_t  nAmount = 0;
                        contract >> nAmount;

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::CREDIT: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        Object object;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            object = TAO::Register::Object(mapStates[hashAddress]);

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, object, nFlags))
                            return debug::error(FUNCTION, "OP::CREDIT: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != object)
                        {
                            Object object1 = Object(object);
                            object1.Parse();

                            Object object2 = Object(prestate);
                            object2.Parse();

                            debug::log(0, FUNCTION, "Balance (dsk): ", object1.get<uint64_t>("balance"));
                            debug::log(0, FUNCTION, "Balance (pre): ", object2.get<uint64_t>("balance"));

                            return debug::error(FUNCTION, "OP::CREDIT: pre-state verification failed");
                        }


                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::CREDIT: not authorized ", contract.Caller().SubString());

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Credit::Execute(object, nAmount, contract.Timestamp()))
                            return false;

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = TAO::Register::State(object);

                        break;
                    }


                    /* Credit tokens to an account you own. */
                    case TAO::Operation::OP::MIGRATE:
                    {
                        /* Seek to address. */
                        contract.Seek(64);

                        /* Get the trust register address. (hash to) */
                        TAO::Register::Address hashAccount;
                        contract >> hashAccount;

                        /* Skip trust key (hash from) */
                        contract.Seek(72);

                        /* Get the amount to migrate. */
                        uint64_t nAmount;
                        contract >> nAmount;

                        /* Get the trust score to migrate. */
                        uint32_t nScore;
                        contract >> nScore;

                        /* Seek to end */
                        contract.Seek(64);

                        /* Verify the first register code. */
                        uint8_t nState;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::MIGRATE: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        Object object;
                        if(mapStates.find(hashAccount) != mapStates.end())
                            object = TAO::Register::Object(mapStates[hashAccount]);

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAccount, object, nFlags))
                            return debug::error(FUNCTION, "OP::MIGRATE: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != object)
                            return debug::error(FUNCTION, "OP::MIGRATE: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::MIGRATE: not authorized ", contract.Caller().SubString());

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Migrate::Execute(object, nAmount, nScore, contract.Timestamp()))
                            return false;

                        /* Write the state to memory map. */
                        mapStates[hashAccount] = TAO::Register::State(object);

                        break;
                    }


                    /* Create unspendable fee that debits from the account and makes this unspendable. */
                    case TAO::Operation::OP::FEE:
                    {
                        /* Get fee account address. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the transfer amount. */
                        uint64_t nFees = 0;
                        contract >> nFees;

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::FEE: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        Object object;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            object = TAO::Register::Object(mapStates[hashAddress]);

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, object, nFlags))
                            return debug::error(FUNCTION, "OP::FEE: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != object)
                            return debug::error(FUNCTION, "OP::FEE: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::FEE: not authorized ", contract.Caller().SubString());

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Fee::Execute(object, nFees, contract.Timestamp()))
                            return false;

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = TAO::Register::State(object);

                        break;
                    }


                    /* Authorize is enabled in private mode only. */
                    case TAO::Operation::OP::AUTHORIZE:
                    {
                        /* Seek to address. */
                        contract.Seek(96);

                        break;
                    }

                    /* Validate a previous contract's conditions */
                    case TAO::Operation::OP::VALIDATE:
                    {
                        /* Seek past validate parameters. */
                        contract.Seek(68);

                        break;
                    }


                    /* Create unspendable legacy script, that acts to debit from the account and make this unspendable. */
                    case TAO::Operation::OP::LEGACY:
                    {
                        /* Get last trust block. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the transfer amount. */
                        uint64_t nAmount = 0;
                        contract >> nAmount;

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::DEBIT: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        Object object;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            object = TAO::Register::Object(mapStates[hashAddress]);

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, object, nFlags))
                            return debug::error(FUNCTION, "OP::DEBIT: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != object)
                            return debug::error(FUNCTION, "OP::DEBIT: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::DEBIT: not authorized ", contract.Caller().SubString());

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Legacy::Execute(object, nAmount, contract.Timestamp()))
                            return false;

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = TAO::Register::State(object);

                        /* Get the script data. */
                        uint64_t nSize = contract.ReadCompactSize(TAO::Operation::Contract::OPERATIONS);
                        contract.Seek(nSize);

                        break;
                    }

                    default:
                        return debug::error(FUNCTION, "invalid code for contract verification");

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
