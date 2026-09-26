# Epoll vs Poll Architecture Diagrams

**Version:** LLL-TAO 5.1.0+ (PR #543)  
**Last Updated:** 2026-04-11

---

## 1. Full System I/O Architecture

Shows how mining, P2P, and API connections use different I/O mechanisms and
DataThread pools.

```mermaid
graph TB
    subgraph Mining["Mining Connections"]
        M1["Miner 1<br>(port 9323)"]
        M2["Miner 2<br>(port 8323)"]
        MN["Miner N"]
    end

    subgraph P2P["P2P Connections"]
        P1["Peer 1<br>(port 9888)"]
        P2["Peer 2"]
    end

    subgraph API["API Connections"]
        A1["API Client<br>(port 8080)"]
    end

    M1 --> DT_M["Mining DataThread Pool<br>(is_mining_data_thread_v = true)"]
    M2 --> DT_M
    MN --> DT_M

    P1 --> DT_P["P2P DataThread Pool"]
    P2 --> DT_P

    A1 --> DT_A["API DataThread Pool"]

    DT_M -->|"Linux"| EPOLL["epoll_wait()<br>1 ms timeout<br>O(ready)"]
    DT_M -->|"Non-Linux<br>fallback"| POLL_M["poll()<br>10 ms timeout<br>O(n)"]
    DT_P --> POLL_P["poll()<br>100 ms timeout<br>O(n)"]
    DT_A --> POLL_A["poll()<br>100 ms timeout<br>O(n)"]

    EPOLL --> PROC["ProcessPacket()<br>Mining"]
    POLL_M --> PROC
    POLL_P --> PROC_P["ProcessPacket()<br>Tritium P2P"]
    POLL_A --> PROC_A["ProcessPacket()<br>API"]

    style DT_M fill:#d4edda,stroke:#28a745
    style DT_P fill:#cce5ff,stroke:#004085
    style DT_A fill:#fff3cd,stroke:#856404
    style EPOLL fill:#d4edda,stroke:#28a745,stroke-width:2px
```

---

## 2. Connection Lifecycle with Epoll Registration

Shows the sequence from TCP connect through mining to disconnect, including
epoll registration and deregistration.

```mermaid
sequenceDiagram
    participant Miner
    participant LT as ListeningThread
    participant DT as Mining DataThread
    participant EP as epoll (kernel)

    Miner->>LT: TCP connect (port 9323 or 8323)
    LT->>LT: accept4(SOCK_CLOEXEC)
    LT->>DT: AddConnection(socket, slot)
    DT->>EP: epoll_ctl(EPOLL_CTL_ADD, fd, {EPOLLIN, slot})
    Note over EP: FD registered for read events

    loop Mining Loop
        EP-->>DT: epoll_wait() returns ready FDs
        DT->>Miner: ReadPacket() — only if data ready
        DT->>DT: ProcessPacket()
        DT->>Miner: QueuePacket(template push)
    end

    Note over DT: Every 250ms: health sweep
    DT->>DT: check_connection_health() on ALL connections

    DT->>EP: epoll_ctl(EPOLL_CTL_DEL, fd)
    DT->>Miner: Disconnect / close socket
```

---

## 3. Dual-Lane Protocol Detection

Shows how `Miner::ReadPacket()` auto-detects the protocol lane based on the
first byte received from the miner.

```mermaid
graph TD
    A["Incoming Connection<br>Port 9323 (Stateless) or 8323 (Legacy)"] --> B["Miner::ReadPacket()<br>(virtual override)"]
    B --> C{"First opcode byte?"}
    C -->|"0xD0xx"| D["16-bit Stateless Frame<br>(Phase 2 protocol)"]
    C -->|"Other<br>(0x00–0xCF, 0xD1–0xFF)"| E["8-bit Legacy Frame<br>(Phase 1 protocol)"]
    D --> F["StatelessMiner::ProcessPacket()<br>Push notifications enabled"]
    E --> G["Legacy ProcessPacket()<br>GET_BLOCK polling mode"]

    style D fill:#d4edda,stroke:#28a745
    style E fill:#fff3cd,stroke:#856404
```

---

## 4. Thread Dispatch Decision

Shows how the DataThread constructor and Thread() entry point select between
epoll and poll paths at compile time and runtime.

```mermaid
graph TD
    A["DataThread<ProtocolType><br>Constructor"] --> B{"Compile-time check:<br>is_mining_data_thread_v?"}
    B -->|"false<br>(TritiumNode, APINode, ...)"| C["m_nEpollFd = -1<br>Use poll() path"]
    B -->|"true<br>(Miner, StatelessMinerConnection)"| D{"Runtime check:<br>__linux__ defined?"}
    D -->|"No<br>(macOS, Windows)"| C
    D -->|"Yes"| E["epoll_create1(EPOLL_CLOEXEC)"]
    E -->|"Success"| F["m_nEpollFd ≥ 0"]
    E -->|"Failure"| G["m_nEpollFd = -1<br>Log error, fall back"]
    F --> H["Thread() entry →<br>ThreadEpoll()"]
    G --> I["Thread() entry →<br>poll() with 10ms timeout"]
    C --> J["Thread() entry →<br>poll() with 100ms timeout"]

    style F fill:#d4edda,stroke:#28a745
    style H fill:#d4edda,stroke:#28a745
```

---

## 5. Epoll vs Poll Comparison

| Aspect | epoll (Mining, Linux) | poll() (Mining, Fallback) | poll() (P2P/API) |
|--------|----------------------|--------------------------|------------------|
| **Complexity** | O(ready) | O(n) | O(n) |
| **Timeout** | 1 ms (`-miningwait`) | 10 ms (`-miningwait`) | 100 ms (hardcoded) |
| **FD Registration** | Kernel-managed set | Rebuild every cycle | Rebuild every cycle |
| **Health Sweep** | Every 250 ms (decoupled) | Every iteration | Every iteration |
| **Platform** | Linux only | All platforms | All platforms |
| **Use Case** | Production mining | Dev/test mining | Production P2P |

---

## Related Documentation

- [Linux Epoll Mining Architecture](../../current/mining/linux-epoll-mining-architecture.md)
- [Dedicated DataThread Decision Record](../../current/mining/dedicated-datathread-decision.md)
- [Health Check Flow Diagram](health-check-flow.md)
