/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Operation/include/unstake.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/stake.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /*  Commit the final state to disk. */
        bool Unstake::Commit(const TAO::Register::State& state, const uint8_t nFlags)
        {
            /* Attempt to write to disk.
             * Unstake operation can be executed on mempool accept, so only write trust for BLOCK flag
             */
            if(nFlags == TAO::Ledger::FLAGS::BLOCK && !LLD::Register->WriteTrust(state.hashOwner, state))
                return debug::error(FUNCTION, "failed to write post-state to disk");

            return true;
        }


        /* Move from stake to balance for trust account (unlock stake). */
        bool Unstake::Execute(TAO::Register::Object &trust, const uint64_t nAmount, const uint64_t nPenalty, const uint64_t nTimestamp)
        {
            /* Parse the account object register. */
            if(!trust.Parse())
                return debug::error(FUNCTION, "Failed to parse account object register");

            /* Check it is a trust account register. */
            if(trust.Standard() != TAO::Register::OBJECTS::TRUST)
                return debug::error(FUNCTION, "cannot unstake from non-trust account");

            /* Get account starting values */
            uint64_t nTrustPrev    = trust.get<uint64_t>("trust");
            uint64_t nBalancePrev  = trust.get<uint64_t>("balance");
            uint64_t nStakePrev    = trust.get<uint64_t>("stake");

            if(nAmount > nStakePrev)
                return debug::error(FUNCTION, "cannot unstake more than existing stake balance");

            if(nPenalty != TAO::Ledger::GetUnstakePenalty(trust, nAmount))
                return debug::error(FUNCTION, "unstake penalty mismatch");

            /* Write the new trust to object register. */
            uint64_t nTrust = 0;
            if(nPenalty < nTrustPrev)
                nTrust = nTrustPrev - nPenalty;

            if(!trust.Write("trust", nTrust))
                return debug::error(FUNCTION, "trust could not be written to object register");

            /* Write the new balance to object register. */
            if(!trust.Write("balance", nBalancePrev + nAmount))
                return debug::error(FUNCTION, "balance could not be written to object register");

            /* Write the new stake to object register. */
            if(!trust.Write("stake", nStakePrev - nAmount))
                return debug::error(FUNCTION, "stake could not be written to object register");

            /* Update the state register's timestamp. */
            trust.nModified = nTimestamp;
            trust.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!trust.IsValid())
                return debug::error(FUNCTION, "trust account is in invalid state");

            return true;
        }


        /* Verify trust validation rules and caller. */
        bool Unstake::Verify(const Contract& contract)
        {
            /* Rewind back on byte. */
            contract.Rewind(1, Contract::OPERATIONS);

            /* Reset register streams. */
            contract.Reset(Contract::REGISTERS);

            /* Get operation byte. */
            uint8_t OP = 0;
            contract >> OP;

            /* Check operation byte. */
            if(OP != OP::UNSTAKE)
                return debug::error(FUNCTION, "called with incorrect OP");

            /* Get the state byte. */
            uint8_t nState = 0; //RESERVED
            contract >>= nState;

            /* Check for the pre-state. */
            if(nState != TAO::Register::STATES::PRESTATE)
                return debug::error(FUNCTION, "register script not in pre-state");

            /* Get the pre-state. */
            TAO::Register::Object object;
            contract >>= object;

            /* Check that pre-state is valid. */
            if(!object.IsValid())
                return debug::error(FUNCTION, "pre-state is in invalid state");

            /* Check ownership of register. */
            if(object.hashOwner != contract.Caller())
                return debug::error(FUNCTION, "caller not authorized ", contract.Caller().SubString());

            return true;
        }
    }
}
