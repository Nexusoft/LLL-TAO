# Graceful Miner Shutdown Protocol

## Overview

When the node shuts down (Ctrl+C / SIGTERM), connected miners receive an explicit
`NODE_SHUTDOWN` notification before TCP connections are torn down.  This lets miners
stop their workers cleanly and observe a reconnect backoff instead of spinning in a
rapid reconnect loop.

Both **stateless** (port 9323) and **legacy** (port 8323) miners receive the same
`NODE_SHUTDOWN (0xD0FF)` packet via a single unified shutdown sequence.

---

## Node-Initiated Shutdown Sequence

1. `LLP::Shutdown()` calls `GracefulDisconnectAllMiners()` **first**, before any server teardown.
2. **Phase 1** — Every connected miner on **both** lanes receives `NODE_SHUTDOWN (0xD0FF)` with a 4-byte reason code.
3. The mining DataThreads are woken so any buffered shutdown packets can flush immediately.
4. **Phase 2** — A single shared flush window (250 ms) gives the kernel TCP send buffers time to transmit the packets before hard close.
5. **Phase 3** — `Disconnect()` closes the TCP socket for each miner and DataThreads are woken again for teardown.
6. `ColinMiningAgent::on_node_shutdown()` runs after the miner disconnect phase, and any exception is logged without aborting shutdown.
7. `FalconAuth::Shutdown()` and then the miner servers (`STATELESS_MINER_SERVER`, `MINING_SERVER`) shut down cleanly.

**Debug log order (expected):**

```
GracefulDisconnectAllMiners: Sending graceful disconnect to all connected miners...
GracefulDisconnectAllMiners: NODE_SHUTDOWN queued for N/M stateless and X/Y legacy miners; waiting 250 ms for egress before disconnect
GracefulDisconnectAllMiners: Graceful disconnect complete: N stateless + M legacy miners disconnected
[Colin shutdown report ...]
Shutting down LLP
```

---

## NODE_SHUTDOWN Wire Format (0xD0FF)

| Field       | Bytes | Encoding    | Value                              |
|-------------|-------|-------------|-------------------------------------|
| opcode      | 2     | big-endian  | `0xD0FF`                           |
| length      | 4     | big-endian  | `4`                                |
| reason_code | 4     | big-endian  | See table below                    |

**Reason codes:**

| Code         | Value        | Meaning                  |
|--------------|--------------|--------------------------|
| GRACEFUL     | `0x00000001` | Planned node shutdown    |
| MAINTENANCE  | `0x00000002` | Maintenance restart      |

---

## Miner-Side Behaviour (NexusMiner)

On receipt of `NODE_SHUTDOWN (0xD0FF)`:

1. Stop all workers cleanly.
2. Log: `[Solo] Node sent graceful shutdown notice (reason=GRACEFUL) — stopping workers`.
3. Hold reconnect for `NODE_SHUTDOWN_BACKOFF_S` (recommended: 60 s).
4. Reconnect normally after backoff expires.

---

## Channel Name Canonical Source

`MiningContext::strChannelName` is the **single source of truth** for human-readable
channel labels ("Prime" / "Hash").  It is set via `MiningContext::WithChannelName()`
whenever `nChannel` changes (e.g. in `ProcessSetChannel()`).

Use `context.strChannelName` in log lines instead of the inline ternary
`(nChannel == 1) ? "Prime" : "Hash"`.

The static helper `MiningContext::ChannelName(uint32_t nChannel)` returns the same
string and can be used where no `MiningContext` is available:

```cpp
// Preferred (context available):
debug::log(0, "Channel: ", context.strChannelName);

// Acceptable (no context, e.g. block submission path):
debug::log(0, "Channel: ", MiningContext::ChannelName(pBlock->nChannel));
```

---

## Files Changed

| File | Change |
|------|--------|
| `src/LLP/include/opcode_utility.h` | Added `NODE_SHUTDOWN = 0xD0FF` to `Stateless` namespace |
| `src/LLP/include/graceful_shutdown.h` | Shared shutdown reason / flush timing constants and per-connection notification state |
| `src/LLP/include/stateless_miner.h` | Added `strChannelName` field, `ChannelName()` static, `WithChannelName()` builder to `MiningContext` |
| `src/LLP/stateless_miner.cpp` | Initialized `strChannelName` in both constructors; implemented `ChannelName()` and `WithChannelName()`; updated `ProcessSetChannel()` |
| `src/LLP/types/stateless_miner_connection.h` | Declared `SendNodeShutdown(uint32_t)` |
| `src/LLP/stateless_miner_connection.cpp` | Implemented `SendNodeShutdown()`; replaced inline channel ternaries with `context.strChannelName` |
| `src/LLP/include/colin_mining_agent.h` | Declared `on_node_shutdown()` |
| `src/LLP/colin_mining_agent.cpp` | Implemented `on_node_shutdown()` |
| `src/LLP/global.cpp` | `GracefulDisconnectAllMiners()` sends NODE_SHUTDOWN to both stateless and legacy lanes in one unified sequence |
| `src/LLP/types/miner.h` | Declared `SendNodeShutdown(uint32_t)` on the legacy `Miner` class |
| `src/LLP/miner.cpp` | Implemented `Miner::SendNodeShutdown()` via `respond_stateless()` |
| `tests/unit/LLP/graceful_shutdown_tests.cpp` | New unit tests |
