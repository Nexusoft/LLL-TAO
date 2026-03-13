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
     * Sessions with a keepalive exchange within this window are not considered idle.
     *
     * This value is intentionally aligned with the miner's maximum degraded recovery
     * window: DEGRADED_MODE_HARD_LIMIT_SECONDS (300s) + 2 keepalive intervals (2×60s = 120s).
     * A miner in DEGRADED MODE sends keepalives every ~60s but may stop sending new
     * MINER_READY for up to 300s while running its escape ladder. Evicting the session
     * during that window would cause Stateless=0 on the next BroadcastChannel event. */
    static const uint64_t KEEPALIVE_GRACE_PERIOD_SEC = 420;

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
        }

        /* Enforce cache limit before adding new miner (DDOS protection) */
        if(fNewMiner)
        {
            EnforceCacheLimit(NodeCache::DEFAULT_MAX_CACHE_SIZE);
        }

        /* Update main miner map */
        mapMiners.InsertOrUpdate(strAddress, context);

        /* Track lane for cross-lane session coordination */
        mapAddressToLane.InsertOrUpdate(strAddress, nLane);

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

        /* Remove from indices */
        if(ctx.hashKeyID != 0)
            mapKeyToAddress.Erase(ctx.hashKeyID);

        if(ctx.nSessionId != 0)
            mapSessionToAddress.Erase(ctx.nSessionId);

        if(ctx.hashGenesis != 0)
            mapGenesisToAddress.Erase(ctx.hashGenesis);

        mapAddressToLane.Erase(strAddress);

        return true;
    }

    /* Remove a miner by key ID */
    bool StatelessMinerManager::RemoveMinerByKeyID(const uint256_t& hashKeyID)
    {
        auto optAddress = mapKeyToAddress.GetAndRemove(hashKeyID);
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

    /* List miners by genesis hash */
    std::vector<MiningContext> StatelessMinerManager::ListMinersByGenesis(
        const uint256_t& hashGenesis
    ) const
    {
        std::vector<MiningContext> vResult;

        /* Iterate all miners and filter by genesis */
        auto vMiners = mapMiners.GetAll();
        for(const auto& ctx : vMiners)
        {
            if(ctx.hashGenesis == hashGenesis)
                vResult.push_back(ctx);
        }

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

    /* Get active session count */
    size_t StatelessMinerManager::GetActiveSessionCount() const
    {
        size_t nCount = 0;
        uint64_t nNow = runtime::unifiedtimestamp();

        auto vMiners = mapMiners.GetAll();
        for(const auto& ctx : vMiners)
        {
            /* Session is active if started and not expired */
            if(ctx.nSessionStart > 0 && !ctx.IsSessionExpired(nNow))
                ++nCount;
        }

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
            uint64_t nTimeSinceActivity = nNow - ctx.nTimestamp;
            uint64_t nTimeSinceKeepalive = (ctx.nLastKeepaliveTime > 0)
                                         ? (nNow - ctx.nLastKeepaliveTime)
                                         : nTimeSinceActivity;

            /* Smart timeout: Only disconnect if truly idle.
             * All conditions must be met:
             * 1. No activity (nTimestamp) for timeout period — nTimestamp is updated by
             *    keepalives, ensuring miners in DEGRADED MODE are not evicted while alive
             * 2. No keepalives ever received (counter is 0) — miners that have sent any
             *    keepalive will not be evicted by this path
             * 3. No recent keepalive exchange within KEEPALIVE_GRACE_PERIOD_SEC — aligned
             *    with DEGRADED_MODE_HARD_LIMIT (300s) + 2 keepalive intervals (120s) */
            bool bTrulyIdle =
                (nTimeSinceActivity > nTimeoutSec) &&
                (ctx.nKeepaliveCount == 0) &&
                (nTimeSinceKeepalive > KEEPALIVE_GRACE_PERIOD_SEC);

            if(bTrulyIdle)
            {
                debug::log(0, FUNCTION, "Session ", ctx.strAddress,
                          " truly idle - activity: ", nTimeSinceActivity, "s, ",
                          "keepalives_rx: ", ctx.nKeepaliveCount, ", ",
                          "keepalives_tx: ", ctx.nKeepaliveSent);

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

    /* Cleanup expired sessions */
    uint32_t StatelessMinerManager::CleanupExpiredSessions()
    {
        uint32_t nRemoved = 0;
        uint64_t nNow = runtime::unifiedtimestamp();

        auto pairs = mapMiners.GetAllPairs();
        for(const auto& pair : pairs)
        {
            /* Use context's own session timeout for expiry check */
            if(pair.second.IsSessionExpired(nNow))
            {
                if(RemoveMiner(pair.first))
                    ++nRemoved;
            }
        }

        if(nRemoved > 0)
        {
            debug::log(2, FUNCTION, "Cleaned up ", nRemoved, " expired sessions");
        }

        return nRemoved;
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

        /* Add session management fields */
        result["session_start"] = ctx.nSessionStart;
        result["session_timeout"] = ctx.nSessionTimeout;
        result["session_duration"] = ctx.GetSessionDuration(nNow);
        result["session_expired"] = ctx.IsSessionExpired(nNow);
        result["keepalive_count"] = ctx.nKeepaliveCount;

        return result.dump(4);
    }

    /* Get all miners status as JSON */
    std::string StatelessMinerManager::GetAllMinersStatus() const
    {
        encoding::json miners = encoding::json::array();
        uint64_t nNow = runtime::unifiedtimestamp();

        auto vMiners = mapMiners.GetAll();
        for(const auto& ctx : vMiners)
        {
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
            miner["session_timeout"] = ctx.nSessionTimeout;
            miner["session_duration"] = ctx.GetSessionDuration(nNow);
            miner["session_expired"] = ctx.IsSessionExpired(nNow);
            miner["keepalive_count"] = ctx.nKeepaliveCount;

            miners.push_back(miner);
        }

        return miners.dump(4);
    }


    /* Notify miners of new round */
    uint32_t StatelessMinerManager::NotifyNewRound(uint32_t nNewHeight)
    {
        /* Update tracked height */
        uint32_t nOldHeight = nCurrentHeight.exchange(nNewHeight);

        /* Only process if height actually changed */
        if(nOldHeight == nNewHeight)
            return 0;

        /* Get all tracked miners and update their contexts with new height */
        uint32_t nNotified = 0;
        auto pairs = mapMiners.GetAllPairs();

        for(const auto& pair : pairs)
        {
            /* Update context with new height */
            MiningContext newCtx = pair.second.WithHeight(nNewHeight);
            mapMiners.InsertOrUpdate(pair.first, newCtx);
            ++nNotified;
        }

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


    /* Get miners for specific channel */
    std::vector<MiningContext> StatelessMinerManager::GetMinersForChannel(uint32_t nChannel) const
    {
        std::vector<MiningContext> vResult;

        auto vMiners = mapMiners.GetAll();
        for(const auto& ctx : vMiners)
        {
            if(ctx.nChannel == nChannel)
                vResult.push_back(ctx);
        }

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


    /* Purge inactive miners based on cache timeout */
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
            
            /* Check if miner has been inactive for longer than purge timeout */
            if((nNow - ctx.nTimestamp) > nPurgeTimeout)
            {
                debug::log(2, FUNCTION, "Purging inactive miner ", ctx.strAddress, 
                          " (inactive for ", (nNow - ctx.nTimestamp), " seconds)");
                
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

        /* Remove least recently active miners first, but prefer unauthenticated and localhost last */
        for(const auto& pair : vMiners)
        {
            if(nRemoved >= nToRemove)
                break;

            const MiningContext& ctx = pair.second;
            
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

                const MiningContext& ctx = pair.second;
                
                /* Skip localhost miners */
                if(NodeCache::IsLocalhost(ctx.strAddress))
                    continue;

                /* Remove authenticated miners */
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


    /* Get count of miners with reward address bound */
    size_t StatelessMinerManager::GetRewardBoundCount() const
    {
        size_t nCount = 0;
        auto vMiners = mapMiners.GetAll();

        for(const auto& ctx : vMiners)
        {
            /* Count miners with reward address explicitly bound */
            if(ctx.fRewardBound && ctx.hashRewardAddress != 0)
                ++nCount;
        }

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


    /* Get a list of all active miners with simplified information */
    std::vector<MinerInfo> StatelessMinerManager::GetMinerList() const
    {
        std::vector<MinerInfo> vResult;
        auto vMiners = mapMiners.GetAll();

        for(const auto& ctx : vMiners)
        {
            MinerInfo info;
            info.hashGenesis = ctx.hashGenesis;
            info.hashRewardAddress = ctx.hashRewardAddress;
            info.fAuthenticated = ctx.fAuthenticated;
            info.fRewardBound = ctx.fRewardBound;
            info.nSessionId = ctx.nSessionId;
            info.strAddress = ctx.strAddress;

            vResult.push_back(info);
        }

        return vResult;
    }


} // namespace LLP
