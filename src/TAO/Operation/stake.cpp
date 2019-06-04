/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/stake.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /*  Commit the final state to disk. */
        bool Stake::Commit(const TAO::Register::State& state, const uint8_t nFlags)
        {
            return LLD::Register->WriteTrust(state.hashOwner, state);
        }


        /* Move balance to stake for trust account. */
        bool Stake::Execute(TAO::Register::Object &object, const uint64_t nAmount, const uint64_t nTimestamp)
        {
            /* Parse the account object register. */
            if(!object.Parse())
                return debug::error(FUNCTION, "Failed to parse account object register");

            /* Get account starting values */
            uint64_t nStakePrev = object.get<uint64_t>("stake");
            uint64_t nBalancePrev = object.get<uint64_t>("balance");

            if(nAmount > nBalancePrev)
                return debug::error(FUNCTION, "cannot add stake exceeding existing trust account balance");

            /* Write the new balance to object register. */
            if(!object.Write("balance", nBalancePrev - nAmount))
                return debug::error(FUNCTION, "balance could not be written to object register");

            /* Write the new stake to object register. */
            if(!object.Write("stake", nStakePrev + nAmount))
                return debug::error(FUNCTION, "stake could not be written to object register");

            /* Update the state register's timestamp. */
            object.nModified = nTimestamp;
            object.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!object.IsValid())
                return debug::error(FUNCTION, "trust account is in invalid state");

            return true;
        }


        /* Verify trust validation rules and caller. */
        bool Stake::Verify(const Contract& contract)
        {
            /* Seek read position to first position. */
            contract.Reset();

            /* Get operation byte. */
            uint8_t OP = 0;
            contract >> OP;

            /* Check operation byte. */
            if(OP != OP::STAKE)
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

            /* Seek read position to first position. */
            contract.Seek(1);

            return true;
        }
    }
}
