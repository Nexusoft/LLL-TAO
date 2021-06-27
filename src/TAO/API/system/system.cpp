/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/include/money.h>
#include <Legacy/wallet/wallet.h>

#include <LLP/types/apinode.h>
#include <LLP/include/network.h>
#include <LLP/include/global.h>
#include <LLP/include/version.h>
#include <LLP/include/lisp.h>
#include <LLP/include/manager.h>
#include <LLP/include/trust_address.h>

#include <TAO/API/system/types/system.h>
#include <TAO/API/types/sessionmanager.h>
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
            mapFunctions["get/metrics"]      = Function(std::bind(&System::Metrics,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["stop"]             = Function(std::bind(&System::Stop,       this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/peers"]       = Function(std::bind(&System::ListPeers,  this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/lisp-eids"]   = Function(std::bind(&System::LispEIDs,   this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["validate/address"] = Function(std::bind(&System::Validate,   this, std::placeholders::_1, std::placeholders::_2));
        }


        /* stop"
        *  Stop Nexus server */
        encoding::json System::Stop(const encoding::json& params, const bool fHelp)
        {
            if(fHelp || params.size() > 1)
                return std::string("stop password=<password> - Stop server, if -system/stop set require password");

            /* Check for password argument. */
            const std::string strPassword = config::GetArg("-system/stop", "");
            if(!strPassword.empty())
            {
                /* Check that we have a password. */
                if(params.find("password") == params.end())
                    throw Exception(-128, "Missing password");

                /* Check our shutdown credentials. */
                if(params["password"] != strPassword)
                    throw Exception(-139, "Invalid credentials");
            }

            // Shutdown will take long enough that the response should get back
            Shutdown();
            return std::string("Nexus server stopping");
        }



        /* Reurns a summary of node and ledger information for the currently running node. */
        encoding::json System::GetInfo(const encoding::json& params, const bool fHelp)
        {
            /* Declare return JSON object */
            encoding::json jsonRet;

            /* The daemon version*/
            jsonRet["version"] = version::CLIENT_VERSION_BUILD_STRING;

            /* The LLP version*/
            jsonRet["protocolversion"] = LLP::PROTOCOL_VERSION;

            /* Legacy wallet version*/
            #ifndef NO_WALLET
            jsonRet["walletversion"] = Legacy::Wallet::GetInstance().GetVersion();
            #endif

            /* Current unified time as reported by this node*/
            jsonRet["timestamp"] =  (int)runtime::unifiedtimestamp();

            /* The hostname of this machine */
            jsonRet["hostname"]  = LLP::strHostname;

            /* The IP address, if known */
            if(LLP::TritiumNode::addrThis.load().IsValid())
                jsonRet["ipaddress"] = LLP::TritiumNode::addrThis.load().ToStringIP();

            /* If this node is running on the testnet then this shows the testnet number*/
            if(config::fTestNet.load())
                jsonRet["testnet"] = config::GetArg("-testnet", 0); //we don't need to show this value if in production mode

            /* Whether this node is running in hybrid or private mode */
            jsonRet["private"] = (config::fHybrid.load() && config::fTestNet.load());
            jsonRet["hybrid"]  = (config::fHybrid.load() && !config::fTestNet.load());

            /* Whether this node is running in multiuser mode */
            jsonRet["multiuser"] = config::fMultiuser.load();

            /* Number of logged in sessions */
            if(config::fMultiuser.load())
                jsonRet["sessions"] = TAO::API::GetSessionManager().Size();

            /* Whether this node is running in client mode */
            jsonRet["clientmode"] = config::fClient.load();

            /* Whether this node is running the legacy wallet */
            #ifdef NO_WALLET
            jsonRet["legacy_unsupported"] = true;
            #endif

            /* The current block height of this node */
            jsonRet["blocks"] = (int)TAO::Ledger::ChainState::nBestHeight.load();

            /* Flag indicating whether this node is currently syncrhonizing */
            jsonRet["synchronizing"] = (bool)TAO::Ledger::ChainState::Synchronizing();

            /* The percentage of the blocks downloaded */
            jsonRet["synccomplete"] = (int)TAO::Ledger::ChainState::PercentSynchronized();

            /* The percentage of the current sync completed */
            jsonRet["syncprogress"] = (int)TAO::Ledger::ChainState::SyncProgress();

            /* Number of transactions in the node's mempool*/
            jsonRet["txtotal"] = TAO::Ledger::mempool.Size();

            /* Then check connections to the tritium server */
            if(LLP::TRITIUM_SERVER)
                jsonRet["connections"] = LLP::TRITIUM_SERVER->GetConnectionCount();

            return jsonRet;
        }


        /* Returns information about the peers currently connected to this node */
        encoding::json System::ListPeers(const encoding::json& params, const bool fHelp)
        {
            /* Declare the JSON response object*/
            encoding::json jsonRet = encoding::json::array();

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
                    encoding::json obj;

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
