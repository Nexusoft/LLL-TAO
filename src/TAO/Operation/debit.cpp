/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/debit.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>
#include <TAO/Register/include/system.h>

#include <TAO/Ledger/include/enum.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commit the final state to disk. */
        bool Debit::Commit(const TAO::Register::Object& account, const uint512_t& hashTx,
                           const uint256_t& hashFrom, const uint256_t& hashTo, const uint8_t nFlags)
        {
            /* Only commit events on new block. */
            if(nFlags & TAO::Ledger::FLAGS::BLOCK)
            {
                /* Read the owner of register. */
                TAO::Register::State state;
                if(!LLD::regDB->ReadState(hashTo, state, nFlags))
                    return debug::error(FUNCTION, "failed to read register to");

                /* Commit an event for other sigchain. */
                if(!LLD::legDB->WriteEvent(state.hashOwner, hashTx))
                    return debug::error(FUNCTION, "failed to write event for account ", state.hashOwner.SubString());
            }

            return LLD::regDB->WriteState(hashFrom, account, nFlags);
        }


        /* Authorizes funds from an account to an account */
        bool Debit::Execute(TAO::Register::Object &account, const uint64_t nAmount, const uint64_t nTimestamp)
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
        bool Debit::Verify(const Contract& contract)
        {
            /* Seek read position to first position. */
            contract.Reset();

            /* Get operation byte. */
            uint8_t OP = 0;
            contract >> OP;

            /* Check operation byte. */
            if(OP != OP::DEBIT)
                return debug::error(FUNCTION, "called with incorrect OP");

            /* Extract the address from contract. */
            uint256_t hashFrom = 0;
            contract >> hashFrom;

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashFrom))
                return debug::error(FUNCTION, "cannot debit with reserved address");

            /* Extract the address from contract. */
            uint256_t hashTo = 0;
            contract >> hashTo;

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashTo))
                return debug::error(FUNCTION, "cannot transfer register to reserved address");

            /* Check for debit to and from same account. */
            if(hashFrom == hashTo)
                return debug::error(FUNCTION, "cannot debit to the same address as from");

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
