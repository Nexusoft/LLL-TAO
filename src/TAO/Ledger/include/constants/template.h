/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Block Template Constants - Master Record"

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_CONSTANTS_TEMPLATE_H
#define NEXUS_TAO_LEDGER_INCLUDE_CONSTANTS_TEMPLATE_H

#include <cstdint>

namespace TAO
{
namespace Ledger
{
namespace TemplateConstants
{
    /** TEMPLATE_CACHE_MAX_AGE_SECONDS
     *
     *  Maximum age of cached template before considered stale.
     *  
     *  BLOCKCHAIN RULE:
     *  - Block interval: ~50 seconds
     *  - 60 seconds provides safety margin
     *  - Prevents miners from working on outdated state
     **/
    constexpr uint32_t TEMPLATE_CACHE_MAX_AGE_SECONDS = 60;
    
    /** TRITIUM_BLOCK_TEMPLATE_SIZE
     *
     *  Fixed size of serialized Tritium block template.
     *  
     *  PROTOCOL CONSTANT (cannot change without hard fork):
     *  - Version (4) + PrevBlock (128) + MerkleRoot (128) + Channel (4)
     *    + Height (4) + nBits (4) + Nonce (8) = 216 bytes
     **/
    constexpr uint32_t TRITIUM_BLOCK_TEMPLATE_SIZE = 216;
    
    /** TEMPLATE_CREATION_WARN_THRESHOLD_MS
     *
     *  Performance threshold for logging slow template creation.
     *  Normal: ~5ms, Slow: >10ms indicates potential issue
     **/
    constexpr uint32_t TEMPLATE_CREATION_WARN_THRESHOLD_MS = 10;
    
    /** MAX_MINERS_PER_NODE
     *
     *  Maximum stateless miners supported per node.
     *  - 500 miners = realistic node capacity
     *  - Can increase to 1000 on high-spec hardware
     **/
    constexpr uint32_t MAX_MINERS_PER_NODE = 500;
    
    /** ENABLE_TEMPLATE_CACHE_STATISTICS
     *
     *  Enable detailed cache performance statistics.
     *  Overhead: Minimal (atomic counters only)
     **/
    constexpr bool ENABLE_TEMPLATE_CACHE_STATISTICS = true;
    
    /** CACHE_LOG_LEVELS **/
    constexpr uint32_t CACHE_HIT_LOG_LEVEL = 2;         // -debug=2
    constexpr uint32_t CACHE_MISS_LOG_LEVEL = 1;        // -debug=1
    constexpr uint32_t CACHE_STATISTICS_LOG_LEVEL = 0;  // Always shown
    constexpr uint32_t CACHE_CREATION_LOG_LEVEL = 0;    // Always shown
    
    /** MINING CHANNEL IDENTIFIERS (Protocol constants) **/
    namespace Channels
    {
        constexpr uint32_t STAKE = 0;  // Proof-of-Stake (not cached)
        constexpr uint32_t PRIME = 1;  // Prime mining (cached)
        constexpr uint32_t HASH = 2;   // Hash mining (cached)
    }
    
} // namespace TemplateConstants
} // namespace Ledger
} // namespace TAO

#endif
