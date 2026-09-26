#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_SYNC_PROFILE_H
#define NEXUS_TAO_LEDGER_INCLUDE_SYNC_PROFILE_H

#include <LLC/types/uint1024.h>

#include <cstdint>

namespace TAO
{
    namespace Ledger
    {
        namespace SyncProfile
        {
            struct Snapshot
            {
                uint64_t nBlocksReceived        = 0;
                uint64_t nBlocksAccepted        = 0;
                uint64_t nBlocksRejected        = 0;
                uint64_t nBlocksOrphaned        = 0;

                uint64_t nListBatches           = 0;
                uint64_t nListBlocks            = 0;
                uint64_t nListMaxBlocks         = 0;
                uint64_t nBufferPressureCuts    = 0;
                uint64_t nRepeatedLastIndex     = 0;
                uint1024_t hashLastIndex        = 0;

                uint64_t nTxnCoordinatorWaitUs  = 0;
                uint64_t nTxnOpenedParticipants = 0;
                uint64_t nTxnTouchedParticipants= 0;
                uint64_t nTxnCheckpointParticipants = 0;
                uint64_t nTxnApplyParticipants  = 0;
                uint64_t nTxnReleaseParticipants= 0;

                uint64_t nTxnCheckpointUs       = 0;
                uint64_t nTxnCheckpointFsyncUs  = 0;
                uint64_t nTxnApplyUs            = 0;
                uint64_t nTxnApplyFsyncUs       = 0;
                uint64_t nTxnReleaseUs          = 0;
                uint64_t nTxnReleaseFsyncUs     = 0;

                uint64_t nIndexUs               = 0;
                uint64_t nConnectUs             = 0;
                uint64_t nSetBestUs             = 0;
            };

            bool Enabled();

            void RecordBlockReceived();
            void RecordProcessStatus(uint8_t nStatus);
            void RecordListBatch(uint32_t nBlocksSent, bool fBufferCut, const uint1024_t& hashLastIndex);

            void RecordTxnCoordinatorWait(uint64_t nElapsedUs);
            void RecordTxnParticipants(uint32_t nOpenedParticipants, uint32_t nTouchedParticipants);
            void RecordTxnCheckpoint(uint32_t nParticipants, uint64_t nElapsedUs, uint64_t nFsyncUs);
            void RecordTxnApply(uint32_t nParticipants, uint64_t nElapsedUs, uint64_t nFsyncUs);
            void RecordTxnRelease(uint32_t nParticipants, uint64_t nElapsedUs, uint64_t nFsyncUs);

            void RecordIndexTime(uint64_t nElapsedUs);
            void RecordConnectTime(uint64_t nElapsedUs);
            void RecordSetBestTime(uint64_t nElapsedUs);

            #ifdef UNIT_TESTS
            Snapshot GetSnapshot();
            void Reset();
            #endif
        }
    }
}

#endif
