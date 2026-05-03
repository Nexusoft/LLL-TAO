/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace LLP
{
namespace PacketFraming
{
    static constexpr uint32_t LEGACY_HEADER_BYTES    = 1;
    static constexpr uint32_t STATELESS_HEADER_BYTES = 2;
    static constexpr uint32_t LENGTH_BYTES           = 4;

    inline uint32_t DecodeLengthBytes(uint8_t nByte0, uint8_t nByte1, uint8_t nByte2, uint8_t nByte3)
    {
        return (static_cast<uint32_t>(nByte0) << 24)
             | (static_cast<uint32_t>(nByte1) << 16)
             | (static_cast<uint32_t>(nByte2) << 8)
             |  static_cast<uint32_t>(nByte3);
    }

    /** Decode a 4-byte big-endian packet payload length field. **/
    inline uint32_t DecodeLength(const std::array<uint8_t, LENGTH_BYTES>& vBytes)
    {
        return DecodeLengthBytes(vBytes[0], vBytes[1], vBytes[2], vBytes[3]);
    }

    /** Decode a 4-byte big-endian packet payload length field from a byte vector. **/
    inline uint32_t DecodeLength(const std::vector<uint8_t>& vBytes)
    {
        return DecodeLengthBytes(vBytes[0], vBytes[1], vBytes[2], vBytes[3]);
    }

    /** Append a 4-byte big-endian packet payload length field. **/
    inline void AppendLength(std::vector<uint8_t>& vBytes, uint32_t nLength)
    {
        vBytes.push_back(static_cast<uint8_t>(nLength >> 24));
        vBytes.push_back(static_cast<uint8_t>(nLength >> 16));
        vBytes.push_back(static_cast<uint8_t>(nLength >> 8));
        vBytes.push_back(static_cast<uint8_t>(nLength));
    }

    /** Append the 1-byte legacy packet header. **/
    inline void AppendLegacyHeader(std::vector<uint8_t>& vBytes, uint8_t nHeader)
    {
        vBytes.push_back(nHeader);
    }

    /** Append the 2-byte big-endian stateless packet header. **/
    inline void AppendStatelessHeader(std::vector<uint8_t>& vBytes, uint16_t nHeader)
    {
        vBytes.push_back(static_cast<uint8_t>(nHeader >> 8));
        vBytes.push_back(static_cast<uint8_t>(nHeader & 0xFF));
    }

    /** Build legacy wire bytes as [1-byte header][4-byte length][payload]. **/
    inline std::vector<uint8_t> BuildLegacyBytes(uint8_t nHeader, uint32_t nLength, const std::vector<uint8_t>& vData)
    {
        std::vector<uint8_t> vBytes;
        vBytes.reserve(LEGACY_HEADER_BYTES + LENGTH_BYTES + vData.size());
        AppendLegacyHeader(vBytes, nHeader);
        AppendLength(vBytes, nLength);
        vBytes.insert(vBytes.end(), vData.begin(), vData.end());
        return vBytes;
    }

    /** Build stateless wire bytes as [2-byte header][4-byte length][payload]. **/
    inline std::vector<uint8_t> BuildStatelessBytes(uint16_t nHeader, uint32_t nLength, const std::vector<uint8_t>& vData)
    {
        std::vector<uint8_t> vBytes;
        vBytes.reserve(STATELESS_HEADER_BYTES + LENGTH_BYTES + vData.size());
        AppendStatelessHeader(vBytes, nHeader);
        AppendLength(vBytes, nLength);
        vBytes.insert(vBytes.end(), vData.begin(), vData.end());
        return vBytes;
    }
}
}
