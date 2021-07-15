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
#include <TAO/API/types/sessionmanager.h>

#include <TAO/API/types/commands/ledger.h>

#include <TAO/Ledger/include/process.h>


#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Synchronizes the block header data from a peer. NOTE: the method call will return as soon as the synchronization
       process is initiated with a peer, NOT when synchronization is complete.  Only applicable in lite / client mode. */
    encoding::json Ledger::SyncHeaders(const encoding::json& params, const bool fHelp)
    {
        /* JSON return value. */
        encoding::json ret;

        /* Sync only applicable in client mode */
        if(!config::fClient.load())
            throw Exception(-308, "API method only available in client mode");

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
            throw Exception(-306, "No connections available");
        }

        return ret;
    }
}
