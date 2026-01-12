/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <Util/include/mutex.h>

class Initialize
{
    /** Track the current stage of initializing we are doing. */
    static std::string strMessage;


    /** Wrap this in a mutex so it is thread safe. **/
    static std::recursive_mutex MUTEX;


public:

    /** Update
     *
     *  Update the current initializing message.
     *
     *  @param[in] strNewMessage The new initialization message.
     *
     **/
    static void Update(const std::string& strNewMessage)
    {
        RECURSIVE(MUTEX);

        strMessage = strNewMessage;
    }


    /** Status
     *
     *  Get the current initialization status.
     *
     *  @return String with current initialization status.
     *
     **/
    static std::string Status()
    {
        RECURSIVE(MUTEX);

        return strMessage;
    }
};
