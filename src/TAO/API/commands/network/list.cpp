/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/global.h>

#include <TAO/API/types/commands/network.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/include/compare.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Lists the current logged in sessions for -multiusername mode. */
    encoding::json Network::List(const encoding::json& jParams, const bool fHelp)
    {
        /* Get our current object type. */
        const std::string strType = ExtractType(jParams);

        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0;

        /* Get the params to apply to the response. */
        std::string strOrder = "desc", strColumn = "lastseen";
        ExtractList(jParams, strOrder, strColumn, nLimit, nOffset);

        /* Build our object list and sort on insert. */
        std::set<encoding::json, CompareResults> setAddresses({}, CompareResults(strOrder, strColumn));

        /* Check our type for nodes. */
        if(strType == "node")
        {
            /* Get addresses from manager. */
            std::vector<LLP::TrustAddress> vAddr;
            if(LLP::TRITIUM_SERVER->GetAddressManager())
                LLP::TRITIUM_SERVER->GetAddressManager()->GetAddresses(vAddr, CONNECT_FLAGS_ALL);

            /* Build our return JSON data. */
            for(const auto& rAddr : vAddr)
            {
                /* Check for invalid addresses. */
                if(rAddr.nLastSeen == 0)
                    continue;

                /* Build our JSON object. */
                encoding::json jAddr =
                {
                    { "address",  rAddr.ToStringIP() },
                    { "lastseen", rAddr.nLastSeen    },
                    { "latency",  rAddr.nLatency     }
                };

                /* Check that we match our filters. */
                if(!FilterResults(jParams, jAddr))
                    continue;

                /* Filter out our expected fieldnames if specified. */
                if(!FilterFieldname(jParams, jAddr))
                    continue;

                setAddresses.insert(jAddr);
            }
        }

        /* Check our type name for peers. */
        if(strType == "peer")
        {
            /* Get the rConnections from the tritium server */
            std::vector<std::shared_ptr<LLP::TritiumNode>> vConnections =
                LLP::TRITIUM_SERVER->GetConnections();

            /* Build our jConnectionect list and sort on insert. */
            std::set<encoding::json, CompareResults> setConnections({}, CompareResults(strOrder, strColumn));

            /* Iterate the rConnections*/
            for(const auto& rConnection : vConnections)
            {
                /* Skip over inactive rConnections. */
                if(!rConnection.get())
                    continue;

                /* Push the active rConnection. */
                if(rConnection.get()->Connected())
                {
                    encoding::json jConnection;

                    /* The IPV4/V6 address */
                    jConnection["address"]  = rConnection.get()->addr.ToString();

                    /* The version string of the connected peer */
                    jConnection["type"]     = rConnection.get()->strFullVersion;

                    /* The protocol version being used to communicate */
                    jConnection["version"]  = rConnection.get()->nProtocolVersion;

                    /* Session ID for the current rConnection */
                    jConnection["session"]  = rConnection.get()->nCurrentSession;

                    /* Flag indicating whether this was an outgoing rConnection or incoming */
                    jConnection["outgoing"] = rConnection.get()->fOUTGOING.load();

                    /* The current height of the peer */
                    jConnection["height"]   = rConnection.get()->nCurrentHeight;

                    /* block hash of the peer's best chain */
                    jConnection["best"]     = rConnection.get()->hashBestChain.SubString();

                    /* block hash of the peer's best chain */
                    if(rConnection.get()->hashGenesis != 0)
                        jConnection["genesis"] = rConnection.get()->hashGenesis.SubString();

                    /* The calculated network latency between this node and the peer */
                    jConnection["latency"]  = rConnection.get()->nLatency.load();

                    /* Unix timestamp of the last time this node had any communications with the peer */
                    jConnection["lastseen"] = rConnection.get()->nLastPing.load();

                    /* Check that we match our filters. */
                    if(!FilterResults(jParams, jConnection))
                        continue;

                    /* Filter out our expected fieldnames if specified. */
                    if(!FilterFieldname(jParams, jConnection))
                        continue;

                    /* Insert into set and automatically sort. */
                    setAddresses.insert(jConnection);
                }
            }
        }

        /* Build our return value. */
        encoding::json jRet = encoding::json::array();

        /* Handle paging and offsets. */
        uint32_t nTotal = 0;
        for(const auto& jAddress : setAddresses)
        {
            /* Check the offset. */
            if(++nTotal <= nOffset)
                continue;

            /* Check the limit */
            if(jRet.size() == nLimit)
                break;

            jRet.push_back(jAddress);
        }

        return jRet;
    }
}
