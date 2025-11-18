/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/system.h>
#include <TAO/API/types/exception.h>

#include <LLP/include/stateless_manager.h>
#include <LLP/include/falcon_auth.h>

#include <Util/include/json.h>
#include <Util/include/convert.h>
#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* List all active stateless miners */
    encoding::json System::ListMiners(const encoding::json& params, const bool fHelp)
    {
        /* Check for help flag. */
        if(fHelp || params.size() != 0)
            return std::string(
                "listminers - Returns an array of all active stateless miners with their status.");

        /* Get all miners from manager */
        std::vector<LLP::MiningContext> vMiners = LLP::StatelessMinerManager::Get().ListMiners();

        /* Build JSON array */
        encoding::json result = encoding::json::array();

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

            result.push_back(miner);
        }

        return result;
    }


    /* Get status for a specific miner */
    encoding::json System::GetMinerStatus(const encoding::json& params, const bool fHelp)
    {
        /* Check for help flag. */
        if(fHelp || params.size() != 1)
            return std::string(
                "getminerstatus <address> - Returns status for a specific miner by address.");

        /* Get address parameter */
        std::string strAddress;
        if(params.is_array() && params.size() > 0)
            strAddress = params[0].get<std::string>();
        else if(params.find("address") != params.end())
            strAddress = params["address"].get<std::string>();
        else
            throw Exception(-1, "Missing address parameter");

        /* Get miner context */
        std::optional<LLP::MiningContext> ctx = 
            LLP::StatelessMinerManager::Get().GetMinerContext(strAddress);

        if(!ctx.has_value())
            throw Exception(-2, "Miner not found");

        /* Build JSON response */
        encoding::json result;
        result["address"] = ctx->strAddress;
        result["channel"] = ctx->nChannel;
        result["height"] = ctx->nHeight;
        result["authenticated"] = ctx->fAuthenticated;
        result["session_id"] = ctx->nSessionId;
        result["protocol_version"] = ctx->nProtocolVersion;
        result["last_seen"] = ctx->nTimestamp;
        result["key_id"] = ctx->hashKeyID.ToString();
        result["genesis"] = ctx->hashGenesis.ToString();

        return result;
    }


    /* Disconnect a specific miner */
    encoding::json System::DisconnectMiner(const encoding::json& params, const bool fHelp)
    {
        /* Check for help flag. */
        if(fHelp || params.size() != 1)
            return std::string(
                "disconnectminer <address> - Disconnect a specific miner by address.");

        /* Get address parameter */
        std::string strAddress;
        if(params.is_array() && params.size() > 0)
            strAddress = params[0].get<std::string>();
        else if(params.find("address") != params.end())
            strAddress = params["address"].get<std::string>();
        else
            throw Exception(-1, "Missing address parameter");

        /* Remove miner */
        bool fSuccess = LLP::StatelessMinerManager::Get().RemoveMiner(strAddress);

        if(!fSuccess)
            throw Exception(-2, "Miner not found");

        /* Build response */
        encoding::json result;
        result["success"] = true;
        result["address"] = strAddress;

        return result;
    }


    /* Generate a new Falcon key pair */
    encoding::json System::GenerateFalconKey(const encoding::json& params, const bool fHelp)
    {
        /* Check for help flag. */
        if(fHelp)
            return std::string(
                "generatefalconkey [label] - Generate a new Falcon key pair for miner authentication.");

        /* Get Falcon auth instance */
        LLP::FalconAuth::IFalconAuth* pAuth = LLP::FalconAuth::Get();
        if(!pAuth)
            throw Exception(-1, "Falcon auth not initialized");

        /* Get optional label parameter */
        std::string strLabel;
        if(params.is_array() && params.size() > 0)
            strLabel = params[0].get<std::string>();
        else if(params.find("label") != params.end())
            strLabel = params["label"].get<std::string>();

        /* Generate key */
        LLP::FalconAuth::KeyMetadata meta = 
            pAuth->GenerateKey(LLP::FalconAuth::Profile::FALCON_512, strLabel);

        /* Build JSON response */
        encoding::json result;
        result["key_id"] = meta.keyId.ToString();
        result["pubkey"] = HexStr(meta.pubkey);
        result["profile"] = (meta.profile == LLP::FalconAuth::Profile::FALCON_512) ? 
            "FALCON_512" : "FALCON_1024";
        result["created"] = meta.created;
        result["last_used"] = meta.lastUsed;
        result["label"] = meta.label;

        return result;
    }


    /* List all stored Falcon keys */
    encoding::json System::ListFalconKeys(const encoding::json& params, const bool fHelp)
    {
        /* Check for help flag. */
        if(fHelp || params.size() != 0)
            return std::string(
                "listfalconkeys - Returns an array of all stored Falcon keys.");

        /* Get Falcon auth instance */
        LLP::FalconAuth::IFalconAuth* pAuth = LLP::FalconAuth::Get();
        if(!pAuth)
            throw Exception(-1, "Falcon auth not initialized");

        /* Get all keys */
        std::vector<LLP::FalconAuth::KeyMetadata> vKeys = pAuth->ListKeys();

        /* Build JSON array */
        encoding::json result = encoding::json::array();

        for(const auto& meta : vKeys)
        {
            encoding::json key;
            key["key_id"] = meta.keyId.ToString();
            key["pubkey"] = HexStr(meta.pubkey);
            key["profile"] = (meta.profile == LLP::FalconAuth::Profile::FALCON_512) ? 
                "FALCON_512" : "FALCON_1024";
            key["created"] = meta.created;
            key["last_used"] = meta.lastUsed;
            key["label"] = meta.label;

            result.push_back(key);
        }

        return result;
    }


    /* Test Falcon authentication for a key */
    encoding::json System::TestFalconAuth(const encoding::json& params, const bool fHelp)
    {
        /* Check for help flag. */
        if(fHelp || params.size() != 1)
            return std::string(
                "testfalconauth <key_id> - Test Falcon authentication by signing and verifying a test message.");

        /* Get Falcon auth instance */
        LLP::FalconAuth::IFalconAuth* pAuth = LLP::FalconAuth::Get();
        if(!pAuth)
            throw Exception(-1, "Falcon auth not initialized");

        /* Get key_id parameter */
        std::string strKeyId;
        if(params.is_array() && params.size() > 0)
            strKeyId = params[0].get<std::string>();
        else if(params.find("key_id") != params.end())
            strKeyId = params["key_id"].get<std::string>();
        else
            throw Exception(-2, "Missing key_id parameter");

        /* Parse key ID */
        uint256_t keyId;
        keyId.SetHex(strKeyId);

        /* Get key metadata */
        std::optional<LLP::FalconAuth::KeyMetadata> meta = pAuth->GetKey(keyId);
        if(!meta.has_value())
            throw Exception(-3, "Key not found");

        /* Build test message */
        std::vector<uint8_t> vMessage;
        vMessage.insert(vMessage.end(), strKeyId.begin(), strKeyId.end());
        vMessage.push_back(0x42); // Test marker

        /* Sign message */
        std::vector<uint8_t> vSignature = pAuth->Sign(keyId, vMessage);
        if(vSignature.empty())
            throw Exception(-4, "Failed to sign test message");

        /* Verify signature */
        LLP::FalconAuth::VerifyResult result = pAuth->Verify(meta->pubkey, vMessage, vSignature);

        /* Build JSON response */
        encoding::json jsonRet;
        jsonRet["success"] = result.fValid;
        jsonRet["key_id"] = keyId.ToString();

        if(!result.fValid)
            jsonRet["error"] = result.strError;
        else
            jsonRet["error"] = "";

        return jsonRet;
    }


    /* Bind a Falcon key to a Tritium genesis */
    encoding::json System::BindFalconKey(const encoding::json& params, const bool fHelp)
    {
        /* Check for help flag. */
        if(fHelp || params.size() != 2)
            return std::string(
                "bindfalconkey <key_id> <genesis> - Bind a Falcon key to a Tritium genesis hash.");

        /* Get Falcon auth instance */
        LLP::FalconAuth::IFalconAuth* pAuth = LLP::FalconAuth::Get();
        if(!pAuth)
            throw Exception(-1, "Falcon auth not initialized");

        /* Get parameters */
        std::string strKeyId;
        std::string strGenesis;

        if(params.is_array() && params.size() >= 2)
        {
            strKeyId = params[0].get<std::string>();
            strGenesis = params[1].get<std::string>();
        }
        else if(params.find("key_id") != params.end() && params.find("genesis") != params.end())
        {
            strKeyId = params["key_id"].get<std::string>();
            strGenesis = params["genesis"].get<std::string>();
        }
        else
            throw Exception(-2, "Missing key_id or genesis parameter");

        /* Parse key ID and genesis */
        uint256_t keyId;
        keyId.SetHex(strKeyId);

        uint256_t hashGenesis;
        hashGenesis.SetHex(strGenesis);

        /* Bind genesis */
        bool fSuccess = pAuth->BindGenesis(keyId, hashGenesis);

        if(!fSuccess)
            throw Exception(-3, "Failed to bind genesis (key may not exist)");

        /* Build response */
        encoding::json result;
        result["success"] = true;
        result["key_id"] = keyId.ToString();
        result["genesis"] = hashGenesis.ToString();

        return result;
    }

} // namespace TAO::API
