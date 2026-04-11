# Nexus LLL-TAO: Stateless Mining Infrastructure — Developer Reference

> **Repository:** [`NamecoinGithub/LLL-TAO`](https://github.com/NamecoinGithub/LLL-TAO)  
> **Primary source file:** [`src/LLP/stateless_miner.cpp`](https://github.com/NamecoinGithub/LLL-TAO/blob/0fa0d72/src/LLP/stateless_miner.cpp)  
> **Symbol index:** `src/LLP/include/stateless_miner.h`  
> **Document version:** 2026-02-17  

---

**Document Version:** 2.0 — Unified keepalive format (PR #299–302, #214–216)  
**Last Updated:** 2026-03-30  
**Breaking Changes:** BuildBestCurrentResponse() deleted; KeepAliveV2AckFrame field `sequence` renamed to `session_id`  
**Addendum:** Section 4.2 and 2.2 annotated 2026-03-30 — node-side vs miner-side staleness distinction.

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [The Multi-Channel Blockchain Model (Upstream Context)](#2-the-multi-channel-blockchain-model-upstream-context)
3. [Upstream Classes & Functions You Must Know](#3-upstream-classes--functions-you-must-know)
4. [Stateless Mining Infrastructure Classes](#4-stateless-mining-infrastructure-classes)
5. [Key Interactions Diagram](#5-key-interactions-diagram)
6. [Authentication Handshake Flow](#6-authentication-handshake-flow)
7. [Template Staleness Detection Flow](#7-template-staleness-detection-flow)
8. [Protocol Lanes: Legacy vs Stateless](#8-protocol-lanes-legacy-vs-stateless)
9. [Quick Reference: Function Index](#9-quick-reference-function-index)

---

## 1. Architecture Overview

The Nexus Stateless Mining Protocol is a **16-bit opcode, Falcon post-quantum authenticated** mining protocol layered on top of the upstream `TAO::Ledger` blockchain state machinery.

```
  ┌──────────────────────────────────────────────────────────┐
  │                NexusMiner (External Miner)               │
  │         Hash/FPGA/GPU — connects on port 9323            │
  └──────────────────────┬───────────────────────────────────┘
                         │  16-bit StatelessPacket frames
                         ▼
  ┌──────────────────────────────────────────────────────────┐
  │              LLP Layer (src/LLP/)                        │
  │                                                          │
  │  StatelessConnection ──► StatelessMinerConnection        │
  │                              │                           │
  │                              ├── StatelessMiner          │
  │                              │   (pure functional)       │
  │                              ├── TemplateMetadata        │
  │                              ├── MiningContext           │
  │                              └── StatelessMinerManager   │
  └──────────────────────┬───────────────────────────────────┘
                         │  Reads chain state
                         ▼
  ┌──────────────────────────────────────────────────────────┐
  │             TAO Ledger (src/TAO/Ledger/)                 │
  │                                                          │
  │   BlockState ──► GetLastState()                          │
  │   ChainState::tStateBest (atomic)                        │
  │   GetNextTargetRequired()   TritiumBlock                 │
  └──────────────────────────────────────────────────────────┘
```

---

## 2. The Multi-Channel Blockchain Model (Upstream Context)

> **Why this matters for mining:** Nexus runs **three concurrent mining channels** on one blockchain. Templates only become stale when the *specific channel* they belong to advances — not when *any* block is mined.

### 2.1 Channel IDs

| Channel | ID | Method | Description |
|---------|-----|--------|-------------|
| Proof-of-Stake | `0` | Trust-based | Not for external miners |
| Prime | `1` | CPU — Fermat prime clusters | Long proof-of-work |
| Hash | `2` | GPU / FPGA — SHA3 | Short proof-of-work |

### 2.2 Unified Height vs. Channel Height

```
Unified Height: increments for EVERY block, regardless of channel

  6535193 (Hash) → 6535194 (Prime) → 6535195 (Hash) → 6535196 (Stake)
  └─ unified = 6535196

Channel Height: increments ONLY when that channel mines a block

  At unified = 6535196:
    Prime channel  = 2165442  (last Prime was at unified 6535194)
    Hash channel   = 4165000  (last Hash was at unified 6535195)
    Stake channel  = 235000   (last Stake was at unified 6535196)
```

**Key insight:** A miner's template for Prime channel is NOT stale when a Hash block is mined. Only when a *new Prime block* appears does `nChannelHeight` advance and the template become obsolete.

> 📝 **Miner note (2026-03):** While the template is not stale in the channel-height sense, its `hashPrevBlock` **is** stale after any cross-channel or Stake block — the unified tip advanced and the chain tip changed. Miner clients must request a fresh template on every unified-tip advance, not only when the same channel mines. See Section 4.2 note and [`unified-tip-and-channel-heights.md`](../current/mining/unified-tip-and-channel-heights.md).

---

## 3. Upstream Classes & Functions You Must Know

These live in the upstream `TAO::Ledger` and `LLC` namespaces and are directly called by our stateless mining infrastructure.

---

### 3.1 `TAO::Ledger::BlockState`

**File:** `src/TAO/Ledger/types/state.h` / `state.cpp`

The complete, persisted representation of one block on the chain.

**Critical fields for mining:**

| Field | Type | Description |
|-------|------|-------------|
| `nChannelHeight` | `uint32_t` | Per-channel block count — **the key staleness field** |
| `nChannel` | `uint32_t` | Which channel mined this block (0/1/2) |
| `nHeight` | `uint32_t` | Unified blockchain height |
| `nBits` | `uint32_t` | Encoded difficulty for this block |
| `nNonce` | `uint64_t` | The solved nonce |
| `nTime` | `uint64_t` | Block timestamp |
| `hashPrevBlock` | `uint1024_t` | Previous block hash |
| `hashMerkleRoot` | `uint512_t` | Merkle root of transactions |

**Key relationship with stateless mining:**

```
TemplateMetadata::IsStale()
    └── calls TAO::Ledger::GetLastState(state, nChannel)
            └── reads BlockState::nChannelHeight
                    └── compares against TemplateMetadata::nChannelHeight
```

*(node-side only — see Section 4.2 note for miner-side requirements)*

---

### 3.2 `TAO::Ledger::GetLastState()`

**File:** `src/TAO/Ledger/state.cpp` and `src/TAO/Ledger/client.cpp`

```cpp
// Upstream signature (state.cpp)
bool GetLastState(BlockState& state, uint32_t nChannel);

// Also for light-sync clients (client.cpp)
bool GetLastState(ClientBlock& state, uint32_t nChannel);
```

**What it does:** Walks backward through the chain from `ChainState::tStateBest`, returning the most recent block belonging to `nChannel`.

**Performance:** O(1) average — channels interleave frequently, so the walk is typically only ~3 blocks back. Called every 5–10 seconds per miner.

**Used by:** `TemplateMetadata::IsStale()` to get the current channel-specific height for staleness comparison.

```
TemplateMetadata::IsStale()
    │
    ├── [PRIMARY] GetLastState(stateBest, nChannel)
    │       → returns stateChannel.nChannelHeight
    │       → compare: stateChannel.nChannelHeight != (template.nChannelHeight - 1)
    │
    └── [SECONDARY] age check: (now - nCreationTime) > 90s
```

*(node-side only — see Section 4.2 note for miner-side requirements)*

---

### 3.3 `TAO::Ledger::GetNextTargetRequired()`

**File:** `src/TAO/Ledger/include/retarget.h`

```cpp
uint32_t GetNextTargetRequired(const BlockState& state, uint32_t nChannel, bool fDebug = true);
```

**What it does:** Computes the difficulty (`nBits`) for the next block in the given channel. Called when serving block templates to miners.

**Used by:** `StatelessMinerConnection::GetCachedDifficulty()` — which adds a 1-second TTL cache to avoid redundant calls across multiple miner connections.

---

### 3.4 `TAO::Ledger::ChainState::tStateBest`

**File:** `src/TAO/Ledger/include/chainstate.h`

```cpp
// Atomic pointer to the current best chain tip
static std::atomic<BlockState*> tStateBest;
```

**What it does:** Thread-safe snapshot of the current best chain tip. Read atomically without locks.

**Used by:** `GetLastState()` as the starting point for the backward channel-height walk; also used by `TemplateMetadata::IsStale()` indirectly.

---

### 3.5 `LLC::FLKey` and `LLC::FalconVersion`

**File:** `src/LLC/include/flkey.h`

```cpp
enum class FalconVersion : uint8_t
{
    FALCON_512  = 1,  // NIST Level 1, 128-bit quantum security
    FALCON_1024 = 2   // NIST Level 5, 256-bit quantum security
};
```

**What it does:** Encapsulates the Falcon post-quantum lattice signature scheme (NTRU-based). Used to authenticate miners without exposing their Tritium genesis account on the wire.

**Key size constants (from `FalconSizes` namespace):**

| Variant | Public Key | Signature |
|---------|-----------|-----------|
| Falcon-512 | 897 bytes | 809 bytes (CT) |
| Falcon-1024 | 1793 bytes | 1577 bytes (CT) |

**Used by:** `MiningContext` (stores `nFalconVersion`), `StatelessMiner::ProcessFalconResponse()`, `StatelessMiner::ProcessMinerAuthInit()`.

---

### 3.6 `TAO::Ledger::TritiumBlock`

**File:** `src/TAO/Ledger/types/tritium.h`

The actual block type used for Tritium-era mining templates. Inherits from `Block`.

**Key fields for mining:**

| Field | Description |
|-------|-------------|
| `producer` | The coinbase/stake transaction |
| `vtx` | Transaction hashes included in the block |
| `ssSystem` | System-level pre/post states |
| `nTime` | Block timestamp |

**Used by:** `TemplateMetadata` wraps a heap-allocated `TAO::Ledger::Block*` (which may be a `TritiumBlock`) in a `std::unique_ptr`.

---

## 4. Stateless Mining Infrastructure Classes

These are the **new classes** introduced in our stateless mining work. They live in `src/LLP/`.

---

### 4.1 `LLP::MiningContext`

**File:** [`src/LLP/include/stateless_miner.h`](https://github.com/NamecoinGithub/LLL-TAO/blob/0fa0d72/src/LLP/include/stateless_miner.h#L409-L691)

**Design:** Immutable value-type snapshot of a miner's complete state. All mutations return a **new** `MiningContext` — the original is untouched (copy-on-write fluent API).

```
"If you need to change something about a miner's state, you get back a new context.
 You never mutate in place. This makes reasoning about multi-threaded access safe."
```

**Fields summary:**

| Category | Fields |
|----------|--------|
| Channel/Height | `nChannel`, `nHeight`, `nLastTemplateChannelHeight` |
| Authentication | `fAuthenticated`, `hashKeyID`, `hashGenesis`, `vAuthNonce`, `vMinerPubKey`, `vDisposablePubKey`, `hashDisposableKeyID` |
| Session | `nSessionId`, `nSessionStart`, `nSessionTimeout`, `nKeepaliveCount`, `nKeepaliveSent`, `nLastKeepaliveTime` |
| Reward | `hashRewardAddress`, `fRewardBound` |
| Encryption | `vChaChaKey`, `fEncryptionReady` |
| Falcon | `nFalconVersion`, `fFalconVersionDetected`, `vchPhysicalSignature`, `fPhysicalFalconPresent` |
| Push Notifications | `fSubscribedToNotifications`, `nSubscribedChannel`, `nLastNotificationTime`, `nNotificationsSent` |
| Network | `strAddress`, `nProtocolVersion`, `nTimestamp` |

**Mutation methods (all return new `MiningContext`):**

| Method | Purpose |
|--------|---------|
| `WithChannel(n)` | Set mining channel (1=Prime, 2=Hash) |
| `WithHeight(n)` | Update blockchain height |
| `WithLastTemplateChannelHeight(n)` | Record channel height of last sent template |
| `WithAuth(bool)` | Mark Falcon authentication status |
| `WithSession(id)` | Set session identifier |
| `WithKeyId(hash)` | Set Falcon key ID |
| `WithGenesis(hash)` | Set Tritium genesis (identity) |
| `WithUserName(str)` | Set username for trust-based addressing |
| `WithNonce(bytes)` | Set auth challenge nonce |
| `WithPubKey(bytes)` | Store miner's Falcon public key |
| `WithDisposableKey(bytes, hash)` | Store disposable Falcon session key state |
| `WithSessionStart(t)` | Record session start timestamp |
| `WithSessionTimeout(t)` | Set inactivity timeout |
| `WithReconnectCount(n)` | Carry authoritative recovery reconnect metadata |
| `WithKeepaliveCount(n)` | Increment keepalive received counter |
| `WithKeepaliveSent(n)` | Increment keepalive sent counter |
| `WithLastKeepaliveTime(t)` | Record last keepalive exchange time |
| `WithRewardAddress(hash)` | Bind explicit reward payout address |
| `WithChaChaKey(bytes)` | Store ChaCha20 session key |
| `WithFalconVersion(ver)` | Record detected Falcon variant |
| `WithPhysicalSignature(bytes)` | Store physical Falcon signature |
| `WithSubscription(channel)` | Subscribe to push notifications |
| `WithNotificationSent(t)` | Update notification counters |
| `WithStakeHeight(n)` | Persist stake channel height across keepalive cycles |

**Query methods:**

| Method | Returns | Description |
|--------|---------|-------------|
| `GetPayoutAddress()` | `uint256_t` | Reward addr if bound, else genesis hash |
| `HasValidPayout()` | `bool` | Is genesis or username set? |
| `IsSessionExpired(now)` | `bool` | Has session timed out? |
| `GetSessionDuration(now)` | `uint64_t` | Seconds since session start |

---

### 4.2 `LLP::TemplateMetadata`

**File:** [`src/LLP/include/stateless_miner.h`](https://github.com/NamecoinGithub/LLL-TAO/blob/0fa0d72/src/LLP/include/stateless_miner.h#L167-L392)

**Design:** Move-only (copy deleted) wrapper around a block template. Owns the block via `std::unique_ptr`. Tracks both **unified height** and **channel-specific height** for accurate staleness detection.

> **The Problem it solves (PR #134):** Before this struct was corrected, templates were compared against *unified* blockchain height. Any block mined on *any* channel would mark all templates stale — causing ~40% wasted mining work.

**Fields:**

| Field | Type | Description |
|-------|------|-------------|
| `pBlock` | `unique_ptr<Block>` | Owned block template (~10 KB) |
| `nCreationTime` | `uint64_t` | When template was created |
| `nHeight` | `uint32_t` | Unified height at creation |
| `nChannelHeight` | `uint32_t` | **CRITICAL** — channel-specific height at creation |
| `hashMerkleRoot` | `uint512_t` | Expected merkle root (for solution validation) |
| `nChannel` | `uint32_t` | 1=Prime or 2=Hash |

**Methods:**

| Method | Signature | Description |
|--------|-----------|-------------|
| `IsStale()` | `bool IsStale(uint64_t nNow = 0) const` | Check if template is obsolete (channel height or age) |
| `GetAge()` | `uint64_t GetAge(uint64_t nNow = 0) const` | Seconds since creation |
| `GetChannelName()` | `std::string GetChannelName() const` | Returns `"Prime"`, `"Hash"`, or `"Stake"` |

**Staleness logic:**

> ⚠️ **NODE-SIDE ONLY — SUPERSEDED FOR MINER CLIENTS (2026-03):** The logic below describes the **node's internal** `TemplateMetadata::IsStale()` check, which correctly uses channel height + age for node-side template management. **Miner clients must NOT rely solely on channel height.** After any block on any channel (including Stake and the opposite PoW channel), `hashPrevBlock` in the miner's template is stale and a fresh template must be requested. See [`docs/current/mining/unified-tip-and-channel-heights.md`](../current/mining/unified-tip-and-channel-heights.md) and `push_notification_handler.cpp` in the NexusMiner repository for the correct miner-side model.

```
IsStale() — checked in order:

  [1] PRIMARY: Channel height advanced?
      blockchain_channel_height != (template.nChannelHeight - 1)
      → calls GetLastState() — O(1) avg, ~3 blocks back
      → TRUE means a same-channel block was mined → STALE

  [2] SECONDARY: Age timeout (safety net)
      (now - nCreationTime) > 90 seconds
      → covers reorgs, partitions, clock skew
      → TRUE → STALE

  Unified height is NOT checked (would cause false positives)
```

**Memory contract:** Move-only. Cannot be copied. Stored in `std::map<uint512_t, TemplateMetadata>` inside `StatelessMinerConnection::mapBlocks`.

---

### 4.3 `LLP::ProcessResult`

**File:** [`src/LLP/include/stateless_miner.h`](https://github.com/NamecoinGithub/LLL-TAO/blob/0fa0d72/src/LLP/include/stateless_miner.h#L700-L734)

**Design:** Return type from every `StatelessMiner` method. Bundles the (possibly updated) `MiningContext` + success/error state + optional response packet.

```cpp
struct ProcessResult {
    const MiningContext   context;    // Updated (or unchanged) miner state
    const bool            fSuccess;  // Did processing succeed?
    const std::string     strError;  // Error message if !fSuccess
    const StatelessPacket response;  // Optional packet to send back to miner
};
```

**Factory methods:**

| Method | Description |
|--------|-------------|
| `ProcessResult::Success(ctx, resp)` | Build a success result |
| `ProcessResult::Error(ctx, error)` | Build an error result, context unchanged |

---

### 4.4 `LLP::StatelessMiner`

**File:** [`src/LLP/include/stateless_miner.h`](https://github.com/NamecoinGithub/LLL-TAO/blob/0fa0d72/src/LLP/include/stateless_miner.h#L745-L945)

**Design:** Pure static class. All methods are stateless. Takes a `MiningContext` and a `StatelessPacket`, returns a `ProcessResult`. No member variables.

```
"Think of StatelessMiner as a pure function: (State, Input) → (NewState, Output)"
```

**Public methods:**

| Method | Opcode Handled | Description |
|--------|---------------|-------------|
| `ProcessPacket(ctx, StatelessPacket)` | Router | Main entry — dispatches to handlers below |
| `ProcessPacket(ctx, Packet)` | Router | Legacy 8-bit overload for backward compat |
| `ProcessMinerAuthInit(ctx, pkt)` | `MINER_AUTH_INIT` | Step 1 of auth: receive pubkey, send nonce challenge |
| `ProcessFalconResponse(ctx, pkt)` | `MINER_AUTH_RESPONSE` | Step 2: verify Falcon signature, set authenticated |
| `ProcessSessionStart(ctx, pkt)` | `SESSION_START` | Start session after auth |
| `ProcessSetChannel(ctx, pkt)` | `SET_CHANNEL` | Miner selects Prime or Hash channel |
| `ProcessSessionKeepalive(ctx, pkt)` | `SESSION_KEEPALIVE` | Heartbeat to maintain session |
| `ProcessSetReward(ctx, pkt)` | `MINER_SET_REWARD` | Bind encrypted reward address |

**Private helpers:**

| Method | Description |
|--------|-------------|
| `BuildAuthMessage(ctx)` | Constructs the bytes that the miner must sign with Falcon |
| `DecryptRewardPayload(encrypted, key, plaintext)` | ChaCha20-Poly1305 decrypt of reward address |
| `EncryptRewardResult(plaintext, key)` | ChaCha20-Poly1305 encrypt of server response |
| `ValidateRewardAddress(hash)` | Sanity-check: non-zero, correct format |

---

### 4.5 `LLP::StatelessMinerConnection`

**File:** [`src/LLP/types/stateless_miner_connection.h`](https://github.com/NamecoinGithub/LLL-TAO/blob/0fa0d72/src/LLP/types/stateless_miner_connection.h)

**Design:** The **connection class** that wraps `StatelessMiner`. Inherits from `StatelessConnection` (which handles 16-bit packet framing). Holds per-connection mutable state.

**Private members:**

| Member | Type | Description |
|--------|------|-------------|
| `context` | `MiningContext` | Current connection's miner state |
| `MUTEX` | `std::mutex` | Guards context updates |
| `mapBlocks` | `map<uint512_t, TemplateMetadata>` | Active block templates keyed by merkle root |
| `mapSessionKeys` | `map<uint32_t, vector<uint8_t>>` | Session ID → Falcon pubkey |
| `SESSION_MUTEX` | `std::mutex` | Guards session key map |
| `m_pPrimeState` | `unique_ptr<PrimeStateManager>` | Fork-aware Prime channel state (PR #136) |
| `m_pHashState` | `unique_ptr<HashStateManager>` | Fork-aware Hash channel state (PR #136) |
| `nBlockIterator` | `static atomic<uint32_t>` | Extra nonce iterator (shared across connections) |
| `nDiffCacheTime` | `static atomic<uint64_t>` | Difficulty cache timestamp |
| `nDiffCacheValue[3]` | `static PaddedDifficultyCache[3]` | Cached difficulty per channel (cache-line padded) |

---

### 4.6 `LLP::StatelessMinerManager`

**File:** [`src/LLP/include/stateless_manager.h`](https://github.com/NamecoinGithub/LLL-TAO/blob/0fa0d72/src/LLP/include/stateless_manager.h)

**Design:** Singleton. Tracks all active miner connections for RPC queries and pool statistics. Uses a concurrent hash map for thread-safe parallel access.

**Key methods:**

| Method | Description |
|--------|-------------|
| `Get()` | Return global singleton instance |
| `UpdateMiner(addr, ctx, lane)` | Register or update a miner's context |
| `RemoveMiner(addr)` | Remove by address |
| `RemoveMinerByKeyID(hash)` | Remove by Falcon key ID |
| `GetMinerContext(addr)` | Lookup by network address → `optional<MiningContext>` |
| `GetMinerContextByKeyID(hash)` | Lookup by Falcon key ID |
| `GetMinerContextBySessionID(id)` | Lookup by session ID |
| `GetMinerContextByGenesis(hash)` | Lookup by genesis hash (for reward mapping) |
| `GetMinerLane(addr)` | Returns `0` (Legacy) or `1` (Stateless) |
| `HasSwitchedLanes(addr, newLane)` | Did this miner change protocol lanes? |

**Also contains:** `MinerInfo` — lightweight struct for pool dashboard display.

---

### 4.7 `LLP::StatelessConnection`

**File:** [`src/LLP/templates/stateless_connection.h`](https://github.com/NamecoinGithub/LLL-TAO/blob/0fa0d72/src/LLP/templates/stateless_connection.h)

**Design:** Handles raw 16-bit packet framing. Inherits from `BaseConnection<StatelessPacket>`.

**Packet format:**
```
[ HEADER: 2 bytes big-endian ][ LENGTH: 4 bytes big-endian ][ DATA: LENGTH bytes ]
```

**Key method:** `ReadPacket()` — reads and assembles the three-part frame into a `StatelessPacket`.

---

### 4.8 `LLP::StatelessPacket`

**File:** [`src/LLP/packets/stateless_packet.h`](https://github.com/NamecoinGithub/LLL-TAO/blob/0fa0d72/src/LLP/packets/stateless_packet.h)

**Design:** The wire format for all stateless mining protocol messages.

```cpp
struct StatelessPacket {
    uint16_t             HEADER;   // 16-bit opcode (e.g., 0xD007, 0xD008)
    uint32_t             LENGTH;   // Payload length
    vector<uint8_t>      DATA;     // Payload bytes
};
```

**Key methods:**

| Method | Description |
|--------|-------------|
| `IsNull()` | Returns true when `HEADER == 0xFFFF` (uninitialized) |
| `GetOpcode()` | Returns raw `HEADER` value |
| `Complete()` | True when `DATA.size() == LENGTH` |
| `SetData(DataStream)` | Set payload from a `DataStream` |

---

### 4.9 `LLP::PushNotificationBuilder`

**File:** [`src/LLP/include/push_notification.h`](https://github.com/NamecoinGithub/LLL-TAO/blob/0fa0d72/src/LLP/include/push_notification.h)

**Design:** Template-based, lane-aware packet builder. Used to notify subscribed miners when a new block is found on their channel.

```cpp
// Example usage (stateless lane):
auto pkt = PushNotificationBuilder::BuildChannelNotification<StatelessPacket>(
    nChannel,            // 1=Prime, 2=Hash
    ProtocolLane::STATELESS,
    stateBest,           // TAO::Ledger::BlockState — current chain tip
    stateChannel,        // TAO::Ledger::BlockState — last block in this channel
    nDifficulty          // uint32_t
);
```

**Payload format (12 bytes, big-endian):**
```
[0-3]  uint32_t nUnifiedHeight   — current blockchain height
[4-7]  uint32_t nChannelHeight   — channel-specific height
[8-11] uint32_t nDifficulty      — current encoded difficulty
```

**Opcode mapping:**

| Lane | Channel | Opcode |
|------|---------|--------|
| LEGACY | Prime (1) | `0xD9` |
| LEGACY | Hash (2) | `0xDA` |
| STATELESS | Prime (1) | `0xD0D9` |
| STATELESS | Hash (2) | `0xD0DA` |

---

## 5. Key Interactions Diagram

```
                      ┌─────────────────────────────────────────┐
                      │         UPSTREAM (TAO / LLC)             │
                      │                                          │
                      │  BlockState                              │
                      │   ├── nChannelHeight  ◄── critical field │
                      │   ├── nHeight                            │
                      │   └── nBits                              │
                      │                                          │
                      │  GetLastState(state, channel)            │
                      │   └── walks chain back to find channel   │
                      │                                          │
                      │  GetNextTargetRequired(state, channel)   │
                      ���   └── computes nBits for next block      │
                      │                                          │
                      │  ChainState::tStateBest (atomic ptr)     │
                      │   └── current chain tip snapshot         │
                      │                                          │
                      │  LLC::FLKey / FalconVersion              │
                      │   └── FALCON_512 / FALCON_1024           │
                      └─────────┬────────────────────────────────┘
                                │ called by
          ┌─────────────────────▼────────────────────────────────┐
          │               LLP STATELESS LAYER                    │
          │                                                       │
          │  TemplateMetadata                                     │
          │   ├── nChannelHeight  ────► compared to GetLastState()│
          │   ├── IsStale()       ────► PRIMARY check (channel ht)│
          │   └── pBlock (unique_ptr)   SECONDARY check (age >90s)│
          │                                                       │
          │  MiningContext  (immutable value type)                │
          │   ├── fAuthenticated ──────► gate all mining ops      │
          │   ├── hashGenesis    ──────► identity + fallback reward│
          │   ├── hashRewardAddress ───► explicit payout address  │
          │   ├── vChaChaKey     ──────► encrypt reward messages  │
          │   ├── nFalconVersion ──────► 512 or 1024              │
          │   └── nLastTemplateChannelHeight ── PR #134 tracking  │
          │                                                       │
          │  StatelessMiner (pure static, no state)              │
          │   ├── ProcessMinerAuthInit()   ► send nonce challenge │
          │   ├── ProcessFalconResponse()  ► verify signature     │
          │   ├── ProcessSessionStart()    ► open session         │
          │   ├── ProcessSetChannel()      ► set Prime/Hash       │
          │   ├── ProcessSessionKeepalive()► reset expiry timer   │
          │   └── ProcessSetReward()       ► decrypt reward addr  ���
          │                                                       │
          │  StatelessMinerConnection (per-connection state)     │
          │   ├── context: MiningContext                          │
          │   ├── mapBlocks: map<merkle, TemplateMetadata>        │
          │   ├── mapSessionKeys: map<sessionId, pubkey>          │
          │   ├── m_pPrimeState / m_pHashState (fork-aware)       │
          │   └── GetCachedDifficulty() ──► 1s TTL cache nBits    │
          │                                                       │
          │  StatelessMinerManager (singleton)                   │
          │   ├── UpdateMiner() / RemoveMiner()                   │
          │   ├── GetMinerContext() by address/keyID/session/genesis│
          │   └── GetMinerLane() / HasSwitchedLanes()             │
          │                                                       │
          │  PushNotificationBuilder                             │
          │   └── BuildChannelNotification<PacketType>()         │
          │       ├── reads stateBest.nHeight                     │
          │       ├── reads stateChannel.nChannelHeight           │
          │       └── sends to subscribed miners on block found   │
          └──────────────────────────────────────────────────────┘
```

---

## 6. Authentication Handshake Flow

```
  Miner                                          Node
    │                                              │
    │──── MINER_AUTH_INIT ──────────────────────►  │
    │     [2 bytes] pubkey_len                     │ ProcessMinerAuthInit()
    │     [pubkey_len] Falcon pubkey               │  - store vMinerPubKey
    │     [2 bytes] miner_id_len                   │  - generate 32-byte nonce
    │     [miner_id_len] miner ID string           │  - store in vAuthNonce
    │                                              │
    │◄─── MINER_AUTH_CHALLENGE ─────────────────── │
    │     [32 bytes] nonce                         │
    │                                              │
    │ (miner signs nonce with its Falcon key)      │
    │                                              │
    │──── MINER_AUTH_RESPONSE ──────────────────► │
    │     Falcon pubkey                            │ ProcessFalconResponse()
    │     Falcon signature                         │  - verify sig over nonce
    │     Optional: genesis hash (32 bytes)        │  - set fAuthenticated = true
    │                                              │  - derive hashKeyID from pubkey
    │◄─── MINER_AUTH_RESULT ─────────────────────  │  - derive ChaCha20 session key
    │     [1 byte] 0x01 = success                  │
    │                                              │
    │──── SESSION_START ────────────────────────►  │ ProcessSessionStart()
    │     channel, protocol version, etc.          │  - requires fAuthenticated
    │                                              │  - sets nSessionId
    │◄─── SESSION_ACK ──────────────────────────── │  - starts session timer
    │                                              │
    │──── SET_CHANNEL ──────────────────────────►  │ ProcessSetChannel()
    │     [4 bytes] nChannel (1 or 2)              │
    │                                              │
    │──── (mining loop) ────────────────────────►  │
    │     GET_BLOCK, SUBMIT_BLOCK, KEEPALIVE...    │
    │                                              │
    │──── MINER_READY ──────────────────────────►  │ WithSubscription()
    │     subscribe to push notifications          │  - sets fSubscribedToNotifications
    │                                              │
    │◄═══ PUSH NOTIFICATION (server-initiated) ═══ │ PushNotificationBuilder
    │     [new block found on subscribed channel]  │  - triggered by chain events
```

---

## 7. Template Staleness Detection Flow

```
  StatelessMinerConnection::ProcessGetBlock()
      │
      ├── Create new block template (new_block())
      │
      ├── Call GetLastState(stateBest, nChannel)     ← UPSTREAM
      │       → returns stateChannel.nChannelHeight
      │
      ├── Construct TemplateMetadata {
      │       pBlock         = raw block pointer
      │       nHeight        = stateBest.nHeight        (unified)
      │       nChannelHeight = stateChannel.nChannelHeight  (channel-specific)
      │       nCreationTime  = now
      │       nChannel       = miner's channel
      │   }
      │
      └── Store in mapBlocks[hashMerkleRoot] = std::move(metadata)

  ─────────── every 5-10 seconds ────────────────────────────────

  StatelessMinerConnection::CleanupStaleTemplates()
      │
      └── for each (merkle, metadata) in mapBlocks:
              │
              └── metadata.IsStale()
                      │
                      ├── [1] GetLastState(stateBest, metadata.nChannel)
                      │       → get current nChannelHeight from chain
                      │
                      ├── [1] current_channel_height != (metadata.nChannelHeight - 1)?
                      │       → YES: STALE — remove template
                      │
                      └── [2] (now - metadata.nCreationTime) > 90s?
                              → YES: STALE — remove template
```

---

## 8. Protocol Lanes: Legacy vs Stateless

Both lanes now share a **single unified 32-byte keepalive response format** (PR #301).

### Port Assignment

| Port | Lane | Packet Type | Auth |
|------|------|-------------|------|
| 9323 | Stateless (primary) | `StatelessPacket` (16-bit opcodes) | Falcon + session |
| 8323 | Legacy (secondary) | `Packet` (8-bit opcodes) | Session only |

### Unified Keepalive Wire Format (both ports)

```
[0-3]   session_id          LE uint32  — session validation (was: sequence on stateless, session_id on legacy)
[4-7]   hashPrevBlock_lo32  BE uint32  — echo of miner's fork canary (0 if v1 miner)
[8-11]  unified_height      BE uint32
[12-15] hash_tip_lo32       BE uint32  — lo32 of node hashBestChain (fork cross-check)
[16-19] prime_height        BE uint32
[20-23] hash_height         BE uint32
[24-27] stake_height        BE uint32  ← THE MISSING PIECE — now tracked on miner side
[28-31] fork_score          BE uint32  — 0=healthy, >0=latent fork divergence (0 if v1 miner)
```

### What Was Deleted

| Deleted | Replaced By |
|---------|-------------|
| `BuildBestCurrentResponse()` — 28-byte legacy builder | `BuildUnifiedResponse()` — 32-byte, both ports |
| `sequence` field in stateless ACK | `session_id` (LE, consistent with legacy) |
| `nBits` in keepalive response | Miner reads nBits from 12-byte GET_BLOCK response |
| `hashBestChain_prefix` (4 raw bytes) | `hash_tip_lo32` (lo32, fork canary) |
| `OnKeepaliveAck()` + `OnLegacyKeepalive()` on miner | `OnKeepaliveResponse()` — single unified method |
| `KeepaliveTelemetryStore` / `KeepaliveTelemetrySnapshot` | `HeightTracker::Snapshot` |
| `keepalive_telemetry.hpp` | Deleted |
| `static_assert(PAYLOAD_SIZE != 28, ...)` | No longer needed (BuildBestCurrentResponse gone) |

---

## 9. Quick Reference: Function Index

| Symbol | File | Role |
|--------|------|------|
| `MiningContext` | `src/LLP/include/stateless_miner.h` | Immutable miner state snapshot |
| `MiningContext::WithChannel()` | `src/LLP/stateless_miner.cpp` | Return context with new channel |
| `MiningContext::WithGenesis()` | `src/LLP/stateless_miner.cpp` | Return context with genesis hash |
| `MiningContext::WithRewardAddress()` | `src/LLP/stateless_miner.cpp` | Bind payout address |
| `MiningContext::WithChaChaKey()` | `src/LLP/stateless_miner.cpp` | Set encryption key |
| `MiningContext::WithFalconVersion()` | `src/LLP/stateless_miner.cpp` | Set Falcon variant detected |
| `MiningContext::GetPayoutAddress()` | `src/LLP/stateless_miner.cpp` | Reward address (explicit or genesis) |
| `MiningContext::IsSessionExpired()` | `src/LLP/stateless_miner.cpp` | Check session timeout |
| `TemplateMetadata` | `src/LLP/include/stateless_miner.h` | Block template + staleness data |
| `TemplateMetadata::IsStale()` | `src/LLP/stateless_miner.cpp` | Channel-height staleness check |
| `TemplateMetadata::GetAge()` | `src/LLP/stateless_miner.cpp` | Template age in seconds |
| `ProcessResult` | `src/LLP/include/stateless_miner.h` | Handler return type (ctx+packet+error) |
| `StatelessMiner::ProcessPacket()` | `src/LLP/stateless_miner.cpp` | Packet router |
| `StatelessMiner::ProcessMinerAuthInit()` | `src/LLP/stateless_miner.cpp` | Auth step 1: receive pubkey, send nonce |
| `StatelessMiner::ProcessFalconResponse()` | `src/LLP/stateless_miner.cpp` | Auth step 2: verify Falcon sig |
| `StatelessMiner::ProcessSessionStart()` | `src/LLP/stateless_miner.cpp` | Open authenticated session |
| `StatelessMiner::ProcessSetReward()` | `src/LLP/stateless_miner.cpp` | Bind encrypted reward address |
| `StatelessMinerConnection` | `src/LLP/types/stateless_miner_connection.h` | Per-connection handler |
| `StatelessMinerManager::Get()` | `src/LLP/include/stateless_manager.h` | Global singleton access |
| `StatelessMinerManager::UpdateMiner()` | `src/LLP/include/stateless_manager.h` | Track miner context |
| `StatelessMinerManager::GetMinerContextByGenesis()` | `src/LLP/include/stateless_manager.h` | Reward mapping lookup |
| `StatelessConnection::ReadPacket()` | `src/LLP/stateless_connection.cpp` | 16-bit frame parser |
| `StatelessPacket` | `src/LLP/packets/stateless_packet.h` | Wire packet structure |
| `PushNotificationBuilder::BuildChannelNotification()` | `src/LLP/include/push_notification.h` | Build new-block push packet |
| `KeepaliveV2::BuildUnifiedResponse()` | `src/LLP/include/keepalive_v2.h` | Build 32-byte unified keepalive reply (both ports; replaces deleted `BuildBestCurrentResponse()`) |
| `KeepaliveV2::ParsePayload()` | `src/LLP/include/keepalive_v2.h` | Parse 4-byte (v1) or 8-byte (v2) keepalive request; extract session_id and miner prevhash canary |
| `StatelessMiner::ProcessKeepaliveV2()` | `src/LLP/stateless_miner.cpp` | Handle KEEPALIVE_V2 (0xD100) on stateless lane; echo miner canary; compute fork_score |
| `StatelessMiner::ProcessSessionKeepalive()` | `src/LLP/stateless_miner.cpp` | Handle SESSION_KEEPALIVE (0xD4) on stateless lane; echo miner canary and compute fork_score (mirrors stateless path) |
| `SessionStatus::SessionStatusRequest` | `src/LLP/include/session_status.h` | 8-byte miner→node session health query payload |
| `SessionStatus::SessionStatusAck` | `src/LLP/include/session_status.h` | 16-byte node→miner session health response payload |
| `SessionStatus::BuildAckPayload()` | `src/LLP/include/session_status.h` | Convenience wrapper to build SESSION_STATUS_ACK wire bytes |
| `SessionStatus::LANE_PRIMARY_ALIVE` … `LANE_AUTHENTICATED` | `src/LLP/include/session_status.h` | Lane health flag bits for ACK [4-7] |
| `SessionStatus::MINER_DEGRADED` … `MINER_SECONDARY_UP` | `src/LLP/include/session_status.h` | Miner status flag bits for REQUEST [4-7] / ACK [12-15] |
| **UPSTREAM** | | |
| `TAO::Ledger::BlockState` | `src/TAO/Ledger/types/state.h` | Full persisted block state |
| `TAO::Ledger::BlockState::nChannelHeight` | `src/TAO/Ledger/types/state.h` | Per-channel block count |
| `TAO::Ledger::GetLastState()` | `src/TAO/Ledger/state.cpp` | Walk chain to find channel block |
| `TAO::Ledger::GetNextTargetRequired()` | `src/TAO/Ledger/include/retarget.h` | Compute next difficulty |
| `TAO::Ledger::ChainState::tStateBest` | `src/TAO/Ledger/include/chainstate.h` | Atomic best chain pointer |
| `TAO::Ledger::TritiumBlock` | `src/TAO/Ledger/types/tritium.h` | Tritium-era block template |
| `LLC::FLKey` | `src/LLC/include/flkey.h` | Falcon key encapsulation |
| `LLC::FalconVersion` | `src/LLC/include/flkey.h` | FALCON_512 / FALCON_1024 enum |

---

*Generated from live code via GitHub Symbol Tab — `NamecoinGithub/LLL-TAO` branch `0fa0d72`. For the latest symbol definitions, use the Symbol Tab in the GitHub file viewer on `src/LLP/stateless_miner.cpp`.*
