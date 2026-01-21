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
     **/
    namespace StatelessOpcodes
    {
        /** STATELESS_MINER_READY (0xD007)
         *
         *  Miner -> Node: Subscribe to template push notifications
         *
         *  When miner sends this opcode, it signals readiness to receive mining
         *  templates. The node immediately responds with STATELESS_GET_BLOCK.
         *
         **/
        static constexpr uint16_t STATELESS_MINER_READY = 0xD007;

        /** STATELESS_GET_BLOCK (0xD008)
         *
         *  Node -> Miner: Push mining template (228 bytes)
         *
         *  Packet format:
         *  - Opcode: 0xD008 (2 bytes, big-endian)
         *  - Metadata (12 bytes, big-endian):
         *    - Unified height (4 bytes)
         *    - Channel height (4 bytes)
         *    - Difficulty (4 bytes)
         *  - Block template (216 bytes): Serialized Tritium block
         *
         **/
        static constexpr uint16_t STATELESS_GET_BLOCK = 0xD008;

        /** STATELESS_SUBMIT_BLOCK (0xD009)
         *
         *  Miner -> Node: Submit solved block with nonce-in-header
         *
         *  This is the nonce-in-header submission format where the miner
         *  embeds the nonce directly into the block header before submission.
         *
         *  Packet format:
         *  - Opcode: 0xD009 (2 bytes, big-endian)
         *  - Block header with nonce:
         *    - Tritium: 216 bytes
         *    - Legacy: 220 bytes
         *
         *  The node extracts the nonce from the header and validates the block.
         *
         **/
        static constexpr uint16_t STATELESS_SUBMIT_BLOCK = 0xD009;

        /** STATELESS_BLOCK_ACCEPTED (0xD00A)
         *
         *  Node -> Miner: Block submission accepted
         *
         *  Sent when a submitted block is valid and accepted into the chain.
         *
         *  Packet format:
         *  - Opcode: 0xD00A (2 bytes, big-endian)
         *  - No payload (empty DATA field)
         *
         **/
        static constexpr uint16_t STATELESS_BLOCK_ACCEPTED = 0xD00A;

        /** STATELESS_BLOCK_REJECTED (0xD00B)
         *
         *  Node -> Miner: Block submission rejected
         *
         *  Sent when a submitted block is invalid or rejected.
         *
         *  Packet format:
         *  - Opcode: 0xD00B (2 bytes, big-endian)
         *  - Optional payload: UTF-8 string with rejection reason
         *
         **/
        static constexpr uint16_t STATELESS_BLOCK_REJECTED = 0xD00B;

        /** STATELESS_SET_CHANNEL (0xD00C)
         *
         *  Miner -> Node: Set mining channel preference
         *
         *  Allows miner to specify which channel to mine on:
         *  - Channel 1: Prime (CPU mining)
         *  - Channel 2: Hash (GPU mining)
         *
         *  Packet format:
         *  - Opcode: 0xD00C (2 bytes, big-endian)
         *  - Channel ID (1 byte): 1=Prime, 2=Hash
         *
         **/
        static constexpr uint16_t STATELESS_SET_CHANNEL = 0xD00C;

        /** STATELESS_SET_REWARD (0xD00D)
         *
         *  Miner -> Node: Bind reward address
         *
         *  Sets the address that will receive mining rewards.
         *
         *  Packet format:
         *  - Opcode: 0xD00D (2 bytes, big-endian)
         *  - Address (variable length, typically 32-64 bytes for Nexus addresses)
         *
         **/
        static constexpr uint16_t STATELESS_SET_REWARD = 0xD00D;

        /** STATELESS_REWARD_RESULT (0xD00E)
         *
         *  Node -> Miner: Reward address binding result
         *
         *  Confirms whether the reward address was successfully bound.
         *
         *  Packet format:
         *  - Opcode: 0xD00E (2 bytes, big-endian)
         *  - Status (1 byte): 0=failure, 1=success
         *  - Optional payload: UTF-8 string with status message
         *
         **/
        static constexpr uint16_t STATELESS_REWARD_RESULT = 0xD00E;

        /** STATELESS_PING (0xD00F)
         *
         *  Bidirectional: Keepalive/heartbeat
         *
         *  Either party can send this to keep the connection alive.
         *  The receiver should respond with STATELESS_PONG.
         *
         *  Packet format:
         *  - Opcode: 0xD00F (2 bytes, big-endian)
         *  - No payload (empty DATA field)
         *
         **/
        static constexpr uint16_t STATELESS_PING = 0xD00F;

        /** STATELESS_PONG (0xD010)
         *
         *  Bidirectional: Keepalive/heartbeat response
         *
         *  Sent in response to STATELESS_PING.
         *
         *  Packet format:
         *  - Opcode: 0xD010 (2 bytes, big-endian)
         *  - No payload (empty DATA field)
         *
         **/
        static constexpr uint16_t STATELESS_PONG = 0xD010;

        /** STATELESS_AUTH_INIT (0xD011)
         *
         *  Miner -> Node: Initialize Falcon authentication
         *
         *  First step in Falcon authentication handshake.
         *
         *  Packet format:
         *  - Opcode: 0xD011 (2 bytes, big-endian)
         *  - Falcon public key (variable length)
         *
         **/
        static constexpr uint16_t STATELESS_AUTH_INIT = 0xD011;

        /** STATELESS_AUTH_RESPONSE (0xD012)
         *
         *  Miner -> Node: Falcon authentication response
         *
         *  Contains Falcon signature proving ownership of public key.
         *
         *  Packet format:
         *  - Opcode: 0xD012 (2 bytes, big-endian)
         *  - Falcon signature (variable length)
         *
         **/
        static constexpr uint16_t STATELESS_AUTH_RESPONSE = 0xD012;

        /** STATELESS_AUTH_RESULT (0xD013)
         *
         *  Node -> Miner: Authentication result
         *
         *  Indicates success or failure of authentication.
         *
         *  Packet format:
         *  - Opcode: 0xD013 (2 bytes, big-endian)
         *  - Status (1 byte): 0=failure, 1=success
         *  - Optional payload: UTF-8 string with status message
         *
         **/
        static constexpr uint16_t STATELESS_AUTH_RESULT = 0xD013;

    } // namespace StatelessOpcodes

} // namespace LLP

#endif
