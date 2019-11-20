/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/wallet/wallet.h>
#include <Legacy/include/money.h>

#include <LLP/types/apinode.h>
#include <LLP/include/network.h>
#include <LLP/include/global.h>
#include <LLP/include/version.h>
#include <LLP/include/lisp.h>
#include <LLP/include/manager.h>
#include <LLP/include/trust_address.h>

#include <TAO/API/types/system.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/json.h>
#include <Util/include/config.h>
#include <Util/include/version.h>
#include <Util/include/signals.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Standard initialization function. */
        void System::Initialize()
        {
            mapFunctions["get/info"]         = Function(std::bind(&System::GetInfo,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["get/trustinfo"]    = Function(std::bind(&System::GetTrustInfo,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["stop"]             = Function(std::bind(&System::Stop,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/peers"]       = Function(std::bind(&System::ListPeers,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/lisp-eids"]   = Function(std::bind(&System::LispEIDs, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["validate/address"] = Function(std::bind(&System::Validate,    this, std::placeholders::_1, std::placeholders::_2));
        }


        /* stop"
        *  Stop Nexus server */
        json::json System::Stop(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() != 0)
                return std::string("stop - Stop Nexus server.");
                
            // Shutdown will take long enough that the response should get back
            Shutdown();
            return std::string("Nexus server stopping");
        }



        /* Reurns a summary of node and ledger information for the currently running node. */
        json::json System::GetInfo(const json::json& params, bool fHelp)
        {
            /* Declare return JSON object */
            json::json jsonRet;

            /* The daemon version*/
            jsonRet["version"] = version::CLIENT_VERSION_BUILD_STRING;

            /* The LLP version*/
            jsonRet["protocolversion"] = LLP::PROTOCOL_VERSION;

            /* Legacy wallet version*/
            jsonRet["walletversion"] = Legacy::Wallet::GetInstance().GetVersion();

            /* Current unified time as reported by this node*/
            jsonRet["timestamp"] =  (int)runtime::unifiedtimestamp();

            /* The hostname of this machine */
            char hostname[128];
            gethostname(hostname, sizeof(hostname));
            jsonRet["hostname"] = std::string(hostname);

            /* If this node is running on the testnet then this shows the testnet number*/
            jsonRet["testnet"] = config::GetArg("-testnet", 0);

            /* Whether this node is running in private mode */
            jsonRet["private"] = config::GetBoolArg("-private");

            /* Whether this node is running in multiuser mode */
            jsonRet["multiuser"] = config::fMultiuser.load();

            /* The current block height of this node */
            jsonRet["blocks"] = (int)TAO::Ledger::ChainState::nBestHeight.load();

            /* Flag indicating whether this node is currently syncrhonizing*/
            jsonRet["synchronizing"] = (bool)TAO::Ledger::ChainState::Synchronizing();

            /* The percentage complete when synchronizing */
            jsonRet["synccomplete"] = (int)TAO::Ledger::ChainState::PercentSynchronized();

            /* Number of transactions in the node's mempool*/
            jsonRet["txtotal"] =TAO::Ledger::mempool.Size() + TAO::Ledger::mempool.SizeLegacy();

            /* Number of peer connections*/
            uint16_t nConnections = 0;

            /* Then check connections to the tritium server */
            if(LLP::TRITIUM_SERVER)
                nConnections += LLP::TRITIUM_SERVER->GetConnectionCount();

            jsonRet["connections"] = nConnections;


            // The EID's of this node if using LISP
            std::map<std::string, LLP::EID> mapEIDs = LLP::GetEIDs();
            if(mapEIDs.size() > 0)
            {
                json::json jsonEIDs = json::json::array();
                for(const auto& eid :mapEIDs)
                {
                    jsonEIDs.push_back(eid.first);
                }
                jsonRet["eids"] = jsonEIDs;
            }

            return jsonRet;

        }


        /* Reurns information about the peers currently connected to this node */
        json::json System::ListPeers(const json::json& params, bool fHelp)
        {
            /* Declare the JSON response object*/
            json::json jsonRet = json::json::array();

            /* Iterate the connections to the tritium server */
            for(uint16_t nThread = 0; nThread < LLP::TRITIUM_SERVER->MAX_THREADS; ++nThread)
            {
                /* Get the data threads. */
                LLP::DataThread<LLP::TritiumNode>* dt = LLP::TRITIUM_SERVER->DATA_THREADS[nThread];

                /* Lock the data thread. */
                uint16_t nSize = static_cast<uint16_t>(dt->CONNECTIONS->size());

                /* Loop through connections in data thread. */
                for(uint16_t nIndex = 0; nIndex < nSize; ++nIndex)
                {
                    try
                    {
                        /* Get reference to the connection */
                        const auto connection = dt->CONNECTIONS->at(nIndex).load();

                        /* Skip over inactive connections. */
                        if(!connection)
                            continue;

                        /* Push the active connection. */
                        if(connection->Connected())
                        {
                            json::json obj;

                            /* The IPV4/V6 address */
                            obj["address"]  = connection->addr.ToString();

                            /* The version string of the connected peer */
                            obj["type"]     = connection->strFullVersion;

                            /* The protocol version being used to communicate */
                            obj["version"]  = connection->nProtocolVersion;

                            /* Session ID for the current connection */
                            obj["session"]  = connection->nCurrentSession;

                            /* Flag indicating whether this was an outgoing connection or incoming */
                            obj["outgoing"] = connection->fOUTGOING.load();

                            /* The current height of the peer */
                            obj["height"]   = connection->nCurrentHeight;

                            /* block hash of the peer's best chain */
                            obj["best"]     = connection->hashBestChain.SubString();

                            /* The calculated network latency between this node and the peer */
                            obj["latency"]  = connection->nLatency.load();

                            /* Unix timestamp of the last time this node had any communications with the peer */
                            obj["lastseen"] = connection->nLastPing.load();

                            /* See if the connection is in the address manager */
                            if(LLP::TRITIUM_SERVER->pAddressManager != nullptr
                            && LLP::TRITIUM_SERVER->pAddressManager->Has(connection->addr))
                            {
                                /* Get the trust address from the address manager */
                                const LLP::TrustAddress& trustAddress = LLP::TRITIUM_SERVER->pAddressManager->Get(connection->addr);

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
                    catch(const std::exception& e)
                    {
                        //debug::error(FUNCTION, e.what());
                    }
                }
            }

            return jsonRet;
        }

    }

}
