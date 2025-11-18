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

#include <Util/include/runtime.h>
#include <Util/include/json.h>
#include <Util/include/mutex.h>
#include <Util/include/hex.h>

#include <map>
#include <mutex>

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

    public:
        FalconAuthImpl() = default;
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
            /* Use SK256 hash of public key as stable identifier */
            return LLC::SK256(pubkey);
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
