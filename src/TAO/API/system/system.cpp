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

#include <TAO/API/include/system.h>
#include <TAO/Ledger/include/chainstate.h>

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

            /* The current block height of this node */
            jsonRet["blocks"] = (int)TAO::Ledger::ChainState::nBestHeight.load();

            /* Only include the sync information when not running in private mode as there is nothing to sync in private */
            if(!config::GetBoolArg("-private"))
            {
                /* Flag indicating whether this node is currently syncrhonizing*/
                jsonRet["synchronizing"] = (bool)TAO::Ledger::ChainState::Synchronizing();

                /* The percentage complete when synchronizing */
                jsonRet["synccomplete"] = (int)TAO::Ledger::ChainState::PercentSynchronized();
            }
            /* Number of transactions in the node's mempool*/
            jsonRet["txtotal"] =(int)Legacy::Wallet::GetInstance().mapWallet.size();

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
                    jsonEIDs.push_back( eid.first);
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

            /* Vector of tritium peers*/
            std::vector<LLP::TrustAddress> vTritiumInfo;

            /* Query address information from Tritium server address manager */
            if(LLP::TRITIUM_SERVER && LLP::TRITIUM_SERVER->pAddressManager)
                 LLP::TRITIUM_SERVER->pAddressManager->GetAddresses(vTritiumInfo, LLP::ConnectState::CONNECTED);

            /* Sort the peers by score (highest first)*/
            std::sort(vTritiumInfo.begin(), vTritiumInfo.end());

            /* add peer info to the response array for each connected peer*/
            for(auto& addr : vTritiumInfo)
            {
                json::json obj;

                /* The IP address of the peer.  This could be an EID*/
                obj["address"]  = addr.ToString();

                /* Whether the IP address is IPv4 or IPv6 */
                obj["version"]  = addr.IsIPv4() ? std::string("IPv4") : std::string("IPv6");

                /* The last known block height of the peer.  This may not be accurate as peers only broadcast their current height periodically */
                obj["height"]   = addr.nHeight;

                /* The calculated network latency between this node and the peer */
                obj["latency"]  = debug::safe_printstr(addr.nLatency, " ms");

                /* Unix timestamp of the last time this node had any communications with the peer */
                obj["lastseen"] = addr.nLastSeen;

                /* The number of connections successfully established with this peer since this node started */
                obj["connects"] = addr.nConnected;

                /* The number of connections dropped with this peer since this node started */
                obj["drops"]    = addr.nDropped;

                /* The number of failed connection attempts to this peer since this node started */
                obj["fails"]    = addr.nFailed;
                
                /* The score value assigned to this peer based on latency and other connection statistics.   */
                obj["score"]    = addr.Score();

                jsonRet.push_back(obj);
            }

            return jsonRet;
        }

    }

}
