# Endianness & Serialisation

**Section:** Node Architecture → RISC-V  
**Version:** LLL-TAO 5.1.0+  
**Last Updated:** 2026-03-10

---

## Overview

Nexus LLL-TAO uses **explicit little-endian serialisation** throughout all protocol packets and on-disk data.  This means RISC-V (natively little-endian) requires no byte-swapping and no architecture-specific code paths in the application layer.

---

## Protocol Byte Order Rules

| Data Type | Byte Order | Serialisation Helper |
|---|---|---|
| `uint16_t` opcode | Little-endian | `WriteLE16` / `ReadLE16` |
| `uint32_t` session ID | Little-endian | `WriteLE32` / `ReadLE32` |
| `uint32_t` nBits | Little-endian | `WriteLE32` / `ReadLE32` |
| `uint64_t` nNonce | Little-endian | `WriteLE64` / `ReadLE64` |
| `uint256_t` genesis hash | Little-endian byte array | Raw 32-byte copy |
| ChaCha20 key (32 bytes) | Raw byte array (no integer interpretation) | Raw copy |
| Reward hash (32 bytes) | Raw byte array | Raw copy |

> **Rule:** Never use `htonl` / `ntohl` / `ntohs` / `htons` in mining protocol code.  These functions produce big-endian output and will produce incorrect results on any little-endian architecture when the intent is to serialise a fixed-endian value.

---

## Mining Template Field Layout (216 bytes, Tritium)

```
  Offset   Size   Field              Byte order
  ──────   ────   ─────              ──────────
    0       4     nVersion           LE uint32
    4      32     hashPrevBlock      LE byte[32]
   36      32     hashMerkleRoot     LE byte[32]
   68       4     nChannel           LE uint32   ← at byte 196 from start
  196       4     nChannel           (see note)
  200       4     nHeight            LE uint32
  204       4     nBits              LE uint32
  208       8     nNonce             LE uint64
  216       —     (end of template)
```

> **Note:** The exact field offsets are defined in `src/TAO/Ledger/block.cpp`.  The values `nChannel@196`, `nHeight@200`, `nBits@204`, `nNonce@208` are the post-genesis-hash offsets in the serialised Tritium block header.  Cross-architecture tests should verify these offsets numerically (see [Test Strategy TC-5.2](../test-strategy.md#tc-52--block-template-field-layout)).

---

## Genesis Hash Endianness

`hashGenesis` is stored and transmitted as a raw 32-byte little-endian byte array.  No reversal is performed at the protocol boundary.

**Important:** Log display may reverse bytes for human readability (most block explorers display big-endian hex).  Ensure that internal comparisons always operate on the canonical little-endian form.

---

## Packet Framing (Stateless Lane, Port 9323)

```
  ┌──────────────────────────────────────────────┐
  │  Stateless Packet Frame (variable length)    │
  │                                              │
  │  [0..1]  uint16_t opcode    (LE)             │
  │  [2..5]  uint32_t length    (LE)             │
  │  [6..]   payload bytes      (opcode-defined) │
  └──────────────────────────────────────────────┘
```

**Legacy lane (port 8323)** uses an 8-bit opcode with a different framing; see `docs/archive/STATELESS_PACKET_FRAMING_CHANGES.md` for the historical context.

---

## ChaCha20 Nonce Handling

ChaCha20-Poly1305 nonces are 12-byte byte arrays.  The node constructs nonces as follows:

```
  nonce[0..3]  = session_id     (LE uint32)
  nonce[4..7]  = sequence_num   (LE uint32, incremented per packet)
  nonce[8..11] = purpose_tag    (LE uint32, opcode-derived constant)
```

All three sub-fields are serialised little-endian and are therefore byte-identical on x86 and RISC-V.

---

## Serialisation Invariant Test (TC-5.1)

The cross-architecture serialisation test (see [Test Strategy TC-5.1](../test-strategy.md#tc-51--session-packet-byte-order-x86-vs-risc-v)) snapshots a `MinerSessionContainer` to bytes on x86 and parses it on RISC-V.  The expected byte pattern for a representative container is:

```
  Bytes 0..3:   nSessionID    (LE uint32)
  Bytes 4..35:  hashGenesis   (32-byte LE array)
  Bytes 36..67: vChacha20Key  (32-byte raw)
  Bytes 68..99: vRewardHash   (32-byte raw)
  Byte  100:    fRewardBound  (uint8, 0 or 1)
  Byte  101:    fAuthenticated(uint8, 0 or 1)
  Byte  102:    eLane         (uint8, 0=STATELESS, 1=LEGACY)
```

The test asserts byte-level equality of the serialised form produced on each architecture.

---

## Related Pages

- [Atomic Operations & Locking](atomic-locking.md)
- [Diagnostics & Testing Notes](diagnostics.md)
- [RISC-V Overview](index.md)
- [Test Strategy — Cross-Arch Tests](../test-strategy.md#5-cross-architecture--serialisation-stability)
