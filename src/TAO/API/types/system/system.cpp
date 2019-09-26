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


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Standard initialization function. */
        void System::Initialize()
        {
            mapFunctions["get/info"] = Function(std::bind(&System::GetInfo,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/peers"] = Function(std::bind(&System::ListPeers,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/lisp-eids"] = Function(std::bind(&System::LispEIDs, this, std::placeholders::_1, std::placeholders::_2));
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
            if(LLP::TRITIUM_SERVER  && LLP::TRITIUM_SERVER->pAddressManager)
                jsonRet["connections"] = LLP::TRITIUM_SERVER->pAddressManager->Count(LLP::ConnectState::CONNECTED);


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
                        /* Skip over inactive connections. */
                        if(!dt->CONNECTIONS->at(nIndex))
                            continue;

                        /* Push the active connection. */
                        if(dt->CONNECTIONS->at(nIndex)->Connected())
                        {
                            json::json obj;

                            /* The IPV4/V6 address */
                            obj["address"]  = dt->CONNECTIONS->at(nIndex)->addr.ToString();

                            /* The version string of the connected peer */
                            obj["type"]     = dt->CONNECTIONS->at(nIndex)->strFullVersion;

                            /* The protocol version being used to communicate */
                            obj["version"]  = dt->CONNECTIONS->at(nIndex)->nProtocolVersion;

                            /* Session ID for the current connection */
                            obj["session"]  = dt->CONNECTIONS->at(nIndex)->nCurrentSession;

                            /* Flag indicating whether this was an outgoing connection or incoming */
                            obj["outgoing"] = dt->CONNECTIONS->at(nIndex)->fOUTGOING.load();

                            /* The current height of the peer */
                            obj["height"]   = dt->CONNECTIONS->at(nIndex)->nCurrentHeight;

                            /* block hash of the peer's best chain */
                            obj["best"]     = dt->CONNECTIONS->at(nIndex)->hashBestChain.SubString();

                            /* The calculated network latency between this node and the peer */
                            obj["latency"]  = dt->CONNECTIONS->at(nIndex)->nLatency.load();

                            /* Unix timestamp of the last time this node had any communications with the peer */
                            obj["lastseen"] = dt->CONNECTIONS->at(nIndex)->nLastPing.load();

                            /* See if the connection is in the address manager */
                            if(LLP::TRITIUM_SERVER->pAddressManager->Has(dt->CONNECTIONS->at(nIndex)->addr))
                            {
                                /* Get the trust address from the address manager */
                                const LLP::TrustAddress& trustAddress = LLP::TRITIUM_SERVER->pAddressManager->Get(dt->CONNECTIONS->at(nIndex)->addr);
                                
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
