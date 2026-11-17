# Failover Event Sequence

Mermaid sequence diagram showing the full failover event sequence from primary node failure through reconnection on the failover node.

```mermaid
sequenceDiagram
    participant WM  as Worker_manager
    participant DCM as DualConnectionManager
    participant PN  as Primary_Node
    participant FN  as Failover_Node
    participant CA  as ColinAgent

    Note over WM,PN: Normal operation — SIM Link ACTIVE on primary node

    WM->>PN: [STATELESS lane] GET_BLOCK
    PN-->>WM: BLOCK_DATA (stateless packet)

    WM->>PN: [LEGACY lane] GET_BLOCK
    PN-->>WM: BLOCK_DATA (legacy packet)

    Note over WM,PN: Primary node drops (network failure)

    PN--xWM: connection closed / timeout

    WM->>DCM: set_lane_dead(stateless=true)
    WM->>DCM: set_lane_dead(stateless=false)
    DCM-->>WM: both lanes DEAD

    loop retry_connect() — exponential backoff
        WM->>PN: TCP connect attempt
        PN--xWM: connection refused / timeout
        WM->>DCM: increment_fail_count()
    end

    Note over WM,DCM: fail_count ≥ N — activating failover

    WM->>DCM: set_failover_active(true, "failover_ip:9323")
    DCM-->>WM: failover ACTIVE, active_endpoint = failover_ip:9323

    WM->>FN: TCP connect (STATELESS lane, port 9323)
    FN-->>WM: TCP ACK

    Note over WM,FN: Falcon re-authentication on failover node (fresh handshake)

    WM->>FN: MINER_AUTH_INIT (fresh, no session ID reuse)
    FN-->>WM: MINER_AUTH_CHALLENGE
    WM->>FN: MINER_AUTH_RESPONSE
    FN-->>WM: MINER_AUTH_RESULT (success, new session_id=Y)

    WM->>DCM: set_lane_alive(stateless=true)
    WM->>DCM: reset_fail_count()

    WM->>FN: TCP connect (LEGACY lane, port 8323)
    FN-->>WM: TCP ACK
    WM->>DCM: set_lane_alive(stateless=false)

    Note over WM,FN: SIM Link re-established on failover node

    DCM-->>WM: is_sim_link_active() = true

    CA->>DCM: get_active_endpoint()
    DCM-->>CA: "failover_ip:9323"
    CA->>DCM: get_secondary_endpoint()
    DCM-->>CA: "failover_ip:8323"
    CA->>DCM: is_using_failover()
    DCM-->>CA: true

    Note over CA: Colin report updated: active=failover_ip, SIM Link active, fail_count=0
```

## Key Points

- **No session ID reuse**: The failover node issues a brand-new session ID (`Y ≠ X`). The miner must perform a full `MINER_AUTH_INIT` handshake — see [falcon-reauth-failover.md](falcon-reauth-failover.md) for details.
- **Fail counter reset**: `reset_fail_count()` is called as soon as the failover connection is established and authenticated.
- **Colin reporting**: `ColinAgent` reads state from `DualConnectionManager` on each report cycle, so the transition is automatically reflected in the next report.
- **SIM Link re-establishment**: The LEGACY lane reconnects independently after the STATELESS lane is authenticated; `is_sim_link_active()` becomes `true` once both lanes are alive.
