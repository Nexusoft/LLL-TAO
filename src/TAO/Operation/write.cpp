/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>
#include <TAO/Register/include/state.h>
#include <TAO/Register/include/enum.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Writes data to a register. */
        bool Write(const uint256_t &hashAddress, const std::vector<uint8_t> &vchData, const uint256_t &hashCaller, const uint8_t nFlags, TAO::Ledger::Transaction &tx)
        {
            /* Read the binary data of the Register. */
            TAO::Register::State state;

            /* Write pre-states. */
            if((nFlags & TAO::Register::FLAGS::PRESTATE))
            {
                if(!LLD::regDB->ReadState(hashAddress, state))
                    return debug::error(FUNCTION, "register address doesn't exist ", hashAddress.ToString());

                tx.ssRegister << (uint8_t)TAO::Register::STATES::PRESTATE << state;
            }

            /* Get pre-states on write. */
            if(nFlags & TAO::Register::FLAGS::WRITE  || nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                /* Get the state byte. */
                uint8_t nState = 0; //RESERVED
                tx.ssRegister >> nState;

                /* Check for the pre-state. */
                if(nState != TAO::Register::STATES::PRESTATE)
                    return debug::error(FUNCTION, "register script not in pre-state");

                /* Get the pre-state. */
                tx.ssRegister >> state;
            }

            /* Check ReadOnly permissions. */
            if(state.nType == TAO::Register::OBJECT::READONLY)
                return debug::error(FUNCTION, "write operation called on read-only register");

            /* Check write permissions for raw state registers. */
            if(state.nType != TAO::Register::OBJECT::RAW)
                return debug::error(FUNCTION, "write operation called on non-raw register");

            /*state Check that the proper owner is commiting the write. */
            if(hashCaller != state.hashOwner)
                return debug::error(FUNCTION, "no write permissions for caller ", hashCaller.ToString());

            /* Check the new data size against register's allocated size. */
            if(vchData.size() != state.vchState.size())
                return debug::error(FUNCTION, "new register state size ", vchData.size(), " mismatch ",  state.vchState.size());

            /* Set the new state of the register. */
            state.nTimestamp = tx.nTimestamp;
            state.SetState(vchData);

            /* Check that the register is in a valid state. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "memory address ", hashAddress.ToString(), " is in invalid state");

            /* Write post-state checksum. */
            if((nFlags & TAO::Register::FLAGS::POSTSTATE))
                tx.ssRegister << (uint8_t)TAO::Register::STATES::POSTSTATE << state.GetHash();

            /* Verify the post-state checksum. */
            if(nFlags & TAO::Register::FLAGS::WRITE || nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                /* Get the state byte. */
                uint8_t nState = 0; //RESERVED
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
            }

            return true;
        }
    }
}
