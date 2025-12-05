# Nexus Mining Guide

## Overview

Nexus supports secure, stateless mining using the Falcon post-quantum signature protocol. The node implements advanced cache management and DDOS protection while supporting both private solo mining and decentralized public pool mining.

## Mining Modes

### Private Solo Mining (Localhost)

Ideal for individuals running both the node and miner on the same machine.

**Features:**
- Extended cache timeout (30 days vs 7 days for remote)
- Optional ChaCha20 encryption (plaintext allowed)
- Simplified handshake process
- Direct reward payout to local Tritium account

**Setup:**

1. Configure your nexus.conf:
```
# Enable mining
mining=1

# Set your payout address (Tritium genesis hash)
miningaddress=YOUR_GENESIS_HASH

# Optional: Allow plaintext handshake for localhost
falconhandshake.allowlocalhostplaintext=1
```

2. Start the node:
```bash
./nexus
```

3. Connect your miner to localhost:
```bash
nexusminer --host=127.0.0.1 --port=9325
```

### Public Pool Mining (Internet)

For running a public mining pool accessible over the internet.

**Features:**
- Mandatory TLS 1.3 encryption
- Required ChaCha20 encryption for Falcon public keys
- 500 miner cache limit for DDOS protection
- 24-hour keepalive requirement
- 7-day inactivity purge
- Rewards tied to miner's Tritium GenesisHash

**Setup:**

1. Configure TLS certificates in nexus.conf:
```
# Enable mining pool
miningpool=1

# Require TLS for all connections
server=1
sslrequired=1

# TLS certificate paths
sslcert=/path/to/server.crt
sslkey=/path/to/server.key

# Require Falcon handshake encryption
falconhandshake.requireencryption=1

# Set cache limits (default: 500)
nodecache.maxsize=500

# Purge timeout in seconds (default: 7 days)
nodecache.purgetimeout=604800

# Keepalive interval (default: 24 hours)
nodecache.keepaliveinterval=86400
```

2. Generate TLS certificates (if not already done):
```bash
openssl req -x509 -newkey rsa:4096 -keyout server.key -out server.crt -days 365 -nodes
```

3. Start the node with public mining enabled:
```bash
./nexus -miningpool=1
```

4. Miners connect with TLS:
```bash
nexusminer --host=your.pool.com --port=9336 --ssl=1
```

## Falcon Handshake Protocol

The Falcon handshake establishes secure, post-quantum authenticated sessions between miners and the node.

### Handshake Flow

1. **Miner Authentication Init (MINER_AUTH_INIT)**
   - Miner sends Falcon public key
   - Optional: Public key encrypted with ChaCha20
   - Includes Tritium GenesisHash (payout address)
   - Includes session timestamp

2. **Node Challenge (MINER_AUTH_CHALLENGE)**
   - Node generates random challenge nonce
   - Challenge size scales with network load (32-64 bytes)
   - Challenge includes timestamp for replay protection

3. **Miner Response (MINER_AUTH_RESPONSE)**
   - Miner signs challenge with Falcon private key
   - Signature wrapped with disposable session key
   - Includes Tritium GenesisHash confirmation

4. **Node Verification (MINER_AUTH_RESULT)**
   - Node verifies Falcon signature
   - Validates GenesisHash ownership
   - Generates dynamic session key
   - Returns success/failure status

5. **Session Start (SESSION_START)**
   - Session officially begins
   - Miner added to cache
   - Session timeout configured
   - Keepalive timer started

### ChaCha20 Encryption

For public pool mining, Falcon public keys are encrypted during the handshake using ChaCha20.

**Key Exchange:**
- Encryption key derived from TLS session
- 32-byte ChaCha20 key
- 12-byte nonce (included in packet)
- Symmetric encryption (same key for encrypt/decrypt)

**Security Properties:**
- Protects Falcon public key from eavesdropping
- Prevents passive attacks on miner identity
- Complements TLS 1.3 transport encryption
- Optional for localhost (trusted environment)

### Session Key Generation

Dynamic session keys are generated per miner session, tied to their Tritium GenesisHash:

```
SessionKey = SK256(GenesisHash || MinerPubKey || RoundedTimestamp)
```

Where:
- `GenesisHash`: Miner's Tritium account identity
- `MinerPubKey`: Miner's Falcon public key
- `RoundedTimestamp`: Current time rounded to hour (for stability)

**Benefits:**
- No persistent key storage required
- Session-specific security
- Automatic key rotation (hourly)
- Tied to payout address verification

## Cache Management

The node implements sophisticated cache management for scalability and DDOS protection.

### Cache Limits

**Default Configuration:**
- Maximum cache size: 500 miners
- Keepalive interval: 24 hours
- Remote purge timeout: 7 days
- Localhost purge timeout: 30 days

### Cache Enforcement

When the cache reaches capacity:

1. **Priority Order (highest to lowest):**
   - Authenticated miners over unauthenticated
   - Localhost miners over remote
   - Recently active miners over inactive (based on last activity timestamp)

2. **Removal Strategy:**
   - Prioritizes removing **inactive** miners (least recently active)
   - First pass: Remove inactive unauthenticated remote miners
   - Second pass: Remove inactive authenticated remote miners (if needed)
   - Final pass: Remove localhost miners (if absolutely necessary)
   - This allows new active miners to join while purging stale connections

3. **Automatic Cleanup:**
   - Runs periodically (configurable interval)
   - Removes miners exceeding purge timeout
   - Respects localhost exception rules

### Keepalive Requirements

Miners must send keepalive pings to remain in cache:

**Default Interval:** 24 hours (86400 seconds)

**Recommended Frequency:**
- Summer months (high activity): 2 pings/day
- Winter months (standard): 1 ping/day
- Localhost miners: Can use longer intervals

**Keepalive Packet:**
```
Type: SESSION_KEEPALIVE
Payload: Empty or optional session data
```

Missing keepalives results in:
- Miner flagged as inactive
- Eventual removal via purge routine
- Session termination

## Session Recovery

The node provides automatic session recovery for miners experiencing temporary network interruptions.

### Network Interruption Handling

**Common Scenarios:**
- WiFi disconnections (home networks)
- ISP temporary outages
- Mobile network handoffs
- Power saving mode on devices
- Router reboots
- Cable modem resets

**Recovery Features:**

1. **Automatic Session Persistence**
   - All authenticated sessions are automatically saved
   - Recovery window: 1 hour (configurable)
   - No manual intervention required

2. **Falcon Key-Based Recovery**
   - Miner reconnects with Falcon key ID
   - No re-authentication needed
   - Session state fully restored

3. **Address-Based Recovery**
   - Alternative recovery by IP address
   - Useful for same-origin reconnects
   - Fallback if key ID not immediately available

### Recovery Process

**Miner Side:**
```bash
# Normal operation
nexusminer --host=pool.example.com --port=9336

# Connection lost (WiFi drops)
# ... network interruption ...

# Automatic reconnection attempt
# Miner presents Falcon key ID from previous session
# Session restored without full handshake
```

**Node Side Validation:**
1. Check if session exists for presented key ID
2. Verify session not expired (< 1 hour since last activity)
3. Verify reconnection count under limit (< 10 attempts)
4. Restore session: channel, genesis hash, authentication status
5. Resume mining operations

**Configuration:**

```bash
# In nexus.conf
sessionrecovery.timeout=3600        # 1 hour default
sessionrecovery.maxreconnects=10    # 10 attempts default
sessionrecovery.enabled=1           # Enable recovery
```

**Monitoring:**

```bash
# Query active recoverable sessions
nexus-cli getsessioncount

# Manually cleanup expired sessions
nexus-cli cleanupsessions

# Check session status
nexus-cli getsessioninfo <keyid>
```

### Benefits for Residential Miners

**Without Session Recovery:**
- Full re-authentication required after every disconnect
- Mining progress lost
- Increased server load
- Poor user experience for home miners

**With Session Recovery:**
- Seamless reconnection within 1-hour window
- Mining continues without interruption
- Reduced authentication overhead
- GenesisHash payout binding maintained
- Suitable for non-datacenter environments

**Limits:**
- Maximum 10 reconnection attempts per session
- 1-hour recovery window (prevents indefinite session persistence)
- Only authenticated sessions are recoverable
- Session purged after timeout or excessive reconnects

## Reward System Integration

Rewards are tied directly to Tritium GenesisHash for stateful validation.

### Payout Flow

**Solo Mining (Localhost):**
1. Miner submits valid block
2. Node verifies signature and work
3. Reward credited to configured mining address
4. Transaction confirmed on blockchain

**Pool Mining (Public):**
1. Miner submits valid block with GenesisHash
2. Node verifies Falcon signature
3. Node validates GenesisHash ownership
4. Reward credited to miner's Tritium account
5. Pool tracks shares and distributes accordingly

### GenesisHash Validation

The node validates miner payout addresses:

```cpp
// Verify miner owns the Genesis account
bool ValidateGenesis(uint256_t hashGenesis, uint256_t hashKeyID)
{
    // 1. Retrieve Tritium account from blockchain
    // 2. Verify account exists and is active
    // 3. Confirm Falcon key is bound to account
    // 4. Validate ownership signature
    return fValid;
}
```

## Security Considerations

### DDOS Protection

The node implements multiple layers of DDOS protection:

1. **Cache Size Limit:** Hard cap at 500 concurrent miners
2. **Keepalive Enforcement:** Inactive miners automatically purged
3. **Rate Limiting:** Challenge complexity scales with load
4. **Signature Verification:** CPU-bound defense against spam
5. **Session Timeouts:** Default 5-minute inactivity timeout

### Localhost Exceptions

Localhost miners receive special treatment:

**Rationale:**
- Trusted environment (same machine)
- Solo mining use case
- Lower DDOS risk
- Enhanced user experience

**Exceptions:**
- Extended cache timeout (30 days vs 7)
- Optional ChaCha20 encryption
- Lower priority for cache eviction
- Simplified validation

### TLS Configuration

For public pools, TLS 1.3 is mandatory with specific cipher suites:

**Recommended Ciphers:**
```
TLS_CHACHA20_POLY1305_SHA256
TLS_AES_256_GCM_SHA384
TLS_AES_128_GCM_SHA256
```

**Configuration:**
```
# Require TLS 1.3 minimum
sslminversion=1.3

# Prefer ChaCha20-Poly1305 for performance
sslciphers=TLS_CHACHA20_POLY1305_SHA256:TLS_AES_256_GCM_SHA384
```

## Monitoring & Debugging

### RPC Commands

Query miner status and cache statistics:

```bash
# Get all active miners
nexus-cli getminers

# Get specific miner status
nexus-cli getminer <address>

# Get cache statistics
nexus-cli getcachestats

# Get Falcon session statistics
nexus-cli getfalconstats
```

### Log Levels

Enable detailed logging for debugging:

```
# In nexus.conf
debug=1
verbose=3

# Falcon-specific logging
debug.falcon=1

# Cache-specific logging
debug.nodecache=1
```

### Common Issues

**Miner disconnects after 24 hours:**
- Cause: Missing keepalive pings
- Solution: Ensure miner sends keepalive at least every 23 hours

**Cache full errors:**
- Cause: Too many concurrent miners
- Solution: Increase cache limit or improve cleanup frequency

**Handshake failures:**
- Cause: Encryption mismatch or expired certificates
- Solution: Verify TLS configuration and certificate validity

**Invalid GenesisHash:**
- Cause: Miner using incorrect payout address
- Solution: Verify Tritium account exists and is active

## Performance Tuning

### High-Volume Pools

For pools expecting >500 concurrent miners:

```
# Increase cache size (use with caution)
nodecache.maxsize=1000

# Reduce purge timeout for faster cleanup
nodecache.purgetimeout=259200  # 3 days

# Increase cleanup frequency
nodecache.cleanupinterval=3600  # Every hour
```

### Resource Optimization

**CPU:**
- Falcon signature verification is CPU-intensive
- Consider multi-core scaling
- Use challenge size scaling to manage load

**Memory:**
- Each cached miner: ~2KB
- 500 miners: ~1MB cache overhead
- Monitor with `getmemoryinfo`

**Network:**
- ChaCha20 adds minimal overhead
- TLS handshake: ~5KB per connection
- Keepalive packets: <100 bytes

## Testing

### Unit Tests

Run the Falcon handshake and cache tests:

```bash
# Build with unit tests
make -f makefile.cli UNIT_TESTS=1

# Run all tests
./nexus --run-tests

# Run specific test suites
./nexus --run-tests "node_cache"
./nexus --run-tests "falcon_handshake"
```

### Integration Tests

Test full mining workflow:

1. Start node in test mode
2. Connect test miner
3. Complete Falcon handshake
4. Submit test shares
5. Verify cache behavior
6. Test keepalive and purging

## Support

For issues or questions:
- GitHub Issues: https://github.com/Nexusoft/LLL-TAO/issues
- Discord: https://discord.gg/nexus
- Telegram: https://t.me/NexusOfficial
