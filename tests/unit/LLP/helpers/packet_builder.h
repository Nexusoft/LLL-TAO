/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLP/packets/packet.h>
#include <LLC/include/random.h>
#include <Util/include/hex.h>

#include <vector>
#include <cstdint>

namespace TestFixtures
{
    /** Packet Type Constants for Testing **/
    namespace PacketTypes
    {
        const LLP::Packet::message_t SET_CHANNEL = 3;
        const LLP::Packet::message_t GET_BLOCK = 4;
        const LLP::Packet::message_t BLOCK_DATA = 5;
        const LLP::Packet::message_t SUBMIT_BLOCK = 6;
        const LLP::Packet::message_t BLOCK_ACCEPTED = 7;
        const LLP::Packet::message_t BLOCK_REJECTED = 8;
        const LLP::Packet::message_t GET_HEIGHT = 9;
        const LLP::Packet::message_t GET_REWARD = 10;
        const LLP::Packet::message_t GET_ROUND = 11;
        
        /* Phase 2 - Falcon Authentication */
        const LLP::Packet::message_t MINER_AUTH_INIT = 207;
        const LLP::Packet::message_t MINER_AUTH_CHALLENGE = 208;
        const LLP::Packet::message_t MINER_AUTH_RESPONSE = 209;
        const LLP::Packet::message_t MINER_AUTH_RESULT = 210;
        
        /* Phase 2 - Session Management */
        const LLP::Packet::message_t SESSION_START = 211;
        const LLP::Packet::message_t SESSION_KEEPALIVE = 212;
        
        /* Phase 2 - Reward Binding */
        const LLP::Packet::message_t MINER_SET_REWARD = 213;
        const LLP::Packet::message_t REWARD_BOUND = 214;
    }
    
    /** PacketBuilder
     *
     *  Fluent builder for creating test packets.
     *  Simplifies packet construction in tests.
     *
     **/
    class PacketBuilder
    {
    private:
        LLP::Packet packet;
        
    public:
        PacketBuilder() : packet() {}
        
        PacketBuilder(LLP::Packet::message_t header) : packet()
        {
            packet.HEADER = header;
        }
        
        PacketBuilder& WithHeader(LLP::Packet::message_t header)
        {
            packet.HEADER = header;
            return *this;
        }
        
        PacketBuilder& WithLength(uint32_t length)
        {
            packet.LENGTH = length;
            return *this;
        }
        
        PacketBuilder& WithData(const std::vector<uint8_t>& data)
        {
            packet.DATA = data;
            packet.LENGTH = data.size();
            return *this;
        }
        
        PacketBuilder& WithData(const uint8_t* data, size_t len)
        {
            packet.DATA.assign(data, data + len);
            packet.LENGTH = len;
            return *this;
        }
        
        PacketBuilder& WithRandomData(size_t len)
        {
            std::vector<uint8_t> data(len);
            LLC::GetRand(data);
            return WithData(data);
        }
        
        PacketBuilder& AppendByte(uint8_t byte)
        {
            packet.DATA.push_back(byte);
            packet.LENGTH = packet.DATA.size();
            return *this;
        }
        
        PacketBuilder& AppendUint32(uint32_t value)
        {
            packet.DATA.push_back((value >> 0) & 0xFF);
            packet.DATA.push_back((value >> 8) & 0xFF);
            packet.DATA.push_back((value >> 16) & 0xFF);
            packet.DATA.push_back((value >> 24) & 0xFF);
            packet.LENGTH = packet.DATA.size();
            return *this;
        }
        
        PacketBuilder& AppendUint64(uint64_t value)
        {
            for(int i = 0; i < 8; i++)
            {
                packet.DATA.push_back((value >> (i * 8)) & 0xFF);
            }
            packet.LENGTH = packet.DATA.size();
            return *this;
        }
        
        PacketBuilder& AppendHash(const uint256_t& hash)
        {
            std::vector<uint8_t> vHash = hash.GetBytes();
            packet.DATA.insert(packet.DATA.end(), vHash.begin(), vHash.end());
            packet.LENGTH = packet.DATA.size();
            return *this;
        }
        
        PacketBuilder& AppendBytes(const std::vector<uint8_t>& bytes)
        {
            packet.DATA.insert(packet.DATA.end(), bytes.begin(), bytes.end());
            packet.LENGTH = packet.DATA.size();
            return *this;
        }
        
        LLP::Packet Build()
        {
            return packet;
        }
    };
    
    /** Packet Factory Functions **/
    
    /** CreateSetChannelPacket
     *
     *  Creates a SET_CHANNEL packet.
     *
     *  @param[in] nChannel The channel to set (1=Prime, 2=Hash, 3=Private)
     *  @return A SET_CHANNEL packet
     *
     **/
    inline LLP::Packet CreateSetChannelPacket(uint32_t nChannel)
    {
        return PacketBuilder(PacketTypes::SET_CHANNEL)
            .AppendUint32(nChannel)
            .Build();
    }
    
    /** CreateGetBlockPacket
     *
     *  Creates a GET_BLOCK packet.
     *
     *  @return A GET_BLOCK packet
     *
     **/
    inline LLP::Packet CreateGetBlockPacket()
    {
        return PacketBuilder(PacketTypes::GET_BLOCK)
            .Build();
    }
    
    /** CreateSubmitBlockPacket
     *
     *  Creates a SUBMIT_BLOCK packet.
     *
     *  @param[in] hashMerkleRoot The merkle root of the block
     *  @param[in] nNonce The mining nonce
     *  @return A SUBMIT_BLOCK packet
     *
     **/
    inline LLP::Packet CreateSubmitBlockPacket(const uint512_t& hashMerkleRoot, uint64_t nNonce)
    {
        return PacketBuilder(PacketTypes::SUBMIT_BLOCK)
            .AppendBytes(hashMerkleRoot.GetBytes())
            .AppendUint64(nNonce)
            .Build();
    }
    
    /** CreateGetHeightPacket
     *
     *  Creates a GET_HEIGHT packet.
     *
     *  @return A GET_HEIGHT packet
     *
     **/
    inline LLP::Packet CreateGetHeightPacket()
    {
        return PacketBuilder(PacketTypes::GET_HEIGHT)
            .Build();
    }
    
    /** CreateGetRewardPacket
     *
     *  Creates a GET_REWARD packet.
     *
     *  @return A GET_REWARD packet
     *
     **/
    inline LLP::Packet CreateGetRewardPacket()
    {
        return PacketBuilder(PacketTypes::GET_REWARD)
            .Build();
    }
    
    /** CreateGetRoundPacket
     *
     *  Creates a GET_ROUND packet.
     *
     *  @return A GET_ROUND packet
     *
     **/
    inline LLP::Packet CreateGetRoundPacket()
    {
        return PacketBuilder(PacketTypes::GET_ROUND)
            .Build();
    }
    
    /** CreateAuthInitPacket
     *
     *  Creates a MINER_AUTH_INIT packet.
     *
     *  @param[in] vPubKey The Falcon public key
     *  @param[in] hashGenesis The genesis hash for authentication
     *  @return A MINER_AUTH_INIT packet
     *
     **/
    inline LLP::Packet CreateAuthInitPacket(const std::vector<uint8_t>& vPubKey, const uint256_t& hashGenesis)
    {
        PacketBuilder builder(PacketTypes::MINER_AUTH_INIT);
        
        /* Append pubkey length and pubkey */
        builder.AppendUint32(vPubKey.size());
        builder.AppendBytes(vPubKey);
        
        /* Append genesis hash */
        builder.AppendHash(hashGenesis);
        
        return builder.Build();
    }
    
    /** CreateAuthResponsePacket
     *
     *  Creates a MINER_AUTH_RESPONSE packet.
     *
     *  @param[in] vSignature The Falcon signature
     *  @return A MINER_AUTH_RESPONSE packet
     *
     **/
    inline LLP::Packet CreateAuthResponsePacket(const std::vector<uint8_t>& vSignature)
    {
        return PacketBuilder(PacketTypes::MINER_AUTH_RESPONSE)
            .AppendUint32(vSignature.size())
            .AppendBytes(vSignature)
            .Build();
    }
    
    /** CreateSessionKeepalivePacket
     *
     *  Creates a SESSION_KEEPALIVE packet.
     *
     *  @return A SESSION_KEEPALIVE packet
     *
     **/
    inline LLP::Packet CreateSessionKeepalivePacket()
    {
        return PacketBuilder(PacketTypes::SESSION_KEEPALIVE)
            .Build();
    }
    
    /** CreateSetRewardPacket
     *
     *  Creates a MINER_SET_REWARD packet (encrypted).
     *
     *  @param[in] vEncryptedData The encrypted reward address data
     *  @return A MINER_SET_REWARD packet
     *
     **/
    inline LLP::Packet CreateSetRewardPacket(const std::vector<uint8_t>& vEncryptedData)
    {
        return PacketBuilder(PacketTypes::MINER_SET_REWARD)
            .AppendUint32(vEncryptedData.size())
            .AppendBytes(vEncryptedData)
            .Build();
    }
    
    /** CreateMalformedPacket
     *
     *  Creates a packet with malformed data for error testing.
     *
     *  @param[in] header The packet header
     *  @param[in] dataSize The size of random data to append
     *  @return A malformed packet
     *
     **/
    inline LLP::Packet CreateMalformedPacket(LLP::Packet::message_t header, size_t dataSize = 100)
    {
        return PacketBuilder(header)
            .WithRandomData(dataSize)
            .Build();
    }
    
    /** CreateOversizedPacket
     *
     *  Creates a packet with excessive data length.
     *
     *  @param[in] header The packet header
     *  @return An oversized packet
     *
     **/
    inline LLP::Packet CreateOversizedPacket(LLP::Packet::message_t header)
    {
        return PacketBuilder(header)
            .WithRandomData(10000000)  // 10MB of random data
            .Build();
    }
    
    /** CreateEmptyPacket
     *
     *  Creates a packet with no data.
     *
     *  @param[in] header The packet header
     *  @return An empty packet
     *
     **/
    inline LLP::Packet CreateEmptyPacket(LLP::Packet::message_t header)
    {
        return PacketBuilder(header)
            .Build();
    }
    
} // namespace TestFixtures
