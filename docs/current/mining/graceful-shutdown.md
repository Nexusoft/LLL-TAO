# Graceful Miner Shutdown Protocol

## Overview

When the node shuts down (Ctrl+C / SIGTERM), connected miners receive an explicit
`NODE_SHUTDOWN` notification before TCP connections are torn down.  This lets miners
stop their workers cleanly and observe a reconnect backoff instead of spinning in a
rapid reconnect loop.

---

## Node-Initiated Shutdown Sequence

1. `LLP::Shutdown()` calls `GracefulDisconnectAllMiners()` **first**, before any server teardown.
2. `ColinMiningAgent::on_node_shutdown()` emits the final diagnostic report so the last Colin snapshot (connected miners, last template, submission stats) is captured in `debug.log`.
3. Each connected stateless miner receives `NODE_SHUTDOWN (0xD0FF)` with a 4-byte reason code.
4. A 20 ms flush window per miner allows the packet to be delivered before the socket closes.
5. `Disconnect()` closes the TCP socket for each miner.
6. Legacy miners (port 8323) receive a bare `Disconnect()` (no shutdown packet — legacy protocol has no equivalent opcode).
7. `FalconAuth::Shutdown()` and then the miner servers (`STATELESS_MINER_SERVER`, `MINING_SERVER`) shut down cleanly.

**Debug log order (expected):**

```
GracefulDisconnectAllMiners: Sending graceful disconnect to all connected miners...
[Colin shutdown report ...]
GracefulDisconnectAllMiners: Graceful disconnect complete: N stateless + M legacy miners disconnected
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
| `src/LLP/include/stateless_miner.h` | Added `strChannelName` field, `ChannelName()` static, `WithChannelName()` builder to `MiningContext` |
| `src/LLP/stateless_miner.cpp` | Initialized `strChannelName` in both constructors; implemented `ChannelName()` and `WithChannelName()`; updated `ProcessSetChannel()` |
| `src/LLP/types/stateless_miner_connection.h` | Declared `SendNodeShutdown(uint32_t)` |
| `src/LLP/stateless_miner_connection.cpp` | Implemented `SendNodeShutdown()`; replaced inline channel ternaries with `context.strChannelName` |
| `src/LLP/include/colin_mining_agent.h` | Declared `on_node_shutdown()` |
| `src/LLP/colin_mining_agent.cpp` | Implemented `on_node_shutdown()` |
| `src/LLP/global.cpp` | Added `GracefulDisconnectAllMiners()` static function; call it at start of `LLP::Shutdown()` |
| `tests/unit/LLP/graceful_shutdown_tests.cpp` | New unit tests |
