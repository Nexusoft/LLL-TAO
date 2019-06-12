/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/legacy.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>
#include <TAO/Register/include/reserved.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commit the final state to disk. */
        bool Legacy::Commit(const TAO::Register::State& state, const uint256_t& hashAddress, const uint8_t nFlags)
        {
            /* Attempt to commit new state to disk. */
            if(!LLD::Register->WriteState(hashAddress, state, nFlags))
                return debug::error(FUNCTION, "failed to write post-state to disk");

            return true;
        }


        /* Authorizes funds from an account to an account */
        bool Legacy::Execute(TAO::Register::Object &account, const uint64_t nAmount, const uint64_t nTimestamp)
        {
            /* Parse the account object register. */
            if(!account.Parse())
                return debug::error(FUNCTION, "failed to parse account object register");

            /* Check for standard types. */
            if(account.Base() != TAO::Register::OBJECTS::ACCOUNT)
                return debug::error(FUNCTION, "cannot debit from non-standard object register");

            /* Check the account balance. */
            if(nAmount > account.get<uint64_t>("balance"))
                return debug::error(FUNCTION, "account doesn't have sufficient balance");

            /* Write the new balance to object register. */
            if(!account.Write("balance", account.get<uint64_t>("balance") - nAmount))
                return debug::error(FUNCTION, "balance could not be written to object register");

            /* Update the register's checksum. */
            account.nModified = nTimestamp;
            account.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!account.IsValid())
                return debug::error(FUNCTION, "memory address is in invalid state");

            return true;
        }


        /* Verify debit validation rules and caller. */
        bool Legacy::Verify(const Contract& contract)
        {
            /* Rewind back on byte. */
            contract.Rewind(1, Contract::OPERATIONS);

            /* Reset register streams. */
            contract.Reset(Contract::REGISTERS);

            /* Get operation byte. */
            uint8_t OP = 0;
            contract >> OP;

            /* Check operation byte. */
            if(OP != OP::LEGACY)
                return debug::error(FUNCTION, "called with incorrect OP");

            /* Extract the address from contract. */
            uint256_t hashFrom = 0;
            contract >> hashFrom;

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashFrom))
                return debug::error(FUNCTION, "cannot transfer reserved address");

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
            contract.Rewind(32, Contract::OPERATIONS);

            /* Reset the register streams. */
            contract.Reset(Contract::REGISTERS);

            return true;
        }
    }
}
