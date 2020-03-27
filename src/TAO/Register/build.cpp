/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

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
#include <TAO/Operation/include/fee.h>
#include <TAO/Operation/include/genesis.h>
#include <TAO/Operation/include/legacy.h>
#include <TAO/Operation/include/migrate.h>
#include <TAO/Operation/include/transfer.h>
#include <TAO/Operation/include/trust.h>
#include <TAO/Operation/include/write.h>

#include <TAO/Register/include/build.h>
#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>

#include <new> //std::bad_alloc

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /* Verify the pre-states of a register to current network state. */
        bool Build(TAO::Operation::Contract &contract, std::map<uint256_t, State>& mapStates, const uint8_t nFlags)
        {
            /* Reset the contract streams. */
            contract.Reset(TAO::Operation::Contract::ALL);

            /* Clear the register stream in case this contract has already been built previously */
            contract.Clear(TAO::Operation::Contract::REGISTERS);

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
                        /* Get next operation. */
                        contract >> nOP;

                        /* Condition has no parameters. */
                        break;
                    }


                    /* Validate a previous contract's conditions */
                    case TAO::Operation::OP::VALIDATE:
                    {
                        /* Seek to end of stream. */
                        contract.Seek(68);

                        /* Get next operation. */
                        contract >> nOP;

                        break;
                    }
                }

                /* Check the current opcode. */
                switch(nOP)
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

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            state = mapStates[hashAddress];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::WRITE: register pre-state doesn't exist");

                        /* Check the ownership. */
                        if(state.hashOwner != contract.Caller())
                            return debug::error(FUNCTION, "OP::WRITE: cannot generate pre-state if not owner");

                        /* Serialize the pre-state into contract. */
                        contract <<= state;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Write::Execute(state, vchData, contract.Timestamp()))
                            return debug::error(FUNCTION, "OP::WRITE: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= state.GetHash();

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = state;

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

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            state = mapStates[hashAddress];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::APPEND: register pre-state doesn't exist");

                        /* Check the ownership. */
                        if(state.hashOwner != contract.Caller())
                            return debug::error(FUNCTION, "OP::APPEND: cannot generate pre-state if not owner");

                        /* Serialize the pre-state into contract. */
                        contract <<= state;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Append::Execute(state, vchData, contract.Timestamp()))
                            return debug::error(FUNCTION, "OP::APPEND: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= state.GetHash();

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = state;

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
                        state.hashOwner  = contract.Caller();

                        /* Generate the post-state. */
                        if(!TAO::Operation::Create::Execute(state, vchData, contract.Timestamp()))
                            return debug::error(FUNCTION, "OP::CREATE: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= state.GetHash();

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = state;

                        break;
                    }


                    /* Transfer ownership of a register to another signature chain. */
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

                        /* Register custody in SYSTEM ownership until claimed, unless the ForceTransfer flag has been set */
                        uint256_t hashNewOwner = 0; //default to SYSTEM
                        if(nType == TAO::Operation::TRANSFER::FORCE)
                            hashNewOwner = hashTransfer;

                        /* Handle version switch for contract to transfer to 0x00 leading genesis-id. */
                        else if(contract.Version() > 1) //this allows us to iterate history while getting properties of transfer to system
                        {
                            /* Set the new owner to transfer recipient. */
                            hashNewOwner = contract.Caller();
                            hashNewOwner.SetType(Ledger::GENESIS::SYSTEM); //this byte (0x00) means there is no valid owner during transfer
                        }

                        /* Serialize the pre-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::PRESTATE);

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            state = mapStates[hashAddress];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::TRANSFER: register pre-state doesn't exist");

                        /* Check the ownership. */
                        if(state.hashOwner != contract.Caller())
                            return debug::error(FUNCTION, "OP::TRANSFER: cannot generate pre-state if not owner");

                        /* Serialize the pre-state into contract. */
                        contract <<= state;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Transfer::Execute(state, hashNewOwner, contract.Timestamp()))
                            return debug::error(FUNCTION, "OP::TRANSFER: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= state.GetHash();

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = state;

                        break;
                    }


                    /* Transfer ownership of a register to another signature chain. */
                    case TAO::Operation::OP::CLAIM:
                    {
                        /* Seek to address. */
                        contract.Seek(68);

                        /* Get last trust block. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Serialize the pre-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::PRESTATE);

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            state = mapStates[hashAddress];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::CLAIM: register pre-state doesn't exist");

                        /* Serialize the pre-state into contract. */
                        contract <<= state;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Claim::Execute(state, contract.Caller(), contract.Timestamp()))
                            return debug::error(FUNCTION, "OP::CLAIM: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= state.GetHash();

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = state;

                        break;
                    }

                    /* Coinbase operation. Creates an account if none exists. */
                    case TAO::Operation::OP::COINBASE:
                    {
                        /* Seek to end. */
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

                        /* Check that indexed trust account exists. */
                        if(!LLD::Register->HasTrust(contract.Caller()))
                            return debug::error(FUNCTION, "OP::TRUST: no trust without genesis");

                        /* Serialize the pre-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::PRESTATE);

                        /* Check temporary memory states first. */
                        Object object;
                        if(mapStates.count(hashAddress))
                            object = TAO::Register::Object(mapStates[hashAddress]);

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, object, nFlags))
                            return debug::error(FUNCTION, "OP::TRUST: register pre-state doesn't exist");

                        /* Serialize the pre-state into contract. */
                        contract <<= object;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Trust::Execute(object, nReward, nScore, nStakeChange, contract.Timestamp()))
                            return false;

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= object.GetHash();

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

                        /* Check that no indexed trust account exists. */
                        if(LLD::Register->HasTrust(contract.Caller()))
                            return debug::error(FUNCTION, "OP::GENESIS: trust account previously indexed");

                        /* Serialize the pre-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::PRESTATE);

                        /* Check temporary memory states first. */
                        Object object;
                        if(mapStates.count(hashAddress))
                            object = TAO::Register::Object(mapStates[hashAddress]);

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, object, nFlags))
                            return debug::error(FUNCTION, "OP::GENESIS: register pre-state doesn't exist");

                        /* Serialize the pre-state into contract. */
                        contract <<= object;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Genesis::Execute(object, nReward, contract.Timestamp()))
                            return debug::error(FUNCTION, "OP::GENESIS: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= object.GetHash();

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = TAO::Register::State(object);

                        break;
                    }


                    /* Debit tokens from an account you own. */
                    case TAO::Operation::OP::DEBIT:
                    {
                        /* Get the register address. */
                        uint256_t hashFrom = 0;
                        contract >> hashFrom;

                        /* Seek to the end. */
                        contract.Seek(32);

                        /* Get the transfer amount. */
                        uint64_t nAmount = 0;
                        contract >> nAmount;

                        /* Get the reference. */
                        uint64_t nReference = 0;
                        contract >> nReference;

                        /* Serialize the pre-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::PRESTATE);

                        /* Check temporary memory states first. */
                        Object object;
                        if(mapStates.find(hashFrom) != mapStates.end())
                            object = TAO::Register::Object(mapStates[hashFrom]);

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashFrom, object, nFlags))
                            return debug::error(FUNCTION, "OP::DEBIT: register pre-state doesn't exist");

                        /* Serialize the pre-state into contract. */
                        contract <<= object;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Debit::Execute(object, nAmount, contract.Timestamp()))
                            return debug::error(FUNCTION, "OP::DEBIT: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= object.GetHash();

                        /* Write the state to memory map. */
                        mapStates[hashFrom] = TAO::Register::State(object);

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

                        /* Get the transfer address. */
                        uint256_t hashProof = 0;
                        contract >> hashProof;

                        /* Get the transfer amount. */
                        uint64_t  nAmount = 0;
                        contract >> nAmount;

                        /* Serialize the pre-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::PRESTATE);

                        /* Check temporary memory states first. */
                        Object object;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            object = TAO::Register::Object(mapStates[hashAddress]);

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, object, nFlags))
                            return debug::error(FUNCTION, "OP::CREDIT: register pre-state doesn't exist");

                        /* Serialize the pre-state into contract. */
                        contract <<= object;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Credit::Execute(object, nAmount, contract.Timestamp()))
                            return debug::error(FUNCTION, "OP::CREDIT: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= object.GetHash();

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = TAO::Register::State(object);

                        break;
                    }


                    /* Migrate a trust key to a trust account register. */
                    case TAO::Operation::OP::MIGRATE:
                    {
                        /* Seek to address. */
                        contract.Seek(64);

                        /* Get the trust register address. (hash to) */
                        TAO::Register::Address hashAccount;
                        contract >> hashAccount;

                        /* Seek to amount. */
                        contract.Seek(72);

                        /* Get the amount to migrate. */
                        uint64_t nAmount = 0;
                        contract >> nAmount;

                        /* Get the trust score to migrate. */
                        uint32_t nScore = 0;
                        contract >> nScore;

                        /* Seek to end */
                        contract.Seek(64);

                        /* Check that no indexed trust account exists. */
                        if(LLD::Register->HasTrust(contract.Caller()))
                            return debug::error(FUNCTION, "OP::MIGRATE: trust account previously indexed");

                        /* Serialize the pre-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::PRESTATE);

                        /* Check temporary memory states first. */
                        Object object;
                        if(mapStates.find(hashAccount) != mapStates.end())
                            object = TAO::Register::Object(mapStates[hashAccount]);

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAccount, object, nFlags))
                            return debug::error(FUNCTION, "OP::MIGRATE: register pre-state doesn't exist");

                        /* Serialize the pre-state into contract. */
                        contract <<= object;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Migrate::Execute(object, nAmount, nScore, contract.Timestamp()))
                            return debug::error(FUNCTION, "OP::MIGRATE: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= object.GetHash();

                        /* Write the state to memory map. */
                        mapStates[hashAccount] = TAO::Register::State(object);

                        break;
                    }


                    /* Create unspendable fee that debits from the account and makes this unspendable. */
                    case TAO::Operation::OP::FEE:
                    {
                        /* Get the register address. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the transfer amount. */
                        uint64_t  nFees = 0;
                        contract >> nFees;

                        /* Serialize the pre-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::PRESTATE);

                        /* Check temporary memory states first. */
                        Object object;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            object = TAO::Register::Object(mapStates[hashAddress]);

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, object, nFlags))
                            return debug::error(FUNCTION, "OP::FEE: register pre-state doesn't exist");

                        /* Serialize the pre-state into contract. */
                        contract <<= object;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Fee::Execute(object, nFees, contract.Timestamp()))
                            return debug::error(FUNCTION, "OP::FEE: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= object.GetHash();

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
                        /* Get the register address. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Get the transfer amount. */
                        uint64_t  nAmount = 0;
                        contract >> nAmount;

                        /* Serialize the pre-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::PRESTATE);

                        /* Check temporary memory states first. */
                        Object object;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            object = TAO::Register::Object(mapStates[hashAddress]);

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, object, nFlags))
                            return debug::error(FUNCTION, "OP::LEGACY: register pre-state doesn't exist");

                        /* Serialize the pre-state into contract. */
                        contract <<= object;

                        /* Calculate the new operation. */
                        if(!TAO::Operation::Legacy::Execute(object, nAmount, contract.Timestamp()))
                            return debug::error(FUNCTION, "OP::LEGACY: cannot generate post-state");

                        /* Serialize the post-state byte into contract. */
                        contract <<= uint8_t(TAO::Register::STATES::POSTSTATE);

                        /* Serialize the checksum into contract. */
                        contract <<= object.GetHash();

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = TAO::Register::State(object);

                        /* Get the script data. */
                        ::Legacy::Script script;
                        contract >> script;

                        break;
                    }

                    default:
                        return debug::error(FUNCTION, "invalid code for register buildling");
                }

                /* Check for end of stream. */
                if(!contract.End())
                    return debug::error(FUNCTION, "can only include one PRIMITIVE OP");
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
