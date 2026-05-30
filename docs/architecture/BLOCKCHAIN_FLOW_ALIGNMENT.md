# Nexus Tritium Blockchain Flow Alignment
## Unified Height Contract · Stake as Reference · Mining Channel Template

**Branch:** `STATELESS-NODE`  
**Canonical PRs:** LLL-TAO #274, #278 · NexusMiner #167, #169, #170  
**Last updated:** 2026-02-24

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [The Height Duality Problem](#2-the-height-duality-problem)
3. [Genesis Block Anchor — Chain Boot Sequence](#3-genesis-block-anchor--chain-boot-sequence)
4. [Diagram A — Full Chain Boot Sequence](#diagram-a--full-chain-boot-sequence)
5. [Stake Minter — Reference Implementation Flow](#5-stake-minter--reference-implementation-flow)
6. [Diagram B — StakeMinter Thread State Machine](#diagram-b--stakeminter-thread-state-machine)
7. [Diagram C — Block Template Construction (All Channels)](#diagram-c--block-template-construction-all-channels)
8. [Diagram D — Height Field Lifecycle](#diagram-d--height-field-lifecycle)
9. [The Unified Height Contract](#9-the-unified-height-contract)
10. [Mining Channel Template (Channels 1, 2, 3)](#10-mining-channel-template-channels-1-2-3)
11. [Diagram E — Staleness Guard Layers](#diagram-e--staleness-guard-layers)
12. [Byte Layout of the 216-byte Block Template](#12-byte-layout-of-the-216-byte-block-template)
13. [Invariants and Verification Checklist](#13-invariants-and-verification-checklist)
14. [Field Reference Table](#14-field-reference-table)

---

## 1. Executive Summary

The Nexus Tritium chain uses **three parallel mining channels** plus **Proof of Stake**, each producing blocks at their own rate, all anchored to a single **unified blockchain height**. A former bug treated channel-specific height (e.g., Prime channel block #2,329,307) as the value for `block.nHeight`, while the consensus rule (`TritiumBlock::Accept()`) and the miner protocol both expected the **unified chain height** (e.g., block #6,604,692). This document records:

- How the genesis block anchors `tStateBest.nHeight` for the entire chain
- How `StakeMinter` uses `AddBlockData()` as the canonical template builder (reference path)
- How mining channels (Prime=1, Hash=2, Private=3) follow the identical pattern
- The two staleness guards that prevent stale blocks from being submitted
- The exact byte layout of the 216-byte template sent to miners

The **Unified Height Contract** is the central invariant:

```
block.nHeight  ==  ChainState::tStateBest.nHeight + 1
```

It must hold for every block on every channel, at the moment `AddBlockData()` is called.

---

## 2. The Height Duality Problem

### Two Heights — One Must Stay Out of the Block Template

Every `BlockState` on disk carries **two** height fields:

| Field | Type | Meaning | In Block Template? |
|---|---|---|---|
| `nHeight` | `uint32_t` | Unified chain height (all channels combined) | ✅ YES — baked into 216 bytes |
| `nChannelHeight` | `uint32_t` | Per-channel block count (Prime/Hash/Stake separately) | ❌ NO — computed post-acceptance |

`nChannelHeight` is computed by `BlockState::SetBest()` → `GetLastState(state, nChannel)` **after** a block is accepted. It is metadata for analytics and ambassador/developer payout thresholds. It is **never** serialized into the 216-byte template.

### The Bug (Pre-PR #278)

```cpp
// BEFORE — WRONG:
BlockState stateChannel = tStateBest;
GetLastState(stateChannel, nChannel);          // walk to last block on this channel
block.nHeight = stateChannel.nChannelHeight + 1;  // e.g., 2,329,307 for Prime
// TritiumBlock::Accept() expects: statePrev.nHeight + 1 = 6,604,692
// ❌ MISMATCH → every mined block rejected
```

```cpp
// AFTER — CORRECT (PR #278):
block.nHeight = tStateBest.nHeight + 1;  // e.g., 6,604,692
// TritiumBlock::Accept(): statePrev.nHeight + 1 = 6,604,692 ✅ MATCH
```

---

## 3. Genesis Block Anchor — Chain Boot Sequence

There are **three genesis block variants** depending on node mode:

### 3a. LegacyGenesis (Full Node — Mainnet & Testnet)

```
File: src/TAO/Ledger/genesis_block.cpp

LegacyGenesis():
  block.nHeight        = 0          ← Unified chain starts at ZERO
  block.nChannelHeight = 1          ← Channel counter starts at 1 (sentinel)
  block.nChannel       = 2          ← Hash channel produced the genesis block
  block.hashPrevBlock  = 0          ← No predecessor (root of chain)
  block.nTime          = 1409456199 ← 2014-08-30 (Nexus launch)
```

### 3b. TritiumGenesis (Client Mode — Mainnet)

```
File: src/TAO/Ledger/genesis_block.cpp

TritiumGenesis():
  block.nHeight        = 2,944,207   ← Snapshot at Tritium activation
  block.hashPrevBlock  = 0xeacd7b...  ← Hash of Legacy era tip at that point
  block.nChannel       = (various)    ← Hardcoded from chain history
  block.nChannelHeight = (various)    ← Hardcoded from chain history
```

### 3c. HybridGenesis (Private / Hybrid Networks)

```
File: src/TAO/Ledger/genesis_block.cpp

HybridGenesis():
  block.nHeight        = 0
  block.hashPrevBlock  = 0
  block.nNonce         = (computed via PoW loop)
```

### 3d. CreateGenesis() Boot Logic

```
File: src/TAO/Ledger/create.cpp  Lines: 759–831

CreateGenesis()
  ├─ hashGenesis = ChainState::Genesis()          ← hardcoded 1024-bit value
  ├─ ReadBlock(hashGenesis, tStateGenesis)?
  │    YES → genesis already on disk, skip
  │    NO  → write the appropriate genesis variant
  │           then:
  │             ChainState::tStateGenesis = state
  │             ChainState::hashBestChain = hashGenesis
  │             ChainState::tStateBest    = tStateGenesis ← THE ANCHOR
  └─ After full sync: tStateBest.nHeight ≈ 6,604,691
```

**After full sync from genesis, `tStateBest.nHeight` advances with every accepted block. The next block on any channel will always be `tStateBest.nHeight + 1`.**

---

## Diagram A — Full Chain Boot Sequence

```
NODE STARTUP
     │
     ▼
┌─────────────────────────────────────────────────────────┐
│  CreateGenesis()                                        │
│                                                         │
│  ┌──────────────┐   NO    ┌────────────────────────┐   │
│  │ ReadBlock(   │────────►│ Select Genesis Variant  │   │
│  │ hashGenesis) │         │                         │   │
│  └──────┬───────┘         │  fClient + !fTestNet?   │   │
│         │ YES             │  → TritiumGenesis()     │   │
│         │                 │    nHeight=2,944,207    │   │
│         ▼                 │                         │   │
│  ┌──────────────┐         │  fHybrid?               │   │
│  │ Already on   │         │  → HybridGenesis()      │   │
│  │ disk — skip  │         │    nHeight=0            │   │
│  └──────────────┘         │                         │   │
│                           │  else (full node):      │   │
│                           │  → LegacyGenesis()      │   │
│                           │    nHeight=0            │   │
│                           └──────────┬──────────────┘   │
│                                      │                   ���
│                                      ▼                   │
│                           ┌──────────────────────┐       │
│                           │ WriteBlock(genesis)   │       │
│                           │ WriteBestChain(hash)  │       │
│                           │                       │       │
│                           │ tStateGenesis = state │       │
│                           │ hashBestChain = hash  │       │
│                           │ tStateBest    = state │◄──── ANCHOR SET
│                           └──────────────────────┘       │
└─────────────────────────────────────────────────────────┘
     │
     ▼
SYNC LOOP (peer connections)
     │
     │  For each block received from peers:
     │    Process(block) → TritiumBlock::Accept()
     │      → BlockState::SetBest()
     │           tStateBest.nHeight advances by 1
     │           tStateBest.nChannelHeight computed per-channel
     │           hashBestChain = new block hash
     │
     ▼
STEADY STATE
  tStateBest.nHeight   ≈ 6,604,691   (unified)
  tStateBest.nChannel  = last accepted channel
  hashBestChain        = hash of last accepted block
     │
     ▼
MINING / STAKING BEGINS
  AddBlockData(tStateBest, nChannel, block)
    block.nHeight = tStateBest.nHeight + 1  ← 6,604,692
```

---

## 5. Stake Minter — Reference Implementation Flow

The Stake Minter (`TritiumMinter`) is the **canonical reference** for correct Tritium block production. Every step it takes is the correct template for all other mining channels.

### 5a. Thread Lifecycle

```
TritiumMinter::Start()
  └─► StakeMinterThread launched on dedicated thread
        │
        ├─ WAIT: until !Synchronizing() && connections > 0
        │
        └─► MAIN LOOP (every ~1 second):
              │
              ├─ hashLastBlock = hashBestChain.load()   ← GUARD 1 ANCHOR
              ├─ CheckUser()
              ├─ FindTrustAccount()     → sets fGenesis flag
              ├─ CreateCandidateBlock() → builds block template
              ├─ CalculateWeights()     → trust/block weights
              └─ MintBlock()
                    ├─ CheckInterval()  (Trust only)
                    ├─ CheckMempool()   (Genesis only)
                    └─ HashBlock()      → GUARD 1 loop + GUARD 2 at submit
```

### 5b. hashLastBlock — Guard 1 Snapshot

```cpp
// tritium_minter.cpp:397
pTritiumMinter->hashLastBlock = TAO::Ledger::ChainState::hashBestChain.load();
```

This snapshot is taken **once per outer loop iteration**, before `CreateCandidateBlock()`. It becomes the Guard 1 sentinel:

```cpp
// stake_minter.cpp:534-535  — HashBlock() inner loop
while(!fStop.load() && !fShutdown.load()
    && hashLastBlock == ChainState::hashBestChain.load())
{
    // mining continues only while chain tip hasn't moved
}
```

If a new block arrives from the network, `hashBestChain` changes, the Guard 1 loop exits, and the outer loop restarts with a fresh `hashLastBlock` snapshot.

### 5c. CreateCandidateBlock() → CreateStakeBlock()

```
StakeMinter::CreateCandidateBlock()
  │
  ├─ block = TritiumBlock()                    ← fresh empty block
  │
  ├─ CreateStakeBlock(user, pin, block, fGenesis)
  │     │
  │     ├─ nChannel = 0                         ← Stake channel
  │     ├─ block.SetNull()
  │     ├─ block.nVersion = CurrentBlockVersion()
  │     ├─ tStateBest = ChainState::tStateBest.load()  ← ATOMIC SNAPSHOT
  │     │
  │     ├─ if(!fGenesis || fPoolStaking)
  │     │     AddTransactions(block)             ← mempool tx selection
  │     │
  │     ├─ CreateTransaction(user, pin, rProducer)  ← bare sigchain tx
  │     ├─ UpdateProducerTimestamp(rProducer)
  │     ├─ block.producer = rProducer
  │     │
  │     │  NOTE: Producer[0] NOT set yet (coinstake op added by CreateCoinstake)
  │     │  NOTE: hashMerkleRoot NOT set yet (deferred — nReward unknown)
  │     │
  │     └─ AddBlockData(tStateBest, nChannel=0, block)
  │           ├─ if(nChannel != 0): build Merkle  ← SKIPPED for stake
  │           ├─ block.hashPrevBlock = tStateBest.GetHash()  ← STALENESS ANCHOR
  │           ├─ block.nChannel      = 0
  │           ├─ block.nHeight       = tStateBest.nHeight + 1  ← UNIFIED
  │           ├─ block.nBits         = GetNextTargetRequired(tStateBest, 0, false)
  │           ├─ block.nNonce        = 1
  │           └─ block.nTime         = max(prevTime+1, unifiedtimestamp())
  │
  └─ CreateCoinstake(hashGenesis)
        ├─ if(!fGenesis):
        │     stateLast ← LLD::Ledger->ReadBlock(hashLast, stateLast)
        │     block.producer[0] << OP::TRUST << hashLast << nTrust << nStakeChange
        └─ if(fGenesis):
              block.producer[0] << OP::GENESIS
        (nReward NOT added yet — deferred to ProcessBlock())
```

### 5d. ProcessBlock() — Finalization

```
StakeMinter::ProcessBlock()
  │
  ├─ nReward = CalculateCoinstakeReward(block.GetBlockTime())
  ├─ block.producer[0] << nReward           ← NOW coinstake is complete
  ├─ block.producer.Build()                 ← pre/post state execution
  │
  ├─ vHashes = all vtx hashes + producer.GetHash()
  ├─ block.hashMerkleRoot = BuildMerkleTree(vHashes)  ← NOW Merkle is set
  │
  ├─ block.producer.Sign(credentials)
  ├─ SignBlock(credentials, pin)            ← block signature
  │
  ├─ GUARD 2: if(block.hashPrevBlock != hashBestChain.load())
  │              → "Generated block is stale" — ABORT
  │
  └─ TAO::Ledger::Process(block, nStatus)   ← submit to consensus
        └─ if(ACCEPTED): relay to network
```

---

## Diagram B — StakeMinter Thread State Machine

```
                    ┌─────────────────────────────────────────────────────┐
                    │              StakeMinterThread                       │
                    │                                                      │
                    │  ┌──────────────────────────────────────────────┐   │
                    │  │  INITIALIZATION PHASE                        │   │
                    │  │  Wait: Synchronizing() || no connections     │   │
                    │  └─────────────────┬────────────────────────────┘   │
                    │                    │ synced + connected              │
                    │                    ▼                                 │
  ┌─────────────────►  ┌──────────────────────────────────────────────┐   │
  │  (1 sec sleep)  │  │  OUTER LOOP ITERATION                        │   │
  │                 │  │                                              │   │
  │                 │  │  1. hashLastBlock = hashBestChain.load() ◄──┼───── GUARD 1 ANCHOR
  │                 │  │  2. CheckUser()         → break if locked    │   │
  │                 │  │  3. FindTrustAccount()  → sets fGenesis      │   │
  │                 │  │  4. CreateCandidateBlock()                   │   │
  │                 │  │       └─ CreateStakeBlock()                  │   │
  │                 │  │            └─ AddBlockData()                 │   │
  │                 │  │                 block.nHeight = N+1 ◄────────┼───── UNIFIED HEIGHT
  │                 │  │                 block.hashPrevBlock = tip ◄──┼───── STALENESS ANCHOR
  │                 │  │       └─ CreateCoinstake()                   │   │
  │                 │  │            └─ OP::GENESIS or OP::TRUST       │   │
  │                 │  │  5. CalculateWeights()                       │   │
  │                 │  │  6. MintBlock()                              │   │
  │                 │  │       ├─ CheckInterval() (Trust)             │   │
  │                 │  │       ├─ CheckMempool()  (Genesis)           │   │
  │                 │  │       └─ HashBlock() ───────────────────────►│   │
  │                 │  └──────────────────────────────────────────────┘   │
  │                 │                                                      │
  │                 │  ┌──────────────────────────────────────────────┐   │
  │                 │  │  HashBlock() INNER LOOP                      │   │
  │                 │  │                                              │   │
  │                 │  │  while(hashLastBlock == hashBestChain) {     │◄──── GUARD 1 CHECK
  │                 │  │    UpdateTime()                              │   │
  │                 │  │    CheckStale() → break if producer stale    │   │
  │                 │  │    nThreshold = GetCurrentThreshold(...)     │   │
  │                 │  │    if(threshold < required) → sleep(10ms)    │   │
  │                 │  │    hashProof = StakeHash()                   │   │
  │                 │  │    if(hashProof <= nHashTarget) {            │   │
  │                 │  │      ProcessBlock()                          │   │
  │                 │  │        ├─ Finalize coinstake + Merkle        │   │
  │                 │  │        ├─ Sign block                         │   │
  │                 │  │        ├─ GUARD 2: hashPrevBlock check ◄─────┼───── STALENESS GUARD 2
  │                 │  │        └─ Process(block) → network           │   │
  │                 │  │      break                                   │   │
  │                 │  │    }                                         │   │
  │                 │  │    ++nNonce                                  │   │
  │                 │  │  }                                           │   │
  │                 │  └──────────────────┬───────────────────────────┘   │
  └─────────────────────────────────────── ┘ loop back                   │
                    └─────────────────────────────────────────────────────┘
```

---

## Diagram C — Block Template Construction (All Channels)

```
  AddBlockData(tStateBest, nChannel, block)
  ═══════════════════════════════════════════════════════════════════

                          nChannel == 0?
                         (Stake / PoS)
                        /              \
                      YES               NO
                       │                │
                       │          Build Merkle Root NOW
                       │          (channels 1, 2, 3 have
                       │           complete producer)
                       │
               Merkle DEFERRED
               (coinstake nReward
                not yet known)
                       │
                       └──────────────────────────────────┐
                                                           │
  ┌──────────────────────────────────────────────────────┐ │
  │  COMMON FIELDS (ALL CHANNELS)                        │ │
  │                                                      │ │
  │  block.hashPrevBlock = tStateBest.GetHash()          │◄┘
  │     └─► PRIMARY STALENESS ANCHOR                     │
  │         Compared at submit time vs hashBestChain     │
  │                                                      │
  │  block.nChannel = nChannel                           │
  │     0=Stake  1=Prime  2=Hash  3=Private              │
  │                                                      │
  │  block.nHeight = tStateBest.nHeight + 1              │
  │     └─► UNIFIED BLOCKCHAIN HEIGHT                    │
  │         Must match TritiumBlock::Accept() check      │
  │         statePrev.nHeight + 1 == block.nHeight       │
  │                                                      │
  │  block.nBits  = GetNextTargetRequired(tStateBest,    │
  │                                       nChannel, false)│
  │                                                      │
  │  block.nNonce = 1                                    │
  │                                                      │
  │  block.nTime  = max(prevBlockTime+1,                 │
  │                     unifiedtimestamp())              │
  └──────────────────────────────────────────────────────┘


  CHANNEL-SPECIFIC PRODUCER CONTENTS:
  ════════════════════════════════════

  Channel 0 (Stake/PoS):
    producer[0] = (bare tx, no op yet)
    → CreateCoinstake() adds:  OP::GENESIS  or  OP::TRUST << hashLast << nTrust
    → ProcessBlock()   adds:  << nReward  (deferred until final block time known)

  Channel 1 (Prime) / Channel 2 (Hash):
    producer[0] = OP::COINBASE << hashRecipient << nBlockReward << nExtraNonce
    → Optional: ambassador/developer payout contracts at interval thresholds

  Channel 3 (Private/Authorize):
    producer[0] = OP::AUTHORIZE << hashPrevTx << hashGenesis
```

---

## Diagram D — Height Field Lifecycle

```
  GENESIS BOOT
  ┌────────────────────────────────────────────────────────┐
  │ LegacyGenesis:  block.nHeight = 0                      │
  │ TritiumGenesis: block.nHeight = 2,944,207              │
  │ HybridGenesis:  block.nHeight = 0                      │
  │                                                        │
  │ → ChainState::tStateBest.nHeight = genesis.nHeight     │
  └────────────────────────────────────────────────────────┘
                           │
                           │ +1 per accepted block (all channels combined)
                           ▼
  SYNC / NORMAL OPERATION
  ┌────────────────────────────────────────────────────────┐
  │ Block accepted by TritiumBlock::Accept():              │
  │   1. Look up statePrev = ReadBlock(hashPrevBlock)      │
  │   2. ASSERT: statePrev.nHeight + 1 == block.nHeight    │
  │   3. BlockState::SetBest():                            │
  │        new_state.nHeight        = block.nHeight        │ ← unified
  │        new_state.nChannelHeight = prev_channel + 1     │ ← per-channel
  │        ChainState::tStateBest   = new_state            │
  │        ChainState::hashBestChain = new_state.GetHash() │
  └────────────────────────────────────────────────────────┘
                           │
                           ▼
  TEMPLATE CREATION
  ┌────────────────────────────────────────────────────────┐
  │ AddBlockData(tStateBest, nChannel, block):             │
  │   block.nHeight = tStateBest.nHeight + 1               │ ← unified
  │   (nChannelHeight is NOT touched here)                 │
  └────────────────────────────────────────────────────────┘
                           │
                           │ template sent to miner / stake hash loop
                           ▼
  BLOCK SUBMISSION
  ┌────────────────────────────────────────────────────────┐
  │ TritiumBlock::Accept():                                │
  │   statePrev = ReadBlock(block.hashPrevBlock)           │
  │   CHECK: statePrev.nHeight + 1 == block.nHeight  ✅    │
  └────────────────────────────────────────────────────────┘
                           │
                           ▼
  POST-ACCEPTANCE (BlockState::SetBest)
  ┌────────────────────────────────────────────────────────┐
  │   GetLastState(stateChannel, nChannel)                 │
  │   new_state.nChannelHeight = stateChannel.nChannelHeight + 1  │
  │   (THIS is where nChannelHeight is computed — AFTER acceptance) │
  └────────────────────────────────────────────────────────┘
```

---

## 9. The Unified Height Contract

### Contract Definition

```
∀ block B produced by AddBlockData(tStateBest, nChannel, B):

  B.nHeight        == tStateBest.nHeight + 1
  B.hashPrevBlock  == tStateBest.GetHash()
  B.nChannel       == nChannel  ∈ {0, 1, 2, 3}
  B.nBits          == GetNextTargetRequired(tStateBest, nChannel, false)

  And upon acceptance:
    statePrev = ReadBlock(B.hashPrevBlock)
    statePrev.nHeight + 1 == B.nHeight    ← consensus enforcement
```

### Why `nChannelHeight` Must Never Enter `block.nHeight`

1. **`TritiumBlock::Accept()` uses unified height** — it reads `statePrev` via `hashPrevBlock`, which is the unified chain tip. `statePrev.nHeight` is always the unified height.

2. **`nChannelHeight` is not in the block bytes** — it is computed by `SetBest()` *after* the block is accepted. If you put channel height into `block.nHeight`, you get two different values that can never match.

3. **The miner validates the same contract** — NexusMiner PR #170 `ValidateTemplate()` checks:
   ```
   block.nHeight == nNodeUnified + 1
   ```
   where `nNodeUnified` is `tStateBest.nHeight`. If channel height is used, the miner rejects the template.

4. **`CheckInterval()` depends on unified height** — `block.nHeight - stateLast.nHeight` where `stateLast` is a `BlockState` read from disk, whose `nHeight` is unified. Mixed heights produce incorrect (often nonsensical) intervals.

---

## 10. Mining Channel Template (Channels 1, 2, 3)

Mining channels follow the **identical** `AddBlockData()` path as stake. The differences are:

| Aspect | Stake (Channel 0) | Prime (1) / Hash (2) | Private (3) |
|---|---|---|---|
| `AddBlockData()` call | `CreateStakeBlock()` | `CreateBlock()` | `CreateBlock()` |
| Producer op | `OP::GENESIS` or `OP::TRUST` | `OP::COINBASE` | `OP::AUTHORIZE` |
| Merkle root in template | ❌ Deferred (nReward unknown) | ✅ Immediate | ✅ Immediate |
| Block cache (`tBlockCache[]`) | ❌ Not cached | ✅ Cached per-channel | ✅ Cached |
| Guard 1 | `hashLastBlock == hashBestChain` loop | `hashPrevBlock == hashBestChain` check at submit | Same |
| Guard 2 | `hashPrevBlock` check in `ProcessBlock()` | `hashPrevBlock` check at submit | Same |
| `nHeight` assignment | `tStateBest.nHeight + 1` | `tStateBest.nHeight + 1` | `tStateBest.nHeight + 1` |

### Mining Channel Template Pattern

Any new mining channel implementation MUST:

```cpp
// Step 1: Snapshot chain state atomically
const BlockState tStateBest = ChainState::tStateBest.load();

// Step 2: Build producer transaction
CreateProducer(user, pin, producer, tStateBest, nVersion, nChannel, ...);

// Step 3: Call AddBlockData — this is the ONLY place nHeight is set
AddBlockData(tStateBest, nChannel, block);
//   ↑ sets: hashPrevBlock, nHeight (UNIFIED), nChannel, nBits, nNonce, nTime

// Step 4: Guard 1 — continuous staleness detection
while(hashLastBlock == ChainState::hashBestChain.load()) {
    // hash / solve
}

// Step 5: Guard 2 — final staleness check before submit
if(block.hashPrevBlock != ChainState::hashBestChain.load())
    return false; // STALE — discard

// Step 6: Submit
TAO::Ledger::Process(block, nStatus);
```

### Block Cache Invalidation (Channels 1, 2, 3)

```cpp
// CreateBlock() — four invalidation conditions:
bool fNeedsNewBlock = false;

// PRIMARY: chain advanced
if(hashBestChain != tBlockCached.hashPrevBlock)
    fNeedsNewBlock = true;

// SECONDARY: user/genesis changed
if(hashGenesis != tBlockCached.producer.hashGenesis)
    fNeedsNewBlock = true;

// TERTIARY: safety timeout (default 90 seconds)
if(unifiedtimestamp() >= tBlockCached.producer.nTimestamp + nExpiration)
    fNeedsNewBlock = true;

// QUATERNARY: sigchain advanced since template was cached (PoW channels only)
// Catches the case where a different-channel block connected and advanced
// WriteLast() for the same genesis without triggering the Primary check.
if(!fNeedsNewBlock && (nChannel == 1 || nChannel == 2) && !tBlockCached.producer.IsFirst()) {
    uint512_t hashDiskLast = 0;
    if(LLD::Ledger->ReadLast(tBlockCached.producer.hashGenesis, hashDiskLast)
       && hashDiskLast != tBlockCached.producer.hashPrevTx)
        fNeedsNewBlock = true;
}
```

---

## Diagram E — Staleness Guard Layers

```
  TEMPLATE CREATED AT TIME T₀
  ┌────────────────────────────────────────────────────────────────┐
  │  block.hashPrevBlock = hashBestChain(T₀)                       │
  │  block.nHeight       = tStateBest.nHeight(T₀) + 1             │
  └────────────────────────────────────────────────────────────────┘
                                │
              ┌─────────────────┼──────────────────┐
              │                 │                  │
   ┌──────────▼──────────┐    ┌─▼────────────────┐ │
   │  GUARD 1            │    │  GUARD 1 (Mining) │ │
   │  (Stake Minter)     │    │  hashLastBlock    │ │
   │                     │    │  snapshot in      │ │
   │  HashBlock() loop:  │    │  outer loop       │ │
   │  while(             │    │  compared at      │ │
   │    hashLastBlock ==  │    │  template request │ │
   │    hashBestChain)   │    │  time             │ │
   │    { mine... }      │    └───────────────────┘ │
   │                     │                          │
   │  If new block        │                          │
   │  arrives →          │                          │
   │  hashBestChain ≠    │                          │
   │  hashLastBlock →    │                          │
   │  loop exits,        │                          │
   │  rebuild block      │                          │
   └─────────────────────┘                          │
                                                    │
   ┌──────────────────────────────────────���─────────▼──────────┐
   │  GUARD 2 — Pre-Submit Check (ALL channels)                │
   │                                                           │
   │  if(block.hashPrevBlock != hashBestChain.load())          │
   │      return false;  // STALE — new block arrived between  │
   │                     // Guard 1 exit and submission        │
   │                                                           │
   │  This catches the race condition:                         │
   │    T₁: hash found / threshold exceeded                    │
   │    T₂: new network block arrives                          │
   │    T₃: submit attempted → BLOCKED by Guard 2             │
   └───────────────────────────────────────────────────────────┘
```

---

## 12. Byte Layout of the 216-byte Block Template

The block template serialized between node and miner is exactly 216 bytes (for Tritium-era blocks, nVersion ≥ 7). The fields in wire order:

```
Offset  Size  Field             Type       Notes
──────  ────  ────────────────  ─────────  ────────────────────────────────────────
  0       4   nVersion          uint32_t   Block version (LE)
  4     128   hashPrevBlock     uint1024_t 1024-bit previous block hash (LE)
132       4   nChannel          uint32_t   0=Stake,1=Prime,2=Hash,3=Private (LE)
136       4   nHeight           uint32_t   UNIFIED blockchain height (LE) ← KEY FIELD
140       4   nBits             uint32_t   Packed difficulty target (LE)
144       8   nNonce            uint64_t   Miner sets this during solving (LE)
152      64   hashMerkleRoot    uint512_t  Merkle root of all transactions (LE)

Total: 216 bytes
```

**Critical invariants for `nHeight` (bytes 136–139, Little-Endian uint32_t):**

- Set ONLY by `AddBlockData()` as `tStateBest.nHeight + 1`
- Never overwritten after serialization
- Read by miner in `ValidateTemplate()` as `nHeight == nNodeUnified + 1`
- Checked by `TritiumBlock::Accept()` as `statePrev.nHeight + 1 == nHeight`
- `nChannelHeight` is NOT in this layout — it is DB metadata only

---

## 13. Invariants and Verification Checklist

Use this checklist when reviewing any new block production code:

### Genesis / Boot Invariants

- [ ] `CreateGenesis()` sets `ChainState::tStateBest = tStateGenesis` ✅
- [ ] `ChainState::hashBestChain` is set to genesis hash ✅
- [ ] `tStateGenesis.nHeight` correctly reflects the anchor height for the chain type ✅
- [ ] After full sync, `tStateBest.nHeight` equals the actual current chain height ✅

### Template Creation Invariants

- [ ] `block.nHeight` is set via `AddBlockData()` as `tStateBest.nHeight + 1` ✅
- [ ] `block.hashPrevBlock` is set via `AddBlockData()` as `tStateBest.GetHash()` ✅
- [ ] `tStateBest` is taken as a single atomic snapshot before any block fields are set ✅
- [ ] `nChannelHeight` is NOT written to `block.nHeight` ❌ (would be a bug)
- [ ] `block.nChannel` is set to the correct channel ID ✅
- [ ] For stake (channel 0): Merkle root is intentionally deferred ✅
- [ ] For channels 1–3: Merkle root is built in `AddBlockData()` ✅

### Staleness Guard Invariants

- [ ] Guard 1: `hashLastBlock` snapshot is taken at the START of each outer loop iteration ✅
- [ ] Guard 1: The hash loop condition compares `hashLastBlock == hashBestChain.load()` ✅
- [ ] Guard 2: `block.hashPrevBlock != hashBestChain.load()` check immediately before `Process()` ✅
- [ ] Guard 2 result: return `false` (stale), not `error()` (not a bug, just network race) ✅

### Consensus Invariants

- [ ] `TritiumBlock::Accept()` will find `statePrev` via `hashPrevBlock` ✅
- [ ] `statePrev.nHeight + 1 == block.nHeight` (unified heights agree) ✅
- [ ] `statePrev` IS the same block as `tStateBest` at template creation time ✅

### Stake-Specific Invariants

- [ ] `stateLast` is loaded via `LLD::Ledger->ReadBlock(hashLast, stateLast)` (by block hash, not `GetLastState()`) ✅
- [ ] `CheckInterval()` computes `block.nHeight - stateLast.nHeight` where both are unified ✅
- [ ] `fGenesis` (trust account flag) is orthogonal to blockchain genesis ✅
- [ ] `block.producer[0] << nReward` is added in `ProcessBlock()`, not before ✅

---

## 14. Field Reference Table

| Symbol | Type | Where Set | Meaning |
|---|---|---|---|
| `tStateBest` | `BlockState` | `ChainState::tStateBest.load()` | Atomic snapshot of current best chain state |
| `hashBestChain` | `uint1024_t` | `ChainState::hashBestChain.load()` | Hash of current chain tip |
| `block.nHeight` | `uint32_t` | `AddBlockData()` only | **Unified** block height = `tStateBest.nHeight + 1` |
| `block.hashPrevBlock` | `uint1024_t` | `AddBlockData()` only | Hash of chain tip at template creation |
| `block.nChannel` | `uint32_t` | `AddBlockData()` only | 0=Stake, 1=Prime, 2=Hash, 3=Private |
| `block.nBits` | `uint32_t` | `AddBlockData()` only | Packed difficulty for this channel |
| `block.nNonce` | `uint64_t` | Init=1 in `AddBlockData()`, miner increments | Proof-of-work/stake nonce |
| `block.hashMerkleRoot` | `uint512_t` | Channels 1–3: `AddBlockData()`; Channel 0: `ProcessBlock()` | Merkle root of all tx including producer |
| `stateLast` | `BlockState` | `LLD::Ledger->ReadBlock(hashLast, stateLast)` | Block containing user's last stake tx |
| `stateLast.nHeight` | `uint32_t` | On-disk unified height | Used in `CheckInterval()` — always unified |
| `nChannelHeight` | `uint32_t` | `BlockState::SetBest()` post-acceptance | Per-channel counter — NEVER in block.nHeight |
| `hashLastBlock` | `uint1024_t` | Snapshot at loop start (`hashBestChain.load()`) | Guard 1 sentinel for stale detection |
| `fGenesis` | `bool` | `StakeMinter::FindTrustAccount()` | True if user has never staked (first stake tx) |
| `fGenesis` (chain) | n/a | Compile-time genesis block functions | Blockchain genesis — orthogonal to stake fGenesis |

---

*Document generated from codebase audit of `NamecoinGithub/LLL-TAO` branch `STATELESS-NODE` commit `791871609e3509fd22dd418ca273ab6dd9ac686f` and `NamecoinGithub/NexusMiner` PR #167.*