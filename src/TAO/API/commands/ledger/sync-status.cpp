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

/* Global TAO namespace. */
namespace TAO::API
{
    /* Returns an object containing sync related information. */
    encoding::json Ledger::SyncStatus(const encoding::json& jParams, const bool fHelp)
    {
        /* We will be using these as our stats values below. */
        uint32_t nPeerHeight = 0, nHours = 0, nMinutes = 0, nSeconds = 0, nRate = 0;

        /* Get the current connected legacy node. */
        std::shared_ptr<LLP::TritiumNode> pnode = LLP::TritiumNode::GetNode(TAO::Ledger::nSyncSession.load());
        try //we want to catch exceptions thrown by atomic_ptr in the case there was a free on another thread
        {
            /* Check for potential overflow if current height is not set. */
            if(pnode && pnode->nCurrentHeight > TAO::Ledger::ChainState::nBestHeight.load())
            {
                /* Set our peer height from syncing node. */
                nPeerHeight = pnode->nCurrentHeight;

                /* Get the total height left to go. */
                uint32_t nRemaining = (pnode->nCurrentHeight - TAO::Ledger::ChainState::nBestHeight.load());
                uint32_t nTotalBlocks = (TAO::Ledger::ChainState::nBestHeight.load() - LLP::TritiumNode::nSyncStart.load());

                /* Calculate blocks per second. */
                nRate = nTotalBlocks / (LLP::TritiumNode::SYNCTIMER.Elapsed() + 1);
                LLP::TritiumNode::nRemainingTime.store(nRemaining / (nRate + 1));

                /* Get the remaining time. */
                nHours   =  LLP::TritiumNode::nRemainingTime.load() / 3600;
                nMinutes = (LLP::TritiumNode::nRemainingTime.load() - (nHours * 3600)) / 60;
                nSeconds = (LLP::TritiumNode::nRemainingTime.load() - (nHours * 3600)) % 60;
            }
        }
        catch(const std::exception& e) {}

        /* Build our return json. */
        encoding::json jRet;

        /* Populate our block related data. */
        jRet["currentBlock"]  = TAO::Ledger::ChainState::nBestHeight.load();
        jRet["networkBlock"]  = nPeerHeight;
        jRet["downloadRate"]  = nRate;

        /* Calculate our peer's block. */
        jRet["synchronizing"] = (TAO::Ledger::ChainState::nBestHeight.load() < nPeerHeight - 1);

        /* The percentage of the blocks downloaded */
        jRet["secondsRemaining"] = LLP::TritiumNode::nRemainingTime.load();
        jRet["timeRemaining"]    = debug::safe_printstr(
                                 "[", std::setw(2), std::setfill('0'), nHours, ":",
                                      std::setw(2), std::setfill('0'), nMinutes, ":",
                                      std::setw(2), std::setfill('0'), nSeconds, "]");

        /* The percentage of the current sync completed */
        jRet["percentSynchronized"] = (uint32_t)TAO::Ledger::ChainState::PercentSynchronized();

        return jRet;
    }
} /* End Ledger Namespace*/
