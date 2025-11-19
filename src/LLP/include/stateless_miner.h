/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_STATELESS_MINER_H
#define NEXUS_LLP_INCLUDE_STATELESS_MINER_H

#include <LLP/packets/packet.h>
#include <LLC/types/uint1024.h>

#include <string>
#include <cstdint>
#include <vector>

namespace LLP
{
    /** MiningContext
     *
     *  Immutable snapshot of a miner's state at a given moment.
     *  All methods that modify state return a new MiningContext.
     *
     *  Phase 2 Extensions:
     *  - hashKeyID: Stable identifier derived from Falcon public key
     *  - hashGenesis: Tritium account identity this miner is mining for
     *
     **/
    struct MiningContext
    {
        uint32_t nChannel;           // Mining channel (1=Prime, 2=Hash)
        uint32_t nHeight;            // Current blockchain height
        uint64_t nTimestamp;         // Last activity timestamp
        std::string strAddress;      // Miner's network address
        uint32_t nProtocolVersion;   // Protocol version
        bool fAuthenticated;         // Whether Falcon auth succeeded
        uint32_t nSessionId;         // Unique session identifier
        uint256_t hashKeyID;         // Phase 2: Falcon key identifier
        uint256_t hashGenesis;       // Phase 2: Tritium genesis hash

        /** Default Constructor **/
        MiningContext();

        /** Parameterized Constructor **/
        MiningContext(
            uint32_t nChannel_,
            uint32_t nHeight_,
            uint64_t nTimestamp_,
            const std::string& strAddress_,
            uint32_t nProtocolVersion_,
            bool fAuthenticated_,
            uint32_t nSessionId_,
            const uint256_t& hashKeyID_,
            const uint256_t& hashGenesis_
        );

        /** WithChannel
         *
         *  Returns a new context with updated channel.
         *
         **/
        MiningContext WithChannel(uint32_t nChannel_) const;

        /** WithHeight
         *
         *  Returns a new context with updated height.
         *
         **/
        MiningContext WithHeight(uint32_t nHeight_) const;

        /** WithTimestamp
         *
         *  Returns a new context with updated timestamp.
         *
         **/
        MiningContext WithTimestamp(uint64_t nTimestamp_) const;

        /** WithAuth
         *
         *  Returns a new context with updated authentication status.
         *
         **/
        MiningContext WithAuth(bool fAuthenticated_) const;

        /** WithSession
         *
         *  Returns a new context with updated session ID.
         *
         **/
        MiningContext WithSession(uint32_t nSessionId_) const;

        /** WithKeyId
         *
         *  Phase 2: Returns a new context with updated Falcon key ID.
         *
         **/
        MiningContext WithKeyId(const uint256_t& hashKeyID_) const;

        /** WithGenesis
         *
         *  Phase 2: Returns a new context with updated Tritium genesis hash.
         *
         **/
        MiningContext WithGenesis(const uint256_t& hashGenesis_) const;
    };


    /** ProcessResult
     *
     *  Result of processing a packet in the stateless miner.
     *  Contains the updated context and optional response packet.
     *
     **/
    struct ProcessResult
    {
        const MiningContext context;
        const bool fSuccess;
        const std::string strError;
        const Packet response;

        /** Success
         *
         *  Create a successful result with updated context and response.
         *
         **/
        static ProcessResult Success(
            const MiningContext& ctx,
            const Packet& resp = Packet()
        );

        /** Error
         *
         *  Create an error result with unchanged context and error message.
         *
         **/
        static ProcessResult Error(
            const MiningContext& ctx,
            const std::string& error
        );

    private:
        ProcessResult(
            const MiningContext& ctx_,
            bool fSuccess_,
            const std::string& error_,
            const Packet& resp_
        );
    };


    /** StatelessMiner
     *
     *  Pure functional packet processor for mining connections.
     *  All methods are stateless and return ProcessResult with updated context.
     *
     *  Phase 2: Falcon-driven authentication is the primary auth mechanism.
     *
     **/
    class StatelessMiner
    {
    public:
        /** ProcessPacket
         *
         *  Main entry point for packet processing.
         *  Routes to appropriate handler based on packet type.
         *
         *  @param[in] context Current miner state
         *  @param[in] packet Incoming packet to process
         *
         *  @return ProcessResult with updated context and optional response
         *
         **/
        static ProcessResult ProcessPacket(
            const MiningContext& context,
            const Packet& packet
        );

        /** ProcessFalconResponse
         *
         *  Phase 2: Process Falcon authentication response.
         *  Verifies Falcon signature and updates context with auth status.
         *
         *  Expected packet format:
         *  - Falcon public key (variable length)
         *  - Falcon signature (variable length)
         *  - Optional: Tritium genesis hash (32 bytes)
         *
         *  @param[in] context Current miner state
         *  @param[in] packet Falcon auth response packet
         *
         *  @return ProcessResult with updated context (authenticated, keyID, genesis)
         *
         **/
        static ProcessResult ProcessFalconResponse(
            const MiningContext& context,
            const Packet& packet
        );

        /** ProcessSessionStart
         *
         *  Phase 2: Process session start request.
         *  Requires prior Falcon authentication.
         *
         *  @param[in] context Current miner state
         *  @param[in] packet Session start packet
         *
         *  @return ProcessResult with updated session parameters
         *
         **/
        static ProcessResult ProcessSessionStart(
            const MiningContext& context,
            const Packet& packet
        );

        /** ProcessSetChannel
         *
         *  Process channel selection packet.
         *
         *  @param[in] context Current miner state
         *  @param[in] packet Set channel packet
         *
         *  @return ProcessResult with updated channel
         *
         **/
        static ProcessResult ProcessSetChannel(
            const MiningContext& context,
            const Packet& packet
        );

        /** ProcessSessionKeepalive
         *
         *  Phase 2: Process session keepalive.
         *  Updates session timestamp to maintain connection.
         *
         *  @param[in] context Current miner state
         *  @param[in] packet Keepalive packet
         *
         *  @return ProcessResult with updated timestamp
         *
         **/
        static ProcessResult ProcessSessionKeepalive(
            const MiningContext& context,
            const Packet& packet
        );

    private:
        /** BuildAuthMessage
         *
         *  Constructs the message that should be signed for Falcon auth.
         *  Currently uses: address + timestamp
         *  TODO: Add challenge nonce for enhanced security
         *
         *  @param[in] context Current miner state
         *
         *  @return Message bytes for signature verification
         *
         **/
        static std::vector<uint8_t> BuildAuthMessage(const MiningContext& context);
    };

} // namespace LLP

#endif
