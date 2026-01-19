# Mining Notification Flow - Diagnostics Guide

## Overview

This document describes the enhanced diagnostics added to isolate and debug issues in the mining notification and polling flow.

## Problem Statement

Miners were experiencing issues where:
1. Push notifications (`PRIME_BLOCK_AVAILABLE` / `HASH_BLOCK_AVAILABLE`) were sent but miners weren't responding
2. Miners fell back to legacy `GET_ROUND` polling instead of using push notifications
3. The `GET_ROUND` → `NEW_ROUND` / `OLD_ROUND` flow wasn't proceeding as expected

## Solution: Enhanced Diagnostics

### A) Alias Opcodes Added

To improve code clarity and logging, two alias opcodes were added:

| Alias Name | Maps To | Opcode Value | Description |
|-----------|---------|--------------|-------------|
| `NEW_PRIME_AVAILABLE` | `PRIME_BLOCK_AVAILABLE` | 217 (0xD9) | Prime channel work available |
| `NEW_HASH_AVAILABLE` | `HASH_BLOCK_AVAILABLE` | 218 (0xDA) | Hash channel work available |

**Key Points:**
- Aliases map to the **same opcode values** as the original opcodes
- Both names appear in logs to aid debugging
- No protocol changes - fully backward compatible
- Helps clarify "new work available" semantics

### B) Enhanced Logging Points

#### 1. Push Notification Sent

When the server sends a push notification:

```
════════════════════════════════════════════════════════════
📢 SENDING PUSH NOTIFICATION TO MINER
════════════════════════════════════════════════════════════
   Opcode:         PRIME_BLOCK_AVAILABLE (NEW_PRIME_AVAILABLE)
   Opcode Value:   0xd9 (217)
   To Address:     192.168.1.100:9325
   Channel:        1 (Prime)
   Payload:
      Unified Height:  6535196
      Channel Height:  2165442
      Difficulty:      0x12345678
   Packet Size:    12 bytes

   ⚠️  EXPECTED CLIENT ACTION:
      Client should respond with GET_BLOCK (129/0x81)
      to fetch new mining template for this channel.

   🔄 FALLBACK BEHAVIOR:
      If client times out or doesn't receive this,
      client may fall back to polling GET_ROUND (133/0x85).
════════════════════════════════════════════════════════════
```

**Location:** `src/LLP/stateless_miner_connection.cpp:SendChannelNotification()`

#### 2. GET_BLOCK Request Received

When the server receives a GET_BLOCK request:

```
════════════════════════════════════════════════════════════
📥 RECEIVED GET_BLOCK REQUEST
════════════════════════════════════════════════════════════
   From:           192.168.1.100:9325
   Opcode:         GET_BLOCK (129/0x81)
   Context:
      Authenticated:  YES
      Channel:        1 (Prime)
      Subscribed:     YES

   ✅ NOTIFICATION FLOW DETECTED:
      Time since last notification: 0.234 seconds
      This appears to be a response to:
         PRIME_BLOCK_AVAILABLE
         (NEW_PRIME_AVAILABLE)
════════════════════════════════════════════════════════════
```

OR if not following a notification:

```
   ℹ️  POLLING MODE:
      This request is NOT following a notification.
      Client may be using legacy polling flow.
```

**Location:** `src/LLP/stateless_miner_connection.cpp:CheckRateLimit()` before GET_BLOCK processing

#### 3. GET_ROUND Request Received (Potential Issue)

When a subscribed miner sends GET_ROUND:

```
════════════════════════════════════════════════════════════
📥 RECEIVED GET_ROUND REQUEST
════════════════════════════════════════════════════════════
   From:           192.168.1.100:9325
   Opcode:         GET_ROUND (133/0x85)
   Context:
      Authenticated:  YES
      Channel:        1 (Prime)
      Subscribed:     YES

   ⚠️  POTENTIAL ISSUE DETECTED:
      Miner is subscribed to push notifications but
      is polling with GET_ROUND. This suggests:
      1. Client didn't receive push notification, OR
      2. Client timed out waiting for notification, OR
      3. Client is using hybrid polling+push strategy

   Expected flow should be:
      Server → PRIME_BLOCK_AVAILABLE
      Client → GET_BLOCK

   Fallback behavior (what's happening now):
      Client → GET_ROUND (polling)
      Server → NEW_ROUND (with heights)
════════════════════════════════════════════════════════════
```

**Location:** `src/LLP/stateless_miner_connection.cpp:CheckRateLimit()` before GET_ROUND processing

#### 4. NEW_ROUND Response Sent

When the server sends NEW_ROUND response:

```
════════════════════════════════════════════════════════════
📤 SENDING NEW_ROUND RESPONSE
════════════════════════════════════════════════════════════
   To:             192.168.1.100:9325
   Opcode:         NEW_ROUND (204/0xCC)
   Response Data:
      Unified Height:  6535196
      Channel Height:  2165442 (channel 1)
      Difficulty:      0x12345678
   Packet Size:    12 bytes

   ⚠️  NOTE:
      This is the legacy polling flow response.
      Preferred flow is push notifications:
         MINER_READY → PRIME_BLOCK_AVAILABLE
         Then client issues GET_BLOCK automatically.
════════════════════════════════════════════════════════════
```

**Location:** `src/LLP/stateless_miner_connection.cpp:ProcessPacket()` after GET_ROUND processing

## How to Use These Diagnostics

### Scenario 1: Normal Push Notification Flow ✅

Expected log sequence:
1. `📥 MINER_READY REQUEST` - Miner subscribes
2. `📢 SENDING PUSH NOTIFICATION` - Server sends notification
3. `📥 RECEIVED GET_BLOCK REQUEST` with `✅ NOTIFICATION FLOW DETECTED` - Miner responds correctly

### Scenario 2: Notification Not Received ⚠️

Problem log sequence:
1. `📥 MINER_READY REQUEST` - Miner subscribes
2. `📢 SENDING PUSH NOTIFICATION` - Server sends notification
3. `📥 RECEIVED GET_ROUND REQUEST` with `⚠️ POTENTIAL ISSUE DETECTED` - Miner falls back to polling

**Possible causes:**
- Network packet loss
- Firewall blocking push notifications
- Client-side timeout too short
- Client not properly handling notification opcode

### Scenario 3: Legacy Polling Flow ℹ️

Expected log sequence for non-subscribed miners:
1. `📥 RECEIVED GET_ROUND REQUEST` with `ℹ️ LEGACY POLLING MODE`
2. `📤 SENDING NEW_ROUND RESPONSE`

This is normal for miners not using push notifications.

## Protocol Flows

### Push Notification Flow (Preferred)

```
Miner                           Node
  |                              |
  |--- MINER_READY (216) ------->|
  |                              |
  |<-- PRIME_BLOCK_AVAILABLE ----|  (217 / NEW_PRIME_AVAILABLE)
  |    [12 bytes: heights+diff]  |
  |                              |
  |--- GET_BLOCK (129) --------->|
  |                              |
  |<-- BLOCK_DATA (0) -----------|
  |    [216+ bytes: template]    |
```

### Legacy Polling Flow (Fallback)

```
Miner                           Node
  |                              |
  |--- GET_ROUND (133) --------->|
  |                              |
  |<-- NEW_ROUND (204) ----------|
  |    [12 bytes: heights+diff]  |
  |                              |
  |--- GET_BLOCK (129) --------->|  (if height changed)
  |                              |
  |<-- BLOCK_DATA (0) -----------|
  |    [216+ bytes: template]    |
```

## Opcode Reference

| Opcode | Value | Direction | Description |
|--------|-------|-----------|-------------|
| `MINER_READY` | 216 (0xD8) | Miner → Node | Subscribe to push notifications |
| `PRIME_BLOCK_AVAILABLE` | 217 (0xD9) | Node → Miner | Prime work available (push) |
| `NEW_PRIME_AVAILABLE` | 217 (0xD9) | Node → Miner | Alias for above |
| `HASH_BLOCK_AVAILABLE` | 218 (0xDA) | Node → Miner | Hash work available (push) |
| `NEW_HASH_AVAILABLE` | 218 (0xDA) | Node → Miner | Alias for above |
| `GET_BLOCK` | 129 (0x81) | Miner → Node | Request mining template |
| `GET_ROUND` | 133 (0x85) | Miner → Node | Poll for height info |
| `NEW_ROUND` | 204 (0xCC) | Node → Miner | Response with heights |
| `OLD_ROUND` | 205 (0xCD) | Node → Miner | Stale/rejected request |

## Debugging Tips

1. **Enable verbose logging:** Set debug level to 2 or lower to see diagnostic messages
   ```bash
   # Level 0: All messages (most verbose)
   ./nexus -verbose=0
   
   # Level 2: Diagnostic messages (recommended for troubleshooting)
   ./nexus -verbose=2
   ```

2. **Search logs for flow issues:**
   ```bash
   # Check if notifications are being sent
   grep "SENDING PUSH NOTIFICATION" debug.log
   
   # Check if clients are falling back to polling
   grep "POTENTIAL ISSUE DETECTED" debug.log
   
   # Check notification→request timing
   grep "NOTIFICATION FLOW DETECTED" debug.log
   ```

3. **Monitor notification timing:**
   Look for the "Time since last notification" value in GET_BLOCK logs. 
   - < 1 second: Good (immediate response)
   - 1-5 seconds: Acceptable
   - > 5 seconds: May indicate timeout/retry

4. **Verify packet sizes:**
   - Push notifications: 12 bytes (unified height, channel height, difficulty)
   - NEW_ROUND response: 12 bytes (stateless) or 16 bytes (legacy)

## Files Modified

- `src/LLP/types/miner.h` - Added alias opcode definitions
- `src/LLP/opcode_utility.cpp` - Added alias constants and name mapping
- `src/LLP/stateless_miner_connection.cpp` - Added comprehensive diagnostics
- `src/LLP/miner.cpp` - Added diagnostics for legacy miner class
- `tests/unit/LLP/opcode_alias.cpp` - Unit tests for alias opcodes

## Client-Side Implementation Notes

**This change is server-side only.** The miner client (NexusMiner) needs to:

1. After receiving `PRIME_BLOCK_AVAILABLE` or `HASH_BLOCK_AVAILABLE`:
   - Immediately send `GET_BLOCK (129)` to fetch new template
   - Don't wait for polling interval

2. If notification timeout occurs (e.g., 5 seconds):
   - Fall back to `GET_ROUND` polling
   - Continue polling until notification received

3. Maintain both flows in parallel:
   - Primary: Wait for push notifications
   - Fallback: Poll GET_ROUND every 5-10 seconds

## Future Improvements

1. Add metrics tracking notification→response timing
2. Add alert when subscribed miners consistently fall back to polling
3. Add network diagnostics to detect packet loss
4. Consider implementing ACK mechanism for notifications
