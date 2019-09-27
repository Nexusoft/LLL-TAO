/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/trust.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /*  Commit the final state to disk. */
        bool Trust::Commit(const TAO::Register::State& state, const uint8_t nFlags)
        {
            /* Get trust account address for state owner */
            uint256_t hashAddress =
                TAO::Register::Address(std::string("trust"), state.hashOwner, TAO::Register::Address::TRUST);

            /* Check that a trust register exists. */
            if(!LLD::Register->HasTrust(state.hashOwner))
                return debug::error(FUNCTION, "trust account not indexed");

            /* Write the register to the database. */
            //if(!LLD::Register->WriteState(hashAddress, state, nFlags))
            //    return debug::error(FUNCTION, "failed to write new state");

            /* Attempt to write to disk.
             * This should never be executed from mempool because Trust should be in producer, but
             * check the nFlags as a precaution
             */
            if(nFlags == TAO::Ledger::FLAGS::BLOCK && !LLD::Register->WriteTrust(state.hashOwner, state))
                return debug::error(FUNCTION, "failed to write post-state to disk");

            return true;
        }


        /* Commits funds from a coinbase transaction. */
        bool Trust::Execute(TAO::Register::Object &trust, const uint64_t nReward, const uint64_t nScore,
                            const int64_t nStakeChange, const uint64_t nTimestamp)
        {
            /* Parse the account object register. */
            if(!trust.Parse())
                return debug::error(FUNCTION, "Failed to parse account object register");

            /* Check it is a trust account register. */
            if(trust.Standard() != TAO::Register::OBJECTS::TRUST)
                return debug::error(FUNCTION, "no trust for non-trust account");

            /* Get account starting values */
            uint64_t nStakePrev = trust.get<uint64_t>("stake");
            uint64_t nBalancePrev = trust.get<uint64_t>("balance");

            uint64_t nStakeAdded = 0;
            uint64_t nStakeRemoved = 0;

            if(nStakeChange > 0)
            {
                if(nStakeChange > nBalancePrev)
                    return debug::error(FUNCTION, "cannot add stake exceeding existing trust account balance");
                else
                    nStakeAdded = nStakeChange;
            }

            else if(nStakeChange < 0)
            {
                if((0 - nStakeChange) > nStakePrev)
                    return debug::error(FUNCTION, "cannot unstake more than existing stake balance");
                else
                    nStakeRemoved = (0 - nStakeChange);
            }

            /* Write the new trust to object register. */
            if(!trust.Write("trust", nScore))
                return debug::error(FUNCTION, "trust could not be written to object register");

            /* Write the new balance to object register. */
            if(!trust.Write("balance", nBalancePrev + nReward + nStakeRemoved - nStakeAdded))
                return debug::error(FUNCTION, "balance could not be written to object register");

            /* Write the new stake to object register. */
            if(!trust.Write("stake", nStakePrev + nStakeAdded - nStakeRemoved))
                return debug::error(FUNCTION, "stake could not be written to object register");

            /* Update the state register's timestamp. */
            trust.nModified = nTimestamp;
            trust.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!trust.IsValid())
                return debug::error(FUNCTION, "trust address is in invalid state");

            return true;
        }


        /* Verify trust validation rules and caller. */
        bool Trust::Verify(const Contract& contract)
        {
            /* Rewind back on byte. */
            contract.Rewind(1, Contract::OPERATIONS);

            /* Reset register streams. */
            contract.Reset(Contract::REGISTERS);

            /* Get operation byte. */
            uint8_t OP = 0;
            contract >> OP;

            /* Check operation byte. */
            if(OP != OP::TRUST)
                return debug::error(FUNCTION, "called with incorrect OP");

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
            if(state.hashOwner != contract.Caller())
                return debug::error(FUNCTION, "caller not authorized ", contract.Caller().SubString());

            /* Reset the register streams. */
            contract.Reset(Contract::REGISTERS);

            return true;
        }
    }
}
