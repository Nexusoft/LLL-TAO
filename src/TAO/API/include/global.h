/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/commands.h>

#include <TAO/API/users/types/users.h> //XXX: temporary work-around for users being littered everywhere and dependent on this header

#include <TAO/API/rpc/types/rpc.h>

namespace TAO::API
{
    #ifndef NO_WALLET
    extern RPC*         legacy;
    #endif

    /** Initialize
     *
     *  Instantiate global instances of the API.
     *
     **/
    void Initialize();


    /** Shutdown
     *
     *  Delete global instances of the API.
     *
     **/
    void Shutdown();
}
