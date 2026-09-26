/*__________________________________________________________________________________________

        Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

        (c) Copyright The Nexus Developers 2014 - 2026

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#include <TAO/Ledger/include/sync_profile.h>

#include <TAO/Ledger/include/process.h>

#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <algorithm>
#include <functional>
#include <mutex>

namespace TAO
{
    namespace Ledger
    {
        namespace SyncProfile
        {
            namespace
            {
                std::mutex PROFILE_MUTEX;
                Snapshot STATS;
                uint64_t nFirstSample = 0;
                uint64_t nLastReport  = 0;


                uint64_t ReportIntervalSeconds()
                {
                    if(config::HasArg("-syncprofile") && config::GetArg("-syncprofile", "").empty())
                        return 10;

                    const int64_t nInterval = config::GetArg("-syncprofile", 0);
                    return (nInterval <= 0 ? 0 : static_cast<uint64_t>(std::max<int64_t>(1, nInterval)));
                }


                void MaybeReportLocked()
                {
                    const uint64_t nInterval = ReportIntervalSeconds();
                    if(nInterval == 0)
                        return;

                    const uint64_t nNow = runtime::timestamp();
                    if(nFirstSample == 0)
                        nFirstSample = nNow;

                    if(nLastReport != 0 && (nNow - nLastReport) < nInterval)
                        return;

                    const double nElapsed = std::max<uint64_t>(1, nNow - nFirstSample);
                    const double nBlocksPerSecond =
                        static_cast<double>(STATS.nBlocksAccepted) / nElapsed;
                    const double nAverageBatch =
                        STATS.nListBatches == 0 ? 0.0 :
                        static_cast<double>(STATS.nListBlocks) / static_cast<double>(STATS.nListBatches);

                    debug::log(0, "SYNCPROFILE:",
                        " received=", STATS.nBlocksReceived,
                        " accepted=", STATS.nBlocksAccepted,
                        " rejected=", STATS.nBlocksRejected,
                        " orphaned=", STATS.nBlocksOrphaned,
                        " bps=", nBlocksPerSecond,
                        " list_batches=", STATS.nListBatches,
                        " lastindex=", STATS.hashLastIndex.SubString(),
                        " avg_batch=", nAverageBatch,
                        " max_batch=", STATS.nListMaxBlocks,
                        " buffer_cuts=", STATS.nBufferPressureCuts,
                        " repeated_lastindex=", STATS.nRepeatedLastIndex,
                        " txn_wait_us=", STATS.nTxnCoordinatorWaitUs,
                        " participants_opened=", STATS.nTxnOpenedParticipants,
                        " participants_touched=", STATS.nTxnTouchedParticipants,
                        " checkpoint_participants=", STATS.nTxnCheckpointParticipants,
                        " checkpoint_us=", STATS.nTxnCheckpointUs,
                        " checkpoint_fsync_us=", STATS.nTxnCheckpointFsyncUs,
                        " apply_participants=", STATS.nTxnApplyParticipants,
                        " apply_us=", STATS.nTxnApplyUs,
                        " apply_fsync_us=", STATS.nTxnApplyFsyncUs,
                        " release_participants=", STATS.nTxnReleaseParticipants,
                        " release_us=", STATS.nTxnReleaseUs,
                        " release_fsync_us=", STATS.nTxnReleaseFsyncUs,
                        " index_us=", STATS.nIndexUs,
                        " connect_us=", STATS.nConnectUs,
                        " setbest_us=", STATS.nSetBestUs);

                    nLastReport = nNow;
                }


                void Update(const std::function<void(Snapshot&)>& fnUpdate)
                {
                    if(!Enabled())
                        return;

                    std::lock_guard<std::mutex> lock(PROFILE_MUTEX);
                    fnUpdate(STATS);
                    MaybeReportLocked();
                }
            }


            bool Enabled()
            {
                return ReportIntervalSeconds() != 0;
            }


            void RecordBlockReceived()
            {
                Update([](Snapshot& stats)
                {
                    ++stats.nBlocksReceived;
                });
            }


            void RecordProcessStatus(const uint8_t nStatus)
            {
                Update([nStatus](Snapshot& stats)
                {
                    if(nStatus & PROCESS::ACCEPTED)
                        ++stats.nBlocksAccepted;
                    else if(nStatus & PROCESS::REJECTED)
                        ++stats.nBlocksRejected;
                    else if(nStatus & PROCESS::ORPHAN)
                        ++stats.nBlocksOrphaned;
                });
            }


            void RecordListBatch(const uint32_t nBlocksSent, const bool fBufferCut, const uint1024_t& hashLastIndex)
            {
                Update([nBlocksSent, fBufferCut, &hashLastIndex](Snapshot& stats)
                {
                    ++stats.nListBatches;
                    stats.nListBlocks += nBlocksSent;
                    stats.nListMaxBlocks = std::max<uint64_t>(stats.nListMaxBlocks, nBlocksSent);

                    if(fBufferCut)
                        ++stats.nBufferPressureCuts;

                    if(stats.hashLastIndex != 0 && stats.hashLastIndex == hashLastIndex)
                        ++stats.nRepeatedLastIndex;

                    stats.hashLastIndex = hashLastIndex;
                });
            }


            void RecordTxnCoordinatorWait(const uint64_t nElapsedUs)
            {
                Update([nElapsedUs](Snapshot& stats)
                {
                    stats.nTxnCoordinatorWaitUs += nElapsedUs;
                });
            }


            void RecordTxnParticipants(const uint32_t nOpenedParticipants, const uint32_t nTouchedParticipants)
            {
                Update([nOpenedParticipants, nTouchedParticipants](Snapshot& stats)
                {
                    stats.nTxnOpenedParticipants += nOpenedParticipants;
                    stats.nTxnTouchedParticipants += nTouchedParticipants;
                });
            }


            void RecordTxnCheckpoint(const uint32_t nParticipants, const uint64_t nElapsedUs, const uint64_t nFsyncUs)
            {
                Update([nParticipants, nElapsedUs, nFsyncUs](Snapshot& stats)
                {
                    stats.nTxnCheckpointParticipants += nParticipants;
                    stats.nTxnCheckpointUs += nElapsedUs;
                    stats.nTxnCheckpointFsyncUs += nFsyncUs;
                });
            }


            void RecordTxnApply(const uint32_t nParticipants, const uint64_t nElapsedUs, const uint64_t nFsyncUs)
            {
                Update([nParticipants, nElapsedUs, nFsyncUs](Snapshot& stats)
                {
                    stats.nTxnApplyParticipants += nParticipants;
                    stats.nTxnApplyUs += nElapsedUs;
                    stats.nTxnApplyFsyncUs += nFsyncUs;
                });
            }


            void RecordTxnRelease(const uint32_t nParticipants, const uint64_t nElapsedUs, const uint64_t nFsyncUs)
            {
                Update([nParticipants, nElapsedUs, nFsyncUs](Snapshot& stats)
                {
                    stats.nTxnReleaseParticipants += nParticipants;
                    stats.nTxnReleaseUs += nElapsedUs;
                    stats.nTxnReleaseFsyncUs += nFsyncUs;
                });
            }


            void RecordIndexTime(const uint64_t nElapsedUs)
            {
                Update([nElapsedUs](Snapshot& stats)
                {
                    stats.nIndexUs += nElapsedUs;
                });
            }


            void RecordConnectTime(const uint64_t nElapsedUs)
            {
                Update([nElapsedUs](Snapshot& stats)
                {
                    stats.nConnectUs += nElapsedUs;
                });
            }


            void RecordSetBestTime(const uint64_t nElapsedUs)
            {
                Update([nElapsedUs](Snapshot& stats)
                {
                    stats.nSetBestUs += nElapsedUs;
                });
            }


            #ifdef UNIT_TESTS
            Snapshot GetSnapshot()
            {
                std::lock_guard<std::mutex> lock(PROFILE_MUTEX);
                return STATS;
            }


            void Reset()
            {
                std::lock_guard<std::mutex> lock(PROFILE_MUTEX);
                STATS = Snapshot();
                nFirstSample = 0;
                nLastReport = 0;
            }
            #endif
        }
    }
}
