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

    } // namespace StatelessOpcodes

} // namespace LLP

#endif
