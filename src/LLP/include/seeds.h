/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_SEEDS_H
#define NEXUS_LLP_INCLUDE_SEEDS_H

#include <string>
#include <vector>

namespace LLP
{

    /** DNS_SeedNodes
     *
     *  These addresses are the first point of contact on the P2P network
     *  They are established and maintained by the owners of each domain.
     *
     **/
    extern const std::vector<std::string> DNS_SeedNodes;


    /** DNS_SeedNodes_Testnet
     *
     *  Testnet seed nodes.
     *
     **/
    extern const std::vector<std::string> DNS_SeedNodes_Testnet;

}

#endif
