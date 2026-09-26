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

#ifdef UNIT_TESTS
#include <functional>
#endif

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

    /** Reserve the coordinator before locks used by transaction callbacks.
     *  Recursive reservations do not open or change transaction ownership. */
    class TransactionCoordinatorGuard
    {
    public:
        TransactionCoordinatorGuard();
        ~TransactionCoordinatorGuard();
        TransactionCoordinatorGuard(const TransactionCoordinatorGuard&) = delete;
        TransactionCoordinatorGuard& operator=(const TransactionCoordinatorGuard&) = delete;
    };

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
     *  @return True if initialization and journal recovery succeeded.
     *
     **/
    bool Initialize();


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
     **/
    void Shutdown();


    /** TxnRecover
     *
     *  Check the transactions for recovery.
     *
     *  @return True if no recovery was needed or every recovered database
     *          committed successfully.
     *
     **/
    bool TxnRecovery();


    /** HasOpenTransaction
     *
     *  Check if a physical database transaction is currently open on any
     *  instance that would be touched by TxnBegin(nFlags, nInstances).
     *  Returns false for memory-only flag modes (MEMPOOL, MINER, SANITIZE).
     *
     */
    bool HasOpenTransaction(const uint8_t nFlags = 0, const uint16_t nInstances = INSTANCES::CONSENSUS);


    /** Txn Begin
     *
     *  Global handler for all LLD instances.
     *
     */
    bool TxnBegin(const uint8_t nFlags = 0, const uint16_t nInstances = INSTANCES::CONSENSUS);


    /** Txn Abort
     *
     *  Global handler for all LLD instances.
     *
     */
    bool TxnAbort(const uint8_t nFlags = 0, const uint16_t nInstances = INSTANCES::CONSENSUS);


    /** Txn Commit
     *
     *  Global handler for all LLD instances.
     *
     *  @return True if every selected checkpoint and per-DB commit succeeded.
     *
     */
    bool TxnCommit(const uint8_t nFlags = 0, const uint16_t nInstances = INSTANCES::CONSENSUS);


    /** Abort a transaction automatically if its scope exits before it is consumed. */
    class TransactionGuard
    {
    public:
        TransactionGuard(const uint8_t nFlags = 0, const uint16_t nInstances = INSTANCES::CONSENSUS);
        ~TransactionGuard();

        TransactionGuard(const TransactionGuard&) = delete;
        TransactionGuard& operator=(const TransactionGuard&) = delete;

        explicit operator bool() const;

    private:
        const uint8_t nFlags;
        const uint16_t nInstances;
        const bool fAcquired;
    };


    #ifdef UNIT_TESTS
    /** Install a test hook invoked after the coordinator is observed locked. */
    void SetTxnCoordinatorWaitHook(const std::function<void()>& fnHook);

    /** Clear the recovery-required latch so unit tests can continue after a
     *  forced partial-apply failure. */
    void ResetTxnRecoveryRequired();
    #endif
}

#endif
