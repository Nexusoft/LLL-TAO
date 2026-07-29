/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_CHANNEL_NOTIFY_RESULT_H
#define NEXUS_LLP_INCLUDE_CHANNEL_NOTIFY_RESULT_H

#include <cstdint>

namespace LLP
{

    /** ChannelNotifyResult
     *
     *  Captures per-lane, per-channel miner notification counts from a single
     *  NotifyChannelMiners() call.  Threaded through BroadcastStatelessChannel /
     *  BroadcastLegacyChannel so the dispatcher can aggregate them into a single
     *  MINER_PUSH_SUMMARY log line without any extra traversal.
     *
     *  Fields
     *  ------
     *  nNotified           : miners that received a push packet.
     *  nSkippedWrongChannel: miners on a different channel (Prime vs Hash).
     *                        For example, all Prime-only stateless miners are skipped
     *                        when broadcasting the Hash channel — this is EXPECTED and
     *                        does NOT indicate a push failure.
     *  nSkippedPolling     : miners using GET_ROUND polling (not subscribed to push).
     *  nSkippedDisconnected: miners whose session was marked inactive.
     *
     **/
    struct ChannelNotifyResult
    {
        uint32_t nNotified{0};
        uint32_t nSkippedWrongChannel{0};
        uint32_t nSkippedPolling{0};
        uint32_t nSkippedDisconnected{0};
    };

} // namespace LLP

#endif
