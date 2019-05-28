/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/state.h>
#include <TAO/Register/include/system.h>

#include <TAO/Ledger/types/mempool.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Claims a register from a transfer. */
        bool Execute::Claim(TAO::Register::State &state, const uint256_t& hashClaim, const uint64_t nTimestamp)
        {
            /* Make sure the register claim is in SYSTEM pending from a transfer. */
            if(state.hashOwner != 0)
                return debug::error(FUNCTION, "can't claim untransferred register");

            /* Set the new owner of the register. */
            state.hashOwner  = hashClaim;
            state.nModified = nTimestamp;
            state.SetChecksum();

            /* Check register for validity. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "post-state is in invalid state");

            return true;
        }


        /* Verify Append and caller register. */
        bool Verify::Claim(const Contract& contract, const Contract& claim, const uint256_t& hashCaller)
        {

            /* Seek claim read position to first. */
            contract.Reset();

            /* Get operation byte. */
            OP = 0;
            contract >> OP;

            /* Check operation byte. */
            if(OP != OP::TRANSFER)
                return debug::error(FUNCTION, "cannot claim a register with no transfer");

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashCaller))
                return debug::error(FUNCTION, "cannot claim register to reserved address");

            /* Extract the address  */
            uint256_t hashAddress = 0;
            contract >> hashAddress;

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashAddress))
                return debug::error(FUNCTION, "cannot claim register with reserved address");

            /* Read the register transfer recipient. */
            uint256_t hashTransfer = 0;
            contract >> hashTransfer;

            /* Get the state byte. */
            uint8_t nState = 0; //RESERVED
            contract >>= nState;

            /* Check for the pre-state. */
            if(nState != TAO::Register::STATES::PRESTATE)
                return debug::error(FUNCTION, "register script not in pre-state");

            /* Get the pre-state. */
            TAO::Register::State state;
            contract >>= state;

            /* Check the addresses match. */
            if(state.hashOwner != hashCaller && hashTransfer != hashCaller)
                return debug::error(FUNCTION, "claim public-id mismatch with transfer address");

            /* Check that pre-state is valid. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "pre-state is in invalid state");

            return true;
        }
    }
}
