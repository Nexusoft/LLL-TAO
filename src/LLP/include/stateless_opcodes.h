/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_STATELESS_OPCODES_H
#define NEXUS_LLP_INCLUDE_STATELESS_OPCODES_H

#include <cstdint>

namespace LLP
{
    /** Stateless Mining Protocol Opcodes (16-bit)
     *
     *  These opcodes are used exclusively with the 16-bit stateless packet framing.
     *  They are NOT compatible with legacy 8-bit LLP framing.
     *
     *  Packet format:
     *  - HEADER: 2 bytes (16-bit opcode, big-endian)
     *  - LENGTH: 4 bytes (32-bit, big-endian)
     *  - DATA:   LENGTH bytes
     *
     *  MIRROR-MAPPED SCHEME:
     *  All stateless opcodes are mirror-mapped from legacy 8-bit opcodes:
     *      statelessOpcode = 0xD000 | legacyOpcode
     *
     *  This provides a 1:1 mapping between legacy and stateless protocols while
     *  maintaining backward compatibility and simplifying protocol bridging.
     *
     **/
    namespace StatelessOpcodes
    {
        /** Helper Functions **/
        
        /** Mirror
         *
         *  Convert legacy 8-bit opcode to mirror-mapped 16-bit stateless opcode.
         *
         *  @param[in] legacyOpcode The legacy 8-bit opcode
         *  @return The mirror-mapped 16-bit stateless opcode (0xD000 | legacyOpcode)
         *
         **/
        constexpr uint16_t Mirror(uint8_t legacyOpcode)
        {
            return 0xD000 | legacyOpcode;
        }

        /** IsStateless
         *
         *  Check if an opcode is a valid stateless opcode (in range 0xD000-0xD0FF).
         *
         *  @param[in] opcode The 16-bit opcode to check
         *  @return true if opcode is in valid stateless range, false otherwise
         *
         **/
        constexpr bool IsStateless(uint16_t opcode)
        {
            return (opcode & 0xFF00) == 0xD000;
        }

        /** Unmirror
         *
         *  Convert mirror-mapped 16-bit stateless opcode back to legacy 8-bit opcode.
         *
         *  @param[in] statelessOpcode The 16-bit stateless opcode
         *  @return The legacy 8-bit opcode (statelessOpcode & 0xFF)
         *
         **/
        constexpr uint8_t Unmirror(uint16_t statelessOpcode)
        {
            return static_cast<uint8_t>(statelessOpcode & 0xFF);
        }
        /** MIRROR-MAPPED OPCODES
         *
         *  All opcodes below are derived from legacy 8-bit mining opcodes using
         *  the mirror-mapping formula: statelessOpcode = 0xD000 | legacyOpcode
         *
         *  Legacy Reference (from miner.h):
         *    BLOCK_DATA=0, SUBMIT_BLOCK=1, BLOCK_HEIGHT=2, SET_CHANNEL=3,
         *    BLOCK_REWARD=4, SET_COINBASE=5, GOOD_BLOCK=6, ORPHAN_BLOCK=7,
         *    CHECK_BLOCK=64, SUBSCRIBE=65, GET_BLOCK=129, GET_HEIGHT=130,
         *    GET_REWARD=131, CLEAR_MAP=132, GET_ROUND=133, BLOCK_ACCEPTED=200,
         *    BLOCK_REJECTED=201, COINBASE_SET=202, COINBASE_FAIL=203,
         *    NEW_ROUND=204, OLD_ROUND=205, CHANNEL_ACK=206,
         *    MINER_AUTH_INIT=207, MINER_AUTH_CHALLENGE=208,
         *    MINER_AUTH_RESPONSE=209, MINER_AUTH_RESULT=210,
         *    SESSION_START=211, SESSION_KEEPALIVE=212,
         *    MINER_SET_REWARD=213, MINER_REWARD_RESULT=214,
         *    MINER_READY=216, PRIME_BLOCK_AVAILABLE=217,
         *    HASH_BLOCK_AVAILABLE=218, PING=253, CLOSE=254
         **/

        /** DATA PACKETS (mirror-mapped from 0-7) **/
        static constexpr uint16_t STATELESS_BLOCK_DATA   = Mirror(0);   // 0xD000
        static constexpr uint16_t STATELESS_SUBMIT_BLOCK = Mirror(1);   // 0xD001
        static constexpr uint16_t STATELESS_BLOCK_HEIGHT = Mirror(2);   // 0xD002
        static constexpr uint16_t STATELESS_SET_CHANNEL  = Mirror(3);   // 0xD003
        static constexpr uint16_t STATELESS_BLOCK_REWARD = Mirror(4);   // 0xD004
        static constexpr uint16_t STATELESS_SET_COINBASE = Mirror(5);   // 0xD005
        static constexpr uint16_t STATELESS_GOOD_BLOCK   = Mirror(6);   // 0xD006
        static constexpr uint16_t STATELESS_ORPHAN_BLOCK = Mirror(7);   // 0xD007

        /** DATA REQUESTS (mirror-mapped from 64-65) **/
        static constexpr uint16_t STATELESS_CHECK_BLOCK  = Mirror(64);  // 0xD040
        static constexpr uint16_t STATELESS_SUBSCRIBE    = Mirror(65);  // 0xD041

        /** REQUEST PACKETS (mirror-mapped from 129-133) **/
        static constexpr uint16_t STATELESS_GET_BLOCK    = Mirror(129); // 0xD081
        static constexpr uint16_t STATELESS_GET_HEIGHT   = Mirror(130); // 0xD082
        static constexpr uint16_t STATELESS_GET_REWARD   = Mirror(131); // 0xD083
        static constexpr uint16_t STATELESS_CLEAR_MAP    = Mirror(132); // 0xD084
        static constexpr uint16_t STATELESS_GET_ROUND    = Mirror(133); // 0xD085

        /** RESPONSE PACKETS (mirror-mapped from 200-206) **/
        static constexpr uint16_t STATELESS_BLOCK_ACCEPTED = Mirror(200); // 0xD0C8
        static constexpr uint16_t STATELESS_BLOCK_REJECTED = Mirror(201); // 0xD0C9
        static constexpr uint16_t STATELESS_COINBASE_SET   = Mirror(202); // 0xD0CA
        static constexpr uint16_t STATELESS_COINBASE_FAIL  = Mirror(203); // 0xD0CB
        static constexpr uint16_t STATELESS_NEW_ROUND      = Mirror(204); // 0xD0CC
        static constexpr uint16_t STATELESS_OLD_ROUND      = Mirror(205); // 0xD0CD
        static constexpr uint16_t STATELESS_CHANNEL_ACK    = Mirror(206); // 0xD0CE

        /** AUTHENTICATION PACKETS (mirror-mapped from 207-210) **/
        static constexpr uint16_t STATELESS_AUTH_INIT      = Mirror(207); // 0xD0CF
        static constexpr uint16_t STATELESS_AUTH_CHALLENGE = Mirror(208); // 0xD0D0
        static constexpr uint16_t STATELESS_AUTH_RESPONSE  = Mirror(209); // 0xD0D1
        static constexpr uint16_t STATELESS_AUTH_RESULT    = Mirror(210); // 0xD0D2

        /** SESSION MANAGEMENT (mirror-mapped from 211-212) **/
        static constexpr uint16_t STATELESS_SESSION_START     = Mirror(211); // 0xD0D3
        static constexpr uint16_t STATELESS_SESSION_KEEPALIVE = Mirror(212); // 0xD0D4

        /** REWARD ADDRESS BINDING (mirror-mapped from 213-214) **/
        static constexpr uint16_t STATELESS_SET_REWARD     = Mirror(213); // 0xD0D5
        static constexpr uint16_t STATELESS_REWARD_RESULT  = Mirror(214); // 0xD0D6

        /** PUSH NOTIFICATIONS (mirror-mapped from 216-218) **/
        static constexpr uint16_t STATELESS_MINER_READY           = Mirror(216); // 0xD0D8
        static constexpr uint16_t STATELESS_PRIME_BLOCK_AVAILABLE = Mirror(217); // 0xD0D9
        static constexpr uint16_t STATELESS_HASH_BLOCK_AVAILABLE  = Mirror(218); // 0xD0DA

        /** GENERIC (mirror-mapped from 253-254) **/
        static constexpr uint16_t STATELESS_PING  = Mirror(253); // 0xD0FD
        static constexpr uint16_t STATELESS_CLOSE = Mirror(254); // 0xD0FE

    } // namespace StatelessOpcodes

} // namespace LLP

#endif
