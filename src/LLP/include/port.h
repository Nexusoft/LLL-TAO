/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_PORT_H
#define NEXUS_LLP_INCLUDE_PORT_H

#include <cstdint>

#include <Util/include/args.h>

#ifdef WIN32
/* In MSVC, this is defined as a macro, undefine it to prevent a compile and link error */
#undef SetPort
#endif

/* Mainnet */
#ifndef TRITIUM_MAINNET_PORT
#define TRITIUM_MAINNET_PORT 9888
#endif

#ifndef TRITIUM_MAINNET_SSL_PORT
#define TRITIUM_MAINNET_SSL_PORT 7888
#endif

#ifndef MAINNET_TIME_LLP_PORT
#define MAINNET_TIME_LLP_PORT 9324
#endif

#ifndef MAINNET_LOOKUP_LLP_PORT
#define MAINNET_LOOKUP_LLP_PORT 9889
#endif

#ifndef MAINNET_API_PORT
#define MAINNET_API_PORT 8080
#endif

#ifndef MAINNET_API_SSL_PORT
#define MAINNET_API_SSL_PORT 8443
#endif

#ifndef MAINNET_RPC_PORT
#define MAINNET_RPC_PORT 9336
#endif

#ifndef MAINNET_RPC_SSL_PORT
#define MAINNET_RPC_SSL_PORT 7336
#endif


#ifndef MAINNET_MINING_LLP_PORT
#define MAINNET_MINING_LLP_PORT 9325
#endif

#ifndef MAINNET_P2P_PORT
#define MAINNET_P2P_PORT 9326
#endif

#ifndef MAINNET_P2P_SSL_PORT
#define MAINNET_P2P_SSL_PORT 7326
#endif


/* Testnet */

#ifndef TRITIUM_TESTNET_PORT
#define TRITIUM_TESTNET_PORT 8888
#endif

#ifndef TRITIUM_TESTNET_SSL_PORT
#define TRITIUM_TESTNET_SSL_PORT 7888
#endif

#ifndef TESTNET_TIME_LLP_PORT
#define TESTNET_TIME_LLP_PORT 8329
#endif

#ifndef TESTNET_LOOKUP_LLP_PORT
#define TESTNET_LOOKUP_LLP_PORT 8889
#endif

#ifndef TESTNET_API_PORT
#define TESTNET_API_PORT 7080
#endif

#ifndef TESTNET_API_SSL_PORT
#define TESTNET_API_SSL_PORT 7443
#endif

#ifndef TESTNET_RPC_PORT
#define TESTNET_RPC_PORT 8336
#endif

#ifndef TESTNET_RPC_SSL_PORT
#define TESTNET_RPC_SSL_PORT 6336
#endif

#ifndef TESTNET_MINING_LLP_PORT
#define TESTNET_MINING_LLP_PORT 8325
#endif

#ifndef TESTNET_P2P_PORT
#define TESTNET_P2P_PORT 8326
#endif

#ifndef TESTNET_P2P_SSL_PORT
#define TESTNET_P2P_SSL_PORT 6326
#endif


namespace LLP
{

    /** GetAPIPort
     *
     *  Get the API LLP Port for Nexus.
     *
     *  @return Returns a 16-bit port number for core mainnet or testnet.
     *
     **/
    inline uint16_t GetAPIPort(const bool fSSL = false)
    {
        /* Check for SSL port. */
        if(fSSL)
        {
            return static_cast<uint16_t>
            (
                config::GetArg(std::string("-apisslport"),
                config::fTestNet.load() ?
                    TESTNET_API_SSL_PORT :
                    MAINNET_API_SSL_PORT)
            );
        }

        return static_cast<uint16_t>
        (
            config::GetArg(std::string("-apiport"),
            config::fTestNet.load() ?
                TESTNET_API_PORT :
                MAINNET_API_PORT)
        );
    }


    /** GetRPCPort
     *
     *  Get the RPC LLP Port for Nexus.
     *
     *  @return Returns a 16-bit port number for core mainnet or testnet.
     *
     **/
    inline uint16_t GetRPCPort()
    {
        return static_cast<uint16_t>
        (
            config::GetArg(std::string("-rpcport"),
            config::fTestNet.load() ?
                TESTNET_RPC_PORT :
                MAINNET_RPC_PORT)
        );
    }


    /** GetTimePort
     *
     *  Get the Unified time LLP Port for Nexus.
     *
     *  @return Returns a 16-bit port number for core mainnet or testnet.
     *
     **/
    inline uint16_t GetTimePort()
    {
        return static_cast<uint16_t>
        (
            config::fTestNet.load() ?
                TESTNET_TIME_LLP_PORT :
                MAINNET_TIME_LLP_PORT
        );
    }


    /** GetLookupPort
     *
     *  Get the lookup LLP port for Nexus.
     *
     *  @return Returns a 16-bit port number for core mainnet or testnet.
     *
     **/
    inline uint16_t GetLookupPort()
    {
        return static_cast<uint16_t>
        (
            config::fTestNet.load() ?
                TESTNET_LOOKUP_LLP_PORT :
                MAINNET_LOOKUP_LLP_PORT
        );
    }


    /** GetMiningPort
     *
     *  Get the Main Mining LLP Port for Nexus.
     *
     *  @return Returns a 16-bit port number for mining mainnet or testnet.
     *
     **/
    inline uint16_t GetMiningPort()
    {
        return static_cast<uint16_t>
        (
            config::GetArg(std::string("-miningport"),
            config::fTestNet.load() ?
                TESTNET_MINING_LLP_PORT :
                MAINNET_MINING_LLP_PORT)
        );
    }


    /** GetDefaultPort
     *
     *  Get the Main Message LLP Port for Nexus Tritium.
     *
     *  @return Returns a 16-bit port number for Nexus Tritium mainnet or testnet.
     *
     **/
    inline uint16_t GetDefaultPort()
    {
        return static_cast<uint16_t>
        (
            config::GetArg(std::string("-port"),
            config::fTestNet.load() ?
                (TRITIUM_TESTNET_PORT + (config::GetArg("-testnet", 0) - 1)) : TRITIUM_MAINNET_PORT));
    }
}

#endif
