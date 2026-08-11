# SIM Link Lane Health States

Mermaid state diagram showing all dual-lane health states and transitions for the SIM Link architecture.

```mermaid
stateDiagram-v2
    [*] --> BOTH_DEAD : initial state (no connections)

    BOTH_DEAD     --> STATELESS_ONLY : set_lane_alive(stateless=true)\nSTATELESS lane (9323) connected
    BOTH_DEAD     --> LEGACY_ONLY    : set_lane_alive(stateless=false)\nLEGACY lane (8323) connected

    STATELESS_ONLY --> BOTH_DEAD      : set_lane_dead(stateless=true)\nSTATELESS dropped
    STATELESS_ONLY --> SIM_LINK_ACTIVE: set_lane_alive(stateless=false)\nLEGACY also connects

    LEGACY_ONLY --> BOTH_DEAD        : set_lane_dead(stateless=false)\nLEGACY dropped
    LEGACY_ONLY --> SIM_LINK_ACTIVE  : set_lane_alive(stateless=true)\nSTATELESS also connects

    SIM_LINK_ACTIVE --> STATELESS_ONLY : set_lane_dead(stateless=false)\nLEGACY lane (8323) dropped
    SIM_LINK_ACTIVE --> LEGACY_ONLY    : set_lane_dead(stateless=true)\nSTATELESS lane (9323) dropped

    note right of BOTH_DEAD
        is_sim_link_active() = false
        Both lanes report DEAD.
        Failover trigger candidate.
    end note

    note right of SIM_LINK_ACTIVE
        is_sim_link_active() = true
        Full SIM Link: workers receive
        block templates from either lane.
        Maximum hash-rate resilience.
    end note
```

## Lane State Descriptions

| State | STATELESS (9323) | LEGACY (8323) | SIM Link |
|-------|-----------------|---------------|----------|
| `BOTH_DEAD` | ✗ dead | ✗ dead | inactive |
| `STATELESS_ONLY` | ✓ alive | ✗ dead | inactive |
| `LEGACY_ONLY` | ✗ dead | ✓ alive | inactive |
| `SIM_LINK_ACTIVE` | ✓ alive | ✓ alive | **ACTIVE** |

## SIM Link Semantics

When `SIM_LINK_ACTIVE`:
- Workers connected via both STATELESS (9323) and LEGACY (8323) lanes simultaneously.
- GET_BLOCK requests can be served on either lane; responses are unified.
- `get_secondary_endpoint()` returns the LEGACY endpoint derived from the active primary.
- If one lane drops, workers seamlessly continue on the surviving lane (no failover needed).

When transitioning **out** of `SIM_LINK_ACTIVE` to `BOTH_DEAD`:
- `failover_connection_tracker` increments the total failover count.
- `retry_connect()` begins exponential back-off.
- Failover state machine transitions to `PRIMARY_FAILING`.
