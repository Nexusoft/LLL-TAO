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
    /* Retrieves the block data for a given hash or height. */
    encoding::json Ledger::GetBlock(const encoding::json& jParams, const bool fHelp)
    {
        if(config::fClient.load())
            throw Exception(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");

        /* Check for the block height parameter. */
        if(jParams.find("hash") == jParams.end() && jParams.find("height") == jParams.end())
            throw Exception(-84, "Missing hash or height");

        /* Declare the BlockState to load from the DB */
        TAO::Ledger::BlockState blockState;

        /* look up by height*/
        if(jParams.find("height") != jParams.end())
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
            if(!LLD::Ledger->ReadBlock(nHeight, blockState))
                throw Exception(-83, "Block not found");
        }
        else if(jParams.find("hash") != jParams.end())
        {
            uint1024_t blockHash;
            /* Load the hash from the jParams*/
            blockHash.SetHex(jParams["hash"].get<std::string>());

            /* Read the block state from the the ledger DB using the hash index */
            if(!LLD::Ledger->ReadBlock(blockHash, blockState))
                throw Exception(-83, "Block not found");
        }

        std::string strVerbose = "default";
        if(jParams.find("verbose") != jParams.end())
            strVerbose = jParams["verbose"].get<std::string>();

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

        encoding::json ret = TAO::API::BlockToJSON(blockState, nVerbose);

        return ret;
    }
}
