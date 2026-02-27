# Nexus Tritium Block Production Flow
## Architecture Reference Document — STATELESS-NODE Branch

**Version:** 1.0  
**Branch:** `STATELESS-NODE`  
**Last Updated:** 2026-02-24 07:39:50  
**Covers:** Genesis initialization → Stake Minter block production → Mining channel template → Accept/Commit  

---

## Table of Contents

1. [Overview and Purpose](#1-overview-and-purpose)
2. [The Unified Height Contract](#2-the-unified-height-contract)
3. [Diagram 1 — Chain Genesis Initialization Tree](#3-diagram-1--chain-genesis-initialization-tree)
4. [Diagram 2 — Stake Minter Full Production Loop](#4-diagram-2--stake-minter-full-production-loop)
5. [Diagram 3 — AddBlockData() Field Population Map](#5-diagram-3--addblockdata-field-population-map)
6. [Diagram 4 — Mining Channel Template Pattern (Channels 1, 2, 3)](#6-diagram-4--mining-channel-template-pattern-channels-1-2-3)
7. [Diagram 5 — Height Lineage: Genesis → Current Block](#7-diagram-5--height-lineage-genesis--current-block)
8. [Diagram 6 — Staleness Guards: Two-Layer Defence](#8-diagram-6--staleness-guards-two-layer-defence)
9. [The Template Pattern: Stake as the Reference Implementation](#9-the-template-pattern-stake-as-the-reference-implementation)
10. [Field Ownership Table](#10-field-ownership-table)
11. [What Must Never Change After AddBlockData()](#11-what-must-never-change-after-addblockdata)
12. [Mining Channel Application of the Pattern](#12-mining-channel-application-of-the-pattern)
13. [Invariants and Consensus Rules](#13-invariants-and-consensus-rules)
14. [File and Function Index](#14-file-and-function-index)

---

## 1. Overview and Purpose

This document describes the **end-to-end block production flow** for the Nexus Tritium protocol on the `STATELESS-NODE` branch. It establishes the canonical reference for:

- How the blockchain genesis anchors all subsequent heights
- Why `block.nHeight` must be the **unified** chain height
- How the Stake Minter produces Tritium blocks, step by step
- How Mining Channels (Prime=1, Hash=2, Private=3) use the **same underlying template pattern** as the Stake Minter
- The two-layer staleness guard system that prevents invalid blocks from being submitted

The Stake Minter is used as the **primary reference implementation** because it exercises every phase of block production, including the deferred Merkle root path that is unique to channel 0. Mining channels follow a simpler variant of the same pattern.

---

## 2. The Unified Height Contract

### The Core Rule

```
block.nHeight  ==  tStateBest.nHeight + 1  (UNIFIED chain height)
```

This single rule, enforced by `TritiumBlock::Accept()`, is the foundation of the entire system.

### Why It Matters

The Nexus blockchain uses a **tri-channel** consensus model:

| Channel | ID | Description |
|---|---|---|
| Proof of Stake | 0 | Trust-weight threshold hashing |
| Prime | 1 | Primorial cluster mining |
| Hash | 2 | SHA-3 / GPU hash mining |
| Private | 3 | Authorized signature chain |

Every channel produces blocks on the **same unified chain**. The `nHeight` field in a block header is the block's position in this unified sequence — NOT its position within its own channel's sub-sequence.

### The Channel Height Distinction

```
block.nHeight       = Unified position in full chain  (e.g., 6,604,692)
BlockState::nChannelHeight = Position within this channel only  (e.g., 2,329,307)
```

`nChannelHeight` is computed **after** acceptance by `BlockState::SetBest()` via `GetLastState()`. It is **metadata** stored in the `BlockState` database record. It is never serialized into the 216-byte block template bytes and must never be used to set `block.nHeight`.

### The Validation Check

In `TritiumBlock::Accept()`:
```cpp
// statePrev is the BlockState at block.hashPrevBlock
// statePrev.nHeight is the UNIFIED height of the previous block
if(statePrev.nHeight + 1 != block.nHeight)
    return debug::error("block height mismatch");
```

---

## 3. Diagram 1 — Chain Genesis Initialization Tree

```
NODE STARTUP
     │
     ▼
CreateGenesis()  [create.cpp:760]
     │
     ├─── config::fHybrid ──────────────────────────────────► HybridGenesis()
     │                                                              │
     │    hashGenesisHybrid = computed PoW solution                │
     │    block.nHeight = 0                                         │
     │    block.nChannelHeight = 1                                  │
     │    block.hashPrevBlock = 0  (root)                          │
     │                                                              │
     ├─── config::fClient && !fTestNet ──────────────────────► TritiumGenesis()
     │                                                              │
     │    Hardcoded BlockState snapshot at height 2,944,207         │
     │    block.nHeight = 2,944,207  ◄── NOT zero!                 │
     │    block.hashPrevBlock = hardcoded 1024-bit hash             │
     │    (Represents the first post-Legacy Tritium block)          │
     │                                                              │
     ├─── config::fClient && fTestNet ───────────────────────► LegacyGenesis()
     │                                                              │
     └─── full node (default) ───────────────────────────────► LegacyGenesis()
                                                                    │
          block.nHeight = 0  ◄── TRUE root                         │
          block.nChannelHeight = 1                                  │
          block.nChannel = 2  (Hash channel for genesis)            │
          block.hashPrevBlock = 0  (null, sentinel)                 │
          block.nTime = 1409456199  (Aug 31, 2014)                  │
                                                                    │
                              ◄─────────────────────────────────────┘
                              ALL PATHS converge to:

     ChainState::tStateGenesis = state            [create.cpp:821]
     ChainState::hashBestChain = hashGenesis      [create.cpp:824]
     ChainState::tStateBest    = tStateGenesis    [create.cpp:825]
          │
          │  (After full network sync, tStateBest advances to current tip)
          │
          ▼
     tStateBest.nHeight ≈ 6,604,691  (current unified tip)
     tStateBest.GetHash() = hashBestChain
          │
          ▼  used by EVERY block creator:
     AddBlockData(tStateBest, nChannel, block)
          block.nHeight      = tStateBest.nHeight + 1  = 6,604,692
          block.hashPrevBlock = tStateBest.GetHash()
```

**Key insight:** Whether the genesis anchor is height 0 (Legacy) or height 2,944,207 (Tritium), after full sync `tStateBest.nHeight` is always the correct current unified tip. The `+1` in `AddBlockData()` is therefore always correct regardless of genesis type.

---

## 4. Diagram 2 — Stake Minter Full Production Loop

```
TritiumMinter::StakeMinterThread()  [tritium_minter.cpp:356]
     │
     ▼  [WAIT PHASE]
     while(Synchronizing() || no_connections)
          sleep(5000ms)
     │
     ▼  [MAIN LOOP — repeats every nSleepTime ms]
     ┌──────────────────────────────────────────────────────────────────┐
     │                                                                  │
     │  ① SNAPSHOT hashLastBlock                                        │
     │     hashLastBlock = ChainState::hashBestChain.load()            │
     │     [tritium_minter.cpp:397]                                     │
     │     ↑ This is Guard 1's reference point                         │
     │                                                                  │
     │  ② FindTrustAccount(hashGenesis)    [stake_minter.cpp:167]      │
     │     fGenesis = !fIndexed                                         │
     │     ├─ fGenesis=true  → first stake ever (OP::GENESIS path)     │
     │     └─ fGenesis=false → repeat stake  (OP::TRUST path)          │
     │                                                                  │
     │  ③ CreateCandidateBlock()           [stake_minter.cpp:351]      │
     │     │                                                            │
     │     │  a) block = TritiumBlock()  (reset)                       │
     │     │                                                            │
     │     │  b) CreateStakeBlock(user, pin, block, fGenesis)          │
     │     │       [create.cpp:713]                                     │
     │     │       │                                                    │
     │     │       ├─ if(!fGenesis || fPoolStaking)                    │
     │     │       │     AddTransactions(block)  ← skip for solo genesis│
     │     │       │                                                    │
     │     │       ├─ CreateTransaction(user, pin, rProducer)          │
     │     │       │     builds bare producer skeleton                  │
     │     │       │                                                    │
     │     │       ├─ UpdateProducerTimestamp(rProducer)               │
     │     │       │                                                    │
     │     │       ├─ block.producer = rProducer                       │
     │     │       │                                                    │
     │     │       └─ AddBlockData(tStateBest, nChannel=0, block)      │
     │     │             [create.cpp:323]                               │
     │     │             block.hashPrevBlock = tStateBest.GetHash()    │
     │     │             block.nChannel      = 0                       │
     │     │             block.nHeight       = tStateBest.nHeight + 1  │
     │     │             block.nBits         = GetNextTargetRequired() │
     │     │             block.nNonce        = 1                       │
     │     │             block.nTime         = max(prev+1, now)        │
     │     │             ⚠ hashMerkleRoot   = NOT SET (channel 0)     │
     │     │                                                            │
     │     │  c) CreateCoinstake(hashGenesis) [tritium_minter.cpp:134] │
     │     │       if(!fGenesis):                                       │
     │     │         stateLast ← LLD::Ledger->ReadBlock(hashLast, ...)  │
     │     │         ← loaded by BLOCK HASH, not GetLastState()!       │
     │     │         stateLast.nHeight = UNIFIED height of last stake  │
     │     │         nBlockAge = tStatePrev.time - stateLast.time      │
     │     │         nTrust = GetTrustScore(...)                       │
     │     │         producer[0] << OP::TRUST << hashLast << nTrust   │
     │     │       if(fGenesis):                                        │
     │     │         producer[0] << OP::GENESIS                        │
     │     │                                                            │
     │  ④ CalculateWeights()               [stake_minter.cpp:384]      │
     │     if(!fGenesis): nTrustWeight + nBlockWeight                  │
     │     if(fGenesis):  GenesisWeight(coinAge) — wait if immature    │
     │                                                                  │
     │  ⑤ MintBlock(hashGenesis)            [tritium_minter.cpp:247]   │
     │     │                                                            │
     │     ├─ CheckInterval(): block.nHeight - stateLast.nHeight       │
     │     │   BOTH are unified heights after PR #278 ✅               │
     │     │                                                            │
     │     ├─ CheckMempool(): skip genesis if tx in mempool            │
     │     │                                                            │
     │     └─ HashBlock(nRequired)          [stake_minter.cpp:522]     │
     │           │                                                      │
     │           │  [GUARD 1 LOOP]                                     │
     │           │  while(!fStop && hashLastBlock == hashBestChain)    │
     │           │  {                                                   │
     │           │    CheckStale() → producer orphan check             │
     │           │    block.UpdateTime()                               │
     │           │    nThreshold = GetCurrentThreshold(nBlockTime,     │
     │           │                                     block.nNonce)   │
     │           │    if(nThreshold >= nRequired):                     │
     │           │      hashProof = block.StakeHash()                  │
     │           │      if(hashProof <= nHashTarget):                  │
     │           │        ProcessBlock() ◄── BLOCK FOUND              │
     │           │        break                                         │
     │           │    ++block.nNonce                                   │
     │           │  }                                                   │
     │           │                                                      │
     │           ▼  ProcessBlock()          [stake_minter.cpp:610]     │
     │                                                                  │
     │    ⑥ FINALIZE BLOCK                                             │
     │       nReward = CalculateCoinstakeReward(block.GetBlockTime())  │
     │       block.producer[0] << nReward   ← appended last           │
     │       block.producer.Build()         ← pre/post state exec     │
     │       vHashes = vtx hashes + producer.GetHash()                │
     │       block.hashMerkleRoot = BuildMerkleTree(vHashes)  ← NOW  │
     │       block.producer.Sign(credentials)                          │
     │       SignBlock(credentials)  ← block.GenerateSignature()      │
     │                                                                  │
     │    ⑦ GUARD 2 — PRE-SUBMIT CHECK                                │
     │       if(block.hashPrevBlock != hashBestChain)                  │
     │           return "Generated block is stale"                     │
     │                                                                  │
     │    ⑧ SUBMIT                                                     │
     │       TAO::Ledger::Process(block, nStatus)                      │
     │       if(nStatus & ACCEPTED): success                           │
     │       if(fGenesis): fGenesis = false  ← switch to TRUST path   │
     │                                                                  │
     └──────────────────────────────────────────────────────────────────┘
          │
          └─► Loop repeats with new hashLastBlock snapshot
```

---

## 5. Diagram 3 — AddBlockData() Field Population Map

```
AddBlockData(tStateBest, nChannel, block)
[create.cpp:323]
═══════════════════════════════════════════════════════════════════

SOURCE                          FIELD                   CHANNEL SCOPE
────────────────────────────────────────────────────────────────────
tStateBest.GetHash()         → block.hashPrevBlock      ALL channels
                               │
                               └─ PRIMARY STALENESS ANCHOR
                                  Compared against hashBestChain
                                  at SUBMIT time (Guard 2)

nChannel (parameter)         → block.nChannel           ALL channels
                               Values: 0=Stake, 1=Prime,
                                       2=Hash,  3=Private

 tStateBest.nHeight + 1       → block.nHeight            ALL channels
                               │
                               └─ UNIFIED height
                                  = position in combined chain
                                  NOT channel-specific height
                                  Validated by Accept():
                                  statePrev.nHeight + 1 == nHeight

GetNextTargetRequired(       → block.nBits              ALL channels
  tStateBest, nChannel)        │
                               └─ Difficulty target for this channel

1 (constant)                 → block.nNonce             ALL channels
                               Mining starts at nNonce=1

max(prevTime+1, now)         → block.nTime              ALL channels

BuildMerkleTree(vHashes)     → block.hashMerkleRoot     CHANNELS 1,2,3
                               ⚠ SKIPPED for nChannel==0
                               (Stake minter sets it in ProcessBlock
                               after nReward is known)

═══════════════════════════════════════════════════════════════════
FIELDS SET ELSEWHERE (not by AddBlockData):

block.nVersion     ← CreateBlock()/CreateStakeBlock() — version switch
block.producer     ← CreateTransaction() + CreateCoinstake()/CreateProducer()
block.vtx          ← AddTransactions()
block.vchBlockSig  ← SignBlock() / block.GenerateSignature()
block.hashMerkleRoot (ch0) ← ProcessBlock() after nReward known
```

---

## 6. Diagram 4 — Mining Channel Template Pattern (Channels 1, 2, 3)

```
MINING CHANNEL TEMPLATE FLOW (Prime=1, Hash=2, Private=3)
Contrast with Stake (Channel 0) above

CreateBlockForStatelessMining(nChannel, nExtraNonce, hashRewardAddress)
[stateless_block_utility.cpp]
     │
     ├─ STATELESS TEMPLATE CACHE CHECK (per-channel)
     │   tStatelessCache[nChannel]
     │   if(fValid
     │      && hashBestChainAtCreation == hashBestChain.load()
     │      && age < blockrefresh timeout (90s)):
     │        Clone cached TritiumBlock  ← REUSE
     │        pCached->nNonce = 1
     │        pCached->UpdateTime()
     │        return pCached  ← fast path (~<1ms)
     │
     └─ CACHE MISS → CreateBlock(user, pin, nChannel, rBlockRet, ...)
          [create.cpp:377]
          │
          ├─ CACHE CHECK (local node wallet cache — tBlockCache[nChannel])
          │   if(hashBestChain == tBlockCached.hashPrevBlock
          │      && hashGenesis == cached.producer.hashGenesis
          │      && !timeout):
          │        rBlockRet = tBlockCached  ← REUSE
          │        AddTransactions(rBlockRet)
          │        UpdateProducerTimestamp()
          │        Sign producer
          │        Rebuild hashMerkleRoot  ← always rebuilt on cache hit
          │        goto RETURN
          │
          └─ NEW BLOCK (cache miss)
               │
               ├─ AddTransactions(rBlockRet)
               │
               ├─ CreateProducer(user, pin, rProducer, ...)
               │   [create.cpp:544]
               │   nChannel==1 or 2:
               │     rProducer[0] << OP::COINBASE
               │                  << hashRewardRecipient   (dynamic or static)
               │                  << nCredit
               │                  << nExtraNonce
               │     + Ambassador/Developer payouts at intervals
               │   nChannel==3 (Private):
               │     rProducer[0] << OP::AUTHORIZE
               │                  << rProducer.hashPrevTx
               │                  << rProducer.hashGenesis
               │
               ├─ UpdateProducerTimestamp(rProducer)
               ├─ Sign producer
               │
               ├─ AddBlockData(tStateBest, nChannel, rBlockRet)
               │   ┌──────────────────────────────────────────┐
               │   │ hashPrevBlock = tStateBest.GetHash()     │ ← anchored
               │   │ nChannel      = nChannel (1, 2, or 3)   │
               │   │ nHeight       = tStateBest.nHeight + 1  │ ← UNIFIED
               │   │ nBits         = target for this channel │
               │   │ nNonce        = 1                       │
               │   │ nTime         = max(prev+1, now)        │
               │   │ hashMerkleRoot = BUILT HERE  ✓          │ ← key diff!
               │   └──────────────────────────────────────────┘
               │
               └─ Store to tBlockCache[nChannel]
                    + Store to tStatelessCache[nChannel]

     ▼
RETURN rBlockRet
     │
     ▼  (sent to mining server / NexusMiner)
  MINER receives 216-byte serialized template:
     bytes[0-3]    nVersion      (LE uint32)
     bytes[4-131]  hashPrevBlock (LE uint1024 = 128 bytes)
     bytes[132-195] hashMerkleRoot (LE uint512 = 64 bytes)
     bytes[196-199] nChannel     (LE uint32)
     bytes[200-203] nHeight      (LE uint32)  ← UNIFIED height
     bytes[204-207] nBits        (LE uint32)
     bytes[208-215] nNonce       (LE uint64)

     ▼
MINER submits solved block (nNonce filled, vOffsets for Prime)
     │
     ▼
SUBMIT_BLOCK handler
     Guard 2: block.hashPrevBlock != hashBestChain → reject STALE
     Process(block, nStatus)
     TritiumBlock::Accept():
       statePrev = ReadBlock(block.hashPrevBlock)
       assert statePrev.nHeight + 1 == block.nHeight  ✓
```

---

## 7. Diagram 5 — Height Lineage: Genesis → Current Block

```
UNIFIED CHAIN HEIGHT LINEAGE
════════════════════════════════════════════════════════════════════

  GENESIS ANCHOR
  ┌─────────────────────────────────────────────────────────────┐
  │ LegacyGenesis / TritiumGenesis / HybridGenesis             │
  │   nHeight       = 0  (Legacy/Hybrid) or 2,944,207 (Tritium)│
  │   nChannelHeight = 1                                        │
  │   hashPrevBlock = 0x000...000  (null sentinel)             │
  │   nChannel      = 2  (Hash channel)                        │
  └─────────────────────────────────────────────────────────────┘
                    │
                    │ SetBest() propagates
                    ▼
  ChainState::tStateBest  ←──────────────── updated on every
  ChainState::hashBestChain                  new block accepted

                    │ After full sync:
                    │ tStateBest.nHeight ≈ 6,604,691
                    ▼
  ┌─────────────────────────────────────────────────────────────┐
  │ CURRENT BEST BLOCK (tip)                                   │
  │   nHeight (unified)  = 6,604,691                          │
  │   nChannelHeight[0]  = ~220,000  (stake blocks only)      │
  │   nChannelHeight[1]  = ~900,000  (prime blocks only)      │
  │   nChannelHeight[2]  = ~2,329,307 (hash blocks only)      │
  │   nChannelHeight[3]  = ~500      (private blocks)         │
  └─────────────────────────────────────────────────────────────┘
                    │
                    │ AddBlockData() — NEXT BLOCK CREATION
                    ▼
  ┌─────────────────────────────────────────────────────────────┐
  │ NEW BLOCK TEMPLATE                                         │
  │   block.nHeight       = 6,604,691 + 1 = 6,604,692  ← KEY │
  │   block.hashPrevBlock = tStateBest.GetHash()              │
  │   block.nChannel      = 0, 1, 2, or 3                    │
  └─────────────────────────────────────────────────────────────┘
                    │
                    │ TritiumBlock::Accept() validates:
                    ▼
  ┌─────────────────────────────────────────────────────────────┐
  │ ACCEPTANCE RULE                                            │
  │   statePrev = ReadBlock(block.hashPrevBlock)               │
  │   statePrev.nHeight = 6,604,691          (unified)        │
  │   block.nHeight     = 6,604,692          (unified)        │
  │   Check: 6,604,691 + 1 == 6,604,692  ✓  PASS             │
  └─────────────────────────────────────────────────────────────┘
                    │
                    │ On success: BlockState::SetBest()
                    ▼
  ┌─────────────────────────────────────────────────────────────┐
  │ AFTER ACCEPTANCE — BlockState metadata computed            │
  │   BlockState::nHeight        = 6,604,692  (stored)        │
  │   BlockState::nChannelHeight = GetLastState()+1            │
  │                              ← computed HERE, never before │
  └─────────────────────────────────────────────────────────────┘
```

---

## 8. Diagram 6 — Staleness Guards: Two-Layer Defence

```
STALENESS GUARD ARCHITECTURE
Two guards prevent submitting blocks built on a superseded chain tip.

                    NETWORK EVENT: New block received
                    ChainState::hashBestChain CHANGES
                    ChainState::tStateBest.nHeight++
                         │
                         ├──────────────────────────────────────┐
                         │                                      │
                         ▼                                      ▼
               ┌─────────────────────┐              ┌──────────────────────┐
               │    GUARD 1          │              │    GUARD 2           │
               │  (Stake Minter)     │              │  (All channels)      │
               │                     │              │                      │
               │ HashBlock() loop:   │              │ ProcessBlock() or    │
               │                     │              │ SubmitBlock handler: │
               │ while(              │              │                      │
               │  hashLastBlock ==   │              │ if(block.hashPrevBlock│
               │  hashBestChain)     │              │  != hashBestChain)   │
               │ {                   │              │ {                    │
               │   ... mine ...      │              │   reject("stale")    │
               │ }                   │              │ }                    │
               │                     │              │                      │
               │ Loop exits when     │              │ Final check before   │
               │ chain advances →    │              │ Process() call       │
               │ start new block     │              │                      │
               └─────────────────────┘              └──────────────────────┘
                         │                                      │
               GUARD 1 catches:                     GUARD 2 catches:
               - New block arrived                  - New block arrived
                 while hashing                        between Guard 1
               - Chain reorg                          exit and submission
               - Network tip advance                - Race condition window

GUARD 1 Reference Set At:
  tritium_minter.cpp:397
  hashLastBlock = ChainState::hashBestChain.load()
  (snapshot at START of each minting iteration)

GUARD 2 Check At:
  stake_minter.cpp:674   (Stake Minter)
  mining_server.cpp      (Mining channels — hashPrevBlock check)

BOTH GUARDS use hashPrevBlock (1024-bit hash) not nHeight (32-bit).
A hash comparison catches same-height reorgs that integer comparison misses.
```

---

## 9. The Template Pattern: Stake as the Reference Implementation

The Stake Minter is the **canonical implementation** of the Nexus block production pattern. All mining channels follow the same structure. Use this as your reference when implementing new channels or debugging.

### The 8-Phase Pattern

```
Phase 1: ANCHOR     Snapshot hashBestChain → hashLastBlock
Phase 2: ACCOUNT    Load trust account / credentials / session
Phase 3: TEMPLATE   CreateStakeBlock() / CreateBlock()
              └─► AddBlockData() sets nHeight, hashPrevBlock, nBits
Phase 4: PRODUCER   CreateCoinstake() / CreateProducer()
              └─► Fills block.producer[0] with OP
Phase 5: WEIGHT     CalculateWeights() — trust/block weight or threshold
Phase 6: HASH       HashBlock() / Mining loop
              └─► Guard 1: while(hashLastBlock == hashBestChain)
Phase 7: FINALIZE   ProcessBlock() / on-solve
              └─► Append nReward, Build(), Merkle root, Sign
Phase 8: SUBMIT     Guard 2 check → Process(block, nStatus)
```

### Channel Comparison Table

| Phase | Stake (ch 0) | Prime (ch 1) | Hash (ch 2) | Private (ch 3) |
|---|---|---|---|---|
| **1. Anchor** | `hashLastBlock = hashBestChain` | Same | Same | Same |
| **2. Account** | Trust account + fGenesis | Credentials + Coinbase recipients | Same as Prime | Credentials only |
| **3. Template** | `CreateStakeBlock()` | `CreateBlock(nChannel=1)` | `CreateBlock(nChannel=2)` | `CreateBlock(nChannel=3)` |
| **4. Producer** | `OP::GENESIS` or `OP::TRUST` | `OP::COINBASE` | `OP::COINBASE` | `OP::AUTHORIZE` |
| **5. Weight** | Trust weight + block weight | Difficulty bits only | Difficulty bits only | No weight |
| **6. Hash** | `StakeHash()` — nNonce threshold | Prime cluster search | SHA-3 hash < target | Block signature |
| **7. Finalize** | `nReward` deferred, Merkle built in ProcessBlock | Merkle built in AddBlockData | Same as Prime | Same as Prime |
| **8. Submit** | `Process(block)` | `Process(block)` via mining server | Same | Same |
| **Merkle timing** | **After** solve (ProcessBlock) | **Before** solve (AddBlockData) | Before solve | Before solve |
| **nHeight source** | `tStateBest.nHeight + 1` | Same | Same | Same |
| **nHeight type** | **UNIFIED** | **UNIFIED** | **UNIFIED** | **UNIFIED** |

### The Critical Difference: Merkle Root Timing

Channel 0 (Stake) defers the Merkle root because the coinstake reward (`nReward`) depends on the **final block timestamp**, which is only known when the block is actually found. All other channels include the Merkle root in the template because their producer (`OP::COINBASE` or `OP::AUTHORIZE`) is fully determined before mining begins.

```cpp
// create.cpp:325 — The gate that enforces this design
if(nChannel != 0)
{
    // Channels 1, 2, 3: build Merkle root now (producer complete)
    block.hashMerkleRoot = block.BuildMerkleTree(vHashes);
}
// Channel 0: hashMerkleRoot left at 0 until ProcessBlock()
```

---

## 10. Field Ownership Table

Which function is responsible for setting each field of a `TritiumBlock`?

| Field | Set By | When | Notes |
|---|---|---|---|
| `nVersion` | `CreateBlock()` / `CreateStakeBlock()` | Before template | Version switch based on timestamp |
| `hashPrevBlock` | `AddBlockData()` | Template creation | = `tStateBest.GetHash()` |
| `hashMerkleRoot` | `AddBlockData()` (ch 1,2,3) or `ProcessBlock()` (ch 0) | Template or post-solve | See Merkle timing above |
| `nChannel` | `AddBlockData()` | Template creation | 0–3 |
| `nHeight` | `AddBlockData()` | Template creation | `tStateBest.nHeight + 1` (UNIFIED) |
| `nBits` | `AddBlockData()` | Template creation | `GetNextTargetRequired()` |
| `nNonce` | `AddBlockData()` (sets to 1), mining loop (increments) | Template → solve | Prime channel also fills `vOffsets` |
| `nTime` | `AddBlockData()` (initial), `UpdateTime()` (loop) | Template → solve | Monotonically increasing |
| `producer` | `CreateTransaction()` + `CreateCoinstake()` / `CreateProducer()` | Before `AddBlockData()` (stake) or after it (mining cache path) | |
| `producer[reward]` | `ProcessBlock()` / miner solve handler | Post-solve | Stake only — reward appended last |
| `vtx` | `AddTransactions()` | Before producer | Mempool transactions |
| `vchBlockSig` | `SignBlock()` / `GenerateSignature()` | Post-solve | Block-level signature |
| `ssSystem` | Node internals | Post-accept | System metadata |

---

## 11. What Must Never Change After AddBlockData()

Once `AddBlockData()` has been called, the following fields are **immutable**. Changing them invalidates the block's proof of work or causes consensus rejection:

| Field | Why Immutable |
|---|---|
| `block.nHeight` | Baked into 216-byte template at bytes[200-203]. Changing it changes the hash input for Prime (ProofHash covers nVersion→nBits range including nHeight). |
| `block.hashPrevBlock` | The chain-link anchor. Changing it disconnects the block from the chain. Also baked into template. |
| `block.nBits` | Defines the difficulty target the miner is solving against. |
| `block.nChannel` | The channel routing for reward calculation and difficulty adjustment. |
| `block.nVersion` | Part of the serialized header used in all hash computations. |

**Safe to change after AddBlockData():**
- `block.nNonce` — this IS the mining variable
- `block.nTime` — updated each second via `UpdateTime()`
- `block.vOffsets` — Prime channel cluster offsets, filled during mining
- `block.producer` (appending reward) — only in `ProcessBlock()` for channel 0
- `block.hashMerkleRoot` — in `ProcessBlock()` for channel 0 after reward known

---

## 12. Mining Channel Application of the Pattern

When implementing a new mining channel or debugging an existing one, use this checklist against the 8-phase pattern:

### Checklist: New Channel Implementation

```
□ Phase 1: hashLastBlock snapshot
      hashLastBlock = ChainState::hashBestChain.load()
      BEFORE any block construction

□ Phase 3: Template creation via CreateBlock()
      nChannel must be 1, 2, or 3 (not 0 — stake only)
      AddBlockData() called with correct nChannel

□ Height verification:
      block.nHeight == tStateBest.nHeight + 1
      (will be checked by Accept() — verify BEFORE submitting)

□ Phase 4: Producer operation
      Channel 1 or 2: OP::COINBASE with valid reward recipient
      Channel 3:      OP::AUTHORIZE with hashPrevTx + hashGenesis

□ Phase 6: Mining loop
      Guard 1: loop condition checks hashLastBlock == hashBestChain
      Break and regenerate template if chain advances

□ Phase 7: Finalization
      hashMerkleRoot MUST be valid before submission
      (Already set by AddBlockData() for ch 1,2,3)
      vchBlockSig MUST be set (GenerateSignature)

□ Phase 8: Guard 2 before Process()
      block.hashPrevBlock == ChainState::hashBestChain.load()
      If not equal: log stale, do NOT call Process()

□ Byte order of template:
      All multi-byte integers written as Little-Endian
      uint32_t fields: LE 4 bytes
      uint64_t fields: LE 8 bytes
      uint512_t (hashMerkleRoot): LE 64 bytes
      uint1024_t (hashPrevBlock): LE 128 bytes
```

---

## 13. Invariants and Consensus Rules

These are the hard invariants enforced by `TritiumBlock::Accept()` that every block producer must satisfy:

| # | Invariant | Code Location |
|---|---|---|
| 1 | `block.nHeight == statePrev.nHeight + 1` | `tritium.cpp` Accept() |
| 2 | `statePrev == ReadBlock(block.hashPrevBlock)` | `tritium.cpp` Accept() |
| 3 | `block.hashMerkleRoot == BuildMerkleTree(vtx + producer)` | `tritium.cpp` Accept() |
| 4 | `block.nBits == GetNextTargetRequired(statePrev, nChannel)` | `tritium.cpp` Accept() |
| 5 | `block.nTime >= statePrev.nTime + 1` | `tritium.cpp` Accept() |
| 6 | `block.producer` signature valid for block version | `tritium.cpp` Accept() |
| 7 | `producer[0]` is correct OP for channel | `tritium.cpp` Accept() |
| 8 | Channel 0: `StakeHash() <= nHashTarget` with correct nNonce | `tritium.cpp` Accept() |
| 9 | Channel 1,2: `ProofHash() <= nHashTarget` | `tritium.cpp` Accept() |
| 10 | `block.vchBlockSig` valid block signature | `tritium.cpp` Accept() |

**Invariant 1 is the unified height contract — all others depend on it being correct first.**

---

## 14. File and Function Index

| File | Function | Role |
|---|---|---|
| `src/TAO/Ledger/create.cpp` | `CreateGenesis()` | Boot genesis block initialization |
| `src/TAO/Ledger/create.cpp` | `AddBlockData()` | **Core template population** — sets nHeight, hashPrevBlock, nBits, nNonce, nTime, nChannel, hashMerkleRoot (non-stake) |
| `src/TAO/Ledger/create.cpp` | `CreateBlock()` | Mining channels (1,2,3) template with cache |
| `src/TAO/Ledger/create.cpp` | `CreateStakeBlock()` | Stake (channel 0) template — no Merkle |
| `src/TAO/Ledger/create.cpp` | `CreateProducer()` | Mining channel producer (COINBASE/AUTHORIZE) |
| `src/TAO/Ledger/create.cpp` | `CreateTransaction()` | Base producer/sigchain transaction |
| `src/TAO/Ledger/create.cpp` | `AddTransactions()` | Mempool tx selection into block |
| `src/TAO/Ledger/stake_minter.cpp` | `CreateCandidateBlock()` | Stake: calls CreateStakeBlock + CreateCoinstake |
| `src/TAO/Ledger/stake_minter.cpp` | `HashBlock()` | **Guard 1 loop** — hashing with staleness check |
| `src/TAO/Ledger/stake_minter.cpp` | `ProcessBlock()` | Stake finalization: reward, Merkle, sign, **Guard 2**, submit |
| `src/TAO/Ledger/stake_minter.cpp` | `CheckInterval()` | Minimum block interval check (unified heights) |
| `src/TAO/Ledger/stake_minter.cpp` | `FindTrustAccount()` | Sets `fGenesis` flag |
| `src/TAO/Ledger/tritium_minter.cpp` | `StakeMinterThread()` | Main minting loop, sets `hashLastBlock` |
| `src/TAO/Ledger/tritium_minter.cpp` | `CreateCoinstake()` | OP::TRUST or OP::GENESIS producer fill; loads `stateLast` via `ReadBlock()` |
| `src/TAO/Ledger/tritium_minter.cpp` | `CalculateCoinstakeReward()` | Deferred reward calc at solve time |
| `src/TAO/Ledger/genesis_block.cpp` | `LegacyGenesis()` | Genesis block: nHeight=0 |
| `src/TAO/Ledger/genesis_block.cpp` | `TritiumGenesis()` | Client genesis: nHeight=2,944,207 |
| `src/TAO/Ledger/genesis_block.cpp` | `HybridGenesis()` | Private network genesis |
| `src/TAO/Ledger/tritium.cpp` | `TritiumBlock::Accept()` | Consensus validation including height invariant |
| `src/TAO/Ledger/tritium.cpp` | `TritiumBlock::UpdateTime()` | Monotonic time advance during mining |

---

## Appendix A: Confirmed Findings (PR #278 Audit)

| Finding | Status |
|---|---|
| `AddBlockData()` sets `block.nHeight = tStateBest.nHeight + 1` (UNIFIED) | ✅ Fixed by PR #278 |
| `CheckInterval()` now computes `block.nHeight - stateLast.nHeight` where both are unified | ✅ Side-fixed by PR #278 |
| `stateLast` is loaded via `LLD::Ledger->ReadBlock(hashLast, ...)` — NOT `GetLastState()` | ✅ Confirmed correct |
| `hashLastBlock` (StakeMinter Guard 1) is set at the top of each minting iteration | ✅ Confirmed correct |
| Merkle root correctly deferred for channel 0, built in `AddBlockData()` for channels 1–3 | ✅ Correct by design |
| `TritiumGenesis()` nHeight=2,944,207 is a snapshot anchor, `AddBlockData()` always adds `+1` | ✅ Correct |
| Guard 2 (`hashPrevBlock != hashBestChain`) check present in `ProcessBlock()` before `Process()` | ✅ Confirmed correct |

## Appendix B: Known Follow-up Items

| Item | Priority | Description |
|---|---|---|
| Guard 1 for Stateless Lane | 🔴 Medium | `MiningContext::hashLastBlock` (added in PR #278 infrastructure) not yet wired into a staleness loop in the stateless miner connection |
| `nChannelHeight=0` in result structs | 🟡 Medium | Callers of `BlockValidationResult`, `BlockAcceptanceResult`, `SubmitResult` must not use `.nChannelHeight` for routing logic |

---

*Document generated from STATELESS-NODE branch audit, 2026-02-24.*  
*Sources: `create.cpp`, `stake_minter.cpp`, `tritium_minter.cpp`, `genesis_block.cpp`, `tritium.cpp`*