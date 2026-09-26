# Phase 2 Stateless Miner LLP Implementation

## Overview

This implementation adds a dedicated **Stateless Miner LLP server** on `miningport` that uses the pure functional `StatelessMiner` processor for Phase 2 mining protocol.

## Architecture

### Key Components

1. **StatelessMiner** (`src/LLP/stateless_miner.cpp`, `src/LLP/include/stateless_miner.h`)
   - Pure functional packet processor
   - Immutable `MiningContext` for state management
   - Returns `ProcessResult` with updated context and optional response packet
   - Handlers for Phase 2 protocol:
     - `ProcessFalconResponse`: Falcon authentication
     - `ProcessSetChannel`: Channel selection (Prime/Hash)
     - `ProcessGetBlock`: Block template requests
     - `ProcessSubmitBlock`: Block submission
     - `ProcessSessionKeepalive`: Session maintenance

2. **StatelessMinerConnection** (`src/LLP/stateless_miner_connection.cpp`, `src/LLP/types/stateless_miner_connection.h`)
   - Connection wrapper integrating `StatelessMiner` with LLP server framework
   - Manages connection lifecycle and context updates
   - Routes packets through `StatelessMiner::ProcessPacket`

3. **STATELESS_MINER_SERVER** (initialized in `src/LLP/global.cpp`)
   - LLP server instance on `miningport`
   - Starts when `-mining=1` is configured
   - Requires `miningpubkey` in `nexus.conf`

## Protocol Flow

### Phase 2 Mining Protocol

1. **Connection**
   ```
   Miner connects to node:miningport
   ```

2. **Falcon Authentication**
   ```
   Miner -> Node: FALCON_RESPONSE (pubkey + signature + optional genesis)
   Node  -> Miner: FALCON_VERIFY_OK (sessionId)
   ```

3. **Channel Selection**
   ```
   Miner -> Node: SET_CHANNEL (1=Prime, 2=Hash)
   Node  -> Miner: CHANNEL_ACK
   ```

4. **Block Mining Loop**
   ```
   Miner -> Node: GET_BLOCK
   Node  -> Miner: BLOCK_DATA (serialized block template)
   
   [Miner performs PoW]
   
   Miner -> Node: SUBMIT_BLOCK (merkle_root + nonce)
   Node  -> Miner: BLOCK_ACCEPTED | BLOCK_REJECTED
   ```

5. **Session Maintenance**
   ```
   Miner -> Node: SESSION_KEEPALIVE (periodic)
   ```

## Packet Definitions

All packet opcodes align with `miner.h` and NexusMiner Phase 2 protocol:

| Opcode | Name               | Direction    | Description                      |
|--------|--------------------|--------------|----------------------------------|
| 0      | BLOCK_DATA         | Node→Miner   | Block template for mining        |
| 1      | SUBMIT_BLOCK       | Miner→Node   | Submit solved block              |
| 3      | SET_CHANNEL        | Miner→Node   | Select mining channel            |
| 129    | GET_BLOCK          | Miner→Node   | Request block template           |
| 200    | BLOCK_ACCEPTED     | Node→Miner   | Block submission accepted        |
| 201    | BLOCK_REJECTED     | Node→Miner   | Block submission rejected        |
| 206    | CHANNEL_ACK        | Node→Miner   | Channel selection acknowledged   |
| 209    | FALCON_RESPONSE    | Miner→Node   | Falcon auth response             |
| 210    | FALCON_VERIFY_OK   | Node→Miner   | Falcon auth success              |
| 211    | SESSION_START      | Miner→Node   | Start session (future use)       |
| 212    | SESSION_KEEPALIVE  | Miner→Node   | Session keepalive                |

## Configuration

### nexus.conf

```conf
# Enable mining server
mining=1

# Mining public key (required)
miningpubkey=<falcon_pubkey_hex>

# Mining port (default: mainnet=9325, testnet=8325)
miningport=9325

# Mining threads (default: 4)
miningthreads=4

# Mining timeout (default: 30 seconds)
miningtimeout=30
```

## Logging

The implementation includes comprehensive logging for debugging:

- **Connection events**: New connections, disconnections
- **Falcon auth**: Auth requests, verification results, session IDs
- **Channel selection**: Channel changes
- **Block operations**: GET_BLOCK requests, SUBMIT_BLOCK results
- **Errors**: Authentication failures, invalid packets, processing errors

Use `-verbose=1` or higher to see detailed packet processing logs.

## Status and Next Steps

### Completed

- ✅ Phase 2 packet opcode definitions
- ✅ `StatelessMiner` packet handlers (functional signatures)
- ✅ `StatelessMinerConnection` wrapper class
- ✅ LLP server initialization on `miningport`
- ✅ Falcon authentication flow
- ✅ Channel selection logic
- ✅ Comprehensive logging framework

### TODO (Future PRs)

1. **Block Creation Integration** (`ProcessGetBlock`)
   - Integrate with `TAO::Ledger::Create::Block`
   - Implement block template serialization
   - Add block map management for tracking in-flight blocks

2. **Block Submission Integration** (`ProcessSubmitBlock`)
   - Parse merkle root and nonce from packet data
   - Validate and sign blocks
   - Process blocks through `TAO::Ledger::Process`
   - Implement accept/reject logic

3. **Session Management**
   - Session ID tracking and expiration
   - Multi-miner session coordination
   - Session-based block ownership

4. **Testing**
   - Unit tests for all packet handlers
   - Integration tests with NexusMiner
   - Load testing with multiple miners

## Design Rationale

### Why Dedicated Stateless Miner Server?

The existing `Miner` class uses a hybrid stateful/stateless approach with mutable state. The dedicated `StatelessMinerConnection` using `StatelessMiner` provides:

1. **Clear separation**: Mining protocol separate from legacy code
2. **Immutability**: Safer concurrent access via immutable `MiningContext`
3. **Testability**: Pure functions easier to unit test
4. **Maintainability**: Protocol changes isolated to `StatelessMiner`
5. **Forward compatibility**: Phase 2 protocol ready for future enhancements

### miningport Repurposing

Repurposing `miningport` for Phase 2 is acceptable because:

- Hard fork is planned
- Legacy miners will be upgraded to NexusMiner
- `miningport` rarely used in current deployments
- Clean break for new protocol

## Coordination with NexusMiner

This LLL-TAO PR must be deployed alongside the companion NexusMiner PR that:

- Connects to `miningport` as LLP endpoint
- Implements Falcon handshake client side
- Uses `SET_CHANNEL` / `GET_BLOCK` / `SUBMIT_BLOCK` instead of legacy `GET_HEIGHT`
- Supports both CPU and GPU mining with new architecture

> "To Man what is Impossible is ONLY possible with GOD"
