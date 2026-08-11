# Nexus Blockchain Flow Alignment: Unified Height Architecture

**Branch:** `STATELESS-NODE`  
**Document Version:** 1.0 — 2026-02-24 08:03:04  
**Scope:** `TAO::Ledger` — `create.cpp`, `stake_minter.cpp`, `tritium_minter.cpp`, `genesis_block.cpp`

---

## 1. Purpose

This document defines the **canonical block production flow** for the Nexus Tritium protocol, using the **Proof-of-Stake (nPoS) channel** as the primary reference implementation. It:

1. Establishes the **Unified Height Contract** — the single rule all channels must follow  
2. Traces the **complete call chain** from node boot to accepted block  
3. Documents **all staleness guards** and their correct ordering  
4. Provides a **mining channel template** derived from the stake model

This document supersedes any inline comments that predated PR #278 (unified height fix).

---

## 2. The Unified Height Contract

```
RULE: block.nHeight == tStateBest.nHeight + 1 (UNIFIED chain height)
```

### 2.1 Why Unified, Not Channel Height?

The Nexus blockchain uses a **single unified height counter** that increments with every accepted block, regardless of which mining channel (Prime=1, Hash=2, Private=3, Stake=0) produced it.

`TritiumBlock::Accept()` validates:

```cpp
// From src/TAO/Ledger/tritium.cpp — Accept()
if(statePrev.nHeight + 1 != nHeight)
    return debug::error(FUNCTION, "incorrect block height");
```

where `statePrev` is the `BlockState` at `hashPrevBlock` — always a **unified** chain position.

### 2.2 What Channel Height Is (and Is Not)

| Field | Type | Stored In | Used For | In Block Template? |
|---|---|---|---|---|
| `nHeight` | Unified chain height | `TritiumBlock`, `BlockState` | `Accept()` validation, NexusMiner contract | ✅ YES — bytes 200-203 |
| `nChannelHeight` | Per-channel counter | `BlockState` only | Ambassador/Developer payout intervals | ❌ NO — metadata only |

`nChannelHeight` is computed by `BlockState::SetBest()` via `GetLastState()` **after** a block is accepted. It must never be written into a block template.

---

## 3. Genesis Block Architecture

```
                    NODE BOOT
                        │
                        ▼
               CreateGenesis()
              [src/TAO/Ledger/create.cpp:760]
                        │
            ┌───────────┼───────────────┐
            │           │               │
      fClient      fHybrid        Full Node
      (mainnet)      │             (default)
            │         │               │
     TritiumGenesis() HybridGenesis() LegacyGenesis()
     nHeight=2944207  nHeight=0      nHeight=0
     nChannel=1       nChannel=2     nChannel=2
            │         │               │
            └─────────┴───────────────┘
                        │
                        ▼
           ChainState::tStateGenesis = state
           ChainState::tStateBest    = tStateGenesis
           ChainState::hashBestChain = hashGenesis
                        │
                        ▼
              [SYNC FROM NETWORK]
              tStateBest advances:
              nHeight: 0 → ... → 6,604,691
              hashBestChain: genesis → tip
```

### 3.1 Genesis Block Field Invariants

| Field | LegacyGenesis | TritiumGenesis (client) | Meaning |
|---|---|---|---|
| `hashPrevBlock` | `0` | hardcoded | Sentinel: no ancestor |
| `nHeight` | `0` | `2,944,207` | First unified height in this genesis variant |
| `nChannel` | `2` (Hash) | `1` (Prime) | Channel that mined genesis |
| `nChannelHeight` | `1` | varies | Channel-local counter; starts at 1 |
| `nNonce` | `2196828850` (main) | hardcoded | Proof-of-work solution |

**Critical:** After full sync, `tStateBest.nHeight` reflects the true current chain tip regardless of which genesis variant was used. All subsequent block template creation uses only `tStateBest.nHeight + 1`.

---

## 4. Stake Channel Block Production — Complete Flow

### 4.1 Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    TritiumMinter Thread                      │
│  [src/TAO/Ledger/tritium_minter.cpp — StakeMinterThread()]  │
│                                                              │
│  PHASE 0: WAIT FOR SYNC                                     │
│  while(Synchronizing() || connections==0) sleep(5s)         │
│                                                              │
│  MAIN LOOP (every ~1s):                                     │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ 1. hashLastBlock ← hashBestChain.load()  [GUARD-1]  │    │
│  │ 2. CheckUser()                                       │    │
│  │ 3. FindTrustAccount(hashGenesis)                     │    │
│  │    → sets fGenesis flag                              │    │
│  │ 4. CreateCandidateBlock()                            │    │
│  │    → CreateStakeBlock(fGenesis)                      │    │
│  │    → CreateCoinstake(hashGenesis)                    │    │
│  │ 5. CalculateWeights()                                │    │
│  │ 6. MintBlock(hashGenesis)                            │    │
│  │    → CheckInterval() / CheckMempool()                │    │
│  │    → HashBlock(nRequired)  ←─── GUARD-1 LOOP        │    │
│  │         → ProcessBlock()  ← on solution found       │    │
│  │              → GUARD-2: hashPrevBlock check          │    │
│  │              → Process(block, nStatus)               │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

### 4.2 Detailed Phase-by-Phase Flow

#### Phase 1: Block Template Creation

```
CreateCandidateBlock()
[stake_minter.cpp:351]
         │
         ▼
CreateStakeBlock(pCredentials, pin, block, fGenesis)
[create.cpp:713]
         │
         ├─── if(!fGenesis || fPoolStaking):
         │         AddTransactions(block)
         │         [mempool → block.vtx]
         │
         ├─── CreateTransaction(user, pin, rProducer)
         │         [bare producer skeleton — no coinstake op yet]
         │         Sets: nSequence, hashGenesis, hashPrevTx,
         │               nKeyType, nNextType, nTimestamp, nVersion
         │
         ├─── UpdateProducerTimestamp(rProducer)
         │         max(rProducer.nTimestamp, tStateBest.GetBlockTime()+1)
         │
         ├─── block.producer = rProducer
         │
         └─── AddBlockData(tStateBest, nChannel=0, block)
                   │
                   ├─── [nChannel==0: SKIP Merkle root]
                   │    (deferred: nReward unknown until block found)
                   │
                   ├─── block.hashPrevBlock = tStateBest.GetHash()
                   │         ↑ PRIMARY STALENESS ANCHOR
                   │         Baked into 216-byte template. Immutable.
                   │
                   ├─── block.nChannel = 0
                   │
                   ├─── block.nHeight = tStateBest.nHeight + 1
                   │         ↑ UNIFIED HEIGHT — PR #278 fix
                   │         Passes: TritiumBlock::Accept() check
                   │         Passes: NexusMiner ValidateTemplate() check
                   │
                   ├─── block.nBits = GetNextTargetRequired(tStateBest, 0, false)
                   │
                   ├─── block.nNonce = 1
                   │
                   └─── block.nTime = max(tStateBest.GetBlockTime()+1, now)
```

#### Phase 2: Coinstake Producer Setup

```
CreateCoinstake(hashGenesis)
[tritium_minter.cpp:134]
         │
         ├─── if(!fGenesis):  [TRUST path]
         │         FindLastStake(hashGenesis, hashLast)
         │         │
         │         stateLast ← LLD::Ledger->ReadBlock(hashLast, stateLast)
         │         │   ↑ LOADED BY BLOCK HASH (not GetLastState!)
         │         │   stateLast.nHeight = UNIFIED height of last stake block
         │         │
         │         tStatePrev ← LLD::Ledger->ReadBlock(block.hashPrevBlock)
         │         │
         │         nBlockAge = tStatePrev.GetBlockTime() - stateLast.GetBlockTime()
         │         nTrust = GetTrustScore(nTrustPrev, nBlockAge, nStake, ...)
         │         │
         │         block.producer[0] << OP::TRUST << hashLast << nTrust << nStakeChange
         │
         └─── if(fGenesis):  [GENESIS path]
                   block.producer[0] << OP::GENESIS
                   [no hashLast — this is the user's first stake]
```

#### Phase 3: Proof-of-Stake Hashing (Guard 1)

```
HashBlock(nRequired)
[stake_minter.cpp:522]
         │
         ▼
while(!fStop && !fShutdown
   && hashLastBlock == hashBestChain.load())   ← GUARD 1
{
         │
         ├─── CheckStale()          ← producer orphan check
         │         mempool.Get(producer.hashGenesis) &&
         │         producer.hashPrevTx != tx.GetHash()
         │         → if stale: break (rebuild block)
         │
         ├─── block.UpdateTime()
         │
         ├─── nThreshold = GetCurrentThreshold(nBlockTime, block.nNonce)
         │
         ├─── if(nThreshold < nRequired): sleep(10ms), continue
         │
         ├─── hashProof = block.StakeHash()
         │
         └─── if(hashProof <= nHashTarget):
                   ProcessBlock()             ← SOLUTION FOUND
                   break
}
```

#### Phase 4: Block Finalization (Guard 2)

```
ProcessBlock()
[stake_minter.cpp:610]
         │
         ├─── nReward = CalculateCoinstakeReward(block.GetBlockTime())
         │         [deferred: uses final block.nTime for exact reward]
         │
         ├─── block.producer[0] << nReward
         │
         ├─── block.producer.Build()          ← execute pre/post state
         │
         ├─── [Build Merkle Root — NOW complete]
         │         vHashes = block.vtx hashes
         │         vHashes += block.producer.GetHash()  ← producer LAST
         │         block.hashMerkleRoot = BuildMerkleTree(vHashes)
         │
         ├─── block.producer.Sign(key)
         │
         ├─── GUARD 2: Pre-submit staleness check
         │         if(block.hashPrevBlock != hashBestChain.load())
         │             return false  ← STALE: chain moved during hashing
         │
         └─── TAO::Ledger::Process(block, nStatus)
                   ├─── TritiumBlock::Check()   ← structural validation
                   ├─── TritiumBlock::Accept()  ← consensus rules
                   │         statePrev = ReadBlock(hashPrevBlock)
                   │         ASSERT: statePrev.nHeight + 1 == block.nHeight
                   └─── BlockState::SetBest()   ← updates chain state
                             nChannelHeight computed HERE (not in template)
```

---

## 5. The Two Staleness Guards — Detailed Specification

### Guard 1: Continuous Mining Loop Guard

```cpp
// tritium_minter.cpp:397 — snapshot taken at loop start
hashLastBlock = TAO::Ledger::ChainState::hashBestChain.load();

// stake_minter.cpp:534-535 — continuous check during hashing
while(...&& hashLastBlock == TAO::Ledger::ChainState::hashBestChain.load())
```

**Purpose:** Terminates mining immediately when the chain tip advances. Forces a full block rebuild for the new chain tip.

**When triggered:** Any peer broadcasts a new accepted block while local hashing is in progress.

**Effect:** The outer loop restarts, rebuilding `block` with the new `tStateBest`.

### Guard 2: Pre-Submit Staleness Check

```cpp
// stake_minter.cpp:674
if(block.hashPrevBlock != TAO::Ledger::ChainState::hashBestChain.load())
    return false;
```

**Purpose:** Final safety net after the hash solution is found but before submission. Guards the narrow race window between Guard 1 exiting the loop and `Process()` being called.

**When triggered:** Chain advanced during the final milliseconds of block finalization.

**Effect:** Discards the mined block, returns false. Outer loop rebuilds.

### Guard Comparison Table

| Guard | Location | Trigger | Frequency | Cost if triggered |
|---|---|---|---|---|
| **Guard 1** | `HashBlock()` inner while-loop | `hashBestChain` changed | Every iteration (~10ms) | Restart block build |
| **Guard 2** | `ProcessBlock()` pre-submit | `hashBestChain` changed | Once per solution found | Discard mined block |

---

## 6. Stake vs. Mining Channel Template Comparison

### 6.1 Shared Infrastructure (All Channels)

All channels use the same core functions:

| Function | Stake (ch 0) | Prime (ch 1) | Hash (ch 2) | Private (ch 3) |
|---|---|---|---|---|
| `AddBlockData()` | ✅ | ✅ | ✅ | ✅ |
| `block.nHeight = tStateBest.nHeight + 1` | ✅ | ✅ | ✅ | ✅ |
| `block.hashPrevBlock = tStateBest.GetHash()` | ✅ | ✅ | ✅ | ✅ |
| `block.nBits = GetNextTargetRequired(...)` | ✅ | ✅ | ✅ | ✅ |
| `TritiumBlock::Accept()` height check | ✅ | ✅ | ✅ | ✅ |

### 6.2 Channel-Specific Differences

| Aspect | Stake (ch 0) | Mining (ch 1, 2) | Private (ch 3) |
|---|---|---|---|
| **Entry point** | `CreateStakeBlock()` | `CreateBlock()` | `CreateBlock()` via `ThreadGenerator()` |
| **Template cache** | No (rebuilt each iteration) | Yes (`tBlockCache[nChannel]`) | No |
| **Merkle root in AddBlockData** | ❌ Deferred | ✅ Built immediately | ✅ Built immediately |
| **When Merkle is built** | `ProcessBlock()` after nReward known | `AddBlockData()` or on cache refresh | `AddBlockData()` |
| **Producer operation** | `OP::GENESIS` or `OP::TRUST` | `OP::COINBASE` | `OP::AUTHORIZE` |
| **Reward calculation timing** | After solution found (`ProcessBlock`) | At template creation (`GetCoinbaseReward`) | N/A |
| **Guard 1 mechanism** | `hashLastBlock` member field | `hashPrevBlock` vs `hashBestChain` in LLP | N/A (condition variable) |
| **Guard 2 mechanism** | `hashPrevBlock != hashBestChain` | `hashPrevBlock != hashBestChain` in LLP | Implicit via `Process()` |
| **State for interval check** | `stateLast` (last stake block) | N/A | N/A |
| **Interval requirement** | `MinStakeInterval(block)` | None | None |

### 6.3 Mining Channel Template (Derived from Stake Model)

If implementing a **new mining channel** or refactoring an existing mining server, use this checklist derived from the stake pattern:

```
MINING CHANNEL BLOCK PRODUCTION CHECKLIST
==========================================

[ ] 1. SNAPSHOT: hashLastBlock = hashBestChain.load()
        Before building any template. Atomic load.

[ ] 2. TEMPLATE CREATION via CreateBlock() or CreateStakeBlock():
        [ ] block.hashPrevBlock = tStateBest.GetHash()
        [ ] block.nHeight       = tStateBest.nHeight + 1   ← UNIFIED
        [ ] block.nChannel      = <channel id>
        [ ] block.nBits         = GetNextTargetRequired(tStateBest, nChannel, false)
        [ ] block.nNonce        = 1
        [ ] block.nTime         = max(tStateBest.GetBlockTime() + 1, now)
        [ ] Merkle root built (unless deferred like stake channel)

[ ] 3. GUARD 1 (continuous loop):
        while(hashLastBlock == hashBestChain.load()) { mine... }
        On exit: rebuild template if hashLastBlock != hashLastBlock

[ ] 4. HASHING: nonce or proof-specific iteration

[ ] 5. GUARD 2 (pre-submit):
        if(block.hashPrevBlock != hashBestChain.load()) discard

[ ] 6. FINALIZE:
        Complete producer (add reward)
        Build Merkle root if deferred
        Sign producer transaction
        Sign block

[ ] 7. SUBMIT:
        TAO::Ledger::Process(block, nStatus)
        Check: nStatus & PROCESS::ACCEPTED

[ ] 8. NEVER:
        [ ] Never set block.nHeight = nChannelHeight + 1
        [ ] Never overwrite hashPrevBlock after serialization
        [ ] Never set block.nHeight from anything other than tStateBest.nHeight + 1
```

---

## 7. CheckInterval() — The Unified Height Dependency

`CheckInterval()` is stake-specific but demonstrates a key cross-channel pattern:

```cpp
// stake_minter.cpp:462
const uint32_t nInterval = block.nHeight - stateLast.nHeight;
```

### 7.1 Correctness Requirement

Both `block.nHeight` and `stateLast.nHeight` must be **unified heights** for this subtraction to be meaningful:

| Scenario | block.nHeight | stateLast.nHeight | nInterval | Correct? |
|---|---|---|---|---|
| **Before PR #278** | channel height (~2.3M) | unified height (~6.6M) | wraps to ~3.7B | ❌ Always passes |
| **After PR #278** | unified height (~6.6M) | unified height (~6.6M) | actual gap (~100) | ✅ Correct |

### 7.2 `stateLast` Loading — Critical Detail

`stateLast` is loaded in `TritiumMinter::CreateCoinstake()`:

```cpp
// tritium_minter.cpp:180-181
stateLast = BlockState();
if(!LLD::Ledger->ReadBlock(hashLast, stateLast))  // ← by BLOCK HASH
    return debug::error(FUNCTION, "Failed to get last block for trust account");
```

**`stateLast` is loaded by the hash of the transaction's containing block** — this gives the full `BlockState` with the correct **unified** `nHeight`. It does **not** use `GetLastState()` which would give channel-anchored state.

---

## 8. Key Data Flows — Height Propagation

```
BOOT TIME:
  LegacyGenesis()  → tStateGenesis.nHeight = 0
  TritiumGenesis() → tStateGenesis.nHeight = 2,944,207
                                    │
                                    ▼
              ChainState::tStateBest = tStateGenesis
                                    │
                              [SYNC LOOP]
                          P2P blocks received
                          BlockState::SetBest()
                                    │
                                    ▼
              ChainState::tStateBest.nHeight advances
              (e.g., 2,944,207 → 6,604,691)
                                    │
                    ┌───────────────┘
                    │
TEMPLATE TIME:      ▼
  tStateBest = ChainState::tStateBest.load()   ← ATOMIC SNAPSHOT
  block.nHeight = tStateBest.nHeight + 1       ← e.g., 6,604,692
  block.hashPrevBlock = tStateBest.GetHash()   ← e.g., 0xeacd...
                    │
                    ▼
ACCEPT TIME:
  statePrev = ReadBlock(block.hashPrevBlock)
  ASSERT: statePrev.nHeight + 1 == block.nHeight
  → 6,604,691 + 1 == 6,604,692  ✅
                    │
                    ▼
SETBEST TIME:
  BlockState::SetBest()
  GetLastState(stateChannel, nChannel)         ← channel-specific walk
  newState.nChannelHeight = stateChannel.nChannelHeight + 1
  → nChannelHeight computed from accepted chain, never from template
```

---

## 9. File Reference Map

| File | Role in Flow | Key Functions |
|---|---|---|
| `src/TAO/Ledger/genesis_block.cpp` | Boot genesis block construction | `LegacyGenesis()`, `TritiumGenesis()`, `HybridGenesis()` |
| `src/TAO/Ledger/create.cpp` | Block template factory | `CreateGenesis()`, `CreateStakeBlock()`, `CreateBlock()`, `AddBlockData()`, `CreateProducer()` |
| `src/TAO/Ledger/stake_minter.cpp` | Stake minting base class | `CreateCandidateBlock()`, `HashBlock()`, `ProcessBlock()`, `CheckInterval()` |
| `src/TAO/Ledger/tritium_minter.cpp` | Stake minting implementation | `StakeMinterThread()`, `CreateCoinstake()`, `MintBlock()`, `CalculateCoinstakeReward()` |
| `src/TAO/Ledger/tritium.cpp` | Block type + Accept() | `TritiumBlock::Accept()` height validation |
| `src/TAO/Ledger/chainstate.cpp` | Global chain state | `tStateBest`, `hashBestChain`, `tStateGenesis` |

---

## 10. Invariants — Compile-Time Checklist

These invariants must hold at all times. Any PR modifying block production must verify all of them:

```
INV-1:  block.nHeight       == ChainState::tStateBest.nHeight + 1
INV-2:  block.hashPrevBlock == ChainState::tStateBest.GetHash()
INV-3:  block.nHeight IS unified chain height (not channel height)
INV-4:  block.hashPrevBlock IS immutable after AddBlockData() returns
INV-5:  block.nHeight       IS immutable after AddBlockData() returns
INV-6:  nChannelHeight is NEVER written to block.nHeight
INV-7:  nChannelHeight is NEVER read from block.nHeight
INV-8:  Merkle root for channel 0 is built AFTER nReward is final
INV-9:  Merkle root for channels 1,2,3 is built BEFORE template is sent to miner
INV-10: stateLast.nHeight is loaded via ReadBlock(hashLast) — by block hash, not GetLastState()
INV-11: hashLastBlock is snapshotted at loop iteration start (Guard 1 anchor)
INV-12: hashPrevBlock is compared to hashBestChain immediately before Process() (Guard 2)
```

---

## 11. Change History

| PR | Change | Invariants Affected |
|---|---|---|
| **PR #274** | Removed Physical Falcon signature from node | Signature flow only |
| **PR #278** | `block.nHeight = tStateBest.nHeight + 1` (unified) in `AddBlockData()` and `stateless_block_utility.cpp` | INV-1, INV-3, INV-6 |
| **PR #278** | `nChannelHeight=0`, `nUnifiedHeight=block.nHeight` in result structs | INV-7 |
| **PR #278** | Added `MiningContext::hashLastBlock` field for stateless Guard 1 | INV-11 (partial) |
| **FOLLOW-UP** | Wire `hashLastBlock` Guard 1 loop in stateless miner connection | INV-11 (complete) |

---

*Document generated from source audit of `NamecoinGithub/LLL-TAO` branch `STATELESS-NODE`.*  
*All line references verified against commit `791871609e3509fd22dd418ca273ab6dd9ac686f`.