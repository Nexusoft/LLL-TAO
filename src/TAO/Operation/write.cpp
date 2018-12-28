/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>
#include <TAO/Register/include/state.h>
#include <TAO/Register/include/enum.h>

namespace TAO::Operation
{

    /* Writes data to a register. */
    bool Write(uint256_t hashAddress, std::vector<uint8_t> vchData, uint256_t hashCaller)
    {
        /* Read the binary data of the Register. */
        TAO::Register::State state;
        if(!LLD::regDB->ReadState(hashAddress, state))
            return debug::error(FUNCTION "register address doewn't exist %s", __PRETTY_FUNCTION__, hashAddress.ToString().c_str());

        /* Check ReadOnly permissions. */
        if(state.nType == TAO::Register::OBJECT::READONLY)
            return debug::error(FUNCTION "write operation called on read-only register", __PRETTY_FUNCTION__);

        /* Check write permissions for raw state registers. */
        if(state.nType != TAO::Register::OBJECT::RAW)
            return debug::error(FUNCTION "write operation called on non-raw register", __PRETTY_FUNCTION__);

        /*state Check that the proper owner is commiting the write. */
        if(hashCaller != state.hashOwner)
            return debug::error(FUNCTION "no write permissions for caller %s", __PRETTY_FUNCTION__, hashCaller.ToString().c_str());

        /* Check the new data size against register's allocated size. */
        if(vchData.size() != state.vchState.size())
            return debug::error(FUNCTION "new register state size %u mismatch %u", __PRETTY_FUNCTION__, vchData.size(), state.vchState.size());

        /* Set the new state of the register. */
        state.SetState(vchData);

        /* Check that the register is in a valid state. */
        if(!state.IsValid())
            return debug::error(FUNCTION "memory address %s is in invalid state", __PRETTY_FUNCTION__, hashAddress.ToString().c_str());

        /* Write the register to the database. */
        if(!LLD::regDB->WriteState(hashAddress, state))
            return debug::error(FUNCTION "failed to write new state", __PRETTY_FUNCTION__);

        return true;
    }
}
