/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/miner_push_dispatcher.h>
#include <LLP/include/global.h>

#include <Util/include/debug.h>
#include <Util/include/config.h>
#include <vector>
namespace LLP
{

    /* Static dedup-key storage — one per PoW channel.
     * Layout: high-32 bits = unified height, low-32 bits = first 4 bytes of hashBestChain.
     * Initial value 0 ensures first real dispatch always proceeds. */
    std::atomic<uint64_t> MinerPushDispatcher::s_nPrimeDedup{0};
    std::atomic<uint64_t> MinerPushDispatcher::s_nHashDedup{0};


    /* Pack (height, hashPrefix4) into a single 64-bit dedup key. */
    static inline uint64_t make_dedup_key(uint32_t nHeight, uint32_t nHashPrefix4)
    {
        return (static_cast<uint64_t>(nHeight) << 32) | static_cast<uint64_t>(nHashPrefix4);
    }


    /* BroadcastChannel — send one channel notification to both lanes. */
    void MinerPushDispatcher::BroadcastChannel(uint32_t nChannel,
                                               uint32_t nHeight,
                                               uint32_t hashPrefix4)
    {
        const char* strChannel = (nChannel == 1) ? "Prime" : "Hash";

        /* Stateless lane */
        uint32_t nStateless = 0;
        if (LLP::STATELESS_MINER_SERVER)
            nStateless = LLP::STATELESS_MINER_SERVER->NotifyChannelMiners(nChannel);
        else
            debug::log(1, FUNCTION, "[PUSH][Stateless][", strChannel, "] Server not active");

        /* Legacy lane */
        uint32_t nLegacy = 0;
        if (LLP::MINING_SERVER)
            nLegacy = LLP::MINING_SERVER->NotifyChannelMiners(nChannel);
        else
            debug::log(1, FUNCTION, "[PUSH][Legacy][", strChannel, "] Server not active");

        /* Summary: one line confirms dedup and exact send counts for this channel. */
        debug::log(0, FUNCTION,
                   "[PUSH][", strChannel, "] height=", nHeight,
                   " hash=", std::hex, hashPrefix4, std::dec,
                   " | Stateless=", nStateless, " Legacy=", nLegacy,
                   " | no duplicates (one send per miner per lane)");
    }


    /* DispatchPushEvent — canonical entry-point for all miner push broadcasts. */
    void MinerPushDispatcher::DispatchPushEvent(uint32_t nHeight,
                                                const uint1024_t& hashBestChain)
    {
        if (config::fShutdown.load())
            return;

        /* Extract the first 4 bytes of hashBestChain as a uint32_t for dedup.
         * GetBytes() on a uint1024_t always returns exactly 128 bytes (little-endian). */
        const std::vector<uint8_t> vHashBytes = hashBestChain.GetBytes();
        uint32_t hashPrefix4 = 0;
        if (vHashBytes.size() == 128)
        {
            hashPrefix4 = (static_cast<uint32_t>(vHashBytes[3]) << 24)
                        | (static_cast<uint32_t>(vHashBytes[2]) << 16)
                        | (static_cast<uint32_t>(vHashBytes[1]) <<  8)
                        |  static_cast<uint32_t>(vHashBytes[0]);
        }
        else
        {
            debug::error(FUNCTION, "Unexpected hashBestChain byte count: ", vHashBytes.size(),
                         " (expected 128); dedup prefix will be 0");
        }

        const uint64_t nNewKey = make_dedup_key(nHeight, hashPrefix4);

        /* --- Prime channel dedup --- */
        {
            uint64_t nOldPrime = s_nPrimeDedup.load(std::memory_order_acquire);
            if (nOldPrime != nNewKey &&
                s_nPrimeDedup.compare_exchange_strong(nOldPrime, nNewKey,
                                                      std::memory_order_release,
                                                      std::memory_order_relaxed))
            {
                BroadcastChannel(1 /* Prime */, nHeight, hashPrefix4);
            }
            else if (nOldPrime == nNewKey)
            {
                debug::log(1, FUNCTION,
                           "[PUSH][Prime] Dedup: already dispatched for height=", nHeight,
                           " hash=", std::hex, hashPrefix4, std::dec, "; skipping");
            }
        }

        /* --- Hash channel dedup --- */
        {
            uint64_t nOldHash = s_nHashDedup.load(std::memory_order_acquire);
            if (nOldHash != nNewKey &&
                s_nHashDedup.compare_exchange_strong(nOldHash, nNewKey,
                                                     std::memory_order_release,
                                                     std::memory_order_relaxed))
            {
                BroadcastChannel(2 /* Hash */, nHeight, hashPrefix4);
            }
            else if (nOldHash == nNewKey)
            {
                debug::log(1, FUNCTION,
                           "[PUSH][Hash] Dedup: already dispatched for height=", nHeight,
                           " hash=", std::hex, hashPrefix4, std::dec, "; skipping");
            }
        }
    }

} // namespace LLP
