/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_INCLUDE_GLOBAL_H
#define NEXUS_LLD_INCLUDE_GLOBAL_H

#include <LLD/types/register.h>
#include <LLD/types/ledger.h>
#include <LLD/types/local.h>
#include <LLD/types/logical.h>
#include <LLD/types/client.h>
#include <LLD/types/legacy.h>
#include <LLD/types/session.h>
#include <LLD/types/trust.h>
#include <LLD/types/contract.h>

namespace LLD
{
    extern LogicalDB*    Logical;
    extern SessionDB*    Sessions;
    extern ContractDB*   Contract;
    extern RegisterDB*   Register;
    extern LedgerDB*     Ledger;
    extern LocalDB*      Local;
    extern ClientDB*     Client;

    //for legacy objects
    extern TrustDB*      Trust;
    extern LegacyDB*     Legacy;

    /** Use this to track database instances to use with our ACID transactions. **/
    struct INSTANCES
    {
        enum : uint16_t
        {
            LOGICAL  = (1 << 1),
            CONTRACT = (1 << 2),
            REGISTER = (1 << 3),
            LEDGER   = (1 << 4),
            LOCAL    = (1 << 5),
            CLIENT   = (1 << 6),
            TRUST    = (1 << 7),
            LEGACY   = (1 << 8),

            //combined values
            CONSENSUS = (CONTRACT | REGISTER | LEDGER | TRUST | LEGACY),
            MERKLE    = (CONTRACT | REGISTER | CLIENT | LOGICAL),
            MEMORY    = (CONTRACT | REGISTER | LEDGER),
        };
    };


    /** Initialize
     *
     *  Initialize the global LLD instances.
     *
     **/
    void Initialize();


    /** Indexing
     *
     *  Run our indexing entries and routines.
     *
     **/
    void Indexing();


    /** Shutdown
     *
     *  Shutdown and cleanup the global LLD instances.
     *
     *  Rejects new transactions, waits until in-flight transaction calls
     *  (including HasOpenTransaction) finish and every ownership slot is
     *  released, then deletes the instances.
     *
     **/
    void Shutdown();


    /** TxnRecover
     *
     *  Check the transactions for recovery.
     *
     **/
    void TxnRecovery();


    /** HasOpenTransaction
     *
     *  Check if a physical database transaction is currently open on any
     *  instance that would be touched by TxnBegin(nFlags, nInstances).
     *  Returns false for memory-only flag modes (MEMPOOL, MINER, SANITIZE).
     *
     *  This is process-wide journal state. It does not mean the calling thread
     *  owns the transaction. Use ThreadOwnsTransaction for that.
     *
     *  The query holds the same shutdown lifetime barrier as TxnBegin,
     *  TxnCommit, and TxnAbort. After the instances are closed it returns
     *  false and does not touch them.
     *
     */
    bool HasOpenTransaction(const uint8_t nFlags = 0, const uint16_t nInstances = INSTANCES::CONSENSUS);


    /** ThreadOwnsTransaction
     *
     *  True when this thread owns the process-wide slot for nFlags.
     *  A foreign or exited owner is not this thread, even if a journal is open.
     *
     */
    bool ThreadOwnsTransaction(const uint8_t nFlags = 0);


    /** ThreadOwnsTransaction
     *
     *  True when this thread owns the process-wide slot for nFlags and the
     *  transaction includes every requested instance.
     *
     */
    bool ThreadOwnsTransaction(const uint8_t nFlags, const uint16_t nInstances);


    /** Txn Begin
     *
     *  Global handler for all LLD instances.
     *
     *  @return false if this begin was rejected because the requested mode, or
     *          the overlay it shares, is already owned, because a selected
     *          physical journal is already open without an owner, or because LLD
     *          is shutting down. Callers must not continue into writes, abort,
     *          or commit of that mode. If the slot is claimed and MemoryBegin or
     *          a selected journal TxnBegin then throws, already-started instances
     *          are released and the slot is cleared before the exception
     *          propagates. A caller catch cannot see a begin that did not return.
     *
     */
    bool TxnBegin(const uint8_t nFlags = 0, const uint16_t nInstances = INSTANCES::CONSENSUS);


    /** Txn Abort
     *
     *  Global handler for all LLD instances.
     *
     *  Owned memory and physical aborts reject masks that omit an instance
     *  opened by their matching begin, then release only the begin mask.
     *  An unowned physical abort uses nInstances for recovery.
     *  An abort is rejected when this thread does not own the requested mode.
     *
     */
    void TxnAbort(const uint8_t nFlags = 0, const uint16_t nInstances = INSTANCES::CONSENSUS);


    /** Txn Commit
     *
     *  Global handler for all LLD instances.
     *
     *  @return true if every owned per-DB commit succeeded, false if any failed.
     *          All selected instances are attempted regardless of individual failures.
     *          Owned MEMPOOL and physical commits reject masks that omit an
     *          instance opened by their matching begin and operate only on the
     *          begin mask. Commits are rejected when this thread does not own
     *          the requested mode.
     *
     */
    bool TxnCommit(const uint8_t nFlags = 0, const uint16_t nInstances = INSTANCES::CONSENSUS);


    /** TransactionGuard
     *
     *  Begins a transaction and aborts it on destruction if this thread still
     *  owns the slot. Release() after a successful commit so a later failure
     *  does not abort a transaction this guard no longer owns.
     *
     */
    class TransactionGuard
    {
        uint8_t nFlags;
        uint16_t nInstances;
        bool fArmed;

    public:
        TransactionGuard(const uint8_t nFlagsIn = 0, const uint16_t nInstancesIn = INSTANCES::CONSENSUS)
        : nFlags(nFlagsIn)
        , nInstances(nInstancesIn)
        , fArmed(TxnBegin(nFlagsIn, nInstancesIn))
        {
        }

        ~TransactionGuard()
        {
            if(fArmed && ThreadOwnsTransaction(nFlags))
                TxnAbort(nFlags, nInstances);
        }

        TransactionGuard(const TransactionGuard&) = delete;
        TransactionGuard& operator=(const TransactionGuard&) = delete;

        explicit operator bool() const
        {
            return fArmed;
        }

        void Release()
        {
            fArmed = false;
        }
    };
}

#endif
