/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_PORT_H
#define NEXUS_LLP_INCLUDE_PORT_H

#include <cstdint>
#include <Util/include/args.h>

#ifdef WIN32
/* In MSVC, this is defined as a macro, undefine it to prevent a compile and link error */
#undef SetPort
#endif

#ifndef MAINNET_PORT
#define MAINNET_PORT 9323
#endif

#ifndef TESTNET_PORT
#define TESTNET_PORT 8323
#endif

#ifndef MAINNET_CORE_LLP_PORT
#define MAINNET_CORE_LLP_PORT 9324
#endif

#ifndef TESTNET_CORE_LLP_PORT
#define TESTNET_CORE_LLP_PORT 8329
#endif

#ifndef MAINNET_MINING_LLP_PORT
#define MAINNET_MINING_LLP_PORT 9325
#endif

#ifndef TESTNET_MINING_LLP_PORT
#define TESTNET_MINING_LLP_PORT 8325
#endif

namespace LLP
{
    /** GetCorePort
     *
     *  Get the Main Core LLP Port for Nexus.
     *
     *  @param[in] testnet Flag for if port is a testnet port
     *
     *  @return Returns a 16-bit port number for core mainnet or testnet.
     *
     **/
    inline uint16_t GetCorePort(const bool testnet = config::fTestNet)
    {
        return testnet ? TESTNET_CORE_LLP_PORT : MAINNET_CORE_LLP_PORT;
    }


    /** GetMiningPort
     *
     *  Get the Main Mining LLP Port for Nexus.
     *
     *  @param[in] testnet Flag for if port is a testnet port
     *
     *  @return Returns a 16-bit port number for mining mainnet or testnet.
     *
     **/
    inline uint16_t GetMiningPort(const bool testnet = config::fTestNet)
    {
        return testnet ? TESTNET_MINING_LLP_PORT : MAINNET_MINING_LLP_PORT;
    }


    /** GetDefaultPort
     *
     *  Get the Main Message LLP Port for Nexus.
     *
     *  @param[in] testnet Flag for if port is a testnet port
     *
     *  @return Returns a 16-bit port number for mining mainnet or testnet.
     *
     **/
    inline uint16_t GetDefaultPort(const bool testnet = config::fTestNet)
    {
        return testnet ? TESTNET_PORT : MAINNET_PORT;
    }

}

#endif
