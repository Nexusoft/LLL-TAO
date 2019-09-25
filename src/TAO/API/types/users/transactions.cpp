/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/users.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get a user's account. */
        json::json Users::Transactions(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Get the Genesis ID. */
            uint256_t hashGenesis = 0;

            /* Watch for destination genesis. */
            if(params.find("genesis") != params.end())
                hashGenesis.SetHex(params["genesis"].get<std::string>());
            else if(params.find("username") != params.end())
                hashGenesis = TAO::Ledger::SignatureChain::Genesis(params["username"].get<std::string>().c_str());
            else if(!config::fMultiuser.load() && mapSessions.count(0))
            {
                /* If no specific genesis or username have been provided then fall back to the active sig chain */
                hashGenesis = mapSessions[0]->Genesis();
            }
            else
                throw APIException(-111, "Missing genesis / username");

            /* The genesis hash of the API caller, if logged in */
            uint256_t hashCaller = users->GetCallersGenesis(params);

            /* Check for paged parameter. */
            uint32_t nPage = 0;
            if(params.find("page") != params.end())
                nPage = std::stoul(params["page"].get<std::string>());

            /* Check for username parameter. */
            uint32_t nLimit = 100;
            if(params.find("limit") != params.end())
                nLimit = std::stoul(params["limit"].get<std::string>());

            /* Get verbose levels. */
            std::string strVerbose = "default";
            if(params.find("verbose") != params.end())
                strVerbose = params["verbose"].get<std::string>();

            uint32_t nVerbose = 1;
            if(strVerbose == "default")
                nVerbose = 1;
            else if(strVerbose == "summary")
                nVerbose = 2;
            else if(strVerbose == "detail")
                nVerbose = 3;

            /* Get the last transaction. */
            uint512_t hashLast = 0;
            if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-144, "No transactions found");

            /* Loop until genesis. */
            uint32_t nTotal = 0;
            while(hashLast != 0)
            {
                /* Get the current page. */
                uint32_t nCurrentPage = nTotal / nLimit;

                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-108, "Failed to read transaction");

                /* Set the next last. */
                hashLast = tx.hashPrevTx;
                ++nTotal;

                /* Check the paged data. */
                if(nCurrentPage < nPage)
                    continue;

                if(nCurrentPage > nPage)
                    break;

                if(nTotal - (nPage * nLimit) > nLimit)
                    break;

                /* Read the block state from the the ledger DB using the transaction hash index */
                TAO::Ledger::BlockState blockState;
                LLD::Ledger->ReadBlock(tx.GetHash(), blockState);

                /* Get the transaction JSON. */
                json::json obj = TAO::API::TransactionToJSON(hashCaller, tx, blockState, nVerbose, hashGenesis);

                ret.push_back(obj);
            }

            return ret;
        }
    }
}
