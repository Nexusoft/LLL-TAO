/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
__________________________________________________________________________________________*/

/** Unit tests for LLP::TxResponseWindow and LLP::CheckAndUseTxResponseWindow.
 *
 *  All tests operate on plain TxResponseWindow value types without any live
 *  networking, sockets, or blockchain state so they are fast and fully isolated.
 *  The helper CheckAndUseTxResponseWindow() is also exercised directly to cover
 *  the budget and expiry code paths that are gated behind the per-peer mutex in
 *  production code.
 *
 *  Test coverage:
 *   1.  Opening a GET window authorises multiple raw-tx payloads before the
 *       matching block arrives.
 *   2.  A separate window (different "session" — simulated via two independent
 *       TxResponseWindow instances) does NOT cross-authorise transactions.
 *   3.  Receiving the matching block closes the GET window; subsequent
 *       transactions become unsolicited again.
 *   4.  A window expires after its TTL and no longer authorises transactions.
 *   5.  The bounded tx budget prevents unlimited authorisation.
 *   6.  Disconnect / reset cleanup leaves the window inactive.
 *   7.  LIST windows stay open across multiple block arrivals and close on
 *       LASTINDEX (simulated as an explicit Close() call).
 *   8.  Non-sync subscription behaviour is unchanged: an inactive window
 *       correctly returns false and does not interfere with subscribed paths.
 **/

#include <LLP/include/tx_response_window.h>

#include <unit/catch2/catch.hpp>

using namespace LLP;


/* ────────────────────────────────────────────────────────────────────────────
 * 1.  GET window authorises multiple transactions before the block arrives.
 * ──────────────────────────────────────────────────────────────────────────── */
TEST_CASE("TxResponseWindow GET authorises multiple txs before block",
    "[llp][tx_response_window]")
{
    TxResponseWindow window;
    REQUIRE_FALSE(window.IsActive());

    /* Open a GET window at t=100. */
    window.Open(TxResponseKind::GET, 100, TX_RESPONSE_WINDOW_GET_TTL_SECONDS, TX_RESPONSE_WINDOW_GET_MAX_TX);
    REQUIRE(window.IsActive());
    REQUIRE(window.eKind == TxResponseKind::GET);
    REQUIRE(window.nTxCount == 0);

    /* First transaction — authorised. */
    REQUIRE(CheckAndUseTxResponseWindow(window, 101));
    REQUIRE(window.nTxCount == 1);
    REQUIRE(window.IsActive());

    /* Second transaction — still authorised. */
    REQUIRE(CheckAndUseTxResponseWindow(window, 102));
    REQUIRE(window.nTxCount == 2);

    /* Tenth transaction — still open; block hasn't arrived yet. */
    for(uint32_t i = 0; i < 8; ++i)
        REQUIRE(CheckAndUseTxResponseWindow(window, 103));

    REQUIRE(window.nTxCount == 10);
    REQUIRE(window.IsActive());
}


/* ────────────────────────────────────────────────────────────────────────────
 * 2.  A separate per-peer window does NOT cross-authorise transactions.
 * ──────────────────────────────────────────────────────────────────────────── */
TEST_CASE("TxResponseWindow session isolation: separate windows are independent",
    "[llp][tx_response_window]")
{
    /* peer A has an open GET window. */
    TxResponseWindow windowA;
    windowA.Open(TxResponseKind::GET, 100, TX_RESPONSE_WINDOW_GET_TTL_SECONDS, TX_RESPONSE_WINDOW_GET_MAX_TX);

    /* peer B has NO open window (simulates a different connection). */
    TxResponseWindow windowB;
    REQUIRE_FALSE(windowB.IsActive());

    /* Authorise through A — succeeds. */
    REQUIRE(CheckAndUseTxResponseWindow(windowA, 101));

    /* Attempt through B — fails: windowB is inactive. */
    REQUIRE_FALSE(CheckAndUseTxResponseWindow(windowB, 101));
    REQUIRE_FALSE(windowB.IsActive());
    REQUIRE(windowB.nTxCount == 0);

    /* A's state is unaffected by the B check. */
    REQUIRE(windowA.IsActive());
    REQUIRE(windowA.nTxCount == 1);
}


TEST_CASE("TxResponseWindow GET closes only for its requested block",
    "[llp][tx_response_window]")
{
    TxResponseWindow window;
    const uint1024_t hashRequested = 123;
    window.Open(TxResponseKind::GET, 100, TX_RESPONSE_WINDOW_GET_TTL_SECONDS,
        TX_RESPONSE_WINDOW_GET_MAX_TX, hashRequested);

    /* Some transactions arrive. */
    REQUIRE(CheckAndUseTxResponseWindow(window, 101));
    REQUIRE(CheckAndUseTxResponseWindow(window, 102));
    REQUIRE(window.nTxCount == 2);

    /* An unrelated block must not close the GET response window. */
    REQUIRE_FALSE(IsMatchingTxResponseBlock(window, uint1024_t(456)));
    REQUIRE(window.IsActive());

    /* The requested block closes the window. */
    REQUIRE(IsMatchingTxResponseBlock(window, hashRequested));
    window.Close();
    REQUIRE_FALSE(window.IsActive());
    REQUIRE(window.nTxCount == 2);

    /* Subsequent TYPES::TRANSACTION should now be treated as unsolicited. */
    REQUIRE_FALSE(CheckAndUseTxResponseWindow(window, 103));
    REQUIRE_FALSE(window.IsActive());
}


/* ────────────────────────────────────────────────────────────────────────────
 * 4.  Window expires after TTL; further txs are unsolicited.
 * ──────────────────────────────────────────────────────────────────────────── */
TEST_CASE("TxResponseWindow expires after TTL",
    "[llp][tx_response_window]")
{
    TxResponseWindow window;

    /* TTL = 30 seconds. */
    const uint64_t nTTL = TX_RESPONSE_WINDOW_GET_TTL_SECONDS;
    window.Open(TxResponseKind::GET, 1000, nTTL, TX_RESPONSE_WINDOW_GET_MAX_TX);

    /* Before expiry: authorised. */
    REQUIRE(CheckAndUseTxResponseWindow(window, 1010));
    REQUIRE(window.IsActive());

    /* Exactly at TTL boundary (not yet expired — uses strict >). */
    REQUIRE(CheckAndUseTxResponseWindow(window, 1000 + nTTL));
    REQUIRE(window.IsActive());

    /* One second past TTL: expired. */
    TxResponseCloseReason reason;
    const bool fAuth = CheckAndUseTxResponseWindow(window, 1000 + nTTL + 1, &reason);
    REQUIRE_FALSE(fAuth);
    REQUIRE(reason == TxResponseCloseReason::EXPIRED);
    REQUIRE_FALSE(window.IsActive());

    /* Window remains closed: subsequent check also returns false. */
    REQUIRE_FALSE(CheckAndUseTxResponseWindow(window, 1000 + nTTL + 2));
}


/* ────────────────────────────────────────────────────────────────────────────
 * 5.  Tx budget prevents unlimited authorisation.
 * ──────────────────────────────────────────────────────────────────────────── */
TEST_CASE("TxResponseWindow budget exhaustion closes window",
    "[llp][tx_response_window]")
{
    TxResponseWindow window;
    /* Use a small budget for the test. */
    const uint32_t nBudget = 5;
    window.Open(TxResponseKind::GET, 1000, TX_RESPONSE_WINDOW_GET_TTL_SECONDS, nBudget);

    /* Exhaust the budget exactly. */
    for(uint32_t i = 0; i < nBudget; ++i)
    {
        REQUIRE(CheckAndUseTxResponseWindow(window, 1001));
    }
    REQUIRE(window.nTxCount == nBudget);
    REQUIRE(window.IsActive());   /* window is active but budget is now ==nMaxTx */

    /* The NEXT transaction hits the budget check (nTxCount >= nMaxTx) and closes. */
    TxResponseCloseReason reason;
    const bool fAuth = CheckAndUseTxResponseWindow(window, 1002, &reason);
    REQUIRE_FALSE(fAuth);
    REQUIRE(reason == TxResponseCloseReason::BUDGET);
    REQUIRE_FALSE(window.IsActive());
}


/* ────────────────────────────────────────────────────────────────────────────
 * 6.  Disconnect / reset cleanup leaves window inactive.
 * ──────────────────────────────────────────────────────────────────────────── */
TEST_CASE("TxResponseWindow disconnect cleanup",
    "[llp][tx_response_window]")
{
    TxResponseWindow window;
    window.Open(TxResponseKind::LIST, 1000, TX_RESPONSE_WINDOW_LIST_TTL_SECONDS, TX_RESPONSE_WINDOW_LIST_MAX_TX);
    REQUIRE(window.IsActive());

    /* Simulate the disconnect handler. */
    const uint32_t nCountBeforeClose = window.nTxCount; /* 0 — no txs yet */
    window.Close();

    REQUIRE_FALSE(window.IsActive());
    REQUIRE(window.nTxCount == nCountBeforeClose);
    (void)nCountBeforeClose;  /* suppress unused-variable warning */

    /* Trying to authorise after disconnect: returns false. */
    REQUIRE_FALSE(CheckAndUseTxResponseWindow(window, 1001));
}


/* ────────────────────────────────────────────────────────────────────────────
 * 7.  LIST window spans multiple blocks; closes on LASTINDEX (simulated Close).
 * ──────────────────────────────────────────────────────────────────────────── */
TEST_CASE("TxResponseWindow LIST stays open across multiple blocks, closes on LASTINDEX",
    "[llp][tx_response_window]")
{
    TxResponseWindow window;
    window.Open(TxResponseKind::LIST, 1000, TX_RESPONSE_WINDOW_LIST_TTL_SECONDS, TX_RESPONSE_WINDOW_LIST_MAX_TX);
    REQUIRE(window.eKind == TxResponseKind::LIST);

    /* Block 1 transactions. */
    for(int i = 0; i < 5; ++i)
        REQUIRE(CheckAndUseTxResponseWindow(window, 1001));
    REQUIRE(window.nTxCount == 5);

    /* Block 1 arrives.  For LIST windows, production code does NOT close the
     * window on TYPES::BLOCK — only the GET branch does.  Verify the window
     * is still open after block 1. */
    REQUIRE(window.IsActive());

    /* Block 2 transactions — still authorised. */
    for(int i = 0; i < 3; ++i)
        REQUIRE(CheckAndUseTxResponseWindow(window, 1002));
    REQUIRE(window.nTxCount == 8);
    REQUIRE(window.IsActive());

    /* Block 2 arrives; LIST window remains open. */
    REQUIRE(window.IsActive());

    /* LASTINDEX arrives — production code calls window.Close() for LIST windows. */
    window.Close();
    REQUIRE_FALSE(window.IsActive());

    /* Transactions after LASTINDEX are unsolicited. */
    REQUIRE_FALSE(CheckAndUseTxResponseWindow(window, 1003));
}


/* ────────────────────────────────────────────────────────────────────────────
 * 8.  Inactive window returns false; no interference with subscribed paths.
 * ──────────────────────────────────────────────────────────────────────────── */
TEST_CASE("TxResponseWindow inactive window does not authorise, does not set reason",
    "[llp][tx_response_window]")
{
    TxResponseWindow window;
    /* Default-constructed window is inactive. */
    REQUIRE_FALSE(window.IsActive());
    REQUIRE(window.eKind == TxResponseKind::NONE);

    /* CheckAndUseTxResponseWindow returns false without touching pReason when
     * the window is inactive (i.e., the caller still needs to check its own
     * subscription flag — the window check is orthogonal). */
    TxResponseCloseReason reason = TxResponseCloseReason::MATCHING_BLOCK;
    const bool fAuth = CheckAndUseTxResponseWindow(window, 999, &reason);

    REQUIRE_FALSE(fAuth);
    /* reason should be unchanged because the inactive-window path returns
     * before setting *pReason. */
    REQUIRE(reason == TxResponseCloseReason::MATCHING_BLOCK);

    /* No side-effects: window stays inactive. */
    REQUIRE_FALSE(window.IsActive());
    REQUIRE(window.nTxCount == 0);
}


/* ────────────────────────────────────────────────────────────────────────────
 * 9.  LIST window respects its larger budget and TTL constants.
 * ──────────────────────────────────────────────────────────────────────────── */
TEST_CASE("TxResponseWindow LIST constants are larger than GET constants",
    "[llp][tx_response_window]")
{
    REQUIRE(TX_RESPONSE_WINDOW_LIST_TTL_SECONDS > TX_RESPONSE_WINDOW_GET_TTL_SECONDS);
    REQUIRE(TX_RESPONSE_WINDOW_LIST_MAX_TX      > TX_RESPONSE_WINDOW_GET_MAX_TX);
}


TEST_CASE("TxResponseWindow request opening and rollback preserve newer request",
    "[llp][tx_response_window]")
{
    TxResponseWindow window;
    const uint1024_t hashFirst = 111;
    const uint64_t nFirstRequest = window.Open(TxResponseKind::GET, 500,
        TX_RESPONSE_WINDOW_GET_TTL_SECONDS, TX_RESPONSE_WINDOW_GET_MAX_TX, hashFirst);

    /* Consume some budget. */
    REQUIRE(CheckAndUseTxResponseWindow(window, 501));
    REQUIRE(CheckAndUseTxResponseWindow(window, 502));
    REQUIRE(window.nTxCount == 2);

    /* A second request deliberately supersedes the first before it is queued. */
    const uint1024_t hashTarget = 222;
    const uint1024_t hashStop = 333;
    const uint64_t nSecondRequest = window.Open(TxResponseKind::LIST, 600,
        TX_RESPONSE_WINDOW_LIST_TTL_SECONDS, TX_RESPONSE_WINDOW_LIST_MAX_TX, hashTarget, hashStop);
    REQUIRE(window.IsActive());
    REQUIRE(window.eKind == TxResponseKind::LIST);
    REQUIRE(window.nOpenedAt == 600);
    REQUIRE(window.nTxCount == 0);   /* counter reset */
    REQUIRE(window.nMaxTx == TX_RESPONSE_WINDOW_LIST_MAX_TX);
    REQUIRE(window.nTTL   == TX_RESPONSE_WINDOW_LIST_TTL_SECONDS);
    REQUIRE(window.hashTarget == hashTarget);
    REQUIRE(window.hashStop == hashStop);

    /* A failed send for the superseded request cannot roll back the newer one. */
    REQUIRE_FALSE(RollbackTxResponseWindow(window, nFirstRequest));
    REQUIRE(window.IsActive());
    REQUIRE(window.nRequestId == nSecondRequest);

    /* A failed send for the active request rolls it back. */
    REQUIRE(RollbackTxResponseWindow(window, nSecondRequest));
    REQUIRE_FALSE(window.IsActive());
    REQUIRE(window.nTxCount == 0);
}
