# LLP Protocol Opcodes Reference - Node Perspective

## Table of Contents

1. [Overview](#overview)
2. [Opcode Allocation Strategy](#opcode-allocation-strategy)
3. [Stateless Mining Opcodes](#stateless-mining-opcodes-0xd000---0xdfff)
4. [Authentication Flow](#authentication-flow)
5. [Configuration Flow](#configuration-flow)
6. [Template Delivery Flow](#template-delivery-flow)
7. [Solution Submission Flow](#solution-submission-flow)
8. [Performance Metrics](#performance-metrics)
9. [Error Handling](#error-handling)
10. [Cross-References](#cross-references)

---

## Overview

This document provides a comprehensive reference for all LLP (Lower Level Protocol) opcodes from the **node perspective**. It focuses on how the Nexus node processes incoming opcodes and generates outgoing responses.

**Protocol Version:** Stateless Mining 1.0 (LLL-TAO 5.1.0+)  
**Architecture:** Push-based notification system with Falcon-1024 authentication  
**Document Version:** 1.0  
**Last Updated:** 2026-01-13

---

## Opcode Allocation Strategy

Nexus uses a 16-bit opcode space (0x0000 - 0xFFFF = 65,536 possible codes) divided into protocol-specific ranges:

| Range | Protocol | Purpose |
|-------|----------|---------|
| `0x0000 - 0x0FFF` | Legacy Mining | Original pool mining protocol (backward compatibility) |
| `0x1000 - 0x1FFF` | Tritium | Blockchain synchronization and P2P communication |
| `0x2000 - 0x2FFF` | Time Sync | Network time protocol for timestamp consensus |
| `0xD000 - 0xDFFF` | **Stateless Mining** | **Production protocol (this document)** |
| `0xE000 - 0xEFFF` | Pool Mining | Reserved for future enhancements |
| `0xF000 - 0xFFFF` | Reserved | Future protocols |

**Why This Matters:**
- Prevents accidental opcode reuse across protocols
- Makes protocol boundaries clear at a glance
- Self-documents the entire LLP architecture
- Simplifies debugging (opcode reveals which protocol)

---

## Stateless Mining Opcodes (0xD000 - 0xDFFF)

### Protocol Flow Summary

```
┌─────────────────────────────────────────────────────────────────┐
│                   STATELESS MINING PROTOCOL                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  Phase 1: Authentication (Falcon-1024)                          │
│  ──────────────────────────────────────────                     │
│  Miner → Node:   MINER_AUTH (0xD000)                            │
│  Node → Miner:   MINER_AUTH_RESPONSE (0xD001)                   │
│                                                                   │
│  Phase 2: Configuration                                          │
│  ──────────────────────────                                      │
│  Miner → Node:   MINER_SET_REWARD (0xD003)                      │
│  Node → Miner:   MINER_REWARD_RESULT (0xD004)                   │
│  Miner → Node:   SET_CHANNEL (0xD005)                           │
│  Node → Miner:   CHANNEL_ACK (0xD006)                           │
│                                                                   │
│  Phase 3: Subscription (Push Notifications)                     │
│  ───────────────────────────────────────────                    │
│  Miner → Node:   MINER_READY (0xD007)                           │
│  Node → Miner:   MINER_READY (0xD007) [1-byte ACK]              │
│  Node → Miner:   GET_BLOCK (0xD008) [Initial 216-byte template] │
│                                                                   │
│  Phase 4: Mining (Push-Based Updates)                           │
│  ──────────────────────────────────────                         │
│  [Blockchain advances - new block mined]                        │
│  Node → Miner:   NEW_BLOCK (0xD009) [Updated 216-byte template] │
│  [Miner works on new template...]                               │
│                                                                   │
│  Phase 5: Solution Submission                                    │
│  ─────────────────────────────────                              │
│  [Miner finds valid nonce]                                      │
│  Miner → Node:   SUBMIT_BLOCK (0xD00A)                          │
│  Node → Miner:   BLOCK_ACCEPTED (0xD00B) or BLOCK_REJECTED (0xD00C) │
│  Node → All:     NEW_BLOCK (0xD009) [Notify all miners]         │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

---

## Authentication Flow

### MINER_AUTH (0xD000)

**Direction:** Miner → Node  
**Handler:** `StatelessMinerConnection::ProcessAuth()`  
**Source File:** `src/LLP/stateless_miner_connection.cpp`

**Description:**  
Initiates Falcon-1024 authentication for quantum-resistant session establishment. The miner sends its genesis hash and Falcon public key (optionally encrypted with ChaCha20).

**Packet Structure:**

```
┌────────────────────────────────────────────────────────┐
│ MINER_AUTH Packet (0xD000)                             │
├────────────────────────────────────────────────────────┤
│ Offset │ Field                │ Size        │ Type     │
├────────┼──────────────────────┼─────────────┼──────────┤
│ 0-31   │ Genesis Hash         │ 32 bytes    │ uint256  │
│ 32-33  │ Pubkey Length        │ 2 bytes     │ uint16   │
│ 34-x   │ Falcon Public Key    │ 897/1793 B  │ bytes    │
│ x+1    │ Miner ID Length      │ 2 bytes     │ uint16   │
│ x+3    │ Miner ID String      │ variable    │ UTF-8    │
└────────────────────────────────────────────────────────┘

Total Size: Variable (46+ bytes base + pubkey + ID)
```

**Falcon Public Key Sizes:**
- Falcon-512: 897 bytes
- Falcon-1024: 1793 bytes

**Node Processing Steps:**

1. **Extract Genesis Hash** (first 32 bytes)
   ```cpp
   uint256_t hashGenesis;
   PACKET.Extract(hashGenesis);
   ```

2. **Derive ChaCha20 Session Key**
   ```cpp
   std::vector<uint8_t> vSeed = "nexus-mining-chacha20-v1" + genesis_bytes;
   uint256_t hashKey = LLC::SK256(vSeed);
   vChaChaKey = std::vector<uint8_t>(hashKey.begin(), hashKey.begin() + 32);
   ```

3. **Extract and Decrypt Falcon Public Key** (if wrapped)
   ```cpp
   uint16_t nPubkeyLen;
   PACKET.Extract(nPubkeyLen);
   std::vector<uint8_t> vPubkey(nPubkeyLen);
   PACKET.Extract(vPubkey);
   
   // If encrypted, decrypt with ChaCha20
   if(fRemoteConnection) {
       vPubkey = DecryptChaCha20(vPubkey, vChaChaKey);
   }
   ```

4. **Detect Falcon Version**
   ```cpp
   LLC::FalconVersion nVersion = LLC::FalconVersion::UNKNOWN;
   if(vPubkey.size() == 897)
       nVersion = LLC::FalconVersion::FALCON512;
   else if(vPubkey.size() == 1793)
       nVersion = LLC::FalconVersion::FALCON1024;
   ```

5. **Verify Genesis Exists on Blockchain**
   ```cpp
   if(!LLD::Ledger->HasFirst(hashGenesis)) {
       return SendResponse(MINER_AUTH_RESPONSE, {0x00}); // Failure
   }
   ```

6. **Validate Falcon Public Key Format**
   ```cpp
   if(nVersion == LLC::FalconVersion::UNKNOWN) {
       return SendResponse(MINER_AUTH_RESPONSE, {0x00}); // Invalid key
   }
   ```

7. **Check Optional Whitelist** (if configured)
   ```cpp
   if(config::HasArg("-minerallowkey")) {
       std::vector<std::string> vAllowedKeys = config::GetMultiArgs("-minerallowkey");
       if(!IsKeyWhitelisted(vPubkey, vAllowedKeys)) {
           return SendResponse(MINER_AUTH_RESPONSE, {0x00}); // Not whitelisted
       }
   }
   ```

8. **Create Session Entry in Cache**
   ```cpp
   uint32_t nSessionId = LLC::GetRand(); // Random 32-bit session ID
   MiningContext ctx;
   ctx.fAuthenticated = true;
   ctx.nSessionId = nSessionId;
   ctx.hashGenesis = hashGenesis;
   ctx.vMinerPubKey = vPubkey;
   ctx.nFalconVersion = nVersion;
   ctx.vChaChaKey = vChaChaKey;
   ctx.nSessionStart = runtime::unifiedtimestamp();
   
   SessionCache.insert({nSessionId, std::move(ctx)});
   ```

9. **Send MINER_AUTH_RESPONSE (0xD001)**

**Performance:**
- Genesis lookup: <1ms (database indexed)
- Falcon key validation: <0.1ms (size check only)
- ChaCha20 decryption: <0.5ms
- **Total processing time: <2ms**

**Security Validations:**
- ✅ Genesis hash length = 32 bytes
- ✅ Genesis exists in blockchain
- ✅ Falcon pubkey length = 897 or 1793 bytes
- ✅ ChaCha20 decryption successful (if wrapped)
- ✅ Optional: minerallowkey whitelist check

**Error Responses:**
- Invalid genesis → `MINER_AUTH_RESPONSE {0x00}`
- Invalid public key → `MINER_AUTH_RESPONSE {0x00}`
- Not whitelisted → `MINER_AUTH_RESPONSE {0x00}`

---

### MINER_AUTH_RESPONSE (0xD001)

**Direction:** Node → Miner  
**Generated by:** `StatelessMinerConnection::SendAuthResponse()`

**Description:**  
Node's response to authentication request. Sends session ID and optional challenge nonce.

**Packet Structure:**

```
┌────────────────────────────────────────────────────────┐
│ MINER_AUTH_RESPONSE Packet (0xD001)                    │
├────────────────────────────────────────────────────────┤
│ Offset │ Field                │ Size        │ Type     │
├────────┼──────────────────────┼─────────────┼──────────┤
│ 0      │ Success Flag         │ 1 byte      │ uint8    │
│ 1-4    │ Session ID           │ 4 bytes     │ uint32   │
│ 5-16   │ Challenge Nonce      │ 12 bytes    │ bytes    │
│ 17-80  │ Challenge Data       │ 64 bytes    │ bytes    │
└────────────────────────────────────────────────────────┘

Total Size: 81 bytes
Success Flag: 0x01 = success, 0x00 = failure
```

**Response Construction:**

```cpp
std::vector<uint8_t> response;
response.push_back(fSuccess ? 0x01 : 0x00);  // Success flag

if(fSuccess) {
    // Session ID (4 bytes)
    response.insert(response.end(), 
        (uint8_t*)&nSessionId, 
        (uint8_t*)&nSessionId + 4);
    
    // Challenge nonce (12 bytes)
    std::vector<uint8_t> vNonce = GenerateRandomNonce(12);
    response.insert(response.end(), vNonce.begin(), vNonce.end());
    
    // Challenge data (64 bytes)
    std::vector<uint8_t> vChallenge = GenerateChallenge(64);
    response.insert(response.end(), vChallenge.begin(), vChallenge.end());
}

SendPacket(Packet(MINER_AUTH_RESPONSE, response));
```

**Success Criteria:**
- Genesis hash valid and exists
- Falcon public key valid format
- Optional whitelist check passed
- Session created successfully

**Session Properties:**
- **Default timeout:** 24 hours (86400 seconds)
- **Session ID:** Random 32-bit integer
- **ChaCha20 key:** Derived from genesis hash
- **Falcon version:** Auto-detected from pubkey size

---

## Configuration Flow

### MINER_SET_REWARD (0xD003)

**Direction:** Miner → Node  
**Handler:** `StatelessMinerConnection::ProcessSetReward()`

**Description:**  
Miner sets the reward payout address for mining rewards. This address may differ from the authentication genesis hash.

**Packet Structure:**

```
┌────────────────────────────────────────────────────────┐
│ MINER_SET_REWARD Packet (0xD003)                       │
├────────────────────────────────────────────────────────┤
│ Offset │ Field                │ Size        │ Type     │
├────────┼──────────────────────┼─────────────┼──────────┤
│ 0-11   │ ChaCha20 Nonce       │ 12 bytes    │ bytes    │
│ 12-43  │ Encrypted Address    │ 32 bytes    │ uint256  │
│ 44-59  │ Poly1305 Tag         │ 16 bytes    │ bytes    │
└────────────────────────────────────────────────────────┘

Total Size: 60 bytes (encrypted payload)
```

**Node Processing:**

1. **Validate Session**
   ```cpp
   if(!SessionCache.count(nSessionId)) {
       return SendResponse(MINER_REWARD_RESULT, {0x00}); // Invalid session
   }
   ```

2. **Extract ChaCha20-Poly1305 Components**
   ```cpp
   std::vector<uint8_t> vNonce(12);
   std::vector<uint8_t> vCiphertext(32);
   std::vector<uint8_t> vTag(16);
   
   PACKET.Extract(vNonce);
   PACKET.Extract(vCiphertext);
   PACKET.Extract(vTag);
   ```

3. **Decrypt Reward Address**
   ```cpp
   MiningContext& ctx = SessionCache[nSessionId];
   std::vector<uint8_t> vPlaintext;
   
   if(!DecryptChaCha20Poly1305(vCiphertext, vTag, vNonce, 
                                 ctx.vChaChaKey, vPlaintext)) {
       return SendResponse(MINER_REWARD_RESULT, {0x00}); // Decryption failed
   }
   
   uint256_t hashReward;
   std::copy(vPlaintext.begin(), vPlaintext.begin() + 32, hashReward.begin());
   ```

4. **Validate Reward Address Type**
   ```cpp
   if(!GenesisConstants::IsValidGenesisType(hashReward)) {
       return SendResponse(MINER_REWARD_RESULT, {0x00}); // Invalid type byte
   }
   ```

5. **Bind Reward Address to Session**
   ```cpp
   ctx.hashRewardAddress = hashReward;
   ctx.fRewardBound = true;
   ```

6. **Send MINER_REWARD_RESULT (0xD004)**

**Note:** The reward address **must** be a valid TritiumGenesis (UserType) sigchain hash. Register Addresses (type 0x02 ACCOUNT registers) are explicitly rejected — `Coinbase::Verify()` enforces this on all network peers; a block with a Register Address in the coinbase field is rejected by the entire network. Pool operators provide their own TritiumGenesis account as the reward address and handle miner payouts separately.

**Performance:**
- ChaCha20-Poly1305 decryption: <0.5ms
- Address validation: <0.1ms
- **Total: <1ms**

---

### MINER_REWARD_RESULT (0xD004)

**Direction:** Node → Miner  
**Generated by:** `StatelessMinerConnection::SendRewardResult()`

**Packet Structure:**

```
┌────────────────────────────────────────────────────────┐
│ MINER_REWARD_RESULT Packet (0xD004)                    │
├────────────────────────────────────────────────────────┤
│ Offset │ Field                │ Size        │ Type     │
├────────┼──────────────────────┼─────────────┼──────────┤
│ 0      │ Success Flag         │ 1 byte      │ uint8    │
└────────────────────────────────────────────────────────┘

Total Size: 1 byte
Success Flag: 0x01 = success, 0x00 = failure
```

**Success Response:** `{0x01}`  
**Failure Responses:**
- Invalid session: `{0x00}`
- Decryption failed: `{0x00}`
- Invalid address format: `{0x00}`

---

### SET_CHANNEL (0xD005)

**Direction:** Miner → Node  
**Handler:** `StatelessMinerConnection::ProcessSetChannel()`

**Description:**  
Miner selects which mining channel to work on (Prime or Hash).

**Packet Structure:**

```
┌────────────────────────────────────────────────────────┐
│ SET_CHANNEL Packet (0xD005)                            │
├────────────────────────────────────────────────────────┤
│ Offset │ Field                │ Size        │ Type     │
├────────┼──────────────────────┼─────────────┼──────────┤
│ 0      │ Channel              │ 1 byte      │ uint8    │
└────────────────────────────────────────────────────────┘

Total Size: 1 byte
Channel Values: 1 = Prime, 2 = Hash
```

**Valid Channels:**
- `1` = Prime channel (CPU mining, Fermat primality testing)
- `2` = Hash channel (GPU/FPGA mining, SHA3 hashing)

**Invalid Channels:**
- `0` = Stake channel (not available for stateless mining)
- `3+` = Reserved/undefined

**Node Processing:**

1. **Validate Session**
   ```cpp
   if(!SessionCache.count(nSessionId)) {
       return SendResponse(CHANNEL_ACK, {0x00}); // Invalid session
   }
   ```

2. **Extract Channel**
   ```cpp
   uint8_t nChannel;
   PACKET.Extract(nChannel);
   ```

3. **Validate Channel**
   ```cpp
   if(nChannel != 1 && nChannel != 2) {
       return SendResponse(CHANNEL_ACK, {0x00}); // Invalid channel
   }
   ```

4. **Update Session**
   ```cpp
   MiningContext& ctx = SessionCache[nSessionId];
   ctx.nChannel = nChannel;
   ```

5. **Send CHANNEL_ACK (0xD006)**

**Performance:** <0.1ms

---

### CHANNEL_ACK (0xD006)

**Direction:** Node → Miner  
**Generated by:** `StatelessMinerConnection::SendChannelAck()`

**Packet Structure:**

```
┌────────────────────────────────────────────────────────┐
│ CHANNEL_ACK Packet (0xD006)                            │
├────────────────────────────────────────────────────────┤
│ Offset │ Field                │ Size        │ Type     │
├────────┼──────────────────────┼─────────────┼──────────┤
│ 0      │ Channel              │ 1 byte      │ uint8    │
└────────────────────────────────────────────────────────┘

Total Size: 1 byte
Success: Echo back valid channel (1 or 2)
Failure: 0x00
```

---

## Template Delivery Flow

### MINER_READY (0xD007)

**Direction:** Miner → Node (subscription), Node → Miner (acknowledgment)  
**Handler:** `StatelessMinerConnection::ProcessMinerReady()`

**Description:**  
Bidirectional opcode. Miner sends to subscribe to push notifications. Node acknowledges with 1-byte response, then immediately sends GET_BLOCK with initial template.

**Miner → Node Packet:**

```
┌────────────────────────────────────────────────────────┐
│ MINER_READY Packet (0xD007) - Miner → Node             │
├────────────────────────────────────────────────────────┤
│ Offset │ Field                │ Size        │ Type     │
├────────┼──────────────────────┼─────────────┼──────────┤
│        │ (Empty packet)       │ 0 bytes     │          │
└────────────────────────────────────────────────────────┘

Total Size: 0 bytes (opcode only)
```

**Node → Miner Acknowledgment:**

```
┌────────────────────────────────────────────────────────┐
│ MINER_READY Packet (0xD007) - Node → Miner (ACK)       │
├────────────────────────────────────────────────────────┤
│ Offset │ Field                │ Size        │ Type     │
├────────┼──────────────────────┼─────────────┼──────────┤
│ 0      │ Acknowledgment       │ 1 byte      │ uint8    │
└────────────────────────────────────────────────────────┘

Total Size: 1 byte
Value: 0x01 (success)
```

**Node Processing:**

1. **Validate Session and Configuration**
   ```cpp
   if(!SessionCache.count(nSessionId)) {
       return; // Invalid session - no response
   }
   
   MiningContext& ctx = SessionCache[nSessionId];
   if(!ctx.fAuthenticated || ctx.nChannel == 0) {
       return; // Not authenticated or channel not set
   }
   ```

2. **Subscribe to Push Notifications**
   ```cpp
   ctx.fSubscribedToNotifications = true;
   ctx.nSubscribedChannel = ctx.nChannel;
   PushNotificationList.insert(nSessionId);
   ```

3. **Send Acknowledgment**
   ```cpp
   SendPacket(Packet(MINER_READY, {0x01}));
   ```

4. **Generate Initial Template**
   ```cpp
   TAO::Ledger::TritiumBlock* pBlock = CreateNewBlock(ctx.nChannel, 
                                                       ctx.GetPayoutAddress());
   ```

5. **Calculate Channel Height**
   ```cpp
   TAO::Ledger::BlockState* pLastState = GetLastState(ctx.nChannel);
   uint32_t nChannelHeight = pLastState->nChannelHeight;
   ```

6. **Store Template Metadata**
   ```cpp
   TemplateMetadata meta(pBlock, 
                        runtime::unifiedtimestamp(),
                        ChainState::tStateBest.load().nHeight,
                        nChannelHeight,
                        pBlock->hashMerkleRoot,
                        ctx.nChannel);
   
   SessionTemplates[nSessionId] = std::move(meta);
   ```

7. **Send GET_BLOCK (0xD008)** with 216-byte template

**Performance:**
- Subscription update: <0.1ms
- Template generation: <1ms
- **Total: <2ms**

---

### GET_BLOCK (0xD008)

**Direction:** Node → Miner  
**Generated by:** `StatelessMinerConnection::SendGetBlock()`

**Description:**  
Sends the initial 216-byte block template to miner immediately after MINER_READY acknowledgment.

**Packet Structure:**

```
┌────────────────────────────────────────────────────────┐
│ GET_BLOCK Packet (0xD008)                              │
├────────────────────────────────────────────────────────┤
│ Offset │ Field                │ Size        │ Type     │
├────────┼──────────────────────┼─────────────┼──────────┤
│ 0-215  │ TritiumBlock         │ 216 bytes   │ serialized│
└────────────────────────────────────────────────────────┘

Total Size: 216 bytes (serialized TritiumBlock)
```

**TritiumBlock Serialization (216 bytes):**

```
Offset  | Field              | Size     | Description
--------|--------------------│----------│-----------------------
0-3     | nVersion           | 4 bytes  | Block version
4-7     | nChannel           | 4 bytes  | Mining channel (1 or 2)
8-39    | hashPrevBlock      | 32 bytes | Previous block hash
40-71   | hashMerkleRoot     | 32 bytes | Merkle root
72-103  | hashProducer       | 32 bytes | Miner reward address
104-111 | nHeight            | 8 bytes  | Blockchain height
112-119 | nBits              | 8 bytes  | Difficulty target
120-127 | nNonce             | 8 bytes  | Nonce (miner fills this)
128-135 | nTime              | 8 bytes  | Block timestamp
136-215 | (Reserved)         | 80 bytes | Future extensions
```

**Template Properties:**
- **Wallet-signed:** Producer signature pre-applied by node
- **Ready to mine:** Miner only needs to find valid nonce
- **Compact:** 216 bytes = minimal bandwidth
- **Self-contained:** All necessary fields included

**Performance:**
- Serialization: <0.5ms
- Transmission: <5ms (typical network)
- **Total latency: <10ms** (node event → miner receives template)

---

### NEW_BLOCK (0xD009)

**Direction:** Node → Miner (push notification)  
**Generated by:** `StatelessMinerConnection::PushNewBlock()`

**Description:**  
Pushes updated 216-byte template to subscribed miners when blockchain advances. This is the core of the **push notification system** that eliminates polling overhead.

**Packet Structure:**

```
┌────────────────────────────────────────────────────────┐
│ NEW_BLOCK Packet (0xD009)                              │
├────────────────────────────────────────────────────────┤
│ Offset │ Field                │ Size        │ Type     │
├────────┼──────────────────────┼─────────────┼──────────┤
│ 0-215  │ TritiumBlock         │ 216 bytes   │ serialized│
└────────────────────────────────────────────────────────┘

Total Size: 216 bytes (identical format to GET_BLOCK)
```

**Trigger Conditions:**

1. **New Block Accepted to Blockchain**
   ```cpp
   // In block acceptance handler
   if(new_block_accepted) {
       BroadcastNewBlockToMiners(new_block.nChannel);
   }
   ```

2. **Template Staleness** (>60 seconds old)
   ```cpp
   if(template.GetAge() > 60) {
       GenerateAndPushNewTemplate(session_id);
   }
   ```

3. **Difficulty Adjustment**
   ```cpp
   if(difficulty_changed) {
       RegenerateAllTemplates();
   }
   ```

**Push Flow:**

```cpp
void BroadcastNewBlockToMiners(uint32_t nChannel) {
    for(auto& [session_id, context] : SessionCache) {
        // Only push to subscribers of this channel
        if(!context.fSubscribedToNotifications)
            continue;
        if(context.nSubscribedChannel != nChannel)
            continue;
        
        // Generate channel-specific template
        TritiumBlock* pBlock = CreateNewBlock(nChannel, 
                                              context.GetPayoutAddress());
        
        // Send NEW_BLOCK
        SendNewBlock(session_id, pBlock);
        
        // Update statistics
        context.nNotificationsSent++;
        context.nLastNotificationTime = runtime::unifiedtimestamp();
    }
}
```

**Performance Target:** **<10ms** from blockchain event to all miners notified

**Actual Performance:**
- Template generation: <1ms per template
- Network transmission: <5ms typical
- **Total:** 5-8ms average (well within <10ms target)

**Channel Isolation:**
- Prime miners receive NEW_BLOCK only when Prime channel advances
- Hash miners receive NEW_BLOCK only when Hash channel advances
- **Result:** Eliminates ~40% false-positive template invalidations

---

## Solution Submission Flow

### SUBMIT_BLOCK (0xD00A)

**Direction:** Miner → Node  
**Handler:** `StatelessMinerConnection::ProcessSubmitBlock()`

**Description:**  
Miner submits solved block with valid nonce and optional Falcon signature.

**Packet Structure:**

```
┌─────────────────────────────────────────────────────────────────┐
│ SUBMIT_BLOCK Packet (0xD00A)                                     │
├─────────────────────────────────────────────────────────────────┤
│ Offset │ Field                       │ Size         │ Type       │
├────────┼─────────────────────────────┼──────────────┼────────────┤
│ 0-215  │ Solved TritiumBlock         │ 216 bytes    │ serialized │
│ 216    │ Has Physical Falcon         │ 1 byte       │ bool       │
│ 217-x  │ Physical Falcon Signature   │ 809/1577 B   │ optional   │
└─────────────────────────────────────────────────────────────────┘

Total Size: 217+ bytes (depends on Falcon signature presence/version)

Physical Falcon Sizes:
- Falcon-512: 809 bytes
- Falcon-1024: 1577 bytes
```

**Node Processing:**

1. **Decrypt Packet** (if encrypted)
   ```cpp
   MiningContext& ctx = SessionCache[nSessionId];
   if(ctx.fEncryptionReady) {
       DecryptPacket(PACKET, ctx.vChaChaKey);
   }
   ```

2. **Extract Block**
   ```cpp
   TAO::Ledger::TritiumBlock block;
   PACKET >> block;
   ```

3. **Validate Proof-of-Work**
   ```cpp
   uint1024_t hashProof = block.GetHash();
   if(hashProof > block.nBits) {
       return SendResponse(BLOCK_REJECTED, "Invalid proof-of-work");
   }
   ```

4. **Verify Nonce Range** (optional check)
   ```cpp
   if(block.nNonce < nonce_range_start || 
      block.nNonce > nonce_range_end) {
       debug::warning("Nonce out of assigned range");
   }
   ```

5. **Extract Optional Physical Falcon Signature**
   ```cpp
   bool fHasPhysical;
   PACKET >> fHasPhysical;
   
   if(fHasPhysical) {
       uint16_t nSigLen;
       PACKET >> nSigLen;
       
       std::vector<uint8_t> vPhysicalSig(nSigLen);
       PACKET >> vPhysicalSig;
       
       // Verify signature matches session's Falcon key
       if(!VerifyFalconSignature(vPhysicalSig, ctx.vMinerPubKey, 
                                  block.GetHash())) {
           return SendResponse(BLOCK_REJECTED, "Invalid Falcon signature");
       }
       
       // Store signature on block
       block.vchBlockSig = vPhysicalSig;
   }
   ```

6. **Validate Block Structure**
   ```cpp
   if(!block.Check()) {
       return SendResponse(BLOCK_REJECTED, "Invalid block structure");
   }
   ```

7. **Validate Transactions**
   ```cpp
   if(!block.CheckTransactions()) {
       return SendResponse(BLOCK_REJECTED, "Invalid transactions");
   }
   ```

8. **Add to Mempool/Blockchain**
   ```cpp
   if(!AcceptBlock(block)) {
       return SendResponse(BLOCK_REJECTED, "Block rejected by consensus");
   }
   ```

9. **Send BLOCK_ACCEPTED (0xD00B)**
10. **Broadcast NEW_BLOCK to all miners**

**Performance Metrics:**

| Validation Step | Target | Typical |
|-----------------|--------|---------|
| PoW verification | <10ms | 5ms |
| Falcon signature | <5ms | 2ms |
| Block structure | <5ms | 2ms |
| Transaction validation | <30ms | 15ms |
| **Total** | **<50ms** | **20-30ms** |

---

### BLOCK_ACCEPTED (0xD00B)

**Direction:** Node → Miner  
**Generated by:** `StatelessMinerConnection::SendBlockAccepted()`

**Packet Structure:**

```
┌────────────────────────────────────────────────────────┐
│ BLOCK_ACCEPTED Packet (0xD00B)                         │
├────────────────────────────────────────────────────────┤
│ Offset │ Field                │ Size        │ Type     │
├────────┼──────────────────────┼─────────────┼──────────┤
│ 0-31   │ Block Hash           │ 32 bytes    │ uint256  │
│ 32-39  │ Height               │ 8 bytes     │ uint64   │
└────────────────────────────────────────────────────────┘

Total Size: 40 bytes
```

**Response Details:**
- **Block Hash:** Hash of accepted block (for miner tracking)
- **Height:** Blockchain height where block was accepted

**What Happens Next:**
1. Block is added to blockchain
2. NEW_BLOCK (0xD009) is pushed to all subscribed miners
3. Miner reward is queued for payout to configured address

---

### BLOCK_REJECTED (0xD00C)

**Direction:** Node → Miner  
**Generated by:** `StatelessMinerConnection::SendBlockRejected()`

**Packet Structure:**

```
┌────────────────────────────────────────────────────────┐
│ BLOCK_REJECTED Packet (0xD00C)                         │
├────────────────────────────────────────────────────────┤
│ Offset │ Field                │ Size        │ Type     │
├────────┼──────────────────────┼─────────────┼──────────┤
│ 0      │ Reason Length        │ 1 byte      │ uint8    │
│ 1-x    │ Reason String        │ variable    │ UTF-8    │
└────────────────────────────────────────────────────────┘

Total Size: 1+ bytes (depends on reason length)
```

**Common Rejection Reasons:**

| Reason String | Cause |
|---------------|-------|
| `"Invalid proof-of-work"` | Hash doesn't meet difficulty target |
| `"Stale template"` | Block based on outdated template |
| `"Invalid Falcon signature"` | Physical Falcon signature verification failed |
| `"Invalid block structure"` | Malformed block data |
| `"Invalid transactions"` | Transaction validation failed |
| `"Block rejected by consensus"` | Consensus rules violation |
| `"Duplicate block"` | Block already exists at this height |

---

## Performance Metrics

### Node-Side Latency Targets

| Operation | Target | Typical | Notes |
|-----------|--------|---------|-------|
| MINER_AUTH processing | <5ms | 2-3ms | Genesis lookup + key validation |
| MINER_SET_REWARD processing | <2ms | <1ms | ChaCha20 decryption |
| Template generation | <1ms | 0.5ms | Block creation + serialization |
| GET_BLOCK response | <5ms | 2-3ms | Initial template delivery |
| NEW_BLOCK push | <10ms | 5-8ms | Blockchain event → all miners |
| SUBMIT_BLOCK validation | <50ms | 20-30ms | Full block verification |
| Session lookup | <0.1ms | <0.05ms | Hash table access |

### Throughput Capacity

**Single Node Mining Server:**
- **Concurrent miners:** 100-500 (depending on hardware)
- **Templates/second:** 1000+ (cached generation)
- **Submissions/second:** 50-100 (validation bottleneck)
- **Push notifications/second:** 1000+ (broadcast)

**Resource Usage:**
- **CPU:** 2-4 cores recommended
- **RAM:** 256-512 MB for session cache
- **Network:** <10 Mbps for 100 miners

---

## Error Handling

### Session Errors

| Error | Opcode | Response |
|-------|--------|----------|
| Invalid session ID | Any | No response (silent drop) |
| Session expired | Any | Connection closed |
| Not authenticated | MINER_SET_REWARD, SET_CHANNEL | Response with 0x00 |

### Authentication Errors

| Error | Response | Packet |
|-------|----------|--------|
| Genesis not found | MINER_AUTH_RESPONSE | `{0x00}` |
| Invalid Falcon key | MINER_AUTH_RESPONSE | `{0x00}` |
| Not whitelisted | MINER_AUTH_RESPONSE | `{0x00}` |
| Decryption failed | MINER_AUTH_RESPONSE | `{0x00}` |

### Block Submission Errors

| Error | Response | Reason String |
|-------|----------|---------------|
| Invalid PoW | BLOCK_REJECTED | "Invalid proof-of-work" |
| Stale template | BLOCK_REJECTED | "Stale template" |
| Bad Falcon sig | BLOCK_REJECTED | "Invalid Falcon signature" |
| Malformed block | BLOCK_REJECTED | "Invalid block structure" |

---

## Cross-References

### Miner Perspective
- [NexusMiner Opcodes Reference](https://github.com/Nexusoft/NexusMiner/blob/main/docs/reference/opcodes-reference.md)
- [NexusMiner Protocol Flow](https://github.com/Nexusoft/NexusMiner/blob/main/docs/current/mining-protocols/stateless-mining.md)
- [NexusMiner Authentication](https://github.com/Nexusoft/NexusMiner/blob/main/docs/current/security/falcon-authentication.md)

### Node Documentation
- [Stateless Mining Protocol](../current/mining/stateless-protocol.md)
- [Mining Server Architecture](../current/mining/mining-server.md)
- [Template Generation](../current/mining/template-generation.md)
- [Push Notifications](../current/mining/push-notifications.md)
- [Falcon Verification](../current/authentication/falcon-verification.md)
- [Node Configuration Reference](nexus.conf.md)

### Source Code References
- `src/LLP/include/llp_opcodes.h` - Opcode definitions
- `src/LLP/stateless_miner_connection.cpp` - Packet handlers
- `src/LLP/include/stateless_miner.h` - Data structures
- `src/TAO/Ledger/create.cpp` - Block template generation

---

## Version Information

**Document Version:** 1.0  
**Protocol Version:** Stateless Mining 1.0  
**LLL-TAO Version:** 5.1.0+  
**Compatibility:** Backward compatible with legacy protocol (Opcodes::Legacy::GET_BLOCK)  
**Last Updated:** 2026-01-13

---

*This reference documents all opcodes from the node's perspective. For the miner's perspective, see the [NexusMiner Opcodes Reference](https://github.com/Nexusoft/NexusMiner/blob/main/docs/reference/opcodes-reference.md).*
