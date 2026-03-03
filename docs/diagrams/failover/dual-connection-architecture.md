# Dual-Connection Architecture

Component architecture diagram for the NexusMiner dual-connection / SIM Link / failover subsystem.

```mermaid
graph TD
    NM[NexusMiner]

    NM --> WM[Worker_manager]

    WM --> DCM[DualConnectionManager\nsingleton]
    WM --> PN_S["Primary_Node\nSTATELESS lane\n(port 9323)"]
    WM --> PN_L["Primary_Node\nLEGACY lane\n(port 8323)"]
    WM --> FN_S["Failover_Node\nSTATELESS lane\n(port 9323)"]
    WM --> FN_L["Failover_Node\nLEGACY lane\n(port 8323)"]

    PN_S & PN_L -- "set_lane_alive/dead()" --> DCM
    FN_S & FN_L -- "set_lane_alive/dead()" --> DCM

    CA[ColinAgent] -- "reads state\nget_active_endpoint()\nget_secondary_endpoint()\nis_using_failover()\nget_primary_fail_count()" --> DCM

    FCT[FailoverConnectionTracker\nPR #336] -- "RecordConnection()\nIsFailover()\nGetTotalFailoverCount()" --> CA

    SRM[SessionRecoveryManager] -- "RecoverSessionByAddress()\nMarkFreshAuth()" --> CA

    subgraph SIM_Link ["SIM Link (both lanes alive)"]
        PN_S
        PN_L
    end

    subgraph Failover_Node_Group ["Failover Node (activated on N failures)"]
        FN_S
        FN_L
    end

    style DCM fill:#d4edda,stroke:#28a745
    style FCT fill:#d1ecf1,stroke:#17a2b8
    style SRM fill:#fff3cd,stroke:#ffc107
    style CA  fill:#f8d7da,stroke:#dc3545
```

## Component Responsibilities

| Component | Responsibility |
|-----------|---------------|
| `Worker_manager` | Manages worker threads, block template distribution, and connection lifecycle. Calls `set_lane_alive/dead()` and `set_failover_active()`. |
| `DualConnectionManager` | Singleton that tracks lane health (STATELESS/LEGACY), failover state, active endpoint, and fail count. Source of truth for all failover decisions. |
| `FailoverConnectionTracker` (PR #336) | Node-side tracker recording which incoming connections originated from a failover switch. Used for Colin reporting of total failover events. |
| `SessionRecoveryManager` | Tracks `SessionRecoveryData` per miner address; `fFreshAuth` flag set after a confirmed failover Falcon handshake. |
| `ColinAgent` | Periodic reporting agent that reads from all three managers to emit the `COLIN NODE MINING REPORT`. |
| Primary / Failover Nodes | Remote Nexus nodes; miner connects to both STATELESS (9323) and LEGACY (8323) ports simultaneously when SIM Link is active. |

## Data Flow

1. `Worker_manager` maintains persistent TCP connections to both ports of the active node (SIM Link).
2. On connection failure, `set_lane_dead()` updates `DualConnectionManager`; `retry_connect()` begins backoff.
3. When `get_primary_fail_count() ≥ N`, `set_failover_active(true, failover_ip)` switches the active endpoint.
4. A fresh Falcon handshake is performed on the failover node; `SessionRecoveryManager.MarkFreshAuth()` is called.
5. `ColinAgent` reads the unified state on each report cycle and formats the `COLIN NODE MINING REPORT`.
