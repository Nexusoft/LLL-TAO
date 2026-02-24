# Stateless Mining Protocol - Node Implementation

## Overview

This document describes the node-side implementation of the stateless mining protocol introduced in LLL-TAO PR #170. This protocol enables miners to participate in the network without maintaining full blockchain state, using a push-based notification system with Falcon-1024 quantum-resistant authentication.

**Status:** Production (Active since v5.1.0)  
**Protocol Version:** 1.0  
**Date:** 2026-01-13

---

## Architecture

The stateless mining protocol implements a **push notification model** where the node proactively sends new block templates to miners whenever the blockchain advances, eliminating the need for miners to poll for updates.

### Core Design Principles

1. **Stateless Miners:** Miners don't need the full blockchain
2. **Stateful Nodes:** Nodes maintain blockchain and sign blocks with wallet keys
3. **Push > Poll:** Instant notifications instead of continuous polling
4. **Security Layers:** Falcon authentication for sessions, wallet signatures for consensus
5. **Channel Isolation:** Prime and Hash miners receive only relevant notifications

### Role Separation

```
┌─────────────────────────────────────────────────┐
│              MINER (Stateless)                  │
├─────────────────────────────────────────────────┤
│ • Falcon keys (session authentication)         │
│ • No blockchain required                        │
│ • Receives 216-byte templates                   │
│ • Performs Proof-of-Work                        │
│ • Submits solutions                             │
└─────────────────────────────────────────────────┘
                       ↕
┌─────────────────────────────────────────────────┐
│               NODE (Stateful)                   │
├─────────────────────────────────────────────────┤
│ • Full blockchain state                         │
│ • Wallet keys (block signing)                   │
│ • Creates 216-byte templates                    │
│ • Pushes templates when blockchain advances     │
│ • Validates submitted solutions                 │
│ • Routes rewards to miner addresses             │
└─────────────────────────────────────────────────┘
```

---

## Mining Server Initialization

### Startup Sequence

The mining server is initialized during node startup if the `mining=1` configuration parameter is set.

```cpp
// src/LLP/global.cpp
if(config::GetBoolArg("-mining", false) && !config::fClient.load())
{
    // Initialize mining server configuration
    LLP::MiningServerConfig CONFIG;
    CONFIG.ENABLE_DDOS     = config::GetBoolArg("-miningddos", false);
    CONFIG.ENABLE_SSL      = config::GetBoolArg("-miningssl", false);
    CONFIG.REQUIRE_SSL     = config::GetBoolArg("-miningsslrequired", false);
    CONFIG.MAX_THREADS     = config::GetArg("-miningthreads", 4);
    CONFIG.DDOS_CSCORE     = config::GetArg("-miningcscore", 1);
    CONFIG.DDOS_RSCORE     = config::GetArg("-miningrscore", 50);
    CONFIG.DDOS_TIMESPAN   = config::GetArg("-miningtimespan", 60);
    CONFIG.SOCKET_TIMEOUT  = config::GetArg("-miningtimeout", 30);
    
    // Start mining LLP server
    uint16_t nPort = LLP::GetMiningPort(); // 9323 mainnet and testnet
    MINING_SERVER = new LLP::Server<LLP::StatelessMinerConnection>(
        nPort, 
        CONFIG.MAX_THREADS,
        CONFIG.SOCKET_TIMEOUT,
        false  // Not using SSL by default
    );
    
    if(!MINING_SERVER->Start())
    {
        debug::error("Failed to start mining LLP server on port ", nPort);
        return false;
    }
    
    debug::log(0, "Mining LLP Server started on port ", nPort);
    debug::log(0, "Authentication: MINER-DRIVEN (Falcon keys from miners)");
    debug::log(0, "Reward Routing: STATELESS (via MINER_SET_REWARD)");
}
```

### Server Configuration

Key configuration parameters:
- **Port:** 9323 (mainnet and testnet)
- **Threads:** 4-16 recommended (configurable via `-miningthreads`)
- **Timeout:** 30 seconds default (configurable via `-miningtimeout`)
- **DDOS Protection:** Optional (enable with `-miningddos=1`)
- **SSL/TLS:** Optional (enable with `-miningssl=1`)

---

## Session Management

### Session Creation on MINER_AUTH

When a miner sends a MINER_AUTH packet, the node creates a new mining session:

```cpp
// src/LLP/stateless_miner_connection.cpp
void StatelessMinerConnection::ProcessAuth(const Packet& packet)
{
    // 1. Extract genesis hash (first 32 bytes)
    uint256_t hashGenesis;
    packet >> hashGenesis;
    
    // 2. Derive ChaCha20 session key from genesis
    std::vector<uint8_t> vSeed;
    vSeed.insert(vSeed.end(), 
                 (const uint8_t*)"nexus-mining-chacha20-v1", 24);
    vSeed.insert(vSeed.end(), hashGenesis.begin(), hashGenesis.end());
    uint256_t hashKey = LLC::SK256(vSeed);
    std::vector<uint8_t> vChaChaKey(hashKey.begin(), hashKey.begin() + 32);
    
    // 3. Extract and decrypt Falcon public key (if wrapped)
    uint16_t nPubkeyLen;
    packet >> nPubkeyLen;
    std::vector<uint8_t> vPubkey(nPubkeyLen);
    packet >> vPubkey;
    
    bool fRemote = !IsLocalhost(GetAddress());
    if(fRemote) {
        vPubkey = DecryptChaCha20(vPubkey, vChaChaKey);
    }
    
    // 4. Verify genesis exists in blockchain
    if(!LLD::Ledger->HasFirst(hashGenesis)) {
        SendAuthResponse(false); // Reject
        return;
    }
    
    // 5. Validate Falcon public key format
    LLC::FalconVersion nVersion = DetectFalconVersion(vPubkey);
    if(nVersion == LLC::FalconVersion::UNKNOWN) {
        SendAuthResponse(false); // Invalid key
        return;
    }
    
    // 6. Check optional whitelist
    if(config::HasArg("-minerallowkey")) {
        if(!IsKeyWhitelisted(vPubkey)) {
            SendAuthResponse(false); // Not whitelisted
            return;
        }
    }
    
    // 7. Create session entry in cache
    uint32_t nSessionId = LLC::GetRand(); // Random 32-bit ID
    MiningContext ctx;
    ctx.fAuthenticated = true;
    ctx.nSessionId = nSessionId;
    ctx.hashGenesis = hashGenesis;
    ctx.vMinerPubKey = vPubkey;
    ctx.nFalconVersion = nVersion;
    ctx.vChaChaKey = vChaChaKey;
    ctx.nSessionStart = runtime::unifiedtimestamp();
    ctx.nSessionTimeout = 86400; // 24 hours default
    
    SessionCache[nSessionId] = std::move(ctx);
    
    // 8. Send success response
    SendAuthResponse(true, nSessionId);
}
```

### Session Cache (24-hour Default)

Sessions are stored in a thread-safe cache with automatic expiry:

```cpp
// Session storage
std::map<uint32_t, MiningContext> SessionCache;
std::mutex SessionCacheMutex;

// Default session lifetime
const uint64_t DEFAULT_SESSION_TIMEOUT = 86400; // 24 hours

// Session cleanup (runs periodically)
void CleanupExpiredSessions()
{
    uint64_t nNow = runtime::unifiedtimestamp();
    std::lock_guard<std::mutex> lock(SessionCacheMutex);
    
    for(auto it = SessionCache.begin(); it != SessionCache.end(); )
    {
        if(it->second.IsSessionExpired(nNow)) {
            debug::log(2, "Expiring session ", it->first, 
                       " (age: ", it->second.GetSessionDuration(nNow), "s)");
            it = SessionCache.erase(it);
        }
        else {
            ++it;
        }
    }
}
```

### Keepalive Handling

Miners can extend session lifetime by sending periodic keepalive packets:

```cpp
void StatelessMinerConnection::ProcessSessionKeepalive(const Packet& packet)
{
    std::lock_guard<std::mutex> lock(SessionCacheMutex);
    
    if(!SessionCache.count(nSessionId)) {
        return; // Invalid session - no response
    }
    
    MiningContext& ctx = SessionCache[nSessionId];
    ctx.nTimestamp = runtime::unifiedtimestamp();
    ctx.nKeepaliveCount++;
    
    // Send acknowledgment
    SendPacket(Packet(Opcodes::StatelessMining::SESSION_KEEPALIVE, {0x01}));
}
```

### Session Cleanup on Disconnect

When a miner disconnects, the session is preserved in cache until timeout:

```cpp
void StatelessMinerConnection::OnDisconnect()
{
    // Don't immediately remove from SessionCache
    // Session will expire after timeout, allowing reconnection
    
    // Remove from push notification list
    std::lock_guard<std::mutex> lock(NotificationListMutex);
    PushNotificationList.erase(nSessionId);
    
    debug::log(1, "Miner disconnected (session ", nSessionId, 
               " preserved for ", GetSessionTimeRemaining(), "s)");
}
```

---

## Template Generation

### Block Template Creation

Templates are generated using the node's wallet keys to pre-sign blocks:

```cpp
TAO::Ledger::TritiumBlock* CreateNewBlock(uint32_t nChannel, 
                                          const uint256_t& hashReward)
{
    // 1. Get best blockchain state
    TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
    
    // 2. Create new block
    TAO::Ledger::TritiumBlock* pBlock = new TAO::Ledger::TritiumBlock();
    pBlock->nVersion = stateBest.nVersion;
    pBlock->nChannel = nChannel;
    pBlock->hashPrevBlock = stateBest.GetHash();  // MUST equal hashBestChain at acceptance

    // Get channel-specific state for the channel target height
    TAO::Ledger::BlockState stateChannel = stateBest;
    TAO::Ledger::GetLastState(stateChannel, nChannel);
    pBlock->nHeight = stateChannel.nChannelHeight + 1;  // channel target height, NOT unified height

    pBlock->nBits = GetNextWorkRequired(stateBest, nChannel);
    pBlock->nTime = runtime::unifiedtimestamp();
    pBlock->nNonce = 0; // Miner will fill this
    
    // 3. Set reward address (miner's configured address)
    pBlock->hashProducer = hashReward;
    
    // 4. Add coinbase transaction
    TAO::Ledger::Transaction txCoinbase;
    txCoinbase.nVersion = 1;
    txCoinbase.nTimestamp = pBlock->nTime;
    
    // Calculate block reward
    uint64_t nReward = GetBlockReward(pBlock->nHeight, nChannel);
    
    // Create coinbase output
    TAO::Operation::Contract contract;
    contract << (uint8_t)TAO::Operation::OP::COINBASE
             << hashReward
             << nReward;
    
    txCoinbase.vContracts.push_back(contract);
    pBlock->vTransactions.push_back(txCoinbase);
    
    // 5. Calculate merkle root
    pBlock->hashMerkleRoot = pBlock->BuildMerkleTree();
    
    // 6. Sign block with node wallet (producer signature)
    if(!SignBlockWithWallet(pBlock)) {
        delete pBlock;
        return nullptr;
    }
    
    return pBlock;
}
```

### Template Generation Performance

```cpp
// Performance tracking
struct TemplateMetrics
{
    uint64_t nTemplatesGenerated = 0;
    uint64_t nTotalGenerationTime = 0; // microseconds
    
    double GetAverageTime() const {
        return nTemplatesGenerated > 0 
            ? (double)nTotalGenerationTime / nTemplatesGenerated 
            : 0.0;
    }
};

TemplateMetrics g_TemplateMetrics;

void RecordTemplateGeneration(uint64_t nMicroseconds)
{
    g_TemplateMetrics.nTemplatesGenerated++;
    g_TemplateMetrics.nTotalGenerationTime += nMicroseconds;
    
    // Log if above target
    if(nMicroseconds > 1000) { // > 1ms
        debug::warning("Slow template generation: ", nMicroseconds, "μs");
    }
}
```

**Target Performance:**
- Template generation: <1ms
- Merkle root calculation: <0.5ms
- Wallet signing: <0.1ms
- **Total:** <1ms per template

---

## Push Notification Flow

### Subscription on MINER_READY

When a miner sends MINER_READY, it subscribes to push notifications:

```cpp
void StatelessMinerConnection::ProcessMinerReady(const Packet& packet)
{
    std::lock_guard<std::mutex> lock(SessionCacheMutex);
    
    if(!SessionCache.count(nSessionId)) {
        return; // Invalid session
    }
    
    MiningContext& ctx = SessionCache[nSessionId];
    
    // Validate authentication and configuration
    if(!ctx.fAuthenticated || ctx.nChannel == 0 || !ctx.fRewardBound) {
        debug::error("Miner not ready: auth=", ctx.fAuthenticated,
                     " channel=", ctx.nChannel,
                     " reward=", ctx.fRewardBound);
        return;
    }
    
    // Subscribe to push notifications
    ctx.fSubscribedToNotifications = true;
    ctx.nSubscribedChannel = ctx.nChannel;
    
    {
        std::lock_guard<std::mutex> lockNotif(NotificationListMutex);
        PushNotificationList.insert(nSessionId);
    }
    
    debug::log(1, "Miner ", nSessionId, " subscribed to ", 
               GetChannelName(ctx.nChannel), " notifications");
    
    // Send acknowledgment
    SendPacket(Packet(Opcodes::StatelessMining::MINER_READY, {0x01}));
    
    // Generate and send initial template
    SendInitialTemplate(ctx);
}
```

### Template Push on Blockchain Advance

When a new block is accepted to the blockchain:

```cpp
// src/TAO/Ledger/accept.cpp
bool AcceptBlock(TAO::Ledger::TritiumBlock& block)
{
    // ... block validation ...
    
    // Add block to chain
    if(!AddToBlockchain(block)) {
        return false;
    }
    
    // Trigger push notifications to miners
    if(MINING_SERVER) {
        MINING_SERVER->BroadcastNewBlock(block.nChannel);
    }
    
    return true;
}

// src/LLP/miner.cpp
void MiningServer::BroadcastNewBlock(uint32_t nChannel)
{
    uint64_t nStart = runtime::microseconds();
    uint32_t nCount = 0;
    
    std::lock_guard<std::mutex> lock(SessionCacheMutex);
    
    for(auto& [session_id, ctx] : SessionCache)
    {
        // Skip unsubscribed miners
        if(!ctx.fSubscribedToNotifications)
            continue;
        
        // Channel isolation: only notify miners on this channel
        if(ctx.nSubscribedChannel != nChannel)
            continue;
        
        // Generate channel-specific template
        TAO::Ledger::TritiumBlock* pBlock = 
            CreateNewBlock(nChannel, ctx.GetPayoutAddress());
        
        if(!pBlock) {
            debug::error("Failed to create template for session ", session_id);
            continue;
        }
        
        // Send NEW_BLOCK notification
        SendNewBlock(session_id, pBlock);
        
        // Update statistics
        ctx.nNotificationsSent++;
        ctx.nLastNotificationTime = runtime::unifiedtimestamp();
        nCount++;
        
        delete pBlock;
    }
    
    uint64_t nElapsed = runtime::microseconds() - nStart;
    debug::log(1, "Broadcasted NEW_BLOCK to ", nCount, " miners in ",
               nElapsed / 1000.0, "ms (channel ", nChannel, ")");
    
    // Alert if above target
    if(nElapsed > 10000) { // > 10ms
        debug::warning("Slow NEW_BLOCK broadcast: ", nElapsed / 1000.0, "ms");
    }
}
```

### Channel-Specific Height Tracking and Best-Tip Anchoring

The protocol uses two independent staleness axes. A template is stale for two distinct reasons:

1. **`tip_moved`** — The unified best-chain tip advanced (any channel produced a block), so
   `hashBestChain` changed. Any block submitted with the old `hashPrevBlock` will be rejected
   by `Block::Accept()`.

2. **`channel_advanced`** — A new block appeared in the miner's specific channel, so the
   expected `nChannelHeight` target has changed.

`channel_advanced` always implies `tip_moved`, but `tip_moved` does **not** imply
`channel_advanced`. Miners must refresh templates on `tip_moved` even if their channel has
not yet produced a new block.

```cpp
// Calculate channel height for template (channel target height)
uint32_t GetChannelTargetHeight(uint32_t nChannel)
{
    TAO::Ledger::BlockState stateChannel = TAO::Ledger::ChainState::tStateBest.load();
    TAO::Ledger::GetLastState(stateChannel, nChannel);
    return stateChannel.nChannelHeight + 1;  // channel target, not unified height
}

// Template metadata includes both axes for staleness detection
struct TemplateMetadata
{
    std::unique_ptr<TAO::Ledger::Block> pBlock;
    uint64_t nCreationTime;
    uint32_t nChannelTarget;   // channel_target at template creation (pBlock->nHeight)
    uint256_t hashPrev;        // hashBestChain at template creation (pBlock->hashPrevBlock)
    uint32_t nChannel;

    // Two-axis staleness check
    bool IsStale(uint64_t nNow = 0) const
    {
        if(nNow == 0)
            nNow = runtime::unifiedtimestamp();

        // Age-based timeout (60 seconds)
        if(nNow - nCreationTime > 60)
            return true;

        // Axis 1: tip_moved — authoritative anchoring check
        if(TAO::Ledger::ChainState::hashBestChain.load() != hashPrev)
            return true;  // tip_moved: hashPrevBlock no longer points to best tip

        // Axis 2: channel_advanced — channel sequence check
        TAO::Ledger::BlockState stateChannel = TAO::Ledger::ChainState::tStateBest.load();
        TAO::Ledger::GetLastState(stateChannel, nChannel);
        if(stateChannel.nChannelHeight + 1 != nChannelTarget)
            return true;  // channel_advanced: channel height has moved

        return false;
    }
};
```

---

## Block Submission Handling

### Validation Flow on SUBMIT_BLOCK

```cpp
void StatelessMinerConnection::ProcessSubmitBlock(const Packet& packet)
{
    uint64_t nStart = runtime::microseconds();
    
    // 1. Decrypt packet if encrypted
    MiningContext& ctx = SessionCache[nSessionId];
    Packet decrypted = packet;
    if(ctx.fEncryptionReady) {
        decrypted = DecryptPacket(packet, ctx.vChaChaKey);
    }
    
    // 2. Extract block
    TAO::Ledger::TritiumBlock block;
    decrypted >> block;
    
    // 3. Validate proof-of-work
    uint1024_t hashProof = block.GetHash();
    if(hashProof > block.nBits) {
        SendBlockRejected("Invalid proof-of-work");
        RecordRejection("pow_invalid");
        return;
    }
    
    // 4. Check if block is based on current best chain (tip_moved check)
    if(block.hashPrevBlock != TAO::Ledger::ChainState::hashBestChain.load()) {
        SendBlockRejected("Stale template: tip_moved");
        RecordRejection("stale_tip");
        return;
    }
    
    // 5. Extract optional Physical Falcon signature
    bool fHasPhysical;
    decrypted >> fHasPhysical;
    
    if(fHasPhysical) {
        uint16_t nSigLen;
        decrypted >> nSigLen;
        
        std::vector<uint8_t> vPhysicalSig(nSigLen);
        decrypted >> vPhysicalSig;
        
        // Verify signature
        if(!VerifyFalconSignature(vPhysicalSig, ctx.vMinerPubKey, 
                                   block.GetHash())) {
            SendBlockRejected("Invalid Falcon signature");
            RecordRejection("signature_invalid");
            return;
        }
        
        // Store signature on block
        block.vchBlockSig = vPhysicalSig;
    }
    
    // 6. Validate block structure
    if(!block.Check()) {
        SendBlockRejected("Invalid block structure");
        RecordRejection("structure_invalid");
        return;
    }
    
    // 7. Validate transactions
    if(!block.CheckTransactions()) {
        SendBlockRejected("Invalid transactions");
        RecordRejection("transactions_invalid");
        return;
    }
    
    // 8. Accept block to blockchain
    if(!TAO::Ledger::AcceptBlock(block)) {
        SendBlockRejected("Block rejected by consensus");
        RecordRejection("consensus_rejected");
        return;
    }
    
    uint64_t nElapsed = runtime::microseconds() - nStart;
    
    // 9. Send acceptance
    SendBlockAccepted(block.GetHash(), block.nHeight);
    
    // 10. Broadcast NEW_BLOCK to all miners
    BroadcastNewBlock(block.nChannel);
    
    // Log successful submission
    debug::log(0, "Block accepted from miner ", nSessionId,
               " height=", block.nHeight,
               " channel=", GetChannelName(block.nChannel),
               " validation_time=", nElapsed / 1000.0, "ms");
    
    RecordAcceptance(nElapsed);
}
```

---

## Performance Metrics

### Real-World Measurements

| Metric | Target | Typical | 95th Percentile |
|--------|--------|---------|-----------------|
| GET_BLOCK response | <5ms | 2-3ms | 4ms |
| NEW_BLOCK push | <10ms | 5-8ms | 9ms |
| Template generation | <1ms | 0.5ms | 0.8ms |
| SUBMIT_BLOCK validation | <50ms | 20-30ms | 45ms |
| Session lookup | <0.1ms | <0.05ms | 0.08ms |
| Falcon signature verify | <5ms | 2ms | 4ms |

### Throughput Capacity

**Single Node:**
- Concurrent miners: 100-500
- Templates/second: 1000+
- Submissions/second: 50-100
- Push notifications/second: 1000+

**Resource Usage (100 miners):**
- CPU: 2-4 cores
- RAM: 256-512 MB
- Network: <10 Mbps

---

## Cross-References

**Miner Perspective:**
- [NexusMiner Stateless Protocol](https://github.com/Nexusoft/NexusMiner/blob/main/docs/current/mining-protocols/stateless-mining.md)
- [NexusMiner Auto-Negotiation](https://github.com/Nexusoft/NexusMiner/blob/main/docs/upgrade-guides/legacy-to-stateless.md)

**Node Documentation:**
- [Mining Server Architecture](mining-server.md)
- [Mining Lanes Cheat Sheet](mining-lanes-cheat-sheet.md)
- [Protocol Lanes](../../protocol/mining-protocol.md)
- [Unified Tip and Channel Heights](unified-tip-and-channel-heights.md) — Canonical semantics reference
- [Push Notification Flow Diagram](../../diagrams/push-notification-flow.md)
- [Opcodes Reference](../../reference/opcodes-reference.md)
- [Configuration Reference](../../reference/nexus.conf.md)

---

## Version Information

**Document Version:** 1.0  
**Protocol Version:** Stateless Mining 1.0  
**LLL-TAO Version:** 5.1.0+  
**PR Reference:** LLL-TAO PR #170  
**Last Updated:** 2026-01-13

---

## Push Throttle and AutoCoolDown

With PR #278 merged, every unified-tip advance triggers a template push to
**both** PoW channels regardless of which channel mined the block.  Two
mechanisms protect the network against push floods and miner polling abuse.

### 1-Second Push Throttle (`TEMPLATE_PUSH_MIN_INTERVAL_MS`)

`SendStatelessTemplate()` and `SendChannelNotification()` (both lanes) check
`m_last_template_push_time` under the connection mutex before transmitting.
If the elapsed time since the last send is less than 1 000 ms the push is
dropped (logged at level 1 so operators can observe throttle events).

Re-subscription responses (from `STATELESS_MINER_READY` / `MINER_READY`
handlers) bypass the throttle entirely via the `m_force_next_push` flag,
ensuring a miner always gets fresh work immediately after re-subscribing
regardless of when the previous push was sent.  This closes the doom-loop
window where a miner could mine a stale template for up to 200 seconds.

**Rationale:**  During a fork-resolution burst the node can fire 5–10
`SetBest()` events in < 100 ms.  Flooding a miner with 10 full 228-byte
templates causes retransmission waste and re-parse churn.  The 1-second floor
is still well above any fork-resolution burst window (~100 ms) and below any
real block-time floor, so miners always get a fresh template within 1 s of the
tip stabilising.

### 200-Second GET_BLOCK Safety-Net (`AutoCoolDown`, `GET_BLOCK_COOLDOWN_SECONDS`)

`LLP::AutoCoolDown m_get_block_cooldown{std::chrono::seconds(200)}` is held
per connection on both `StatelessMinerConnection` and `Miner`.  Because the
node now pushes templates on every tip advance, miners should almost never
need to poll with `GET_BLOCK`.  The 200-second cooldown is a **last-resort
safety net** for lost connections:

- Capped reconnect latency: 200 s instead of 300 s in the worst case.
- Still well above the node's 10 s minimum between repeat requests (avoids
  the hard DDOS ban).

The old 300-second strategy was calibrated for polling miners.  With push now
the norm, 200 s is the correct ceiling.

### Class `LLP::AutoCoolDown`

A new header-only helper defined in `src/LLP/include/auto_cooldown.h`.
It replaces ad-hoc magic-number cooldown comments with a self-contained
object:

```cpp
AutoCoolDown cd(std::chrono::seconds(200));
if (!cd.Ready()) return;   // still cooling down
cd.Reset();                // start new cooldown
```

**Cross-reference:** See [push-refresh-loop.md](push-refresh-loop.md) for
the full five-diagram sequence showing how the throttle and cooldown interact
with `BlockState::SetBest()`.
