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

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Transfers a register between sigchains. */
        bool Claim(const uint512_t& hashTx, const uint8_t nFlags, TAO::Ledger::Transaction &tx)
        {
            /* Check for reserved values. */
            if(TAO::Register::Reserved(tx.hashGenesis))
                return debug::error(FUNCTION, "cannot claim register to reserved address");

            /* Read the claimed transaction. */
            TAO::Ledger::Transaction txClaim;
            if(!LLD::legDB->ReadTx(hashTx, txClaim))
                return debug::error(FUNCTION, hashTx.ToString(), " tx doesn't exist");

            /* Extract the state from tx. */
            uint8_t TX_OP;
            txClaim.ssOperation >> TX_OP;

            /* Check for transfer. */
            if(TX_OP != OP::TRANSFER)
                return debug::error(FUNCTION, "cannot claim a register with no transfer");

            /* Extract the address  */
            uint256_t hashAddress;
            txClaim.ssOperation >> hashAddress;

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashAddress))
                return debug::error(FUNCTION, "cannot claim register with reserved address");

            /* Check if this transfer is already claimed. */
            if(LLD::legDB->HasProof(hashAddress, hashTx))
                return debug::error(FUNCTION, "transfer is already claimed");

            /* Read the register transfer recipient. */
            uint256_t hashTransfer;
            txClaim.ssOperation >> hashTransfer;

            /* Check for claim back to self. */
            if(txClaim.hashGenesis == tx.hashGenesis)
            {
                /* Erase the previous event if claimed back to self. */
                if((nFlags & TAO::Register::FLAGS::WRITE) && !LLD::legDB->EraseEvent(hashTransfer))
                    return debug::error(FUNCTION, "can't erase event for return to self");
            }

            /* Check the addresses match. */
            else if(hashTransfer != tx.hashGenesis)
                return debug::error(FUNCTION, "claim genesis mismatch with transfer address");

            /* Read the register from the database. */
            TAO::Register::State state = TAO::Register::State();
            if(!LLD::regDB->ReadState(hashAddress, state))
                return debug::error(FUNCTION, "Register ", hashAddress.ToString(), " doesn't exist in register DB");

            /* Make sure the register claim is in SYSTEM pending from a transfer. */
            if(state.hashOwner != 0)
                return debug::error(FUNCTION, tx.hashGenesis.ToString(), " not authorized to transfer register");

            /* Set the new owner of the register. */
            state.hashOwner  = tx.hashGenesis;
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
                if((nFlags & TAO::Register::FLAGS::WRITE) && !LLD::regDB->WriteState(hashAddress, state))
                    return debug::error(FUNCTION, "failed to write new state");

                /* Write the proof to the database. */
                if((nFlags & TAO::Register::FLAGS::WRITE) && !LLD::legDB->WriteProof(hashAddress, hashTx))
                    return debug::error(FUNCTION, "failed to write new state");
            }

            return true;
        }
    }
}
