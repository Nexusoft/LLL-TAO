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

namespace LLP
{
namespace OpcodeUtility
{
    /** Opcode range constants for stateless mining protocol
     *  
     *  The stateless miner port uses a dedicated opcode range 0xCF-0xDA (207-218)
     *  for mining-specific operations. These opcodes are NOT available on legacy
     *  mining ports and are part of the stateless authentication protocol.
     **/
    
    /** First stateless mining opcode (MINER_AUTH_INIT = 207 / 0xCF) */
    static const uint8_t STATELESS_OPCODE_FIRST = 207;
    
    /** Last stateless mining opcode (HASH_BLOCK_AVAILABLE = 218 / 0xDA) */
    static const uint8_t STATELESS_OPCODE_LAST = 218;
    
    /** Authentication opcode range (207-212)
     *  MINER_AUTH_INIT      = 207 (0xCF)
     *  MINER_AUTH_CHALLENGE = 208 (0xD0)
     *  MINER_AUTH_RESPONSE  = 209 (0xD1)
     *  MINER_AUTH_RESULT    = 210 (0xD2)
     *  SESSION_START        = 211 (0xD3)
     *  SESSION_KEEPALIVE    = 212 (0xD4)
     **/
    static const uint8_t AUTH_OPCODE_FIRST = 207;
    static const uint8_t AUTH_OPCODE_LAST = 212;
    
    /** Maximum packet length for any mining packet (2MB + overhead for full blocks) */
    static const uint32_t MAX_ANY_PACKET_LENGTH = 3 * 1024 * 1024;  // 3 MB safety margin


    /** IsStatelessMiningOpcode
     *
     *  Determines if an opcode is part of the stateless mining protocol range.
     *  Stateless mining opcodes are in range 0xD0-0xDA (208-218).
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
     *  Returns a human-readable name for an opcode for logging purposes.
     *
     *  @param[in] nOpcode The opcode to get the name for
     *
     *  @return String name of the opcode (e.g., "GET_BLOCK", "SUBMIT_BLOCK")
     *
     **/
    std::string GetOpcodeName(uint8_t nOpcode);


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
