# Mining Debug Cheat Sheet

Common mining issues with diagnostic steps and AI-assisted resolution prompts.

---

## Connection Issues

### Miner Cannot Connect
**Symptoms:** Connection refused or timeout

**Diagnostic Steps:**
1. Verify node is running: `ps aux | grep nexus`
2. Check port availability: `netstat -tlnp | grep 9323`
3. Review node logs for connection errors

**AI Prompt:** "What are the common reasons a miner cannot connect to a Nexus node on port 9323?"

### Authentication Failure
**Symptoms:** MINER_AUTH rejected, AUTH_RESULT with error

**Diagnostic Steps:**
1. Verify Falcon key pair is valid
2. Check timestamp is within acceptable range (replay protection)
3. Verify GenesisHash ownership on blockchain
4. Check if session cache is full (500 entry limit)

**AI Prompt:** "Trace the Falcon handshake validation in `falcon_handshake.h` - what can cause AUTH to fail?"

---

## Block Template Issues

### No Block Available
**Symptoms:** No PRIME_BLOCK_AVAILABLE or HASH_BLOCK_AVAILABLE notifications

**Diagnostic Steps:**
1. Verify miner sent MINER_READY after authentication
2. Check SET_CHANNEL was acknowledged with CHANNEL_ACK
3. Verify blockchain is synced

**AI Prompt:** "What conditions must be met before a node sends push notifications to a miner?"

### Stale Templates
**Symptoms:** Mining on outdated block templates

**Diagnostic Steps:**
1. Check push notification delivery timing
2. Verify network connectivity for new block events
3. Review template refresh interval

**AI Prompt:** "How does the node decide when to send a new PRIME_BLOCK_AVAILABLE notification?"

---

## Block Submission Issues

### Block Rejected
**Symptoms:** BLOCK_REJECTED response after SUBMIT_BLOCK

**Common Causes:**
- Stale template (new block arrived during mining)
- Invalid proof of work (hash doesn't meet difficulty)
- Invalid prime chain (insufficient cluster size)
- Signature verification failure

**AI Prompt:** "What validation checks are performed when a SUBMIT_BLOCK is received?"

### Block Accepted but No Reward
**Symptoms:** BLOCK_ACCEPTED but no balance change

**Diagnostic Steps:**
1. Verify MINER_SET_REWARD was configured correctly
2. Check reward address GenesisHash
3. Verify coinbase transaction was included

**AI Prompt:** "How does the mining reward payout work after a block is accepted?"

---

## Performance Issues

### High Latency Responses
**Symptoms:** Slow template delivery or notification delays

**Diagnostic Steps:**
1. Check node CPU and memory usage
2. Review thread pool size configuration
3. Monitor network latency to peers

**AI Prompt:** "What configuration options affect mining server performance?"

### Frequent Disconnections
**Symptoms:** Session drops requiring re-authentication

**Diagnostic Steps:**
1. Check keep-alive ping interval (24h requirement)
2. Review session cache purge settings
3. Verify network stability

**AI Prompt:** "What are the session timeout and purge rules for mining connections?"

---

## Configuration Reference

```bash
# Mining pool mode
-miningpool=1

# Falcon handshake timeout (seconds)
-falconhandshake.timeout=60

# Require encryption for all miners
-falconhandshake.requireencryption=1

# Cache purge interval (seconds, default: 7 days)
-nodecache.purgetimeout=604800
```

---

## Cross-References

- [Mining Flow Diagram](../../diagrams/architecture/mining-flow-complete.md)
- [Falcon Auth Sequence](../../diagrams/protocols/falcon-auth-sequence.md)
- [Mining Server Docs](../../current/mining/mining-server.md)
