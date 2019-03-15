/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/global.h>

namespace LLP
{
    /* Declare the Global LLP Instances. */
    Server<TritiumNode>* TRITIUM_SERVER;
    Server<LegacyNode> * LEGACY_SERVER;
    Server<TimeNode>   * TIME_SERVER;

    Server<CoreNode>*     CORE_SERVER;
    Server<RPCNode>*      RPC_SERVER;
    Server<LegacyMiner>*  LEGACY_MINING_SERVER;
    Server<TritiumMiner>* TRITIUM_MINING_SERVER;
}
