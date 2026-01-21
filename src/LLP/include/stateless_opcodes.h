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

        /** STATELESS_GET_HEIGHT (0xD014)
         *
         *  Miner -> Node: Request blockchain height
         *
         *  Packet format:
         *  - Opcode: 0xD014 (2 bytes, big-endian)
         *  - No payload (empty DATA field)
         *
         **/
        static constexpr uint16_t STATELESS_GET_HEIGHT = 0xD014;

        /** STATELESS_BLOCK_HEIGHT (0xD015)
         *
         *  Node -> Miner: Blockchain height response
         *
         *  Packet format:
         *  - Opcode: 0xD015 (2 bytes, big-endian)
         *  - Height (4 bytes, big-endian)
         *
         **/
        static constexpr uint16_t STATELESS_BLOCK_HEIGHT = 0xD015;

        /** STATELESS_GET_REWARD (0xD016)
         *
         *  Miner -> Node: Request current mining reward
         *
         *  Packet format:
         *  - Opcode: 0xD016 (2 bytes, big-endian)
         *  - No payload (empty DATA field)
         *
         **/
        static constexpr uint16_t STATELESS_GET_REWARD = 0xD016;

        /** STATELESS_BLOCK_REWARD (0xD017)
         *
         *  Node -> Miner: Mining reward response
         *
         *  Packet format:
         *  - Opcode: 0xD017 (2 bytes, big-endian)
         *  - Reward amount (8 bytes, uint64_t)
         *
         **/
        static constexpr uint16_t STATELESS_BLOCK_REWARD = 0xD017;

        /** STATELESS_GET_ROUND (0xD018)
         *
         *  Miner -> Node: Request round/height information
         *
         *  This provides multi-channel height information for staleness detection.
         *
         *  Packet format:
         *  - Opcode: 0xD018 (2 bytes, big-endian)
         *  - No payload (empty DATA field)
         *
         **/
        static constexpr uint16_t STATELESS_GET_ROUND = 0xD018;

        /** STATELESS_NEW_ROUND (0xD019)
         *
         *  Node -> Miner: New round response (height info)
         *
         *  Contains unified height and per-channel heights for multi-channel
         *  staleness detection.
         *
         *  Packet format:
         *  - Opcode: 0xD019 (2 bytes, big-endian)
         *  - Unified height (4 bytes)
         *  - Prime channel height (4 bytes)
         *  - Hash channel height (4 bytes)
         *  - Stake channel height (4 bytes)
         *
         **/
        static constexpr uint16_t STATELESS_NEW_ROUND = 0xD019;

        /** STATELESS_OLD_ROUND (0xD01A)
         *
         *  Node -> Miner: Old/stale round response
         *
         *  Sent when the round information is stale or unchanged.
         *
         *  Packet format:
         *  - Opcode: 0xD01A (2 bytes, big-endian)
         *  - No payload (empty DATA field)
         *
         **/
        static constexpr uint16_t STATELESS_OLD_ROUND = 0xD01A;

        /** STATELESS_MINER_READY (0xD01B)
         *
         *  Miner -> Node: Miner ready notification
         *
         *  Indicates miner is ready to receive work.
         *
         *  Packet format:
         *  - Opcode: 0xD01B (2 bytes, big-endian)
         *  - No payload (empty DATA field)
         *
         **/
        static constexpr uint16_t STATELESS_MINER_READY_ALT = 0xD01B;

        /** STATELESS_PRIME_BLOCK_AVAILABLE (0xD01C)
         *
         *  Node -> Miner: Prime channel block available notification
         *
         *  Push notification that a new Prime channel block is ready.
         *
         *  Packet format:
         *  - Opcode: 0xD01C (2 bytes, big-endian)
         *  - No payload (empty DATA field)
         *
         **/
        static constexpr uint16_t STATELESS_PRIME_BLOCK_AVAILABLE = 0xD01C;

        /** STATELESS_HASH_BLOCK_AVAILABLE (0xD01D)
         *
         *  Node -> Miner: Hash channel block available notification
         *
         *  Push notification that a new Hash channel block is ready.
         *
         *  Packet format:
         *  - Opcode: 0xD01D (2 bytes, big-endian)
         *  - No payload (empty DATA field)
         *
         **/
        static constexpr uint16_t STATELESS_HASH_BLOCK_AVAILABLE = 0xD01D;

    } // namespace StatelessOpcodes

} // namespace LLP

#endif
