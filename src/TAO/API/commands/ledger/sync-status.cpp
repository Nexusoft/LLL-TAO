/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/types/tritium.h>

#include <TAO/API/types/commands/ledger.h>

#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/format.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/process.h>

#include <Util/types/precision.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Returns an object containing sync related information. */
    encoding::json Ledger::SyncStatus(const encoding::json& jParams, const bool fHelp)
    {
        /* Get the total height left to go. */
        const uint32_t nTotalBlocks = (TAO::Ledger::ChainState::nBestHeight.load() - LLP::TritiumNode::nSyncStart.load());

        /* Get the remaining time. */
        const uint32_t nHours   =  LLP::TritiumNode::nRemainingTime.load() / 3600;
        const uint32_t nMinutes = (LLP::TritiumNode::nRemainingTime.load() - (nHours * 3600)) / 60;
        const uint32_t nSeconds = (LLP::TritiumNode::nRemainingTime.load() - (nHours * 3600)) % 60;

        /* Build our return json. */
        encoding::json jRet;

        /* Populate our block related data. */
        jRet["networkBlock"]  = LLP::TritiumNode::nSyncStop.load();
        jRet["downloadRate"]  = nTotalBlocks / (LLP::TritiumNode::SYNCTIMER.Elapsed() + 1);;

        /* Calculate our peer's block. */
        jRet["completed"] = precision_t(TAO::Ledger::ChainState::PercentSynchronized(), 2).double_t();

        /* The percentage of the current sync completed */
        jRet["progress"]  = precision_t(TAO::Ledger::ChainState::SyncProgress(), 2).double_t();

        /* The percentage of the blocks downloaded */
        jRet["secondsRemaining"] = LLP::TritiumNode::nRemainingTime.load();
        jRet["timeRemaining"]    = debug::safe_printstr
                                   (
                                      std::setw(2), std::setfill('0'), nHours,   ":",
                                      std::setw(2), std::setfill('0'), nMinutes, ":",
                                      std::setw(2), std::setfill('0'), nSeconds
                                   );

        return jRet;
    }
} /* End Ledger Namespace*/
