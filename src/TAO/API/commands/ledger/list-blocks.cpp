/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/eckey.h>

#include <LLD/include/global.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/json.h>

#include <TAO/API/types/commands/ledger.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/state.h>

#include <Util/include/hex.h>
#include <Util/include/string.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Retrieves the block data for a sequential range of blocks starting at a given hash or height. */
    encoding::json Ledger::ListBlocks(const encoding::json& jParams, const bool fHelp)
    {
        /* Number of results to return. */
        uint32_t nLimit = 10, nOffset = 0;

        /* Sort order to apply */
        std::string strOrder = "desc", strColumn = "height";

        /* Get the jParams to apply to the response. */
        ExtractList(jParams, strOrder, nLimit, nOffset);

        /* Declare the BlockState to load from the DB */
        TAO::Ledger::BlockState tBlock;

        /* Check for height parameter */
        if(CheckParameter(jParams, "height", "string, number"))
        {
            /* Check that the node is configured to index blocks by height */
            if(!config::GetBoolArg("-indexheight"))
                throw Exception(-85, "getblock by height requires the daemon to be started with the -indexheight flag.");

            /* Convert the incoming height string to an int*/
            const uint32_t nHeight = ExtractInteger<uint32_t>(jParams, "height");

            /* Check that the requested height is within our chain range*/
            if(nHeight > TAO::Ledger::ChainState::nBestHeight.load())
                throw Exception(-82, "Block number out of range.");

            /* Read the block state from the the ledger DB using the height index */
            if(!LLD::Ledger->ReadBlock(nHeight, tBlock))
                throw Exception(-83, "Block not found");
        }

        /* Check for block-hash parameter. */
        else if(CheckParameter(jParams, "hash", "string"))
        {
            uint1024_t blockHash = uint1024_t(jParams["hash"].get<std::string>());

            /* Read the block state from the the ledger DB using the hash index */
            if(!LLD::Ledger->ReadBlock(blockHash, tBlock))
                throw Exception(-83, "Block not found");
        }
        else
            throw Exception(-56, "Missing Parameter [hash or height]");

        /* Default to verbosity 1 which includes only the hash */
        const uint32_t nVerbose = ExtractVerbose(jParams);

        /* Declare the JSON array to return */
        encoding::json ret = encoding::json::array();

        /* Iterate through blocks until we hit the limit or no more blocks*/
        uint32_t nTotal = 0;
        while(!tBlock.IsNull())
        {
            /* Convert the block to JSON data */
            encoding::json jsonBlock = TAO::API::BlockToJSON(tBlock, nVerbose);

            tBlock = tBlock.Next();

            /* Check to see whether the block has had all children filtered out */
            if(jsonBlock.empty())
                continue;

            /* Check the offset. */
            if(++nTotal <= nOffset)
                continue;

            /* Check the limit */
            if(nTotal - nOffset > nLimit)
                break;

            /* Add it to the return JSON array */
            ret.push_back(jsonBlock);


        }
        
        return ret;
    }
}
