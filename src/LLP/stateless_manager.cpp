/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/stateless_manager.h>
#include <LLP/include/genesis_constants.h>
#include <LLP/include/node_cache.h>
#include <LLP/include/node_session_registry.h>
#include <LLP/include/mining_timers.h>

#include <TAO/Ledger/types/block.h>
#include <TAO/Register/types/address.h>

#include <Util/include/json.h>
#include <Util/include/string.h>
#include <Util/include/runtime.h>
#include <Util/include/debug.h>

#include <optional>
#include <sstream>
#include <algorithm>

namespace LLP
{
    /* Grace period for keepalive check in smart timeout logic.
     * Now sourced from the centralized MiningTimers::KEEPALIVE_GRACE_PERIOD_SEC
     * constant.  See mining_timers.h for the full rationale. */
    static constexpr uint64_t KEEPALIVE_GRACE_PERIOD_SEC = MiningTimers::KEEPALIVE_GRACE_PERIOD_SEC;

    /* Get singleton instance */
    StatelessMinerManager& StatelessMinerManager::Get()
    {
        static StatelessMinerManager instance;
        return instance;
    }

    /* Update or add a miner */
    void StatelessMinerManager::UpdateMiner(
        const std::string& strAddress,
        const MiningContext& context,
        uint8_t nLane
    )
    {
        /* Get existing context once to avoid double lookups */
        auto optExisting = mapMiners.Get(strAddress);
        bool fNewMiner = !optExisting.has_value();
        bool fWasAuthenticated = false;
        uint32_t nPrevKeepalives = 0;

        if(!fNewMiner)
        {
            const MiningContext& existing = optExisting.value();
            fWasAuthenticated = existing.fAuthenticated;
            nPrevKeepalives = existing.nKeepaliveCount;

            /* Clean up stale secondary index entries when keys/session/genesis change.
             * Without this, old index entries point to the wrong miner after re-auth.
             * CompareAndErase atomically guards against erasing a concurrent UpdateMiner's entry. */
            if(existing.hashKeyID != 0 && existing.hashKeyID != context.hashKeyID)
                mapKeyToAddress.CompareAndErase(existing.hashKeyID, strAddress);
            if(existing.nSessionId != 0 && existing.nSessionId != context.nSessionId)
                mapSessionToAddress.CompareAndErase(existing.nSessionId, strAddress);
            if(existing.hashGenesis != 0 && existing.hashGenesis != context.hashGenesis)
                mapGenesisToAddress.CompareAndErase(existing.hashGenesis, strAddress);

            /* Clean up stale IP index when the address (IP:port) changes.
             * Previously UpdateMiner() never cleaned old IP mappings, so after a
             * NAT port change the old IP→IP:oldPort entry persisted and
             * GetMinerContextByIP() could return stale data. */
            if(!existing.strAddress.empty() && existing.strAddress != strAddress)
            {
                const size_t nOldColon = existing.strAddress.rfind(':');
                if(nOldColon != std::string::npos)
                {
                    const std::string strOldIP = existing.strAddress.substr(0, nOldColon);
                    mapIPToAddress.CompareAndErase(strOldIP, existing.strAddress);
                }
            }
        }

        /* Enforce cache limit before adding new miner (DDOS protection).
         * Use atomic counter for cheap threshold check to avoid lock contention
         * on the hot path — full enforcement only when 20% over limit. */
        if(fNewMiner)
        {
            size_t nCurrent = nTotalMiners.load();
            size_t nLimit = NodeCache::DEFAULT_MAX_CACHE_SIZE;
            if(nCurrent > nLimit + nLimit / 5)
                EnforceCacheLimit(nLimit);
        }

        /* Update main miner map */
        mapMiners.InsertOrUpdate(strAddress, context);

        /* Track lane for cross-lane session coordination */
        mapAddressToLane.InsertOrUpdate(strAddress, nLane);

        /* Update IP index for port-agnostic lookups */
        {
            const size_t nColon = strAddress.rfind(':');
            if(nColon != std::string::npos)
                mapIPToAddress.InsertOrUpdate(strAddress.substr(0, nColon), strAddress);
        }

        /* Update atomic counters for lock-free stats */
        if(fNewMiner)
        {
            /* Increment total miners and get new count atomically */
            size_t nNewCount = ++nTotalMiners;

            /* Update peak session count if needed (use post-increment value) */
            size_t nPeak = nPeakSessions.load();
            while(nNewCount > nPeak && !nPeakSessions.compare_exchange_weak(nPeak, nNewCount))
            {
                /* CAS failed, nPeak is updated with current value, retry */
            }
        }

        /* Update authenticated counter */
        if(context.fAuthenticated && !fWasAuthenticated)
        {
            ++nAuthenticatedMiners;
        }

        /* Track keepalives via atomic counter using cached previous value */
        if(context.nKeepaliveCount > nPrevKeepalives)
        {
            nTotalKeepalives += (context.nKeepaliveCount - nPrevKeepalives);
        }

        /* Update keyID index if authenticated */
        if(context.fAuthenticated && context.hashKeyID != 0)
        {
            mapKeyToAddress.InsertOrUpdate(context.hashKeyID, strAddress);
        }

        /* Update session index if session ID is set */
        if(context.nSessionId != 0)
        {
            mapSessionToAddress.InsertOrUpdate(context.nSessionId, strAddress);
        }

        /* Update genesis index for GenesisHash reward mapping */
        if(context.hashGenesis != 0)
        {
            mapGenesisToAddress.InsertOrUpdate(context.hashGenesis, strAddress);
        }
    }


    /* Atomically transform a miner's context in-place */
    bool StatelessMinerManager::TransformMiner(
        const std::string& strAddress,
        std::function<MiningContext(const MiningContext&)> transformer,
        uint8_t nLane
    )
    {
        /* Read current state for pre-transform index comparison */
        auto optOld = mapMiners.Get(strAddress);
        if(!optOld.has_value())
            return false;

        const MiningContext& oldCtx = optOld.value();
        uint32_t nOldKeepalives = oldCtx.nKeepaliveCount;
        bool fWasAuthenticated = oldCtx.fAuthenticated;
        uint256_t hashOldKeyID = oldCtx.hashKeyID;
        uint32_t nOldSessionId = oldCtx.nSessionId;
        uint256_t hashOldGenesis = oldCtx.hashGenesis;

        /* Apply transformer atomically — operates on the CURRENT value inside the map,
         * not on the snapshot we read above. This eliminates TOCTOU races. */
        MiningContext newCtx;
        bool fTransformed = mapMiners.Transform(strAddress,
            [&transformer, &newCtx](const MiningContext& current) {
                newCtx = transformer(current);
                return newCtx;
            });

        if(!fTransformed)
            return false;

        /* Update lane tracking */
        mapAddressToLane.InsertOrUpdate(strAddress, nLane);

        /* Update IP index */
        {
            const size_t nColon = strAddress.rfind(':');
            if(nColon != std::string::npos)
                mapIPToAddress.InsertOrUpdate(strAddress.substr(0, nColon), strAddress);
        }

        /* Clean up stale secondary index entries */
        if(hashOldKeyID != 0 && hashOldKeyID != newCtx.hashKeyID)
            mapKeyToAddress.Erase(hashOldKeyID);
        if(nOldSessionId != 0 && nOldSessionId != newCtx.nSessionId)
            mapSessionToAddress.Erase(nOldSessionId);
        if(hashOldGenesis != 0 && hashOldGenesis != newCtx.hashGenesis)
            mapGenesisToAddress.Erase(hashOldGenesis);

        /* Update authenticated counter */
        if(newCtx.fAuthenticated && !fWasAuthenticated)
            ++nAuthenticatedMiners;

        /* Track keepalives */
        if(newCtx.nKeepaliveCount > nOldKeepalives)
            nTotalKeepalives += (newCtx.nKeepaliveCount - nOldKeepalives);

        /* Update secondary indices */
        if(newCtx.fAuthenticated && newCtx.hashKeyID != 0)
            mapKeyToAddress.InsertOrUpdate(newCtx.hashKeyID, strAddress);
        if(newCtx.nSessionId != 0)
            mapSessionToAddress.InsertOrUpdate(newCtx.nSessionId, strAddress);
        if(newCtx.hashGenesis != 0)
            mapGenesisToAddress.InsertOrUpdate(newCtx.hashGenesis, strAddress);

        return true;
    }


    /* Atomically transform a miner's context looked up by session ID */
    bool StatelessMinerManager::TransformMinerBySession(
        uint32_t nSessionId,
        std::function<MiningContext(const MiningContext&)> transformer
    )
    {
        auto optAddress = mapSessionToAddress.Get(nSessionId);
        if(!optAddress.has_value())
            return false;

        return TransformMiner(optAddress.value(), transformer);
    }

    /* Get miner lane by address */
    std::optional<uint8_t> StatelessMinerManager::GetMinerLane(
        const std::string& strAddress
    ) const
    {
        return mapAddressToLane.Get(strAddress);
    }

    /* Check if miner has switched lanes */
    bool StatelessMinerManager::HasSwitchedLanes(
        const std::string& strAddress,
        uint8_t nNewLane
    ) const
    {
        auto optLane = mapAddressToLane.Get(strAddress);
        if(!optLane.has_value())
            return false;

        return optLane.value() != nNewLane;
    }

    /* Remove a miner by address */
    bool StatelessMinerManager::RemoveMiner(const std::string& strAddress)
    {
        auto optContext = mapMiners.GetAndRemove(strAddress);
        if(!optContext.has_value())
            return false;

        const MiningContext& ctx = optContext.value();

        /* Update atomic counters */
        if(nTotalMiners > 0)
            --nTotalMiners;

        if(ctx.fAuthenticated && nAuthenticatedMiners > 0)
            --nAuthenticatedMiners;

        /* Remove from secondary indices atomically.
         * CompareAndErase removes the entry only if it still points back to THIS
         * address, preventing TOCTOU races where a concurrent UpdateMiner() for
         * the same key but a different address may have already replaced the mapping. */
        if(ctx.hashKeyID != 0)
            mapKeyToAddress.CompareAndErase(ctx.hashKeyID, strAddress);

        if(ctx.nSessionId != 0)
            mapSessionToAddress.CompareAndErase(ctx.nSessionId, strAddress);

        if(ctx.hashGenesis != 0)
            mapGenesisToAddress.CompareAndErase(ctx.hashGenesis, strAddress);

        mapAddressToLane.Erase(strAddress);

        /* Remove IP index entry — only if it still points to THIS address.
         * Multiple miners behind the same NAT share the same IP prefix;
         * unconditionally erasing would orphan the other miner's mapping. */
        {
            const size_t nColon = strAddress.rfind(':');
            if(nColon != std::string::npos)
            {
                const std::string strIP = strAddress.substr(0, nColon);
                mapIPToAddress.CompareAndErase(strIP, strAddress);
            }
        }

        /* Cross-cache consistency: mark session as dead in NodeSessionRegistry.
         * Centralised here so that every removal path
         * (CleanupInactive, PurgeInactiveMiners, EnforceCacheLimit,
         * RemoveMinerByKeyID, direct disconnects) gets this automatically. */
        if(ctx.hashKeyID != 0)
        {
            NodeSessionRegistry::Get().MarkDisconnected(ctx.hashKeyID, ProtocolLane::STATELESS);
            NodeSessionRegistry::Get().MarkDisconnected(ctx.hashKeyID, ProtocolLane::LEGACY);
        }

        return true;
    }

    /* Remove a miner by key ID */
    bool StatelessMinerManager::RemoveMinerByKeyID(const uint256_t& hashKeyID)
    {
        /* GetAndRemove is intentionally NOT used here.
         * RemoveMiner() already guards its mapKeyToAddress erase with a value
         * check, so we only need the address lookup (non-destructive Get).
         * The previous code did GetAndRemove + RemoveMiner which double-erased
         * mapKeyToAddress, creating a window where a concurrent UpdateMiner()
         * could have its freshly-inserted index entry silently destroyed. */
        auto optAddress = mapKeyToAddress.Get(hashKeyID);
        if(!optAddress.has_value())
            return false;

        return RemoveMiner(optAddress.value());
    }

    /* Get miner context by address */
    std::optional<MiningContext> StatelessMinerManager::GetMinerContext(
        const std::string& strAddress
    ) const
    {
        return mapMiners.Get(strAddress);
    }

    /* Get miner context by key ID */
    std::optional<MiningContext> StatelessMinerManager::GetMinerContextByKeyID(
        const uint256_t& hashKeyID
    ) const
    {
        auto optAddress = mapKeyToAddress.Get(hashKeyID);
        if(!optAddress.has_value())
            return std::nullopt;

        return mapMiners.Get(optAddress.value());
    }


    /* Look up a miner context by IP address only (port-agnostic fallback).
     * Uses mapIPToAddress secondary index for O(1) lookup instead of O(N) scan. */
    std::optional<MiningContext> StatelessMinerManager::GetMinerContextByIP(
        const std::string& strIP
    ) const
    {
        auto optAddress = mapIPToAddress.Get(strIP);
        if(!optAddress.has_value())
            return std::nullopt;

        return mapMiners.Get(optAddress.value());
    }

    std::optional<MiningContext> StatelessMinerManager::GetMinerContextByAddressOrIP(
        const std::string& strAddress,
        uint32_t nExpectedSessionId,
        bool fMigrateAddress
    )
    {
        auto optContext = mapMiners.Get(strAddress);
        if(optContext.has_value())
            return optContext;

        /* Try IP-only key (stateless miners are canonically keyed by IP without port) */
        const size_t nColon = strAddress.rfind(':');
        if(nColon == std::string::npos)
            return std::nullopt;

        const std::string strIP = strAddress.substr(0, nColon);
        auto optNormalizedContext = mapMiners.Get(strIP);
        if(optNormalizedContext.has_value())
            return optNormalizedContext;

        /* IP-only fallback via mapIPToAddress is architecturally unsound for NAT
         * environments: multiple miners behind the same NAT share the same IP, so
         * the index can only track one of them.  Address migration based on this
         * index creates persistent data-consistency issues.
         *
         * Callers should use session-ID-based lookups (GetMinerContextBySessionID)
         * or the NodeSessionRegistry for cross-connection session recovery. */
        return std::nullopt;
    }

    /* Get miner context by session ID */
    std::optional<MiningContext> StatelessMinerManager::GetMinerContextBySessionID(
        uint32_t nSessionId
    ) const
    {
        auto optAddress = mapSessionToAddress.Get(nSessionId);
        if(!optAddress.has_value())
            return std::nullopt;

        return mapMiners.Get(optAddress.value());
    }

    /* Get miner context by genesis hash */
    std::optional<MiningContext> StatelessMinerManager::GetMinerContextByGenesis(
        const uint256_t& hashGenesis
    ) const
    {
        auto optAddress = mapGenesisToAddress.Get(hashGenesis);
        if(!optAddress.has_value())
            return std::nullopt;

        return mapMiners.Get(optAddress.value());
    }

    /* List all miners */
    std::vector<MiningContext> StatelessMinerManager::ListMiners() const
    {
        return mapMiners.GetAll();
    }

    /* List miners by genesis hash.
     * Uses ForEach() to avoid full snapshot copy + filter pattern. */
    std::vector<MiningContext> StatelessMinerManager::ListMinersByGenesis(
        const uint256_t& hashGenesis
    ) const
    {
        std::vector<MiningContext> vResult;

        mapMiners.ForEach([&](const std::string& key, const MiningContext& ctx) {
            if(ctx.hashGenesis == hashGenesis)
                vResult.push_back(ctx);
        });

        return vResult;
    }

    /* Get miner count (lock-free via atomic) */
    size_t StatelessMinerManager::GetMinerCount() const
    {
        return nTotalMiners.load();
    }

    /* Get authenticated miner count (lock-free via atomic) */
    size_t StatelessMinerManager::GetAuthenticatedCount() const
    {
        return nAuthenticatedMiners.load();
    }

    /* Get active session count.
     * A session is active if it has been started (nSessionStart > 0) and
     * still has recent activity within the global liveness window.
     * Session liveness is governed by CleanupInactive() with
     * SESSION_LIVENESS_TIMEOUT_SECONDS (86400s).
     * Uses ForEach() to avoid full snapshot copy. */
    size_t StatelessMinerManager::GetActiveSessionCount() const
    {
        size_t nCount = 0;
        uint64_t nNow = runtime::unifiedtimestamp();

        mapMiners.ForEach([&](const std::string& key, const MiningContext& ctx) {
            if(ctx.nSessionStart > 0 &&
               (nNow - ctx.nTimestamp) <= NodeCache::SESSION_LIVENESS_TIMEOUT_SECONDS)
                ++nCount;
        });

        return nCount;
    }

    /* Get total keepalives (lock-free via atomic) */
    uint64_t StatelessMinerManager::GetTotalKeepalives() const
    {
        return nTotalKeepalives.load();
    }

    /* Get peak session count */
    size_t StatelessMinerManager::GetPeakSessionCount() const
    {
        return nPeakSessions.load();
    }

    /* Cleanup inactive miners */
    uint32_t StatelessMinerManager::CleanupInactive(uint64_t nTimeoutSec)
    {
        uint32_t nRemoved = 0;
        uint64_t nNow = runtime::unifiedtimestamp();

        auto pairs = mapMiners.GetAllPairs();
        for(const auto& pair : pairs)
        {
            const MiningContext& ctx = pair.second;

            if(ctx.IsConsideredInactive(nNow, nTimeoutSec))
            {
                uint64_t nTimeSinceActivity = nNow - ctx.nTimestamp;

                debug::log(0, FUNCTION, "Session ", ctx.strAddress,
                          " truly idle - activity: ", nTimeSinceActivity, "s, ",
                          "keepalives_rx: ", ctx.nKeepaliveCount, ", ",
                          "keepalives_tx: ", ctx.nKeepaliveSent);

                /* RemoveMiner() handles cross-cache cleanup (NodeSessionRegistry
                 * MarkDisconnected) internally. */
                if(RemoveMiner(pair.first))
                    ++nRemoved;
            }
        }

        if(nRemoved > 0)
        {
            debug::log(0, FUNCTION, "Cleaned up ", nRemoved, " truly idle miners");
        }

        return nRemoved;
    }


    /* CleanupSessionScopedMaps — no-op after SIM-LINK removal.
     * Kept as stub for callers; returns 0. */
    uint32_t StatelessMinerManager::CleanupSessionScopedMaps()
    {
        return 0;
    }

    /* Get miner status as JSON */
    std::string StatelessMinerManager::GetMinerStatus(const std::string& strAddress) const
    {
        auto optContext = mapMiners.Get(strAddress);
        if(!optContext.has_value())
            return "{\"error\": \"Miner not found\"}";

        const MiningContext& ctx = optContext.value();
        uint64_t nNow = runtime::unifiedtimestamp();

        /* Build JSON response with Phase 2 identity fields and session info */
        encoding::json result;
        result["address"] = ctx.strAddress;
        result["channel"] = ctx.nChannel;
        result["height"] = ctx.nHeight;
        result["authenticated"] = ctx.fAuthenticated;
        result["session_id"] = ctx.nSessionId;
        result["protocol_version"] = ctx.nProtocolVersion;
        result["last_seen"] = ctx.nTimestamp;
        result["key_id"] = ctx.hashKeyID.ToString();
        result["genesis"] = ctx.hashGenesis.ToString();

        /* Add session management fields.
         * Session liveness is governed by CleanupInactive() with a 24-hour
         * 3-way AND check; the "session_active" field reflects whether the
         * session has recent activity within the global liveness window. */
        result["session_start"] = ctx.nSessionStart;
        result["session_duration"] = ctx.GetSessionDuration(nNow);
        result["session_active"] = (ctx.nSessionStart > 0 &&
            (nNow - ctx.nTimestamp) <= NodeCache::SESSION_LIVENESS_TIMEOUT_SECONDS);
        result["keepalive_count"] = ctx.nKeepaliveCount;

        return result.dump(4);
    }

    /* Get all miners status as JSON.
     * Uses ForEach() to avoid full snapshot copy. */
    std::string StatelessMinerManager::GetAllMinersStatus() const
    {
        encoding::json miners = encoding::json::array();
        uint64_t nNow = runtime::unifiedtimestamp();

        mapMiners.ForEach([&](const std::string& key, const MiningContext& ctx) {
            encoding::json miner;
            miner["address"] = ctx.strAddress;
            miner["channel"] = ctx.nChannel;
            miner["height"] = ctx.nHeight;
            miner["authenticated"] = ctx.fAuthenticated;
            miner["session_id"] = ctx.nSessionId;
            miner["protocol_version"] = ctx.nProtocolVersion;
            miner["last_seen"] = ctx.nTimestamp;
            miner["key_id"] = ctx.hashKeyID.ToString();
            miner["genesis"] = ctx.hashGenesis.ToString();

            /* Add session management fields */
            miner["session_start"] = ctx.nSessionStart;
            miner["session_duration"] = ctx.GetSessionDuration(nNow);
            miner["session_active"] = (ctx.nSessionStart > 0 &&
                (nNow - ctx.nTimestamp) <= NodeCache::SESSION_LIVENESS_TIMEOUT_SECONDS);
            miner["keepalive_count"] = ctx.nKeepaliveCount;

            miners.push_back(miner);
        });

        return miners.dump(4);
    }


    /* Notify miners of new round.
     * Uses TransformAll() to atomically update each miner's height
     * under a single write lock, eliminating the snapshot-overwrite race
     * where concurrent UpdateMiner() calls could be silently lost. */
    uint32_t StatelessMinerManager::NotifyNewRound(uint32_t nNewHeight)
    {
        /* Update tracked height */
        uint32_t nOldHeight = nCurrentHeight.exchange(nNewHeight);

        /* Only process if height actually changed */
        if(nOldHeight == nNewHeight)
            return 0;

        /* Atomically transform all entries — no snapshot, no stale overwrites */
        uint32_t nNotified = mapMiners.TransformAll(
            [nNewHeight](const MiningContext& ctx) {
                return ctx.WithHeight(nNewHeight);
            });

        if(nNotified > 0)
        {
            debug::log(2, FUNCTION, "Notified ", nNotified, " miners of new round at height ", nNewHeight);
        }

        return nNotified;
    }


    /* Get current tracked block height */
    uint32_t StatelessMinerManager::GetCurrentHeight() const
    {
        return nCurrentHeight.load();
    }


    /* Set current tracked block height */
    void StatelessMinerManager::SetCurrentHeight(uint32_t nHeight)
    {
        nCurrentHeight.store(nHeight);
    }


    /* Check if new round has started */
    bool StatelessMinerManager::IsNewRound(uint32_t nLastHeight) const
    {
        return nCurrentHeight.load() != nLastHeight;
    }


    /* Get miners for specific channel.
     * Uses ForEach() to avoid full snapshot copy. */
    std::vector<MiningContext> StatelessMinerManager::GetMinersForChannel(uint32_t nChannel) const
    {
        std::vector<MiningContext> vResult;

        mapMiners.ForEach([&](const std::string& key, const MiningContext& ctx) {
            if(ctx.nChannel == nChannel)
                vResult.push_back(ctx);
        });

        return vResult;
    }


    /* Get total templates served (lock-free via atomic) */
    uint64_t StatelessMinerManager::GetTotalTemplatesServed() const
    {
        return nTotalTemplatesServed.load();
    }


    /* Increment templates served counter */
    void StatelessMinerManager::IncrementTemplatesServed()
    {
        ++nTotalTemplatesServed;
    }


    /* Get total blocks submitted (lock-free via atomic) */
    uint64_t StatelessMinerManager::GetTotalBlocksSubmitted() const
    {
        return nTotalBlocksSubmitted.load();
    }


    /* Increment blocks submitted counter */
    void StatelessMinerManager::IncrementBlocksSubmitted()
    {
        ++nTotalBlocksSubmitted;
    }


    /* Get total blocks accepted (lock-free via atomic) */
    uint64_t StatelessMinerManager::GetTotalBlocksAccepted() const
    {
        return nTotalBlocksAccepted.load();
    }


    /* Increment blocks accepted counter */
    void StatelessMinerManager::IncrementBlocksAccepted()
    {
        ++nTotalBlocksAccepted;
    }


    /* Purge inactive miners based on cache timeout.
     *
     * This is a CACHE HYGIENE function, NOT a session liveness function.
     * Session liveness is governed exclusively by CleanupInactive() which uses
     * the 24-hour 3-way AND check (activity + keepalive count + grace period).
     *
     * PurgeInactiveMiners runs on a much longer timescale:
     *   - Remote miners:    7 days   (604800s)  via DEFAULT_CACHE_PURGE_TIMEOUT
     *   - Localhost miners: 30 days  (2592000s) via LOCALHOST_CACHE_PURGE_TIMEOUT
     *
     * Its purpose is to clean up stale map entries for miners that disconnected
     * long ago but whose context was not removed (e.g., due to unclean TCP close).
     * It does NOT affect active miners with keepalives. */
    uint32_t StatelessMinerManager::PurgeInactiveMiners()
    {
        uint32_t nRemoved = 0;
        uint64_t nNow = runtime::unifiedtimestamp();

        auto pairs = mapMiners.GetAllPairs();
        for(const auto& pair : pairs)
        {
            const MiningContext& ctx = pair.second;
            
            /* Get appropriate timeout based on address (localhost vs remote) */
            uint64_t nPurgeTimeout = NodeCache::GetPurgeTimeout(ctx.strAddress);
            
            /* Check if miner has been inactive for longer than purge timeout.
             * Uses IsConsideredInactive() to respect keepalive grace — a miner
             * that is still sending keepalives should never be purged regardless
             * of how long ago the session started. */
            if(ctx.IsConsideredInactive(nNow, nPurgeTimeout))
            {
                debug::log(2, FUNCTION, "Purging inactive miner ", ctx.strAddress, 
                          " (inactive for ", (nNow - ctx.nTimestamp), " seconds)");

                /* RemoveMiner() handles cross-cache cleanup (NodeSessionRegistry
                 * MarkDisconnected) internally. */
                if(RemoveMiner(pair.first))
                    ++nRemoved;
            }
        }

        if(nRemoved > 0)
        {
            debug::log(1, FUNCTION, "Purged ", nRemoved, " inactive miners from cache");
        }

        return nRemoved;
    }


    /* Enforce cache size limit for DDOS protection */
    uint32_t StatelessMinerManager::EnforceCacheLimit(size_t nMaxSize)
    {
        size_t nCurrentSize = GetMinerCount();
        
        /* No action needed if under limit */
        if(nCurrentSize <= nMaxSize)
            return 0;

        uint32_t nToRemove = static_cast<uint32_t>(nCurrentSize - nMaxSize);
        uint32_t nRemoved = 0;

        debug::log(1, FUNCTION, "Cache limit exceeded (", nCurrentSize, "/", nMaxSize, 
                  "), removing ", nToRemove, " least recently active entries");

        /* Get all miners sorted by last activity timestamp */
        auto pairs = mapMiners.GetAllPairs();
        std::vector<std::pair<std::string, MiningContext>> vMiners(pairs.begin(), pairs.end());

        /* Sort by timestamp ascending (least recently active first) */
        /* This purges inactive older miners to allow newer active miners */
        std::sort(vMiners.begin(), vMiners.end(), 
            [](const auto& a, const auto& b) {
                return a.second.nTimestamp < b.second.nTimestamp;
            });

        /* Helper: re-read live state to avoid removing a miner that was
         * refreshed after the snapshot was taken. Returns nullopt if gone. */
        auto fnGetLive = [this](const std::string& strAddr) -> std::optional<MiningContext>
        {
            return mapMiners.Get(strAddr);
        };

        /* Remove least recently active miners first, but prefer unauthenticated and localhost last.
         * Re-read the live entry before removing to avoid stale-snapshot deletes. */
        for(const auto& pair : vMiners)
        {
            if(nRemoved >= nToRemove)
                break;

            auto optLive = fnGetLive(pair.first);
            if(!optLive.has_value())
                continue;
            const MiningContext& ctx = optLive.value();

            /* Skip localhost miners in first pass */
            if(NodeCache::IsLocalhost(ctx.strAddress))
                continue;

            /* Remove unauthenticated miners first */
            if(!ctx.fAuthenticated)
            {
                if(RemoveMiner(pair.first))
                    ++nRemoved;
            }
        }

        /* Second pass: remove authenticated remote miners if needed */
        if(nRemoved < nToRemove)
        {
            for(const auto& pair : vMiners)
            {
                if(nRemoved >= nToRemove)
                    break;

                auto optLive = fnGetLive(pair.first);
                if(!optLive.has_value())
                    continue;
                const MiningContext& ctx = optLive.value();

                /* Skip localhost miners */
                if(NodeCache::IsLocalhost(ctx.strAddress))
                    continue;

                if(ctx.fAuthenticated)
                {
                    if(RemoveMiner(pair.first))
                        ++nRemoved;
                }
            }
        }

        /* Final pass: remove localhost miners if absolutely necessary */
        if(nRemoved < nToRemove)
        {
            for(const auto& pair : vMiners)
            {
                if(nRemoved >= nToRemove)
                    break;

                if(!fnGetLive(pair.first).has_value())
                    continue;

                if(RemoveMiner(pair.first))
                    ++nRemoved;
            }
        }

        if(nRemoved > 0)
        {
            debug::log(1, FUNCTION, "Enforced cache limit: removed ", nRemoved, 
                      " miners (cache now at ", GetMinerCount(), "/", nMaxSize, ")");
        }

        return nRemoved;
    }


    /* Check if miner needs to send keepalive */
    bool StatelessMinerManager::CheckKeepaliveRequired(
        const std::string& strAddress,
        uint64_t nInterval
    ) const
    {
        auto optContext = mapMiners.Get(strAddress);
        if(!optContext.has_value())
            return false;

        const MiningContext& ctx = optContext.value();
        uint64_t nNow = runtime::unifiedtimestamp();

        /* Check if time since last activity exceeds interval */
        return (nNow - ctx.nTimestamp) > nInterval;
    }


    /* Get reward address for a miner */
    uint256_t StatelessMinerManager::GetRewardAddress(const std::string& strAddress) const
    {
        auto optContext = mapMiners.Get(strAddress);
        if(!optContext.has_value())
            return uint256_t(0);

        return optContext.value().hashGenesis;
    }


    /* Get count of miners with reward address bound.
     * Uses ForEach() to avoid full snapshot copy. */
    size_t StatelessMinerManager::GetRewardBoundCount() const
    {
        size_t nCount = 0;

        mapMiners.ForEach([&](const std::string& key, const MiningContext& ctx) {
            if(ctx.fRewardBound && ctx.hashRewardAddress != 0)
                ++nCount;
        });

        return nCount;
    }


    /* Validate a miner's genesis hash (account resolution removed - obsolete) */
    uint8_t StatelessMinerManager::ValidateMinerGenesis(
        const std::string& strAddress,
        TAO::Register::Address& hashDefault) const
    {
        /* Get miner context */
        auto optContext = mapMiners.Get(strAddress);
        if(!optContext.has_value())
        {
            debug::log(0, FUNCTION, "Miner not found: ", strAddress);
            return LLP::GenesisConstants::NOT_ON_CHAIN;
        }

        const MiningContext& context = optContext.value();

        /* Check if genesis is set */
        if(context.hashGenesis == 0)
        {
            debug::log(2, FUNCTION, "Miner has zero genesis: ", strAddress);
            return LLP::GenesisConstants::ZERO_GENESIS;
        }

        /* Validate genesis */
        LLP::GenesisConstants::ValidationResult result = 
            LLP::GenesisConstants::ValidateGenesis(context.hashGenesis);
        
        if(result != LLP::GenesisConstants::VALID)
        {
            debug::log(0, FUNCTION, "Genesis validation failed for ", strAddress, ": ",
                      LLP::GenesisConstants::GetValidationResultString(result));
            return result;
        }

        /* Note: Account resolution has been removed. With the new Direct Reward Address system,
         * miners provide reward addresses directly via MINER_SET_REWARD. */
        hashDefault = uint256_t(0);

        return LLP::GenesisConstants::VALID;
    }


    /* Get a list of all active miners with simplified information.
     * Uses ForEach() to avoid full snapshot copy. */
    std::vector<MinerInfo> StatelessMinerManager::GetMinerList() const
    {
        std::vector<MinerInfo> vResult;

        mapMiners.ForEach([&](const std::string& key, const MiningContext& ctx) {
            MinerInfo info;
            info.hashGenesis = ctx.hashGenesis;
            info.hashRewardAddress = ctx.hashRewardAddress;
            info.fAuthenticated = ctx.fAuthenticated;
            info.fRewardBound = ctx.fRewardBound;
            info.nSessionId = ctx.nSessionId;
            info.strAddress = ctx.strAddress;

            vResult.push_back(info);
        });

        return vResult;
    }


} // namespace LLP
