# Flow Architecture Diagrams - Node Reference

## Overview

This document provides comprehensive flow diagrams for the Nexus node's stateless mining protocol. These diagrams illustrate the complete lifecycle from miner connection to block acceptance.

**Document Version:** 1.0  
**Last Updated:** 2026-01-13

---

## Complete Protocol Flow

### Full Mining Session Lifecycle

```mermaid
sequenceDiagram
    participant M as Miner
    participant N as Node
    participant BC as Blockchain
    
    Note over M,BC: Phase 1: Authentication
    M->>N: MINER_AUTH (0xD000)<br/>[Genesis + Falcon PubKey]
    N->>N: Verify genesis exists
    N->>N: Validate Falcon key format
    N->>N: Check optional whitelist
    N->>N: Create session + ChaCha20 key
    N->>M: MINER_AUTH_RESPONSE (0xD001)<br/>[Success + Session ID]
    
    Note over M,BC: Phase 2: Configuration
    M->>N: MINER_SET_REWARD (0xD003)<br/>[Encrypted reward address]
    N->>N: Decrypt with ChaCha20
    N->>N: Validate address format
    N->>N: Bind address to session
    N->>M: MINER_REWARD_RESULT (0xD004)<br/>[Success]
    
    M->>N: SET_CHANNEL (0xD005)<br/>[1=Prime or 2=Hash]
    N->>N: Validate channel
    N->>N: Update session
    N->>M: CHANNEL_ACK (0xD006)<br/>[Channel confirmed]
    
    Note over M,BC: Phase 3: Subscription
    M->>N: MINER_READY (0xD007)
    N->>N: Subscribe to notifications
    N->>M: MINER_READY ACK (0xD007)<br/>[0x01]
    N->>N: Generate initial template
    N->>M: GET_BLOCK (0xD008)<br/>[216-byte template]
    
    Note over M,BC: Phase 4: Mining Loop
    loop Mining
        M->>M: Hash nonce candidates
        
        alt Blockchain Advances
            BC->>N: New block detected
            N->>N: Generate fresh template
            N->>M: NEW_BLOCK (0xD009)<br/>[Updated 216-byte template]
            M->>M: Discard old work<br/>Start new template
        end
        
        alt Solution Found
            M->>N: SUBMIT_BLOCK (0xD00A)<br/>[Solved block + sig]
            N->>N: Validate PoW
            N->>N: Verify Falcon signature
            N->>N: Check block structure
            N->>BC: Accept to blockchain
            BC->>N: Block accepted
            N->>M: BLOCK_ACCEPTED (0xD00B)<br/>[Block hash + height]
            N->>M: NEW_BLOCK (0xD009)<br/>[Next template]
            
            Note over N: Broadcast to all miners
            N->>M: NEW_BLOCK (0xD009)<br/>[Next template]
        end
    end
```

---

## Authentication Flow Detail

### MINER_AUTH Processing

```mermaid
flowchart TD
    Start[Miner sends MINER_AUTH] --> Extract[Extract genesis hash]
    Extract --> Derive[Derive ChaCha20 key<br/>from genesis]
    Derive --> ExtractKey[Extract Falcon<br/>public key length]
    ExtractKey --> CheckLen{Key length<br/>897 or 1793?}
    
    CheckLen -->|Invalid| Reject1[Send MINER_AUTH_RESPONSE<br/>0x00 - Failure]
    CheckLen -->|Valid| CheckRemote{Remote<br/>connection?}
    
    CheckRemote -->|Yes| Decrypt[Decrypt key<br/>with ChaCha20]
    CheckRemote -->|No| CheckGenesis
    Decrypt --> CheckGenesis{Genesis<br/>exists in<br/>blockchain?}
    
    CheckGenesis -->|No| Reject2[Send MINER_AUTH_RESPONSE<br/>0x00 - Failure]
    CheckGenesis -->|Yes| DetectVersion[Detect Falcon version<br/>512 or 1024]
    
    DetectVersion --> CheckWhitelist{minerallowkey<br/>configured?}
    CheckWhitelist -->|Yes| ValidateWhitelist{Key in<br/>whitelist?}
    CheckWhitelist -->|No| CreateSession
    
    ValidateWhitelist -->|No| Reject3[Send MINER_AUTH_RESPONSE<br/>0x00 - Failure]
    ValidateWhitelist -->|Yes| CreateSession[Create session entry<br/>Generate session ID]
    
    CreateSession --> StoreContext[Store MiningContext:<br/>- Session ID<br/>- Genesis hash<br/>- Falcon pubkey<br/>- ChaCha20 key<br/>- Falcon version]
    
    StoreContext --> Success[Send MINER_AUTH_RESPONSE<br/>0x01 + Session ID<br/>+ Nonce + Challenge]
    
    Success --> End[Authentication Complete]
    Reject1 --> EndFail[Authentication Failed]
    Reject2 --> EndFail
    Reject3 --> EndFail
    
    style Start fill:#e1f5ff
    style Success fill:#d4edda
    style End fill:#d4edda
    style Reject1 fill:#f8d7da
    style Reject2 fill:#f8d7da
    style Reject3 fill:#f8d7da
    style EndFail fill:#f8d7da
```

---

## Template Generation and Delivery

### Initial Template (GET_BLOCK)

```mermaid
flowchart TD
    Start[MINER_READY received] --> Validate{Session<br/>authenticated<br/>& configured?}
    
    Validate -->|No| End1[Silently ignore]
    Validate -->|Yes| Subscribe[Add to push<br/>notification list]
    
    Subscribe --> AckReady[Send MINER_READY ACK<br/>0xD007 with 0x01]
    
    AckReady --> GetState[Get best blockchain state]
    GetState --> CreateBlock[Create TritiumBlock:<br/>- Version<br/>- Channel<br/>- Previous hash<br/>- Height<br/>- Difficulty bits<br/>- Timestamp]
    
    CreateBlock --> SetReward[Set producer address<br/>from session]
    SetReward --> AddCoinbase[Add coinbase transaction<br/>with reward amount]
    
    AddCoinbase --> CalcMerkle[Calculate merkle root]
    CalcMerkle --> SignWallet[Sign block with<br/>node wallet key]
    
    SignWallet --> GetChannelHeight[Get channel-specific height<br/>via GetLastState]
    GetChannelHeight --> StoreMetadata[Store TemplateMetadata:<br/>- Block pointer<br/>- Creation time<br/>- Unified height<br/>- Channel height<br/>- Merkle root<br/>- Channel]
    
    StoreMetadata --> Serialize[Serialize TritiumBlock<br/>to 216 bytes]
    Serialize --> SendBlock[Send GET_BLOCK 0xD008<br/>with 216-byte template]
    
    SendBlock --> End2[Template delivered<br/><5ms typical]
    
    style Start fill:#e1f5ff
    style End2 fill:#d4edda
    style End1 fill:#f8d7da
```

### Push Notification (NEW_BLOCK)

```mermaid
flowchart TD
    Trigger[Blockchain advances<br/>New block accepted] --> DetectChannel[Identify channel of<br/>new block]
    
    DetectChannel --> GetSessions[Get all active<br/>mining sessions]
    
    GetSessions --> Loop{For each<br/>session}
    
    Loop -->|Next| CheckSubscribed{Subscribed to<br/>notifications?}
    CheckSubscribed -->|No| Loop
    CheckSubscribed -->|Yes| CheckChannel{Subscribed<br/>channel matches?}
    
    CheckChannel -->|No| Loop
    CheckChannel -->|Yes| GenTemplate[Generate fresh template<br/>for this channel]
    
    GenTemplate --> Serialize[Serialize to<br/>216 bytes]
    Serialize --> SendNotif[Send NEW_BLOCK 0xD009<br/>to this miner]
    
    SendNotif --> UpdateStats[Update statistics:<br/>- Notifications sent++<br/>- Last notification time]
    
    UpdateStats --> Loop
    
    Loop -->|Done| Measure[Measure total time]
    Measure --> CheckTarget{Latency<br/><10ms?}
    
    CheckTarget -->|Yes| Success[Broadcast successful]
    CheckTarget -->|No| Warning[Log performance warning]
    
    Warning --> Success
    
    style Trigger fill:#e1f5ff
    style Success fill:#d4edda
    style Warning fill:#fff3cd
```

---

## Block Submission Flow

### SUBMIT_BLOCK Validation

```mermaid
flowchart TD
    Start[Miner sends SUBMIT_BLOCK] --> CheckSession{Valid<br/>session?}
    
    CheckSession -->|No| End1[Silently ignore]
    CheckSession -->|Yes| CheckEncryption{Encryption<br/>enabled?}
    
    CheckEncryption -->|Yes| Decrypt[Decrypt packet<br/>with ChaCha20]
    CheckEncryption -->|No| Extract
    Decrypt --> Extract[Extract TritiumBlock<br/>216 bytes]
    
    Extract --> CheckPoW{Proof-of-work<br/>valid?}
    CheckPoW -->|No| Reject1[BLOCK_REJECTED<br/>Invalid proof-of-work]
    CheckPoW -->|Yes| CheckPrev{Previous hash<br/>matches best<br/>chain?}
    
    CheckPrev -->|No| Reject2[BLOCK_REJECTED<br/>Stale template]
    CheckPrev -->|Yes| CheckFalcon{Physical Falcon<br/>signature<br/>present?}
    
    CheckFalcon -->|Yes| VerifyFalcon{Signature<br/>valid?}
    CheckFalcon -->|No| CheckStructure
    
    VerifyFalcon -->|No| Reject3[BLOCK_REJECTED<br/>Invalid Falcon signature]
    VerifyFalcon -->|Yes| AttachSig[Attach signature<br/>to block]
    
    AttachSig --> CheckStructure{Block structure<br/>valid?}
    CheckStructure -->|No| Reject4[BLOCK_REJECTED<br/>Invalid block structure]
    CheckStructure -->|Yes| CheckTx{Transactions<br/>valid?}
    
    CheckTx -->|No| Reject5[BLOCK_REJECTED<br/>Invalid transactions]
    CheckTx -->|Yes| AcceptBlock[Add block to<br/>blockchain]
    
    AcceptBlock --> CheckAccepted{Accepted by<br/>consensus?}
    CheckAccepted -->|No| Reject6[BLOCK_REJECTED<br/>Block rejected by consensus]
    CheckAccepted -->|Yes| SendAccept[BLOCK_ACCEPTED 0xD00B<br/>Block hash + height]
    
    SendAccept --> BroadcastNew[Broadcast NEW_BLOCK 0xD009<br/>to all subscribed miners]
    
    BroadcastNew --> Success[Block accepted<br/>20-30ms typical]
    
    Reject1 --> EndFail[Validation failed]
    Reject2 --> EndFail
    Reject3 --> EndFail
    Reject4 --> EndFail
    Reject5 --> EndFail
    Reject6 --> EndFail
    End1 --> EndFail
    
    style Start fill:#e1f5ff
    style Success fill:#d4edda
    style EndFail fill:#f8d7da
    style Reject1 fill:#f8d7da
    style Reject2 fill:#f8d7da
    style Reject3 fill:#f8d7da
    style Reject4 fill:#f8d7da
    style Reject5 fill:#f8d7da
    style Reject6 fill:#f8d7da
```

---

## Session Management

### Session Lifecycle

```mermaid
stateDiagram-v2
    [*] --> Disconnected: Initial State
    
    Disconnected --> Authenticating: MINER_AUTH received
    Authenticating --> Authenticated: Authentication successful
    Authenticating --> Disconnected: Authentication failed
    
    Authenticated --> Configuring: Configuration phase
    Configuring --> Configured: Reward + Channel set
    Configuring --> Authenticated: Configuration error
    
    Configured --> Subscribed: MINER_READY received
    Subscribed --> Mining: GET_BLOCK sent
    
    Mining --> Mining: NEW_BLOCK (blockchain advances)
    Mining --> Mining: SUBMIT_BLOCK validation
    Mining --> Mining: Keepalive heartbeat
    
    Mining --> Disconnected: Connection lost
    Mining --> Disconnected: Timeout expired
    
    Disconnected --> Authenticating: Reconnection
    
    note right of Authenticated
        Session created
        24h default timeout
    end note
    
    note right of Mining
        Active mining
        Push notifications enabled
    end note
```

### Session Cache Cleanup

```mermaid
flowchart TD
    Start[Periodic cleanup<br/>timer triggered] --> Lock[Acquire cache lock]
    
    Lock --> GetTime[Get current timestamp]
    GetTime --> Iterate{For each<br/>session}
    
    Iterate -->|Next| CalcAge[Calculate session age]
    CalcAge --> CheckExpired{Age ><br/>timeout?}
    
    CheckExpired -->|No| Iterate
    CheckExpired -->|Yes| Remove[Remove from cache]
    Remove --> Log[Log expiry]
    Log --> Iterate
    
    Iterate -->|Done| Unlock[Release cache lock]
    Unlock --> End[Cleanup complete]
    
    style Start fill:#e1f5ff
    style End fill:#d4edda
```

---

## Channel Isolation Architecture

### Multi-Channel Template Routing

```mermaid
flowchart TD
    Event[New Block Event] --> Identify[Identify block channel]
    
    Identify --> Prime{Channel 1<br/>Prime?}
    Identify --> Hash{Channel 2<br/>Hash?}
    
    Prime -->|Yes| FilterPrime[Filter sessions:<br/>Subscribed to Prime]
    Hash -->|Yes| FilterHash[Filter sessions:<br/>Subscribed to Hash]
    
    FilterPrime --> GenPrime[Generate Prime<br/>template]
    FilterHash --> GenHash[Generate Hash<br/>template]
    
    GenPrime --> SendPrime[Send NEW_BLOCK<br/>to Prime miners only]
    GenHash --> SendHash[Send NEW_BLOCK<br/>to Hash miners only]
    
    SendPrime --> End[Channel isolation<br/>prevents false staleness]
    SendHash --> End
    
    style Event fill:#e1f5ff
    style End fill:#d4edda
    style FilterPrime fill:#cfe2ff
    style FilterHash fill:#f8d7da
```

---

## Performance Optimization Flow

### Template Caching Strategy

```mermaid
flowchart TD
    Request[Template request] --> CheckCache{Template<br/>in cache?}
    
    CheckCache -->|Yes| CheckStale{Template<br/>stale?}
    CheckCache -->|No| Generate[Generate new template]
    
    CheckStale -->|No| Return1[Return cached<br/>template<br/><0.1ms]
    CheckStale -->|Yes| Generate
    
    Generate --> GetState[Get blockchain state]
    GetState --> CreateBlock[Create TritiumBlock]
    CreateBlock --> CalcMerkle[Calculate merkle root]
    CalcMerkle --> SignBlock[Sign with wallet]
    SignBlock --> Store[Store in cache]
    Store --> Return2[Return new template<br/><1ms]
    
    Return1 --> End[Template delivered]
    Return2 --> End
    
    style Request fill:#e1f5ff
    style End fill:#d4edda
    style Return1 fill:#d4edda
    style Return2 fill:#fff3cd
```

---

## Error Handling Flow

### Connection Error Recovery

```mermaid
flowchart TD
    Error[Connection error] --> Type{Error type?}
    
    Type -->|Timeout| HandleTimeout[Check keepalive]
    Type -->|Network error| HandleNetwork[Log network issue]
    Type -->|Protocol error| HandleProtocol[Log protocol violation]
    
    HandleTimeout --> CheckKeepalive{Keepalive<br/>recent?}
    CheckKeepalive -->|Yes| Reconnect[Attempt reconnection]
    CheckKeepalive -->|No| CloseSession[Close connection<br/>Preserve session]
    
    HandleNetwork --> CheckRetry{Retry<br/>possible?}
    CheckRetry -->|Yes| Reconnect
    CheckRetry -->|No| CloseSession
    
    HandleProtocol --> CloseSession
    
    Reconnect --> Success{Reconnect<br/>successful?}
    Success -->|Yes| RestoreState[Restore mining state]
    Success -->|No| CloseSession
    
    RestoreState --> End1[Resume mining]
    CloseSession --> PreserveCache[Session remains in cache<br/>until timeout]
    PreserveCache --> End2[Connection closed<br/>Can reconnect later]
    
    style Error fill:#f8d7da
    style End1 fill:#d4edda
    style End2 fill:#fff3cd
```

---

## Cross-References

**Related Documentation:**
- [Stateless Protocol](../current/mining/stateless-protocol.md)
- [Mining Server Architecture](../current/mining/mining-server.md)
- [Opcodes Reference](opcodes-reference.md)
- [Configuration Reference](nexus.conf.md)

**Miner Perspective:**
- [NexusMiner Flow Diagrams](https://github.com/Nexusoft/NexusMiner/blob/main/docs/reference/Flow-Architecture-Diagram-REF.md)
- [NexusMiner Protocol Flow](https://github.com/Nexusoft/NexusMiner/blob/main/docs/current/mining-protocols/stateless-mining.md)

---

## Version Information

**Document Version:** 1.0  
**Protocol Version:** Stateless Mining 1.0  
**LLL-TAO Version:** 5.1.0+  
**Last Updated:** 2026-01-13

---

*These diagrams represent the node's perspective. For miner-side diagrams, see [NexusMiner documentation](https://github.com/Nexusoft/NexusMiner/blob/main/docs/reference/Flow-Architecture-Diagram-REF.md).*
