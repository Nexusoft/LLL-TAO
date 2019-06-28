/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/global.h>
#include <LLP/include/network.h>

namespace LLP
{
    /* Declare the Global LLP Instances. */
    Server<TritiumNode>* TRITIUM_SERVER;
    Server<LegacyNode> * LEGACY_SERVER;
    Server<TimeNode>   * TIME_SERVER;

    Server<APINode>*      API_SERVER;
    Server<RPCNode>*      RPC_SERVER;
    Server<LegacyMiner>*  LEGACY_MINING_SERVER;
    Server<TritiumMiner>* TRITIUM_MINING_SERVER;


    /*  Initialize the LLP. */
    bool Initialize()
    {
        /* Initialize the underlying network resources such as sockets, etc */
        if(!NetworkInitialize())
            return debug::error(FUNCTION, "NetworkInitialize: Failed initializing network resources.");

        return true;
    }


    /*  Shutdown the LLP. */
    void Shutdown()
    {
        /* Shutdown the time server and its subsystems. */
        Shutdown<TimeNode>(TIME_SERVER);

        /* Shutdown the tritium server and its subsystems. */
        Shutdown<TritiumNode>(TRITIUM_SERVER);

        /* Shutdown the legacy server and its subsystems. */
        Shutdown<LegacyNode>(LEGACY_SERVER);

        /* Shutdown the core API server and its subsystems. */
        Shutdown<APINode>(API_SERVER);

        /* Shutdown the RPC server and its subsystems. */
        Shutdown<RPCNode>(RPC_SERVER);

        /* Shutdown the legacy mining server and its subsystems. */
        Shutdown<LegacyMiner>(LEGACY_MINING_SERVER);

        /* Shutdown the tritium mining server and its subsystems. */
        Shutdown<TritiumMiner>(TRITIUM_MINING_SERVER);

        /* After all servers shut down, clean up underlying network resources. */
        NetworkShutdown();
    }

}
