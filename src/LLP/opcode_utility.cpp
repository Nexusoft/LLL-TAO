/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/opcode_utility.h>
#include <LLP/packets/packet.h>
#include <LLP/packets/stateless_packet.h>
#include <LLP/include/falcon_constants.h>

#include <Util/include/debug.h>

#include <sstream>

namespace LLP
{
namespace OpcodeUtility
{


    bool IsStatelessMiningOpcode(uint8_t nOpcode)
    {
        /* Stateless mining opcodes are in range 0xCF-0xDA (207-218) */
        return (nOpcode >= STATELESS_OPCODE_FIRST && nOpcode <= STATELESS_OPCODE_LAST);
    }


    bool IsAuthenticationOpcode(uint8_t nOpcode)
    {
        /* Authentication opcodes are in range 207-212 (0xCF-0xD4) */
        return (nOpcode >= AUTH_OPCODE_FIRST && nOpcode <= AUTH_OPCODE_LAST);
    }


    bool IsLegacyOpcodeAllowedOnStatelessPort(uint8_t nOpcode)
    {
        /* Whitelist of legacy opcodes allowed on stateless port
         * Note: Stateless-specific opcodes (207-218) are handled separately
         * by IsStatelessMiningOpcode() and should not be listed here.
         */
        switch(nOpcode)
        {
            /* Block operations */
            case Opcodes::SUBMIT_BLOCK:   // 1
            case Opcodes::GET_BLOCK:      // 129
            case Opcodes::GET_HEIGHT:     // 130
            case Opcodes::GET_REWARD:     // 131
            case Opcodes::GET_ROUND:      // 133
            
            /* Channel management */
            case Opcodes::SET_CHANNEL:    // 3
            
            /* Generic */
            case Opcodes::PING:           // 253
                return true;
            
            default:
                return false;
        }
    }


    std::string GetOpcodeName(uint8_t nOpcode)
    {
        /* Map opcodes to human-readable names */
        switch(nOpcode)
        {
            /* Data packets */
            case Opcodes::BLOCK_DATA:              return "BLOCK_DATA";
            case Opcodes::SUBMIT_BLOCK:            return "SUBMIT_BLOCK";
            case Opcodes::BLOCK_HEIGHT:            return "BLOCK_HEIGHT";
            case Opcodes::SET_CHANNEL:             return "SET_CHANNEL";
            case Opcodes::BLOCK_REWARD:            return "BLOCK_REWARD";
            
            /* Request packets */
            case Opcodes::GET_BLOCK:               return "GET_BLOCK";
            case Opcodes::GET_HEIGHT:              return "GET_HEIGHT";
            case Opcodes::GET_REWARD:              return "GET_REWARD";
            case Opcodes::GET_ROUND:               return "GET_ROUND";
            
            /* Response packets */
            case Opcodes::BLOCK_ACCEPTED:          return "BLOCK_ACCEPTED";
            case Opcodes::BLOCK_REJECTED:          return "BLOCK_REJECTED";
            case Opcodes::NEW_ROUND:               return "NEW_ROUND";
            case Opcodes::OLD_ROUND:               return "OLD_ROUND";
            case Opcodes::CHANNEL_ACK:             return "CHANNEL_ACK";
            
            /* Authentication packets */
            case Opcodes::MINER_AUTH_INIT:         return "MINER_AUTH_INIT";
            case Opcodes::MINER_AUTH_CHALLENGE:    return "MINER_AUTH_CHALLENGE";
            case Opcodes::MINER_AUTH_RESPONSE:     return "MINER_AUTH_RESPONSE";
            case Opcodes::MINER_AUTH_RESULT:       return "MINER_AUTH_RESULT";
            case Opcodes::SESSION_START:           return "SESSION_START";
            case Opcodes::SESSION_KEEPALIVE:       return "SESSION_KEEPALIVE";
            
            /* Reward binding packets */
            case Opcodes::MINER_SET_REWARD:        return "MINER_SET_REWARD";
            case Opcodes::MINER_REWARD_RESULT:     return "MINER_REWARD_RESULT";
            
            /* Push notification packets */
            case Opcodes::MINER_READY:             return "MINER_READY";
            case Opcodes::PRIME_BLOCK_AVAILABLE:   return "PRIME_BLOCK_AVAILABLE / NEW_PRIME_AVAILABLE";
            case Opcodes::HASH_BLOCK_AVAILABLE:    return "HASH_BLOCK_AVAILABLE / NEW_HASH_AVAILABLE";
            
            /* Generic packets */
            case Opcodes::PING:                    return "PING";
            case Opcodes::CLOSE:                   return "CLOSE";
            
            default:
            {
                std::ostringstream oss;
                oss << "UNKNOWN(0x" << std::hex << static_cast<uint32_t>(nOpcode) << ")";
                return oss.str();
            }
        }
    }


    /** IsHeaderOnlyRequest
     *
     *  Check if an opcode represents a header-only request packet.
     *  
     *  Header-only requests are commands that contain no data payload
     *  (LENGTH must be 0). These are typically GET-style operations that
     *  request information from the node without submitting any data.
     *
     *  @param[in] nOpcode The packet opcode to check.
     *
     *  @return True if the opcode is a header-only request, false otherwise.
     *
     **/
    bool IsHeaderOnlyRequest(uint8_t nOpcode)
    {
        /* These opcodes are requests that contain no data payload */
        return (nOpcode == Opcodes::GET_BLOCK || nOpcode == Opcodes::GET_HEIGHT ||
                nOpcode == Opcodes::GET_REWARD || nOpcode == Opcodes::GET_ROUND ||
                nOpcode == Opcodes::PING || nOpcode == Opcodes::CLOSE ||
                nOpcode == Opcodes::MINER_READY);
    }


    /** GetMaxPayloadSize
     *
     *  Get the maximum allowed payload size for opcodes with fixed-length constraints.
     *  
     *  Returns the maximum number of bytes allowed in a packet's data payload
     *  for opcodes that have known, fixed upper bounds. These limits prevent
     *  resource exhaustion attacks and ensure protocol compliance.
     *
     *  Maximum sizes are based on protocol specifications:
     *  - SET_CHANNEL: 4 bytes (supports both 1-byte and legacy 4-byte LE format)
     *  - SESSION_KEEPALIVE: 8 bytes (4-byte session ID + optional padding)
     *  - SESSION_START: 8 bytes (optional timeout value + padding)
     *  - MINER_AUTH_CHALLENGE: 40 bytes (length field + 32-byte nonce + padding)
     *  - MINER_AUTH_RESULT: 10 bytes (status + session ID + error code + padding)
     *
     *  @param[in] nOpcode The packet opcode to check.
     *
     *  @return Maximum payload size in bytes, or -1 if no fixed maximum applies.
     *          Returning -1 means the opcode is validated elsewhere (e.g., SUBMIT_BLOCK)
     *          or has no specific maximum constraint.
     *
     **/
    static int32_t GetMaxPayloadSize(uint8_t nOpcode)
    {
        /* Return -1 for opcodes with no fixed maximum (already validated elsewhere) */
        if(nOpcode == Opcodes::SET_CHANNEL)
            return 4;  /* 1-byte or 4-byte LE format */
        if(nOpcode == Opcodes::SESSION_KEEPALIVE)
            return 8;  /* 4-byte session ID + padding */
        if(nOpcode == Opcodes::SESSION_START)
            return 8;  /* optional timeout + padding */
        if(nOpcode == Opcodes::MINER_AUTH_CHALLENGE)
            return 40; /* nonce length field + 32-byte nonce + padding */
        if(nOpcode == Opcodes::MINER_AUTH_RESULT)
            return 10; /* status byte + session ID + error code + padding */
        
        return -1; /* No fixed maximum */
    }


    bool ValidatePacketLength(const Packet& packet, std::string* strReason)
    {
        /* Check against maximum packet length */
        if(packet.LENGTH > MAX_ANY_PACKET_LENGTH)
        {
            if(strReason)
            {
                std::ostringstream oss;
                oss << "Packet length " << packet.LENGTH << " exceeds maximum " 
                    << MAX_ANY_PACKET_LENGTH << " bytes";
                *strReason = oss.str();
            }
            return false;
        }
        
        /* Check packet-specific length constraints based on opcode */
        uint8_t nOpcode = packet.HEADER;
        
        /* Authentication packets have specific size constraints */
        if(nOpcode == Opcodes::MINER_AUTH_INIT)
        {
            /* MINER_AUTH_INIT: pubkey_len(2) + pubkey(897-1793) + miner_id_len(2) + miner_id(0-256) + genesis(0-32)
             * Minimum: 901 bytes (Falcon-512, no miner_id, no genesis)
             * Maximum: 2085 bytes (Falcon-1024, max miner_id, with genesis)
             */
            if(packet.LENGTH < FalconConstants::MINER_AUTH_INIT_MIN || 
               packet.LENGTH > FalconConstants::MINER_AUTH_INIT_MAX)
            {
                if(strReason)
                {
                    std::ostringstream oss;
                    oss << "MINER_AUTH_INIT length " << packet.LENGTH 
                        << " outside valid range [" << FalconConstants::MINER_AUTH_INIT_MIN
                        << ", " << FalconConstants::MINER_AUTH_INIT_MAX << "]";
                    *strReason = oss.str();
                }
                return false;
            }
        }
        else if(nOpcode == Opcodes::MINER_AUTH_RESPONSE)
        {
            /* MINER_AUTH_RESPONSE: pubkey_len(2) + pubkey(897-1793) + timestamp(8) + sig_len(2) + sig(600-1577)
             * With optional ChaCha20 encryption overhead: +28 bytes
             * Minimum: ~1510 bytes (Falcon-512, no encryption)
             * Maximum: 3410 bytes (Falcon-1024 encrypted)
             */
            const size_t MIN_AUTH_RESPONSE = FalconConstants::LENGTH_FIELD_SIZE + 
                                              FalconConstants::FALCON512_PUBKEY_SIZE + 
                                              FalconConstants::TIMESTAMP_SIZE + 
                                              FalconConstants::LENGTH_FIELD_SIZE + 
                                              FalconConstants::FALCON512_SIG_MIN;  // ~1509 bytes
            
            if(packet.LENGTH < MIN_AUTH_RESPONSE || 
               packet.LENGTH > FalconConstants::AUTH_RESPONSE_ENCRYPTED_MAX)
            {
                if(strReason)
                {
                    std::ostringstream oss;
                    oss << "MINER_AUTH_RESPONSE length " << packet.LENGTH 
                        << " outside valid range [" << MIN_AUTH_RESPONSE
                        << ", " << FalconConstants::AUTH_RESPONSE_ENCRYPTED_MAX << "]";
                    *strReason = oss.str();
                }
                return false;
            }
        }
        else if(nOpcode == Opcodes::SUBMIT_BLOCK)
        {
            /* SUBMIT_BLOCK: Full block (up to 2MB) + timestamp + signature + encryption
             * Maximum: SUBMIT_BLOCK_WRAPPER_ENCRYPTED_MAX bytes
             */
            if(packet.LENGTH > FalconConstants::SUBMIT_BLOCK_WRAPPER_ENCRYPTED_MAX)
            {
                if(strReason)
                {
                    std::ostringstream oss;
                    oss << "SUBMIT_BLOCK length " << packet.LENGTH 
                        << " exceeds maximum " << FalconConstants::SUBMIT_BLOCK_WRAPPER_ENCRYPTED_MAX;
                    *strReason = oss.str();
                }
                return false;
            }
        }
        
        /* Validate header-only requests have no payload */
        if(IsHeaderOnlyRequest(nOpcode))
        {
            if(packet.LENGTH != 0)
            {
                if(strReason)
                {
                    std::ostringstream oss;
                    oss << GetOpcodeName(nOpcode) << " expects no data payload, received " 
                        << packet.LENGTH << " bytes";
                    *strReason = oss.str();
                }
                return false;
            }
        }
        
        /* Validate fixed-size opcodes don't exceed maximum */
        int32_t nMaxSize = GetMaxPayloadSize(nOpcode);
        if(nMaxSize >= 0 && packet.LENGTH > static_cast<uint32_t>(nMaxSize))
        {
            if(strReason)
            {
                std::ostringstream oss;
                oss << GetOpcodeName(nOpcode) << " payload size " << packet.LENGTH 
                    << " exceeds maximum of " << nMaxSize << " bytes";
                *strReason = oss.str();
            }
            return false;
        }
        
        return true;
    }


    bool ValidateOpcodeForStatelessPort(uint8_t nOpcode, std::string* strReason)
    {
        /* Check if it's a stateless-specific opcode */
        if(IsStatelessMiningOpcode(nOpcode))
            return true;
        
        /* Check if it's an allowed authentication opcode (207-212) */
        if(IsAuthenticationOpcode(nOpcode))
            return true;
        
        /* Check if it's a whitelisted legacy opcode */
        if(IsLegacyOpcodeAllowedOnStatelessPort(nOpcode))
            return true;
        
        /* Opcode not allowed on stateless port */
        if(strReason)
        {
            std::ostringstream oss;
            oss << "Opcode " << GetOpcodeName(nOpcode) << " (0x" << std::hex 
                << static_cast<uint32_t>(nOpcode) << ") not allowed on stateless port";
            *strReason = oss.str();
        }
        
        return false;
    }


    bool ValidateAuthenticationGate(uint8_t nOpcode, bool fAuthenticated, 
                                    std::string* strReason)
    {
        /* Authentication opcodes are always allowed (they're part of the auth flow) */
        if(IsAuthenticationOpcode(nOpcode))
            return true;
        
        /* PING is always allowed regardless of authentication */
        if(nOpcode == Opcodes::PING)
            return true;
        
        /* All other opcodes require authentication */
        if(!fAuthenticated)
        {
            if(strReason)
            {
                std::ostringstream oss;
                oss << "Opcode " << GetOpcodeName(nOpcode) 
                    << " requires authentication";
                *strReason = oss.str();
            }
            return false;
        }
        
        return true;
    }


    bool ValidatePacket(const Packet& packet, bool fAuthenticated, 
                        std::string* strReason)
    {
        uint8_t nOpcode = packet.HEADER;
        
        /* Validate packet length */
        if(!ValidatePacketLength(packet, strReason))
            return false;
        
        /* Validate opcode is allowed on stateless port */
        if(!ValidateOpcodeForStatelessPort(nOpcode, strReason))
            return false;
        
        /* Validate authentication gate */
        if(!ValidateAuthenticationGate(nOpcode, fAuthenticated, strReason))
            return false;
        
        return true;
    }


    std::string GetOpcodeName16(uint16_t nOpcode)
    {
        /* Un-mirrored stateless-only opcodes — check before IsStateless() */
        if(nOpcode == Stateless::KEEPALIVE_V2)      return "KEEPALIVE_V2";
        if(nOpcode == Stateless::KEEPALIVE_V2_ACK)  return "KEEPALIVE_V2_ACK";
        if(nOpcode == Stateless::PING_DIAG)         return "PING_DIAG";
        if(nOpcode == Stateless::PONG_DIAG)         return "PONG_DIAG";

        /* Check if it's a mirrored stateless opcode */
        if(Stateless::IsStateless(nOpcode))
        {
            /* Convert to legacy opcode and get name, then prefix with STATELESS_ */
            uint8_t legacyOpcode = Stateless::Unmirror(nOpcode);
            std::string strLegacyName = GetOpcodeName(legacyOpcode);
            
            /* If it's an unknown opcode, return hex format */
            if(strLegacyName.find("UNKNOWN") != std::string::npos)
            {
                std::ostringstream oss;
                oss << "STATELESS_UNKNOWN(0x" << std::hex << nOpcode << ")";
                return oss.str();
            }
            
            return "STATELESS_" + strLegacyName;
        }
        
        /* Not a stateless opcode */
        std::ostringstream oss;
        oss << "INVALID_16BIT(0x" << std::hex << nOpcode << ")";
        return oss.str();
    }


    bool HasDataPayload(uint8_t nOpcode)
    {
        /* Traditional data packets (0-127) carry payload */
        if(nOpcode < 128)
            return true;
        
        /* Mining round response packets carry height data (204-205) */
        if(nOpcode >= Opcodes::NEW_ROUND && nOpcode <= Opcodes::OLD_ROUND)
            return true;
        
        /* Channel acknowledgment requires data (channel number) */
        if(nOpcode == Opcodes::CHANNEL_ACK)
            return true;
        
        /* Falcon authentication packets (207-212) carry crypto payloads */
        if(nOpcode >= Opcodes::MINER_AUTH_INIT && nOpcode <= Opcodes::SESSION_KEEPALIVE)
            return true;
        
        /* Reward binding packets (213-214) carry encrypted payloads */
        if(nOpcode >= Opcodes::MINER_SET_REWARD && nOpcode <= Opcodes::MINER_REWARD_RESULT)
            return true;
        
        /* Push notification packets (217-218) carry block metadata */
        if(nOpcode == Opcodes::PRIME_BLOCK_AVAILABLE || nOpcode == Opcodes::HASH_BLOCK_AVAILABLE)
            return true;
        
        /* NOTE: Legacy PING (0xFD = 253) is header-only — no payload.
         * The stateless PING_DIAG (0xD0E0) is data-bearing but is
         * handled by HasDataPayload16(), not this function. */
        
        /* All other opcodes are header-only */
        return false;
    }


    bool HasDataPayload16(uint16_t nOpcode)
    {
        /* Un-mirrored stateless-only data-bearing opcodes */
        if(nOpcode == Stateless::KEEPALIVE_V2)      return true;  // 0xD100: 8B
        if(nOpcode == Stateless::KEEPALIVE_V2_ACK)  return true;  // 0xD101: 28B
        if(nOpcode == Stateless::PING_DIAG)         return true;  // 0xD0E0: 64B
        if(nOpcode == Stateless::PONG_DIAG)         return true;  // 0xD0E1: 64B

        /* For mirrored opcodes, unmirror and delegate to HasDataPayload(uint8_t) */
        if(Stateless::IsStateless(nOpcode))
            return HasDataPayload(Stateless::Unmirror(nOpcode));

        return false;
    }


    bool IsUnmirroredStatelessOpcode(uint16_t nOpcode)
    {
        switch(nOpcode)
        {
            case Stateless::KEEPALIVE_V2:      // 0xD100
            case Stateless::KEEPALIVE_V2_ACK:  // 0xD101
            case Stateless::PING_DIAG:         // 0xD0E0
            case Stateless::PONG_DIAG:         // 0xD0E1
                return true;
            default:
                return false;
        }
    }


    uint32_t GetExpectedPayloadSize16(uint16_t nOpcode)
    {
        switch(nOpcode)
        {
            case Stateless::KEEPALIVE_V2:      return 8;   // sequence(4) + hashPrevBlock_lo32(4), miner→node
            case Stateless::KEEPALIVE_V2_ACK:  return 28;  // 7×uint32_t chain state telemetry, node→miner
            case Stateless::PING_DIAG:         return 64;  // PingFrame
            case Stateless::PONG_DIAG:         return 64;  // PongFrame
            default:                           return 0;   // variable or header-only
        }
    }


    bool ValidatePacketLength(const StatelessPacket& packet, std::string* strReason)
    {
        /* Check against maximum packet length */
        if(packet.LENGTH > MAX_ANY_PACKET_LENGTH)
        {
            if(strReason)
            {
                std::ostringstream oss;
                oss << "Packet length " << packet.LENGTH << " exceeds maximum "
                    << MAX_ANY_PACKET_LENGTH << " bytes";
                *strReason = oss.str();
            }
            return false;
        }

        /* Fixed-size payload enforcement for un-mirrored stateless opcodes */
        uint32_t nExpected = GetExpectedPayloadSize16(packet.HEADER);
        if(nExpected > 0 && packet.LENGTH != nExpected)
        {
            if(strReason)
            {
                std::ostringstream oss;
                oss << "Fixed-payload opcode 0x" << std::hex << packet.HEADER
                    << " requires exactly " << std::dec << nExpected
                    << " bytes, got " << packet.LENGTH;
                *strReason = oss.str();
            }
            return false;
        }

        return true;
    }


} // namespace OpcodeUtility
} // namespace LLP
