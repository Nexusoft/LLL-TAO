/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_ADMISSIBILITY_H
#define NEXUS_TAO_LEDGER_INCLUDE_ADMISSIBILITY_H

#include <string>
#include <cstdint>

/* Global TAO namespace. */
namespace TAO
{
    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** AdmissibilityClass
         *
         *  Classifies why a transaction or block is considered invalid so that
         *  callers can distinguish permanent failures from local-state-dependent
         *  ones.  This is the key distinction that breaks stranded-state loops:
         *  code that compares against local chain state must not treat a
         *  local-state failure as a permanent rejection.
         *
         *  Rule: nDepth (sigchain staleness depth from ComputeForkDivergence)
         *  MUST NOT be used as input to any classification decision.  A
         *  long-idle sigchain on the main chain is DEFERRED_LOCAL_STATE, not
         *  INVALID_ABSOLUTE, regardless of how large nDepth is.
         *
         **/
        enum class AdmissibilityClass
        {
            VALID,                /* passes all checks — proceed normally */
            INVALID_ABSOLUTE,     /* fails regardless of chain state — reject, penalise peer */
            DEFERRED_LOCAL_STATE, /* would pass at network height — retain and retry, do not reject */
            UNKNOWN               /* insufficient data to classify */
        };


        /** AdmissibilityResult
         *
         *  Carries an AdmissibilityClass, a human-readable reason, and an
         *  optional diagnostic depth field.  The depth is labelled diagnostic-
         *  only and MUST NOT be used to drive eviction or rejection decisions.
         *
         **/
        struct AdmissibilityResult
        {
            AdmissibilityClass nClass   = AdmissibilityClass::UNKNOWN;
            std::string        strReason;

            /** Diagnostic-only: number of blocks the ancestor block is behind
             *  the best chain tip.  This measures sigchain staleness, NOT fork
             *  divergence.  A large value indicates an idle sigchain on the
             *  main chain — it must NEVER trigger eviction or rejection. **/
            uint32_t nDiagnosticDepth   = 0;
        };


        /* ---------------------------------------------------------------
         * Thread-local side channel
         *
         * Transaction::Connect() is a const method with a bool return type.
         * When it classifies a failure as DEFERRED_LOCAL_STATE it sets this
         * thread-local so Mempool::Accept() can check the classification
         * without a signature change.  The value is reset to UNKNOWN on
         * every read so stale classifications do not persist across calls.
         * --------------------------------------------------------------- */

        /** g_nLastConnectClass
         *
         *  Per-thread classification set by the last Transaction::Connect()
         *  call that returned false.  Reset to UNKNOWN by TakeLastConnectClass().
         *
         **/
        inline thread_local AdmissibilityClass g_nLastConnectClass = AdmissibilityClass::UNKNOWN;


        /** SetLastConnectClass
         *
         *  Called from Transaction::Connect() when it classifies a failure.
         *
         *  @param[in] nClass  The classification to store.
         *
         **/
        inline void SetLastConnectClass(const AdmissibilityClass nClass)
        {
            g_nLastConnectClass = nClass;
        }


        /** TakeLastConnectClass
         *
         *  Read and reset the thread-local classification.  Should be called
         *  by Mempool::Accept() immediately after a failed Connect() to
         *  decide whether to defer or permanently reject the transaction.
         *
         *  @return The classification set by the last Connect() failure, or
         *          UNKNOWN if Connect() did not set one.
         *
         **/
        inline AdmissibilityClass TakeLastConnectClass()
        {
            const AdmissibilityClass cls = g_nLastConnectClass;
            g_nLastConnectClass = AdmissibilityClass::UNKNOWN;
            return cls;
        }

    } // namespace Ledger
} // namespace TAO

#endif // NEXUS_TAO_LEDGER_INCLUDE_ADMISSIBILITY_H
