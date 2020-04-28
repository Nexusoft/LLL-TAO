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


        /** The network current transaction version. **/
        extern const uint32_t NETWORK_TRANSACTION_CURRENT_VERSION;


        /** The testnet current transaction version. **/
        extern const uint32_t TESTNET_TRANSACTION_CURRENT_VERSION;


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


        /** Nexus Tritium Timelock
         *
         *  Activated timestamp of first tritium block.
         *
         **/
        extern const uint32_t NEXUS_TRITIUM_TIMELOCK;


        /** Testnet Version Timelock
         *
         *  Lock for the Nexus block version upgrades.
         *
         **/
        extern const uint32_t TESTNET_BLOCK_VERSION_TIMELOCK[];


        /** Network Version Timelock
         *
         *  Lock for the Nexus block version upgrades.
         *
         **/
        extern const uint32_t NETWORK_BLOCK_VERSION_TIMELOCK[];


        /** Nexus Network Timelock
         *
         *  Activated main network at timestamp.
         *
         **/
        extern const uint32_t NEXUS_NETWORK_TIMELOCK;


        /** Testnet Transaction Version Timelock
         *
         *  Lock for the Nexus transaction version upgrades.
         *
         **/
        extern const uint32_t TESTNET_TRANSACTION_VERSION_TIMELOCK[];


        /** Network Transaction Version Timelock
         *
         *  Lock for the Nexus transaction version upgrades.
         *
         **/
        extern const uint32_t NETWORK_TRANSACTION_VERSION_TIMELOCK[];


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


        /** BlockVersionActive
         *
         *  Helper function to test if a given block version is active
         *  at the time of the provided timestamp.
         *
         *  @param[in] nTimestamp The timestamp to check against.
         *  @param[in] nVersion The version to check against.
         *
         **/
        bool BlockVersionActive(const uint64_t nTimestamp, const uint32_t nVersion);


        /** TransactionVersionActive
         *
         *  Helper function to test if a given transaction version is active
         *  at the time of the provided timestamp.
         *
         *  @param[in] nTimestamp The timestamp to check against.
         *  @param[in] nVersion The version to check against.
         *
         **/
        bool TransactionVersionActive(const uint64_t nTimestamp, const uint32_t nVersion);


        /** CurrentBlockVersion
         *
         *  Retrieve the current block version from mainnet or testnet.
         *
         **/
        uint32_t CurrentBlockVersion();


        /** CurrentTransactionVersion
         *
         *  Retrieve the current transaction version from mainnet or testnet.
         *
         **/
        uint32_t CurrentTransactionVersion();


        /** CurrentBlockTimelock
         *
         *  Retrieve the current block timelock activation from mainnet or testnet.
         *
         **/
        uint32_t CurrentBlockTimelock();


        /** CurrentTransactionTimelock
         *
         *  Retrieve the current transaction timelock activation from mainnet or testnet.
         *
         **/
        uint32_t CurrentTransactionTimelock();


        /** StartBlockTimelock
         *
         *  Retrieve the timelock activation for a given block version on mainnet or testnet.
         *
         *  Invalid versions (version 0 or anything after current version) return 0. This should be checked.
         *
         *  @param[in] nVersion The version of the timelock to retreive
         *
         *  @return version activation timelock
         *
         **/
        uint64_t StartBlockTimelock(const uint32_t nVersion);


        /** EndBlockTimelock
         *
         *  Retrieve the ending timelock for a given block version on mainnet or testnet. This value equals the StartBlockTimelock()
         *  value of the next version.
         *
         *  Invalid versions (version 0 or anything after current version) return 0, as does the current version.
         *  This should be checked.
         *
         *  @param[in] nVersion The version of the timelock to retreive
         *
         *  @return version ending timelock
         *
         **/
        uint64_t EndBlockTimelock(const uint32_t nVersion);


        /** StartTransactionTimelock
         *
         *  Retrieve the timelock activation for a given transaction version on mainnet or testnet.
         *
         *  Invalid versions (version 0 or anything after current version) return 0. This should be checked.
         *
         *  @param[in] nVersion The version of the timelock to retreive
         *
         *  @return version activation timelock
         *
         **/
        uint64_t StartTransactionTimelock(const uint32_t nVersion);


        /** EndTransactionTimelock
         *
         *  Retrieve the ending timelock for a given transaction version on mainnet or testnet.
         *  This value equals the StartTransactionTimelock() value of the next version.
         *
         *  Invalid versions (version 0 or anything after current version) return 0, as does the current version.
         *  This should be checked.
         *
         *  @param[in] nVersion The version of the timelock to retreive
         *
         *  @return version ending timelock
         *
         **/
        uint64_t EndTransactionTimelock(const uint32_t nVersion);

    }
}

#endif
