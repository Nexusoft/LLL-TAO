# Block Template Immutability & Shared-Cache Constraints

**Branch:** `STATELESS-NODE`  
**Date:** 2026-03-17  
**Status:** CLOSED — Investigation complete. PRs #412 and #413 closed.  
**Context:** Post-mortem for `ChannelTemplateCache` design exploration (PR #412, PR #413).

---

## 1. Executive Summary

A proposal to introduce a server-level `ChannelTemplateCache` — one cached block
template shared across all miner connections for a given channel — was investigated
and ultimately rejected.  The fundamental blocker is **template immutability**:
the reward (coinbase) address is baked into a wallet-signed producer transaction at
template creation time and **cannot be changed without rebuilding and re-signing the
entire block**.

Because every authenticated stateless miner may have a distinct `hashRewardAddress`,
a single shared template can only be correct for **one** reward address.  For the
general case (heterogeneous miners, each with their own payout address) the cache
would require one template per address — identical in cost to the status quo.

The existing per-connection coalescing logic (`m_template_create_in_flight` +
`TEMPLATE_CREATE_CV`) already captures the one case where a shared cache would have
been correct (solo mining or a pool where all miners share one address).

---

## 2. The Immutable Template Chain

### 2.1 Template Creation

```
CreateProducer()  [src/TAO/Ledger/create.cpp]
  │
  ├─ rProducer[0] << OP::COINBASE
  ├─ rProducer[0] << hashRewardRecipient   ← BAKED IN — reward address committed here
  ├─ rProducer[0] << nCredit
  └─ rProducer[0] << nExtraNonce

producer.Sign(wallet_key)                  ← SIGNED — wallet signature over the above

hashMerkleRoot = BuildMerkleTree(
    [...vtx hashes..., producer.GetHash()]) ← COMMITTED — merkle root covers producer
```

**Key invariant:** `hashMerkleRoot` is the block's identity key (used in `mapBlocks`
for template lookup at `SUBMIT_BLOCK` time).  It commits to the producer transaction,
which commits to the reward address.  Any mutation of the reward address invalidates
the producer signature, changes `producer.GetHash()`, and therefore changes
`hashMerkleRoot` — producing a different block entirely.

### 2.2 What the Miner Is Allowed to Write

At `SUBMIT_BLOCK`, the node calls `sign_block(nNonce, hashMerkleRoot, vOffsets)`.
Internally this calls the channel-specific solved-candidate builder:

| Channel | Builder | Miner-writable fields | Everything else |
|---------|---------|----------------------|-----------------|
| Prime (1) | `BuildSolvedPrimeCandidateFromTemplate` | `nNonce`, `vOffsets` | IMMUTABLE |
| Hash (2)  | `BuildSolvedHashCandidateFromTemplate`  | `nNonce`             | IMMUTABLE |

`nTime` is **not** writable by the miner (neither `ProofHash` computation includes
`nTime`; `UpdateTime()` was deliberately removed from both channel paths).
`hashRewardRecipient` inside `producer` is **not** writable — it is inside the
wallet-signed producer transaction.

After the solved-candidate is built:
```
FinalizeWalletSignatureForSolvedBlock()   ← node re-signs with new nNonce/vOffsets
ValidateMinedBlock() → Check()
  → VerifyWork()          ← proof-of-work gate
  → VerifySignature()     ← wallet signature gate (catches any tampering)
AcceptMinedBlock() → network relay
```

---

## 3. Why a Shared Cache Cannot Work in the General Case

### 3.1 The Address-Per-Miner Problem

```
Miner A  hashRewardAddress = 0xAAAA...  → needs template with coinbase → 0xAAAA
Miner B  hashRewardAddress = 0xBBBB...  → needs template with coinbase → 0xBBBB

A shared cache holds ONE template.
  If it holds 0xAAAA, Miner B receives block rewards at Miner A's address.
  If it holds 0xBBBB, Miner A receives block rewards at Miner B's address.
  Either outcome is a consensus-level correctness failure.
```

"Patch the address at serve time" is impossible — see §2.1.  The address is inside
a signed, serialised contract stream.  Patching it requires re-running `CreateProducer`
+ `producer.Sign` + `BuildMerkleTree`, which is identical in cost to creating a new
block from scratch.

### 3.2 Cache Benefit Matrix

| Deployment scenario | Unique templates needed | Shared cache benefit |
|---------------------|------------------------|----------------------|
| Solo node (1 miner, node's own address) | 1 | ✅ Full — but per-connection coalescing already handles this |
| Pool (N miners, 1 shared pool address) | 1 | ✅ Full — but per-connection coalescing already handles this |
| Heterogeneous miners (N miners, N addresses) | N | ❌ None — N builds required regardless |

The general stateless case is heterogeneous miners.  A shared cache provides zero
benefit there.

### 3.3 What Already Exists Is Sufficient

`StatelessMinerConnection::new_block()` already contains:
```cpp
// Per-connection in-flight coalescing
std::unique_lock<std::mutex> lock(TEMPLATE_CREATE_MUTEX);
if (m_template_create_in_flight) {
    while (m_template_create_in_flight && !config::fShutdown.load())
        TEMPLATE_CREATE_CV.wait_for(lock, std::chrono::milliseconds(500));
    // Staleness re-validation on waited result (FIX #332)
    ...
}
```

This is precisely the BUILDING-state wait that PR #413 introduced at server level —
but implemented at connection level where the correct `hashRewardAddress` is already
known.  For concurrent GET_BLOCK calls from the same miner connection, the coalescer
serves a single template to all concurrent requests.  For different miners with
different addresses, each connection builds its own template — which is required for
correctness.

---

## 4. Diagrams

### 4.1 Full Template Lifecycle

```
┌─────────────────────────────────────────────────────────────────────┐
│                        TEMPLATE CREATION                            │
│                                                                     │
│  GET_BLOCK received from authenticated miner                        │
│           │                                                         │
│           ▼                                                         │
│  new_block()                                                        │
│    │  Determine hashReward:                                         │
│    │    1. context.hashRewardAddress  (MINER_SET_REWARD)            │
│    │    2. context.hashGenesis        (Falcon auth fallback)        │
│    │    3. wallet genesis             (legacy solo fallback)        │
│    │                                                                │
│    ▼                                                                │
│  CreateBlockForStatelessMining(nChannel, nExtraNonce, hashReward)   │
│    │                                                                │
│    ▼                                                                │
│  CreateProducer()                                                   │
│    │  rProducer[0] << OP::COINBASE                                  │
│    │  rProducer[0] << hashReward     ◄── REWARD ADDRESS BAKED IN   │
│    │  rProducer[0] << nCredit                                       │
│    │  rProducer[0] << nExtraNonce                                   │
│    │                                                                │
│    ▼                                                                │
│  producer.Sign(wallet_key)           ◄── WALLET SIGNATURE APPLIED  │
│    │                                                                │
│    ▼                                                                │
│  hashMerkleRoot = BuildMerkleTree(   ◄── MERKLE ROOT COMMITTED     │
│      vtx_hashes + producer.GetHash())                               │
│    │                                                                │
│    ▼                                                                │
│  mapBlocks[hashMerkleRoot] = TemplateMetadata{pBlock}              │
│    │                                                                │
│    ▼                                                                │
│  Serialize 228-byte payload → BLOCK_DATA → miner                   │
└─────────────────────────────────────────────────────────────────────┘
                          │
                          │  Miner mines (searches nNonce space)
                          ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        SUBMIT_BLOCK                                 │
│                                                                     │
│  Miner sends: [encrypted full block][timestamp][Falcon sig]        │
│           │                                                         │
│           ▼                                                         │
│  ChaCha20 decrypt + Falcon signature verify                         │
│           │                                                         │
│           ▼                                                         │
│  Template lookup: mapBlocks[hashMerkleRoot]                         │
│           │                                                         │
│           ▼                                                         │
│  sign_block(nNonce, hashMerkleRoot, vOffsets)                       │
│    │                                                                │
│    │  BuildSolved*CandidateFromTemplate()                           │
│    │    nNonce   ◄���─ only miner-written field                       │
│    │    vOffsets ◄── Prime channel only                             │
│    │    nTime      = preserved from template  (IMMUTABLE)           │
│    │    producer   = preserved from template  (IMMUTABLE)           │
│    │    hashReward = preserved from template  (IMMUTABLE)           │
│    │    vchBlockSig cleared → FinalizeWalletSignature re-signs      │
│    │                                                                │
│    ▼                                                                │
│  hashPrevBlock vs hashBestChain stale guard                         │
│           │                                                         │
│           ▼                                                         │
│  ValidateMinedBlock()                                               │
│    → Check() → VerifyWork() + VerifySignature()                     │
│           │                                                         │
│           ▼                                                         │
│  AcceptMinedBlock() → relay to network                              │
└─────────────────────────────────────────────────────────────────────┘
```

### 4.2 Why "Patch at Serve Time" Is Impossible

```
producer (wallet-signed serialized byte stream):
┌──────────────────────────────────────────────────────────┐
│ OP::COINBASE │ hashRewardRecipient │ nCredit │ nExtraNonce│
│              └──── want to change this ────┘             │
│                                                           │
│  producer.Sign(wallet_key) covers the entire stream above │
│  → changing hashRewardRecipient breaks producer.Verify()  │
│  → must call CreateProducer() + Sign() again              │
│  → producer.GetHash() changes                             │
│  → BuildMerkleTree() produces a new hashMerkleRoot        │
│  → mapBlocks key changes → it's a completely new block    │
└──────────────────────────────────────────────────────────┘
```

Cost of "patching" = Cost of creating a brand new block.  The cache saves nothing.

### 4.3 Shared Cache: Correctness Matrix

```
                     hashRewardAddress in cache
                     ┌──────────┬──────────────┐
                     │  0xAAAA  │    0xBBBB    │
          ┌──────────┼──────────┼──────────────┤
Miner A   │  0xAAAA  │  ✅ OK  │  ❌ WRONG    │
(wants A) ├──────────┼──────────┼──────────────┤
Miner B   │  0xBBBB  │  ❌ WRONG│  ✅ OK      │
(wants B) └──────────┴──────────┴──────────────┘

Only one of the two miners can be served correctly at any time.
The cache is only correct for the degenerate case where all miners
share the same reward address (solo or pool).  That case is already
handled by the existing per-connection coalescer.
```

### 4.4 Existing Per-Connection Coalescer (Sufficient for Solo/Pool)

```
Connection A ──► new_block() ──► m_template_create_in_flight = true
                                  │
Connection B ──► new_block() ─►  wait_for(TEMPLATE_CREATE_CV, 500ms)
                                  │        (same address as A — same result is fine)
Connection C ──► new_block() ─►  wait_for(TEMPLATE_CREATE_CV, 500ms)
                                  │
                                  ▼ (A completes)
                 m_last_created_template set
                 TEMPLATE_CREATE_CV.notify_all()
                                  │
               B ◄── receives template (staleness re-validated, FIX #332)
               C ◄── receives template (staleness re-validated, FIX #332)

Cost: 1 build, N serves — exactly what the server-level cache would have provided,
      but at the connection scope where the reward address is already correct.
```

---

## 5. Decision Record

### 5.1 What Was Investigated

| PR | Design | Verdict |
|----|--------|---------|
| #412 | `ChannelTemplateCache` with `std::atomic_flag` try-lock, `StoreRewardAddress()`, `GetCurrentForReward()` address gate | Correct for solo/pool; reward-address gate acknowledged the problem but required last-write-wins semantics incompatible with heterogeneous miners |
| #413 | `ChannelTemplateCache` with `EMPTY→BUILDING→IDLE` state machine, `condition_variable::wait_for(500ms)`, `Clone()` into per-connection `mapBlocks` | Better threading design; `detach(uint256_t(0))` in dispatcher confirmed the address problem; `GetCurrentForReward()` gate removed, making it incorrect for non-solo use |

### 5.2 Root Assumption That Failed

> *"The reward address can be filled in or overridden at SUBMIT_BLOCK time."*

This assumption is false.  The reward address is inside a wallet-signed contract
inside a producer transaction whose hash is committed into the merkle root.  The
merkle root is the block's identity.  It cannot be mutated after template creation
without full block reconstruction.

### 5.3 Final Decision

**Close PR #412 and PR #413.**

The complexity and correctness risk of a server-level shared-template cache outweigh
any marginal performance gain for the general stateless mining case.  The existing
per-connection coalescing already covers the solo/pool case correctly.  No further
work on `ChannelTemplateCache` is planned.

---

## 6. Reference: Key Source Locations

| Concern | File | Lines |
|---------|------|-------|
| Reward address baked into producer | `src/TAO/Ledger/create.cpp` | `CreateProducer()` |
| Producer wallet signing | `src/TAO/Ledger/create.cpp` | `producer.Sign(...)` |
| Merkle root commitment | `src/TAO/Ledger/create.cpp` | `BuildMerkleTree(...)` |
| Miner-writable fields only (nNonce) | `src/TAO/Ledger/stateless_block_utility.cpp` | `BuildSolvedPrimeCandidateFromTemplate`, `BuildSolvedHashCandidateFromTemplate` |
| Re-signing after nNonce applied | `src/TAO/Ledger/stateless_block_utility.cpp` | `FinalizeWalletSignatureForSolvedBlock` |
| Per-connection coalescer | `src/LLP/stateless_miner_connection.cpp` | `new_block()` — `m_template_create_in_flight` block |
| Consensus verification chain | `src/TAO/Ledger/tritium.cpp` | `TritiumBlock::Check()` |

---

*Document generated 2026-03-17 — NexusMiner / LLL-TAO stateless mining architecture review.*