/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/types/tritium.h>
#include <LLP/include/global.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/checkpoints.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/genesis_block.h>
#include <TAO/Ledger/include/timelocks.h>

#include <TAO/API/include/global.h>

#include <functional>
#include <map>
#include <vector>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {
        /* The best block height in the chain. */
        std::atomic<uint32_t> ChainState::nBestHeight;


        /* The highest block height advertised by any connected peer. */
        std::atomic<uint32_t> ChainState::nMaxPeerHeight(0);


        /* The best trust in the chain. */
        std::atomic<uint64_t> ChainState::nBestChainTrust;


        /* The current checkpoint height. */
        std::atomic<uint64_t> ChainState::nCheckpointHeight;


        /* The best block in the chain. */
        std::atomic<bool> ChainState::fChainReorg;


        /* The best hash in the chain. */
        memory::atomic<uint1024_t> ChainState::hashBestChain;


        /* Hardened Checkpoint. */
        memory::atomic<uint1024_t> ChainState::hashCheckpoint;


        /* The best block in the chain. */
        memory::atomic<BlockState> ChainState::tStateBest;


        /* The best block in the chain. */
        BlockState ChainState::tStateGenesis;


        /* Publish all best-tip fields from the same committed state. */
        void ChainState::SetBestHeight(const BlockState& state)
        {
            tStateBest.store(state);
            hashBestChain.store(state.GetHash());
            nBestChainTrust.store(state.nChainTrust, std::memory_order_release);
            TAO::API::nBlockCounter.fetch_add(1, std::memory_order_release);
            nBestHeight.store(state.nHeight, std::memory_order_release);
        }


        /* Flag to tell if initial blocks are downloading. */
        //static std::atomic<bool> fSynchronizing(true);
        bool ChainState::Synchronizing()
        {
            /* Static values to check synchronization status. */
            static memory::atomic<uint1024_t> hashLast;
            static std::atomic<uint64_t> nLastTime;

            bool fSynchronizing = true;
            if(!config::GetBoolArg("-sync", true)) //hard value to rely on if needed
                return false;

            /* Persistent switch once synchronized. */
            //if(!fSynchronizing.load())
            //    return false;

            #ifndef UNIT_TESTS

            /* Check for null best state. */
            if(tStateBest.load().IsNull())
                return true;

            /* Check if there's been a new block from internal static values. */
            if(hashBestChain.load() != hashLast.load())
            {
                hashLast = hashBestChain.load();
                nLastTime = runtime::unifiedtimestamp();
            }

            /* Special testnet rules*/
            if(config::fTestNet.load())
            {
                /* Check for specific conditions such as local testnet */
                const bool fLocalTestnet   =
                    (config::fTestNet.load() && (!config::GetBoolArg("-dns", true) || config::fHybrid.load()));

                /* Check for shutdown. */
                if(config::fShutdown.load())
                    return false;

                /* Check for active connections. */
                bool fHasConnections =
                    (LLP::TRITIUM_SERVER && LLP::TRITIUM_SERVER->GetConnectionCount() > 0);

                /* Set the synchronizing flag. */
                fSynchronizing =
                (
                    /* If using main testnet then rely on the LLP synchronized flag */
                    (!fLocalTestnet && !LLP::TritiumNode::fSynchronized.load()
                        && tStateBest.load().GetBlockTime() < runtime::unifiedtimestamp() - 20 * 60)

                    /* If local testnet with connections then rely on LLP flag  */
                    || (fLocalTestnet && fHasConnections && !LLP::TritiumNode::fSynchronized.load())

                    /* If local testnet with no connections then assume sync'd if the last block was more than 30s ago
                       and block age is more than 20 mins, which gives us a 30s window to connect to a local peer */
                    || (fLocalTestnet && !fHasConnections
                        && runtime::unifiedtimestamp() - nLastTime < 30
                        && tStateBest.load().GetBlockTime() < runtime::unifiedtimestamp() - 20 * 60)
                );

                return fSynchronizing;
            }

            /* Check if block has been created within 60 minutes. */
            fSynchronizing =
            (
                (!LLP::TritiumNode::fSynchronized.load() &&
                (tStateBest.load().GetBlockTime() < runtime::unifiedtimestamp() - 60 * 60))
            );

            return fSynchronizing;

            /* On unit tests, always keep Synchronizing off. */
            #else
            _unused(fSynchronizing); //suppress compiler warnings
            return false;
            #endif
        }


        /* Real value of the total synchronization percent completion. */
        double ChainState::PercentSynchronized()
        {
            /* The timstamp of the genesis block.   */
            static const uint32_t nGenesis =
                (config::fClient.load() ? TritiumGenesis().nHeight : config::fHybrid.load() ? HybridGenesis().nHeight : LegacyGenesis().nHeight);

            /* Calculate the time between the last block received and now. */
            const uint32_t nBlocks = tStateBest.load().nHeight - nGenesis;
            const uint32_t nTotals = (LLP::TritiumNode::nSyncStop.load() - nGenesis);

            /* Calculate the sync percent. */
            return std::min(100.0, (100.0 * nBlocks) / nTotals);
        }


        /* Percentage of blocks synchronized since the node started. */
        double ChainState::SyncProgress()
        {
            /* Catch if we aren't syncing yet. */
            if(LLP::TritiumNode::nSyncStop.load() == 0)
                return 0.0;

            /* Total blocks synchronized */
            const uint32_t nBlocks = tStateBest.load().nHeight - LLP::TritiumNode::nSyncStart.load();
            const uint32_t nTotals = LLP::TritiumNode::nSyncStop.load() - LLP::TritiumNode::nSyncStart.load();

            /* Calculate the sync percent. */
            return std::min(100.0, (100.0 * nBlocks) / nTotals);
        }

        namespace
        {
#ifdef UNIT_TESTS
            static std::function<bool(const BlockState&)> fnCheckpointRepairSetBestHook;
#endif
            /* Startup audit focuses on recent ancestry where production holes were observed,
             * while keeping healthy startup I/O bounded. */
            static constexpr uint32_t STARTUP_CHAIN_AUDIT_DEPTH = 512;

            bool VerifyCheckpointRollbackPath(const BlockState& stateBest,
                                              const BlockState& stateTarget,
                                              uint32_t& nRollbackDepth)
            {
                nRollbackDepth = 0;

                if(stateTarget.IsNull())
                    return debug::error(FUNCTION, "checkpoint rollback preflight failed: target is null");

                if(stateBest.nHeight < stateTarget.nHeight)
                {
                    return debug::error(FUNCTION,
                        "checkpoint rollback preflight failed: target height ", stateTarget.nHeight,
                        " exceeds current best height ", stateBest.nHeight);
                }

                BlockState stateWalk = stateBest;
                uint1024_t hashWalk = stateWalk.GetHash();
                const uint1024_t hashTarget = stateTarget.GetHash();
                const uint32_t nExpectedDepth = stateBest.nHeight - stateTarget.nHeight;
                if(hashWalk == hashTarget)
                    return true;

                while(hashWalk != hashTarget)
                {
                    if(nRollbackDepth >= nExpectedDepth)
                    {
                        return debug::error(FUNCTION,
                            "checkpoint rollback preflight failed: checkpoint hash ",
                            hashTarget.SubString(), " is not on the current best-chain ancestry");
                    }

                    if(stateWalk.hashPrevBlock == 0)
                    {
                        return debug::error(FUNCTION,
                            "checkpoint rollback preflight failed: reached genesis before target hash ",
                            hashTarget.SubString(), " after ", nRollbackDepth, " steps");
                    }

                    BlockState statePrev;
                    if(!LLD::Ledger->ReadBlock(stateWalk.hashPrevBlock, statePrev))
                    {
                        return debug::error(FUNCTION,
                            "checkpoint rollback preflight failed: missing ancestor block ",
                            stateWalk.hashPrevBlock.SubString(), " while walking back from height ",
                            stateWalk.nHeight, " hash ", hashWalk.SubString());
                    }

                    const uint1024_t hashPrev = statePrev.GetHash();
                    if(hashPrev != stateWalk.hashPrevBlock)
                    {
                        return debug::error(FUNCTION,
                            "checkpoint rollback preflight failed: ancestor hash mismatch expected ",
                            stateWalk.hashPrevBlock.SubString(), " but read ", hashPrev.SubString());
                    }

                    if(statePrev.hashNextBlock != hashWalk)
                    {
                        return debug::error(FUNCTION,
                            "checkpoint rollback preflight failed: broken link at height ",
                            statePrev.nHeight, " hash ", hashPrev.SubString(), " expected next ",
                            hashWalk.SubString(), " but found ", statePrev.hashNextBlock.SubString());
                    }

                    if(stateWalk.nHeight == 0 || statePrev.nHeight + 1 != stateWalk.nHeight)
                    {
                        return debug::error(FUNCTION,
                            "checkpoint rollback preflight failed: non-contiguous heights current=",
                            stateWalk.nHeight, " prev=", statePrev.nHeight, " at hash ",
                            hashWalk.SubString());
                    }

                    stateWalk = statePrev;
                    hashWalk = hashPrev;
                    ++nRollbackDepth;
                }

                if(nRollbackDepth != nExpectedDepth)
                {
                    return debug::error(FUNCTION,
                        "checkpoint rollback preflight failed: reached target hash at depth ",
                        nRollbackDepth, " but expected depth ", nExpectedDepth);
                }

                return true;
            }


            struct ChainHoleAudit
            {
                bool fHoleDetected{false};
                bool fCanRepairFromHeight{false};
                uint32_t nDepthScanned{0};
                BlockState stateChild;
                uint1024_t hashMissingPrev;
                std::vector<BlockState> vRecoveredSuffix;
            };


            bool ReadRecoveryCandidateFromHeightIndex(const uint32_t nHeight, BlockState& stateOut)
            {
                if(!LLD::Ledger->ReadBlock(nHeight, stateOut))
                    return false;

                const uint1024_t hashState = stateOut.GetHash();
                if(hashState == 0 || stateOut.nHeight != nHeight)
                    return false;

                return true;
            }


            bool BuildHeightIndexedRepairSuffix(const uint32_t nMaxDepth, ChainHoleAudit& audit)
            {
                audit.fCanRepairFromHeight = false;
                audit.vRecoveredSuffix.clear();

                BlockState stateChild = audit.stateChild;
                uint1024_t hashChild = stateChild.GetHash();
                if(hashChild == 0)
                    return debug::error(FUNCTION, "best-chain repair preflight failed: child hash is null");

                const uint32_t nBound = std::max<uint32_t>(1, nMaxDepth);
                while(stateChild.hashPrevBlock != 0 && audit.vRecoveredSuffix.size() < nBound)
                {
                    BlockState statePrevByHash;
                    if(LLD::Ledger->ReadBlock(stateChild.hashPrevBlock, statePrevByHash))
                    {
                        const uint1024_t hashPrev = statePrevByHash.GetHash();
                        if(hashPrev != stateChild.hashPrevBlock)
                        {
                            return debug::error(FUNCTION,
                                "best-chain repair preflight failed: anchor hash mismatch expected ",
                                stateChild.hashPrevBlock.SubString(), " but read ", hashPrev.SubString(),
                                " for child height ", stateChild.nHeight, " hash ", hashChild.SubString());
                        }

                        if(stateChild.nHeight == 0 || statePrevByHash.nHeight + 1 != stateChild.nHeight)
                        {
                            return debug::error(FUNCTION,
                                "best-chain repair preflight failed: anchor height mismatch child=",
                                stateChild.nHeight, " prev=", statePrevByHash.nHeight, " childHash=",
                                hashChild.SubString(), " prevHash=", hashPrev.SubString());
                        }

                        if(statePrevByHash.hashNextBlock != hashChild)
                        {
                            return debug::error(FUNCTION,
                                "best-chain repair preflight failed: anchor successor mismatch at height ",
                                statePrevByHash.nHeight, " hash ", hashPrev.SubString(), " expected next ",
                                hashChild.SubString(), " but found ", statePrevByHash.hashNextBlock.SubString());
                        }

                        audit.fCanRepairFromHeight = !audit.vRecoveredSuffix.empty();
                        return true;
                    }

                    if(stateChild.nHeight == 0)
                        break;

                    BlockState statePrevByHeight;
                    if(!ReadRecoveryCandidateFromHeightIndex(stateChild.nHeight - 1, statePrevByHeight))
                        return true;

                    const uint1024_t hashPrevByHeight = statePrevByHeight.GetHash();
                    if(hashPrevByHeight != stateChild.hashPrevBlock
                    || statePrevByHeight.nHeight + 1 != stateChild.nHeight
                    || statePrevByHeight.hashNextBlock != hashChild)
                        return true;

                    audit.vRecoveredSuffix.push_back(statePrevByHeight);
                    stateChild = statePrevByHeight;
                    hashChild = hashPrevByHeight;
                }

                if(stateChild.hashPrevBlock == 0
                && stateChild.nHeight == 0
                && hashChild == ChainState::Genesis())
                    audit.fCanRepairFromHeight = !audit.vRecoveredSuffix.empty();

                return true;
            }


            bool ScanActiveBestChain(const uint32_t nMaxDepth, ChainHoleAudit& audit)
            {
                audit = ChainHoleAudit();

                const BlockState stateBest = ChainState::tStateBest.load();
                if(stateBest.IsNull())
                    return debug::error(FUNCTION, "best chain audit failed: current best state is null");

                BlockState stateWalk = stateBest;
                uint1024_t hashWalk = stateWalk.GetHash();
                if(hashWalk == 0)
                    return debug::error(FUNCTION, "best chain audit failed: current best hash is null");

                const uint32_t nBound = std::max<uint32_t>(1, nMaxDepth);
                for(uint32_t nDepth = 0; nDepth < nBound && stateWalk.hashPrevBlock != 0; ++nDepth)
                {
                    BlockState statePrev;
                    if(!LLD::Ledger->ReadBlock(stateWalk.hashPrevBlock, statePrev))
                    {
                        audit.fHoleDetected = true;
                        audit.nDepthScanned = nDepth;
                        audit.stateChild = stateWalk;
                        audit.hashMissingPrev = stateWalk.hashPrevBlock;

                        debug::error(FUNCTION,
                            "active-chain ancestry hole detected at depth ", nDepth, ": child height ",
                            stateWalk.nHeight, " hash ", hashWalk.SubString(), " references missing predecessor ",
                            stateWalk.hashPrevBlock.SubString());

                        const uint32_t nRepairDepth = nBound - nDepth;
                        if(BuildHeightIndexedRepairSuffix(nRepairDepth, audit))
                        {
                            if(audit.fCanRepairFromHeight)
                            {
                                const BlockState& stateHighestRecovered = audit.vRecoveredSuffix.front();
                                const BlockState& stateLowestRecovered = audit.vRecoveredSuffix.back();
                                debug::log(0, FUNCTION,
                                    "located matching predecessor suffix via height index: recovered ",
                                    audit.vRecoveredSuffix.size(), " hash alias(es) covering heights ",
                                    stateLowestRecovered.nHeight, "..", stateHighestRecovered.nHeight,
                                    " for child height ", stateWalk.nHeight, " hash ", hashWalk.SubString());
                            }
                        }
                        else
                            return false;

                        return true;
                    }

                    const uint1024_t hashPrev = statePrev.GetHash();
                    if(hashPrev != stateWalk.hashPrevBlock)
                    {
                        return debug::error(FUNCTION,
                            "active-chain ancestry mismatch at depth ", nDepth, ": expected predecessor hash ",
                            stateWalk.hashPrevBlock.SubString(), " but read ", hashPrev.SubString(),
                            " from child height ", stateWalk.nHeight, " hash ", hashWalk.SubString());
                    }

                    if(stateWalk.nHeight == 0 || statePrev.nHeight + 1 != stateWalk.nHeight)
                    {
                        return debug::error(FUNCTION,
                            "active-chain ancestry mismatch at depth ", nDepth, ": non-contiguous heights child=",
                            stateWalk.nHeight, " prev=", statePrev.nHeight, " childHash=", hashWalk.SubString());
                    }

                    if(statePrev.hashNextBlock != hashWalk)
                    {
                        return debug::error(FUNCTION,
                            "active-chain ancestry mismatch at depth ", nDepth, ": predecessor height ",
                            statePrev.nHeight, " hash ", hashPrev.SubString(), " points to next ",
                            statePrev.hashNextBlock.SubString(), " expected ", hashWalk.SubString());
                    }

                    stateWalk = statePrev;
                    hashWalk = hashPrev;
                    audit.nDepthScanned = nDepth + 1;
                }

                if(stateWalk.hashPrevBlock == 0
                && (stateWalk.nHeight != 0 || hashWalk != ChainState::Genesis()))
                {
                    return debug::error(FUNCTION,
                        "active-chain ancestry terminates at invalid genesis: height ",
                        stateWalk.nHeight, " hash ", hashWalk.SubString(),
                        " expected ", ChainState::Genesis().SubString());
                }

                return true;
            }


            bool RepairActiveChainHole(const bool fAllowRepair, const uint32_t nMaxDepth)
            {
                ChainHoleAudit audit;
                if(!ScanActiveBestChain(nMaxDepth, audit))
                    return false;

                if(!audit.fHoleDetected)
                    return true;

                const BlockState stateBest = ChainState::tStateBest.load();
                if(!fAllowRepair)
                {
                    return debug::error(FUNCTION,
                        "best-chain ancestry is not contiguous near tip (best height ", stateBest.nHeight,
                        " hash ", stateBest.GetHash().SubString(), "). Restart with -repairchain=1 to attempt "
                        "non-destructive index repair when recoverable; otherwise restore from backup or rebuild "
                        "from a trusted snapshot.");
                }

                if(!audit.fCanRepairFromHeight || audit.vRecoveredSuffix.empty())
                {
                    return debug::error(FUNCTION,
                        "best-chain repair could not locate a verified predecessor record for missing hash ",
                        audit.hashMissingPrev.SubString(), " referenced by child height ",
                        audit.stateChild.nHeight, " hash ", audit.stateChild.GetHash().SubString(),
                        ". No mutation performed. Back up data directory, restore from backup, or rebuild from "
                        "a trusted snapshot.");
                }

                LLD::TransactionGuard transaction;
                if(!transaction)
                    return debug::error(FUNCTION, "failed to begin best-chain repair transaction");

                /* Discard unreadable cached data and preserve the sector shared with the height index. */
                for(const auto& stateRecovered : audit.vRecoveredSuffix)
                {
                    const uint1024_t hashRecovered = stateRecovered.GetHash();
                    if(hashRecovered == 0
                    || !LLD::Ledger->Erase(hashRecovered, true)
                    || !LLD::Ledger->Index(hashRecovered,
                        std::make_pair(std::string("height"), stateRecovered.nHeight)))
                    {
                        LLD::TxnAbort();
                        return debug::error(FUNCTION, "failed to restore missing predecessor block key ",
                            hashRecovered.SubString());
                    }
                }

                /* Validate the staged repair before making any mutation durable. */
                ChainHoleAudit verifyAudit;
                if(!ScanActiveBestChain(nMaxDepth, verifyAudit))
                    return false;

                if(verifyAudit.fHoleDetected)
                {
                    return debug::error(FUNCTION,
                        "best-chain repair did not restore contiguity near tip; unresolved missing predecessor ",
                        verifyAudit.hashMissingPrev.SubString(), " at child height ",
                        verifyAudit.stateChild.nHeight, " hash ",
                        verifyAudit.stateChild.GetHash().SubString());
                }

                if(!LLD::TxnCommit())
                {
                    LLD::TxnAbort();
                    return debug::error(FUNCTION, "disk commit failed while restoring predecessor block key");
                }

                for(const auto& stateRecovered : audit.vRecoveredSuffix)
                {
                    const uint1024_t hashRecovered = stateRecovered.GetHash();
                    BlockState stateRestored;
                    if(!LLD::Ledger->ReadBlock(hashRecovered, stateRestored)
                    || stateRestored.GetHash() != hashRecovered
                    || stateRestored.nHeight != stateRecovered.nHeight)
                    {
                        return debug::error(FUNCTION,
                            "post-repair verification failed for restored predecessor block key ",
                            hashRecovered.SubString());
                    }
                }

                debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "WARNING: ", ANSI_COLOR_RESET,
                    "restored ", audit.vRecoveredSuffix.size(), " missing predecessor key(s) ending at ",
                    audit.hashMissingPrev.SubString(), " using height-index recovery data");

                return true;
            }


            bool RecoverMissingHardcodedCheckpoint(const std::map<uint32_t, uint1024_t>& mapCheckpointList,
                                                   const bool fAllowRepair)
            {
                for(auto it = mapCheckpointList.rbegin(); it != mapCheckpointList.rend(); ++it)
                {
                    /* Check that we are within correct height ranges. */
                    if(it->first > ChainState::tStateBest.load().nHeight)
                        continue;

                    /* Load the hardcoded checkpoint from disk. */
                    BlockState stateCheck;
                    if(LLD::Ledger->ReadBlock(it->second, stateCheck))
                    {
                        if(stateCheck.GetHash() == it->second && stateCheck.nHeight == it->first)
                            continue;

                        debug::warning(FUNCTION,
                            "hardcoded checkpoint record is unreadable or invalid at height ", it->first,
                            " expected hash ", it->second.SubString(), " read hash ",
                            stateCheck.GetHash().SubString(), " read height ", stateCheck.nHeight,
                            ". Treating this checkpoint as missing for read-only startup diagnostics.");
                    }
                    else
                    {
                        debug::warning(FUNCTION,
                            "missing hardcoded checkpoint record at height ", it->first, " hash ",
                            it->second.SubString(), ". No rollback will be attempted by default.");
                    }

                    if(!fAllowRepair)
                    {
                        debug::warning(FUNCTION,
                            "historical hardcoded checkpoint record is unavailable at height ", it->first,
                            " hash ", it->second.SubString(),
                            "; this is retained as startup diagnostics only and does not imply active-chain "
                            "damage. To attempt checkpoint-record rollback only, restart with "
                            "-repaircheckpoints=1 after backing up the data directory.");
                        continue;
                    }

                    BlockState stateAncestor;
                    auto iAncestor = it;
                    bool fFoundAncestor = false;
                    while(++iAncestor != mapCheckpointList.rend())
                    {
                        if(!LLD::Ledger->HasBlock(iAncestor->second))
                            continue;

                        if(!LLD::Ledger->ReadBlock(iAncestor->second, stateAncestor))
                            continue;

                        if(stateAncestor.GetHash() != iAncestor->second || stateAncestor.nHeight != iAncestor->first)
                        {
                            debug::error(FUNCTION,
                                "skipping invalid fallback hardcoded checkpoint at height ", iAncestor->first,
                                " expected hash ", iAncestor->second.SubString(), " read hash ",
                                stateAncestor.GetHash().SubString(), " read height ", stateAncestor.nHeight);
                            continue;
                        }

                        fFoundAncestor = true;
                        break;
                    }

                    if(!fFoundAncestor)
                    {
                        return debug::error(FUNCTION,
                            "missing earliest applicable hardcoded checkpoint record. Back up data directory and "
                            "restore from backup or rebuild from a trusted snapshot.");
                    }

                    const BlockState stateBest = ChainState::tStateBest.load();
                    uint32_t nRollbackDepth = 0;
                    if(!VerifyCheckpointRollbackPath(stateBest, stateAncestor, nRollbackDepth))
                    {
                        return debug::error(FUNCTION,
                            "unable to prove a complete linked rollback path from current best height ",
                            stateBest.nHeight, " hash ", stateBest.GetHash().SubString(),
                            " to hardcoded ancestor height ",
                            iAncestor->first, " hash ", iAncestor->second.SubString(),
                            ". No mutation performed. Back up data directory and restore from backup, "
                            "or rebuild from a trusted snapshot.");
                    }

                    debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "WARNING: ", ANSI_COLOR_RESET,
                        " hardcoded checkpoint repair candidate depth=", nRollbackDepth,
                        " targetHeight=", iAncestor->first, " targetHash=", iAncestor->second.SubString());

                    if(!fAllowRepair)
                    {
                        return debug::error(FUNCTION,
                            "destructive hardcoded checkpoint rollback is disabled by default. Restart with "
                            "-repaircheckpoints=1 only after backing up the data directory.");
                    }

                    const BlockState stateCurrentBest = ChainState::tStateBest.load();
                    if(stateCurrentBest.GetHash() != stateBest.GetHash()
                    || stateCurrentBest.nHeight != stateBest.nHeight)
                    {
                        return debug::error(FUNCTION,
                            "hardcoded checkpoint repair aborted: best chain changed during preflight from height ",
                            stateBest.nHeight, " hash ", stateBest.GetHash().SubString(), " to height ",
                            stateCurrentBest.nHeight, " hash ", stateCurrentBest.GetHash().SubString(),
                            ". No mutation performed; retry startup.");
                    }

                    debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "WARNING: ", ANSI_COLOR_RESET,
                        " REPAIRING TO HARDCODED Ancestor ", iAncestor->first, " Hash ",
                        iAncestor->second.SubString(), " Depth ", nRollbackDepth);

                    /* Set the best to older block. */
                    LLD::TransactionGuard transaction;
                    if(!transaction)
                        return debug::error(FUNCTION, "failed to begin checkpoint revert transaction");

                    bool fSetBest = false;
#ifdef UNIT_TESTS
                    if(fnCheckpointRepairSetBestHook)
                        fSetBest = fnCheckpointRepairSetBestHook(stateAncestor);
                    else
#endif
                        fSetBest = stateAncestor.SetBest();
                    if(!fSetBest)
                    {
                        LLD::TxnAbort();
                        return debug::error(FUNCTION, "failed to revert to hardcoded ancestor checkpoint");
                    }
                    else if(LLD::HasOpenTransaction() && !LLD::TxnCommit())
                    {
                        LLD::TxnAbort();
                        return debug::error(FUNCTION,
                            "disk commit failed after reverting to hardcoded ancestor checkpoint");
                    }

                    break;
                }

                return true;
            }
        }


        /* Initialize the Chain State. */
        bool ChainState::Initialize()
        {
            /* Initialize the Genesis. */
            if(!CreateGenesis())
                return debug::error(FUNCTION, "failed to create genesis");

            /* Read the best chain. */
            if(!LLD::Ledger->ReadBestChain(hashBestChain))
                return debug::error(FUNCTION, "failed to read best chain");

            /* Get the best chain stats. */
            if(!LLD::Ledger->ReadBlock(hashBestChain.load(), tStateBest))
            {
                debug::error(FUNCTION, "failed to read best block, attempting to recover database");

                /* If hashBestChain exists, but block doesn't attempt to recover database from invalid write.  */
                BlockState tStateBestKnown = tStateGenesis;
                while(!tStateBestKnown.IsNull())
                {
                    tStateBest = tStateBestKnown;

                    if(tStateBestKnown.hashNextBlock == 0)
                        break;

                    tStateBestKnown = tStateBestKnown.Next();
                }

                /* Once new best chain is found, write it to disk. */
                hashBestChain = tStateBest.load().GetHash();
                if(!LLD::Ledger->WriteBestChain(hashBestChain.load()))
                    return debug::error(FUNCTION, "failed to write best chain");

                debug::log(0, FUNCTION, "database successfully recovered");
            }

            /* Check database consistency. */
            if(tStateBest.load().GetHash() != hashBestChain.load())
                return debug::error(FUNCTION, "disk index inconsistent with best chain");

            /* Reverse iterator to find the most recent common ancestor. Skip if not on mainnet*/
            if(!config::fHybrid.load() && !config::fTestNet.load() && !config::fClient.load())
            {
                if(!RepairActiveChainHole(config::GetBoolArg("-repairchain", false), STARTUP_CHAIN_AUDIT_DEPTH))
                    return false;

                if(!RecoverMissingHardcodedCheckpoint(mapCheckpoints, config::GetBoolArg("-repaircheckpoints", false)))
                    return false;
            }

            /* Rewind the chain a total number of blocks. */
            uint64_t nRevertBlocks = config::GetArg("-revertblocks", 0);
            if(nRevertBlocks > 0)
            {
                /* Rollback the chain a given number of blocks. */
                TAO::Ledger::BlockState state = tStateBest.load();
                for(int i = 0; i < nRevertBlocks; ++i)
                {
                    /* Check for Genesis. */
                    if(state.hashPrevBlock == 0)
                        break;

                    /* Iterate backwards the total number of blocks requested. */
                    state = state.Prev();
                    if(!state)
                        return debug::error(FUNCTION, "failed to find ancestor block");
                }

                /* Set the best to older block. */
                LLD::TransactionGuard transaction;
                if(!transaction)
                    return debug::error(FUNCTION, "failed to begin rewind transaction");

                /* Abort our transaction if we fail to rollback. */
                if(!state.SetBest())
                {
                    /* Debug Output. */
                    debug::log(0, FUNCTION, "-revertblocks=XXX failed to remove ", nRevertBlocks, " blocks");
                    LLD::TxnAbort();
                    return false;
                }
                else
                {
                    /* Debug Output. */
                    debug::log(0, FUNCTION, "-revertblocks=XXX requested removal of ", nRevertBlocks, " blocks");
                    /* SetBest() commits the transaction internally when it succeeds.
                     * Guard with HasOpenTransaction() so that the now-closed outer
                     * transaction is not misreported as a commit failure. */
                    if(LLD::HasOpenTransaction() && !LLD::TxnCommit())
                        return debug::error(FUNCTION, "disk commit failed after -revertblocks rewind");
                }
            }

            /* Fill out the best chain stats. */
            SetBestHeight(tStateBest.load());

            /* Set the checkpoint. */
            hashCheckpoint = tStateBest.load().hashCheckpoint;

            /* Find the last checkpoint. */
            if(tStateBest != tStateGenesis)
            {
                /* Go back 10 checkpoints on startup. */
                for(uint32_t i = 0; i < config::GetArg("-checkcheckpoints", 100); ++i)
                {
                    /* Search back until fail or different checkpoint. */
                    BlockState state;
                    if(!LLD::Ledger->ReadBlock(hashCheckpoint.load(), state))
                        break;

                    /* Check we haven't reached the genesis */
                    if(state == tStateGenesis)
                        break;

                    /* Get the previous state. */
                    state = state.Prev();
                    if(!state)
                        return debug::error(FUNCTION, "failed to find the checkpoint");

                    /* Set the checkpoint. */
                    hashCheckpoint    = state.hashCheckpoint;

                    /* Get checkpoint state. */
                    BlockState stateCheckpoint;
                    if(!LLD::Ledger->ReadBlock(state.hashCheckpoint, stateCheckpoint))
                        return debug::error(FUNCTION, "failed to read checkpoint");

                    /* Set the correct height for the checkpoint. */
                    nCheckpointHeight = stateCheckpoint.nHeight;
                }
            }

            /* Ensure the block height index is intact */
            if(config::GetBoolArg("-indexheight") || config::GetBoolArg("-reindexheight"))
            {
                /* Build our indexing height. */
                TAO::Ledger::BlockState tLastBlock;
                if(config::GetBoolArg("-reindexheight") || !LLD::Ledger->ReadBlock(nCheckpointHeight.load(), tLastBlock))
                {
                    /* Check for first block index. */
                    if(LLD::Ledger->ReadBlock(1, tLastBlock)) //check for genesis
                    {
                        /* We use this to jump back more than 1 db read at a time. */
                        uint32_t nInterval = 1;

                        /* Check back to our last index. */
                        uint32_t nLastHeight = nCheckpointHeight.load();
                        while(!LLD::Ledger->ReadBlock(nLastHeight, tLastBlock) && !config::fShutdown.load())
                        {
                            /* Exit if we reach the genesis. */
                            if(nLastHeight == 0)
                            {
                                tLastBlock = tStateGenesis;
                                break;
                            }

                            /* Jump backwards at increasing intervals. */
                            nLastHeight = std::max(uint32_t(0), nLastHeight - nInterval++);
                        }
                    }
                    else
                        tLastBlock = tStateGenesis;

                    /* Use genesis as our hash start. */
                    uint1024_t hashStart = tLastBlock.GetHash();

                    /* Track our timing. */
                    runtime::timer tElapsed;
                    tElapsed.Start();

                    /* List our blocks via a batch read for efficiency. */
                    std::vector<TAO::Ledger::BlockState> vStates;
                    while(!config::fShutdown.load() && hashStart != TAO::Ledger::ChainState::hashBestChain.load() &&
                        LLD::Ledger->BatchRead(hashStart, "block", vStates, 1000, true))
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
                                /* Read the correct block from next index. */
                                if(!LLD::Ledger->ReadBlock(tLastBlock.hashNextBlock, tBlock))
                                    return debug::error("Block not found: ", tLastBlock.hashNextBlock.SubString());

                                /* Update hashStart. */
                                hashStart = tBlock.GetHash();
                            }

                            /* Cache the block hash. */
                            tLastBlock = tBlock;

                            /* Add a meter for progress output. */
                            if(tLastBlock.nHeight % 10000 == 0)
                            {
                                /* Calculate our percentage completed. */
                                const double dPercentage = (100.0 * tLastBlock.nHeight) / TAO::Ledger::ChainState::nBestHeight.load();

                                /* Get elapsed timestamp. */
                                const uint64_t nElapsed = tElapsed.ElapsedMilliseconds() + 1;

                                /* Find remaining time. */
                                const uint64_t nRemaining =
                                    uint64_t(nElapsed * TAO::Ledger::ChainState::nBestHeight.load()) / tBlock.nHeight;

                                /* Log status message of completion time. */
                                debug::log(0, "Completed ", tLastBlock.nHeight, "/",
                                    TAO::Ledger::ChainState::nBestHeight.load(), std::fixed, " [", dPercentage, " %]",
                                    "[", (nRemaining - nElapsed) / 1000, "s remaining]");
                            }

                            /* Write the new heights to disk. */
                            if(!LLD::Ledger->IndexBlock(tBlock.nHeight, hashStart))
                                return debug::error("Failed to index height: ", hashStart.SubString());
                        }
                    }
                }
            }

            /* Check if we need to persist the -indexheight flag. */
            else
            {
                /* Build our indexing height. */
                TAO::Ledger::BlockState tLastBlock;
                if(LLD::Ledger->ReadBlock(nCheckpointHeight.load(), tLastBlock))
                {
                    /* Check there is no argument supplied. */
                    if(!config::HasArg("-indexheight"))
                    {
                        /* Warn that -indexheight is persistent. */
                        debug::log(0, FUNCTION, "-indexheight enabled from valid indexes, to disable please use -noindexheight");

                        /* Set indexing argument now. */
                        RECURSIVE(config::ARGS_MUTEX);
                        config::mapArgs["-indexheight"] = "1";
                    }
                    else
                    {
                        /* Check for disabled mode. */
                        if(!config::GetBoolArg("-indexheight"))
                            debug::warning(FUNCTION, "-indexheight disabled with valid indexes, to enable please remove -noindexheight");
                    }
                }
            }

            /* Print our best block to console. */
            tStateBest.load().print();

            /* Log the weights. */
            debug::log(0, FUNCTION, "WEIGHTS",
                " Prime ", tStateBest.load().nChannelWeight[1].Get64(),
                " Hash ",  tStateBest.load().nChannelWeight[2].Get64(),
                " Stake ", tStateBest.load().nChannelWeight[0].Get64());


            /* Debug logging. */
            debug::log(0, FUNCTION, config::fTestNet.load() ? "Test" : "Nexus", " Network: genesis=", Genesis().SubString(),
            " nBitsStart=0x", std::hex, bnProofOfWorkStart[0].GetCompact(), " best=", hashBestChain.load().SubString(),
            " checkpoint=", hashCheckpoint.load().SubString()," height=", std::dec, tStateBest.load().nHeight);

            return true;
        }


#ifdef UNIT_TESTS
        bool ChainState::RunHardcodedCheckpointRecoveryForTests(const std::map<uint32_t, uint1024_t>& mapCheckpointsTest,
                                                                const bool fAllowRepair)
        {
            return RecoverMissingHardcodedCheckpoint(mapCheckpointsTest, fAllowRepair);
        }

        bool ChainState::RunBestChainIntegrityAuditForTests(const bool fAllowRepair, const uint32_t nMaxDepth)
        {
            return RepairActiveChainHole(fAllowRepair, nMaxDepth);
        }


        void ChainState::SetCheckpointRepairSetBestHook(const std::function<bool(const BlockState&)>& fnHook)
        {
            fnCheckpointRepairSetBestHook = fnHook;
        }
#endif


        /* Get the hash of the genesis block. */
        uint1024_t ChainState::Genesis()
        {
            return (config::fHybrid.load() ? TAO::Ledger::hashGenesisHybrid : config::fTestNet.load() ? TAO::Ledger::hashGenesisTestnet : (config::fClient.load() ? TAO::Ledger::hashTritium : TAO::Ledger::hashGenesis));
        }


        /* Repair in-memory checkpoint state when it drifts from the on-disk best-chain state. */
        bool ChainState::RepairCheckpointIfStale()
        {
            /* In-memory gate: compare tStateBest.hashCheckpoint (the checkpoint embedded
             * in the best block state struct) against the standalone hashCheckpoint atomic.
             * If they are consistent there is nothing to repair — avoid disk I/O entirely.
             * This eliminates the I/O amplification DoS vector where a remote peer could
             * trigger repeated ReadBlock() calls by spamming blocks with bad checkpoints. */
            const uint1024_t hashMemCheckpoint  = ChainState::hashCheckpoint.load();
            const uint1024_t hashBestCheckpoint = ChainState::tStateBest.load().hashCheckpoint;

            if(hashMemCheckpoint == hashBestCheckpoint)
            {
                /* In-memory state is consistent — no repair needed. */
                return false;
            }

            /* In-memory mismatch detected: repair from on-disk best state. */
            debug::error(FUNCTION, "CHECKPOINT STALE: in-memory=", hashMemCheckpoint.SubString(),
                " tStateBest.hashCheckpoint=", hashBestCheckpoint.SubString(),
                " — repairing");

            /* Read the on-disk best-chain block to get the authoritative checkpoint hash. */
            BlockState stateBestDisk;
            if(!LLD::Ledger->ReadBlock(ChainState::hashBestChain.load(), stateBestDisk))
            {
                debug::error(FUNCTION, "repair failed: could not read best block from disk");
                return false;
            }

            /* Double-check disk matches in-memory tStateBest expectation. */
            if(stateBestDisk.hashCheckpoint != hashBestCheckpoint)
            {
                debug::error(FUNCTION, "repair aborted: disk checkpoint=",
                    stateBestDisk.hashCheckpoint.SubString(),
                    " does not match tStateBest.hashCheckpoint=", hashBestCheckpoint.SubString());
                return false;
            }

            /* Read the checkpoint block so we can update nCheckpointHeight. */
            const uint1024_t hashCheckpointOld = hashMemCheckpoint;
            ChainState::hashCheckpoint = stateBestDisk.hashCheckpoint;

            BlockState stateCheckpoint;
            if(!LLD::Ledger->ReadBlock(stateBestDisk.hashCheckpoint, stateCheckpoint))
            {
                /* Restore old checkpoint to avoid a partial update. */
                ChainState::hashCheckpoint = hashCheckpointOld;
                debug::error(FUNCTION, "repair failed: could not read checkpoint block");
                return false;
            }

            ChainState::nCheckpointHeight = stateCheckpoint.nHeight;

            debug::log(0, FUNCTION, "Checkpoint repair SUCCESS: hash=",
                ChainState::hashCheckpoint.load().SubString(),
                " height=", ChainState::nCheckpointHeight.load());

            return true;
        }
    }
}
