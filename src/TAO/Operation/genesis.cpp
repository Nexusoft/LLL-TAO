/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>

#include <TAO/Register/types/object.h>
#include <TAO/Register/include/system.h>

#include <TAO/Ledger/include/stake.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commit the final state to disk. */
        bool Genesis::Commit(const TAO::Register::State& state, const uint256_t& hashAddress, const uint8_t nFlags)
        {
            /* Check that a trust register exists. */
            if(LLD::regDB->HasTrust(state.hashOwner))
                return debug::error(FUNCTION, "cannot create genesis when already exists");

            /* Write the register to the database. */
            if(!LLD::regDB->WriteState(hashAddress, state, nFlags))
                return debug::error(FUNCTION, "failed to write new state");

            /* Update the register database with the index. */
            if(!LLD::regDB->IndexTrust(state.hashOwner, hashAddress))
                return debug::error(FUNCTION, "could not index the address to the genesis");
        }


        /* Commits funds from a coinbase transaction. */
        bool Genesis::Execute(TAO::Register::Object &trust, const uint64_t nReward, const uint64_t nTimestamp)
        {
            /* Parse the account object register. */
            if(!trust.Parse())
                return debug::error(FUNCTION, "failed to parse trust account object register");

            /* Check that there is no stake. */
            if(trust.get<uint64_t>("stake") != 0)
                return debug::error(FUNCTION, "cannot create genesis with already existing stake");

            /* Check that there is no trust. */
            if(trust.get<uint64_t>("trust") != 0)
                return debug::error(FUNCTION, "cannot create genesis with already existing trust");

            /* Check available balance to stake. */
            if(trust.get<uint64_t>("balance") == 0)
                return debug::error(FUNCTION, "cannot create genesis with no available balance");

            /* Move existing balance to stake. */
            if(!trust.Write("stake", trust.get<uint64_t>("balance")))
                return debug::error(FUNCTION, "stake could not be written to object register");

            /* Write the stake reward to balance in object register. */
            if(!trust.Write("balance", nReward))
                return debug::error(FUNCTION, "balance could not be written to object register");

            /* Update the state register's timestamp. */
            trust.nModified = nTimestamp;
            trust.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!trust.IsValid())
                return debug::error(FUNCTION, "memory address is in invalid state");

            return true;
        }


        /* Verify trust validation rules and caller. */
        bool Genesis::Verify(const Contract& contract)
        {
            /* Seek read position to first position. */
            contract.Reset();

            /* Get operation byte. */
            uint8_t OP = 0;
            contract >> OP;

            /* Check operation byte. */
            if(OP != OP::GENESIS)
                return debug::error(FUNCTION, "called with incorrect OP");

            /* Extract the address from contract. */
            uint256_t hashAddress = 0;
            contract >> hashAddress;

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashAddress))
                return debug::error(FUNCTION, "cannot write to register with reserved address");

            /* Get the state byte. */
            uint8_t nState = 0; //RESERVED
            contract >>= nState;

            /* Check for the pre-state. */
            if(nState != TAO::Register::STATES::PRESTATE)
                return debug::error(FUNCTION, "register script not in pre-state");

            /* Get the pre-state. */
            TAO::Register::State state;
            contract >>= state;

            /* Check that pre-state is valid. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "pre-state is in invalid state");

            /* Check ownership of register. */
            if(state.hashOwner != contract.hashCaller)
                return debug::error(FUNCTION, tx.hashGenesis.ToString(), "caller not authorized to debit from register");

            return true;
        }
    }
}
