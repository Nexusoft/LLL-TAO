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
        TAO::Register::State regState;
        if(!LLD::regDB->ReadState(hashAddress, regState))
            return debug::error(FUNCTION "Register Address doewn't exist %s", __PRETTY_FUNCTION__, hashAddress.ToString().c_str());

        /* Check ReadOnly permissions. */
        if(regState.nType == TAO::Register::OBJECT::READONLY)
            return debug::error(FUNCTION "Write operation called on read-only register", __PRETTY_FUNCTION__);

        /*state Check that the proper owner is commiting the write. */
        if(hashCaller != regState.hashOwner)
            return debug::error(FUNCTION "No write permissions for caller %s", __PRETTY_FUNCTION__, hashCaller.ToString().c_str());

        /* Check the new data size against register's allocated size. */
        if(vchData.size() != regState.nLength)
            return debug::error(FUNCTION "New Register State size %u mismatch %u", __PRETTY_FUNCTION__, vchData.size(), regState.nLength);

        /* Set the new state of the register. */
        regState.SetState(vchData);

        /* Write the register to the database. */
        if(!LLD::regDB->WriteState(hashAddress, regState))
            return debug::error(FUNCTION "Failed to write new state", __PRETTY_FUNCTION__);

        return true;
    }
}
