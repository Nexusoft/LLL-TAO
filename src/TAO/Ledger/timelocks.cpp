/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/timelocks.h>

#include <Util/include/args.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Helper function to determine if the network timelock has been met. */
        bool NetworkActive(const uint64_t nTimestamp)
        {
            /* Check the network time-lock. */
            if(nTimestamp < (config::fTestNet.load() ? NEXUS_TESTNET_TIMELOCK : NEXUS_NETWORK_TIMELOCK))
                return false;

            return true;
        }


        /* Helper function to determine if a timelock has been met for a channel. */
        bool ChannelActive(const uint64_t nTimestamp, const uint32_t nChannel)
        {
            /* Check for private blocks (they have no channel time-lock). */
            if(nChannel == 3)
                return true;

            /* Check the Current Channel Time-Lock. */
            if(nTimestamp < (config::fTestNet.load() ? CHANNEL_TESTNET_TIMELOCK[nChannel] : CHANNEL_NETWORK_TIMELOCK[nChannel]))
                return false;

            return true;
        }


        /* Helper function to determine if a give block version is active. */
        bool VersionActive(const uint64_t nTimestamp, const uint32_t nVersion)
        {
            /* Check for version 0. */
            if(nVersion == 0)
                return false;

            /* Version 1 is always active. */
            if(nVersion == 1)
                return true;

            /* Cache testnet flag. */
            bool fTestNet = config::fTestNet.load();

            /* Get current version. */
            uint32_t nCurrent  = (fTestNet ? TESTNET_BLOCK_CURRENT_VERSION : NETWORK_BLOCK_CURRENT_VERSION);

            /* Get the current timelock. */
            uint64_t nTimelock = (fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] :
                                             NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2]);

            /* Check current version. */
            if(nVersion > nCurrent)
                return false;

            /* Check the Current Version Block Time-Lock. Allow Version (Current -1) Blocks for 1 Hour after Time Lock. */
            if(nVersion == (nCurrent - 1) && (nTimestamp - 3600) > nTimelock)
                return false;

            /* Check the Current Version Block Time-Lock. */
            if(nVersion >= nCurrent && nTimestamp <= nTimelock)
                return false;

            return true;
        }
    }
}
