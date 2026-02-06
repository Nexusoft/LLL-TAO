# Mining Protocol Lanes

This document defines the two protocol lanes used for mining communication between Nexus nodes and miners.

---

## Protocol Lanes

### Legacy Tritium Protocol (8-bit)
- **Port:** 9325 (mainnet), 8323 (testnet)
- **Opcodes:** 0x00-0xFF (single byte)
- **Wire format:** `[opcode(1)][length(4)][payload]`
- **Push notifications:**
  - `PRIME_BLOCK_AVAILABLE` (0xD9)
  - `HASH_BLOCK_AVAILABLE` (0xDA)
- **Subscribe:** `MINER_READY` (0xD8)
- **Connection class:** `LLP::Miner`

### Stateless Tritium Protocol (16-bit)
- **Port:** 9323 (mainnet and testnet)
- **Opcodes:** 0xD000-0xD0FF (mirror-mapped)
- **Wire format:** `[opcode(2, big-endian)][length(4, big-endian)][payload]`
- **Push notifications:**
  - `STATELESS_PRIME_BLOCK_AVAILABLE` (0xD0D9)
  - `STATELESS_HASH_BLOCK_AVAILABLE` (0xD0DA)
- **Subscribe:** `STATELESS_MINER_READY` (0xD0D8)
- **Formula:** `stateless_opcode = 0xD000 | legacy_opcode`
- **Connection class:** `LLP::StatelessMinerConnection`

---

## Unified Push Notification Payload

Both protocol lanes use an identical 12-byte payload for push notifications:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0-3 | 4 bytes | `nUnifiedHeight` | Current blockchain height (all channels), big-endian |
| 4-7 | 4 bytes | `nChannelHeight` | Channel-specific height, big-endian |
| 8-11 | 4 bytes | `nBits` | Current difficulty for the channel, big-endian |

The unified builder `PushNotificationBuilder::BuildChannelNotification<T>()` constructs this payload
for both lanes using template specialization:
- `BuildChannelNotification<Packet>` → Legacy Tritium Protocol (8-bit opcodes)
- `BuildChannelNotification<StatelessPacket>` → Stateless Tritium Protocol (16-bit opcodes)

---

## Cross-References

- [Opcodes Reference](../reference/opcodes-reference.md) — Full opcode table
- [Mining Lanes Cheat Sheet](../current/mining/mining-lanes-cheat-sheet.md) — Quick navigation guide
- [Stateless Protocol (Node)](../current/mining/stateless-protocol.md) — Node implementation details
- [Mining Server Architecture](../current/mining/mining-server.md) — Server components
- [Push Notification Flow Diagram](../diagrams/push-notification-flow.md) — Sequence diagram
- Source: `src/LLP/types/miner.h`, `src/LLP/include/push_notification.h`
