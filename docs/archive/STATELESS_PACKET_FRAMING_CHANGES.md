# Stateless Mining Packet Framing Changes

## Overview

This document describes the changes made to fix stateless mining packet framing and support NexusMiner stateless opcodes without breaking legacy LLP.

## Problem Statement

The original implementation had a hack in `LLP::Connection::ReadPacket()` that treated any inbound leading byte `0xD0` as a 16-bit opcode prefix. This approach was:
- Incompatible with NexusMiner's expected stateless framing (uint16 header + uint32 length + payload)
- Fragile and race-prone
- Mixed legacy 8-bit and stateless 16-bit framing in the same connection type

## Solution

### 1. Removed 0xD0 Hack from Legacy Connection

**Files Changed:**
- `src/LLP/connection.cpp`
- `src/LLP/packets/packet.h`

**Changes:**
- Removed the 0xD0 prefix detection logic from `Connection::ReadPacket()`
- Restored strict legacy LLP framing (8-bit HEADER + 4-byte LENGTH + DATA)
- Removed 16-bit opcode support from the `Packet` class
- `Packet` is now purely for 8-bit legacy LLP framing

### 2. Created Dedicated Stateless Packet Type

**New File:** `src/LLP/packets/stateless_packet.h`

**Structure:**
```cpp
class StatelessPacket
{
    uint16_t HEADER;                // 16-bit opcode (big-endian on wire)
    uint32_t LENGTH;                // 32-bit length (big-endian on wire)
    std::vector<uint8_t> DATA;      // Payload
};
```

**Wire Format:**
```
[HEADER (2 bytes, big-endian)] [LENGTH (4 bytes, big-endian)] [DATA (LENGTH bytes)]
```

**Key Features:**
- Always uses 16-bit framing
- Supports both legacy 8-bit opcodes (zero-extended to 16-bit) and new 16-bit stateless opcodes
- Compatible with NexusMiner's expected framing

### 3. Created Dedicated Stateless Connection Type

**New Files:**
- `src/LLP/templates/stateless_connection.h`
- `src/LLP/stateless_connection.cpp`

**Structure:**
```cpp
class StatelessConnection : public BaseConnection<StatelessPacket>
{
    void ReadPacket() final;  // Reads 16-bit framed packets
};
```

**ReadPacket Implementation:**
1. Reads 2 bytes for HEADER (big-endian)
2. Reads 4 bytes for LENGTH (big-endian)
3. Reads LENGTH bytes for DATA
4. Fires PACKET event when complete

### 4. Updated StatelessMinerConnection

**Files Changed:**
- `src/LLP/types/stateless_miner_connection.h`
- `src/LLP/stateless_miner_connection.cpp`

**Changes:**
- Changed inheritance from `Connection` to `StatelessConnection`
- Updated all packet handling to use `StatelessPacket` instead of `Packet`
- Updated `respond()` method signature to accept `StatelessPacket`
- All packets now use 16-bit framing

### 5. Opcode Compatibility Strategy

The implementation uses a unified 16-bit framing approach where:

- **Legacy 8-bit opcodes** (0x00-0xFF) are zero-extended to 16-bit:
  - `GET_BLOCK` (129 / 0x81) → 0x0081
  - `SUBMIT_BLOCK` (1 / 0x01) → 0x0001
  - `MINER_AUTH_INIT` (207 / 0xCF) → 0x00CF
  - etc.

- **New stateless 16-bit opcodes** use the full 16-bit space:
  - `STATELESS_MINER_READY` = 0xD007
  - `STATELESS_GET_BLOCK` = 0xD008

This approach maintains backward compatibility while enabling proper stateless framing. The `StatelessPacket::HEADER` field is a `uint16_t`, so comparisons like `PACKET.HEADER == GET_BLOCK` work correctly because the 8-bit value is automatically zero-extended to 16-bit.

## Benefits

1. **Clean Separation:** Legacy LLP (8-bit) and Stateless (16-bit) framing are now completely separate
2. **No More Hacks:** Removed the fragile 0xD0 prefix detection hack
3. **NexusMiner Compatible:** Proper 16-bit framing with length field as expected by NexusMiner
4. **Backward Compatible:** Legacy opcodes continue to work via zero-extension
5. **Type Safety:** Compile-time enforcement of packet types via templates

## Impact on Other Code

### Unchanged Components
- `LLP::Connection` - Still handles legacy 8-bit LLP framing for other LLP connections
- `LLP::Packet` - Still used by legacy LLP connections
- Legacy miner protocol - Unaffected, continues using `Miner` class and `Connection`

### Changed Components
- `LLP::StatelessMinerConnection` - Now uses 16-bit framing for ALL packets
- Stateless mining opcodes - Now properly framed with 16-bit header + 32-bit length

## Port Configuration

The stateless miner server continues to use port 8323 by default. The code is structured to support dual listener mapping in the future if needed.

## Testing Recommendations

1. **Unit Tests:**
   - Test `StatelessPacket` serialization/deserialization
   - Test `StatelessConnection::ReadPacket()` with various packet sizes
   - Test opcode comparison (8-bit vs 16-bit)

2. **Integration Tests:**
   - Verify legacy LLP connections still work correctly
   - Verify stateless mining connections work with NexusMiner
   - Test mixed opcode scenarios (legacy and stateless on same connection)

3. **Runtime Validation:**
   - Add logging to confirm packet framing
   - Verify wire format matches expected (wireshark/tcpdump)

## Migration Notes

For developers working with stateless mining:

1. **Creating Packets:**
   ```cpp
   // Legacy (8-bit)
   Packet packet(OPCODE);
   
   // Stateless (16-bit) - handles both legacy and stateless opcodes
   StatelessPacket packet(OPCODE);  // OPCODE can be 8-bit or 16-bit
   ```

2. **Reading Packets:**
   ```cpp
   // In StatelessMinerConnection::ProcessPacket()
   StatelessPacket PACKET = this->INCOMING;
   
   // Check opcode (works for both legacy and stateless)
   if(PACKET.HEADER == GET_BLOCK)  // Legacy 8-bit opcode (zero-extended)
   if(PACKET.GetOpcode() == STATELESS_MINER_READY)  // 16-bit stateless opcode
   ```

3. **Sending Packets:**
   ```cpp
   StatelessPacket response(OPCODE);
   response.DATA = data;
   response.LENGTH = data.size();
   respond(response);
   ```

## Files Modified

### New Files:
- `src/LLP/packets/stateless_packet.h`
- `src/LLP/templates/stateless_connection.h`
- `src/LLP/stateless_connection.cpp`

### Modified Files:
- `src/LLP/connection.cpp`
- `src/LLP/packets/packet.h`
- `src/LLP/types/stateless_miner_connection.h`
- `src/LLP/stateless_miner_connection.cpp`

## Opcode Reference

### Legacy 8-bit Opcodes (0x00-0xFF → 0x0000-0x00FF)
- GET_BLOCK = 129 (0x0081)
- SUBMIT_BLOCK = 1 (0x0001)
- GET_HEIGHT = 130 (0x0082)
- GET_REWARD = 131 (0x0083)
- GET_ROUND = 133 (0x0085)
- SET_CHANNEL = 3 (0x0003)
- MINER_AUTH_INIT = 207 (0x00CF)
- MINER_AUTH_CHALLENGE = 208 (0x00D0)
- MINER_AUTH_RESPONSE = 209 (0x00D1)
- MINER_AUTH_RESULT = 210 (0x00D2)

### Stateless 16-bit Opcodes (0xD000-0xDFFF)
- STATELESS_MINER_READY = 0xD007
- STATELESS_GET_BLOCK = 0xD008

## Security Considerations

- The 0xD0 hack was removed, eliminating a potential source of protocol confusion
- All packets now have explicit length fields, reducing buffer overflow risks
- Type safety is enforced at compile time via templates

## Future Work

- Consider adding dedicated stateless mining port configuration
- Add unit tests for StatelessPacket and StatelessConnection
- Add integration tests for mixed protocol scenarios
- Consider adding packet validation in StatelessConnection::ReadPacket()
