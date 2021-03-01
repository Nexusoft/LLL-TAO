/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/global.h>

#include <TAO/API/system/types/system.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Reurns information about the peers currently connected to this node */
        json::json System::Peers(const json::json& params, bool fHelp)
        {
            /* Declare the JSON response object*/
            json::json jsonRet = json::json::array();

            /* Get the connections from the tritium server */
            std::vector<std::shared_ptr<LLP::TritiumNode>> vConnections = LLP::TRITIUM_SERVER->GetConnections();

            /* Iterate the connections*/
            for(const auto& connection : vConnections)
            {
                /* Skip over inactive connections. */
                if(!connection.get())
                    continue;

                /* Push the active connection. */
                if(connection.get()->Connected())
                {
                    json::json obj;

                    /* The IPV4/V6 address */
                    obj["address"]  = connection.get()->addr.ToString();

                    /* The version string of the connected peer */
                    obj["type"]     = connection.get()->strFullVersion;

                    /* The protocol version being used to communicate */
                    obj["version"]  = connection.get()->nProtocolVersion;

                    /* Session ID for the current connection */
                    obj["session"]  = connection.get()->nCurrentSession;

                    /* Flag indicating whether this was an outgoing connection or incoming */
                    obj["outgoing"] = connection.get()->fOUTGOING.load();

                    /* The current height of the peer */
                    obj["height"]   = connection.get()->nCurrentHeight;

                    /* block hash of the peer's best chain */
                    obj["best"]     = connection.get()->hashBestChain.SubString();

                    /* block hash of the peer's best chain */
                    if(connection.get()->hashGenesis != 0)
                        obj["genesis"] = connection.get()->hashGenesis.SubString();

                    /* The calculated network latency between this node and the peer */
                    obj["latency"]  = connection.get()->nLatency.load();

                    /* Unix timestamp of the last time this node had any communications with the peer */
                    obj["lastseen"] = connection.get()->nLastPing.load();

                    /* See if the connection is in the address manager */
                    if(LLP::TRITIUM_SERVER->GetAddressManager() != nullptr
                    && LLP::TRITIUM_SERVER->GetAddressManager()->Has(connection.get()->addr))
                    {
                        /* Get the trust address from the address manager */
                        const LLP::TrustAddress& trustAddress = LLP::TRITIUM_SERVER->GetAddressManager()->Get(connection.get()->addr);

                        /* The number of connections successfully established with this peer since this node started */
                        obj["connects"] = trustAddress.nConnected;

                        /* The number of connections dropped with this peer since this node started */
                        obj["drops"]    = trustAddress.nDropped;

                        /* The number of failed connection attempts to this peer since this node started */
                        obj["fails"]    = trustAddress.nFailed;

                        /* The score value assigned to this peer based on latency and other connection statistics.   */
                        obj["score"]    = trustAddress.Score();
                    }

                    jsonRet.push_back(obj);
                }
            }

        return jsonRet;
        
        }  

    }

}