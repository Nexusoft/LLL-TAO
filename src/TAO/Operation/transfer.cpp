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
#include <TAO/Register/types/state.h>
#include <TAO/Register/include/system.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Transfers a register between sigchains. */
        bool Transfer(const uint256_t& hashAddress, const uint256_t& hashTransfer,
                      const uint8_t nFlags, TAO::Ledger::Transaction &tx)
        {
            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashAddress))
                return debug::error(FUNCTION, "cannot transfer register with reserved address");

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashTransfer))
                return debug::error(FUNCTION, "cannot transfer register to reserved address");

            /* Read the register from the database. */
            TAO::Register::State state;
            if(!LLD::regDB->ReadState(hashAddress, state, nFlags))
                return debug::error(FUNCTION, "Register ", hashAddress.ToString(), " doesn't exist in register DB");

            /* Make sure that you won the rights to register first. */
            if(state.hashOwner != tx.hashGenesis)
                return debug::error(FUNCTION, tx.hashGenesis.ToString(), " not authorized to transfer register");

            /* Check that you aren't sending to yourself. */
            if(state.hashOwner == hashTransfer)
                return debug::error(FUNCTION, tx.hashGenesis.ToString(), " cannot transfer to self when already owned");

            /* Check if transfer is to a register. */
            TAO::Register::Object object;
            if(LLD::regDB->ReadState(hashTransfer, object, nFlags))
            {
                /* Parse the object. */
                if(!object.Parse())
                    return debug::error(FUNCTION, "couldn't parse object register");

                /* Check for token. */
                if(object.Standard() != TAO::Register::OBJECTS::TOKEN)
                    return debug::error(FUNCTION, "cannot transfer to non-token");

                /* Set the owner. */
                state.hashOwner = hashTransfer;
            }
            else
                state.hashOwner  = 0; //register custody is in SYSTEM ownership until claimed

            /* Set the new owner of the register. */
            state.nTimestamp = tx.nTimestamp;
            state.SetChecksum();

            /* Check register for validity. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "memory address ", hashAddress.ToString(), " is in invalid state");

            /* Write post-state checksum. */
            if((nFlags & TAO::Register::FLAGS::POSTSTATE))
                tx.ssRegister << (uint8_t)TAO::Register::STATES::POSTSTATE << state.GetHash();

            /* Verify the post-state checksum. */
            if(nFlags & TAO::Register::FLAGS::WRITE || nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                /* Get the state byte. */
                uint8_t nState; //RESERVED
                tx.ssRegister >> nState;

                /* Check for the pre-state. */
                if(nState != TAO::Register::STATES::POSTSTATE)
                    return debug::error(FUNCTION, "register script not in post-state");

                /* Get the post state checksum. */
                uint64_t nChecksum;
                tx.ssRegister >> nChecksum;

                /* Check for matching post states. */
                if(nChecksum != state.GetHash())
                    return debug::error(FUNCTION, "register script has invalid post-state");

                /* Write the register to the database. */
                if(!LLD::regDB->WriteState(hashAddress, state, nFlags))
                    return debug::error(FUNCTION, "failed to write new state");

                /* Write the notification foreign index. */
                if(nFlags & TAO::Register::FLAGS::WRITE && !LLD::legDB->WriteEvent(hashTransfer, tx.GetHash()))
                    return debug::error(FUNCTION, "failed to commit event to ledger DB");

            }

            return true;
        }
    }
}
