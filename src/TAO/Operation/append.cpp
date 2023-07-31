/*__________________________________________________________________________________________

        (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

        (c) Copyright The Nexus Developers 2014 - 2021

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "Doubt is the precursor to fear" - Alex Hannold

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/append.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/state.h>
#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/reserved.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commit the final state to disk. */
        bool Append::Commit(const TAO::Register::State& state, const uint256_t& hashAddress, const uint8_t nFlags)
        {
            /* Attempt to write new state to disk. */
            if(!LLD::Register->WriteState(hashAddress, state, nFlags))
                return debug::error(FUNCTION, "failed to write post-state to disk");

            return true;
        }


        /* Execute a append operation to bring register into new state. */
        bool Append::Execute(TAO::Register::State &state, const std::vector<uint8_t>& vchData, const uint64_t nTimestamp)
        {
            /* Append the state data. */
            state.Append(vchData);

            /* Update the state register checksum. */
            state.nModified = nTimestamp;
            state.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "post-state is in invalid state");

            return true;
        }


        /* Verify append validation rules and caller. */
        bool Append::Verify(const Contract& contract)
        {
            /* Rewind back on byte. */
            contract.Rewind(1, Contract::OPERATIONS);

            /* Reset register streams. */
            contract.Reset(Contract::REGISTERS);

            /* Get operation byte. */
            uint8_t OP = 0;
            contract >> OP;

            /* Check operation byte. */
            if(OP != OP::APPEND)
                return debug::error(FUNCTION, "called with incorrect OP");

            /* Extract the address from contract. */
            TAO::Register::Address hashAddress;
            contract >> hashAddress;

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashAddress))
                return debug::error(FUNCTION, "cannot append register with reserved address");

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

            /* Check for valid register types. */
            if(state.nType != TAO::Register::REGISTER::RAW && state.nType != TAO::Register::REGISTER::APPEND)
                return debug::error(FUNCTION, "cannot call on non raw or append register");

            /* Check that the proper owner is committing the write. */
            if(contract.Caller() != state.hashOwner)
                return debug::error(FUNCTION, "caller not authorized ", contract.Caller().SubString());

            /* Seek read position to first position. */
            contract.Rewind(32, Contract::OPERATIONS);

            /* Reset the register streams. */
            contract.Reset(Contract::REGISTERS);


            return true;
        }
    }
}
