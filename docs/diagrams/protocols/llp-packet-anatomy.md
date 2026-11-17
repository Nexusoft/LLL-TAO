# LLP Packet Anatomy

Wire format visualization for the Lower Level Protocol (LLP) packet structure.

---

## Legacy Tritium Protocol (8-bit, Port 8323)

```mermaid
flowchart LR
    subgraph "Legacy Packet Structure"
        H1[HEADER<br/>1 byte<br/>uint8 opcode]
        L1[LENGTH<br/>4 bytes BE<br/>uint32 payload size]
        P1[PAYLOAD<br/>Variable length<br/>Raw data]
    end
    H1 --> L1 --> P1
```

```
┌──────────┬──────────────┬─────────────────────┐
│ HEADER   │ LENGTH       │ PAYLOAD             │
│ 1 byte   │ 4 bytes BE   │ Variable            │
│ 0x00-0xFF│ uint32       │ Raw bytes           │
├──────────┼──────────────┼─────────────────────┤
│ Example: │              │                     │
│ 0xD8     │ 0x00000000   │ (empty)             │
│ MINER_   │ Length = 0   │ Ready signal only   │
│ READY    │              │                     │
└──────────┴──────────────┴─────────────────────┘
```

---

## Stateless Tritium Protocol (16-bit, Port 9323)

```mermaid
flowchart LR
    subgraph "Stateless Packet Structure"
        H2[HEADER<br/>2 bytes BE<br/>uint16 opcode]
        L2[LENGTH<br/>4 bytes BE<br/>uint32 payload size]
        P2[PAYLOAD<br/>Variable length<br/>Raw data]
    end
    H2 --> L2 --> P2
```

```
┌───────────┬──────────────┬─────────────────────┐
│ HEADER    │ LENGTH       │ PAYLOAD             │
│ 2 bytes BE│ 4 bytes BE   │ Variable            │
│ 0xD000-   │ uint32       │ Raw bytes           │
│ 0xD0FF    │              │                     │
├───────────┼──────────────┼─────────────────────┤
│ Example:  │              │                     │
│ 0xD0D8    │ 0x00000000   │ (empty)             │
│ STATELESS_│ Length = 0   │ Ready signal only   │
│ MINER_    │              │                     │
│ READY     │              │                     │
└───────────┴──────────────┴─────────────────────┘
```

---

## Opcode Mirror Mapping

```mermaid
flowchart LR
    subgraph "Legacy 8-bit"
        A1["SUBMIT_BLOCK<br/>0x01"]
        A2["GET_BLOCK<br/>0x81"]
        A3["BLOCK_ACCEPTED<br/>0xC8"]
        A4["MINER_READY<br/>0xD8"]
        A5["PRIME_BLOCK_AVAILABLE<br/>0xD9"]
        A6["HASH_BLOCK_AVAILABLE<br/>0xDA"]
    end

    subgraph "Stateless 16-bit"
        B1["0xD001"]
        B2["0xD081"]
        B3["0xD0C8"]
        B4["0xD0D8"]
        B5["0xD0D9"]
        B6["0xD0DA"]
    end

    A1 -->|"0xD000 OR"| B1
    A2 -->|"0xD000 OR"| B2
    A3 -->|"0xD000 OR"| B3
    A4 -->|"0xD000 OR"| B4
    A5 -->|"0xD000 OR"| B5
    A6 -->|"0xD000 OR"| B6
```

**Formula:** `stateless_opcode = 0xD000 | legacy_opcode`

---

## Block Data Payload Sizes

| Packet Type | Legacy Size | Stateless Size | Notes |
|---|---|---|---|
| Push Notification | 12 bytes | 12 bytes | Height + Channel + Bits |
| Block Template | 216 bytes | 228 bytes | 12 meta + 216 block |
| Submit Block | Variable | Variable | Includes solution |

---

## Cross-References

- [Push Notification Flow](../push-notification-flow.md)
- [Mining Flow](../architecture/mining-flow-complete.md)
- [Opcodes Reference](../../reference/opcodes-reference.md)
- Source: `src/LLP/include/opcode_utility.h`
