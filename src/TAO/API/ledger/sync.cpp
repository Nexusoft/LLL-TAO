/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/types/tritium.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/types/sessionmanager.h>

#include <TAO/Ledger/include/process.h>


#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Synchronizes the signature chain for the currently logged in user.  Only applicable in lite / client mode. */
        json::json Ledger::SyncSigChain(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Sync only applicable in client mode */
            if(!config::fClient.load())
                throw APIException(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");

            /* Check number of connections */
            if(LLP::TRITIUM_SERVER && LLP::TRITIUM_SERVER->GetConnectionCount() == 0)
                throw APIException(-306, "No connections available");

            /* Get the session to be used for this API call */
            Session& session = users->GetSession(params, true, false);

            /* The callers genesis */
            uint256_t hashGenesis = session.GetAccount()->Genesis();

            /* Sync the sig chain */
            if(!DownloadSigChain(hashGenesis, true))
                throw APIException(-307, "Failed to download signature chain");

            /* Add the genesis */
            ret["genesis"] = hashGenesis.GetHex();

            /* sig chain transaction count */
            uint32_t nTransactions = 0;
            /* Read the last transaction for the sig chain */
            uint512_t hashLast = 0;
            if(LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-108, "Failed to read transaction");

                /* Number of transactions is the last sequence number + 1 (since the sequence is 0 based) */
                nTransactions = tx.nSequence + 1;
            }

            /* populate the transaction count */
            ret["transactions"] = nTransactions;

            /* Get the notifications so that we can return the notification count. */
            std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> vContracts;
            Users::GetOutstanding(hashGenesis, false, vContracts);

            /* Get any expired contracts not yet voided. */
            Users::GetExpired(hashGenesis, false, vContracts);

            /* Get any legacy transactions . */
            std::vector<std::pair<std::shared_ptr<Legacy::Transaction>, uint32_t>> vLegacyTx;
            Users::GetOutstanding(hashGenesis, false, vLegacyTx);
            
            ret["notifications"] = vContracts.size() + vLegacyTx.size();
            
            return ret;
        }


        /* Synchronizes the block header data from a peer. NOTE: the method call will return as soon as the synchronization 
           process is initiated with a peer, NOT when synchronization is complete.  Only applicable in lite / client mode. */
        json::json Ledger::SyncHeaders(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Sync only applicable in client mode */
            if(!config::fClient.load())
                throw APIException(-308, "API method only available in client mode");

            /* Reset the global synchronized flag */
            LLP::TritiumNode::fSynchronized.store(false);

            /* Get a peer connection to sync from */
            std::shared_ptr<LLP::TritiumNode> pNode = LLP::TRITIUM_SERVER->GetConnection();
            if(pNode != nullptr)
            {
                /* Initiate a new sync */
                pNode->Sync();

                /* populate the response */
                ret["success"] = true;
            }
            else
            {
                throw APIException(-306, "No connections available");
            }
            
            return ret;
        }
    }
}
