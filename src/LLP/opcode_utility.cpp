/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/opcode_utility.h>
#include <LLP/packets/packet.h>
#include <LLP/include/falcon_constants.h>

#include <Util/include/debug.h>

#include <sstream>

namespace LLP
{
namespace OpcodeUtility
{
    /** Opcode constants from miner.h for reference
     *  
     *  NOTE: These are duplicated here rather than including miner.h to avoid
     *  circular dependencies. The opcode_utility is intended to be a lightweight
     *  utility that can be included by miner.h and other files without creating
     *  include cycles. The values must be kept in sync with src/LLP/types/miner.h.
     **/
    namespace Opcodes
    {
        /* Data packets */
        const uint8_t BLOCK_DATA     = 0;
        const uint8_t SUBMIT_BLOCK   = 1;
        const uint8_t BLOCK_HEIGHT   = 2;
        const uint8_t SET_CHANNEL    = 3;
        const uint8_t BLOCK_REWARD   = 4;
        
        /* Request packets */
        const uint8_t GET_BLOCK      = 129;
        const uint8_t GET_HEIGHT     = 130;
        const uint8_t GET_REWARD     = 131;
        const uint8_t GET_ROUND      = 133;
        
        /* Response packets */
        const uint8_t BLOCK_ACCEPTED = 200;
        const uint8_t BLOCK_REJECTED = 201;
        const uint8_t NEW_ROUND      = 204;
        const uint8_t OLD_ROUND      = 205;
        const uint8_t CHANNEL_ACK    = 206;
        
        /* Authentication packets */
        const uint8_t MINER_AUTH_INIT      = 207;
        const uint8_t MINER_AUTH_CHALLENGE = 208;
        const uint8_t MINER_AUTH_RESPONSE  = 209;
        const uint8_t MINER_AUTH_RESULT    = 210;
        const uint8_t SESSION_START        = 211;
        const uint8_t SESSION_KEEPALIVE    = 212;
        
        /* Reward binding packets */
        const uint8_t MINER_SET_REWARD     = 213;
        const uint8_t MINER_REWARD_RESULT  = 214;
        
        /* Push notification packets */
        const uint8_t MINER_READY          = 216;
        const uint8_t PRIME_BLOCK_AVAILABLE = 217;
        const uint8_t HASH_BLOCK_AVAILABLE  = 218;
        
        /* Alias opcodes for compatibility (map to same values as above) */
        const uint8_t NEW_PRIME_AVAILABLE = 217;  // Alias for PRIME_BLOCK_AVAILABLE
        const uint8_t NEW_HASH_AVAILABLE  = 218;  // Alias for HASH_BLOCK_AVAILABLE
        
        /* Generic packets */
        const uint8_t PING           = 253;
        const uint8_t CLOSE          = 254;
    }


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


    /** Helper: Check if opcode is a header-only request (no payload allowed) */
    static bool IsHeaderOnlyRequest(uint8_t opcode)
    {
        /* These opcodes are requests that contain no data payload */
        return (opcode == Opcodes::GET_BLOCK || opcode == Opcodes::GET_HEIGHT ||
                opcode == Opcodes::GET_REWARD || opcode == Opcodes::GET_ROUND ||
                opcode == Opcodes::PING || opcode == Opcodes::CLOSE ||
                opcode == Opcodes::MINER_READY);
    }


    /** Helper: Get maximum allowed payload size for fixed-length opcodes */
    static int32_t GetMaxPayloadSize(uint8_t opcode)
    {
        /* Return -1 for opcodes with no fixed maximum (already validated elsewhere) */
        if(opcode == Opcodes::SET_CHANNEL)
            return 4;  /* 1-byte or 4-byte LE format */
        if(opcode == Opcodes::SESSION_KEEPALIVE)
            return 8;  /* 4-byte session ID + padding */
        if(opcode == Opcodes::SESSION_START)
            return 8;  /* optional timeout + padding */
        if(opcode == Opcodes::MINER_AUTH_CHALLENGE)
            return 40; /* nonce length field + 32-byte nonce + padding */
        if(opcode == Opcodes::MINER_AUTH_RESULT)
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
            /* SUBMIT_BLOCK: Full block (up to 2MB) + timestamp + signatures + encryption
             * Maximum: 2,100,346 bytes (2MB block + Falcon-1024 dual signatures + encryption)
             */
            if(packet.LENGTH > FalconConstants::SUBMIT_BLOCK_DUAL_SIG_ENCRYPTED_MAX)
            {
                if(strReason)
                {
                    std::ostringstream oss;
                    oss << "SUBMIT_BLOCK length " << packet.LENGTH 
                        << " exceeds maximum " << FalconConstants::SUBMIT_BLOCK_DUAL_SIG_ENCRYPTED_MAX;
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

} // namespace OpcodeUtility
} // namespace LLP
