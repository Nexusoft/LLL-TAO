/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/eckey.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/state.h>

#include <Util/include/hex.h>
#include <Util/include/string.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Creates a block . */
        json::json Ledger::Create(const json::json& params, bool fHelp)
        {
            throw APIException(-1, "Method not available.");
        }

        /* Retrieves the blockhash for the given height. */
        json::json Ledger::BlockHash(const json::json& params, bool fHelp)
        {
            /* Check that the node is configured to index blocks by height */
            if(!config::GetBoolArg("-indexheight"))
            {
                throw APIException(-79, "getblockhash requires the daemon to be started with the -indexheight flag.");
            }

            /* Check for the block height parameter. */
            if(params.find("height") == params.end())
                throw APIException(-80, "Missing height");

            /* Check that the height parameter is numeric*/
            std::string strHeight = params["height"].get<std::string>();

            if(!IsAllDigit(strHeight))
                throw APIException(-81, "Invalid height parameter");

            /* Convert the incoming height string to an int*/
            uint32_t nHeight = std::stoul(strHeight);

            /* Check that the requested height is within our chain range*/
            if(nHeight > TAO::Ledger::ChainState::nBestHeight.load())
                throw APIException(-82, "Block number out of range.");

            TAO::Ledger::BlockState blockState;
            /* Read the block state from the the ledger DB using the height index */
            if(!LLD::Ledger->ReadBlock(nHeight, blockState))
                throw APIException(-83, "Block not found");

            json::json ret;
            ret["hash"] = blockState.GetHash().GetHex();

            return ret;
        }


        /* Retrieves the block data for a given hash or height. */
        json::json Ledger::Block(const json::json& params, bool fHelp)
        {
            /* Check for the block height parameter. */
            if(params.find("hash") == params.end() && params.find("height") == params.end())
                throw APIException(-84, "Missing hash or height");

            /* Declare the BlockState to load from the DB */
            TAO::Ledger::BlockState blockState;

            /* look up by height*/
            if(params.find("height") != params.end())
            {
                /* Check that the node is configured to index blocks by height */
                if(!config::GetBoolArg("-indexheight"))
                    throw APIException(-85, "getblock by height requires the daemon to be started with the -indexheight flag.");

                /* Check that the height parameter is numeric*/
                std::string strHeight = params["height"].get<std::string>();

                if(!IsAllDigit(strHeight))
                    throw APIException(-81, "Invalid height parameter");

                /* Convert the incoming height string to an int*/
                uint32_t nHeight = std::stoul(strHeight);

                /* Check that the requested height is within our chain range*/
                if(nHeight > TAO::Ledger::ChainState::nBestHeight.load())
                    throw APIException(-82, "Block number out of range.");

                /* Read the block state from the the ledger DB using the height index */
                if(!LLD::Ledger->ReadBlock(nHeight, blockState))
                    throw APIException(-83, "Block not found");
            }
            else if(params.find("hash") != params.end())
            {
                uint1024_t blockHash;
                /* Load the hash from the params*/
                blockHash.SetHex(params["hash"].get<std::string>());

                /* Read the block state from the the ledger DB using the hash index */
                if(!LLD::Ledger->ReadBlock(blockHash, blockState))
                    throw APIException(-83, "Block not found");
            }

            std::string strVerbose = "default";
            if(params.find("verbose") != params.end())
                strVerbose = params["verbose"].get<std::string>();

            /* Default to verbosity 1 which includes only the hash */
            uint32_t nVerbose = 1;
            if(strVerbose == "none")
                nVerbose = 0;
            else if(strVerbose == "default")
                nVerbose = 1;
            else if(strVerbose == "summary")
                nVerbose = 2;
            else if(strVerbose == "detail")
                nVerbose = 3;

            json::json ret = TAO::API::BlockToJSON(blockState, nVerbose);

            return ret;
        }


        /* Retrieves the block data for a sequential range of blocks starting at a given hash or height. */
        json::json Ledger::Blocks(const json::json& params, bool fHelp)
        {
            /* Check for the block height parameter. */
            if(params.find("hash") == params.end() && params.find("height") == params.end())
                throw APIException(-84, "Missing hash or height");

            /* Declare the BlockState to load from the DB */
            TAO::Ledger::BlockState blockState;

            /* Check for page parameter. */
            uint32_t nPage = 0;
            if(params.find("page") != params.end())
                nPage = std::stoul(params["page"].get<std::string>());

            /* Get the limit parameter, default to 10. */
            uint32_t nLimit = 10;
            if(params.find("limit") != params.end())
                nLimit = std::stoul(params["limit"].get<std::string>());

            /* look up by height*/
            if(params.find("height") != params.end())
            {
                /* Check that the node is configured to index blocks by height */
                if(!config::GetBoolArg("-indexheight"))
                    throw APIException(-85, "getblock by height requires the daemon to be started with the -indexheight flag.");

                /* Check that the height parameter is numeric*/
                std::string strHeight = params["height"].get<std::string>();

                if(!IsAllDigit(strHeight))
                    throw APIException(-81, "Invalid height parameter");

                /* Convert the incoming height string to an int*/
                uint32_t nHeight = std::stoul(strHeight);

                /* Check that the requested height is within our chain range*/
                if(nHeight > TAO::Ledger::ChainState::nBestHeight.load())
                    throw APIException(-82, "Block number out of range.");

                /* Read the block state from the the ledger DB using the height index */
                if(!LLD::Ledger->ReadBlock(nHeight, blockState))
                    throw APIException(-83, "Block not found");
            }
            else if(params.find("hash") != params.end())
            {
                uint1024_t blockHash;
                /* Load the hash from the params*/
                blockHash.SetHex(params["hash"].get<std::string>());

                /* Read the block state from the the ledger DB using the hash index */
                if(!LLD::Ledger->ReadBlock(blockHash, blockState))
                    throw APIException(-83, "Block not found");
            }

            /* Get the transaction verbosity level from the request*/
            std::string strVerbose = "default";
            if(params.find("verbose") != params.end())
                strVerbose = params["verbose"].get<std::string>();

            /* Default to verbosity 1 which includes only the hash */
            uint32_t nVerbose = 1;
            if(strVerbose == "none")
                nVerbose = 0;
            else if(strVerbose == "default")
                nVerbose = 1;
            else if(strVerbose == "summary")
                nVerbose = 2;
            else if(strVerbose == "detail")
                nVerbose = 3;

            /* Declare the JSON array to return */
            json::json ret = json::json::array();

            /* Iterate through blocks until we hit the limit or no more blocks*/
            uint32_t nTotal = 0;
            while(!blockState.IsNull())
            {
                /* Get the current page. */
                uint32_t nCurrentPage = nTotal / nLimit;

                ++nTotal;

                TAO::Ledger::BlockState blockToAdd = blockState;

                /* Move on to the next block in the sequence*/
                blockState = blockState.Next();

                /* Check the paged data. */
                if(nCurrentPage < nPage)
                    continue;

                if(nCurrentPage > nPage)
                    break;

                if(nTotal - (nPage * nLimit) > nLimit)
                    break;


                /* convert the block to JSON data and add it to the return JSON array*/
                ret.push_back(TAO::API::BlockToJSON(blockToAdd, nVerbose));

                ;

            }
            return ret;
        }
    }

}
