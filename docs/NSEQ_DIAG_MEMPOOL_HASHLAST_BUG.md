# CreateTransaction Mempool hashLast Desync Bug — Root Cause Analysis & Fix

## Overview

A critical one-line bug in `CreateTransaction()` in `src/TAO/Ledger/create.cpp` caused the
producer transaction's `hashPrevTx` to point to the **wrong previous transaction** whenever the
mempool contained a more recent sigchain transaction than what was on disk or in Sessions.  The
identical bug exists in upstream `Nexusoft/LLL-TAO` at commit
`ff33dea5fc4e9dc3bf325c059278d6938934bfb9` (lines 84-99 of the same file).

This was the last remaining bug blocking full end-to-end Stateless Mining reward claims.

---

## Problem Statement

`CreateTransaction()` resolves the "last sigchain transaction" from up to three sources and must
keep two pieces of state in sync:

| Variable   | Purpose                                                   |
|------------|-----------------------------------------------------------|
| `txPrev`   | The actual previous transaction (for nSequence, nNextType, etc.) |
| `hashLast` | The 512-bit hash of that transaction — becomes `tx.hashPrevTx` |

When the mempool branch wins the resolution, `txPrev` was updated but `hashLast` was **not**.
The fix is a single missing line.

---

## ASCII Diagram — Three-Source Resolution in `CreateTransaction()`

```
          ┌─────────────────────────────────────────────────────────────────────┐
          │                     CreateTransaction()                             │
          │                                                                     │
          │  hashLast = 0                                                       │
          │  txPrev   = (empty)                                                 │
          └───────────────────────────────┬─────────────────────────────────────┘
                                          │
          ┌───────────────────────────────▼─────────────────────────────────────┐
          │  Source 1: LLD::Sessions->ReadLast(hashGenesis, hashLast)           │
          │                                                                     │
          │  IF found:  hashLast ← sessions-last hash      ✅ hashLast updated │
          │             txPrev   ← sessions tx              ✅ txPrev  updated  │
          └───────────────────────────────┬─────────────────────────────────────┘
                                          │
          ┌───────────────────────────────▼─────────────────────────────────────┐
          │  Source 2: mempool.Get(hashGenesis, txMem)                          │
          │                                                                     │
          │  IF mempool tx exists AND txMem.nSequence > txPrev.nSequence:       │
          │      txPrev  ← txMem               ✅ txPrev  updated               │
          │  ❌ MISSING:  hashLast NOT updated  ← BUG IS HERE                  │
          │                                                                     │
          │  (else-if branch below is SKIPPED when mempool.Get() succeeds)      │
          └───────────────────────────────┬─────────────────────────────────────┘
                                          │
          ┌───────────────────────────────▼─────────────────────────────────────┐
          │  Source 3 (else-if): LLD::Ledger->ReadLast(hashGenesis, hashLast)   │
          │                                                                     │
          │  Only runs when mempool.Get() returned false.                       │
          │  IF found:  hashLast ← ledger-last hash        ✅ hashLast updated  │
          │             txPrev   ← ledger tx               ✅ txPrev  updated   │
          └───────────────────────────────┬─────────────────────────────────────┘
                                          │
          ┌───────────────────────────────▼─────────────────────────────────────┐
          │  Build the new transaction                                          │
          │                                                                     │
          │  tx.nSequence  = txPrev.nSequence + 1   ← from txPrev  ✅          │
          │  tx.hashPrevTx = hashLast               ← from hashLast ❌ STALE  │
          └─────────────────────────────────────────────────────────────────────┘
```

When Source 2 (mempool) wins:
- `txPrev.nSequence` is the mempool tx's sequence  → `tx.nSequence` is one ahead ✅
- `hashLast` is still the Sessions (or zero) value → `tx.hashPrevTx` points to the **wrong tx** ❌

---

## Before / After Code Comparison

### Buggy code (before fix)

```cpp
/* If we don't have the indexes available we need to build from ledger state. */
TAO::Ledger::Transaction txMem;
if(mempool.Get(hashGenesis, txMem))
{
    fUsedMempool = true;

    /* Check that the mempool transaction is greater than our logical database. */
    if(txMem.nSequence > txPrev.nSequence)
    {
        txPrev = txMem;
        // ❌ hashLast is NOT updated here — it still holds the Sessions value (or 0)
        strSeqSource = (fUsedSessionIndex ? "mempool_override_sessions" : "mempool");
    }
}
else if(LLD::Ledger->ReadLast(hashGenesis, hashLast))
{
    // This branch is SKIPPED when mempool.Get() returns true, even if it found nothing useful
    ...
}
```

### Fixed code (after fix)

```cpp
TAO::Ledger::Transaction txMem;
if(mempool.Get(hashGenesis, txMem))
{
    fUsedMempool = true;

    if(txMem.nSequence > txPrev.nSequence)
    {
        txPrev    = txMem;
        hashLast  = txMem.GetHash();    // ✅ CRITICAL: sync hashLast to mempool tx hash
        strSeqSource = (fUsedSessionIndex ? "mempool_override_sessions" : "mempool");
    }
}
else if(LLD::Ledger->ReadLast(hashGenesis, hashLast))
{
    ...
}
```

The one-line addition `hashLast = txMem.GetHash();` is the complete fix.

---

## Step-by-Step Failure Scenario

Given a fresh miner sigchain (genesis tx, nSequence = 0) and a login that deposited a session
transaction (nSequence = 1) into the mempool:

| Step | Action | `hashLast` | `txPrev.nSequence` |
|------|--------|------------|---------------------|
| 1 | `LLD::Sessions->ReadLast()` succeeds | hash of seq-0 tx | 0 |
| 2 | `mempool.Get()` succeeds, seq-1 wins | **still hash of seq-0** ❌ | 1 ✅ |
| 3 | `else if(LLD::Ledger->ReadLast())` skipped | unchanged | unchanged |
| 4 | `tx.nSequence = txPrev.nSequence + 1` | — | **2** (off by one) |
| 5 | `tx.hashPrevTx = hashLast` | **hash of seq-0** ❌ | — |

During `BlockState::Connect()` → `Transaction::Connect()`:
- The validator reads `hashPrevTx` → loads the seq-0 tx.
- Expects `nSequence == txPrev.nSequence + 1 == 1`.
- Finds `nSequence == 2` in the producer.
- Emits: `"prev transaction incorrect sequence"`.

---

## Log Evidence

The following error sequence in the node logs indicates this bug:

```
ERROR: Connect : prev transaction incorrect sequence
ERROR: Connect : failed to connect transaction
ERROR: SetBest : failed to connect <block_hash>
ERROR: Index : failed to set best chain
ERROR: ProcessPacket : ✗ AcceptMinedBlock ledger write failed after STATELESS_BLOCK_ACCEPTED sent: block rejected
```

With `-nseqdiag` enabled, the diagnostic log preceding the failure shows
`hashLast_mismatch=yes`, confirming the desync:

```
[NSEQ_DIAG][CreateTransaction] genesis=<hash> source=mempool_override_sessions
  used_sessions=yes used_mempool=yes fallback_ledger=no
  hashLast=<hash_of_seq0> txPrev.hash=<hash_of_seq1>
  hashLast_mismatch=yes txPrev.nSequence=1 chosen.nSequence=2
```

After the fix, `hashLast_mismatch` must always report `no`.

---

## Impact

This bug affects **all mining reward claims** where a mempool transaction exists for the miner's
sigchain at the time `CreateProducer()` → `CreateTransaction()` runs.  Common cases include:

- The miner's session login itself created a tx that landed in the mempool before block production.
- Any API call (name registration, token create, etc.) made by the miner's wallet between logins.
- Any other pending sigchain operation visible in the mempool for that genesis.

On a fresh node with an empty mempool (first-ever block), the bug is dormant.  As soon as any
mempool activity exists for the signing genesis, every subsequent mined block will fail to commit.

---

## Stake Minter Verification

This bug affects the **Proof-of-Stake (staking) path** identically to PoW mining.
The Tritium Stake Minter calls `CreateStakeBlock()` (create.cpp:865), which calls
the same `CreateTransaction()` function (create.cpp:893) that PoW uses via
`CreateProducer()` (create.cpp:705).

**Call chain:**
```
TritiumMinter::StakeMinterThread()  →  CreateCandidateBlock()
  →  CreateStakeBlock(pCredentials, strPIN, block, fGenesis)
    →  CreateTransaction(user, pin, rProducer)   ← SAME FIXED FUNCTION
```

Because all block production paths converge on a single `CreateTransaction()`,
the fix automatically applies to staking.  See
`docs/SIGCHAIN_SEQUENCE_FIX_VERIFICATION.md` for the full cross-path verification
and `docs/diagrams/mining/sigchain-sequence-resolution.md` for flow diagrams.

---

## Additional Fixes (PR #493)

Two additional bugs were identified and fixed in `CreateTransaction()` during the
same investigation:

### Fix 1: Mempool genesis (seq=0) ignored when txPrev is unset

When the mempool contained the genesis profile (seq=0) and Sessions had no entry,
`txPrev` was default-constructed with `hashGenesis==0` and `nSequence==0`.
The comparison `txMem.nSequence > txPrev.nSequence` evaluated to `0 > 0 → false`,
silently ignoring the only valid transaction.  The disk fallback was also skipped
(see Fix 2).

**Fix:** Added `|| (txPrev.hashGenesis == 0 && txMem.hashGenesis != 0)`.

### Fix 2: `else if` prevented disk fallback

The disk lookup was gated by `else if(LLD::Ledger->ReadLast(...))`, meaning it was
skipped whenever `mempool.Get()` returned true — even if the mempool entry didn't
win the comparison.

**Fix:** Changed to unconditional block with separate `hashDiskLast` variable.

---

## Upstream Reference

The identical bugs are present in `Nexusoft/LLL-TAO` at commit
`ff33dea5fc4e9dc3bf325c059278d6938934bfb9`, lines 84-99 of `src/TAO/Ledger/create.cpp`.
This document and its accompanying PR fix should be reported to upstream maintainer Colin McKinlay
so that the upstream codebase can receive the same corrections.

---

## Regression Prevention

Dedicated unit tests were added:

- `tests/unit/TAO/Ledger/create_transaction.cpp` — Tests the three-source resolution
  logic including the hashLast sync fix and both Fix 1 (genesis override) and Fix 2
  (unconditional disk check).

- `tests/unit/TAO/Ledger/stake_sequence.cpp` — Tests the same resolution logic in
  stake minter scenarios (bootstrap, restart, mempool-only genesis, etc.).

The tests exercise the resolution with controlled source inputs and assert:

1. `hashLast` equals the hash of the winning transaction.
2. `txPrev.nSequence` equals the winning transaction's sequence.
3. `hashLast` and `txPrev` always come from the same source (invariant check).
4. Mempool genesis (seq=0) is accepted when txPrev is unset (Fix 1).
5. Disk is always consulted even when mempool is present (Fix 2).

Run the regression tests with:

```bash
make -f makefile.cli test
# or run just the ledger group:
./tests/unit_tests --run_test=ledger
```

---

## Related PRs

| PR | Description |
|----|-------------|
| #479 | Added `SequenceDiagnosticsEnabled()` and `[NSEQ_DIAG]` logging infrastructure |
| #480 | Moved `STATELESS_BLOCK_ACCEPTED` after ledger commit — prerequisite fix |
| #481 | Added vtx sigchain pre-checks — complementary, does not fix core issue |
| #493 | Fix 1 (genesis override) + Fix 2 (unconditional disk) + PruneCommittedVtx |
| This PR | One-line fix: `hashLast = txMem.GetHash()` + unit test + documentation |

---

## Related Documentation

| Document | Description |
|----------|-------------|
| `docs/SIGCHAIN_SEQUENCE_FIX_VERIFICATION.md` | Full cross-path verification (PoW + PoS) |
| `docs/diagrams/mining/sigchain-sequence-resolution.md` | Flow diagrams for all scenarios |
| `docs/NSEQ_DIAG_MEMPOOL_READLAST_FIX.md` | ValidateVtxSigchainConsistency fix |
