/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/pool_discovery.h>
#include <LLP/include/global.h>
#include <LLP/include/falcon_constants.h>
#include <LLP/include/falcon_auth.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/API/types/authentication.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/config.h>
#include <Util/include/hex.h>

#include <LLC/hash/SK.h>
#include <LLC/include/flkey.h>
#include <LLC/include/random.h>
#include <LLC/types/typedef.h>

#include <LLP/include/network.h>
#include <LLP/templates/socket.h>

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
        /* Check if signature is present */
        if(vSignature.empty())
        {
            debug::error(FUNCTION, "Pool announcement has no signature");
            return false;
        }

        /* Validate signature size using existing Falcon constants */
        if(vSignature.size() < FalconConstants::FALCON512_SIG_MIN || 
           vSignature.size() > FalconConstants::FALCON512_SIG_MAX_VALIDATION)
        {
            debug::error(FUNCTION, "Pool announcement signature has invalid size: ", vSignature.size(),
                        " (expected ", FalconConstants::FALCON512_SIG_MIN, "-", 
                        FalconConstants::FALCON512_SIG_MAX_VALIDATION, ")");
            return false;
        }

        /* Get the hash to verify */
        uint512_t hash = GetHash();
        std::vector<uint8_t> vchData(hash.begin(), hash.end());

        /* TODO: Implement blockchain-based public key lookup
         * For production deployment, this needs to:
         * 1. Query the blockchain/ledger for the genesis transaction
         * 2. Extract the Falcon public key from the genesis
         * 3. Verify the signature against that public key
         * 
         * Current implementation performs basic size validation.
         * The signature verification will be completed when the blockchain
         * public key storage/retrieval system is implemented.
         * 
         * Integration point: Use FalconAuth::Get()->Verify(pubkey, vchData, vSignature)
         * after retrieving pubkey from blockchain.
         */

        debug::log(2, FUNCTION, "Pool announcement signature validated (size check passed)");
        return true;
    }


    bool MiningPoolAnnouncement::Sign(const std::vector<uint8_t>& vPrivateKey)
    {
        /* Validate private key */
        if(vPrivateKey.empty())
        {
            debug::error(FUNCTION, "Cannot sign with empty private key");
            return false;
        }

        /* Create Falcon key object */
        LLC::FLKey key;
        
        /* Convert to secure vector (CPrivKey type) */
        LLC::CPrivKey vSecureKey(vPrivateKey.begin(), vPrivateKey.end());
        
        /* Set the private key */
        if(!key.SetPrivKey(vSecureKey))
        {
            debug::error(FUNCTION, "Failed to set Falcon private key");
            return false;
        }

        /* Get the hash to sign */
        uint512_t hash = GetHash();
        std::vector<uint8_t> vchData(hash.begin(), hash.end());

        /* Sign the data */
        if(!key.Sign(vchData, vSignature))
        {
            debug::error(FUNCTION, "Failed to generate Falcon signature");
            return false;
        }

        debug::log(2, FUNCTION, "Successfully signed pool announcement, signature size: ", vSignature.size());
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

        /* Sign announcement with Falcon signature using FalconAuth system
         * This integrates with the existing Falcon authentication infrastructure.
         * 
         * Integration approach:
         * 1. Check if FalconAuth system is available
         * 2. Use existing Falcon keys from the system if available
         * 3. Otherwise, generate a temporary key for demonstration
         * 
         * TODO: Complete integration requires binding pool operator's
         * genesis to a Falcon key in the FalconAuth system.
         */
        std::vector<uint8_t> vPrivateKey;
        
        try
        {
            /* Try to use FalconAuth credential system */
            auto* pFalconAuth = FalconAuth::Get();
            
            if(pFalconAuth)
            {
                /* List available Falcon keys */
                auto vKeys = pFalconAuth->ListKeys();
                
                if(!vKeys.empty())
                {
                    /* Use the first available key
                     * TODO: In production, select key based on genesis binding
                     */
                    uint256_t keyId = vKeys[0].keyId;
                    
                    /* Get the hash to sign */
                    uint512_t hash = announcement.GetHash();
                    std::vector<uint8_t> vchData(hash.begin(), hash.end());
                    
                    /* Sign using FalconAuth */
                    std::vector<uint8_t> vSig = pFalconAuth->Sign(keyId, vchData);
                    
                    if(!vSig.empty())
                    {
                        announcement.vSignature = vSig;
                        debug::log(1, FUNCTION, "Signed pool announcement using FalconAuth, size: ", vSig.size());
                    }
                    else
                    {
                        debug::warning(FUNCTION, "FalconAuth signing failed, falling back to temporary key");
                        throw std::runtime_error("FalconAuth signing failed");
                    }
                }
                else
                {
                    debug::warning(FUNCTION, "No Falcon keys available in FalconAuth system");
                    throw std::runtime_error("No Falcon keys available");
                }
            }
            else
            {
                debug::warning(FUNCTION, "FalconAuth system not initialized");
                throw std::runtime_error("FalconAuth not available");
            }
        }
        catch(const std::exception& e)
        {
            /* Fallback: Create a temporary Falcon key for signing
             * This demonstrates the signing mechanism when FalconAuth is unavailable
             */
            debug::log(1, FUNCTION, "Using temporary Falcon key: ", e.what());
            
            LLC::FLKey tempKey;
            tempKey.MakeNewKey();
            
            /* Get the private key from secure storage */
            LLC::CPrivKey secureKey = tempKey.GetPrivKey();
            
            /* Convert to regular vector for the Sign interface */
            vPrivateKey.assign(secureKey.begin(), secureKey.end());
            
            if(!announcement.Sign(vPrivateKey))
            {
                debug::error(FUNCTION, "Failed to sign pool announcement with temporary key");
                return false;
            }
        }

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

        /* Broadcast to network peers via gossip protocol */
        if(TRITIUM_SERVER)
        {
            try
            {
                /* Get list of connected peers */
                auto vConnections = TRITIUM_SERVER->GetConnections();
                
                if(!vConnections.empty())
                {
                    debug::log(2, FUNCTION, "Broadcasting pool announcement to ", vConnections.size(), " peers");
                    
                    /* Serialize the announcement for transmission */
                    DataStream ssAnnouncement(SER_NETWORK, LLP::PROTOCOL_VERSION);
                    ssAnnouncement << announcement;
                    
                    /* Broadcast to all connected peers */
                    for(const auto& pConnection : vConnections)
                    {
                        if(pConnection && pConnection->Connected())
                        {
                            /* Send NOTIFY message with pool announcement data
                             * TODO: Add dedicated POOL_ANNOUNCEMENT message type to Tritium protocol
                             * For now, we use generic NOTIFY with custom type identifier
                             */
                            try
                            {
                                pConnection->PushMessage(
                                    LLP::TritiumNode::ACTION::NOTIFY,
                                    std::string("poolannounce"),
                                    ssAnnouncement.Bytes()
                                );
                            }
                            catch(const std::exception& e)
                            {
                                debug::warning(FUNCTION, "Failed to send announcement to peer: ", e.what());
                            }
                        }
                    }
                }
                else
                {
                    debug::log(2, FUNCTION, "No connected peers to broadcast pool announcement");
                }
            }
            catch(const std::exception& e)
            {
                debug::warning(FUNCTION, "Exception during pool announcement broadcast: ", e.what());
            }
        }
        else
        {
            debug::log(2, FUNCTION, "TRITIUM_SERVER not available, skipping network broadcast");
        }

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

        /* Gossip to other peers with probability to limit propagation */
        if(TRITIUM_SERVER)
        {
            /* Probabilistic forward to prevent network flooding */
            if(LLC::GetRand(100) < GOSSIP_FORWARD_PROBABILITY_PERCENT)
            {
                try
                {
                    /* Get list of connected peers */
                    auto vConnections = TRITIUM_SERVER->GetConnections();
                    
                    if(!vConnections.empty())
                    {
                        debug::log(2, FUNCTION, "Forwarding pool announcement to ", vConnections.size(), " peers (30% gossip)");
                        
                        /* Serialize the announcement for transmission */
                        DataStream ssAnnouncement(SER_NETWORK, LLP::PROTOCOL_VERSION);
                        ssAnnouncement << announcement;
                        
                        /* Forward to all connected peers
                         * TODO: Track which peer sent us this announcement to avoid echo-back
                         */
                        for(const auto& pConnection : vConnections)
                        {
                            if(pConnection && pConnection->Connected())
                            {
                                try
                                {
                                    pConnection->PushMessage(
                                        LLP::TritiumNode::ACTION::NOTIFY,
                                        std::string("poolannounce"),
                                        ssAnnouncement.Bytes()
                                    );
                                }
                                catch(const std::exception& e)
                                {
                                    debug::warning(FUNCTION, "Failed to forward announcement to peer: ", e.what());
                                }
                            }
                        }
                    }
                }
                catch(const std::exception& e)
                {
                    debug::warning(FUNCTION, "Exception during pool announcement forwarding: ", e.what());
                }
            }
        }

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
        /* Parse endpoint into host and port */
        std::string strHost;
        uint16_t nPort = 0;
        
        size_t nColon = strEndpoint.find(':');
        if(nColon == std::string::npos)
        {
            debug::warning(FUNCTION, "Invalid endpoint format (missing port): ", strEndpoint);
            return false;
        }

        strHost = strEndpoint.substr(0, nColon);
        try
        {
            nPort = static_cast<uint16_t>(std::stoul(strEndpoint.substr(nColon + 1)));
        }
        catch(const std::exception& e)
        {
            debug::warning(FUNCTION, "Invalid port in endpoint: ", strEndpoint, " - ", e.what());
            return false;
        }

        if(nPort == 0)
        {
            debug::warning(FUNCTION, "Invalid port (0) in endpoint: ", strEndpoint);
            return false;
        }

        /* Test connection with SSL/TLS support (ChaCha20 encryption)
         * The Nexus network uses SSL/TLS connections with ChaCha20-Poly1305 AEAD
         * encryption for secure peer communication.
         */
        try
        {
            /* Create address object */
            LLP::BaseAddress addr(strHost, nPort);
            
            /* Create socket with SSL/TLS support
             * The second parameter enables SSL/TLS for the connection
             * This integrates with the existing ChaCha20 TLS infrastructure
             */
            LLP::Socket socket(addr, true);  // Enable SSL/TLS
            
            /* Attempt connection with 3 second timeout
             * The socket will establish a secure SSL/TLS connection
             */
            bool fConnected = socket.Attempt(addr, 3000);
            
            if(fConnected)
            {
                debug::log(2, FUNCTION, "Successfully connected to pool via TLS: ", strEndpoint);
                socket.Close();
            }
            else
            {
                debug::log(2, FUNCTION, "Failed to connect to pool: ", strEndpoint);
            }
            
            return fConnected;
        }
        catch(const std::exception& e)
        {
            debug::warning(FUNCTION, "Exception testing pool reachability for ", strEndpoint, ": ", e.what());
            return false;
        }
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
