/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/stateless_manager.h>

#include <Util/include/json.h>
#include <Util/include/string.h>
#include <Util/include/runtime.h>
#include <Util/include/debug.h>

#include <optional>
#include <sstream>

namespace LLP
{
    /* Get singleton instance */
    StatelessMinerManager& StatelessMinerManager::Get()
    {
        static StatelessMinerManager instance;
        return instance;
    }

    /* Update or add a miner */
    void StatelessMinerManager::UpdateMiner(
        const std::string& strAddress,
        const MiningContext& context
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

        /* Update main miner map */
        mapMiners.InsertOrUpdate(strAddress, context);

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
            if((nNow - pair.second.nTimestamp) > nTimeoutSec)
            {
                if(RemoveMiner(pair.first))
                    ++nRemoved;
            }
        }

        if(nRemoved > 0)
        {
            debug::log(2, FUNCTION, "Cleaned up ", nRemoved, " inactive miners");
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

} // namespace LLP
