/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2021

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

        /* The network current block version. */
        const uint32_t NETWORK_BLOCK_CURRENT_VERSION = 8;


        /* The testnet current block version. */
        const uint32_t TESTNET_BLOCK_CURRENT_VERSION = 8;


        /* The network current transaction version. */
        const uint32_t NETWORK_TRANSACTION_CURRENT_VERSION = 4;


        /* The testnet current transaction version. */
        const uint32_t TESTNET_TRANSACTION_CURRENT_VERSION = 4;


         /* Activated test network at timestamp. */
        const uint32_t NEXUS_TESTNET_TIMELOCK = 1421250000;        //--- Nexus Testnet Activation:        1/14/2015 08:38:00 GMT - 6


        /* Activated main network at timestamp. */
        const uint32_t NEXUS_NETWORK_TIMELOCK = 1411510800;        //--- Nexus Network Launch:           09/23/2014 16:20:00 GMT - 6


        /* Lock for the Nexus block version upgrades. */
        const uint32_t TESTNET_BLOCK_VERSION_TIMELOCK[]   =
        {
            1412676000,        //--- Block Version 2 Testnet Activation:  10/07/2014 04:00:00 GMT - 6
            1421293891,        //--- Block Version 3 Testnet Activation:  01/15/2015 07:51:31 GMT - 6
            1421949600,        //--- Block Version 4 Testnet Activation:  05/10/2015 08:01:00 GMT - 6
            1536562800,        //--- Block Version 5 Testnet Activation:  09/10/2018 00:00:00 GMT - 7
            1537167600,        //--- Block Version 6 Testnet Activation:  09/17/2018 00:00:00 GMT - 7
            1572325200,        //--- Block Version 7 Testnet Activation:  10/28/2019 22:00:00 GMT - 7
            1582946400         //--- Block Version 8 Testnet Activation:  02/28/2020 20:20:00 GMT - 7
        };


        /* Lock for the Nexus block version upgrades. */
        const uint32_t NETWORK_BLOCK_VERSION_TIMELOCK[]   =
        {
            1412964000,        //--- Block Version 2 Activation:          10/10/2014 12:00:00 GMT - 6
            1421949600,        //--- Block Version 3 Activation:          01/22/2015 12:00:00 GMT - 6
            1438369200,        //--- Block Version 4 Activation:          07/31/2015 12:00:00 GMT - 7
            1536977460,        //--- Block Version 5 Activation:          09/14/2018 19:11:00 GMT - 7
            1538791860,        //--- Block Version 6 Activation:          10/05/2018 19:11:00 GMT - 7
            1573539371,        //--- Block Version 7 Activation:          11/11/2019 23:11:11 GMT - 6
            1584676140         //--- Block Version 8 Activation:          03/20/2020 03:49:00 GMT (03/19/2020 20:49:00 GMT -7)
        };


        /* Activated timestamp of first tritium block.*/
        const uint32_t NEXUS_TRITIUM_TIMELOCK = 1573566343; //---->       11/12/2019 06:45:43 GMT - 7


        /* Lock for the Nexus transaction version upgrades. */
        uint32_t TESTNET_TRANSACTION_VERSION_TIMELOCK[]   =
        {
            1572325200,        //--- Transaction Version 1 Testnet Activation:  10/28/2019 22:00:00 GMT - 7
            1582946400,        //--- Transaction Version 2 Testnet Activation:  02/28/2020 20:20:00 GMT - 7
            1631046780,        //--- Transaction Version 3 Testnet Activation:  09/07/2021 13:33:00 GMT - 7
            1631563980         //--- Transaction Version 4 Testnet Activation:  09/13/2021 13:13:00 GMT - 7
        };


        /* Lock for the Nexus transaction version upgrades. */
        uint32_t NETWORK_TRANSACTION_VERSION_TIMELOCK[]   =
        {
            1573539371,        //--- Transaction Version 1 Activation:      11/11/2019 23:11:11 GMT - 6
            1584676140,        //--- Transaction Version 2 Activation:      03/19/2020 20:49:00 GMT - 7
            1631301120,        //--- Transaction Version 3 Activation:      09/10/2021 12:12:00 GMT - 7
            1631819333         //--- Transaction Version 4 Activation:      09/16/2021 12:08:53 GMT - 7
        };


        /* Lock to activate each corresponding proof channel. */
        const uint32_t CHANNEL_TESTNET_TIMELOCK[] =
        {
            1421949600,        //--- Stake Testnet Activation:            05/10/2015 08:01:00 GMT - 6
            1411437371,        //--- Prime Testnet Activation:            09/22/2014 18:56:11 GMT - 6
            1411437371         //--- Hash Testnet Activation:             09/22/2014 18:56:11 GMT - 6
        };


        /* Lock to activate each corresponding proof channel. */
        const uint32_t CHANNEL_NETWORK_TIMELOCK[] =
        {
            1438369200,        //--- Stake Channel Activation:            07/31/2015 12:00:00 GMT - 7
            1411510800,        //--- Prime Channel Activation:            09/23/2014 16:20:00 GMT - 6
            1413914400         //--- Hash Channel Activation:             10/21/2014 12:00:00 GMT - 6
        };


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


        /* Test if a given block version is active at the time of the provided timestamp. */
        bool BlockVersionActive(const uint64_t nTimestamp, const uint32_t nVersion)
        {
            /* Check for version 0. */
            if(nVersion == 0)
                return false;

            /* Get current version. */
            uint32_t nCurrent = CurrentBlockVersion();

            /* Check for version after current. */
            if(nVersion > nCurrent)
                return false;

            /* Get the starting timelock for version. */
            uint64_t nStart = StartBlockTimelock(nVersion);
            if(nStart == 0)
                return false;

            /* Get the ending timelock for version. */
            uint64_t nEnd = EndBlockTimelock(nVersion);

            /* Current will not have an ending timelock, so that is valid */
            if((nVersion < nCurrent) && nEnd == 0)
                return false;

            /* Version is inactive if before starting timestamp */
            if(nTimestamp < nStart)
                return false;

            /* Version is inactive if more than one hour after ending timestamp */
            if((nVersion < nCurrent) && (nTimestamp - 3600) > nEnd)
                return false;

            return true;
        }


        /* Retrieve the current block version from mainnet or testnet. */
        uint32_t CurrentBlockVersion()
        {
            return config::fTestNet.load() ? TESTNET_BLOCK_CURRENT_VERSION : NETWORK_BLOCK_CURRENT_VERSION;
        }


        /* Retrieve the current block timelock activation from mainnet or testnet. */
        uint32_t CurrentBlockTimelock()
        {
            return config::fTestNet.load() ? TESTNET_BLOCK_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2]
                                           : NETWORK_BLOCK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2];
        }


        /* Retrieve the timelock activation for a given block version on mainnet or testnet. */
        uint64_t StartBlockTimelock(const uint32_t nVersion)
        {
            uint32_t nCurrent = CurrentBlockVersion();

            /* Version 0 or invalid versions that have no timelock activation */
            if((nVersion == 0) || (nVersion > nCurrent))
                return 0;

            /* Timelock activation for version 1 is the start of network */
            if(nVersion == 1)
                return config::fTestNet.load() ? NEXUS_TESTNET_TIMELOCK : NEXUS_NETWORK_TIMELOCK;

            /* For other versions, return the timelock activation time */
            return config::fTestNet.load() ? TESTNET_BLOCK_VERSION_TIMELOCK[nVersion - 2]
                                           : NETWORK_BLOCK_VERSION_TIMELOCK[nVersion - 2];
        }


        /* Retrieve the ending timelock for a given block version on mainnet or testnet. */
        uint64_t EndBlockTimelock(const uint32_t nVersion)
        {
            uint32_t nCurrent = CurrentBlockVersion();

            /* Version 0 or versions that have no ending timelock (including current) */
            if((nVersion == 0) || (nVersion >= nCurrent))
                return 0;

            /* For other versions, ending timelock is the timelock activation time for the next version */
            return config::fTestNet.load() ? TESTNET_BLOCK_VERSION_TIMELOCK[nVersion - 1]
                                           : NETWORK_BLOCK_VERSION_TIMELOCK[nVersion - 1];
        }


        /* Test if a given transaction version is active at the time of the provided timestamp. */
        bool TransactionVersionActive(const uint64_t nTimestamp, const uint32_t nVersion)
        {
            /* Check for version 0. */
            if(nVersion == 0)
                return false;

            /* Get current version. */
            const uint32_t nCurrent = CurrentTransactionVersion();

            /* Check for version after current. */
            if(nVersion > nCurrent)
                return false;

            /* Get the starting timelock for version. */
            const uint64_t nStart = StartTransactionTimelock(nVersion);
            if(nStart == 0)
                return false;

            /* Get the ending timelock for version. */
            const uint64_t nEnd = EndTransactionTimelock(nVersion);

            /* Current will not have an ending timelock, so that is valid */
            if((nVersion < nCurrent) && nEnd == 0)
                return false;

            /* Version is inactive if before starting timestamp */
            if(nTimestamp < nStart)
                return false;

            /* Version is inactive if more than one hour after ending timestamp */
            if((nVersion < nCurrent) && (nTimestamp - 3600) > nEnd)
                return false;

            return true;
        }


        /* Retrieve the current transaction version from mainnet or testnet. */
        uint32_t CurrentTransactionVersion()
        {
            return config::fTestNet.load() ? TESTNET_TRANSACTION_CURRENT_VERSION : NETWORK_TRANSACTION_CURRENT_VERSION;
        }


        /* Retrieve the current transaction timelock activation from mainnet or testnet. */
        uint32_t CurrentTransactionTimelock()
        {
            return config::fTestNet.load() ? TESTNET_BLOCK_VERSION_TIMELOCK[TESTNET_TRANSACTION_CURRENT_VERSION - 1]
                                           : NETWORK_BLOCK_VERSION_TIMELOCK[NETWORK_TRANSACTION_CURRENT_VERSION - 1];
        }


        /* Retrieve the timelock activation for a given transaction version on mainnet or testnet. */
        uint64_t StartTransactionTimelock(const uint32_t nVersion)
        {
            /* Get our current transaction version. */
            const uint32_t nCurrent =
                CurrentTransactionVersion();

            /* Version 0 or invalid versions that have no timelock activation */
            if((nVersion == 0) || (nVersion > nCurrent))
                return 0;

            /* For other versions, return the timelock activation time */
            return config::fTestNet.load() ? TESTNET_TRANSACTION_VERSION_TIMELOCK[nVersion - 1]
                                           : NETWORK_TRANSACTION_VERSION_TIMELOCK[nVersion - 1];
        }


        /* Retrieve the ending timelock for a given transaction version on mainnet or testnet. */
        uint64_t EndTransactionTimelock(const uint32_t nVersion)
        {
            /* Get the transaction version. */
            const uint32_t nCurrent =
                CurrentTransactionVersion();

            /* Version 0 or versions that have no ending timelock (including current) */
            if((nVersion == 0) || (nVersion >= nCurrent))
                return 0;

            /* For other versions, ending timelock is the timelock activation time for the next version */
            return config::fTestNet.load() ? TESTNET_TRANSACTION_VERSION_TIMELOCK[nVersion]
                                           : NETWORK_TRANSACTION_VERSION_TIMELOCK[nVersion];
        }
    }
}
