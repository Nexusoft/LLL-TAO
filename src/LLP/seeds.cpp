/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#include <LLP/include/seeds.h>

namespace LLP
{

    /*  These addresses are the first point of contact on the P2P network
    *  They are established and maintained by the owners of each domain. */
    const std::vector<std::string> DNS_SeedNodes =
    {
        //viz DNS seeds
        "node1.nexus.io",
        "node2.nexus.io",
        "node3.nexus.io",
        "node4.nexus.io",

        //interactions DNS seeds
        "lite1.nexenture.xyz",
        "lite2.nexenture.xyz",
        "lite3.nexenture.xyz",
        "lite4.nexenture.xyz",

        //cryptowise university
        "seed1.cryptowiseuniversity.net",
        "seed2.cryptowiseuniversity.net",
        "seed3.cryptowiseuniversity.net",
        "seed4.cryptowiseuniversity.net",
    };


    /*  Testnet seed nodes. */
    const std::vector<std::string> DNS_SeedNodes_Testnet =
    {
        "node1.nexus.io",
        "node2.nexus.io",
        "testnet1.nexus-interactions.io",
        "testnet2.nexus-interactions.io",
        "testnet3.nexus-interactions.io"
    };
}
