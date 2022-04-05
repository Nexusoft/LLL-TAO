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
#include <TAO/API/types/authentication.h>

#include <TAO/API/types/commands/ledger.h>

#include <TAO/Ledger/include/process.h>


#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Synchronizes the signature chain for the currently logged in user.  Only applicable in lite / client mode. */
    encoding::json Ledger::SyncSigChain(const encoding::json& params, const bool fHelp)
    {
        /* JSON return value. */
        encoding::json ret;

        /* Sync only applicable in client mode */
        if(!config::fClient.load())
            throw Exception(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");

        /* Check number of connections */
        if(LLP::TRITIUM_SERVER && LLP::TRITIUM_SERVER->GetConnectionCount() == 0)
            throw Exception(-306, "No connections available");

        /* The callers genesis */
        const uint256_t hashGenesis =
            Authentication::Caller(params);

        /* Sync the sig chain */
        if(!Users::DownloadSigChain(hashGenesis, true))
            throw Exception(-307, "Failed to download signature chain");

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
                throw Exception(-108, "Failed to read transaction");

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
}
