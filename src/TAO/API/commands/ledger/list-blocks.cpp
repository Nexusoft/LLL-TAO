/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/compare.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/json.h>

#include <TAO/API/types/commands/ledger.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/state.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Retrieves the block data for a sequential range of blocks starting at a given hash or height. */
    encoding::json Ledger::ListBlocks(const encoding::json& jParams, const bool fHelp)
    {
        /* Number of results to return. */
        uint32_t nLimit = 10, nOffset = 0;

        /* Sort order to apply */
        std::string strOrder = "desc";

        /* Get the jParams to apply to the response. */
        ExtractList(jParams, strOrder, nLimit, nOffset);

        /* Get our starting hash now. */
        uint1024_t hashStart = 0;

        /* Check for height parameter */
        TAO::Ledger::BlockState tLastBlock;
        if(CheckParameter(jParams, "height", "string, number"))
        {
            /* Check that the node is configured to index blocks by height */
            if(!config::GetBoolArg("-indexheight"))
                throw Exception(-85, "getblock by height requires the daemon to be started with the -indexheight flag.");

            /* Convert the incoming height string to an int*/
            const uint32_t nHeight = ExtractInteger<uint32_t>(jParams, "height") + nOffset;

            /* Check that the requested height is within our chain range*/
            if(nHeight > TAO::Ledger::ChainState::nBestHeight.load())
                throw Exception(-82, "Block number out of range.");

            /* Read the block state from the the ledger DB using the height index */
            if(!LLD::Ledger->ReadBlock(nHeight, tLastBlock))
                throw Exception(-83, "Block not found");

            /* Get our starting hash now. */
            hashStart = tLastBlock.GetHash();
        }

        /* Check for block-hash parameter. */
        else if(CheckParameter(jParams, "hash", "string"))
        {
            /* Check for offset if using hash. */
            if(nOffset > 0)
                throw Exception(-81, "[list/blocks] cannot use parameter [offset] combined with [hash]");

            /* Get our starting block hash. */
            hashStart = uint1024_t(jParams["hash"].get<std::string>());
            if(!LLD::Ledger->ReadBlock(hashStart, tLastBlock))
                throw Exception(-83, "Block not found");
        }
        else
        {
            /* Grab latest checkpoint from best block. */
            tLastBlock = TAO::Ledger::ChainState::stateBest.load();

            /* Set starting based on second to last checkpoint block. */
            while(tLastBlock.nHeight + nLimit >= TAO::Ledger::ChainState::nBestHeight.load())
            {
                /* Iterate backwards recording our hashes. */
                tLastBlock = tLastBlock.Prev();
                hashStart  = tLastBlock.GetHash();
            }
        }

        /* Default to verbosity 1 which includes only the hash */
        const uint32_t nVerbose = ExtractVerbose(jParams);

        /* Build our object list and sort on insert. */
        std::set<encoding::json, CompareResults> setBlocks({}, CompareResults(strOrder, "height"));

        /* List our blocks via a batch read for efficiency. */
        std::vector<TAO::Ledger::BlockState> vStates;
        while(setBlocks.size() < nLimit && LLD::Ledger->BatchRead(hashStart, "block", vStates, 1000, true))
        {
            /* Loop through all available states. */
            for(auto& tBlock : vStates)
            {
                /* Update start every iteration. */
                hashStart = tBlock.GetHash();

                /* Skip if not in main chain. */
                if(!tBlock.IsInMainChain())
                    continue;

                /* Check for matching hashes. */
                if(tBlock.hashPrevBlock != tLastBlock.GetHash())
                {
                    debug::log(0, FUNCTION, "Reading block ", tLastBlock.hashNextBlock.SubString());

                    /* Read the correct block from next index. */
                    if(!LLD::Ledger->ReadBlock(tLastBlock.hashNextBlock, tBlock))
                        throw Exception(-83, "Block not found: ", tLastBlock.hashNextBlock.SubString());

                    /* Update hashStart. */
                    hashStart = tBlock.GetHash();
                }

                /* Cache the block hash. */
                tLastBlock = tBlock;

                /* Build JSON object. */
                encoding::json jBlock =
                    BlockToJSON(tLastBlock, nVerbose);

                /* Filter our results now if desired. */
                if(!FilterResults(jParams, jBlock))
                    continue;

                /* Filter out our expected fieldnames if specified. */
                FilterFieldname(jParams, jBlock);

                /* Add to our sorted list. */
                setBlocks.insert(jBlock);

                /* Check our total values. */
                if(setBlocks.size() == nLimit)
                    break;
            }
        }

        /* Build our return value. */
        encoding::json jRet = encoding::json::array();

        /* Make last copy into returned array. */
        for(const auto& jBlock : setBlocks)
            jRet.push_back(jBlock);

        return jRet;
    }
}
