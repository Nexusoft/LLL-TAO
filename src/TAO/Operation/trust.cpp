/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/trust.h>

#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /*  Commit the final state to disk. */
        bool Trust::Commit(const TAO::Register::State& state, const uint8_t nFlags)
        {
            return LLD::regDB->WriteTrust(state.hashOwner, state);
        }


        /* Commits funds from a coinbase transaction. */
        bool Trust::Execute(TAO::Register::Object &trust, const uint64_t nReward, const uint64_t nScore, const uint64_t nTimestamp)
        {
            /* Parse the account object register. */
            if(!trust.Parse())
                return debug::error(FUNCTION, "Failed to parse account object register");

            /* Write the new trust to object register. */
            if(!trust.Write("trust", nScore))
                return debug::error(FUNCTION, "trust could not be written to object register");

            /* Write the new balance to object register. */
            if(!trust.Write("balance", trust.get<uint64_t>("balance") + nReward))
                return debug::error(FUNCTION, "balance could not be written to object register");

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
            /* Seek read position to first position. */
            contract.Reset();

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
            TAO::Register::State trust;
            contract >>= trust;

            /* Check that pre-state is valid. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "pre-state is in invalid state");

            /* Check ownership of register. */
            if(trust.hashOwner != contract.hashCaller)
                return debug::error(FUNCTION, contract.hashCaller.SubString()," caller not authorized to debit from register");

            return true;
        }
    }
}
