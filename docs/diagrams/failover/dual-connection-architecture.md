# Dual-Connection Architecture

> **⚠️ Partial Update (2026-04-11):** `SessionRecoveryManager` was **removed** in PR #532.
> The `fFreshAuth` flag and `MarkFreshAuth()` API no longer exist.  On disconnect,
> miners perform a full Falcon re-authentication.  The diagram below retains the
> historical reference for context but marks it as removed.

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

    SRM["SessionRecoveryManager\n⛔ REMOVED (PR #532)\nFull re-auth on reconnect"]

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
    style SRM fill:#e2e3e5,stroke:#6c757d,stroke-dasharray: 5 5
    style CA  fill:#f8d7da,stroke:#dc3545
```

## Component Responsibilities

| Component | Responsibility |
|-----------|---------------|
| `Worker_manager` | Manages worker threads, block template distribution, and connection lifecycle. Calls `set_lane_alive/dead()` and `set_failover_active()`. |
| `DualConnectionManager` | Singleton that tracks lane health (STATELESS/LEGACY), failover state, active endpoint, and fail count. Source of truth for all failover decisions. |
| `FailoverConnectionTracker` (PR #336) | Node-side tracker recording which incoming connections originated from a failover switch. Used for Colin reporting of total failover events. |
| `SessionRecoveryManager` | **⛔ REMOVED (PR #532)** — Previously tracked `SessionRecoveryData` per miner address with `fFreshAuth` flag. Now miners perform full re-authentication on reconnect. |
| `ColinAgent` | Periodic reporting agent that reads from all three managers to emit the `COLIN NODE MINING REPORT`. |
| Primary / Failover Nodes | Remote Nexus nodes; miner connects to both STATELESS (9323) and LEGACY (8323) ports simultaneously when SIM Link is active. |

## Data Flow

1. `Worker_manager` maintains persistent TCP connections to both ports of the active node (SIM Link).
2. On connection failure, `set_lane_dead()` updates `DualConnectionManager`; `retry_connect()` begins backoff.
3. When `get_primary_fail_count() ≥ N`, `set_failover_active(true, failover_ip)` switches the active endpoint.
4. A fresh Falcon handshake is performed on the failover node (full re-authentication; `SessionRecoveryManager` was removed in PR #532).
5. `ColinAgent` reads the unified state on each report cycle and formats the `COLIN NODE MINING REPORT`.
