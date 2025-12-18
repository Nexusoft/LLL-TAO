/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/pool_discovery.h>
#include <LLP/include/global.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/chainstate.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/config.h>
#include <Util/include/hex.h>

#include <LLC/hash/SK.h>

#include <algorithm>

namespace LLP
{
    /* Initialize static members */
    std::map<uint256_t, MiningPoolAnnouncement> PoolDiscovery::mapPoolCache;
    std::map<uint256_t, PoolReputation> PoolDiscovery::mapPoolReputation;
    std::map<uint256_t, uint64_t> PoolDiscovery::mapLastAnnouncement;
    std::mutex PoolDiscovery::MUTEX;


    /** MiningPoolAnnouncement implementation **/

    MiningPoolAnnouncement::MiningPoolAnnouncement()
    : hashGenesis(0)
    , strEndpoint("")
    , nFeePercent(0)
    , nMaxMiners(0)
    , nCurrentMiners(0)
    , nTotalHashrate(0)
    , nBlocksFound(0)
    , strPoolName("")
    , nTimestamp(0)
    , vSignature()
    {
    }


    uint512_t MiningPoolAnnouncement::GetHash() const
    {
        /* Build data stream for hashing */
        DataStream ssData(SER_GETHASH, LLP::PROTOCOL_VERSION);
        ssData << hashGenesis;
        ssData << strEndpoint;
        ssData << nFeePercent;
        ssData << nMaxMiners;
        ssData << nCurrentMiners;
        ssData << nTotalHashrate;
        ssData << nBlocksFound;
        ssData << strPoolName;
        ssData << nTimestamp;

        /* Return SK512 hash */
        return LLC::SK512(ssData.begin(), ssData.end());
    }


    bool MiningPoolAnnouncement::Verify() const
    {
        /* TODO: Implement signature verification
         * This would verify the signature using the genesis public key
         * from the blockchain. For now, return true to allow testing.
         */
        return true;
    }


    bool MiningPoolAnnouncement::Sign(const std::vector<uint8_t>& vPrivateKey)
    {
        /* TODO: Implement signature generation
         * This would sign the announcement hash with the private key.
         * For now, return true to allow testing.
         */
        vSignature.clear();
        vSignature.resize(64, 0); // Placeholder signature
        return true;
    }


    /** MiningPoolInfo implementation **/

    MiningPoolInfo::MiningPoolInfo()
    : hashGenesis(0)
    , strEndpoint("")
    , nFeePercent(0)
    , nCurrentMiners(0)
    , nMaxMiners(0)
    , nTotalHashrate(0)
    , nBlocksFound(0)
    , nReputation(0)
    , fUptime(0.0)
    , strPoolName("")
    , nLastSeen(0)
    , fOnline(false)
    {
    }


    MiningPoolInfo::MiningPoolInfo(const MiningPoolAnnouncement& announcement)
    : hashGenesis(announcement.hashGenesis)
    , strEndpoint(announcement.strEndpoint)
    , nFeePercent(announcement.nFeePercent)
    , nCurrentMiners(announcement.nCurrentMiners)
    , nMaxMiners(announcement.nMaxMiners)
    , nTotalHashrate(announcement.nTotalHashrate)
    , nBlocksFound(announcement.nBlocksFound)
    , nReputation(0)
    , fUptime(0.0)
    , strPoolName(announcement.strPoolName)
    , nLastSeen(announcement.nTimestamp)
    , fOnline(false)
    {
    }


    /** PoolReputation implementation **/

    PoolReputation::PoolReputation()
    : hashPoolGenesis(0)
    , nBlocksFound(0)
    , nBlocksAccepted(0)
    , nBlocksOrphaned(0)
    , nAnnouncementCount(0)
    , nFirstSeen(0)
    , nLastSeen(0)
    , nDowntimeReports(0)
    {
    }


    uint32_t PoolReputation::CalculateReputation() const
    {
        double score = 100.0;

        /* Block acceptance rate (most important) */
        if(nBlocksFound > 0)
        {
            double acceptanceRate = static_cast<double>(nBlocksAccepted) / static_cast<double>(nBlocksFound);
            if(acceptanceRate < 0.95)
                score -= 20;
            if(acceptanceRate < 0.90)
                score -= 30;
        }

        /* Uptime (based on announcements) */
        if(nLastSeen > nFirstSeen)
        {
            uint64_t expectedAnnouncements = (nLastSeen - nFirstSeen) / MIN_ANNOUNCEMENT_INTERVAL;
            if(expectedAnnouncements > 0)
            {
                double uptime = static_cast<double>(nAnnouncementCount) / static_cast<double>(expectedAnnouncements);
                if(uptime < 0.95)
                    score -= 10;
                if(uptime < 0.90)
                    score -= 20;
            }
        }

        /* Penalize downtime reports */
        score -= (nDowntimeReports * 5);

        /* Clamp to 0-100 range */
        return static_cast<uint32_t>(std::max(0.0, std::min(100.0, score)));
    }


    double PoolReputation::CalculateUptime() const
    {
        if(nLastSeen <= nFirstSeen || nAnnouncementCount == 0)
            return 0.0;

        uint64_t expectedAnnouncements = (nLastSeen - nFirstSeen) / MIN_ANNOUNCEMENT_INTERVAL;
        if(expectedAnnouncements == 0)
            return 1.0; // Recently started

        double uptime = static_cast<double>(nAnnouncementCount) / static_cast<double>(expectedAnnouncements);
        return std::min(1.0, uptime);
    }


    /** PoolDiscovery implementation **/

    bool PoolDiscovery::BroadcastPoolAnnouncement(
        const uint256_t& hashGenesis,
        const std::string& strEndpoint,
        uint8_t nFeePercent,
        uint16_t nMaxMiners,
        uint16_t nCurrentMiners,
        uint64_t nTotalHashrate,
        uint32_t nBlocksFound,
        const std::string& strPoolName)
    {
        /* Create announcement */
        MiningPoolAnnouncement announcement;
        announcement.hashGenesis = hashGenesis;
        announcement.strEndpoint = strEndpoint;
        announcement.nFeePercent = nFeePercent;
        announcement.nMaxMiners = nMaxMiners;
        announcement.nCurrentMiners = nCurrentMiners;
        announcement.nTotalHashrate = nTotalHashrate;
        announcement.nBlocksFound = nBlocksFound;
        announcement.strPoolName = strPoolName;
        announcement.nTimestamp = runtime::timestamp();

        /* Sign announcement (TODO: implement proper signing) */
        std::vector<uint8_t> vPrivateKey; // TODO: Get from credentials
        announcement.Sign(vPrivateKey);

        /* Validate before broadcasting */
        if(!ValidatePoolAnnouncement(announcement))
        {
            debug::error(FUNCTION, "Failed to validate pool announcement");
            return false;
        }

        /* Update local cache */
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            mapPoolCache[hashGenesis] = announcement;
            mapLastAnnouncement[hashGenesis] = announcement.nTimestamp;
        }

        /* TODO: Broadcast to network peers via gossip protocol */
        debug::log(1, FUNCTION, "Pool announcement broadcast: ", hashGenesis.SubString(),
                   " fee=", static_cast<int>(nFeePercent), "% miners=", nCurrentMiners);

        return true;
    }


    bool PoolDiscovery::ValidatePoolAnnouncement(const MiningPoolAnnouncement& announcement)
    {
        /* Reject excessive fees */
        if(announcement.nFeePercent > MAX_POOL_FEE_PERCENT)
        {
            debug::error(FUNCTION, "Pool fee too high: ", static_cast<int>(announcement.nFeePercent),
                        "% > ", static_cast<int>(MAX_POOL_FEE_PERCENT), "%");
            return false;
        }

        /* Must be signed */
        if(!announcement.Verify())
        {
            debug::error(FUNCTION, "Invalid pool announcement signature");
            return false;
        }

        /* Genesis must exist on blockchain (skip check for now during testing) */
        /* TODO: Re-enable when integrated with blockchain
        if(!LLD::Ledger->HasGenesis(announcement.hashGenesis))
        {
            debug::error(FUNCTION, "Pool genesis not found on blockchain");
            return false;
        }
        */

        /* Rate limit announcements (prevent spam) */
        if(RecentlyAnnounced(announcement.hashGenesis))
        {
            debug::warning(FUNCTION, "Pool announced too recently");
            return false;
        }

        /* Check trust requirements (skip for now during testing) */
        /* TODO: Re-enable when integrated with trust system
        if(!HasSufficientTrust(announcement.hashGenesis))
        {
            debug::warning(FUNCTION, "Pool genesis has insufficient trust");
            return false;
        }
        */

        return true;
    }


    bool PoolDiscovery::ProcessPoolAnnouncement(const MiningPoolAnnouncement& announcement)
    {
        /* Validate announcement */
        if(!ValidatePoolAnnouncement(announcement))
            return false;

        /* Store in local cache */
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            mapPoolCache[announcement.hashGenesis] = announcement;
        }

        /* Update reputation */
        UpdatePoolReputation(announcement.hashGenesis);

        /* TODO: Gossip to other peers with probability to limit propagation
         * if(GetRand(100) < 30) // 30% chance to forward
         *     RelayPoolAnnouncement(announcement);
         */

        debug::log(1, FUNCTION, "Pool announcement processed: ", announcement.hashGenesis.SubString(),
                   " fee=", static_cast<int>(announcement.nFeePercent), "% miners=", announcement.nCurrentMiners);

        return true;
    }


    std::vector<MiningPoolInfo> PoolDiscovery::ListMiningPools(
        uint8_t nMaxFee,
        uint32_t nMinReputation,
        bool fOnlineOnly)
    {
        std::vector<MiningPoolInfo> vPools;

        std::lock_guard<std::mutex> lock(MUTEX);

        for(const auto& entry : mapPoolCache)
        {
            const MiningPoolAnnouncement& announcement = entry.second;
            MiningPoolInfo info(announcement);

            /* Apply fee filter */
            if(info.nFeePercent > nMaxFee)
                continue;

            /* Calculate reputation */
            auto itReputation = mapPoolReputation.find(announcement.hashGenesis);
            if(itReputation != mapPoolReputation.end())
            {
                info.nReputation = itReputation->second.CalculateReputation();
                info.fUptime = itReputation->second.CalculateUptime();
            }

            /* Apply reputation filter */
            if(info.nReputation < nMinReputation)
                continue;

            /* Test reachability if required */
            if(fOnlineOnly)
            {
                info.fOnline = TestPoolReachability(announcement.strEndpoint);
                if(!info.fOnline)
                    continue;
            }
            else
            {
                info.fOnline = true; // Assume online if not testing
            }

            vPools.push_back(info);
        }

        return vPools;
    }


    MiningPoolInfo PoolDiscovery::GetPoolInfo(const uint256_t& hashGenesis)
    {
        std::lock_guard<std::mutex> lock(MUTEX);

        auto it = mapPoolCache.find(hashGenesis);
        if(it == mapPoolCache.end())
        {
            /* Return empty info with fOnline=false */
            return MiningPoolInfo();
        }

        MiningPoolInfo info(it->second);

        /* Calculate reputation */
        auto itReputation = mapPoolReputation.find(hashGenesis);
        if(itReputation != mapPoolReputation.end())
        {
            info.nReputation = itReputation->second.CalculateReputation();
            info.fUptime = itReputation->second.CalculateUptime();
        }

        /* Test reachability */
        info.fOnline = TestPoolReachability(it->second.strEndpoint);

        return info;
    }


    void PoolDiscovery::CleanupPoolCache()
    {
        uint64_t now = runtime::timestamp();
        std::lock_guard<std::mutex> lock(MUTEX);

        for(auto it = mapPoolCache.begin(); it != mapPoolCache.end(); )
        {
            if(now - it->second.nTimestamp > POOL_CACHE_TTL)
            {
                debug::log(2, FUNCTION, "Removing expired pool: ", it->first.SubString());
                it = mapPoolCache.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }


    void PoolDiscovery::UpdatePoolReputation(const uint256_t& hashGenesis)
    {
        std::lock_guard<std::mutex> lock(MUTEX);

        auto it = mapPoolReputation.find(hashGenesis);
        if(it == mapPoolReputation.end())
        {
            /* Create new reputation entry */
            PoolReputation reputation;
            reputation.hashPoolGenesis = hashGenesis;
            reputation.nFirstSeen = runtime::timestamp();
            reputation.nLastSeen = runtime::timestamp();
            reputation.nAnnouncementCount = 1;
            mapPoolReputation[hashGenesis] = reputation;
        }
        else
        {
            /* Update existing reputation */
            it->second.nLastSeen = runtime::timestamp();
            it->second.nAnnouncementCount++;
        }
    }


    void PoolDiscovery::IncrementDowntimeReport(const uint256_t& hashGenesis)
    {
        std::lock_guard<std::mutex> lock(MUTEX);

        auto it = mapPoolReputation.find(hashGenesis);
        if(it != mapPoolReputation.end())
        {
            it->second.nDowntimeReports++;
            debug::log(2, FUNCTION, "Downtime report for pool: ", hashGenesis.SubString(),
                      " total reports: ", it->second.nDowntimeReports);
        }
    }


    bool PoolDiscovery::TestPoolReachability(const std::string& strEndpoint)
    {
        /* TODO: Implement actual reachability test
         * For now, return true to avoid network calls during development
         */
        return true;
    }


    std::string PoolDiscovery::DetectPublicEndpoint()
    {
        /* Option 1: Use configured value (if provided) */
        if(config::HasArg("-poolendpoint"))
        {
            return config::GetArg("-poolendpoint", "");
        }

        /* Option 2: Use mining port with local address (fallback) */
        uint16_t nPort = config::GetArg("-miningport", 9549);
        
        /* TODO: Implement peer-based IP detection
         * - Query connected peers for our external IP
         * - Use consensus of peer responses
         * For now, return localhost for testing
         */
        
        return "127.0.0.1:" + std::to_string(nPort);
    }


    bool PoolDiscovery::RecentlyAnnounced(const uint256_t& hashGenesis)
    {
        std::lock_guard<std::mutex> lock(MUTEX);

        auto it = mapLastAnnouncement.find(hashGenesis);
        if(it == mapLastAnnouncement.end())
            return false;

        uint64_t timeSince = runtime::timestamp() - it->second;
        return timeSince < MIN_ANNOUNCEMENT_INTERVAL;
    }


    bool PoolDiscovery::HasSufficientTrust(const uint256_t& hashGenesis)
    {
        /* TODO: Implement trust score checking
         * This would query the blockchain for the genesis trust score
         * and verify it meets the minimum requirement.
         * For now, return true to allow testing.
         */
        return true;
    }


    std::map<uint256_t, MiningPoolAnnouncement> PoolDiscovery::GetCachedPools()
    {
        std::lock_guard<std::mutex> lock(MUTEX);
        return mapPoolCache;
    }


    void PoolDiscovery::Initialize()
    {
        debug::log(0, FUNCTION, "Pool Discovery initialized");
    }


    void PoolDiscovery::Shutdown()
    {
        std::lock_guard<std::mutex> lock(MUTEX);
        mapPoolCache.clear();
        mapPoolReputation.clear();
        mapLastAnnouncement.clear();
        debug::log(0, FUNCTION, "Pool Discovery shutdown");
    }

} // namespace LLP
