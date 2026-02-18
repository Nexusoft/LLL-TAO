# Mining Server Architecture

## Overview

The Nexus mining server implements the LLP (Lower Level Protocol) for stateless mining with Falcon-1024 authentication. This document describes the server architecture, connection handling, and internal components.

**Component:** `LLP::Server<StatelessMinerConnection>`  
**Port:** 9323 (mainnet and testnet)  
**Protocol:** Stateless Mining 1.0  
**Version:** LLL-TAO 5.1.0+

---

## Server Architecture

### Component Hierarchy

```
┌──────────────────────────────────────────────────────┐
│                  Mining Server                        │
├──────────────────────────────────────────────────────┤
│                                                        │
│  LLP::Server<StatelessMinerConnection>               │
│    ├─ Thread Pool (configurable, default: 4)         │
│    ├─ Connection Manager                             │
│    ├─ Session Cache (24h default)                    │
│    ├─ Template Generator                             │
│    └─ Push Notification Broadcaster                  │
│                                                        │
│  Per-Connection:                                      │
│  StatelessMinerConnection                            │
│    ├─ Packet Parser                                  │
│    ├─ Authentication Handler                         │
│    ├─ Template Delivery                              │
│    ├─ Block Validation                               │
│    └─ ChaCha20 Encryption                            │
│                                                        │
└──────────────────────────────────────────────────────┘
```

### Thread Model

The server uses a multi-threaded architecture:

**Main Thread:**
- Accepts incoming connections
- Dispatches connections to worker threads
- Manages server lifecycle

**Worker Threads:**
- Handle packet processing
- Manage connection state
- Perform block validation
- Send responses

**Notification Thread:**
- Broadcasts NEW_BLOCK to subscribed miners
- Triggered by blockchain events
- Asynchronous, non-blocking

---

## Connection Lifecycle

### 1. Connection Establishment

```cpp
// src/LLP/server.h
template<class ProtocolType>
class Server
{
public:
    bool Start()
    {
        // Bind to port
        if(!socket_.Bind(nPort_))
            return false;
        
        // Start listening
        if(!socket_.Listen(SOMAXCONN))
            return false;
        
        // Launch accept thread
        threadAccept_ = std::thread(&Server::AcceptConnections, this);
        
        debug::log(0, FUNCTION, "LLP Server started on port ", nPort_);
        return true;
    }
    
private:
    void AcceptConnections()
    {
        while(fRunning_)
        {
            // Accept new connection
            Socket socket = socket_.Accept();
            
            if(!socket.IsValid())
                continue;
            
            // Apply rate limiting
            if(IsRateLimited(socket.GetAddress()))
            {
                socket.Close();
                continue;
            }
            
            // Create connection object
            ProtocolType* pConnection = new ProtocolType(socket);
            
            // Dispatch to worker thread
            AddConnection(pConnection);
        }
    }
};
```

### 2. Authentication Phase

```
Miner connects → MINER_AUTH packet → Node validates → Session created
```

**Key Steps:**
1. Extract genesis hash and Falcon public key
2. Verify genesis exists on blockchain
3. Validate Falcon key format (897 or 1793 bytes)
4. Check optional whitelist (`-minerallowkey`)
5. Create session with ChaCha20 key
6. Send MINER_AUTH_RESPONSE with session ID

**Duration:** <2ms typical

### 3. Configuration Phase

```
MINER_SET_REWARD → SET_CHANNEL → MINER_READY
```

**Key Steps:**
1. Bind reward payout address (ChaCha20 encrypted)
2. Select mining channel (1=Prime, 2=Hash)
3. Subscribe to push notifications
4. Receive initial GET_BLOCK template

**Duration:** <5ms total

### 4. Active Mining Phase

```
[Mining...] ← NEW_BLOCK notifications ← [Blockchain advances]
```

**Key Features:**
- Push-based template delivery (no polling)
- Channel-isolated notifications
- <10ms latency from blockchain event to miner notification

### 5. Block Submission

```
SUBMIT_BLOCK → Validation → BLOCK_ACCEPTED or BLOCK_REJECTED
```

**Validation Steps:**
1. Decrypt packet (if ChaCha20 enabled)
2. Verify proof-of-work
3. Validate Falcon signature (if Physical Falcon present)
4. Check block structure
5. Validate transactions
6. Accept to blockchain
7. Broadcast NEW_BLOCK to all miners

**Duration:** 20-30ms typical

### 6. Session Maintenance

```
[Periodic keepalives] → Session timeout extended → [Continue mining]
```

**Timeout Management:**
- Default: 24 hours (86400 seconds)
- Keepalive interval: Recommended every 5 minutes
- Disconnection: Session preserved until timeout

### 7. Disconnection

```
Miner disconnects → Session preserved → Automatic cleanup after timeout
```

**Cleanup:**
- Remove from push notification list
- Preserve session cache for reconnection
- Automatic expiry after timeout

---

## Internal Components

### Session Cache

Thread-safe storage for active mining sessions:

```cpp
// Session storage
std::map<uint32_t, MiningContext> SessionCache;
std::mutex SessionCacheMutex;

// Session access
MiningContext& GetSession(uint32_t nSessionId)
{
    std::lock_guard<std::mutex> lock(SessionCacheMutex);
    return SessionCache.at(nSessionId);
}

// Session cleanup
void CleanupExpiredSessions()
{
    std::lock_guard<std::mutex> lock(SessionCacheMutex);
    uint64_t nNow = runtime::unifiedtimestamp();
    
    for(auto it = SessionCache.begin(); it != SessionCache.end(); )
    {
        if(it->second.IsSessionExpired(nNow))
            it = SessionCache.erase(it);
        else
            ++it;
    }
}
```

**Capacity:** Scales to 1000+ concurrent sessions  
**Lookup:** O(log n) via std::map  
**Memory:** ~1 KB per session

### Template Generator

Creates wallet-signed block templates:

```cpp
TAO::Ledger::TritiumBlock* CreateNewBlock(uint32_t nChannel, 
                                          const uint256_t& hashReward)
{
    // Get blockchain state
    TAO::Ledger::BlockState stateBest = 
        TAO::Ledger::ChainState::tStateBest.load();
    
    // Create block structure
    TAO::Ledger::TritiumBlock* pBlock = new TAO::Ledger::TritiumBlock();
    pBlock->nVersion = stateBest.nVersion;
    pBlock->nChannel = nChannel;
    pBlock->hashPrevBlock = stateBest.GetHash();
    pBlock->nHeight = stateBest.nHeight + 1;
    pBlock->nBits = GetNextWorkRequired(stateBest, nChannel);
    pBlock->nTime = runtime::unifiedtimestamp();
    pBlock->hashProducer = hashReward;
    
    // Add coinbase transaction
    AddCoinbaseTransaction(pBlock, hashReward);
    
    // Calculate merkle root
    pBlock->hashMerkleRoot = pBlock->BuildMerkleTree();
    
    // Sign with wallet
    SignBlockWithWallet(pBlock);
    
    return pBlock;
}
```

**Performance:** <1ms per template  
**Caching:** Not cached (always fresh)  
**Concurrency:** Thread-safe via atomic operations

### Push Notification Broadcaster

Sends NEW_BLOCK to subscribed miners:

```cpp
std::set<uint32_t> PushNotificationList;
std::mutex NotificationListMutex;

void BroadcastNewBlock(uint32_t nChannel)
{
    std::lock_guard<std::mutex> lock(SessionCacheMutex);
    
    for(auto session_id : PushNotificationList)
    {
        MiningContext& ctx = SessionCache[session_id];
        
        // Channel isolation
        if(ctx.nSubscribedChannel != nChannel)
            continue;
        
        // Generate template
        TAO::Ledger::TritiumBlock* pBlock = 
            CreateNewBlock(nChannel, ctx.GetPayoutAddress());
        
        // Send notification
        SendNewBlock(session_id, pBlock);
        
        delete pBlock;
    }
}
```

**Latency:** <10ms for 100 miners  
**Concurrency:** Parallel template generation  
**Scalability:** Linear O(n) with miner count

### Packet Parser

Handles incoming LLP packets:

```cpp
class StatelessMinerConnection : public Connection
{
    void ProcessPacket(const Packet& packet)
    {
        switch(packet.HEADER)
        {
            case Opcodes::StatelessMining::MINER_AUTH:
                return ProcessAuth(packet);
                
            case Opcodes::StatelessMining::MINER_SET_REWARD:
                return ProcessSetReward(packet);
                
            case Opcodes::StatelessMining::SET_CHANNEL:
                return ProcessSetChannel(packet);
                
            case Opcodes::StatelessMining::MINER_READY:
                return ProcessMinerReady(packet);
                
            case Opcodes::StatelessMining::SUBMIT_BLOCK:
                return ProcessSubmitBlock(packet);
                
            case Opcodes::StatelessMining::SESSION_KEEPALIVE:
                return ProcessSessionKeepalive(packet);
                
            default:
                debug::warning("Unknown opcode: 0x", 
                              std::hex, packet.HEADER);
                return;
        }
    }
};
```

---

## Performance Characteristics

### Latency Targets

| Operation | Target | Typical |
|-----------|--------|---------|
| Connection accept | <1ms | <0.5ms |
| Authentication | <5ms | 2-3ms |
| Template generation | <1ms | 0.5ms |
| Template delivery | <5ms | 2-3ms |
| Block validation | <50ms | 20-30ms |
| NEW_BLOCK broadcast | <10ms | 5-8ms |

### Throughput Capacity

**Per Server Instance:**
- Concurrent connections: 100-500
- New connections/sec: 100+
- Templates generated/sec: 1000+
- Blocks validated/sec: 50-100
- Push notifications/sec: 1000+

### Resource Usage

**CPU:**
- Idle: <1% single core
- Active (100 miners): 2-4 cores
- Peak (block submissions): 4-8 cores

**Memory:**
- Base: 50-100 MB
- Per session: ~1 KB
- 100 sessions: ~150 MB total
- 500 sessions: ~600 MB total

**Network:**
- Idle: <1 Kbps
- Active mining (100 miners): 5-10 Mbps
- Peak (NEW_BLOCK broadcast): 20-50 Mbps burst

---

## Security Features

### DDoS Protection

Optional rate limiting and connection scoring:

```ini
# Enable DDoS protection
miningddos=1
miningcscore=1        # Connection score threshold
miningrscore=50       # Request score threshold
miningtimespan=60     # Time window (seconds)
```

**Implementation:**
- Connection-based scoring
- Request-based scoring
- Automatic IP banning
- Configurable thresholds

### SSL/TLS Encryption

Optional transport layer security:

```ini
# Enable SSL for mining
miningssl=1
miningsslrequired=1   # Reject non-SSL connections
sslcert=/path/to/server.crt
sslkey=/path/to/server.key
```

**Features:**
- TLS 1.3 support
- Perfect forward secrecy
- Certificate validation
- Optional client certificates

### Key Whitelisting

Restrict mining to specific Falcon public keys:

```ini
# Whitelist specific miner keys
minerallowkey=0123456789abcdef...
minerallowkey=fedcba9876543210...
```

**Use Cases:**
- Private mining pools
- Corporate mining operations
- Restricted testnet access

---

## Configuration Reference

### Core Settings

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `mining` | bool | `false` | Enable mining server |
| `miningport` | int | `9323` | Mining server port |
| `miningthreads` | int | CPU cores | Worker thread count |
| `miningtimeout` | int | `30` | Socket timeout (seconds) |

### Security Settings

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `miningssl` | bool | `false` | Enable SSL/TLS |
| `miningsslrequired` | bool | `false` | Require SSL for all connections |
| `miningddos` | bool | `false` | Enable DDoS protection |
| `minerallowkey` | string | none | Whitelist Falcon public keys |

### Performance Settings

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `miningcscore` | int | `1` | Connection score threshold |
| `miningrscore` | int | `50` | Request score threshold |
| `miningtimespan` | int | `60` | DDoS time window (seconds) |

**Full Reference:** [nexus.conf.md](../../reference/nexus.conf.md)

---

## Monitoring and Diagnostics

### Log Messages

**Startup:**
```
[Mining] LLP Server started on port 9323
[Mining] Authentication: MINER-DRIVEN (Falcon keys from miners)
[Mining] Reward Routing: STATELESS (via MINER_SET_REWARD)
```

**Connections:**
```
[Mining] Miner connected from 192.168.1.100:54321
[Mining] Miner 12345678 authenticated (Falcon-1024, genesis: abc123...)
[Mining] Miner 12345678 subscribed to Prime notifications
```

**Template Delivery:**
```
[Mining] Sent GET_BLOCK to miner 12345678 (channel=Prime)
[Mining] Broadcasted NEW_BLOCK to 42 miners in 6.2ms (channel=Hash)
```

**Block Submissions:**
```
[Mining] Block accepted from miner 12345678 (height=6535199, validation=24ms)
[Mining] Block rejected from miner 87654321 (reason: Invalid proof-of-work)
```

### Performance Metrics

Enable verbose logging for performance tracking:

```ini
verbose=2  # Standard logging
verbose=3  # Verbose (includes timing)
```

**Tracked Metrics:**
- Template generation time
- Packet processing time
- Validation duration
- Broadcast latency

### Debug Mode

Enable maximum verbosity for troubleshooting:

```ini
verbose=5  # Maximum verbosity (trace level)
```

**Includes:**
- All packet contents
- Session state transitions
- Cache operations
- Thread activity

---

## Cross-References

**Related Documentation:**
- [Stateless Protocol](stateless-protocol.md)
- [Mining Lanes Cheat Sheet](mining-lanes-cheat-sheet.md)
- [Protocol Lanes](../../protocol/mining-protocol.md)
- [Push Notification Flow Diagram](../../diagrams/push-notification-flow.md)
- [Configuration Reference](../../reference/nexus.conf.md)
- [Troubleshooting](../troubleshooting/mining-server-issues.md)

**Source Files:**
- `src/LLP/global.cpp` - Server initialization
- `src/LLP/miner.cpp` - Server implementation
- `src/LLP/stateless_miner_connection.cpp` - Connection handling
- `src/LLP/include/stateless_miner.h` - Data structures

---

## Version Information

**Document Version:** 1.0  
**LLL-TAO Version:** 5.1.0+  
**Last Updated:** 2026-01-13
