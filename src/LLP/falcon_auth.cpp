/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/falcon_auth.h>

#include <LLC/hash/SK.h>
#include <LLC/include/flkey.h>
#include <LLC/include/random.h>
#include <LLC/include/mining_session_keys.h>

#include <Util/include/runtime.h>
#include <Util/include/json.h>
#include <Util/include/mutex.h>
#include <Util/include/hex.h>
#include <Util/include/debug.h>

#include <map>
#include <mutex>
#include <atomic>

namespace LLP
{
namespace FalconAuth
{
    /* VerifyResult factory methods */
    VerifyResult VerifyResult::Success(const uint256_t& keyId)
    {
        VerifyResult result;
        result.fValid = true;
        result.keyId = keyId;
        result.strError = "";
        return result;
    }

    VerifyResult VerifyResult::Failure(const std::string& error)
    {
        VerifyResult result;
        result.fValid = false;
        result.keyId = 0;
        result.strError = error;
        return result;
    }


    /* Implementation class */
    class FalconAuthImpl : public IFalconAuth
    {
    private:
        /* Stored keys: keyId -> (metadata, private_key) */
        std::map<uint256_t, std::pair<KeyMetadata, LLC::CPrivKey>> mapKeys;

        /* Genesis bindings: keyId -> hashGenesis */
        std::map<uint256_t, uint256_t> mapGenesisBindings;

        /* Mutex for thread safety */
        mutable std::mutex MUTEX;

        /* Challenge configuration */
        ChallengeConfig config;

        /* Statistics for session management */
        std::atomic<uint64_t> nChallengesGenerated{0};
        std::atomic<uint64_t> nChallengesVerified{0};
        std::atomic<uint64_t> nChallengesFailed{0};

    public:
        FalconAuthImpl()
        : mapKeys()
        , mapGenesisBindings()
        , MUTEX()
        , config()
        , nChallengesGenerated(0)
        , nChallengesVerified(0)
        , nChallengesFailed(0)
        {
        }

        FalconAuthImpl(const ChallengeConfig& config_)
        : mapKeys()
        , mapGenesisBindings()
        , MUTEX()
        , config(config_)
        , nChallengesGenerated(0)
        , nChallengesVerified(0)
        , nChallengesFailed(0)
        {
        }

        ~FalconAuthImpl() override = default;

        KeyMetadata GenerateKey(Profile profile, const std::string& label) override
        {
            /* Generate Falcon key pair using LLC */
            LLC::FLKey key;
            
            /* MakeNewKey() does not take parameters - always generates 512-bit keys (log=9) */
            key.MakeNewKey();
            
            if(!key.IsValid())
                throw std::runtime_error("Failed to generate Falcon key");

            /* Get public and private key bytes */
            std::vector<uint8_t> vPubkey = key.GetPubKey();
            LLC::CPrivKey vPrivkey = key.GetPrivKey();

            /* Derive key ID from public key */
            uint256_t keyId = DeriveKeyId(vPubkey);

            /* Create metadata */
            KeyMetadata meta;
            meta.keyId = keyId;
            meta.pubkey = vPubkey;
            meta.profile = profile;
            meta.created = runtime::unifiedtimestamp();
            meta.lastUsed = meta.created;
            meta.label = label.empty() ? "key_" + keyId.SubString() : label;

            /* Store key */
            {
                LOCK(MUTEX);
                mapKeys[keyId] = std::make_pair(meta, vPrivkey);
            }

            return meta;
        }

        std::vector<KeyMetadata> ListKeys() const override
        {
            LOCK(MUTEX);
            std::vector<KeyMetadata> vKeys;
            vKeys.reserve(mapKeys.size());

            for(const auto& pair : mapKeys)
                vKeys.push_back(pair.second.first);

            return vKeys;
        }

        std::optional<KeyMetadata> GetKey(const uint256_t& keyId) const override
        {
            LOCK(MUTEX);
            auto it = mapKeys.find(keyId);
            if(it == mapKeys.end())
                return std::nullopt;

            return it->second.first;
        }

        std::vector<uint8_t> Sign(
            const uint256_t& keyId,
            const std::vector<uint8_t>& message
        ) override
        {
            LOCK(MUTEX);
            auto it = mapKeys.find(keyId);
            if(it == mapKeys.end())
                return std::vector<uint8_t>();

            const LLC::CPrivKey& vPrivkey = it->second.second;

            /* Create FLKey from private key */
            LLC::FLKey key;
            if(!key.SetPrivKey(vPrivkey))
                return std::vector<uint8_t>();

            /* Sign message */
            std::vector<uint8_t> vSignature;
            if(!key.Sign(message, vSignature))
                return std::vector<uint8_t>();

            /* Update last used timestamp */
            it->second.first.lastUsed = runtime::unifiedtimestamp();

            return vSignature;
        }

        VerifyResult Verify(
            const std::vector<uint8_t>& pubkey,
            const std::vector<uint8_t>& message,
            const std::vector<uint8_t>& signature
        ) override
        {
            /* Create FLKey from public key */
            LLC::FLKey key;
            if(!key.SetPubKey(pubkey))
                return VerifyResult::Failure("Invalid public key");

            /* Verify signature */
            if(!key.Verify(message, signature))
                return VerifyResult::Failure("Signature verification failed");

            /* Derive key ID */
            uint256_t keyId = DeriveKeyId(pubkey);

            return VerifyResult::Success(keyId);
        }

        uint256_t DeriveKeyId(const std::vector<uint8_t>& pubkey) const override
        {
            /* Use unified helper for key ID derivation */
            return LLC::MiningSessionKeys::DeriveKeyId(pubkey);
        }

        bool BindGenesis(
            const uint256_t& keyId,
            const uint256_t& hashGenesis
        ) override
        {
            LOCK(MUTEX);
            
            /* Check if key exists */
            if(mapKeys.find(keyId) == mapKeys.end())
                return false;

            /* Store binding */
            mapGenesisBindings[keyId] = hashGenesis;
            return true;
        }

        std::optional<uint256_t> GetBoundGenesis(const uint256_t& keyId) const override
        {
            LOCK(MUTEX);
            auto it = mapGenesisBindings.find(keyId);
            if(it == mapGenesisBindings.end())
                return std::nullopt;

            return it->second;
        }

        std::vector<uint8_t> GenerateChallenge(size_t nActiveSessions) override
        {
            /* Calculate challenge size based on network load */
            size_t nChallengeSize = GetChallengeSize(nActiveSessions);

            /* Generate random challenge bytes */
            std::vector<uint8_t> vChallenge;
            vChallenge.reserve(nChallengeSize);

            /* Use LLC random generator for cryptographic randomness */
            /* Generate challenge in 32-byte chunks using GetRand256 */
            while(vChallenge.size() < nChallengeSize)
            {
                uint256_t nRandom = LLC::GetRand256();
                std::vector<uint8_t> vRandom = nRandom.GetBytes();

                size_t nRemaining = nChallengeSize - vChallenge.size();
                size_t nCopy = std::min(nRemaining, vRandom.size());
                vChallenge.insert(vChallenge.end(), vRandom.begin(), vRandom.begin() + nCopy);
            }

            /* Increment stats */
            ++nChallengesGenerated;

            debug::log(3, FUNCTION, "Generated challenge of size ", nChallengeSize,
                       " for ", nActiveSessions, " active sessions");

            return vChallenge;
        }

        VerifyResult VerifyChallenge(
            const std::vector<uint8_t>& pubkey,
            const std::vector<uint8_t>& challenge,
            const std::vector<uint8_t>& signature,
            uint64_t nTimestamp
        ) override
        {
            /* Check timestamp for replay protection */
            uint64_t nNow = runtime::unifiedtimestamp();
            if(nTimestamp > nNow + 60)  // 60 seconds future tolerance
            {
                ++nChallengesFailed;
                return VerifyResult::Failure("Challenge timestamp is in the future");
            }

            if(nTimestamp < nNow - config.nChallengeTimeout)
            {
                ++nChallengesFailed;
                return VerifyResult::Failure("Challenge has expired");
            }

            /* Verify the signature over the challenge */
            VerifyResult result = Verify(pubkey, challenge, signature);

            if(result.fValid)
                ++nChallengesVerified;
            else
                ++nChallengesFailed;

            return result;
        }

        size_t GetChallengeSize(size_t nActiveSessions) const override
        {
            /* Scale challenge size based on network load */
            /* Below threshold: use minimum size for efficiency */
            /* Above threshold: scale linearly up to maximum */

            if(nActiveSessions <= config.nScaleThreshold)
                return config.nMinChallengeSize;

            /* Calculate scaling factor */
            /* Sessions beyond threshold trigger gradual increase in challenge size */
            size_t nExcess = nActiveSessions - config.nScaleThreshold;
            size_t nRange = config.nMaxChallengeSize - config.nMinChallengeSize;

            /* Scaling ratio: challenge size increases by 1/4 of the range for every
             * nScaleThreshold additional sessions. This provides gradual scaling that:
             * - Reaches 1/4 range increase at 2x threshold (200 sessions if threshold=100)
             * - Reaches 1/2 range increase at 3x threshold (300 sessions)
             * - Reaches full range at 5x threshold (500 sessions)
             * This balances security (larger challenges) vs efficiency (smaller challenges). */
            const size_t SCALE_DIVISOR = 4;  // Sessions needed to increase by 1/4 of range
            size_t nIncrease = (nExcess * nRange) / (SCALE_DIVISOR * config.nScaleThreshold);
            if(nIncrease > nRange)
                nIncrease = nRange;

            return config.nMinChallengeSize + nIncrease;
        }

        std::string GetSessionStats() const override
        {
            encoding::json stats;
            stats["challenges_generated"] = nChallengesGenerated.load();
            stats["challenges_verified"] = nChallengesVerified.load();
            stats["challenges_failed"] = nChallengesFailed.load();
            stats["min_challenge_size"] = config.nMinChallengeSize;
            stats["max_challenge_size"] = config.nMaxChallengeSize;
            stats["scale_threshold"] = config.nScaleThreshold;
            stats["challenge_timeout"] = config.nChallengeTimeout;

            {
                LOCK(MUTEX);
                stats["keys_stored"] = mapKeys.size();
                stats["genesis_bindings"] = mapGenesisBindings.size();
            }

            return stats.dump(4);
        }
    };


    /* Global instance */
    static std::unique_ptr<IFalconAuth> g_pFalconAuth;
    static std::mutex g_MutexInit;

    /* Get global instance */
    IFalconAuth* Get()
    {
        LOCK(g_MutexInit);
        return g_pFalconAuth.get();
    }

    /* Initialize */
    void Initialize()
    {
        LOCK(g_MutexInit);
        if(!g_pFalconAuth)
            g_pFalconAuth = std::make_unique<FalconAuthImpl>();
    }

    /* Initialize with config */
    void InitializeWithConfig(const ChallengeConfig& config)
    {
        LOCK(g_MutexInit);
        if(!g_pFalconAuth)
            g_pFalconAuth = std::make_unique<FalconAuthImpl>(config);
    }

    /* Shutdown */
    void Shutdown()
    {
        LOCK(g_MutexInit);
        g_pFalconAuth.reset();
    }

    /* Convert key metadata to JSON */
    std::string KeyMetadataToJSON(const KeyMetadata& meta)
    {
        encoding::json j;
        j["key_id"] = meta.keyId.ToString();
        j["pubkey"] = HexStr(meta.pubkey);
        j["profile"] = (meta.profile == Profile::FALCON_512) ? "FALCON_512" : "FALCON_1024";
        j["created"] = meta.created;
        j["last_used"] = meta.lastUsed;
        j["label"] = meta.label;
        return j.dump(4);
    }

} // namespace FalconAuth
} // namespace LLP
