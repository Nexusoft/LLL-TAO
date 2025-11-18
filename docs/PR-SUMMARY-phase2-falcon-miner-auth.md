# Phase 2: Falcon-Driven Miner Authentication Layer

## Overview

This implementation completes Phase 2 of the unified mining stack by adding a fully stateless, Falcon-driven miner authentication layer inside LLL-TAO. The work builds on Phase 1 to provide identity-aware mining contexts tied to Tritium accounts.

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

All state updates return new contexts via `WithX()` methods.

#### 2. StatelessMiner (Pure Functional Processor)
Location: `src/LLP/stateless_miner.cpp`

Pure functions for packet processing:
- `ProcessPacket()`: Main routing logic
- `ProcessFalconAuthResponse()`: **Phase 2** Falcon signature verification
- `ProcessSessionStart()`: **Phase 2** Requires prior authentication
- `ProcessSetChannel()`: Channel selection with legacy compatibility

#### 3. StatelessMinerManager (State Tracking)
Location: `src/LLP/stateless_manager.cpp`

Thread-safe manager for active miner contexts:
- `UpdateMiner()`: Update or add miner
- `RemoveMiner()`: Disconnect miner
- `GetMinerContext()`: Query miner state
- `ListMiners()`: Get all miners
- **Phase 2**: JSON output includes `key_id` and `genesis`

#### 4. FalconAuth (Post-Quantum Authentication)
Location: `src/LLP/falcon_auth.cpp`

Falcon signature operations and identity binding:
- `GenerateKey()`: Create Falcon key pair
- `Sign()`: Sign message with private key
- `Verify()`: Verify signature with public key
- `DeriveKeyId()`: Compute stable key identifier
- **Phase 2**: `BindGenesis()`: Link Falcon key to Tritium account
- **Phase 2**: `GetBoundGenesis()`: Retrieve bound identity

## RPC Commands

All commands added to the `system` namespace:

### Miner Status Commands
- `system/list/miners` - List all active stateless miners
- `system/get/minerstatus <address>` - Get status for specific miner
- `system/disconnect/miner <address>` - Disconnect a miner

### Falcon Key Management
- `system/generate/falconkey [label]` - Generate new Falcon key pair
- `system/list/falconkeys` - List all stored Falcon keys
- `system/test/falconauth <key_id>` - Test sign/verify for a key
- `system/bind/falconkey <key_id> <genesis>` - Bind key to Tritium identity

## Authentication Flow

### Phase 2 Falcon Authentication

1. **Miner Connects**
   - Connection established with `strAddress` and `nTimestamp`
   - Initial context created: `fAuthenticated = false`

2. **Falcon Auth Packet (MINER_AUTH_RESPONSE)**
   - Miner sends:
     - Falcon public key
     - Signature over `(strAddress + nTimestamp)`
     - Optional: Tritium `hashGenesis`
   
3. **Verification**
   - Node calls `FalconAuth::Verify(pubkey, message, signature)`
   - Derives `hashKeyID = SK256(pubkey)`
   - Checks for bound genesis via `GetBoundGenesis(keyId)`
   - Generates `sessionId` from `hashKeyID` (lower 32 bits)

4. **Success**
   - Returns new context:
     - `fAuthenticated = true`
     - `nSessionId = derived`
     - `hashKeyID = verified`
     - `hashGenesis = bound or provided`
   - Sends success response with `sessionId`

5. **Session Start (Post-Auth)**
   - `SESSION_START` now **requires** `fAuthenticated == true`
   - Acts as session negotiation, not authentication

## Security Considerations

### What This Changes
- Mining authentication mechanism (stateless → Falcon-based)
- Session management (authenticated before session start)
- Identity binding (Falcon keys → Tritium accounts)

### What This Does NOT Change
- Block validation rules
- Consensus mechanisms
- Transaction formats
- Reward calculations

### Security Properties
- **Post-quantum**: Uses Falcon lattice-based signatures
- **Immutable contexts**: Prevents state corruption
- **Replay protection**: Timestamp in signed message
- **Identity binding**: Optional Tritium genesis linkage

## Testing

Unit tests: `tests/unit/LLP/stateless_miner.cpp`

Coverage:
- MiningContext immutability
- ProcessResult factories
- SET_CHANNEL processing (1-byte and 4-byte legacy)
- SESSION_START authentication requirement
- FalconAuth key operations
- Genesis binding

## Future Work (Phase 3)

- Auto-configuration from `NEXUS_GENESIS` environment variable
- Direct node-side mining for GUI integration
- Challenge-response nonces for enhanced security
- Persistent Falcon key storage (currently in-memory)
- MinerBridge integration with existing `Miner` class

## References

- Problem statement: See original Phase 2 objective document
- Phase 1: Stateless miner core infrastructure
- Falcon signatures: LLC FLKey implementation
- Tritium: Signature chain accounts

---

"To Man what is Impossible is ONLY possible with GOD"
