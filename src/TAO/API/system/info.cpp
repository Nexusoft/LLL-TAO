/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/system/types/system.h>
#include <TAO/API/include/utils.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

         /* Reurns a summary of node and ledger information for the currently running node. */
        json::json System::Info(const json::json& params, bool fHelp)
        {
            /* Declare return JSON object */
            json::json jsonRet;

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
            char hostname[128];
            gethostname(hostname, sizeof(hostname));
            jsonRet["hostname"] = std::string(hostname);

            /* The IP address, if known */
            if(LLP::TritiumNode::thisAddress.IsValid())
                jsonRet["ipaddress"] = LLP::TritiumNode::thisAddress.ToStringIP();

            /* If this node is running on the testnet then this shows the testnet number*/
            jsonRet["testnet"] = config::GetArg("-testnet", 0);

            /* Whether this node is running in private mode */
            jsonRet["private"] = config::GetBoolArg("-private");

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

    }

}