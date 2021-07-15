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
    /* Retrieves the blockhash for the given height. */
    encoding::json Ledger::GetBlockHash(const encoding::json& jParams, const bool fHelp)
    {
        /* Check that the node is configured to index blocks by height */
        if(!config::GetBoolArg("-indexheight"))
            throw Exception(-79, "[getblockhash] requires the daemon to be started with the -indexheight flag.");

        /* Convert the incoming height string to an int*/
        const uint32_t nHeight = ExtractInteger<uint32_t>(jParams, "height");

        /* Check that the requested height is within our chain range*/
        if(nHeight > TAO::Ledger::ChainState::nBestHeight.load())
            throw Exception(-82, "Block number out of range.");

        /* Read the block state from the the ledger DB using the height index */
        TAO::Ledger::BlockState blockState;
        if(!LLD::Ledger->ReadBlock(nHeight, blockState))
            throw Exception(-83, "Block not found");

        encoding::json ret;
        ret["hash"] = blockState.GetHash().GetHex();

        return ret;
    }
}
