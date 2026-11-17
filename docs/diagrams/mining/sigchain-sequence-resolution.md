# Sigchain Sequence Resolution — Flow Diagrams

## Overview

This document contains ASCII flow diagrams showing how `CreateTransaction()` resolves
the sigchain sequence (`nSequence` + `hashPrevTx`) for all Tritium block producers,
and how the fix corrects the three bugs identified in the resolution logic.

---

## Diagram 1: Block Production Entry Points → CreateTransaction()

All block production paths converge on a single `CreateTransaction()` function.

```
                         ┌──────────────────────────────────────────────┐
                         │           BLOCK PRODUCTION ENTRY POINTS      │
                         └──────────────────────┬───────────────────────┘
                                                │
            ┌───────────────────┬───────────────┼───────────────┬───────────────────┐
            │                   │               │               │                   │
            ▼                   ▼               ▼               ▼                   ▼
     ┌─────────────┐   ┌──────────────┐  ┌────────────┐  ┌────────────┐   ┌──────────────┐
     │  Prime (1)  │   │  Hash (2)    │  │ Stake (0)  │  │ Hybrid (3) │   │  Stateless   │
     │  PoW Mining │   │  PoW Mining  │  │   PoS      │  │  Private   │   │  Mining 9323 │
     └──────┬──────┘   └──────┬───────┘  └─────┬──────┘  └─────┬──────┘   └──────┬───────┘
            │                 │                 │               │                  │
            ▼                 ▼                 │               ▼                  ▼
     ┌────────────────────────────┐             │        ┌───────────────────────────┐
     │      CreateBlock()        │             │        │   CreateBlock() (same)    │
     │      create.cpp:437       │             │        │                           │
     └──────────┬────────────────┘             │        └──────────┬────────────────┘
                │                               │                   │
                ▼                               ▼                   ▼
     ┌────────────────────────┐     ┌─────────────────────────┐
     │   CreateProducer()     │     │  CreateStakeBlock()      │
     │   create.cpp:695       │     │  create.cpp:865          │
     └──────────┬─────────────┘     └──────────┬──────────────┘
                │                               │
                └───────────────┬───────────────┘
                                │
                                ▼
                ╔═══════════════════════════════════════╗
                ║       CreateTransaction()             ║
                ║       create.cpp:73–239               ║
                ║                                       ║
                ║  Sets: nSequence, hashPrevTx,         ║
                ║        hashGenesis, nKeyType,         ║
                ║        nNextType, hashRecovery,       ║
                ║        nTimestamp, hashNext            ║
                ║                                       ║
                ║  *** ALL FIXES ARE HERE ***            ║
                ╚═══════════════════════════════════════╝
```

---

## Diagram 2: Three-Source Resolution in CreateTransaction() (FIXED)

```
        ┌──────────────────────────────────────────────────────────────────────────┐
        │                    CreateTransaction()                                   │
        │                                                                          │
        │   hashLast = 0                                                           │
        │   txPrev   = (default: hashGenesis=0, nSequence=0)                       │
        └────────────────────────────────┬─────────────────────────────────────────┘
                                         │
        ┌────────────────────────────────▼─────────────────────────────────────────┐
        │  SOURCE 1: LLD::Sessions->ReadLast(hashGenesis, hashLast)                │
        │                                                                          │
        │  IF found:  hashLast ← sessions-last hash             ✅ hashLast set    │
        │             txPrev   ← sessions tx                    ✅ txPrev  set     │
        │  ELSE:      hashLast remains 0, txPrev remains empty                     │
        └────────────────────────────────┬─────────────────────────────────────────┘
                                         │
        ┌────────────────────────────────▼─────────────────────────────────────────┐
        │  SOURCE 2: mempool.Get(hashGenesis, txMem)                               │
        │                                                                          │
        │  IF mempool tx exists AND                                                │
        │      (txMem.nSequence > txPrev.nSequence                                 │
        │       OR (txPrev.hashGenesis==0 AND txMem.hashGenesis!=0)):  ← FIX #1    │
        │                                                                          │
        │      txPrev   ← txMem                                ✅ txPrev  set      │
        │      hashLast ← txMem.GetHash()                      ✅ hashLast synced  │
        └────────────────────────────────┬─────────────────────────────────────────┘
                                         │
        ┌────────────────────────────────▼─────────────────────────────────────────┐
        │  SOURCE 3: LLD::Ledger->ReadLast(hashGenesis, hashDiskLast)  ← FIX #2   │
        │  (ALWAYS checked — no longer "else if")                                  │
        │                                                                          │
        │  IF found AND txDisk.nSequence > txPrev.nSequence:                       │
        │      txPrev   ← txDisk                               ✅ txPrev  set      │
        │      hashLast ← hashDiskLast                         ✅ hashLast set     │
        │                                                                          │
        │  (Disk only wins if it has a HIGHER sequence than                         │
        │   whatever Sessions or Mempool already set)                              │
        └────────────────────────────────┬─────────────────────────────────────────┘
                                         │
        ┌────────────────────────────────▼─────────────────────────────────────────┐
        │  BUILD NEW TRANSACTION                                                   │
        │                                                                          │
        │  IF txPrev.hashGenesis != 0:     (prior tx exists)                       │
        │      tx.nSequence  = txPrev.nSequence + 1        ✅ correct              │
        │      tx.hashPrevTx = hashLast                    ✅ consistent           │
        │      tx.hashGenesis = txPrev.hashGenesis                                 │
        │                                                                          │
        │  ELSE IF tx.IsFirst():           (genesis — first ever tx)               │
        │      tx.nSequence  = 0                                                   │
        │      tx.hashPrevTx = 0                                                   │
        │      tx.hashGenesis = hashGenesis                                        │
        └──────────────────────────────────────────────────────────────────────────┘
```

---

## Diagram 3: Stake Minter Full Flow

```
       ┌────────────────────────────────────────────────────────┐
       │  TritiumMinter::StakeMinterThread()                    │
       │  [tritium_minter.cpp:356]                              │
       └───────────────────────────┬────────────────────────────┘
                                   │
                                   ▼
       ┌────────────────────────────────────────────────────────┐
       │  CheckUser()                                           │
       │  Verify: Authentication::Unlocked(PinUnlock::STAKING)  │
       └───────────────────────────┬────────────────────────────┘
                                   │ ✅ unlocked
                                   ▼
       ┌────────────────────────────────────────────────────────┐
       │  hashGenesis = Authentication::Caller()                │
       └───────────────────────────┬────────────────────────────┘
                                   │
                                   ▼
       ┌────────────────────────────────────────────────────────┐
       │  FindTrustAccount(hashGenesis)                         │
       │  Read trust register, determine Genesis vs Trust       │
       └───────────────────────────┬────────────────────────────┘
                                   │
                                   ▼
       ┌────────────────────────────────────────────────────────┐
       │  CreateCandidateBlock()                                │
       │                                                        │
       │  ┌──────────────────────────────────────────────────┐  │
       │  │ Unlock(strPIN, PinUnlock::STAKING)               │  │
       │  │ pCredentials = Authentication::Credentials()      │  │
       │  └──────────────────────────────────────────────────┘  │
       │                                                        │
       │  ┌──────────────────────────────────────────────────┐  │
       │  │ CreateStakeBlock(pCredentials, strPIN, block)     │  │
       │  │                                                    │ │
       │  │   ├─ AddTransactions(block)  [if !fGenesis]        │ │
       │  │   │                                                │ │
       │  │   └─ CreateTransaction(user, pin, rProducer) ◄─────┤ │
       │  │        │                                           │ │
       │  │        ├─ Source 1: Sessions                       │ │
       │  │        ├─ Source 2: Mempool  ← FIX #1              │ │
       │  │        └─ Source 3: Disk     ← FIX #2              │ │
       │  │                                                    │ │
       │  │   block.producer = rProducer                       │ │
       │  │     nSequence  = txPrev.nSequence + 1  ✅          │ │
       │  │     hashPrevTx = hashLast              ✅          │ │
       │  └──────────────────────────────────────────────────┘  │
       │                                                        │
       │  ┌──────────────────────────────────────────────────┐  │
       │  │ CreateCoinstake(hashGenesis)                      │  │
       │  │   Sets up OP::TRUST or OP::GENESIS operation      │  │
       │  └──────────────────────────────────────────────────┘  │
       └───────────────────────────┬────────────────────────────┘
                                   │
                                   ▼
       ┌────────────────────────────────────────────────────────┐
       │  CalculateWeights()                                    │
       │  Compute trust weight, block weight, stake rate        │
       └───────────────────────────┬────────────────────────────┘
                                   │
                                   ▼
       ┌────────────────────────────────────────────────────────┐
       │  MintBlock() → HashBlock()                             │
       │                                                        │
       │  HASHING LOOP:                                         │
       │    while (not stale && not shutdown):                   │
       │      nonce++                                           │
       │      hash = StakeHash()                                │
       │      if hash < target:                                 │
       │        ProcessBlock()  ──────────────────────────┐     │
       │                                                  │     │
       └──────────────────────────────────────────────────┼─────┘
                                                          │
                                                          ▼
       ┌────────────────────────────────────────────────────────┐
       │  ProcessBlock()                                        │
       │                                                        │
       │  1. CalculateCoinstakeReward(nTime)                    │
       │  2. block.producer[0] << nReward                       │
       │  3. block.producer.Build()                             │
       │                                                        │
       │  4. Unlock(strPIN, PinUnlock::STAKING)                 │
       │  5. pCredentials = Authentication::Credentials()        │
       │                                                        │
       │  6. block.producer.Sign(                               │
       │       pCredentials->Generate(                          │
       │         block.producer.nSequence,  ← set by            │
       │         strPIN))                      CreateTransaction │
       │                                                        │
       │  7. BuildMerkleTree()                                  │
       │  8. SignBlock(pCredentials, strPIN)                     │
       │                                                        │
       │  9. TAO::Ledger::Process(block, nStatus)               │
       │                                                        │
       │  10. Check: nStatus & PROCESS::ACCEPTED                │
       │                                                        │
       │  ✅ Block is committed to chain and relayed to network │
       └────────────────────────────────────────────────────────┘
```

---

## Diagram 4: Bug Scenarios — Before and After Fix

### Scenario A: First Mining Block with Mempool Genesis

```
BEFORE FIX (BUG):
═══════════════════

  State:
    Sessions:  empty
    Mempool:   genesis profile tx (hashGenesis=G, seq=0)
    Disk:      empty

  CreateTransaction() resolution:
    Source 1 (Sessions): miss        → txPrev=(empty), hashLast=0
    Source 2 (Mempool):  hit (seq=0) → txMem.nSequence(0) > txPrev.nSequence(0)?
                                       0 > 0 = FALSE ❌ — mempool genesis IGNORED
    Source 3 (Disk):     SKIPPED (else if)

  Result:
    txPrev.hashGenesis == 0  →  falls into IsFirst() branch
    tx.nSequence = 0    ← DUPLICATE GENESIS!
    tx.hashPrevTx = 0

  Failure at Connect():
    "duplicate genesis-id" error  ← Block rejected

═══════════════════

AFTER FIX:
═══════════════════

  CreateTransaction() resolution:
    Source 1 (Sessions): miss        → txPrev=(empty), hashLast=0
    Source 2 (Mempool):  hit (seq=0) → txPrev.hashGenesis==0 AND txMem.hashGenesis!=0?
                                       TRUE ✅ — mempool genesis ACCEPTED
                                       txPrev=txMem, hashLast=txMem.GetHash()
    Source 3 (Disk):     miss        → (no override)

  Result:
    txPrev.hashGenesis != 0  →  enters non-genesis branch
    tx.nSequence = 0 + 1 = 1    ✅ CORRECT
    tx.hashPrevTx = hash(genesis_tx)  ✅ CORRECT

  Block connects successfully ✅
```

### Scenario B: Mempool Override Loses hashLast

```
BEFORE FIX (BUG):
═══════════════════

  State:
    Sessions:  tx at seq=0 (hashLast=H0)
    Mempool:   tx at seq=1
    Disk:      tx at seq=0

  CreateTransaction() resolution:
    Source 1 (Sessions): hit → txPrev=seq0, hashLast=H0
    Source 2 (Mempool):  hit → seq1 > seq0?  YES
                               txPrev=txMem(seq1) ✅
                               hashLast=H0  ← NOT UPDATED  ❌  (BUG)
    Source 3 (Disk):     SKIPPED (else if, because mempool.Get() returned true)

  Result:
    tx.nSequence = 1 + 1 = 2
    tx.hashPrevTx = H0  ← WRONG (should be hash of seq1 tx)

  Failure at Connect():
    "prev transaction incorrect sequence" error

═══════════════════

AFTER FIX:
═══════════════════

  CreateTransaction() resolution:
    Source 1 (Sessions): hit → txPrev=seq0, hashLast=H0
    Source 2 (Mempool):  hit → seq1 > seq0?  YES
                               txPrev=txMem(seq1) ✅
                               hashLast=txMem.GetHash() ✅  (FIX: line 124)
    Source 3 (Disk):     checked → seq0 > seq1?  NO  (mempool wins)

  Result:
    tx.nSequence = 1 + 1 = 2  ✅
    tx.hashPrevTx = hash(seq1 tx)  ✅

  Block connects successfully ✅
```

---

## Diagram 5: Submit-Time Pre-Validation Pipeline

```
  SUBMIT_BLOCK received (both port 8323 and port 9323)
          │
          ▼
  ┌─────────────────────────────────────────┐
  │ PruneCommittedVtxTransactions(block)    │ ← NEW: removes already-indexed vtx
  │                                         │   Prevents "transaction overwrites
  │  Scan vtx for HasIndex() = true         │   not allowed" in Connect()
  │  Remove committed entries               │
  │  Rebuild merkle + re-sign if needed     │
  └────────────────────┬────────────────────┘
                       │
                       ▼
  ┌─────────────────────────────────────────┐
  │ RefreshProducerIfStale(block)           │ ← FIX #3: mempool fallback
  │                                         │
  │  Check vtx for same-genesis entries     │   When ReadLast(disk) fails,
  │  Check disk ReadLast                    │   now also checks mempool.Get()
  │  Check mempool.Get() ← NEW             │
  │  Re-sequence producer if stale          │
  └────────────────────┬────────────────────┘
                       │
                       ▼
  ┌─────────────────────────────────────────┐
  │ ValidateVtxSigchainConsistency(block)   │
  │                                         │
  │  Pre-check vtx sigchain ordering        │
  │  Uses FLAGS::MEMPOOL for ReadLast       │
  └────────────────────┬────────────────────┘
                       │
                       ▼
  ┌─────────────────────────────────────────┐
  │ ValidateMinedBlock(block)               │
  │                                         │
  │  Stale detection, block.Check()         │
  └────────────────────┬────────────────────┘
                       │
                       ▼
  ┌─────────────────────────────────────────┐
  │ AcceptMinedBlock(block)                 │
  │                                         │
  │  TAO::Ledger::Process(block, nStatus)   │
  │    → TritiumBlock::Accept()             │
  │      → BlockState::Connect()            │
  │        → Transaction::Connect() per vtx │
  │          Validates nSequence chain       │
  │          Calls WriteFirst / WriteLast    │
  └────────────────────┬────────────────────┘
                       │
                  ┌────┴────┐
                  │         │
                  ▼         ▼
          ACCEPTED    REJECTED
```

---

## Diagram 6: nSequence Chain Integrity

```
  Sigchain Linked List (each tx points to its predecessor):

  ┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
  │ TX (seq=0)      │     │ TX (seq=1)      │     │ TX (seq=2)      │
  │ hashGenesis=G   │◄────│ hashPrevTx=H0   │◄────│ hashPrevTx=H1   │
  │ hashPrevTx=0    │     │ hashGenesis=G   │     │ hashGenesis=G   │
  │ hash=H0         │     │ hash=H1         │     │ hash=H2         │
  │                 │     │                 │     │                 │
  │ (genesis/       │     │ (login /        │     │ (producer for   │
  │  profile create)│     │  API operation) │     │  mined block)   │
  └─────────────────┘     └─────────────────┘     └─────────────────┘

  CreateTransaction() must produce:
    tx.nSequence  = highest_known_seq + 1
    tx.hashPrevTx = hash_of_highest_known_tx

  Both values MUST be derived from the SAME source transaction.
  The bug was: nSequence came from mempool (correct) but hashPrevTx
  came from Sessions/zero (wrong source) → broken chain.
```
