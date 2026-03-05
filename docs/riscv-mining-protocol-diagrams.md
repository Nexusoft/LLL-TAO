# RISC-V Mining Protocol Architecture Diagrams

## Executive Summary

This document provides comprehensive C++ architecture diagrams for the unified mining protocol implementation in Nexus LLL-TAO. These diagrams focus on the dual-lane (Legacy + Stateless) mining architecture, Protocol Lane identification, and push notification broadcasting system.

## Table of Contents

1. [Protocol Lane Architecture](#protocol-lane-architecture)
2. [MiningContext Structure](#miningcontext-structure)
3. [Push Notification Flow](#push-notification-flow)
4. [MinerPushDispatcher Integration](#minerpushdispatcher-integration)
5. [Connection Initialization](#connection-initialization)
6. [Lane-Aware Opcode Selection](#lane-aware-opcode-selection)

---

## 1. Protocol Lane Architecture

### Overview

The Nexus mining protocol supports two parallel "lanes" for miner communication:

```
┌─────────────────────────────────────────────────────────────────┐
│                    NEXUS MINING ARCHITECTURE                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌───────────────────────┐       ┌───────────────────────┐    │
│  │   LEGACY LANE         │       │   STATELESS LANE      │    │
│  │   (Port 8323)         │       │   (Port 9323)         │    │
│  ├───────────────────────┤       ├───────────────────────┤    │
│  │ • 8-bit opcodes       │       │ • 16-bit opcodes      │    │
│  │ • 0xD9 (Prime)        │       │ • 0xD0D9 (Prime)      │    │
│  │ • 0xDA (Hash)         │       │ • 0xD0DA (Hash)       │    │
│  │                       │       │                       │    │
│  │ Connection Type:      │       │ Connection Type:      │    │
│  │   Miner               │       │   StatelessMiner      │    │
│  │                       │       │   Connection          │    │
│  └───────────────────────┘       └───────────────────────┘    │
│              │                             │                   │
│              └─────────────┬───────────────┘                   │
│                            │                                   │
│                   ┌────────▼────────┐                         │
│                   │  MiningContext  │                         │
│                   │  nProtocolLane  │                         │
│                   └─────────────────┘                         │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

### Lane Identification

```cpp
// ProtocolLane enum in push_notification.h
enum class ProtocolLane : uint8_t
{
    LEGACY    = 0,  // Legacy Tritium Protocol (8-bit opcodes, port 8323)
    STATELESS = 1   // Stateless Tritium Protocol (16-bit opcodes, port 9323)
};
```

---

## 2. MiningContext Structure

### Class Diagram

```
┌────────────────────────────────────────────────────────────────┐
│                        MiningContext                            │
├────────────────────────────────────────────────────────────────┤
│ Fields (Immutable State):                                       │
│                                                                 │
│  Channel Information:                                           │
│    • uint32_t nChannel                - Mining channel (1=Prime, 2=Hash)
│    • std::string strChannelName       - "Prime" | "Hash"       │
│                                                                 │
│  Protocol Lane (NEW - Phase 3):                                │
│    • ProtocolLane nProtocolLane       - LEGACY or STATELESS    │
│                                                                 │
│  Session Tracking:                                              │
│    • uint32_t nHeight                 - Unified blockchain height
│    • uint64_t nTimestamp              - Last activity timestamp│
│    • std::string strAddress           - Miner's IP address     │
│    • uint32_t nProtocolVersion        - Protocol version       │
│    • uint32_t nSessionId              - Unique session ID      │
│    • uint64_t nSessionStart           - Session start time     │
│    • uint64_t nSessionTimeout         - Timeout in seconds     │
│                                                                 │
│  Authentication (Falcon):                                       │
│    • bool fAuthenticated              - Auth success flag      │
│    • uint256_t hashKeyID              - Falcon key identifier  │
│    • uint256_t hashGenesis            - Genesis hash           │
│    • std::string strUserName          - Username               │
│    • std::vector<uint8_t> vAuthNonce  - Challenge nonce        │
│    • std::vector<uint8_t> vMinerPubKey - Falcon public key     │
│                                                                 │
│  Encryption (ChaCha20):                                         │
│    • std::vector<uint8_t> vChaChaKey  - Session key            │
│    • bool fEncryptionReady            - Encryption state       │
│                                                                 │
│  Push Notifications:                                            │
│    • bool fSubscribedToNotifications  - MINER_READY received   │
│    • uint32_t nSubscribedChannel      - Subscribed channel     │
│    • uint64_t nLastNotificationTime   - Last push timestamp    │
│    • uint64_t nNotificationsSent      - Total push count       │
│                                                                 │
│  Template Tracking:                                             │
│    • uint32_t nLastTemplateChannelHeight - Last template height│
│    • uint1024_t hashLastBlock         - Best chain snapshot    │
│    • CanonicalChainState canonical_snap - Chain state snapshot │
│                                                                 │
├────────────────────────────────────────────────────────────────┤
│ Factory Methods (Immutable Updates):                            │
│                                                                 │
│    • WithChannel(uint32_t)            → MiningContext          │
│    • WithHeight(uint32_t)             → MiningContext          │
│    • WithAuth(bool)                   → MiningContext          │
│    • WithProtocolLane(ProtocolLane)   → MiningContext  [NEW]   │
│    • WithSubscription(uint32_t)       → MiningContext          │
│    • WithNotificationSent(uint64_t)   → MiningContext          │
│    • [... 15+ other With* methods]                             │
│                                                                 │
├────────────────────────────────────────────────────────────────┤
│ Helper Methods:                                                 │
│                                                                 │
│    • IsStateless() const → bool               [NEW]            │
│      Returns true if nProtocolLane == STATELESS                │
│                                                                 │
│    • GetPayoutAddress() const → uint256_t                      │
│      Returns reward address or genesis hash                    │
│                                                                 │
│    • HasValidPayout() const → bool                             │
│      Checks if payout address is set                           │
│                                                                 │
│    • static ChannelName(uint32_t) → std::string               │
│      Converts channel number to name                           │
│                                                                 │
└────────────────────────────────────────────────────────────────┘
```

### Usage Example

```cpp
// Initialize context with default values (LEGACY lane)
LLP::MiningContext ctx;

// Update context immutably
ctx = ctx.WithChannel(1)                                  // Prime channel
         .WithProtocolLane(ProtocolLane::STATELESS)       // Stateless lane
         .WithAuth(true)                                  // Authenticated
         .WithSubscription(1);                            // Subscribe to Prime

// Check lane type
if (ctx.IsStateless()) {
    // Use 16-bit opcodes (0xD0D9, 0xD0DA)
} else {
    // Use 8-bit opcodes (0xD9, 0xDA)
}
```

---

## 3. Push Notification Flow

### High-Level Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                    PUSH NOTIFICATION FLOW                         │
└──────────────────────────────────────────────────────────────────┘

    New Block Mined or Accepted
             │
             ▼
    ┌────────────────────┐
    │  TAO::Ledger::     │
    │  BlockState::      │
    │  SetBest()         │
    └────────┬───────────┘
             │
             ▼
    ┌─────────────────────────────────────┐
    │  MinerPushDispatcher::              │
    │  DispatchPushEvent(nHeight, hash)   │
    │                                     │
    │  • Deduplication check              │
    │  • Extract 4-byte hash prefix       │
    │  • CAS atomic update                │
    └────────┬───────────────────────────┘
             │
             ├──────────────┬──────────────┐
             ▼              ▼              ▼
      ┌──────────┐   ┌──────────┐   [Channels]
      │  Prime   │   │   Hash   │
      │ Channel  │   │ Channel  │
      └────┬─────┘   └────┬─────┘
           │              │
           ├──────────────┴──────────────┐
           │                             │
           ▼                             ▼
    ┌─────────────┐              ┌─────────────┐
    │ STATELESS   │              │   LEGACY    │
    │   LANE      │              │    LANE     │
    │ (Port 9323) │              │ (Port 8323) │
    └──────┬──────┘              └──────┬──────┘
           │                             │
           │  For each subscribed miner: │
           │                             │
           ▼                             ▼
    ┌──────────────────┐        ┌──────────────────┐
    │ StatelessMiner   │        │      Miner       │
    │ Connection::     │        │ ::SendChannel    │
    │ SendChannel      │        │ Notification()   │
    │ Notification()   │        └────────┬─────────┘
    └────────┬─────────┘                 │
             │                            │
             └────────────┬───────────────┘
                          │
                          ▼
            ┌──────────────────────────────┐
            │  PushNotificationBuilder::   │
            │  BuildChannelNotification    │
            │                              │
            │  Inputs:                     │
            │    • nChannel (1 or 2)       │
            │    • ProtocolLane (enum)     │
            │    • stateBest               │
            │    • stateChannel            │
            │    • nDifficulty             │
            │    • hashBestChain           │
            │                              │
            │  Output:                     │
            │    • Packet/StatelessPacket  │
            │    • Lane-aware opcode       │
            │    • 140-byte payload        │
            └──────────────────────────────┘
```

### Deduplication Mechanism

```
┌────────────────────────────────────────────────────────────┐
│              DEDUPLICATION STATE (ATOMIC)                   │
├────────────────────────────────────────────────────────────┤
│                                                             │
│  Per-Channel Atomic Keys (uint64_t):                       │
│                                                             │
│    s_nPrimeDedup  =  [height:32][hash_prefix:32]          │
│    s_nHashDedup   =  [height:32][hash_prefix:32]          │
│                                                             │
│  Algorithm:                                                 │
│                                                             │
│    1. Extract hash prefix (first 4 bytes of hashBestChain) │
│    2. Pack: nNewKey = (nHeight << 32) | hashPrefix4        │
│    3. For each channel:                                     │
│         nOldKey = s_nChannelDedup.load(acquire)            │
│         if (nOldKey != nNewKey)                            │
│             if (CAS(nOldKey, nNewKey))                     │
│                 BroadcastChannel(...)                      │
│             else                                            │
│                 // Another thread won - skip               │
│         else                                                │
│             // Already dispatched - skip                    │
│                                                             │
│  Result: Exactly ONE broadcast per (height, hash, channel) │
│                                                             │
└────────────────────────────────────────────────────────────┘
```

---

## 4. MinerPushDispatcher Integration

### Class Responsibility

```
┌────────────────────────────────────────────────────────────┐
│                   MinerPushDispatcher                       │
├────────────────────────────────────────────────────────────┤
│                                                             │
│  Responsibility:                                            │
│    Canonical single entry-point for ALL miner push         │
│    notification broadcasts across the entire system.       │
│                                                             │
│  Design Goals:                                              │
│    1. Exactly 4 lane-level sends per push event            │
│       • Prime → Legacy lane                                 │
│       • Prime → Stateless lane                              │
│       • Hash → Legacy lane                                  │
│       • Hash → Stateless lane                               │
│                                                             │
│    2. Deduplication by (height, hash_prefix, channel)      │
│       • Prevents duplicate sends from double-calls          │
│       • Thread-safe atomic CAS operations                   │
│                                                             │
│    3. Unified logging at verbosity 0                        │
│       • One line per (channel, lane)                        │
│       • Summary line per channel with send counts           │
│                                                             │
├────────────────────────────────────────────────────────────┤
│  Public API:                                                │
│                                                             │
│    static void DispatchPushEvent(                          │
│        uint32_t nHeight,                                    │
│        const uint1024_t& hashBestChain                     │
│    );                                                       │
│                                                             │
│  Private Implementation:                                    │
│                                                             │
│    static void BroadcastChannel(                           │
│        uint32_t nChannel,                                   │
│        uint32_t nHeight,                                    │
│        uint32_t hashPrefix4                                 │
│    );                                                       │
│                                                             │
│    static std::atomic<uint64_t> s_nPrimeDedup;             │
│    static std::atomic<uint64_t> s_nHashDedup;              │
│                                                             │
└────────────────────────────────────────────────────────────┘
```

### State Transition (Before → After Phase 3)

```
BEFORE Phase 3 (Inline Lambda in state.cpp):
┌────────────────────────────────────────────────────────┐
│  SetBest() {                                            │
│    ...                                                  │
│    auto BroadcastChannelNotification = [&](ch) {       │
│        if (STATELESS_SERVER)                           │
│            nS = STATELESS_SERVER->NotifyChannelMiners  │
│        if (MINING_SERVER)                              │
│            nL = MINING_SERVER->NotifyChannelMiners     │
│        log(...)                                         │
│    };                                                   │
│    BroadcastChannelNotification(PRIME);                │
│    BroadcastChannelNotification(HASH);                 │
│  }                                                      │
└────────────────────────────────────────────────────────┘

AFTER Phase 3 (Canonical Dispatcher):
┌────────────────────────────────────────────────────────┐
│  SetBest() {                                            │
│    ...                                                  │
│    MinerPushDispatcher::DispatchPushEvent(             │
│        nHeight,                                         │
│        hash                                             │
│    );                                                   │
│  }                                                      │
└────────────────────────────────────────────────────────┘

Benefits:
  ✓ Single canonical entry point
  ✓ Deduplication built-in
  ✓ Unified logging
  ✓ Easier testing and maintenance
  ✓ No code duplication
```

---

## 5. Connection Initialization

### StatelessMinerConnection Initialization

```
┌──────────────────────────────────────────────────────────┐
│  StatelessMinerConnection::Event(EVENTS::CONNECT)       │
├──────────────────────────────────────────────────────────┤
│                                                           │
│  1. DDOS cooldown check                                  │
│     ├─ If in cooldown: Disconnect                        │
│     └─ Else: Continue                                    │
│                                                           │
│  2. Log connection from IP:port                          │
│                                                           │
│  3. Initialize MiningContext:                            │
│     ┌──────────────────────────────────────┐           │
│     │ context = MiningContext(              │           │
│     │     nChannel: 0,                      │           │
│     │     nHeight: ChainState::nBestHeight, │           │
│     │     strAddress: GetAddress(),         │           │
│     │     fAuthenticated: false,            │           │
│     │     ... other defaults ...            │           │
│     │ );                                    │           │
│     └──────────────────────────────────────┘           │
│                                                           │
│  4. Set Protocol Lane: [NEW - Phase 3]                   │
│     ┌──────────────────────────────────────┐           │
│     │ context = context.WithProtocolLane(  │           │
│     │     ProtocolLane::STATELESS          │           │
│     │ );                                    │           │
│     └──────────────────────────────────────┘           │
│                                                           │
│  5. Attempt session recovery by IP address               │
│                                                           │
│  6. Initialize channel state managers (Prime + Hash)     │
│                                                           │
└──────────────────────────────────────────────────────────┘
```

### Legacy Miner Initialization

```
┌──────────────────────────────────────────────────────────┐
│  Miner::GetContext()                                      │
├──────────────────────────────────────────────────────────┤
│                                                           │
│  Called on-demand when context is needed (e.g., for      │
│  push notifications)                                      │
│                                                           │
│  ┌────────────────────────────────────────┐             │
│  │ MiningContext ctx;                      │             │
│  │ ctx.nChannel = nChannel;                │             │
│  │ ctx.fSubscribedToNotifications =        │             │
│  │     fSubscribedToNotifications;         │             │
│  │ ctx.nSubscribedChannel =                │             │
│  │     nSubscribedChannel;                 │             │
│  │ ctx.strAddress = GetAddress();          │             │
│  │                                         │             │
│  │ // NEW - Phase 3:                       │             │
│  │ ctx.nProtocolLane =                     │             │
│  │     ProtocolLane::LEGACY;               │             │
│  │                                         │             │
│  │ return ctx;                             │             │
│  └────────────────────────────────────────┘             │
│                                                           │
└──────────────────────────────────────────────────────────┘
```

---

## 6. Lane-Aware Opcode Selection

### Opcode Mapping Table

```
┌─────────────────────────────────────────────────────────────┐
│                    OPCODE SELECTION TABLE                    │
├────────────┬────────────────┬─────────────┬────────────────┤
│  Channel   │  Lane          │  Opcode     │  Decimal       │
├────────────┼────────────────┼─────────────┼────────────────┤
│  Prime (1) │  LEGACY        │  0xD9       │  217           │
│  Prime (1) │  STATELESS     │  0xD0D9     │  53465         │
│  Hash (2)  │  LEGACY        │  0xDA       │  218           │
│  Hash (2)  │  STATELESS     │  0xD0DA     │  53466         │
└────────────┴────────────────┴─────────────┴────────────────┘
```

### PushNotificationBuilder Implementation

```cpp
// Unified template-based builder (supports both lanes)
template<typename PacketType>
static PacketType BuildChannelNotification(
    uint32_t nChannel,                      // 1=Prime, 2=Hash
    ProtocolLane lane,                      // LEGACY or STATELESS
    const TAO::Ledger::BlockState& stateBest,
    const TAO::Ledger::BlockState& stateChannel,
    uint32_t nDifficulty,
    const uint1024_t& hashBestChain
) {
    PacketType packet;

    // Lane-aware opcode selection
    packet.HEADER = GetNotificationOpcode(nChannel, lane);

    // 140-byte payload (big-endian network byte order)
    std::vector<uint8_t> payload = BuildPayload(
        stateBest.nHeight,              // [0-3]   unified height
        stateChannel.nChannelHeight,    // [4-7]   channel height
        nDifficulty,                    // [8-11]  difficulty
        hashBestChain                   // [12-139] best chain hash
    );

    packet.SetData(payload);
    return packet;
}
```

### Opcode Selection Logic

```
GetNotificationOpcode(nChannel, lane):
┌─────────────────────────────────────────────┐
│  if (lane == LEGACY)                        │
│      if (nChannel == 1)                     │
│          return 0xD9  // PRIME_BLOCK_AVAIL  │
│      else if (nChannel == 2)                │
│          return 0xDA  // HASH_BLOCK_AVAIL   │
│  else if (lane == STATELESS)                │
│      if (nChannel == 1)                     │
│          return 0xD0D9 // PRIME_BLOCK_AVAIL │
│      else if (nChannel == 2)                │
│          return 0xD0DA // HASH_BLOCK_AVAIL  │
│  else                                        │
│      throw error("Invalid channel/lane")    │
└─────────────────────────────────────────────┘
```

---

## 7. Testing Strategy

### Unit Test Coverage

```
┌────────────────────────────────────────────────────────────┐
│               TEST COVERAGE MATRIX                          │
├────────────────────────────────────────────────────────────┤
│                                                             │
│  1. MiningContext ProtocolLane Field                       │
│     ✓ Default constructor initializes to LEGACY           │
│     ✓ Parameterized constructor initializes to LEGACY     │
│     ✓ WithProtocolLane() sets LEGACY lane                 │
│     ✓ WithProtocolLane() sets STATELESS lane              │
│     ✓ IsStateless() returns false for LEGACY              │
│     ✓ IsStateless() returns true for STATELESS            │
│     ✓ Chaining WithProtocolLane() with other methods      │
│     ✓ Immutability preserved across updates               │
│                                                             │
│  2. PushNotificationBuilder (Existing)                     │
│     ✓ Opcode selection for LEGACY lane                    │
│     ✓ Opcode selection for STATELESS lane                 │
│     ✓ Payload construction (140 bytes)                    │
│     ✓ Big-endian encoding validation                      │
│                                                             │
│  3. MinerPushDispatcher (Existing)                         │
│     ✓ Deduplication by (height, hash, channel)            │
│     ✓ Atomic CAS operations                               │
│     ✓ Broadcast to both lanes                             │
│     ✓ Logging at verbosity 0                              │
│                                                             │
│  4. Push Throttle (Existing)                               │
│     ✓ First push always sends                             │
│     ✓ Second push within interval throttled               │
│     ✓ m_force_next_push bypass                            │
│                                                             │
└────────────────────────────────────────────────────────────┘
```

### Test Files

- `tests/unit/LLP/test_protocol_lane.cpp` — NEW (Phase 3)
- `tests/unit/LLP/push_notification_tests.cpp` — Existing
- `tests/unit/LLP/push_throttle_test.cpp` — Existing

---

## 8. Memory Layout and Performance

### MiningContext Size Analysis

```
┌────────────────────────────────────────────────────────────┐
│              MININGCONTEXT MEMORY LAYOUT                    │
├────────────────────────────────────────────────────────────┤
│                                                             │
│  Fixed-size fields:                                         │
│    • Scalars (uint32/64, bool):        ~120 bytes         │
│    • uint256_t fields (2x):            ~64 bytes          │
│    • uint1024_t (hashLastBlock):       ~128 bytes         │
│    • std::array<uint8_t, 4>:           ~4 bytes           │
│                                                             │
│  Variable-size fields:                                      │
│    • std::string (strAddress, etc):    ~32-128 bytes      │
│    • std::vector (vAuthNonce, etc):    ~32-256 bytes      │
│    • CanonicalChainState:              ~sizeof(struct)     │
│                                                             │
│  NEW Field (Phase 3):                                       │
│    • ProtocolLane (uint8_t):           1 byte + padding    │
│                                                             │
│  Total (approximate):                  ~600-800 bytes      │
│                                                             │
│  Performance Impact:                                        │
│    • Immutable updates: O(1) copy + field update          │
│    • IsStateless(): O(1) enum comparison                  │
│    • WithProtocolLane(): O(1) copy + assignment           │
│                                                             │
└────────────────────────────────────────────────────────────┘
```

### Atomic Deduplication Performance

```
┌────────────────────────────────────────────────────────────┐
│          ATOMIC DEDUPLICATION PERFORMANCE                   │
├────────────────────────────────────────────────────────────┤
│                                                             │
│  Algorithm: Compare-And-Swap (CAS)                         │
│                                                             │
│  Per-Channel Cost:                                          │
│    • 1x atomic load (acquire): ~5-10 cycles               │
│    • 1x comparison: ~1 cycle                               │
│    • 1x atomic CAS (release): ~10-20 cycles               │
│                                                             │
│  Total per DispatchPushEvent():                            │
│    • Prime channel: ~30-60 cycles                          │
│    • Hash channel: ~30-60 cycles                           │
│    • Total: ~60-120 cycles (~20-40 ns on 3 GHz CPU)       │
│                                                             │
│  Contention Handling:                                       │
│    • Failed CAS: Thread loses race, skips broadcast        │
│    • No retry loop needed                                   │
│    • Lock-free algorithm                                    │
│                                                             │
└────────────────────────────────────────────────────────────┘
```

---

## 9. RISC-V Architecture Considerations

### Cache Line Alignment

```
┌────────────────────────────────────────────────────────────┐
│               CACHE LINE OPTIMIZATION                       │
├────────────────────────────────────────────────────────────┤
│                                                             │
│  RISC-V Cache Line Size: 64 bytes (typical)                │
│                                                             │
│  Atomic Variables Layout:                                   │
│                                                             │
│    Cache Line 0:                                            │
│      [0-7]:   s_nPrimeDedup (uint64_t)                     │
│      [8-63]:  Padding / other data                         │
│                                                             │
│    Cache Line 1:                                            │
│      [0-7]:   s_nHashDedup (uint64_t)                      │
│      [8-63]:  Padding / other data                         │
│                                                             │
│  False Sharing Prevention:                                  │
│    • Separate atomic variables should be on different       │
│      cache lines to prevent false sharing                   │
│    • alignas(64) can be used if needed                     │
│                                                             │
│  RISC-V Atomic Instructions Used:                          │
│    • LR.D (Load-Reserved Double)                           │
│    • SC.D (Store-Conditional Double)                       │
│    • FENCE (Memory ordering)                               │
│                                                             │
└────────────────────────────────────────────────────────────┘
```

### Thread Safety Guarantees

```
┌────────────────────────────────────────────────────────────┐
│                 THREAD SAFETY MODEL                         │
├────────────────────────────────────────────────────────────┤
│                                                             │
│  MiningContext:                                             │
│    • Immutable after construction                          │
│    • All updates create NEW context                        │
│    • Thread-safe by design (no shared state)              │
│    • Copy semantics (value type)                           │
│                                                             │
│  MinerPushDispatcher:                                       │
│    • Static atomic variables for dedup state               │
│    • CAS operations with memory_order_acquire/release      │
│    • Lock-free algorithm                                    │
│    • Safe for concurrent SetBest() calls                   │
│                                                             │
│  Connection Objects:                                        │
│    • StatelessMinerConnection: LOCK(MUTEX) for context     │
│    • Miner: LOCK(MUTEX) for GetContext()                   │
│    • Per-connection synchronization                         │
│                                                             │
└────────────────────────────────────────────────────────────┘
```

---

## 10. Migration Path and Backward Compatibility

### Phase 3 Changes Summary

```
┌────────────────────────────────────────────────────────────┐
│               PHASE 3 REFACTOR SUMMARY                      │
├────────────────────────────────────────────────────────────┤
│                                                             │
│  ADDED:                                                     │
│    ✓ MiningContext.nProtocolLane field                     │
│    ✓ MiningContext.WithProtocolLane() factory method       │
│    ✓ MiningContext.IsStateless() helper method             │
│                                                             │
│  MODIFIED:                                                  │
│    ✓ MiningContext constructors (initialize nProtocolLane) │
│    ✓ StatelessMinerConnection::Event() CONNECT case        │
│    ✓ Miner::GetContext() (set nProtocolLane)               │
│    ✓ state.cpp SetBest() (use MinerPushDispatcher)         │
│                                                             │
│  REMOVED:                                                   │
│    ✓ Inline BroadcastChannelNotification lambda            │
│      (replaced with MinerPushDispatcher::DispatchPushEvent)│
│                                                             │
│  UNCHANGED:                                                 │
│    ✓ Protocol opcodes (8-bit vs 16-bit)                   │
│    ✓ Packet formats (140-byte payload)                     │
│    ✓ Miner behavior (no protocol changes)                  │
│    ✓ Network communication (ports 8323, 9323)             │
│                                                             │
└────────────────────────────────────────────────────────────┘
```

### Backward Compatibility

```
┌────────────────────────────────────────────────────────────┐
│            BACKWARD COMPATIBILITY ANALYSIS                  │
├────────────────────────────────────────────────────────────┤
│                                                             │
│  Network Protocol:                                          │
│    ✓ No changes to wire format                            │
│    ✓ No changes to opcodes                                 │
│    ✓ No changes to packet structure                        │
│    ✓ Miners do not need updates                            │
│                                                             │
│  API Compatibility:                                         │
│    ✓ MiningContext is internal structure                   │
│    ✓ No public API changes                                 │
│    ✓ Factory method additions (backward compatible)        │
│                                                             │
│  State/Storage:                                             │
│    ✓ No persistent storage changes                         │
│    ✓ Session recovery unaffected                           │
│    ✓ MiningContext not serialized to disk                  │
│                                                             │
│  Testing:                                                   │
│    ✓ Existing tests pass unchanged                         │
│    ✓ New tests added for Phase 3 features                 │
│                                                             │
└────────────────────────────────────────────────────────────┘
```

---

## 11. Future Extensions

### Potential Phase 4+ Features

```
┌────────────────────────────────────────────────────────────┐
│              FUTURE EXTENSION OPPORTUNITIES                 │
├────────────────────────────────────────────────────────────┤
│                                                             │
│  1. Multi-Lane Session Migration                           │
│     • Allow miners to upgrade from LEGACY to STATELESS     │
│     • Preserve session state during migration              │
│     • Graceful fallback if STATELESS unavailable           │
│                                                             │
│  2. Lane-Specific Statistics                                │
│     • Track push notification counts per lane              │
│     • Monitor lane-specific performance metrics            │
│     • Colin Mining Agent integration                        │
│                                                             │
│  3. Dynamic Lane Selection                                  │
│     • Auto-select optimal lane based on miner capabilities │
│     • Load balancing across lanes                          │
│     • Failover between lanes                               │
│                                                             │
│  4. Protocol Version Negotiation                            │
│     • Advertise supported protocol versions                │
│     • Allow miners to request preferred lane               │
│     • Backward compatibility with older miners             │
│                                                             │
│  5. Lane-Aware Rate Limiting                                │
│     • Per-lane DDOS protection                             │
│     • Different rate limits for LEGACY vs STATELESS        │
│     • Fair resource allocation                             │
│                                                             │
└────────────────────────────────────────────────────────────┘
```

---

## References

### Source Files

- `src/LLP/include/stateless_miner.h` — MiningContext definition
- `src/LLP/stateless_miner.cpp` — MiningContext implementation
- `src/LLP/include/push_notification.h` — ProtocolLane enum
- `src/LLP/include/miner_push_dispatcher.h` — Dispatcher interface
- `src/LLP/miner_push_dispatcher.cpp` — Dispatcher implementation
- `src/TAO/Ledger/state.cpp` — SetBest() integration
- `src/LLP/stateless_miner_connection.cpp` — Stateless lane initialization
- `src/LLP/miner.cpp` — Legacy lane initialization

### Test Files

- `tests/unit/LLP/test_protocol_lane.cpp` — Protocol lane tests (NEW)
- `tests/unit/LLP/push_notification_tests.cpp` — Opcode selection tests
- `tests/unit/LLP/push_throttle_test.cpp` — Push throttle tests

### Documentation

- `docs/riscv-design.md` — RISC-V architecture overview
- `docs/riscv-build-guide.md` — Build instructions for RISC-V
- `docs/diagrams/push-notification-flow.md` — Push notification diagrams

---

## Conclusion

This document provides a comprehensive C++ architecture reference for the unified mining protocol implementation in Nexus LLL-TAO. The Phase 3 refactor consolidates push notification broadcasting into a single canonical entry point (MinerPushDispatcher) while adding explicit protocol lane tracking to MiningContext. This improves maintainability, testability, and clarity without changing the wire protocol or requiring miner updates.

The architecture is designed for RISC-V portability, using standard C++17 features, atomic operations, and lock-free algorithms that map efficiently to RISC-V atomic instructions (LR/SC).

---

**Document Version:** 1.0
**Last Updated:** 2026-03-05
**Authors:** Nexus Development Team
**License:** MIT
