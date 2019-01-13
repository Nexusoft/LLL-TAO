/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/address.h>

namespace LLP
{
    Address::Address() : Service()
    {
        Init();
    }

    Address::~Address()
    {
    }


    Address::Address(Service ipIn, uint64_t nServicesIn) : Service(ipIn)
    {
        Init();
        nServices = nServicesIn;
    }


    void Address::Init()
    {
        nServices = NODE_NETWORK;
        nTime = 100000000;
        nLastTry = 0;
    }

}
