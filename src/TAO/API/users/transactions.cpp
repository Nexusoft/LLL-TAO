/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/users/types/users.h>

#include <TAO/API/include/extract.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/json.h>
#include <TAO/API/types/sessionmanager.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/hex.h>
#include <Util/include/string.h>

#include <TAO/Operation/include/enum.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Get a user's account. */
        encoding::json Users::Transactions(const encoding::json& params, const bool fHelp)
        {
            /* JSON return value. */
            encoding::json ret = encoding::json::array();

            /* Get the Genesis ID. */
            uint256_t hashGenesis = 0;

            /* Check to see if specific genesis has been supplied */
            if(params.find("genesis") != params.end() && !params["genesis"].get<std::string>().empty())
                hashGenesis.SetHex(params["genesis"].get<std::string>());

            /* Check if username has been supplied instead. */
            else if(params.find("username") != params.end() && !params["username"].get<std::string>().empty())
                hashGenesis = TAO::Ledger::SignatureChain::Genesis(params["username"].get<std::string>().c_str());

            /* Check for logged in user.  NOTE: we rely on the GetSession method to check for the existence of a valid session ID
               in the parameters in multiuser mode, or that a user is logged in for single user mode. Otherwise the GetSession
               method will throw an appropriate error. */
            else
                hashGenesis = Commands::Get<Users>()->GetSession(params).GetAccount()->Genesis();

            /* The genesis hash of the API caller, if logged in */
            uint256_t hashCaller = Commands::Get<Users>()->GetCallersGenesis(params);
            if(config::fClient.load() && hashGenesis != hashCaller)
                throw APIException(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");

            /* Get verbose levels. */
            std::string strVerbose = "default";
            if(params.find("verbose") != params.end())
                strVerbose = params["verbose"].get<std::string>();

            /* Check the verbosity levels. */
            uint32_t nVerbose = 1;
            if(strVerbose == "default")
                nVerbose = 1;
            else if(strVerbose == "summary")
                nVerbose = 2;
            else if(strVerbose == "detail")
                nVerbose = 3;

            /* Number of results to return. */
            uint32_t nLimit = 100, nOffset = 0;

            /* Get the params to apply to the response. */
            std::string strOrder = "desc";
            ExtractList(params, strOrder, nLimit, nOffset);

            /* Get the last transaction. */
            uint512_t hashLast = 0;
            if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-144, "No transactions found");

            /* Loop until genesis, storing all tx into a vector (these will be in descending order). */
            std::vector<TAO::Ledger::Transaction> vtx;
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                {
                    /* In client mode it is possible to not have the full sig chain if it is still being downloaded asynchronously.*/
                    if(config::fClient.load())
                        break;
                    else
                        throw APIException(-108, "Failed to read transaction");
                }

                /* Set the next last. */
                hashLast = !tx.IsFirst() ? tx.hashPrevTx : 0;

                vtx.push_back(tx);
            }

            /* Transactions sorted in descending order. Reverse the order if ascending requested. */
            if(strOrder == "asc")
                std::reverse(vtx.begin(), vtx.end());

            uint32_t nTotal = 0;
            for(auto tx : vtx)
            {
                /* Read the block state from the the ledger DB using the transaction hash index */
                TAO::Ledger::BlockState blockState;
                LLD::Ledger->ReadBlock(tx.GetHash(), blockState);

                /* Get the transaction JSON. */
                encoding::json obj = TAO::API::TransactionToJSON(hashCaller, tx, blockState, nVerbose, hashGenesis);

                /* Check to see whether the transaction has had all children filtered out */
                if(obj.empty())
                    continue;

                /* Check the offset. */
                if(++nTotal <= nOffset)
                    continue;

                /* Check the limit */
                if(nTotal - nOffset > nLimit)
                    break;

                ret.push_back(obj);
            }

            return ret;
        }
    }
}
