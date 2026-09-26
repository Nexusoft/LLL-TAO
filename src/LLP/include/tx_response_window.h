/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
__________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_TX_RESPONSE_WINDOW_H
#define NEXUS_LLP_INCLUDE_TX_RESPONSE_WINDOW_H

#include <cstdint>

#include <LLC/types/uint1024.h>

namespace LLP
{
    /** Default TTL (seconds) for a GET SPECIFIER::TRANSACTIONS response window.
     *
     *  Single-block recovery: the peer sends all of the block's inline
     *  transactions then the block itself.  30 seconds is well above any
     *  realistic per-block send latency and tight enough to bound the
     *  permission window against a slow/misbehaving peer. **/
    inline constexpr uint64_t TX_RESPONSE_WINDOW_GET_TTL_SECONDS = 30;

    /** Default TTL (seconds) for a LIST SPECIFIER::TRANSACTIONS response window.
     *
     *  Branch-recovery LIST responses can span a sequence of blocks
     *  (transactions + block, transactions + block, …, LASTINDEX).  120 s
     *  gives a generous budget for multi-block batches while still bounding
     *  the open-window time against a peer that never sends LASTINDEX. **/
    inline constexpr uint64_t TX_RESPONSE_WINDOW_LIST_TTL_SECONDS = 120;

    /** Maximum raw TYPES::TRANSACTION messages allowed through a single
     *  GET SPECIFIER::TRANSACTIONS response window.
     *
     *  A single block contains at most a few thousand transactions in the
     *  largest realistic cases.  2000 is safely above that while preventing
     *  a peer from exploiting an open GET window to deliver unbounded traffic. **/
    inline constexpr uint32_t TX_RESPONSE_WINDOW_GET_MAX_TX = 2000;

    /** Maximum raw TYPES::TRANSACTION messages allowed through a single
     *  LIST SPECIFIER::TRANSACTIONS response window.
     *
     *  A branch-recovery LIST can span multiple blocks; 10 000 covers realistic
     *  multi-block ranges while remaining a hard cap against flooding. **/
    inline constexpr uint32_t TX_RESPONSE_WINDOW_LIST_MAX_TX = 10000;


    /** Kind of request that opened a TxResponseWindow. **/
    enum class TxResponseKind : uint8_t
    {
        NONE = 0, ///< No window is open.
        GET  = 1, ///< ACTION::GET SPECIFIER::TRANSACTIONS TYPES::BLOCK
        LIST = 2, ///< ACTION::LIST SPECIFIER::TRANSACTIONS TYPES::BLOCK
    };

    /** Reason a TxResponseWindow was closed.  Used in lifecycle log messages. **/
    enum class TxResponseCloseReason : uint8_t
    {
        MATCHING_BLOCK = 0, ///< GET: the requested block was received.
        LASTINDEX      = 1, ///< LIST: LASTINDEX notification received.
        EXPIRED        = 2, ///< TTL elapsed before the response completed.
        BUDGET         = 3, ///< Per-window tx-count budget exhausted.
        DISCONNECT     = 4, ///< Peer connection was closed.
        SEND_FAILED    = 5, ///< The request could not be queued.
    };


    /** @struct TxResponseWindow
     *
     *  Per-peer bounded response context that authorises raw TYPES::TRANSACTION
     *  messages when they arrive as part of a locally-initiated
     *  SPECIFIER::TRANSACTIONS block response from the same peer.
     *
     *  Design invariants:
     *  - Opened when this node sends ACTION::GET or ACTION::LIST with
     *    SPECIFIER::TRANSACTIONS on a given peer connection.
     *  - Closed when the corresponding block (GET) or LASTINDEX (LIST)
     *    arrives, when the TTL expires, when the tx-count budget is exhausted,
     *    or when the connection is torn down.
     *  - Only one window is tracked per peer connection.  A second open() call
     *    replaces any existing window (safe: the new request supersedes the old).
     *  - Designed as a plain value type with no blockchain-type dependencies
     *    so it can be exercised by unit tests without live sockets or globals.
     *
     *  Thread safety: external callers must hold a mutex whenever this struct
     *  is accessed from more than one thread (see TritiumNode::m_txRespWindow
     *  and TritiumNode::m_txRespWindowMutex). **/
    struct TxResponseWindow
    {
        TxResponseKind eKind    = TxResponseKind::NONE;
        uint64_t       nOpenedAt = 0;    ///< Unix timestamp (seconds) when opened.
        uint64_t       nTTL      = 0;    ///< TTL in seconds.
        uint32_t       nTxCount  = 0;    ///< Transactions accepted through this window so far.
        uint32_t       nMaxTx    = 0;    ///< Safety budget (max tx allowed).
        uint64_t       nRequestId = 0;   ///< Monotonically increasing request identifier.
        uint1024_t     hashTarget = 0;   ///< GET block hash or LIST locator target.
        uint1024_t     hashStop   = 0;   ///< LIST stop hash (zero for GET).


        /** IsActive — returns true if the window is open. **/
        bool IsActive() const noexcept
        {
            return eKind != TxResponseKind::NONE;
        }

        /** IsExpired — returns true if the TTL has elapsed. **/
        bool IsExpired(const uint64_t nNow) const noexcept
        {
            return IsActive() && nNow > nOpenedAt + nTTL;
        }

        /** IsBudgetExhausted — returns true when the tx-count limit is reached. **/
        bool IsBudgetExhausted() const noexcept
        {
            return IsActive() && nTxCount >= nMaxTx;
        }

        /** Open — initialise the window for a new request.
         *
         *  Replaces any previously active window.
         *
         *  @param[in] eKindIn   GET or LIST.
         *  @param[in] nNowIn    Current unix timestamp (seconds).
         *  @param[in] nTTLIn    TTL in seconds.
         *  @param[in] nMaxTxIn  Tx-count budget. **/
        uint64_t Open(const TxResponseKind eKindIn,
                  const uint64_t       nNowIn,
                  const uint64_t       nTTLIn,
                  const uint32_t       nMaxTxIn,
                  const uint1024_t&    hashTargetIn = 0,
                  const uint1024_t&    hashStopIn = 0) noexcept
        {
            eKind    = eKindIn;
            nOpenedAt = nNowIn;
            nTTL     = nTTLIn;
            nTxCount = 0;
            nMaxTx   = nMaxTxIn;
            ++nRequestId;
            hashTarget = hashTargetIn;
            hashStop   = hashStopIn;
            return nRequestId;
        }

        /** Close — deactivate the window while retaining request diagnostics. **/
        void Close() noexcept
        {
            eKind    = TxResponseKind::NONE;
        }
    };

    /** Returns true when a GET window belongs to the received block. **/
    inline bool IsMatchingTxResponseBlock(const TxResponseWindow& window,
                                          const uint1024_t& hashBlock) noexcept
    {
        return window.eKind == TxResponseKind::GET
            && window.hashTarget == hashBlock;
    }

    /** Roll back a just-opened window only if no newer request superseded it. **/
    inline bool RollbackTxResponseWindow(TxResponseWindow& window,
                                         const uint64_t nRequestId) noexcept
    {
        if(window.IsActive() && window.nRequestId == nRequestId)
        {
            window.Close();
            return true;
        }

        return false;
    }


    /** CheckAndUseTxResponseWindow
     *
     *  Check whether an incoming raw TYPES::TRANSACTION is authorised by an
     *  active response window.
     *
     *  If the window is active, not yet expired, and still within budget,
     *  increments nTxCount and returns true (the packet is permitted).
     *
     *  If the window is expired or budget-exhausted, closes the window and
     *  returns false; *pReason is set to the applicable close reason when
     *  pReason is non-null.
     *
     *  If no window is active, returns false immediately (pReason is not set).
     *
     *  This helper is free-standing so it can be exercised by unit tests without
     *  a live TritiumNode instance.
     *
     *  @param[in,out] window   The per-peer window to check and update.
     *  @param[in]     nNow     Current unix timestamp (seconds).
     *  @param[out]    pReason  Optional: set on false-return when window was active.
     *
     *  @return true if the transaction is authorised. **/
    inline bool CheckAndUseTxResponseWindow(TxResponseWindow&    window,
                                            const uint64_t       nNow,
                                            TxResponseCloseReason* pReason = nullptr)
    {
        if(!window.IsActive())
            return false;

        if(window.IsExpired(nNow))
        {
            window.Close();
            if(pReason) *pReason = TxResponseCloseReason::EXPIRED;
            return false;
        }

        if(window.IsBudgetExhausted())
        {
            window.Close();
            if(pReason) *pReason = TxResponseCloseReason::BUDGET;
            return false;
        }

        ++window.nTxCount;
        return true;
    }
}

#endif
