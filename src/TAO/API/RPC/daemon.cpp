/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/rpc.h>
#include <Util/include/json.h>
#include <LLP/include/inv.h>
#include <LLP/include/hosts.h>
#include <LLP/include/global.h>

namespace TAO::API
{
    // json::json stop(const json::json& jsonParams, bool fHelp)
    // {
    //     if (fHelp || jsonParams.size() != 0)
    //         return std::string(
    //             "stop"
    //             " - Stop Nexus server.");
    //     // Shutdown will take long enough that the response should get back
    //     StartShutdown();
    //     return "Nexus server stopping";
    // }

    json::json RPC::GetConnectionCount(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() != 0)
            return std::string(
                "getconnectioncount"
                " - Returns the number of connections to other nodes.");

        return GetTotalConnectionCount();
    }

    json::json RPC::Reset(const json::json& jsonParams, bool fHelp)
    {
        if(fHelp || jsonParams.size() != 0)
            return std::string(
                "reset"
                " - Restart all node connections");

    //     //disconnect all nodes currently active
    //     {
    //         LOCK(cs_vNodes);
    //         // Disconnect unused nodes
    //         vector<CNode*> vNodesCopy = vNodes;
    //         BOOST_FOREACH(CNode* pnode, vNodesCopy)
    //         {
    //             // remove from vNodes
    //             vNodes.erase(remove(vNodes.begin(), vNodes.end(), pnode), vNodes.end());

    //             // close socket and cleanup
    //             pnode->CloseSocketDisconnect();
    //             pnode->Cleanup();

    //             // hold in disconnected pool until all refs are released
    //             pnode->nReleaseTime = max(pnode->nReleaseTime, GetUnifiedTimestamp() + 15 * 60);
    //             if (pnode->fNetworkNode || pnode->fInbound)
    //                 pnode->Release();
    //             vNodesDisconnected.push_back(pnode);
    //         }
    //     }
    //     ReadConfigFile(mapArgs, mapMultiArgs);

    //     return "success";
    }

}
