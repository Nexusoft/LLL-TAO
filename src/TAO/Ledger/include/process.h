/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2026

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_PROCESS_H
#define NEXUS_TAO_LEDGER_INCLUDE_PROCESS_H

#include <LLC/types/uint1024.h>

#include <TAO/Ledger/types/block.h>

#include <map>
#include <mutex>
#include <memory>

namespace LLP { class TritiumNode; }

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** Enum values for returning the block's state after processing. **/
        namespace PROCESS
        {
            enum
            {
                ORPHAN     = (1 << 1), //has no previous block
                DUPLICATE  = (1 << 2), //already in database
                ACCEPTED   = (1 << 3), //processed fully
                REJECTED   = (1 << 4), //block was rejected
                IGNORED    = (1 << 5), //ignore protocol requests
                INCOMPLETE = (1 << 6), //block contains missing transactions
            };
        }


        /** Static instantiation of orphan blocks in queue to process. **/
        extern std::map<uint1024_t, std::unique_ptr<TAO::Ledger::Block>> mapOrphans;


        /** Track the times we have requested processed missing transactions so we don't loop too much. **/
        extern std::pair<uint1024_t, uint64_t> pairLastMissing;


        /** Mutex to protect checking more than one block at a time. **/
        extern std::mutex PROCESSING_MUTEX;


        /** Sync timer value. **/
        extern uint64_t nSynchronizationTimer;


        /** Current sync node. **/
        extern std::atomic<uint64_t> nSyncSession;


        /* Stats variable for syncing. */
        extern std::atomic<uint64_t> nProcessedContracts;

        /** Process Block Function
         *
         *  Processes a block incoming over the network.
         *
         *  @param[in] block The block being processed
         *  @param[out] nStatus The status flags returned.
         *  @param[out] pnode The node that block came from.
         *
         **/
        void Process(const TAO::Ledger::Block& block, uint8_t &nStatus, LLP::TritiumNode* pnode = nullptr);

    }
}

#endif
