/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/mining.h>
#include <TAO/API/include/extract.h>

#include <LLP/include/pool_discovery.h>

#include <Util/include/config.h>
#include <Util/include/json.h>
#include <Util/include/debug.h>
#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Enable pool service and begin broadcasting announcements. */
    encoding::json Mining::StartPool(const encoding::json& jParams, const bool fHelp)
    {
        /* Check for help flag. */
        if(fHelp)
            return std::string(
                "start/pool"
                "\n\nEnable pool service and begin broadcasting announcements.\n"
                "\n\nArguments:\n"
                "{\"fee\":2,\"maxminers\":500,\"name\":\"My Pool\",\"genesis\":\"8abc123def...\"}\n"
                "\n\nfee (required): Pool fee percentage (0-5)\n"
                "maxminers (optional, default=500): Maximum concurrent miners\n"
                "name (optional): Human-readable pool name\n"
                "genesis (required): Pool operator's genesis hash\n"
                "\n\nReturns:\n"
                "{\n"
                "  \"success\": true,\n"
                "  \"message\": \"Pool service started\"\n"
                "}\n"
            );

        /* Extract required parameters */
        if(jParams.find("fee") == jParams.end())
            throw Exception(-1, "Missing fee parameter");

        uint8_t nFee = static_cast<uint8_t>(ExtractInteger<uint32_t>(jParams, "fee"));

        /* Validate fee */
        if(nFee > LLP::MAX_POOL_FEE_PERCENT)
        {
            throw Exception(-2, "Fee exceeds maximum: " + std::to_string(LLP::MAX_POOL_FEE_PERCENT) + "%");
        }

        /* Extract genesis */
        if(jParams.find("genesis") == jParams.end())
            throw Exception(-3, "Missing genesis parameter");

        std::string strGenesis = ExtractString(jParams, "genesis");
        uint256_t hashGenesis;
        hashGenesis.SetHex(strGenesis);

        /* Extract optional parameters */
        uint16_t nMaxMiners = 500;
        if(jParams.find("maxminers") != jParams.end())
            nMaxMiners = static_cast<uint16_t>(ExtractInteger<uint32_t>(jParams, "maxminers"));

        std::string strPoolName = "Nexus Mining Pool";
        if(jParams.find("name") != jParams.end())
            strPoolName = ExtractString(jParams, "name");

        /* Detect endpoint */
        std::string strEndpoint = LLP::PoolDiscovery::DetectPublicEndpoint();

        /* Broadcast pool announcement */
        bool fSuccess = LLP::PoolDiscovery::BroadcastPoolAnnouncement(
            hashGenesis,
            strEndpoint,
            nFee,
            nMaxMiners,
            0,  // nCurrentMiners (will be updated)
            0,  // nTotalHashrate (will be updated)
            0,  // nBlocksFound (will be updated)
            strPoolName
        );

        if(!fSuccess)
        {
            throw Exception(-4, "Failed to broadcast pool announcement");
        }

        /* Update configuration (runtime only, doesn't modify nexus.conf) */
        config::mapArgs["-mining.pool.enabled"] = "1";
        config::mapArgs["-mining.pool.fee"] = std::to_string(nFee);
        config::mapArgs["-mining.pool.maxminers"] = std::to_string(nMaxMiners);
        config::mapArgs["-mining.pool.name"] = strPoolName;
        config::mapArgs["-mining.pool.genesis"] = strGenesis;

        /* Build response */
        encoding::json jsonRet;
        jsonRet["success"] = true;
        jsonRet["message"] = "Pool service started";
        jsonRet["genesis"] = hashGenesis.ToString();
        jsonRet["endpoint"] = strEndpoint;
        jsonRet["fee"] = static_cast<int>(nFee);
        jsonRet["maxminers"] = static_cast<int>(nMaxMiners);
        jsonRet["name"] = strPoolName;

        debug::log(0, FUNCTION, "Pool service started: ", hashGenesis.SubString(),
                   " fee=", static_cast<int>(nFee), "%");

        return jsonRet;
    }
}
