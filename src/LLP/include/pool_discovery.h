/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_POOL_DISCOVERY_H
#define NEXUS_LLP_INCLUDE_POOL_DISCOVERY_H

#include <LLC/types/uint1024.h>
#include <Util/templates/datastream.h>

#include <string>
#include <cstdint>
#include <vector>
#include <map>
#include <mutex>

namespace LLP
{
    /** Maximum pool fee percentage (hard cap) */
    const uint8_t MAX_POOL_FEE_PERCENT = 5;

    /** Minimum announcement interval (prevent spam) */
    const uint64_t MIN_ANNOUNCEMENT_INTERVAL = 3600; // 1 hour

    /** Pool cache TTL (24 hours) */
    const uint64_t POOL_CACHE_TTL = 86400;

    /** Gossip protocol forward probability (30%) */
    const uint8_t GOSSIP_FORWARD_PROBABILITY_PERCENT = 30;


    /** LocalPoolConfig
     *  Configuration for running this node as a mining pool.
     */
    struct LocalPoolConfig
    {
        bool fEnabled = false;                    // Pool mode enabled
        std::string strPoolName = "Nexus Pool";   // Display name
        uint8_t nFeePercent = 0;                  // 0-10% (0 = no fee)
        std::string strFeeAddress = "";           // Fee recipient
        uint16_t nMaxMiners = 1000;               // Connection limit
        uint64_t nMinPayoutNXS = 1000000;         // Min payout (0.01 NXS)
        bool fAutoAnnounce = true;                // Broadcast to network
        uint32_t nAnnounceIntervalSec = 300;      // Announcement frequency
        bool fUseFalcon1024 = true;               // Use Falcon-1024 for signatures (default)
    };

    /** MinerSession
     *  Tracks an authenticated miner's session and statistics.
     */
    struct MinerSession
    {
        uint512_t hashGenesis;                    // Miner identity
        std::string strAddress;                   // IP address
        uint32_t nChannel = 0;                    // 1=Prime, 2=Hash
        uint64_t nConnectTime = 0;                // Connection timestamp
        uint64_t nLastActivityTime = 0;           // Last activity
        
        // Per-miner statistics (non-atomic since map is mutex-protected)
        uint64_t nSharesSubmitted = 0;
        uint64_t nSharesAccepted = 0;
        uint64_t nBlocksFound = 0;
        uint64_t nTotalRewardNXS = 0;             // Accumulated rewards
        
        // Falcon version used by this miner
        bool fUsesFalcon1024 = true;              // Track miner's Falcon version
        
        MinerSession() = default;
    };

    /** PoolMetrics
     *  Aggregated metrics for the local pool.
     *  Note: Uses non-atomic types since returned by value.
     *  Access protected by LOCAL_MUTEX in PoolDiscovery.
     */
    struct PoolMetrics
    {
        // Connection metrics
        uint32_t nActiveConnections = 0;
        uint32_t nTotalConnections = 0;
        uint32_t nAuthenticatedMiners = 0;
        uint32_t nFalcon512Miners = 0;           // Miners using Falcon-512
        uint32_t nFalcon1024Miners = 0;          // Miners using Falcon-1024
        
        // Block metrics
        uint64_t nBlocksSubmitted = 0;
        uint64_t nBlocksAccepted = 0;
        uint64_t nBlocksRejected = 0;
        
        // Protection metrics (from AutoCooldownManager)
        uint32_t nActiveCooldowns = 0;
        uint64_t nRateLimitViolations = 0;
        
        // Reward metrics
        uint64_t nTotalRewardsNXS = 0;
        uint64_t nTotalFeesNXS = 0;
        
        // Uptime
        uint64_t nStartTime = 0;
        
        PoolMetrics() : nStartTime(0) {}
    };


    /** MiningPoolAnnouncement
     *
     *  Structure broadcast by pool operators to announce their mining service.
     *  Signed with pool operator's genesis credentials for verification.
     *
     **/
    struct MiningPoolAnnouncement
    {
        uint256_t hashGenesis;           // Pool operator's genesis (identity)
        std::string strEndpoint;         // IP:PORT (auto-detected or configured)
        uint8_t nFeePercent;             // Pool fee (0-5%)
        uint16_t nMaxMiners;             // Max concurrent miners
        uint16_t nCurrentMiners;         // Current connected miners
        uint64_t nTotalHashrate;         // Total pool hashrate
        uint32_t nBlocksFound;           // Lifetime blocks found
        std::string strPoolName;         // Human-readable name
        uint64_t nTimestamp;             // Announcement timestamp
        std::vector<uint8_t> vSignature; // Signed with pool operator's key

        /** Default Constructor **/
        MiningPoolAnnouncement();

        /** Serialization **/
        IMPLEMENT_SERIALIZE
        (
            READWRITE(hashGenesis);
            READWRITE(strEndpoint);
            READWRITE(nFeePercent);
            READWRITE(nMaxMiners);
            READWRITE(nCurrentMiners);
            READWRITE(nTotalHashrate);
            READWRITE(nBlocksFound);
            READWRITE(strPoolName);
            READWRITE(nTimestamp);
            READWRITE(vSignature);
        )

        /** GetHash
         *
         *  Get hash of announcement data for signature verification.
         *
         *  @return 512-bit hash of announcement
         *
         **/
        uint512_t GetHash() const;

        /** Verify
         *
         *  Verify the signature on this announcement.
         *
         *  @return true if signature is valid
         *
         **/
        bool Verify() const;

        /** Sign
         *
         *  Sign this announcement with the given private key.
         *
         *  @param[in] vPrivateKey The private key to sign with
         *
         *  @return true if signing succeeded
         *
         **/
        bool Sign(const std::vector<uint8_t>& vPrivateKey);
    };


    /** MiningPoolInfo
     *
     *  Extended pool information including reputation and availability.
     *  Returned by pool discovery queries.
     *
     **/
    struct MiningPoolInfo
    {
        uint256_t hashGenesis;
        std::string strEndpoint;
        uint8_t nFeePercent;
        uint16_t nCurrentMiners;
        uint16_t nMaxMiners;
        uint64_t nTotalHashrate;
        uint32_t nBlocksFound;
        uint32_t nReputation;        // Calculated reputation score (0-100)
        double fUptime;              // Calculated uptime percentage
        std::string strPoolName;
        uint64_t nLastSeen;          // Timestamp of last announcement
        bool fOnline;                // Reachability test result

        /** Default Constructor **/
        MiningPoolInfo();

        /** Constructor from announcement **/
        MiningPoolInfo(const MiningPoolAnnouncement& announcement);
    };


    /** PoolReputation
     *
     *  Track pool reliability and calculate reputation score.
     *
     **/
    struct PoolReputation
    {
        uint256_t hashPoolGenesis;

        // Blockchain metrics (verifiable)
        uint32_t nBlocksFound;           // Lifetime blocks found by pool
        uint32_t nBlocksAccepted;        // Blocks accepted by network
        uint32_t nBlocksOrphaned;        // Orphaned blocks

        // Network metrics (reported)
        uint32_t nAnnouncementCount;     // How many announcements seen
        uint64_t nFirstSeen;             // When pool first appeared
        uint64_t nLastSeen;              // Last announcement timestamp
        uint32_t nDowntimeReports;       // How many times reported offline

        /** Default Constructor **/
        PoolReputation();

        /** CalculateReputation
         *
         *  Calculate reputation score (0-100) based on metrics.
         *
         *  @return Reputation score
         *
         **/
        uint32_t CalculateReputation() const;

        /** CalculateUptime
         *
         *  Calculate uptime percentage based on announcements.
         *
         *  @return Uptime as a percentage (0.0 - 1.0)
         *
         **/
        double CalculateUptime() const;
    };


    /** PoolDiscovery
     *
     *  Manages pool announcement broadcasting, caching, and discovery.
     *  Implements gossip protocol for decentralized pool discovery.
     *
     **/
    class PoolDiscovery
    {
    private:
        /** Local cache of pool announcements **/
        static std::map<uint256_t, MiningPoolAnnouncement> mapPoolCache;

        /** Pool reputation tracking **/
        static std::map<uint256_t, PoolReputation> mapPoolReputation;

        /** Last announcement timestamps (for rate limiting) **/
        static std::map<uint256_t, uint64_t> mapLastAnnouncement;

        /** Mutex for thread-safe access **/
        static std::mutex MUTEX;

        /** Local pool state **/
        static LocalPoolConfig m_localConfig;
        static std::map<uint512_t, MinerSession> m_localSessions;
        static PoolMetrics m_localMetrics;
        static std::mutex LOCAL_MUTEX;

    public:
        /** BroadcastPoolAnnouncement
         *
         *  Create and broadcast a pool announcement.
         *  Called by pool operators to advertise their service.
         *
         *  @param[in] hashGenesis Pool operator's genesis
         *  @param[in] strEndpoint Pool endpoint (IP:PORT)
         *  @param[in] nFeePercent Pool fee percentage
         *  @param[in] nMaxMiners Maximum concurrent miners
         *  @param[in] nCurrentMiners Current connected miners
         *  @param[in] nTotalHashrate Total pool hashrate
         *  @param[in] nBlocksFound Lifetime blocks found
         *  @param[in] strPoolName Human-readable pool name
         *
         *  @return true if broadcast succeeded
         *
         **/
        static bool BroadcastPoolAnnouncement(
            const uint256_t& hashGenesis,
            const std::string& strEndpoint,
            uint8_t nFeePercent,
            uint16_t nMaxMiners,
            uint16_t nCurrentMiners,
            uint64_t nTotalHashrate,
            uint32_t nBlocksFound,
            const std::string& strPoolName
        );

        /** ValidatePoolAnnouncement
         *
         *  Validate a pool announcement before accepting.
         *
         *  @param[in] announcement The announcement to validate
         *
         *  @return true if announcement is valid
         *
         **/
        static bool ValidatePoolAnnouncement(const MiningPoolAnnouncement& announcement);

        /** ProcessPoolAnnouncement
         *
         *  Process a received pool announcement from a peer.
         *
         *  @param[in] announcement The announcement to process
         *
         *  @return true if announcement was accepted
         *
         **/
        static bool ProcessPoolAnnouncement(const MiningPoolAnnouncement& announcement);

        /** ListMiningPools
         *
         *  Query for available mining pools.
         *
         *  @param[in] nMaxFee Filter: maximum fee percentage (default 5)
         *  @param[in] nMinReputation Filter: minimum reputation score (default 0)
         *  @param[in] fOnlineOnly Filter: only responsive pools (default true)
         *
         *  @return List of pool information
         *
         **/
        static std::vector<MiningPoolInfo> ListMiningPools(
            uint8_t nMaxFee = 5,
            uint32_t nMinReputation = 0,
            bool fOnlineOnly = true
        );

        /** GetPoolInfo
         *
         *  Get information about a specific pool.
         *
         *  @param[in] hashGenesis The pool's genesis hash
         *
         *  @return Pool information (may have fOnline=false if not found)
         *
         **/
        static MiningPoolInfo GetPoolInfo(const uint256_t& hashGenesis);

        /** CleanupPoolCache
         *
         *  Remove expired pool announcements from cache.
         *
         **/
        static void CleanupPoolCache();

        /** UpdatePoolReputation
         *
         *  Update reputation metrics for a pool.
         *
         *  @param[in] hashGenesis The pool's genesis hash
         *
         **/
        static void UpdatePoolReputation(const uint256_t& hashGenesis);

        /** IncrementDowntimeReport
         *
         *  Record a downtime report for a pool.
         *
         *  @param[in] hashGenesis The pool's genesis hash
         *
         **/
        static void IncrementDowntimeReport(const uint256_t& hashGenesis);

        /** TestPoolReachability
         *
         *  Test if a pool is reachable at its endpoint.
         *
         *  @param[in] strEndpoint The pool's IP:PORT endpoint
         *
         *  @return true if pool is reachable
         *
         **/
        static bool TestPoolReachability(const std::string& strEndpoint);

        /** DetectPublicEndpoint
         *
         *  Auto-detect the node's public IP address and port.
         *
         *  @return Public endpoint string (IP:PORT)
         *
         **/
        static std::string DetectPublicEndpoint();

        /** RecentlyAnnounced
         *
         *  Check if a pool has announced recently (rate limiting).
         *
         *  @param[in] hashGenesis The pool's genesis hash
         *
         *  @return true if pool announced within MIN_ANNOUNCEMENT_INTERVAL
         *
         **/
        static bool RecentlyAnnounced(const uint256_t& hashGenesis);

        /** GetCachedPools
         *
         *  Get all cached pool announcements.
         *
         *  @return Map of genesis hash to announcement
         *
         **/
        static std::map<uint256_t, MiningPoolAnnouncement> GetCachedPools();

        /** Initialize
         *
         *  Initialize the pool discovery system.
         *
         **/
        static void Initialize();

        /** Shutdown
         *
         *  Shutdown the pool discovery system.
         *
         **/
        static void Shutdown();

        // ═══════════════════════════════════════════════════════════════════
        // LOCAL POOL OPERATIONS (NEW)
        // ═══════════════════════════════════════════════════════════════════
        
        /** SetLocalPoolConfig
         *
         *  Set local pool configuration.
         *
         *  @param[in] config Pool configuration
         *
         **/
        static void SetLocalPoolConfig(const LocalPoolConfig& config);
        
        /** GetLocalPoolConfig
         *
         *  Get local pool configuration.
         *
         *  @return Local pool configuration
         *
         **/
        static const LocalPoolConfig& GetLocalPoolConfig();
        
        /** IsLocalPoolEnabled
         *
         *  Check if local pool mode is enabled.
         *
         *  @return true if local pool mode is enabled
         *
         **/
        static bool IsLocalPoolEnabled();
        
        // Fee Management
        
        /** CalculatePoolFee
         *
         *  Calculate pool fee for a given reward.
         *
         *  @param[in] nRewardNXS Reward amount in NXS
         *
         *  @return Fee amount in NXS
         *
         **/
        static uint64_t CalculatePoolFee(uint64_t nRewardNXS);
        
        /** SetPoolFee
         *
         *  Set pool fee percentage.
         *
         *  @param[in] nFeePercent Fee percentage (0-10)
         *
         *  @return true if fee was set successfully
         *
         **/
        static bool SetPoolFee(uint8_t nFeePercent);
        
        // Session Tracking
        
        /** OnMinerAuthenticated
         *
         *  Record miner authentication.
         *
         *  @param[in] hashGenesis Miner's genesis hash
         *  @param[in] strAddress Miner's IP address
         *  @param[in] nChannel Mining channel (1=Prime, 2=Hash)
         *  @param[in] fUsesFalcon1024 Whether miner uses Falcon-1024
         *
         **/
        static void OnMinerAuthenticated(const uint512_t& hashGenesis, 
                                         const std::string& strAddress, 
                                         uint32_t nChannel,
                                         bool fUsesFalcon1024);
        
        /** OnMinerDisconnected
         *
         *  Record miner disconnection.
         *
         *  @param[in] hashGenesis Miner's genesis hash
         *
         **/
        static void OnMinerDisconnected(const uint512_t& hashGenesis);
        
        /** OnBlockSubmitted
         *
         *  Record block submission.
         *
         *  @param[in] hashGenesis Miner's genesis hash
         *  @param[in] fAccepted Whether block was accepted
         *
         **/
        static void OnBlockSubmitted(const uint512_t& hashGenesis, bool fAccepted);
        
        /** OnBlockFound
         *
         *  Record block found by miner.
         *
         *  @param[in] hashGenesis Miner's genesis hash
         *  @param[in] nRewardNXS Block reward in NXS
         *
         **/
        static void OnBlockFound(const uint512_t& hashGenesis, uint64_t nRewardNXS);
        
        /** GetActiveSessions
         *
         *  Get all active miner sessions.
         *
         *  @return Vector of active sessions
         *
         **/
        static std::vector<MinerSession> GetActiveSessions();
        
        // Metrics Integration
        
        /** GetLocalPoolMetrics
         *
         *  Get local pool metrics.
         *
         *  @return Pool metrics
         *
         **/
        static PoolMetrics GetLocalPoolMetrics();
        
        /** RefreshMetrics
         *
         *  Refresh pool metrics (integrates with AutoCooldownManager).
         *
         **/
        static void RefreshMetrics();
        
        /** GetActiveCooldownCount
         *
         *  Get count of active cooldowns from AutoCooldownManager.
         *
         *  @return Number of active cooldowns
         *
         **/
        static uint32_t GetActiveCooldownCount();
        
        /** GetRateLimitViolationCount
         *
         *  Get count of rate limit violations.
         *
         *  @return Number of rate limit violations
         *
         **/
        static uint64_t GetRateLimitViolationCount();
        
        /** IncrementRateLimitViolations
         *
         *  Increment the rate limit violations counter.
         *
         **/
        static void IncrementRateLimitViolations();
    };

} // namespace LLP

#endif // NEXUS_LLP_INCLUDE_POOL_DISCOVERY_H
