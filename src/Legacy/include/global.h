/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <Legacy/rpc/types/rpc.h>

namespace Legacy
{
    #ifndef NO_WALLET
    extern Legacy::RPC* Commands;
    #endif

    /** Initialize
     *
     *  Instantiate global instances of the API.
     *
     **/
    bool Initialize();


    /** Shutdown
     *
     *  Delete global instances of the API.
     *
     **/
    void Shutdown();
}
