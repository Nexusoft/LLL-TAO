/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_SESSION_STATUS_H
#define NEXUS_LLP_INCLUDE_SESSION_STATUS_H

#include <cstdint>
#include <vector>

namespace LLP
{
namespace SessionStatus
{
    //=========================================================================
    // Lane health flags (ACK bytes [4-7])
    //=========================================================================
    static constexpr uint32_t LANE_PRIMARY_ALIVE   = 0x01; // bit 0: stateless lane up
    static constexpr uint32_t LANE_SECONDARY_ALIVE = 0x02; // bit 1: legacy lane up
    static constexpr uint32_t LANE_SIM_LINK_ACTIVE = 0x04; // bit 2: dual-lane SIM Link mode
    static constexpr uint32_t LANE_AUTHENTICATED   = 0x08; // bit 3: session authenticated

    //=========================================================================
    // Miner status flags (REQUEST bytes [4-7], echo'd in ACK [12-15])
    //=========================================================================
    static constexpr uint32_t MINER_DEGRADED       = 0x01; // bit 0: degraded mode active
    static constexpr uint32_t MINER_HAS_TEMPLATE   = 0x02; // bit 1: valid template held
    static constexpr uint32_t MINER_WORKERS_ACTIVE = 0x04; // bit 2: workers running
    static constexpr uint32_t MINER_SECONDARY_UP   = 0x08; // bit 3: secondary lane connected

    //=========================================================================
    // Wire payload sizes
    //=========================================================================
    static constexpr uint32_t REQUEST_PAYLOAD_SIZE = 8;   // miner -> node
    static constexpr uint32_t ACK_PAYLOAD_SIZE     = 16;  // node -> miner

    //=========================================================================
    // SessionStatusRequest — 8-byte miner -> node payload
    //=========================================================================
    struct SessionStatusRequest
    {
        uint32_t session_id{0};    // bytes [0-3] little-endian
        uint32_t status_flags{0};  // bytes [4-7] big-endian: MINER_* flags

        /** Serialize to 8-byte wire format */
        std::vector<uint8_t> Serialize() const
        {
            std::vector<uint8_t> v;
            v.reserve(8);
            // session_id: little-endian
            v.push_back(static_cast<uint8_t>( session_id        & 0xFF));
            v.push_back(static_cast<uint8_t>((session_id >>  8) & 0xFF));
            v.push_back(static_cast<uint8_t>((session_id >> 16) & 0xFF));
            v.push_back(static_cast<uint8_t>((session_id >> 24) & 0xFF));
            // status_flags: big-endian
            v.push_back(static_cast<uint8_t>((status_flags >> 24) & 0xFF));
            v.push_back(static_cast<uint8_t>((status_flags >> 16) & 0xFF));
            v.push_back(static_cast<uint8_t>((status_flags >>  8) & 0xFF));
            v.push_back(static_cast<uint8_t>( status_flags        & 0xFF));
            return v;
        }

        /** Parse from wire bytes. Returns false if buffer too small. */
        bool Parse(const std::vector<uint8_t>& data)
        {
            if(data.size() < 8) return false;
            session_id =  static_cast<uint32_t>(data[0])
                       | (static_cast<uint32_t>(data[1]) <<  8)
                       | (static_cast<uint32_t>(data[2]) << 16)
                       | (static_cast<uint32_t>(data[3]) << 24);
            status_flags = (static_cast<uint32_t>(data[4]) << 24)
                         | (static_cast<uint32_t>(data[5]) << 16)
                         | (static_cast<uint32_t>(data[6]) <<  8)
                         |  static_cast<uint32_t>(data[7]);
            return true;
        }
    };

    //=========================================================================
    // SessionStatusAck — 16-byte node -> miner payload
    //=========================================================================
    struct SessionStatusAck
    {
        uint32_t session_id{0};         // bytes [0-3]  little-endian, authoritative node session_id
        uint32_t lane_health_flags{0};  // bytes [4-7]  big-endian: LANE_* flags
        uint32_t uptime_seconds{0};     // bytes [8-11] big-endian: node uptime
        uint32_t status_echo_flags{0};  // bytes [12-15] big-endian: echo of request flags

        /** Build ACK from node session state + echoed miner status flags */
        static SessionStatusAck Build(uint32_t session_id_,
                                      uint32_t lane_health,
                                      uint32_t uptime_secs,
                                      uint32_t echo_flags)
        {
            SessionStatusAck a;
            a.session_id        = session_id_;
            a.lane_health_flags = lane_health;
            a.uptime_seconds    = uptime_secs;
            a.status_echo_flags = echo_flags;
            return a;
        }

        /** Serialize to 16-byte wire format */
        std::vector<uint8_t> Serialize() const
        {
            std::vector<uint8_t> v;
            v.reserve(16);
            auto p32le = [&](uint32_t x) {
                v.push_back(static_cast<uint8_t>( x        & 0xFF));
                v.push_back(static_cast<uint8_t>((x >>  8) & 0xFF));
                v.push_back(static_cast<uint8_t>((x >> 16) & 0xFF));
                v.push_back(static_cast<uint8_t>((x >> 24) & 0xFF));
            };
            auto p32be = [&](uint32_t x) {
                v.push_back(static_cast<uint8_t>((x >> 24) & 0xFF));
                v.push_back(static_cast<uint8_t>((x >> 16) & 0xFF));
                v.push_back(static_cast<uint8_t>((x >>  8) & 0xFF));
                v.push_back(static_cast<uint8_t>( x        & 0xFF));
            };
            p32le(session_id);
            p32be(lane_health_flags);
            p32be(uptime_seconds);
            p32be(status_echo_flags);
            return v;
        }

        /** Parse from wire bytes. Returns false if buffer too small. */
        bool Parse(const std::vector<uint8_t>& data)
        {
            if(data.size() < 16) return false;
            session_id =  static_cast<uint32_t>(data[0])
                       | (static_cast<uint32_t>(data[1]) <<  8)
                       | (static_cast<uint32_t>(data[2]) << 16)
                       | (static_cast<uint32_t>(data[3]) << 24);
            lane_health_flags = (static_cast<uint32_t>(data[4])  << 24)
                              | (static_cast<uint32_t>(data[5])  << 16)
                              | (static_cast<uint32_t>(data[6])  <<  8)
                              |  static_cast<uint32_t>(data[7]);
            uptime_seconds    = (static_cast<uint32_t>(data[8])  << 24)
                              | (static_cast<uint32_t>(data[9])  << 16)
                              | (static_cast<uint32_t>(data[10]) <<  8)
                              |  static_cast<uint32_t>(data[11]);
            status_echo_flags = (static_cast<uint32_t>(data[12]) << 24)
                              | (static_cast<uint32_t>(data[13]) << 16)
                              | (static_cast<uint32_t>(data[14]) <<  8)
                              |  static_cast<uint32_t>(data[15]);
            return true;
        }
    };

    /** BuildAckPayload convenience wrapper */
    inline std::vector<uint8_t> BuildAckPayload(uint32_t session_id,
                                                 uint32_t lane_health,
                                                 uint32_t uptime_secs,
                                                 uint32_t echo_flags)
    {
        return SessionStatusAck::Build(session_id, lane_health, uptime_secs, echo_flags).Serialize();
    }

} // namespace SessionStatus
} // namespace LLP

#endif
