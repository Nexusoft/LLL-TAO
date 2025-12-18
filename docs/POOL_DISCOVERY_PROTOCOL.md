# Pool Discovery Protocol Specification

## Overview

The Pool Discovery Protocol enables decentralized mining pool infrastructure on the Nexus network by allowing pool operators to broadcast their availability and miners to discover pools by genesis hash - eliminating the need for manual IP address configuration.

## Core Concepts

### Current State (Manual)
```
Miner needs to know:
├─ Pool IP address: 123.45.67.89
├─ Pool port: 9549
└─ Manual configuration required ❌
```

### Goal State (Discovery)
```
Pool broadcasts:
├─ Genesis hash: 8abc123def... (identity)
├─ Endpoint: Auto-discovered
├─ Fee: 2%
├─ Reputation: 98/100
└─ Miners query and auto-connect ✅
```

## Protocol Components

### 1. Pool Announcement Structure

Pool operators broadcast a signed announcement containing:

```cpp
struct MiningPoolAnnouncement {
    uint256_t hashGenesis;           // Pool operator's genesis (identity)
    std::string strEndpoint;         // IP:PORT (auto-detected or configured)
    uint8_t nFeePercent;             // Pool fee (0-5%)
    uint16_t nMaxMiners;             // Max concurrent miners
    uint16_t nCurrentMiners;         // Current connected miners
    uint64_t nTotalHashrate;         // Total pool hashrate
    uint32_t nBlocksFound;           // Lifetime blocks found
    std::string strPoolName;         // Human-readable name
    uint64_t nTimestamp;             // Announcement timestamp
    std::vector<uint8_t> vSignature; // Signed with pool operator's key
};
```

### 2. Broadcasting Mechanism

**Frequency:**
- Announcements broadcast every 1 hour (configurable via `MIN_ANNOUNCEMENT_INTERVAL`)
- On-demand broadcast on pool startup
- Updates on miner count/hashrate changes

**Gossip Protocol:**
- Announcements propagate peer-to-peer
- Each peer caches announcements (24 hour TTL)
- Probabilistic re-broadcast (30% chance) to limit network traffic

### 3. Pool Discovery

Nodes can query for available pools using the `mining/listpools` RPC endpoint:

```json
{
  "maxfee": 5,           // Filter: max fee percentage (0-5)
  "minreputation": 0,    // Filter: min reputation score (0-100)
  "onlineonly": true     // Filter: only responsive pools
}
```

**Returns:**
```json
[
  {
    "genesis": "8abc123def...",
    "endpoint": "123.45.67.89:9549",
    "fee": 2,
    "currentminers": 150,
    "maxminers": 500,
    "hashrate": 1000000000,
    "blocksfound": 1234,
    "reputation": 98,
    "uptime": 0.995,
    "name": "Example Mining Pool",
    "lastseen": 1734529175,
    "online": true
  }
]
```

## Validation Rules

### Announcement Validation

1. **Fee Cap:** Pool fee must be ≤ 5% (`MAX_POOL_FEE_PERCENT`)
2. **Signature:** Must be validly signed by genesis credentials
3. **Genesis Exists:** Genesis hash must exist on blockchain
4. **Rate Limiting:** Announcements limited to 1 per hour per genesis
5. **Trust Requirement:** Genesis must have ≥ 30 days trust (configurable)

### Validation Code

```cpp
bool ValidatePoolAnnouncement(const MiningPoolAnnouncement& announcement) {
    // Reject excessive fees
    if (announcement.nFeePercent > MAX_POOL_FEE_PERCENT)
        return false;
    
    // Must be signed
    if (!announcement.Verify())
        return false;
    
    // Genesis must exist on blockchain
    if (!LLD::Ledger->HasGenesis(announcement.hashGenesis))
        return false;
    
    // Rate limit announcements (prevent spam)
    if (RecentlyAnnounced(announcement.hashGenesis))
        return false;
    
    // Check trust requirements
    if (!HasSufficientTrust(announcement.hashGenesis))
        return false;
    
    return true;
}
```

## Reputation System

Pool reputation is calculated based on verifiable blockchain metrics and network uptime:

```cpp
struct PoolReputation {
    // Blockchain metrics (verifiable)
    uint32_t nBlocksFound;           // Lifetime blocks found by pool
    uint32_t nBlocksAccepted;        // Blocks accepted by network
    uint32_t nBlocksOrphaned;        // Orphaned blocks
    
    // Network metrics (reported)
    uint32_t nAnnouncementCount;     // How many announcements seen
    uint64_t nFirstSeen;             // When pool first appeared
    uint64_t nLastSeen;              // Last announcement timestamp
    uint32_t nDowntimeReports;       // How many times reported offline
};
```

### Reputation Score Calculation (0-100)

```
Base Score: 100

Deductions:
- Block acceptance rate < 95%: -20 points
- Block acceptance rate < 90%: -30 points (additional)
- Uptime < 95%: -10 points
- Uptime < 90%: -20 points (additional)
- Each downtime report: -5 points

Final Score: max(0, min(100, score))
```

## Endpoint Auto-Detection

Pool nodes automatically detect their public IP address:

**Priority Order:**
1. **Configured Value:** `-poolendpoint` command line argument
2. **Peer Consensus:** Ask connected peers what IP they see
3. **External Service:** Query external IP detection service (fallback)
4. **Local Network:** Use local network address (last resort)

```cpp
std::string DetectPublicEndpoint() {
    // Option 1: Use configured value
    if (config::HasArg("-poolendpoint"))
        return config::GetArg("-poolendpoint", "");
    
    // Option 2: Detect from peer connections
    std::string strPublicIP = GetPublicIPFromPeers();
    if (!strPublicIP.empty())
        return strPublicIP + ":" + std::to_string(nPort);
    
    // Option 3: Query external service (fallback)
    // Option 4: Use local network address (last resort)
}
```

## Security Measures

### Anti-Spam

- **Rate Limiting:** 1 announcement per hour per genesis
- **Trust Requirement:** Minimum 30 days trust to announce
- **Cache Limits:** Maximum 500 pools cached per node
- **TTL Expiration:** Announcements expire after 24 hours

### Anti-Sybil

- **Genesis Requirement:** Pool must be identified by on-chain genesis
- **Trust Scoring:** Only established accounts can announce pools
- **Signature Verification:** All announcements must be signed

### DoS Protection

- **Probabilistic Gossip:** Only 30% of peers re-broadcast
- **Cache Size Limits:** Enforce maximum cached pools
- **Automatic Cleanup:** Expired announcements removed automatically

## Configuration

### For Pool Operators (nexus.conf)

```conf
# Enable pool service
mining.pool.enabled=1

# Pool configuration
mining.pool.fee=2                    # 2% fee (0-5% allowed)
mining.pool.maxminers=500            # Max 500 concurrent miners
mining.pool.name=YourName Pool       # Human-readable name

# Endpoint (auto-detected if not set)
mining.pool.endpoint=123.45.67.89:9549

# Broadcasting
mining.pool.announce=1               # Enable announcements
mining.pool.announceinterval=3600    # Announce every hour
```

### For All Nodes

```conf
# Pool discovery
mining.discovery.enabled=1           # Enable pool discovery
mining.discovery.maxfee=5            # Only cache pools <= 5% fee
mining.discovery.minreputation=80    # Only cache pools >= 80 reputation
```

## Future Enhancements

### Planned Features

1. **Packet Types:** Define specific packet types for pool announcements (POOL_ANNOUNCE, POOL_QUERY, POOL_RESPONSE)
2. **Network Integration:** Full integration with Tritium peer-to-peer network
3. **Advanced Reputation:** Machine learning-based reputation scoring
4. **Geographic Discovery:** Location-based pool discovery for latency optimization
5. **Load Balancing:** Automatic miner distribution across pools

### Extensibility

The protocol is designed to be extensible:
- Additional metrics can be added to announcements
- New reputation factors can be incorporated
- Custom discovery algorithms can be implemented
- Third-party pool registries can be integrated

## Implementation Status

**Current Phase:** Core Infrastructure
- ✅ Pool announcement structures
- ✅ Broadcasting mechanism (stub)
- ✅ Validation rules
- ✅ Reputation calculation
- ✅ RPC endpoints
- ⏳ Network gossip integration (TODO)
- ⏳ Signature implementation (TODO)
- ⏳ Trust score validation (TODO)

## References

- [Pool Operator Guide](POOL_OPERATOR_GUIDE.md)
- [RPC Mining Endpoints](RPC_MINING_ENDPOINTS.md)
- [Mining Architecture](MINING_ARCHITECTURE.md)
