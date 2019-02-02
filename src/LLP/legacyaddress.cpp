/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/legacyaddress.h>
#include <Util/include/debug.h>
#include <algorithm>

namespace LLP
{
    /* Default constructor */
    LegacyAddress::LegacyAddress()
    : BaseAddress()
    , nLastTry(0)
    , nServices(NODE_NETWORK)
    , nTime(100000000)
    {
    }


    /* Copy constructor */
    LegacyAddress::LegacyAddress(const LegacyAddress &other)
    : BaseAddress()
    , nLastTry(other.nLastTry)
    , nServices(other.nServices)
    , nTime(other.nTime)
    {
        nPort = other.nPort;

        for(uint8_t i = 0; i < 16; ++i)
            ip[i] = other.ip[i];
    }


    /* Copy constructor */
    LegacyAddress::LegacyAddress(const BaseAddress &ipIn, uint64_t nServicesIn)
    : BaseAddress(ipIn)
    , nLastTry(0)
    , nServices(nServicesIn)
    , nTime(100000000)
    {
    }


    /* Default destructor */
    LegacyAddress::~LegacyAddress()
    {
    }


    /* Copy assignment operator */
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


    /* Prints information about this address. */
    void LegacyAddress::Print()
    {
        debug::log(0, "LegacyAddress(", ToString(), ")");
        debug::log(0, ":\t", "nLastTry=", nLastTry);
        debug::log(0, ":\t", "nServices=", nServices);
        debug::log(0, ":\t", "nTime=", nTime);
    }

}
