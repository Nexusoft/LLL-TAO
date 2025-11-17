# Testing Falcon-based Miner Authentication

This document describes how to manually test the new Falcon-based authentication handshake for stateless MinerLLP connections.

## Prerequisites

1. Build the Nexus node with Falcon support:
   ```bash
   make -j 4 -f makefile.cli
   ```

2. Create a test configuration file `~/.Nexus/nexus.conf`:
   ```
   # Enable Falcon support
   falcon=1
   
   # Set to testnet or local testnet for testing
   testnet=1
   
   # Enable verbose logging for debugging
   verbose=3
   
   # Other standard settings
   daemon=1
   server=1
   ```

3. Start the Nexus node:
   ```bash
   ./nexus
   ```

## Test Scenarios

### Scenario 1: Successful Authentication

**Goal:** Verify that a miner with a valid Falcon key pair can authenticate successfully.

**Steps:**
1. Connect to the node at `127.0.0.1:8323` using a test client or modified NexusMiner
2. Send `MINER_AUTH_INIT` packet (opcode 207) with:
   - 2 bytes: pubkey_len (big-endian)
   - N bytes: Falcon public key
   - 2 bytes: miner_id_len (big-endian)
   - M bytes: miner_id string (e.g., "test-miner-1")

3. Receive `MINER_AUTH_CHALLENGE` packet (opcode 208) containing:
   - 2 bytes: nonce_len (big-endian, should be 32)
   - 32 bytes: random nonce

4. Sign the nonce with the Falcon private key corresponding to the public key sent
5. Send `MINER_AUTH_RESPONSE` packet (opcode 209) with:
   - 2 bytes: sig_len (big-endian)
   - N bytes: Falcon signature over the nonce

6. Receive `MINER_AUTH_RESULT` packet (opcode 210) containing:
   - 1 byte: 0x01 (success)

**Expected Node Logs:**
```
MinerLLP: New connection accepted from 127.0.0.1:xxxxx
MinerLLP: Using stateless Miner session for localhost connection
MinerLLP: MINER_AUTH_INIT from 127.0.0.1 miner_id=test-miner-1 pubkey_len=XXX
MinerLLP: MINER_AUTH_CHALLENGE nonce_len=32
MinerLLP: MINER_AUTH_RESPONSE from 127.0.0.1 sig_len=XXX
MinerLLP: MINER_AUTH success for miner_id=test-miner-1 from 127.0.0.1
```

7. After successful authentication, send mining commands:
   - `SET_CHANNEL` (opcode 3) - should succeed
   - `GET_HEIGHT` (opcode 130) - should succeed
   - `GET_BLOCK` (opcode 129) - should succeed

**Expected Result:** All mining commands should execute successfully after authentication.

### Scenario 2: Failed Authentication (Invalid Signature)

**Goal:** Verify that an invalid signature is rejected and connection is closed.

**Steps:**
1. Connect and send `MINER_AUTH_INIT` as in Scenario 1
2. Receive `MINER_AUTH_CHALLENGE` with nonce
3. Send `MINER_AUTH_RESPONSE` with an **invalid signature** (e.g., random bytes or signature from wrong key)

**Expected Node Logs:**
```
MinerLLP: MINER_AUTH_INIT from 127.0.0.1 miner_id=test-miner-1 pubkey_len=XXX
MinerLLP: MINER_AUTH_CHALLENGE nonce_len=32
MinerLLP: MINER_AUTH_RESPONSE from 127.0.0.1 sig_len=XXX
MinerLLP: MINER_AUTH verification FAILED for miner_id=test-miner-1 from 127.0.0.1
```

**Expected Result:**
- Receive `MINER_AUTH_RESULT` with 0x00 (failure)
- Connection should be closed by the node

### Scenario 3: Mining Command Before Authentication

**Goal:** Verify that mining commands are rejected before authentication completes.

**Steps:**
1. Connect to the node at `127.0.0.1:8323`
2. **Without** sending `MINER_AUTH_INIT`, immediately send a mining command like `SET_CHANNEL` (opcode 3)

**Expected Node Logs:**
```
MinerLLP: New connection accepted from 127.0.0.1:xxxxx
MinerLLP: Using stateless Miner session for localhost connection
MinerLLP: Stateless miner attempted SET_CHANNEL before authentication from 127.0.0.1 header=0x3
```

**Expected Result:**
- Receive `MINER_AUTH_RESULT` with 0x00 (failure)
- Command is rejected with error message

### Scenario 4: Protocol Errors

**Goal:** Verify proper handling of malformed packets.

**Test Cases:**
1. Send `MINER_AUTH_INIT` with packet too small (< 4 bytes)
2. Send `MINER_AUTH_INIT` with pubkey_len exceeding packet size
3. Send `MINER_AUTH_RESPONSE` before `MINER_AUTH_INIT`
4. Send `MINER_AUTH_RESPONSE` with invalid sig_len

**Expected Result:** All cases should log appropriate error and either disconnect or return error response.

### Scenario 5: PING Without Authentication

**Goal:** Verify that PING command works without authentication (for connectivity testing).

**Steps:**
1. Connect to the node at `127.0.0.1:8323`
2. Send `PING` command (opcode 253) **without** authenticating first

**Expected Result:**
- Receive `PING` response (opcode 253)
- Connection remains open
- PING does not require authentication

## Verifying Logs

To view detailed logs:
```bash
tail -f ~/.Nexus/testnet/debug.log | grep -E "MinerLLP|MINER_AUTH"
```

Key log markers to look for:
- `MinerLLP: MINER_AUTH_INIT` - Initial auth request received
- `MinerLLP: MINER_AUTH_CHALLENGE` - Challenge sent to miner
- `MinerLLP: MINER_AUTH success` - Authentication succeeded
- `MinerLLP: MINER_AUTH verification FAILED` - Authentication failed
- `MinerLLP: Stateless miner attempted` - Unauthenticated command attempt

## Test Client Implementation Notes

A test client needs to:
1. Generate or load a Falcon key pair (using Nexus LLC::FLKey class)
2. Implement the binary packet format for MinerLLP
3. Handle the challenge-response flow
4. Sign the nonce using Falcon signature algorithm

For reference, see the NexusMiner companion PR at NamecoinGithub/NexusMiner for a complete implementation.

## Future Enhancements (Not in This PR)

The following features will be added in a follow-up PR:
- Configuration file mapping of miner Falcon pubkeys to genesis hashes
- Using `hashGenesisForMiner` to set the sigchain context for block creation
- Multiple authenticated miner support with different identities

## Security Considerations

- Authentication state is per-connection and in-memory only
- Nonce is generated using cryptographically secure RNG (LLC::GetRand256)
- Falcon signature verification uses existing LLC::FLKey implementation
- Failed authentication closes the connection immediately
- All mining commands require successful authentication in stateless mode
