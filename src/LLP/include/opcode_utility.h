/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_OPCODE_UTILITY_H
#define NEXUS_LLP_INCLUDE_OPCODE_UTILITY_H

#include <cstdint>
#include <string>

/* Forward declarations to avoid heavy includes */
namespace LLP { class Packet; }
namespace LLP { class StatelessPacket; }

namespace LLP
{
namespace OpcodeUtility
{
    //=========================================================================
    // CANONICAL 8-BIT OPCODE DEFINITIONS
    // This is the SINGLE SOURCE OF TRUTH for all mining protocol opcodes.
    // DO NOT duplicate these values elsewhere.
    //=========================================================================
    namespace Opcodes
    {
        /* Data packets (0-127) - carry payload */
        static constexpr uint8_t BLOCK_DATA     = 0;
        static constexpr uint8_t SUBMIT_BLOCK   = 1;
        static constexpr uint8_t BLOCK_HEIGHT   = 2;
        static constexpr uint8_t SET_CHANNEL    = 3;
        static constexpr uint8_t BLOCK_REWARD   = 4;
        static constexpr uint8_t SET_COINBASE   = 5;
        static constexpr uint8_t GOOD_BLOCK     = 6;
        static constexpr uint8_t ORPHAN_BLOCK   = 7;
        static constexpr uint8_t CHECK_BLOCK    = 64;
        static constexpr uint8_t SUBSCRIBE      = 65;

        /* Request packets (128-203) - header-only, no payload */
        static constexpr uint8_t GET_BLOCK      = 129;
        static constexpr uint8_t GET_HEIGHT     = 130;
        static constexpr uint8_t GET_REWARD     = 131;
        static constexpr uint8_t CLEAR_MAP      = 132;
        static constexpr uint8_t GET_ROUND      = 133;

        /* Response packets (200-205) */
        static constexpr uint8_t BLOCK_ACCEPTED = 200;
        static constexpr uint8_t BLOCK_REJECTED = 201;
        static constexpr uint8_t COINBASE_SET   = 202;
        static constexpr uint8_t COINBASE_FAIL  = 203;
        static constexpr uint8_t NEW_ROUND      = 204;
        static constexpr uint8_t OLD_ROUND      = 205;

        /* Channel acknowledgment (206) - carries 1-byte payload */
        static constexpr uint8_t CHANNEL_ACK    = 206;

        /* Authentication packets (207-212) - carry payload */
        static constexpr uint8_t MINER_AUTH_INIT      = 207;
        static constexpr uint8_t MINER_AUTH_CHALLENGE = 208;
        static constexpr uint8_t MINER_AUTH_RESPONSE  = 209;
        static constexpr uint8_t MINER_AUTH_RESULT    = 210;
        static constexpr uint8_t SESSION_START        = 211;
        static constexpr uint8_t SESSION_KEEPALIVE    = 212;

        /* Reward binding packets (213-214) - carry encrypted payload */
        static constexpr uint8_t MINER_SET_REWARD     = 213;
        static constexpr uint8_t MINER_REWARD_RESULT  = 214;

        /* Push notification packets (216-218) */
        static constexpr uint8_t MINER_READY          = 216;  // header-only
        static constexpr uint8_t PRIME_BLOCK_AVAILABLE = 217; // 12-byte payload
        static constexpr uint8_t HASH_BLOCK_AVAILABLE  = 218; // 12-byte payload

        /* Alias opcodes for compatibility (map to same values as above) */
        static constexpr uint8_t NEW_PRIME_AVAILABLE = 217;  // Alias for PRIME_BLOCK_AVAILABLE
        static constexpr uint8_t NEW_HASH_AVAILABLE  = 218;  // Alias for HASH_BLOCK_AVAILABLE

        /* Session status query packets (219-220) - paired with SESSION_KEEPALIVE group */
        static constexpr uint8_t SESSION_STATUS     = 219; // 8-byte payload: session_id + status_flags
        static constexpr uint8_t SESSION_STATUS_ACK = 220; // 16-byte payload: session health response

        /* Generic packets */
        static constexpr uint8_t PING  = 253;
        static constexpr uint8_t CLOSE = 254;
    }

    //=========================================================================
    // 16-BIT STATELESS MIRROR-MAPPED OPCODES
    // Formula: statelessOpcode = 0xD000 | legacyOpcode
    // Absorbs everything from stateless_opcodes.h
    //=========================================================================
    namespace Stateless
    {
        /** Convert legacy 8-bit opcode to 16-bit stateless opcode */
        constexpr uint16_t Mirror(uint8_t legacyOpcode)
        {
            return 0xD000 | legacyOpcode;
        }

        /** Convert 16-bit stateless opcode back to legacy 8-bit */
        constexpr uint8_t Unmirror(uint16_t statelessOpcode)
        {
            return static_cast<uint8_t>(statelessOpcode & 0xFF);
        }

        /** Check if opcode is in valid stateless range (0xD000-0xD0FF) */
        constexpr bool IsStateless(uint16_t opcode)
        {
            return (opcode & 0xFF00) == 0xD000;
        }

        /* All mirror-mapped constants */
        /** BLOCK_DATA (0xD000): node→miner block template payload.
         *  Sent as the response to an explicit GET_BLOCK request, and as a recovery
         *  fallback when a GET_ROUND detects a changed best block (auto-send path).
         *  Distinct from GET_BLOCK (0xD081) which is the push-notification opcode
         *  used by SendStatelessTemplate() to proactively deliver new templates. */
        static constexpr uint16_t BLOCK_DATA     = Mirror(Opcodes::BLOCK_DATA);       // 0xD000
        static constexpr uint16_t SUBMIT_BLOCK   = Mirror(Opcodes::SUBMIT_BLOCK);     // 0xD001
        static constexpr uint16_t BLOCK_HEIGHT   = Mirror(Opcodes::BLOCK_HEIGHT);     // 0xD002
        static constexpr uint16_t SET_CHANNEL    = Mirror(Opcodes::SET_CHANNEL);      // 0xD003
        static constexpr uint16_t BLOCK_REWARD   = Mirror(Opcodes::BLOCK_REWARD);     // 0xD004
        static constexpr uint16_t SET_COINBASE   = Mirror(Opcodes::SET_COINBASE);     // 0xD005
        static constexpr uint16_t GOOD_BLOCK     = Mirror(Opcodes::GOOD_BLOCK);       // 0xD006
        static constexpr uint16_t ORPHAN_BLOCK   = Mirror(Opcodes::ORPHAN_BLOCK);     // 0xD007
        static constexpr uint16_t CHECK_BLOCK    = Mirror(Opcodes::CHECK_BLOCK);      // 0xD040
        static constexpr uint16_t SUBSCRIBE      = Mirror(Opcodes::SUBSCRIBE);        // 0xD041
        /** GET_BLOCK (0xD081): push-notification opcode used by SendStatelessTemplate().
         *  Carries 12-byte metadata (heights + nBits) followed by the 216-byte block template.
         *  Sent proactively to subscribed miners when a new block template is available.
         *  Not to be confused with BLOCK_DATA (0xD000) which is the response to an
         *  explicit GET_BLOCK request or the recovery auto-send fallback. */
        static constexpr uint16_t GET_BLOCK      = Mirror(Opcodes::GET_BLOCK);        // 0xD081
        static constexpr uint16_t GET_HEIGHT     = Mirror(Opcodes::GET_HEIGHT);       // 0xD082
        static constexpr uint16_t GET_REWARD     = Mirror(Opcodes::GET_REWARD);       // 0xD083
        static constexpr uint16_t CLEAR_MAP      = Mirror(Opcodes::CLEAR_MAP);        // 0xD084
        static constexpr uint16_t GET_ROUND      = Mirror(Opcodes::GET_ROUND);        // 0xD085
        static constexpr uint16_t BLOCK_ACCEPTED = Mirror(Opcodes::BLOCK_ACCEPTED);   // 0xD0C8
        static constexpr uint16_t BLOCK_REJECTED = Mirror(Opcodes::BLOCK_REJECTED);   // 0xD0C9
        static constexpr uint16_t COINBASE_SET   = Mirror(Opcodes::COINBASE_SET);     // 0xD0CA
        static constexpr uint16_t COINBASE_FAIL  = Mirror(Opcodes::COINBASE_FAIL);    // 0xD0CB
        static constexpr uint16_t NEW_ROUND      = Mirror(Opcodes::NEW_ROUND);        // 0xD0CC
        static constexpr uint16_t OLD_ROUND      = Mirror(Opcodes::OLD_ROUND);        // 0xD0CD
        static constexpr uint16_t CHANNEL_ACK    = Mirror(Opcodes::CHANNEL_ACK);      // 0xD0CE
        static constexpr uint16_t AUTH_INIT      = Mirror(Opcodes::MINER_AUTH_INIT);      // 0xD0CF
        static constexpr uint16_t AUTH_CHALLENGE = Mirror(Opcodes::MINER_AUTH_CHALLENGE); // 0xD0D0
        static constexpr uint16_t AUTH_RESPONSE  = Mirror(Opcodes::MINER_AUTH_RESPONSE);  // 0xD0D1
        static constexpr uint16_t AUTH_RESULT    = Mirror(Opcodes::MINER_AUTH_RESULT);    // 0xD0D2
        static constexpr uint16_t SESSION_START     = Mirror(Opcodes::SESSION_START);     // 0xD0D3
        static constexpr uint16_t SESSION_KEEPALIVE = Mirror(Opcodes::SESSION_KEEPALIVE); // 0xD0D4
        static constexpr uint16_t SET_REWARD     = Mirror(Opcodes::MINER_SET_REWARD);     // 0xD0D5
        static constexpr uint16_t REWARD_RESULT  = Mirror(Opcodes::MINER_REWARD_RESULT);  // 0xD0D6
        static constexpr uint16_t MINER_READY           = Mirror(Opcodes::MINER_READY);           // 0xD0D8
        static constexpr uint16_t PRIME_BLOCK_AVAILABLE = Mirror(Opcodes::PRIME_BLOCK_AVAILABLE); // 0xD0D9
        static constexpr uint16_t HASH_BLOCK_AVAILABLE  = Mirror(Opcodes::HASH_BLOCK_AVAILABLE);  // 0xD0DA
        static constexpr uint16_t PRIME_AVAILABLE       = PRIME_BLOCK_AVAILABLE;                   // Alias: 0xD0D9
        static constexpr uint16_t HASH_AVAILABLE        = HASH_BLOCK_AVAILABLE;                    // Alias: 0xD0DA
        static constexpr uint16_t SESSION_STATUS        = Mirror(Opcodes::SESSION_STATUS);         // 0xD0DB
        static constexpr uint16_t SESSION_STATUS_ACK    = Mirror(Opcodes::SESSION_STATUS_ACK);     // 0xD0DC
        /* Backward compat aliases */
        static constexpr uint16_t STATELESS_SESSION_STATUS     = SESSION_STATUS;
        static constexpr uint16_t STATELESS_SESSION_STATUS_ACK = SESSION_STATUS_ACK;
        static constexpr uint16_t PING  = Mirror(Opcodes::PING);   // 0xD0FD
        static constexpr uint16_t CLOSE = Mirror(Opcodes::CLOSE);  // 0xD0FE

        /* Backward compatibility aliases with STATELESS_ prefix */
        static constexpr uint16_t STATELESS_BLOCK_DATA     = BLOCK_DATA;
        static constexpr uint16_t STATELESS_SUBMIT_BLOCK   = SUBMIT_BLOCK;
        static constexpr uint16_t STATELESS_BLOCK_HEIGHT   = BLOCK_HEIGHT;
        static constexpr uint16_t STATELESS_SET_CHANNEL    = SET_CHANNEL;
        static constexpr uint16_t STATELESS_BLOCK_REWARD   = BLOCK_REWARD;
        static constexpr uint16_t STATELESS_SET_COINBASE   = SET_COINBASE;
        static constexpr uint16_t STATELESS_GOOD_BLOCK     = GOOD_BLOCK;
        static constexpr uint16_t STATELESS_ORPHAN_BLOCK   = ORPHAN_BLOCK;
        static constexpr uint16_t STATELESS_CHECK_BLOCK    = CHECK_BLOCK;
        static constexpr uint16_t STATELESS_SUBSCRIBE      = SUBSCRIBE;
        static constexpr uint16_t STATELESS_GET_BLOCK      = GET_BLOCK;
        static constexpr uint16_t STATELESS_GET_HEIGHT     = GET_HEIGHT;
        static constexpr uint16_t STATELESS_GET_REWARD     = GET_REWARD;
        static constexpr uint16_t STATELESS_CLEAR_MAP      = CLEAR_MAP;
        static constexpr uint16_t STATELESS_GET_ROUND      = GET_ROUND;
        static constexpr uint16_t STATELESS_BLOCK_ACCEPTED = BLOCK_ACCEPTED;
        static constexpr uint16_t STATELESS_BLOCK_REJECTED = BLOCK_REJECTED;
        static constexpr uint16_t STATELESS_COINBASE_SET   = COINBASE_SET;
        static constexpr uint16_t STATELESS_COINBASE_FAIL  = COINBASE_FAIL;
        static constexpr uint16_t STATELESS_NEW_ROUND      = NEW_ROUND;
        static constexpr uint16_t STATELESS_OLD_ROUND      = OLD_ROUND;
        static constexpr uint16_t STATELESS_CHANNEL_ACK    = CHANNEL_ACK;
        static constexpr uint16_t STATELESS_AUTH_INIT      = AUTH_INIT;
        static constexpr uint16_t STATELESS_AUTH_CHALLENGE = AUTH_CHALLENGE;
        static constexpr uint16_t STATELESS_AUTH_RESPONSE  = AUTH_RESPONSE;
        static constexpr uint16_t STATELESS_AUTH_RESULT    = AUTH_RESULT;
        static constexpr uint16_t STATELESS_SESSION_START     = SESSION_START;
        static constexpr uint16_t STATELESS_SESSION_KEEPALIVE = SESSION_KEEPALIVE;
        static constexpr uint16_t STATELESS_SET_REWARD     = SET_REWARD;
        static constexpr uint16_t STATELESS_REWARD_RESULT  = REWARD_RESULT;
        static constexpr uint16_t STATELESS_MINER_READY           = MINER_READY;
        static constexpr uint16_t STATELESS_PRIME_BLOCK_AVAILABLE = PRIME_BLOCK_AVAILABLE;
        static constexpr uint16_t STATELESS_HASH_BLOCK_AVAILABLE  = HASH_BLOCK_AVAILABLE;
        static constexpr uint16_t STATELESS_PING  = PING;
        static constexpr uint16_t STATELESS_CLOSE = CLOSE;

        //=====================================================================
        // UN-MIRRORED STATELESS-ONLY OPCODES
        // These opcodes do NOT follow the 0xD000|legacy formula.
        // They have NO legacy lane equivalent and are STATELESS PORT ONLY.
        // They all carry DATA payloads — never header-only.
        //=====================================================================

        /** KEEPALIVE_V2 (0xD100)
         *
         *  Stateless-only session keepalive sent by the Miner to the Node.
         *  Carries an 8-byte payload:
         *
         *  PAYLOAD (8 bytes, big-endian):
         *    [0-3]  uint32_t  sequence            Monotonic keepalive sequence number
         *    [4-7]  uint32_t  hashPrevBlock_lo32  Low 32 bits of miner's current prevHash (fork canary)
         *
         *  The node responds with a 32-byte KEEPALIVE_V2_ACK (0xD101).
         *  Miner must send KEEPALIVE_V2 at least every session_timeout seconds
         *  to prevent the stateless server from closing the authenticated session.
         *
         *  NOT available on legacy port 8323. Use bare PING (0xFD) there.
         **/
        static constexpr uint16_t KEEPALIVE_V2     = 0xD100;   // DATA-bearing, stateless-only

        /** KEEPALIVE_V2_ACK (0xD101)
         *
         *  Node response to KEEPALIVE_V2. Carries chain state telemetry for
         *  the miner's fork detection and height management systems.
         *  Also used for the legacy SESSION_KEEPALIVE v2 response (port 8323).
         *
         *  PAYLOAD (32 bytes):
         *    [0-3]   uint32_t  session_id          little-endian — session validation
         *    [4-7]   uint32_t  hashPrevBlock_lo32  big-endian — echo of miner's prevHash canary (0 on legacy path)
         *    [8-11]  uint32_t  unified_height      big-endian — node's unified block height
         *    [12-15] uint32_t  hash_tip_lo32       big-endian — low 32 bits of node's hashBestChain
         *    [16-19] uint32_t  prime_height        big-endian — node's Prime channel height
         *    [20-23] uint32_t  hash_height         big-endian — node's Hash channel height
         *    [24-27] uint32_t  stake_height        big-endian — node's Stake channel height
         *    [28-31] uint32_t  fork_score          big-endian — 0=healthy, >0=divergence magnitude
         *
         *  nBits is NOT included — miner obtains difficulty from the 12-byte GET_BLOCK response.
         **/
        static constexpr uint16_t KEEPALIVE_V2_ACK = 0xD101;   // DATA-bearing, stateless-only

        /** PING_DIAG (0xD0E0)
         *
         *  Colin AI Miner Agent 64-byte diagnostic PING. Sent by the node to
         *  each active miner at every emit_report() cycle (default: 60s).
         *  Carries a fully structured PingFrame (see colin_mining_agent.h).
         *
         *  PAYLOAD: 64 bytes (PingFrame, big-endian, cache-line aligned)
         *  Direction: Node → Miner
         *  NOT mirrored from legacy PING (0xFD) — completely separate opcode.
         *  NOT available on legacy port 8323.
         **/
        static constexpr uint16_t PING_DIAG        = 0xD0E0;   // DATA-bearing, stateless-only

        /** PONG_DIAG (0xD0E1)
         *
         *  Colin AI Miner response to PING_DIAG. Carries a 64-byte PongFrame
         *  with echoed sequence/timestamp and live miner telemetry.
         *
         *  PAYLOAD: 64 bytes (PongFrame, big-endian)
         *  Direction: Miner → Node
         *  NOT mirrored. NOT available on legacy port 8323.
         **/
        static constexpr uint16_t PONG_DIAG        = 0xD0E1;   // DATA-bearing, stateless-only
    }

    //=========================================================================
    // REJECTION REASON CODES
    // 1-byte payload sent with BLOCK_REJECTED (0xD0C9) to distinguish rejection type.
    // Decoded by the miner (solo.cpp) to track STALE vs invalid submissions.
    //=========================================================================
    enum class RejectionReason : uint8_t
    {
        STALE       = 1,  // hashPrevBlock != hashBestChain (Guard 2 mismatch)
        INVALID_POW = 2,  // Proof of work failed validation
        INVALID_SIG = 3,  // Falcon signature verification failed
        DUPLICATE   = 4,  // Block already submitted
        FORK        = 5,  // Block on an alternate chain
    };


    //=========================================================================
    // OPCODE CLASSIFICATION FUNCTIONS
    //=========================================================================

    /** Opcode range constants for stateless mining protocol
     *  
     *  The stateless miner port uses a dedicated opcode range 0xCF-0xDC (207-220)
     *  for mining-specific operations. These opcodes are NOT available on legacy
     *  mining ports and are part of the stateless authentication protocol.
     **/
    
    /** First stateless mining opcode (MINER_AUTH_INIT = 207 / 0xCF) */
    static constexpr uint8_t STATELESS_OPCODE_FIRST = 207;
    
    /** Last stateless mining opcode (SESSION_STATUS_ACK = 220 / 0xDC) */
    static constexpr uint8_t STATELESS_OPCODE_LAST = 220;
    
    /** Authentication opcode range (207-212)
     *  MINER_AUTH_INIT      = 207 (0xCF)
     *  MINER_AUTH_CHALLENGE = 208 (0xD0)
     *  MINER_AUTH_RESPONSE  = 209 (0xD1)
     *  MINER_AUTH_RESULT    = 210 (0xD2)
     *  SESSION_START        = 211 (0xD3)
     *  SESSION_KEEPALIVE    = 212 (0xD4)
     **/
    static constexpr uint8_t AUTH_OPCODE_FIRST = 207;
    static constexpr uint8_t AUTH_OPCODE_LAST = 212;
    
    /** Maximum packet length for any mining packet (2MB + overhead for full blocks) */
    static constexpr uint32_t MAX_ANY_PACKET_LENGTH = 3 * 1024 * 1024;  // 3 MB safety margin


    /** IsStatelessMiningOpcode
     *
     *  Determines if an opcode is part of the stateless mining protocol range.
     *  Stateless mining opcodes are in range 0xCF-0xDA (207-218).
     *
     *  @param[in] nOpcode The opcode to check
     *
     *  @return true if opcode is in stateless mining range, false otherwise
     *
     **/
    bool IsStatelessMiningOpcode(uint8_t nOpcode);


    /** IsAuthenticationOpcode
     *
     *  Determines if an opcode is part of the authentication protocol.
     *  Authentication opcodes are in range 207-212 (0xCF-0xD4).
     *
     *  @param[in] nOpcode The opcode to check
     *
     *  @return true if opcode is an authentication opcode, false otherwise
     *
     **/
    bool IsAuthenticationOpcode(uint8_t nOpcode);


    /** IsLegacyOpcodeAllowedOnStatelessPort
     *
     *  Determines if a legacy opcode is allowed on the stateless miner port.
     *  The stateless port supports a whitelist of legacy opcodes for backward
     *  compatibility with basic mining operations.
     *
     *  Allowed legacy opcodes:
     *  - SUBMIT_BLOCK (1)
     *  - GET_BLOCK (129)
     *  - GET_HEIGHT (130)
     *  - GET_REWARD (131)
     *  - GET_ROUND (133)
     *  - SET_CHANNEL (3)
     *  - PING (253)
     *
     *  @param[in] nOpcode The opcode to check
     *
     *  @return true if legacy opcode is allowed on stateless port, false otherwise
     *
     **/
    bool IsLegacyOpcodeAllowedOnStatelessPort(uint8_t nOpcode);


    /** GetOpcodeName
     *
     *  Returns a human-readable name for an 8-bit opcode for logging purposes.
     *
     *  @param[in] nOpcode The opcode to get the name for
     *
     *  @return String name of the opcode (e.g., "GET_BLOCK", "SUBMIT_BLOCK")
     *
     **/
    std::string GetOpcodeName(uint8_t nOpcode);


    /** GetOpcodeName16
     *
     *  Returns a human-readable name for a 16-bit stateless opcode for logging purposes.
     *  Handles both mirrored opcodes (0xD0xx) and un-mirrored stateless-only opcodes:
     *    - KEEPALIVE_V2 (0xD100), KEEPALIVE_V2_ACK (0xD101)
     *    - PING_DIAG (0xD0E0), PONG_DIAG (0xD0E1)
     *
     *  @param[in] nOpcode The 16-bit opcode to get the name for
     *
     *  @return String name of the opcode (e.g., "STATELESS_GET_BLOCK", "KEEPALIVE_V2")
     *
     **/
    std::string GetOpcodeName16(uint16_t nOpcode);


    /** IsHeaderOnlyRequest
     *
     *  Check if an opcode represents a header-only request packet.
     *  
     *  Header-only requests are commands that contain no data payload
     *  (LENGTH must be 0). These are typically GET-style operations that
     *  request information from the node without submitting any data.
     *
     *  @param[in] nOpcode The opcode to check
     *
     *  @return true if opcode is a header-only request, false otherwise
     *
     **/
    bool IsHeaderOnlyRequest(uint8_t nOpcode);


    /** HasDataPayload
     *
     *  Check if an opcode carries a data payload in the packet body.
     *  
     *  This replaces hardcoded range constants previously scattered in
     *  packet.h and other files. Provides a centralized classification
     *  of which opcodes carry payload data vs header-only packets.
     *
     *  @param[in] nOpcode The opcode to check
     *
     *  @return true if opcode carries data payload, false for header-only
     *
     **/
    bool HasDataPayload(uint8_t nOpcode);


    /** HasDataPayload16
     *
     *  Check if a 16-bit stateless opcode carries a data payload.
     *
     *  Covers both mirrored opcodes (0xD0xx) and un-mirrored stateless-only
     *  opcodes (0xD1xx). Un-mirrored data-bearing opcodes:
     *    - KEEPALIVE_V2     (0xD100): 8-byte payload
     *    - KEEPALIVE_V2_ACK (0xD101): 32-byte payload
     *    - PING_DIAG        (0xD0E0): 64-byte PingFrame
     *    - PONG_DIAG        (0xD0E1): 64-byte PongFrame
     *
     *  @param[in] nOpcode  The 16-bit stateless opcode to check
     *
     *  @return true if opcode carries data payload, false for header-only
     *
     **/
    bool HasDataPayload16(uint16_t nOpcode);


    /** IsUnmirroredStatelessOpcode
     *
     *  Check if a 16-bit opcode is a stateless-only un-mirrored opcode.
     *  Un-mirrored opcodes have NO legacy lane equivalent and must NEVER
     *  be sent or accepted on legacy port 8323.
     *
     *  Un-mirrored opcodes: KEEPALIVE_V2 (0xD100), KEEPALIVE_V2_ACK (0xD101),
     *  PING_DIAG (0xD0E0), PONG_DIAG (0xD0E1).
     *
     *  @param[in] nOpcode  The 16-bit opcode to check
     *
     *  @return true if this is a stateless-only un-mirrored opcode
     *
     **/
    bool IsUnmirroredStatelessOpcode(uint16_t nOpcode);


    /** GetExpectedPayloadSize16
     *
     *  Return the expected exact payload size (bytes) for opcodes with
     *  fixed-length payloads. Returns 0 for variable-length or header-only.
     *
     *  Fixed-size opcodes:
     *    - KEEPALIVE_V2     (0xD100): 8 bytes
     *    - KEEPALIVE_V2_ACK (0xD101): 32 bytes
     *    - PING_DIAG        (0xD0E0): 64 bytes
     *    - PONG_DIAG        (0xD0E1): 64 bytes
     *
     *  @param[in] nOpcode  The 16-bit opcode to query
     *
     *  @return Expected payload size in bytes, or 0 if variable/header-only
     *
     **/
    uint32_t GetExpectedPayloadSize16(uint16_t nOpcode);


    /** ValidatePacketLength
     *
     *  Validates that a packet's length is within acceptable bounds.
     *  Checks against maximum packet length and opcode-specific requirements.
     *
     *  @param[in] packet The packet to validate
     *  @param[out] strReason Optional reason string for validation failure
     *
     *  @return true if packet length is valid, false otherwise
     *
     **/
    bool ValidatePacketLength(const Packet& packet, std::string* strReason = nullptr);


    /** ValidatePacketLength
     *
     *  Validates that a stateless packet's length is within acceptable bounds.
     *  Enforces exact payload sizes for fixed-length un-mirrored stateless opcodes:
     *    - KEEPALIVE_V2: exactly 8 bytes
     *    - KEEPALIVE_V2_ACK: exactly 32 bytes
     *    - PING_DIAG / PONG_DIAG: exactly 64 bytes
     *
     *  @param[in] packet The stateless packet to validate
     *  @param[out] strReason Optional reason string for validation failure
     *
     *  @return true if packet length is valid, false otherwise
     *
     **/
    bool ValidatePacketLength(const StatelessPacket& packet, std::string* strReason = nullptr);


    /** ValidateOpcodeForStatelessPort
     *
     *  Validates that an opcode is allowed on the stateless miner port.
     *  Checks both stateless-specific opcodes and whitelisted legacy opcodes.
     *
     *  @param[in] nOpcode The opcode to validate
     *  @param[out] strReason Optional reason string for validation failure
     *
     *  @return true if opcode is allowed on stateless port, false otherwise
     *
     **/
    bool ValidateOpcodeForStatelessPort(uint8_t nOpcode, std::string* strReason = nullptr);


    /** ValidateAuthenticationGate
     *
     *  Validates that a packet respects the authentication gate.
     *  Non-authentication opcodes are rejected if the connection is not authenticated.
     *
     *  @param[in] nOpcode The opcode to check
     *  @param[in] fAuthenticated Whether the connection is authenticated
     *  @param[out] strReason Optional reason string for validation failure
     *
     *  @return true if packet passes authentication gate, false otherwise
     *
     **/
    bool ValidateAuthenticationGate(uint8_t nOpcode, bool fAuthenticated, 
                                    std::string* strReason = nullptr);


    /** ValidatePacket
     *
     *  Comprehensive packet validation for stateless miner connections.
     *  Combines length validation, opcode validation, and authentication gate checks.
     *
     *  @param[in] packet The packet to validate
     *  @param[in] fAuthenticated Whether the connection is authenticated
     *  @param[out] strReason Optional reason string for validation failure
     *
     *  @return true if packet is valid, false otherwise
     *
     **/
    bool ValidatePacket(const Packet& packet, bool fAuthenticated, 
                        std::string* strReason = nullptr);

} // namespace OpcodeUtility
} // namespace LLP

#endif
