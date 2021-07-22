/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/ledger.h>

#include <TAO/API/include/extract.h>
#include <TAO/API/include/format.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/include/chainstate.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Returns an object containing mining-related information. */
    encoding::json Ledger::GetInfo(const encoding::json& jParams, const bool fHelp)
    {
        /* Build our proof of stake data. */
        encoding::json jRet =
        {
            { "stake", ChannelToJSON(0) },
            { "prime", ChannelToJSON(1) },
            { "hash",  ChannelToJSON(2) },
        };

        /* Grab our best block. */
        const TAO::Ledger::BlockState tBestBlock =
            TAO::Ledger::ChainState::stateBest.load();

        /* Add chain-state data. */
        jRet["height"]     = tBestBlock.nHeight;
        jRet["timestamp"]  = tBestBlock.GetBlockTime();
        jRet["checkpoint"] = tBestBlock.hashCheckpoint.GetHex();

        return jRet;
    }
} /* End Ledger Namespace*/
