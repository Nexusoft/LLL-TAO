/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_TIMELOCKS_H
#define NEXUS_TAO_LEDGER_INCLUDE_TIMELOCKS_H

#include <cstdint>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** The network current block version. **/
        extern const uint32_t NETWORK_BLOCK_CURRENT_VERSION;


        /** The testnet current block version. **/
        extern const uint32_t TESTNET_BLOCK_CURRENT_VERSION;


        /** Nexus Testnet Timelock
         *
         *  Activated test network at timestamp.
         *
         **/
        extern const uint32_t NEXUS_TESTNET_TIMELOCK;


        /** Nexus Network Timelock
         *
         *  Activated main network at timestamp.
         *
         **/
        extern const uint32_t NEXUS_NETWORK_TIMELOCK;


        /** Testnet Version Timelock
         *
         *  Lock for the Nexus block version upgrades.
         *
         **/
        extern const uint32_t TESTNET_VERSION_TIMELOCK[];


        /** Network Version Timelock
         *
         *  Lock for the Nexus block version upgrades.
         *
         **/
        extern const uint32_t NETWORK_VERSION_TIMELOCK[];


        /** Testnet Channel Timelock
         *
         *  Lock to activate each corresponding proof channel.
         *
         **/
        extern const uint32_t CHANNEL_TESTNET_TIMELOCK[];


        /** Mainnet Channel Timelock
         *
         *  Lock to activate each corresponding proof channel.
         *
         **/
        extern const uint32_t CHANNEL_NETWORK_TIMELOCK[];


        /** NetworkActive
         *
         *  Helper function to determine if the network timelock has been met.
         *
         *  @param[in] nTimestamp The timestamp to check against
         *
         *  @return true if the network is currently active.
         *
         **/
        bool NetworkActive(const uint64_t nTimestamp);


        /** ChannelActive
         *
         *  Helper function to determine if a timelock has been met for a channel.
         *
         *  @param[in] nTimestamp The timestamp to check against
         *  @param[in] nChannel The channel to check for
         *
         *  @return true if the channel is currently active.
         *
         **/
        bool ChannelActive(const uint64_t nTimestamp, const uint32_t nChannel);


        /** VersionActive
         *
         *  Helper function to determine if a give block version is active.
         *
         *  @param[in] nTimestamp The timestamp to check against.
         *  @param[in] nVersion The version to check against.
         *
         **/
        bool VersionActive(const uint64_t nTimestamp, const uint32_t nVersion);


        /** CurrentVersion
         *
         *  Returns current block version from mainnet or testnet.
         *
         **/
        uint32_t CurrentVersion();


        /** CurrentTimelock
         *
         *  Returns current block timelock activation from mainnet or testnet.
         *
         **/
        uint32_t CurrentTimelock();

    }
}

#endif
