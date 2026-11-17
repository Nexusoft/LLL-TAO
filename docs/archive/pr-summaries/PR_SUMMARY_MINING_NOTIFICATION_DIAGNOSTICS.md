# PR Summary: Mining Notification Flow Fix

## Overview
This PR adds comprehensive diagnostics and alias opcodes to isolate and debug issues in the mining notification and polling flow between miners and nodes.

## Problem Statement
Miners were experiencing issues where:
1. Push notifications (`PRIME_BLOCK_AVAILABLE`/`HASH_BLOCK_AVAILABLE`) were being sent but miners weren't responding with follow-up requests
2. Miners fell back to legacy `GET_ROUND` polling instead of using the push notification system
3. It was difficult to diagnose where the notification→request flow was breaking

## Solution
This PR implements enhanced diagnostics that make it easy to identify and debug flow issues:

### 1. Alias Opcodes (Backward Compatible)
- `NEW_PRIME_AVAILABLE` → alias for `PRIME_BLOCK_AVAILABLE (217/0xD9)`
- `NEW_HASH_AVAILABLE` → alias for `HASH_BLOCK_AVAILABLE (218/0xDA)`

**Benefits:**
- Improves code clarity and self-documentation
- Both names appear in logs for better readability
- No protocol changes - fully backward compatible
- Helps clarify "new work available" semantics

### 2. Enhanced Diagnostic Logging
Added detailed logging at 4 key points in the mining flow:

#### A. Push Notification Sent
```
════════════════════════════════════════════════════════════
📢 SENDING PUSH NOTIFICATION TO MINER
════════════════════════════════════════════════════════════
   Opcode:         HASH_BLOCK_AVAILABLE (NEW_HASH_AVAILABLE)
   To Address:     192.168.1.100:9325
   Channel:        2 (Hash)
   Payload:
      Unified Height:  6535196
      Channel Height:  4165001
      Difficulty:      0x12345678
   
   ⚠️  EXPECTED CLIENT ACTION:
      Client should respond with GET_BLOCK (129/0x81)
```

#### B. GET_BLOCK Received
```
════════════════════════════════════════════════════════════
📥 RECEIVED GET_BLOCK REQUEST
════════════════════════════════════════════════════════════
   From:           192.168.1.100:9325
   Context:
      Channel:        2 (Hash)
      Subscribed:     YES
   
   ✅ NOTIFICATION FLOW DETECTED:
      Time since last notification: 0.234 seconds
```

#### C. GET_ROUND Received (Issue Detection)
```
════════════════════════════════════════════════════════════
📥 RECEIVED GET_ROUND REQUEST
════════════════════════════════════════════════════════════
   From:           192.168.1.100:9325
   Context:
      Subscribed:     YES
   
   ⚠️  POTENTIAL ISSUE DETECTED:
      Miner is subscribed to push notifications but
      is polling with GET_ROUND. This suggests:
      1. Client didn't receive push notification, OR
      2. Client timed out waiting for notification
```

#### D. NEW_ROUND Response Sent
```
════════════════════════════════════════════════════════════
📤 SENDING NEW_ROUND RESPONSE
════════════════════════════════════════════════════════════
   To:             192.168.1.100:9325
   Response Data:
      Unified Height:  6535196
      Channel Height:  4165001
      Difficulty:      0x12345678
```

### 3. Automatic Issue Detection
The diagnostics automatically identify problematic scenarios:
- **Notification Flow**: Shows timing between notification and client response
- **Fallback Detection**: Identifies when subscribed miners revert to polling
- **Flow Type**: Distinguishes legacy polling vs push notification flows

### 4. Comprehensive Documentation
- **User Guide**: `docs/MINING_NOTIFICATION_DIAGNOSTICS.md`
  - Protocol flow diagrams
  - Troubleshooting scenarios with expected log output
  - Debugging tips and log search patterns
  - Opcode reference table
  
- **Unit Tests**: `tests/unit/LLP/opcode_alias.cpp`
  - Verifies alias opcode mappings
  - Tests protocol flow constants
  - Ensures backward compatibility

## Technical Details

### Files Modified
1. `src/LLP/types/miner.h` - Added alias opcode definitions and documentation
2. `src/LLP/opcode_utility.cpp` - Added alias constants and name mapping
3. `src/LLP/stateless_miner_connection.cpp` - Enhanced diagnostics for stateless miners
4. `src/LLP/miner.cpp` - Enhanced diagnostics for legacy miner class
5. `tests/unit/LLP/opcode_alias.cpp` - Unit tests
6. `docs/MINING_NOTIFICATION_DIAGNOSTICS.md` - Comprehensive guide

### Code Quality Improvements
- **Robustness**: Fixed potential integer underflow in time calculations
- **Performance**: Optimized log levels (level 2) to prevent log file bloat
- **Safety**: Added guards against clock adjustments and race conditions
- **Testing**: Unit tests verify all alias mappings

### Protocol Compatibility
✅ No protocol changes  
✅ Aliases map to existing opcodes  
✅ Backward compatible with all clients  
✅ Both old and new opcode names appear in logs  

### Performance Impact
- Diagnostic logs use level 2 (not level 0) to minimize production overhead
- Logs only appear when debug level is set to 2 or lower
- No performance impact on packet processing or notification sending

## Usage

### Enable Diagnostic Logging
```bash
# Level 2 (recommended for troubleshooting)
./nexus -verbose=2

# Level 0 (all messages, very verbose)
./nexus -verbose=0
```

### Search Logs for Issues
```bash
# Check if notifications are being sent
grep "SENDING PUSH NOTIFICATION" debug.log

# Check if clients are falling back to polling
grep "POTENTIAL ISSUE DETECTED" debug.log

# Check notification→request timing
grep "NOTIFICATION FLOW DETECTED" debug.log
```

### Interpret Log Output
- **Normal Flow**: `SENDING PUSH NOTIFICATION` → `NOTIFICATION FLOW DETECTED` in GET_BLOCK
- **Issue**: `SENDING PUSH NOTIFICATION` → `POTENTIAL ISSUE DETECTED` in GET_ROUND
- **Legacy**: No subscription, direct GET_ROUND → `LEGACY POLLING MODE`

## Testing

### Unit Tests
```bash
# Run opcode alias tests
./tests/unit/LLP/opcode_alias.cpp
```

### Manual Testing
1. Start node with verbose logging: `./nexus -verbose=2`
2. Connect miner that subscribes to notifications (sends `MINER_READY`)
3. Observe logs for push notifications being sent
4. Verify miner responds with `GET_BLOCK` (check for `NOTIFICATION FLOW DETECTED`)
5. If miner falls back to polling, check for `POTENTIAL ISSUE DETECTED`

## Expected Impact

### For Node Operators
- Easier to diagnose mining connection issues
- Clear identification of client-side vs server-side problems
- Detailed logs help guide miner configuration

### For Miner Developers
- Clear documentation of expected client behavior
- Logs show exact timing and flow expectations
- Helps debug why notifications aren't being received

### For Protocol Development
- Provides foundation for future notification system improvements
- Documents current flow for reference
- Makes protocol behavior transparent

## Next Steps (Client Side)
Server-side work is complete. The miner client (NexusMiner repo) should:

1. **Implement automatic GET_BLOCK** after receiving notifications:
   ```
   On PRIME_BLOCK_AVAILABLE or HASH_BLOCK_AVAILABLE:
     → Immediately send GET_BLOCK (129)
     → Don't wait for polling interval
   ```

2. **Maintain fallback behavior**:
   ```
   If notification timeout (e.g., 5 seconds):
     → Fall back to GET_ROUND polling
     → Continue polling until notification received
   ```

3. **Use server logs for debugging**:
   - Check if notifications are being sent
   - Verify timing between notification and client response
   - Identify network connectivity issues

## Acceptance Criteria
- [x] Alias opcodes defined and documented
- [x] Opcode names appear in logs
- [x] Comprehensive logging at all key flow points
- [x] Automatic detection of flow issues
- [x] Documentation with troubleshooting guide
- [x] Unit tests for opcode mappings
- [x] Code review feedback addressed
- [x] Production-ready log levels
- [x] Backward compatible with existing clients

## References
- Issue: Mining notification flow not proceeding as expected
- Base Branch: `STATELESS-NODE`
- Related: NexusMiner client implementation (separate repo)
