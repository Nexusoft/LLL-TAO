/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/address.h>
#include <algorithm>

namespace LLP
{
    Address::Address()
    : Service()
    , nServices(NODE_NETWORK)
    , nTime(100000000)
    , nLastTry(0)
    {
    }

    Address::Address(const Address &other)
    : Service()
    , nServices(other.nServices)
    , nTime(other.nTime)
    , nLastTry(other.nLastTry)
    {
        nPort = other.nPort;

        for(uint8_t i = 0; i < 16; ++i)
            ip[i] = other.ip[i];
    }

    Address::Address(Address &other)
    : Service()
    , nServices(other.nServices)
    , nTime(other.nTime)
    , nLastTry(other.nLastTry)
    {
        nPort = other.nPort;

        for(uint8_t i = 0; i < 16; ++i)
            ip[i] = other.ip[i];
    }

    Address::Address(const Service &ipIn, uint64_t nServicesIn)
    : Service(ipIn)
    , nServices(nServicesIn)
    , nTime(100000000)
    , nLastTry(0)
    {
    }

    Address::Address(Service &ipIn, uint64_t nServicesIn)
    : Service(ipIn)
    , nServices(nServicesIn)
    , nTime(100000000)
    , nLastTry(0)
    {
    }


    Address::~Address()
    {
    }

    Address &Address::operator=(const Address &other)
    {
        for(uint8_t i = 0; i < 16; ++i)
            ip[i] = other.ip[i];

        nPort = other.nPort;

        nServices = other.nServices;
        nTime = other.nTime;
        nLastTry = other.nLastTry;

        return *this;
    }

    Address &Address::operator=(Address &other)
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
