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
#include <string>

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


    /** FormatHashLaneSummary
     *
     *  Formats the hash-lane portion of a MINER_PUSH_SUMMARY log line.
     *
     *  When fBroadcast is false (e.g. Hash channel suppressed by dedup), returns
     *  " | hash not_broadcast" so operators/monitors can distinguish that case
     *  from a real Hash broadcast that happened to observe zero wrong-channel
     *  skips.  The "(expected if all miners are Prime)" annotation is only
     *  appended when a Hash broadcast actually ran and recorded wrong-channel
     *  skips.
     *
     *  @param[in] fBroadcast  True if the Hash channel broadcast was attempted.
     *  @param[in] tHash       Notification counters from the Hash broadcast.
     *
     **/
    inline std::string FormatHashLaneSummary(bool fBroadcast,
                                             const ChannelNotifyResult& tHash)
    {
        if(!fBroadcast)
            return " | hash not_broadcast";

        std::string strSummary =
            " | hash notified=" + std::to_string(tHash.nNotified) +
            " wrong_channel=" + std::to_string(tHash.nSkippedWrongChannel);

        if(tHash.nSkippedWrongChannel > 0)
            strSummary += " (expected if all miners are Prime)";

        strSummary +=
            " polling=" + std::to_string(tHash.nSkippedPolling) +
            " disconnected=" + std::to_string(tHash.nSkippedDisconnected);

        return strSummary;
    }

} // namespace LLP

#endif
