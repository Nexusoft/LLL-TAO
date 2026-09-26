# Pool Operator Guide

## Introduction

This guide explains how to set up and operate a decentralized mining pool on the Nexus network using the Pool Discovery Protocol.

## Prerequisites

### System Requirements

- **Hardware:**
  - 4+ CPU cores
  - 8+ GB RAM
  - 100+ GB SSD storage
  - Reliable internet connection

- **Software:**
  - Nexus Core daemon (latest version)
  - Open ports: 9549 (mining), 9888 (P2P)

### Account Requirements

- **Genesis Account:** Active Nexus genesis account with credentials
- **Trust Score:** Minimum 30 days trust (recommended for announcement)
- **Stake:** Optional NXS stake for enhanced reputation

## Quick Start

### 1. Configure nexus.conf

Create or edit your `~/.Nexus/nexus.conf`:

```conf
# Enable mining server
mining=1
miningport=9549

# Enable pool service
mining.pool.enabled=1
mining.pool.fee=2                    # 2% fee (0-5% allowed)
mining.pool.maxminers=500            # Max concurrent miners
mining.pool.name=My Nexus Pool       # Human-readable name

# Optional: Specify public endpoint (auto-detected if omitted)
# mining.pool.endpoint=YOUR_PUBLIC_IP:9549

# Enable pool discovery
mining.discovery.enabled=1
```

### 2. Start the Daemon

```bash
./nexus
```

The daemon will:
- Start the mining server on port 9549
- Auto-detect your public IP address
- Begin accepting miner connections

### 3. Start Pool Broadcasting

Use the RPC command to start broadcasting your pool:

```bash
# Example using curl
curl -X POST http://localhost:8080/mining/start/pool \
  -H "Content-Type: application/json" \
  -d '{
    "fee": 2,
    "maxminers": 500,
    "name": "My Nexus Pool",
    "genesis": "YOUR_GENESIS_HASH"
  }'
```

**Response:**
```json
{
  "success": true,
  "message": "Pool service started",
  "genesis": "8abc123def...",
  "endpoint": "123.45.67.89:9549",
  "fee": 2,
  "maxminers": 500,
  "name": "My Nexus Pool"
}
```

### 4. Verify Pool Status

Check your pool's status:

```bash
curl http://localhost:8080/mining/get/poolstats
```

**Response:**
```json
{
  "enabled": true,
  "genesis": "8abc123def...",
  "endpoint": "123.45.67.89:9549",
  "fee": 2,
  "maxminers": 500,
  "connectedminers": 0,
  "authenticatedminers": 0,
  "rewardboundminers": 0,
  "totalhashrate": 0,
  "blocksfound": 0,
  "uptime": 1.0,
  "name": "My Nexus Pool"
}
```

## RPC Commands

### Start Pool Service

**Endpoint:** `mining/start/pool`

**Parameters:**
```json
{
  "fee": 2,                          // Required: Pool fee (0-5%)
  "maxminers": 500,                  // Optional: Max miners (default 500)
  "name": "My Pool",                 // Optional: Pool name
  "genesis": "YOUR_GENESIS_HASH"     // Required: Your genesis hash
}
```

### Stop Pool Service

**Endpoint:** `mining/stop/pool`

**Parameters:** None

**Response:**
```json
{
  "success": true,
  "message": "Pool service stopped"
}
```

### Get Pool Statistics

**Endpoint:** `mining/get/poolstats`

**Returns:** Current pool metrics including connected miners, hashrate, blocks found

### Get Connected Miners

**Endpoint:** `mining/get/connectedminers`

**Returns:** List of all currently connected miners with their statistics

## Pool Configuration

### Fee Structure

**Allowed Range:** 0-5%

```conf
mining.pool.fee=2  # 2% pool fee
```

**Fee Enforcement:**
- Hard capped at 5% by protocol
- Announcements with fees >5% are rejected
- Fee changes require pool restart

### Miner Limits

**Configure maximum concurrent miners:**

```conf
mining.pool.maxminers=500
```

**Recommendations:**
- Start with 100-200 for testing
- Scale up based on server capacity
- Monitor CPU and network usage

### Endpoint Configuration

**Auto-detection (recommended):**
```conf
# Omit mining.pool.endpoint - daemon will auto-detect
```

**Manual configuration:**
```conf
mining.pool.endpoint=123.45.67.89:9549
```

**Verify endpoint is reachable:**
```bash
# From external machine
telnet YOUR_PUBLIC_IP 9549
```

## Network Setup

### Firewall Configuration

**Required open ports:**

```bash
# Mining port (TCP)
ufw allow 9549/tcp

# P2P network port (TCP)
ufw allow 9888/tcp

# API port (TCP, if exposing API)
ufw allow 8080/tcp
```

### NAT/Router Setup

If running behind NAT:

1. **Port Forwarding:**
   - Forward external port 9549 → internal IP:9549
   - Forward external port 9888 → internal IP:9888

2. **Static IP:**
   - Configure static local IP on pool server
   - Update router DHCP reservation

3. **Dynamic DNS (optional):**
   - Set up DDNS if using dynamic WAN IP
   - Update `mining.pool.endpoint` with DDNS hostname

## Monitoring

### Real-Time Statistics

**Check pool stats periodically:**
```bash
watch -n 10 'curl -s http://localhost:8080/mining/get/poolstats | jq'
```

**Monitor connected miners:**
```bash
watch -n 30 'curl -s http://localhost:8080/mining/get/connectedminers | jq'
```

### Log Monitoring

**Pool-related log entries:**
```bash
tail -f ~/.Nexus/debug.log | grep -i "pool\|mining"
```

**Look for:**
- Pool announcement broadcasts
- Miner connections/disconnections
- Block submissions
- Fee calculations
- Validation errors

### Health Checks

**Automated monitoring script:**
```bash
#!/bin/bash
# Check if pool is running and accepting connections
nc -zv YOUR_PUBLIC_IP 9549
if [ $? -eq 0 ]; then
    echo "Pool is reachable"
else
    echo "WARNING: Pool unreachable!"
fi
```

## Miner Management

### Accepting Miners

Miners connect using Falcon authentication:

1. Miner sends `MINER_AUTH_INIT` with genesis and Falcon public key
2. Pool responds with `MINER_AUTH_CHALLENGE` containing nonce
3. Miner signs nonce and sends `MINER_AUTH_RESPONSE`
4. Pool validates signature and sends `MINER_AUTH_RESULT`

**No manual miner approval required** - all authenticated miners are accepted (up to `maxminers`).

### Reward Distribution

**Current Implementation:**
- Miners set their own reward address via `MINER_SET_REWARD` packet
- Pool tracks reward addresses in `MiningContext`
- Block rewards automatically routed to miner's specified address

**Future Enhancement:**
- Proportional reward distribution based on shares
- Pool operator fee deduction
- Payment scheduling and batching

## Troubleshooting

### Pool Not Discoverable

**Problem:** Miners can't find your pool

**Solutions:**
1. Verify pool broadcasting is started: `mining/get/poolstats`
2. Check genesis hash is correct
3. Confirm fee is ≤ 5%
4. Wait up to 1 hour for announcement propagation
5. Verify sufficient trust score (30+ days)

### Miners Can't Connect

**Problem:** Miners timeout when connecting

**Solutions:**
1. Check firewall allows port 9549
2. Verify NAT/port forwarding configured
3. Test external connectivity: `telnet YOUR_IP 9549`
4. Review `debug.log` for connection errors
5. Confirm `maxminers` limit not reached

### Low Reputation Score

**Problem:** Pool has poor reputation

**Solutions:**
1. Improve block acceptance rate (reduce orphans)
2. Maintain consistent uptime (broadcast regularly)
3. Build trust over time (30+ day minimum)
4. Avoid downtime reports by ensuring availability

### High Fee Rejected

**Problem:** Can't set pool fee above 5%

**This is intentional:**
- Protocol enforces 5% hard cap
- Prevents exploitative fee structures
- Encourages competitive fee market
- Announcements with fees >5% are rejected network-wide

## Best Practices

### Security

1. **Credentials:** Secure your genesis credentials
2. **Firewall:** Only open necessary ports
3. **Updates:** Keep Nexus Core up to date
4. **Backups:** Regular backups of wallet and config
5. **Monitoring:** Set up alerts for downtime

### Performance

1. **Hardware:** Use SSD storage for fast I/O
2. **Bandwidth:** Ensure adequate upload bandwidth
3. **Connectivity:** Stable internet connection required
4. **Resources:** Monitor CPU/RAM usage under load
5. **Scaling:** Increase `maxminers` gradually

### Reliability

1. **Uptime:** Maintain 99%+ uptime for good reputation
2. **Announcements:** Broadcast hourly (automatic)
3. **Monitoring:** Use automated health checks
4. **Redundancy:** Consider failover systems
5. **Documentation:** Keep operational records

## Fee Recommendations

### Competitive Fees

Market research suggests:
- **0-1%:** Highly competitive, attracts miners
- **1-2%:** Standard rate, sustainable operations
- **2-3%:** Higher margin, premium service
- **3-5%:** Maximum range, must justify value

### Fee Calculation

Pool fees are automatically deducted from block rewards:

```
Miner Reward = Block Reward × (1 - Fee Percentage)
Pool Revenue = Block Reward × Fee Percentage
```

Example (2% fee, 1 NXS block reward):
- Miner receives: 0.98 NXS
- Pool receives: 0.02 NXS

## Support

### Resources

- **Documentation:** `/docs` directory
- **RPC Reference:** [RPC_MINING_ENDPOINTS.md](RPC_MINING_ENDPOINTS.md)
- **Protocol Spec:** [POOL_DISCOVERY_PROTOCOL.md](POOL_DISCOVERY_PROTOCOL.md)

### Community

- **Discord:** Nexus community channels
- **Forum:** Official Nexus forums
- **GitHub:** Report issues and contribute

## Changelog

### Version 1.0 (Current)
- Initial pool discovery protocol implementation
- RPC endpoints for pool management
- Automatic endpoint detection
- Basic reputation system
- Fee enforcement (0-5% cap)

### Planned Features
- Advanced reputation metrics
- Geographic pool discovery
- Load balancing
- Pool statistics dashboard
- Payment scheduling system
