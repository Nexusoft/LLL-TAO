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
#include <LLC/include/flkey.h>

#include <string>
#include <cstdint>
#include <vector>

/* Forward declarations */
namespace TAO { namespace Ledger { class Block; class TritiumBlock; } }

namespace LLP
{
    /** TemplateMetadata
     * 
     *  Tracks metadata about mining templates for staleness detection.
     *  This structure contains all the information needed to validate
     *  whether a template is still valid for mining.
     * 
     *  Used by StatelessMinerConnection to implement comprehensive
     *  template lifecycle management with age-based and height-based
     *  expiration (PR #131: Mining Template Staleness Prevention).
     */
    struct TemplateMetadata
    {
        TAO::Ledger::Block* pBlock;      // Pointer to the block template
        uint64_t nCreationTime;           // When template was created (unified timestamp)
        uint32_t nHeight;                 // Blockchain height at creation
        uint512_t hashMerkleRoot;         // Expected merkle root for validation
        uint32_t nChannel;                // Mining channel (1=Prime, 2=Hash)
        
        /** Default constructor */
        TemplateMetadata() 
            : pBlock(nullptr)
            , nCreationTime(0)
            , nHeight(0)
            , hashMerkleRoot(0)
            , nChannel(0)
        {
        }
        
        /** Parameterized constructor */
        TemplateMetadata(TAO::Ledger::Block* pBlock_, uint64_t nCreationTime_, 
                        uint32_t nHeight_, const uint512_t& hashMerkleRoot_, uint32_t nChannel_)
            : pBlock(pBlock_)
            , nCreationTime(nCreationTime_)
            , nHeight(nHeight_)
            , hashMerkleRoot(hashMerkleRoot_)
            , nChannel(nChannel_)
        {
        }
        
        /** IsStale
         * 
         *  Check if template has exceeded maximum age.
         *  Templates older than MAX_TEMPLATE_AGE_SECONDS are considered stale.
         * 
         *  @param[in] nNow Current timestamp (optional, uses current time if 0)
         *  @return true if template is too old
         */
        bool IsStale(uint64_t nNow = 0) const;
        
        /** IsHeightValid
         * 
         *  Check if template height matches current blockchain height.
         *  Templates are only valid for the next block (nBestHeight + 1).
         * 
         *  @param[in] nCurrentHeight Current blockchain best height (optional, reads from ChainState if 0)
         *  @return true if template is for the correct height
         */
        bool IsHeightValid(uint32_t nCurrentHeight = 0) const;
    };

    /** MiningContext
     *
     *  Immutable snapshot of a miner's state at a given moment.
     *  All methods that modify state return a new MiningContext.
     *
     *  Phase 2 Extensions:
     *  - hashKeyID: Stable identifier derived from Falcon public key
     *  - hashGenesis: Tritium account identity this miner is mining for
     *
     *  Session Stability Extensions:
     *  - nSessionStart: When the session was established
     *  - nSessionTimeout: Configurable timeout for session expiry
     *  - nKeepaliveCount: Number of keepalives received (for monitoring)
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
        uint256_t hashGenesis;       // Phase 2: Tritium genesis hash (authentication)
        std::string strUserName;     // Phase 2: Username for trust-based addressing
        std::vector<uint8_t> vAuthNonce;  // Challenge nonce for authentication
        std::vector<uint8_t> vMinerPubKey; // Miner's Falcon public key
        uint64_t nSessionStart;      // Timestamp when session was established
        uint64_t nSessionTimeout;    // Session timeout in seconds (default 300s)
        uint32_t nKeepaliveCount;    // Number of keepalives received
        
        /* Reward address binding (set via MINER_SET_REWARD) */
        uint256_t hashRewardAddress; // Reward payout address (separate from auth genesis)
        bool fRewardBound;           // Whether reward address has been set

        /* ChaCha20 encryption state for secure communication */
        std::vector<uint8_t> vChaChaKey; // ChaCha20 session key derived from genesis
        bool fEncryptionReady;           // Whether ChaCha20 encryption is established

        /* Falcon version tracking (PR #122: Falcon Protocol Integration) */
        LLC::FalconVersion nFalconVersion;      // Detected Falcon version (512 or 1024)
        bool fFalconVersionDetected;            // Whether version has been detected
        std::vector<uint8_t> vchPhysicalSignature; // Optional Physical Falcon signature
        bool fPhysicalFalconPresent;            // Whether Physical Falcon signature is present

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

        /** WithUserName
         *
         *  Phase 2: Returns a new context with updated username for trust-based addressing.
         *
         **/
        MiningContext WithUserName(const std::string& strUserName_) const;

        /** WithNonce
         *
         *  Returns a new context with updated authentication nonce.
         *
         **/
        MiningContext WithNonce(const std::vector<uint8_t>& vNonce_) const;

        /** WithPubKey
         *
         *  Returns a new context with updated miner public key.
         *
         **/
        MiningContext WithPubKey(const std::vector<uint8_t>& vPubKey_) const;

        /** WithSessionStart
         *
         *  Returns a new context with updated session start timestamp.
         *
         **/
        MiningContext WithSessionStart(uint64_t nSessionStart_) const;

        /** WithSessionTimeout
         *
         *  Returns a new context with updated session timeout.
         *
         **/
        MiningContext WithSessionTimeout(uint64_t nSessionTimeout_) const;

        /** WithKeepaliveCount
         *
         *  Returns a new context with updated keepalive count.
         *
         **/
        MiningContext WithKeepaliveCount(uint32_t nKeepaliveCount_) const;

        /** WithRewardAddress
         *
         *  Returns a new context with updated reward address and bound flag.
         *
         *  @param[in] hashReward_ The reward address to set
         *
         **/
        MiningContext WithRewardAddress(const uint256_t& hashReward_) const;

        /** WithChaChaKey
         *
         *  Returns a new context with updated ChaCha20 encryption key.
         *  CRITICAL: This ensures encryption state is synchronized between
         *  Miner connection and StatelessMinerManager.
         *
         *  @param[in] vKey_ The ChaCha20 session key
         *
         **/
        MiningContext WithChaChaKey(const std::vector<uint8_t>& vKey_) const;

        /** WithFalconVersion
         *
         *  Returns a new context with updated Falcon version and detection flag.
         *  Used during MINER_AUTH to store auto-detected Falcon version.
         *
         *  @param[in] version_ The detected Falcon version (512 or 1024)
         *
         **/
        MiningContext WithFalconVersion(LLC::FalconVersion version_) const;

        /** WithPhysicalSignature
         *
         *  Returns a new context with Physical Falcon signature stored.
         *  Used during SUBMIT_BLOCK when Physical Falcon signature is present.
         *
         *  @param[in] vSig_ The Physical Falcon signature bytes
         *
         **/
        MiningContext WithPhysicalSignature(const std::vector<uint8_t>& vSig_) const;

        /** GetPayoutAddress
         *
         *  Get the payout address for rewards.
         *  Returns reward address if bound via MINER_SET_REWARD, otherwise genesis hash.
         *  Falls back to 0 if username-based addressing needs resolution.
         *
         *  @return Reward address, genesis hash, or 0 for username resolution
         *
         **/
        uint256_t GetPayoutAddress() const;

        /** HasValidPayout
         *
         *  Check if the context has a valid payout configuration.
         *
         *  @return true if genesis or username is set
         *
         **/
        bool HasValidPayout() const;

        /** IsSessionExpired
         *
         *  Check if the session has expired based on timeout.
         *
         *  @param[in] nNow Current timestamp (defaults to current time if 0)
         *
         *  @return true if session has expired
         *
         **/
        bool IsSessionExpired(uint64_t nNow = 0) const;

        /** GetSessionDuration
         *
         *  Get the duration of the current session in seconds.
         *
         *  @param[in] nNow Current timestamp (defaults to current time if 0)
         *
         *  @return Session duration in seconds
         *
         **/
        uint64_t GetSessionDuration(uint64_t nNow = 0) const;
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

        /** ProcessMinerAuthInit
         *
         *  Process MINER_AUTH_INIT packet - first step of authentication handshake.
         *  Stores miner's public key and generates challenge nonce.
         *
         *  Expected packet format:
         *  - [2 bytes] pubkey_len (big-endian)
         *  - [pubkey_len bytes] Falcon public key
         *  - [2 bytes] miner_id_len (big-endian)
         *  - [miner_id_len bytes] miner_id string (UTF-8)
         *
         *  @param[in] context Current miner state
         *  @param[in] packet Auth init packet
         *
         *  @return ProcessResult with challenge response and updated context
         *
         **/
        static ProcessResult ProcessMinerAuthInit(
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

        /** ProcessSetReward
         *
         *  Process MINER_SET_REWARD packet to bind reward address.
         *  Validates and stores the encrypted reward address for this mining session.
         *
         *  @param[in] context Current miner state
         *  @param[in] packet Set reward packet with encrypted address
         *
         *  @return ProcessResult with updated context or error
         *
         **/
        static ProcessResult ProcessSetReward(
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

        /** DecryptRewardPayload
         *
         *  Decrypts reward address payload using ChaCha20-Poly1305.
         *
         *  @param[in] vEncrypted The encrypted payload (nonce + ciphertext + tag)
         *  @param[in] vKey The ChaCha20 session key
         *  @param[out] vPlaintext The decrypted data
         *
         *  @return True if decryption succeeded
         *
         **/
        static bool DecryptRewardPayload(
            const std::vector<uint8_t>& vEncrypted,
            const std::vector<uint8_t>& vKey,
            std::vector<uint8_t>& vPlaintext
        );

        /** EncryptRewardResult
         *
         *  Encrypts reward result response using ChaCha20-Poly1305.
         *
         *  @param[in] vPlaintext The data to encrypt
         *  @param[in] vKey The ChaCha20 session key
         *
         *  @return Encrypted payload (nonce + ciphertext + tag)
         *
         **/
        static std::vector<uint8_t> EncryptRewardResult(
            const std::vector<uint8_t>& vPlaintext,
            const std::vector<uint8_t>& vKey
        );

        /** ValidateRewardAddress
         *
         *  Validates that a reward address is non-zero and properly formatted.
         *
         *  @param[in] hashReward The reward address to validate
         *
         *  @return True if valid format
         *
         **/
        static bool ValidateRewardAddress(const uint256_t& hashReward);
    };

} // namespace LLP

#endif
