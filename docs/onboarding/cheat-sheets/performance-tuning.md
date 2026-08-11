# Performance Tuning Cheat Sheet

Optimization patterns and configuration for Nexus node and mining performance.

---

## Mining Server Tuning

### Thread Pool Configuration
The mining server uses a configurable thread pool for handling miner connections.

**Key Parameters:**
- Worker thread count (default: 4)
- Connection queue depth
- Session cache size (max: 500)

**AI Prompt:** "What thread pool configuration options exist for the mining server?"

### Push Notification Optimization
Push notifications (12 bytes) are broadcast to all subscribed miners when a new block arrives.

**Optimization Patterns:**
- Minimize time between block acceptance and notification
- Use async broadcast thread to avoid blocking validation
- Batch notifications when multiple miners are on same connection

**AI Prompt:** "How does the push notification broadcaster handle multiple subscribed miners?"

---

## Network Performance

### Peer Connection Management
Trust-scored peers receive priority connections, improving network reliability.

```
Trust Score Weights:
  Connected:  100.0  (most impactful)
  Session:     80.0
  Latency:     10.0
  Failed:     -20.0
  Fails:      -10.0
  Dropped:     -5.0
```

**Optimization:** Focus on maintaining stable connections to high-trust peers.

**AI Prompt:** "How does the trust scoring system prioritize peer connections?"

### Latency Reduction
- Use geographic peer selection when possible
- Monitor peer latency metrics in trust score
- Configure appropriate connection timeouts

---

## Block Validation Performance

### Validation Pipeline
The block validation pipeline runs sequentially. Each stage must pass before the next.

**Bottleneck Areas:**
1. **Signature verification** - CPU-intensive, especially with Falcon
2. **Merkle tree computation** - Scales with transaction count
3. **State lookups** - Database I/O for register state verification

**Optimization Patterns:**
- Pre-validate transactions as they enter mempool
- Cache frequently accessed register states
- Parallel signature verification where possible

**AI Prompt:** "What are the most CPU-intensive parts of block validation?"

---

## Memory Management

### Session Cache
- Maximum 500 entries for DDOS protection
- Localhost sessions persist up to 30 days
- Remote sessions purge after 7 days
- Keep-alive ping required every 24 hours

**AI Prompt:** "How can I monitor session cache usage and identify memory pressure?"

### Register State Cache
- Frequently accessed registers benefit from caching
- PRESTATE/POSTSTATE pairs are held during operation execution
- Committed states are written to database

---

## Configuration Quick Reference

```bash
# Mining performance
-miningpool=1                          # Enable pool mode
-mining.threads=4                      # Worker thread count

# Network performance
-falconhandshake.timeout=60            # Auth timeout (seconds)
-nodecache.purgetimeout=604800         # Cache purge (7 days)

# Connection limits
-maxconnections=125                    # Max peer connections
```

---

## Monitoring Checklist

- [ ] CPU usage during block validation
- [ ] Memory usage of session cache
- [ ] Network latency to top peers
- [ ] Push notification delivery time
- [ ] Block template generation time
- [ ] Transaction verification throughput

---

## Cross-References

- [Trust Network Topology](../../diagrams/architecture/trust-network-topology.md)
- [Mining Flow](../../diagrams/architecture/mining-flow-complete.md)
- [Consensus Validation](../../diagrams/architecture/consensus-validation-flow.md)
- [Mining Debug](mining-debug.md)
