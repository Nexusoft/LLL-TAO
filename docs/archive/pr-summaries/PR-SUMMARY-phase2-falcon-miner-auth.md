# Phase 2: Falcon-Driven Miner Authentication Layer

## Overview

This implementation completes Phase 2 of the unified mining stack by adding a fully stateless, Falcon-driven miner authentication layer inside LLL-TAO. The work builds on Phase 1 to provide identity-aware mining contexts tied to Tritium accounts.

**Key Changes in This Update:**
- Aligned authentication protocol with NexusMiner's expected MINER_AUTH flow
- Implemented proper challenge-response authentication (INIT → CHALLENGE → RESPONSE → RESULT)
- Added support for Unified Hybrid Falcon Signature Protocol in block submissions
- Updated default testnet mining port to 8323

## Architecture

### Core Components

#### 1. MiningContext (Immutable State)
Location: `src/LLP/include/stateless_miner.h`

Immutable snapshot of a miner's state including:
- `nChannel`: Mining channel (1=Prime, 2=Hash)
- `nHeight`: Current blockchain height
- `nTimestamp`: Last activity timestamp
- `strAddress`: Miner's network address
- `nProtocolVersion`: Protocol version
- `fAuthenticated`: Falcon authentication status
- `nSessionId`: Unique session identifier
- **Phase 2**: `hashKeyID`: Falcon key identifier
- **Phase 2**: `hashGenesis`: Tritium genesis hash
- **NEW**: `vAuthNonce`: Challenge nonce for authentication
- **NEW**: `vMinerPubKey`: Miner's Falcon public key

All state updates return new contexts via `WithX()` methods.

#### 2. StatelessMiner (Pure Functional Processor)
Location: `src/LLP/stateless_miner.cpp`

Pure functions for packet processing:
- `ProcessPacket()`: Main routing logic
- **NEW** `ProcessMinerAuthInit()`: Handles MINER_AUTH_INIT (207), generates challenge nonce
- `ProcessFalconResponse()`: Handles MINER_AUTH_RESPONSE (209), verifies signature over nonce
- `ProcessSessionStart()`: **Phase 2** Requires prior authentication
- `ProcessSetChannel()`: Channel selection with legacy compatibility

#### 3. StatelessMinerConnection (Connection Handler)
Location: `src/LLP/stateless_miner_connection.cpp`

Connection-specific handling:
- **UPDATED** `SUBMIT_BLOCK`: Now supports Unified Hybrid Protocol with optional Falcon signature

#### 4. FalconAuth (Post-Quantum Authentication)
Location: `src/LLP/falcon_auth.cpp`

Falcon signature operations and identity binding (unchanged):
- `GenerateKey()`: Create Falcon key pair
- `Sign()`: Sign message with private key
- `Verify()`: Verify signature with public key
- `DeriveKeyId()`: Compute stable key identifier
- `BindGenesis()`: Link Falcon key to Tritium account
- `GetBoundGenesis()`: Retrieve bound identity

## Authentication Flow

### Phase 2 MINER_AUTH Protocol (Challenge-Response)

This aligns with NexusMiner's expected protocol:

1. **Miner Connects**
   - Connection established with `strAddress` and `nTimestamp`
   - Initial context created: `fAuthenticated = false`

2. **MINER_AUTH_INIT (207)** - Miner → Node (Enhanced Format)
   - Miner sends:
     - `[2 bytes]` pubkey_len (big-endian)
     - `[pubkey_len bytes]` Falcon public key (897 bytes)
     - `[2 bytes]` miner_id_len (big-endian)
     - `[miner_id_len bytes]` miner_id string (UTF-8, optional label)
     - `[32 bytes]` hashGenesis (uint256_t) ← NEW: Tritium genesis for reward routing
   - Node stores pubkey, validates genesis binding, and generates random 32-byte nonce
   - The hashGenesis field:
     - Specifies which Tritium account should receive mining rewards
     - Node validates this matches any existing Falcon→Genesis binding
     - If no binding exists, the node may auto-bind or require explicit binding
     - Legacy clients without hashGenesis are still supported (field is optional)

3. **MINER_AUTH_CHALLENGE (208)** - Node → Miner
   - Node sends:
     - `[2 bytes]` nonce_len (big-endian, always 32)
     - `[32 bytes]` random nonce

4. **MINER_AUTH_RESPONSE (209)** - Miner → Node
   - Miner signs nonce with Falcon private key
   - Miner sends:
     - `[2 bytes]` sig_len (big-endian)
     - `[sig_len bytes]` Falcon signature over nonce

5. **MINER_AUTH_RESULT (210)** - Node → Miner
   - Node verifies signature using stored pubkey
   - Node sends:
     - `[1 byte]` status (0x01 = success, 0x00 = failure)
   - On success: context updated with `fAuthenticated = true`, `hashKeyID`, etc.

## Unified Hybrid Falcon Signature Protocol

Block submissions now support an optional Falcon signature for enhanced identity verification:

### SUBMIT_BLOCK Formats

**Legacy Format** (backward compatible):
```
[64 bytes] merkle_root
[8 bytes]  nonce
```

**Unified Hybrid Format** (with Falcon signature):
```
[64 bytes] merkle_root
[8 bytes]  nonce
[2 bytes]  sig_len (big-endian)
[sig_len]  Falcon signature over (merkle_root || nonce)
```

The signature provides additional non-repudiation on top of the session-based authentication.

## Configuration

### nexus.conf Directives

To enable stateless mining:
```
mining=1
miningport=8323   # Default for testnet
miningpubkey=YOUR_FALCON_PUBKEY_HEX
```

### Port Configuration

- **Testnet**: 8323 (updated from 8325)
- **Mainnet**: 9325 (unchanged)

## Testing

Unit tests: `tests/unit/LLP/stateless_miner.cpp`

Coverage:
- MiningContext immutability
- WithNonce and WithPubKey methods
- ProcessResult factories
- MINER_AUTH_INIT processing
- SET_CHANNEL processing (1-byte and 4-byte legacy)
- SESSION_START authentication requirement
- FalconAuth key operations
- Genesis binding

## Security Considerations

### Security Properties
- **Post-quantum**: Uses Falcon lattice-based signatures
- **Challenge-response**: Fresh nonce per session prevents replay attacks
- **Immutable contexts**: Prevents state corruption
- **Identity binding**: Optional Tritium genesis linkage

### What This Does NOT Change
- Block validation rules
- Consensus mechanisms
- Transaction formats
- Reward calculations

## References

- Problem statement: See original Phase 2 objective document
- Phase 1: Stateless miner core infrastructure
- Falcon signatures: LLC FLKey implementation
- Tritium: Signature chain accounts

---

"To Man what is Impossible is ONLY possible with GOD"
