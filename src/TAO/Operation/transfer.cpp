/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Doubt is the precursor to fear" - Alex Hannold

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/transfer.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/types/object.h>
#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/system.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commit the final state to disk. */
        bool Transfer::Commit(const TAO::Register::State& state, const uint512_t& hashTx,
                              const uint256_t& hashAddress, const uint256_t& hashTransfer, const uint8_t nFlags)
        {
            /* Only commit events on new block. */
            if(nFlags & TAO::Ledger::FLAGS::BLOCK)
            {
                /* Write the transfer event. */
                if(!LLD::Ledger->WriteEvent(hashTransfer, hashTx))
                    return debug::error(FUNCTION, "failed to write event");
            }

            return LLD::Register->WriteState(hashAddress, state, nFlags);
        }


        /* Transfers a register between sigchains. */
        bool Transfer::Execute(TAO::Register::State &state, const uint256_t& hashTransfer, const uint64_t nTimestamp)
        {
            /* Set the new register's owner. */
            state.hashOwner  = 0; //register custody is in SYSTEM ownership until claimed

            /* Set the register checksum. */
            state.nModified = nTimestamp;
            state.SetChecksum();

            /* Check register for validity. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "post-state is in invalid state");

            return true;
        }


        /* Verify claim validation rules and caller. */
        bool Transfer::Verify(const Contract& contract)
        {
            /* Seek read position to first position. */
            contract.Reset();

            /* Get operation byte. */
            uint8_t OP = 0;
            contract >> OP;

            /* Check operation byte. */
            if(OP != OP::TRANSFER)
                return debug::error(FUNCTION, "called with incorrect OP");

            /* Extract the address from contract. */
            uint256_t hashAddress = 0;
            contract >> hashAddress;

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashAddress))
                return debug::error(FUNCTION, "cannot transfer reserved address");

            /* Extract the address from contract. */
            uint256_t hashTransfer = 0;
            contract >> hashTransfer;

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashTransfer))
                return debug::error(FUNCTION, "cannot transfer register to reserved address");

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

            /* Check that you aren't sending to yourself. */
            if(state.hashOwner == hashTransfer)
                return debug::error(FUNCTION, state.hashOwner.SubString(), " transfer to self");

            /* Check for valid register types. */
            if(state.nType == TAO::Register::REGISTER::OBJECT)
            {
                /* Create an object to check values. */
                TAO::Register::Object object = TAO::Register::Object(state);

                /* Parse the object register. */
                if(!object.Parse())
                    return debug::error(FUNCTION, "failed to parse the object register");

                /* Don't allow transferring trust accounts. */
                if(object.Standard() == TAO::Register::OBJECTS::TRUST)
                    return debug::error(FUNCTION, "cannot transfer a trust account");
            }

            /* Check that the proper owner is commiting the write. */
            if(contract.Caller() != state.hashOwner)
                return debug::error(FUNCTION, "caller not authorized ", contract.Caller().SubString());

            /* Seek read position to first position. */
            contract.Seek(1);

            return true;
        }
    }
}
