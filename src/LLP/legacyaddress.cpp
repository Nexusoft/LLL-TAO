/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/legacyaddress.h>
#include <algorithm>

namespace LLP
{
    LegacyAddress::LegacyAddress()
    : NetAddr()
    , nServices(NODE_NETWORK)
    , nTime(100000000)
    , nLastTry(0)
    {
    }

    LegacyAddress::LegacyAddress(const LegacyAddress &other)
    : NetAddr()
    , nServices(other.nServices)
    , nTime(other.nTime)
    , nLastTry(other.nLastTry)
    {
        nPort = other.nPort;

        for(uint8_t i = 0; i < 16; ++i)
            ip[i] = other.ip[i];
    }


    LegacyAddress::LegacyAddress(const NetAddr &ipIn, uint64_t nServicesIn)
    : NetAddr(ipIn)
    , nServices(nServicesIn)
    , nTime(100000000)
    , nLastTry(0)
    {
    }


    LegacyAddress::~LegacyAddress()
    {
    }

    LegacyAddress &LegacyAddress::operator=(const LegacyAddress &other)
    {
        for(uint8_t i = 0; i < 16; ++i)
            ip[i] = other.ip[i];

        nPort = other.nPort;

        nServices = other.nServices;
        nTime = other.nTime;
        nLastTry = other.nLastTry;

        return *this;
    }

}
