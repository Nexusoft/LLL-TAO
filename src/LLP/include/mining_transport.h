/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_MINING_TRANSPORT_H
#define NEXUS_LLP_INCLUDE_MINING_TRANSPORT_H

#include <LLP/include/opcode_utility.h>
#include <LLP/packets/packet.h>
#include <LLP/packets/stateless_packet.h>

#include <cstdint>
#include <vector>

namespace LLP
{
    enum class MiningTransportLane : uint8_t
    {
        LEGACY_8323,
        STATELESS_9323
    };


    /** MiningTransport
     *
     *  Thin transport-encoding boundary for shared mining semantics.
     *  Business logic should choose the canonical legacy semantic opcode;
     *  this adapter is the only place that decides whether the wire frame is
     *  legacy 8-bit Packet or stateless 16-bit StatelessPacket.
     *
     **/
    class MiningTransport
    {
    public:
        static std::vector<uint8_t> BuildResponseBytes(
            MiningTransportLane eLane,
            uint8_t nLegacyOpcode,
            const std::vector<uint8_t>& vData = std::vector<uint8_t>())
        {
            if(eLane == MiningTransportLane::STATELESS_9323)
            {
                StatelessPacket packet(OpcodeUtility::Stateless::Mirror(nLegacyOpcode));
                packet.LENGTH = static_cast<uint32_t>(vData.size());
                packet.DATA = vData;
                return packet.GetBytes();
            }

            Packet packet(nLegacyOpcode);
            packet.LENGTH = static_cast<uint32_t>(vData.size());
            packet.DATA = vData;
            return packet.GetBytes();
        }
    };
}

#endif
