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

#include <Util/include/json.h>
#include <Util/include/debug.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Returns a list of available mining pools discovered on the network. */
    encoding::json Mining::ListPools(const encoding::json& jParams, const bool fHelp)
    {
        /* Check for help flag. */
        if(fHelp)
            return std::string(
                "list/pools"
                "\n\nReturns a list of available mining pools discovered on the network.\n"
                "\n\nArguments:\n"
                "{\"maxfee\":5,\"minreputation\":0,\"onlineonly\":true}\n"
                "\n\nmaxfee (optional, default=5): Maximum pool fee percentage (0-5)\n"
                "minreputation (optional, default=0): Minimum reputation score (0-100)\n"
                "onlineonly (optional, default=true): Only return pools that are currently reachable\n"
                "\n\nReturns:\n"
                "[\n"
                "  {\n"
                "    \"genesis\": \"8abc123def...\",\n"
                "    \"endpoint\": \"123.45.67.89:9549\",\n"
                "    \"fee\": 2,\n"
                "    \"currentminers\": 150,\n"
                "    \"maxminers\": 500,\n"
                "    \"hashrate\": 1000000000,\n"
                "    \"blocksfound\": 1234,\n"
                "    \"reputation\": 98,\n"
                "    \"uptime\": 0.995,\n"
                "    \"name\": \"Example Mining Pool\",\n"
                "    \"lastseen\": 1734529175,\n"
                "    \"online\": true\n"
                "  }\n"
                "]\n"
            );

        /* Extract parameters with defaults */
        uint8_t nMaxFee = 5;
        if(jParams.find("maxfee") != jParams.end())
            nMaxFee = static_cast<uint8_t>(ExtractInteger(jParams, "maxfee"));

        uint32_t nMinReputation = 0;
        if(jParams.find("minreputation") != jParams.end())
            nMinReputation = static_cast<uint32_t>(ExtractInteger(jParams, "minreputation"));

        bool fOnlineOnly = true;
        if(jParams.find("onlineonly") != jParams.end())
            fOnlineOnly = ExtractBoolean(jParams, "onlineonly");

        /* Get pools from discovery */
        std::vector<LLP::MiningPoolInfo> vPools = LLP::PoolDiscovery::ListMiningPools(
            nMaxFee,
            nMinReputation,
            fOnlineOnly
        );

        /* Build JSON array */
        encoding::json jsonRet = encoding::json::array();

        for(const auto& pool : vPools)
        {
            encoding::json jsonPool;
            jsonPool["genesis"] = pool.hashGenesis.ToString();
            jsonPool["endpoint"] = pool.strEndpoint;
            jsonPool["fee"] = static_cast<int>(pool.nFeePercent);
            jsonPool["currentminers"] = static_cast<int>(pool.nCurrentMiners);
            jsonPool["maxminers"] = static_cast<int>(pool.nMaxMiners);
            jsonPool["hashrate"] = static_cast<uint64_t>(pool.nTotalHashrate);
            jsonPool["blocksfound"] = static_cast<int>(pool.nBlocksFound);
            jsonPool["reputation"] = static_cast<int>(pool.nReputation);
            jsonPool["uptime"] = pool.fUptime;
            jsonPool["name"] = pool.strPoolName;
            jsonPool["lastseen"] = static_cast<uint64_t>(pool.nLastSeen);
            jsonPool["online"] = pool.fOnline;

            jsonRet.push_back(jsonPool);
        }

        return jsonRet;
    }
}
