/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/json.h>

#include <TAO/API/types/commands/ledger.h>

#include <TAO/Ledger/include/chainstate.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Retrieves the block data for a given hash or height. */
    encoding::json Ledger::GetBlock(const encoding::json& jParams, const bool fHelp)
    {
        /* Check for height parameter */
        TAO::Ledger::BlockState tLastBlock;
        if(CheckParameter(jParams, "height", "string, number"))
        {
            /* Check that the node is configured to index blocks by height */
            if(!config::GetBoolArg("-indexheight"))
                throw Exception(-85, "getblock by height requires the daemon to be started with the -indexheight flag.");

            /* Convert the incoming height string to an int*/
            const uint32_t nHeight =
                ExtractNumber<uint32_t>(jParams, "height");

            /* Check that the requested height is within our chain range*/
            if(nHeight > TAO::Ledger::ChainState::nBestHeight.load())
                throw Exception(-60, "[height] out of range [", TAO::Ledger::ChainState::nBestHeight.load(), "]");

            /* Read the block state from the the ledger DB using the height index */
            if(!LLD::Ledger->ReadBlock(nHeight, tLastBlock))
                throw Exception(-83, "Block not found");
        }

        /* Check for block-hash parameter. */
        else if(CheckParameter(jParams, "hash", "string"))
        {
            /* Get our starting block hash. */
            if(!LLD::Ledger->ReadBlock(uint1024_t(jParams["hash"].get<std::string>()), tLastBlock))
                throw Exception(-83, "Block not found");
        }

        /* If no parameters are supplied, do a short regular random-read list. */
        else
            throw Exception(-56, "Missing Parameter [hash or height]");

        /* Default to verbosity 1 which includes only the hash */
        const uint32_t nVerbose =
            ExtractVerbose(jParams, 1);

        /* Build our response JSON. */
        encoding::json jRet =
            TAO::API::BlockToJSON(tLastBlock, nVerbose);

        /* Filter our fieldnames. */
        FilterFieldname(jParams, jRet);

        return jRet;
    }
}
