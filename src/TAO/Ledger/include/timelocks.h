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

#include <inttypes.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** The network current block version. **/
        const uint32_t NETWORK_BLOCK_CURRENT_VERSION = 6;


        /** The testnet current block version. **/
        const uint32_t TESTNET_BLOCK_CURRENT_VERSION = 6;


        /** Nexus Testnet Timelock
         *
         *  Activated test network at timestamp.
         *
         **/
        const uint32_t NEXUS_TESTNET_TIMELOCK             = 1421250000;        //--- Nexus Testnet Activation:        1/14/2015 08:38:00 GMT - 6


        /** Nexus Network Timelock
         *
         *  Activated main network at timestamp.
         *
         **/
        const uint32_t NEXUS_NETWORK_TIMELOCK             = 1411510800;        //--- Nexus Network Launch:           09/23/2014 16:20:00 GMT - 6


        /** Testnet Version Timelock
         *
         *  Lock for the Nexus block version upgrades.
         *
         **/
        const uint32_t TESTNET_VERSION_TIMELOCK[]   =
        {
            1412676000,        //--- Block Version 2 Testnet Activation:  10/07/2014 04:00:00 GMT - 6
            1421293891,        //--- Block Version 3 Testnet Activation:  01/15/2015 07:51:31 GMT - 6
            1421949600,        //--- Block Version 4 Testnet Activation:  05/10/2015 08:01:00 GMT - 6
            1536562800,        //--- Block Version 5 Testnet Activation:  09/10/2018 00:00:00 GMT - 7
            1537167600,        //--- Block Version 6 Testnet Activation:  09/17/2018 00:00:00 GMT - 7
        };


        /** Network Version Timelock
         *
         *  Lock for the Nexus block version upgrades.
         *
         **/
        const uint32_t NETWORK_VERSION_TIMELOCK[]   =
        {
            1412964000,        //--- Block Version 2 Activation:          10/10/2014 12:00:00 GMT - 6
            1421949600,        //--- Block Version 3 Activation:          01/22/2015 12:00:00 GMT - 6
            1438369200,        //--- Block Version 4 Activation:          07/31/2015 12:00:00 GMT - 7
            1536977460,        //--- Block Version 5 Activation:          09/14/2018 19:11:00 GMT - 7
            1538791860,        //--- Block Version 6 Activation:          10/05/2018 19:11:00 GMT - 7
        };


        /** Testnet Channel Timelock
         *
         *  Lock to activate each corresponding proof channel.
         *
         **/
        const uint32_t CHANNEL_TESTNET_TIMELOCK[] =
        {
            1421949600,        //--- POS Testnet Activation:              05/10/2015 08:01:00 GMT - 6
            1411437371,        //--- CPU Testnet Activation:              09/22/2014 18:56:11 GMT - 6
            1411437371         //--- GPU Testnet Activation:              09/22/2014 18:56:11 GMT - 6
        };


        /** Mainnet Channel Timelock
         *
         *  Lock to activate each corresponding proof channel.
         *
         **/
        const uint32_t CHANNEL_NETWORK_TIMELOCK[] =
        {
            1438369200,        //--- POS Channel Activation:              07/31/2015 12:00:00 GMT - 7
            1411510800,        //--- CPU Channel Activation:              09/23/2014 16:20:00 GMT - 6
            1413914400         //--- GPU Channel Activation:              10/21/2014 12:00:00 GMT - 6
        };
    }
}

#endif
