/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/rpc/types/rpc.h>

#include <LLP/include/global.h>
#include <LLP/include/hosts.h>
#include <LLP/include/inv.h>
#include <LLP/include/port.h>

#include <Util/include/json.h>
#include <Util/include/signals.h>

/* Global TAO namespace. */
namespace Legacy
{
    /* stop"
    *  Stop Nexus server */
    encoding::json RPC::Stop(const encoding::json& params, const bool fHelp)
    {
        if(fHelp || params.size() != 0)
            return std::string("stop - Stop Nexus server.");
        // Shutdown will take long enough that the response should get back
        Shutdown();
        return "Nexus server stopping";
    }

    /* getconnectioncount
       Returns the number of connections to other nodes */
    encoding::json RPC::GetConnectionCount(const encoding::json& params, const bool fHelp)
    {
        if(fHelp || params.size() != 0)
            return std::string(
                "getconnectioncount"
                " - Returns the number of connections to other nodes.");

        return GetTotalConnectionCount();
    }
}
