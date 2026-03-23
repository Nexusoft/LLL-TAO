# DualConnectionManager: Failover Integration with SIM Link

## Overview

The `DualConnectionManager` provides centralized tracking of connection lane health and failover state for the Nexus mining node. It coordinates with the Colin Mining Agent to provide real-time visibility into SIM Link dual-connection status and automatic failover between primary and backup mining pool nodes.

---

## ⚠️ SIM-LINK Deprecation Notice

The SIM-LINK cross-lane fallback mechanism — where templates issued on one lane
(port 9323 or 8323) can be resolved by `SUBMIT_BLOCK` arriving on the other lane — is
**deprecated** and scheduled for removal once the real second-node failover model
(`DualConnectionManager`) is complete.

### Why It Is Deprecated

During a rare dry spell (~600 s with no blocks mined on any channel), the miner entered
DEGRADED MODE because its template aged past the hard `MAX_TEMPLATE_AGE_SECONDS = 600 s`
cutoff. The SIM-LINK legacy lane (port 8323) was acting as a fallback when stateless
push notifications stalled, but this created an implicit dependency on two ports to the
same node rather than a true second-node failover.

### Node-Side Mitigation: Proactive Heartbeat Refresh

The node now implements a **proactive template heartbeat refresh** to prevent the 600 s
timeout from triggering during legitimate dry spells:

| Elapsed since last push | Action |
|-------------------------|--------|
| 300 s | NOTICE log: dry spell detected |
| 450 s | WARNING log: approaching refresh threshold |
| 480 s | **HEARTBEAT fires**: template re-pushed to all subscribed miners |
| 550 s | CRITICAL log (if heartbeat did not reset the clock, e.g. no miners connected) |
| 600 s | Hard cutoff: template expired (miner enters degraded mode) |

The heartbeat is driven by `MinerPushDispatcher::HeartbeatRefreshCheck()`, called every
`HEARTBEAT_CHECK_INTERVAL_SECONDS` (60 s) from `Server<ProtocolType>::Meter()`.

```cpp
// src/LLP/include/falcon_constants.h
static const uint64_t TEMPLATE_HEARTBEAT_REFRESH_SECONDS = 480;  // fires at 8 min
static const uint64_t MAX_TEMPLATE_AGE_SECONDS            = 600;  // hard cutoff at 10 min

// src/LLP/include/mining_constants.h
constexpr uint64_t HEARTBEAT_CHECK_INTERVAL_SECONDS = 60;         // check every 60 s
```

### Disabling the SIM-LINK Fallback

To disable the cross-lane `FindSessionBlock` resolution path and test without it:

```
./nexus -deprecate-simlink-fallback=1
```

When set, block submissions on either lane that cannot find a local template will be
**rejected outright** (no cross-lane lookup), matching the behaviour of the planned
second-node failover model.

---

## Architecture

### SIM Link Dual-Connection Model

NexusMiner uses a dual-connection architecture called "SIM Link":
- **Primary Lane (STATELESS)**: Port 9323, 16-bit stateless protocol
- **Secondary Lane (LEGACY)**: Port 8323, 8-bit legacy protocol

Both lanes connect to the same node and submit the same block solutions simultaneously. The node deduplicates submissions using `ColinMiningAgent::check_and_record_submission()`.

### Failover Behavior

| Event | Response |
|-------|----------|
| Primary 9323 drops | SIM Link: secondary 8323 keeps workers alive; GET_BLOCK sent on legacy lane |
| Both ports on primary drop | Failover: switch to failover node:9323, SIM Link re-establishes on failover |
| Failover also drops (N retries) | Cycle back to primary, exponential backoff |
| Any successful connect | Reset fail counter, preserve `m_using_failover` flag for correct side tracking |

## DualConnectionManager API

### Singleton Access

```cpp
#include <LLP/include/dual_connection_manager.h>

DualConnectionManager& dcm = DualConnectionManager::Get();
```

### Lane Health Tracking

#### Mark Lane Alive

```cpp
// Called when a connection succeeds
dcm.set_lane_alive(true);   // STATELESS lane (9323) connected
dcm.set_lane_alive(false);  // LEGACY lane (8323) connected
```

#### Mark Lane Dead

```cpp
// Called when a connection drops
dcm.set_lane_dead(true);    // STATELESS lane (9323) disconnected
dcm.set_lane_dead(false);   // LEGACY lane (8323) disconnected
```

#### Query Lane Status

```cpp
bool stateless_up = dcm.is_stateless_lane_alive();
bool legacy_up = dcm.is_legacy_lane_alive();
bool sim_link_active = dcm.is_sim_link_active();  // Both lanes alive
```

### Failover State Management

#### Set Active Node

```cpp
// Called when failover state changes
dcm.set_failover_active(false, "192.168.1.10:9323");  // Using primary
dcm.set_failover_active(true, "192.168.1.11:9323");   // Using failover
```

#### Query Failover State

```cpp
bool using_failover = dcm.is_using_failover();
std::string primary_endpoint = dcm.get_active_endpoint();      // e.g., "192.168.1.10:9323"
std::string secondary_endpoint = dcm.get_secondary_endpoint(); // e.g., "192.168.1.10:8323"
```

### Failure Counter

```cpp
// Increment on connection failure
dcm.increment_fail_count();

// Reset on successful connection
dcm.reset_fail_count();

// Query current count
uint32_t fails = dcm.get_primary_fail_count();
```

## Integration with Colin Mining Agent

The Colin Mining Agent now displays failover status in its periodic report:

```
╔══════════════════════════════════════════════════════════════════╗
║  COLIN NODE MINING REPORT  [2026-03-03 12:34:56]                ║
╠══════════════════════════════════════════════════════════════════╣
║  CONNECTED MINERS: 1                                              ║
╠══════════════════════════════════════════════════════════════════╣
║  NODE CONNECTION STATUS:                                          ║
║    Active Node: 192.168.1.10:9323 [PRIMARY]                       ║
║    SIM Link Secondary: 192.168.1.10:8323                          ║
║    Lane Status: STATELESS(9323)=ALIVE, LEGACY(8323)=ALIVE [SIM LINK ACTIVE] ║
╠══════════════════════════════════════════════════════════════════╣
```

When failover activates:

```
║  NODE CONNECTION STATUS:                                          ║
║    Active Node: 192.168.1.11:9323 [FAILOVER]                      ║
║    SIM Link Secondary: 192.168.1.11:8323                          ║
║    Lane Status: STATELESS(9323)=ALIVE, LEGACY(8323)=ALIVE [SIM LINK ACTIVE] ║
║    Fail Count: 3 consecutive failures                             ║
```

## Implementation Notes

### Thread Safety

All `DualConnectionManager` methods are thread-safe:
- Lane liveness uses `std::atomic<bool>` for lock-free reads
- Failover state and endpoint updates are protected by `std::mutex`
- Logging is performed under lock to prevent interleaved messages

### Secondary Endpoint Derivation

The secondary (LEGACY) endpoint is automatically derived from the primary (STATELESS) endpoint by replacing port 9323 with 8323:

```cpp
// Primary: 192.168.1.10:9323
// Secondary: 192.168.1.10:8323
std::string secondary = dcm.get_secondary_endpoint();
```

This ensures the SIM Link secondary connection always points to the correct node after failover.

### Logging

The manager logs state transitions at verbosity level 0 (always visible):
- Lane transitions (ALIVE ↔ DEAD)
- Failover state changes (PRIMARY ↔ FAILOVER)
- SIM Link activation
- Fail counter resets

## Usage Example: Miner Connection Handlers

### On Connection Success

```cpp
void StatelessMinerConnection::OnConnect()
{
    // Mark STATELESS lane as alive
    DualConnectionManager::Get().set_lane_alive(true);

    // Reset fail counter on successful connection
    DualConnectionManager::Get().reset_fail_count();
}

void Miner::OnConnect()  // Legacy connection
{
    // Mark LEGACY lane as alive
    DualConnectionManager::Get().set_lane_alive(false);
}
```

### On Connection Failure

```cpp
void StatelessMinerConnection::OnDisconnect()
{
    // Mark STATELESS lane as dead
    DualConnectionManager::Get().set_lane_dead(true);

    // Increment fail counter
    DualConnectionManager::Get().increment_fail_count();
}

void Miner::OnDisconnect()  // Legacy connection
{
    // Mark LEGACY lane as dead
    DualConnectionManager::Get().set_lane_dead(false);
}
```

### Failover Activation (Miner-Side)

```cpp
void Worker_manager::retry_connect()
{
    DualConnectionManager& dcm = DualConnectionManager::Get();

    // Check if both lanes are down
    if(!dcm.is_stateless_lane_alive() && !dcm.is_legacy_lane_alive())
    {
        uint32_t fail_count = dcm.get_primary_fail_count();

        // After N failures, flip to failover
        if(fail_count >= MAX_RETRY_COUNT)
        {
            bool using_failover = dcm.is_using_failover();

            // Toggle between primary and failover
            if(!using_failover)
            {
                // Switch to failover
                dcm.set_failover_active(true, "192.168.1.11:9323");
                retry_secondary_connect();  // Re-point SIM Link secondary
            }
            else
            {
                // Cycle back to primary
                dcm.set_failover_active(false, "192.168.1.10:9323");
                retry_secondary_connect();
            }

            dcm.reset_fail_count();  // Reset for next round
        }
    }
}

void Worker_manager::retry_secondary_connect()
{
    DualConnectionManager& dcm = DualConnectionManager::Get();
    std::string secondary_ep = dcm.get_secondary_endpoint();

    // Reconnect LEGACY lane to failover node's secondary port
    connect_to(secondary_ep);
}
```

## Testing

### Unit Test Structure

```cpp
TEST_CASE("DualConnectionManager lane tracking", "[llp]")
{
    DualConnectionManager& dcm = DualConnectionManager::Get();

    // Initial state: both lanes dead
    REQUIRE(!dcm.is_stateless_lane_alive());
    REQUIRE(!dcm.is_legacy_lane_alive());
    REQUIRE(!dcm.is_sim_link_active());

    // Connect STATELESS lane
    dcm.set_lane_alive(true);
    REQUIRE(dcm.is_stateless_lane_alive());
    REQUIRE(!dcm.is_sim_link_active());

    // Connect LEGACY lane → SIM Link active
    dcm.set_lane_alive(false);
    REQUIRE(dcm.is_sim_link_active());

    // Disconnect STATELESS → degraded mode
    dcm.set_lane_dead(true);
    REQUIRE(!dcm.is_stateless_lane_alive());
    REQUIRE(dcm.is_legacy_lane_alive());
}

TEST_CASE("DualConnectionManager failover tracking", "[llp]")
{
    DualConnectionManager& dcm = DualConnectionManager::Get();

    // Set primary active
    dcm.set_failover_active(false, "192.168.1.10:9323");
    REQUIRE(!dcm.is_using_failover());
    REQUIRE(dcm.get_active_endpoint() == "192.168.1.10:9323");
    REQUIRE(dcm.get_secondary_endpoint() == "192.168.1.10:8323");

    // Simulate failures
    dcm.increment_fail_count();
    dcm.increment_fail_count();
    dcm.increment_fail_count();
    REQUIRE(dcm.get_primary_fail_count() == 3);

    // Switch to failover
    dcm.set_failover_active(true, "192.168.1.11:9323");
    REQUIRE(dcm.is_using_failover());
    REQUIRE(dcm.get_active_endpoint() == "192.168.1.11:9323");
    REQUIRE(dcm.get_secondary_endpoint() == "192.168.1.11:8323");

    // Reset on successful connection
    dcm.reset_fail_count();
    REQUIRE(dcm.get_primary_fail_count() == 0);
}
```

## Configuration

No configuration is required. The `DualConnectionManager` is a singleton that activates automatically when the node starts.

Miner-side configuration (NexusMiner):
```conf
# Primary node
mining_node=192.168.1.10:9323

# Failover node
failover_node=192.168.1.11:9323

# Max retries before failover
max_retry_count=3
```

## See Also

- [Colin Mining Agent](./mining-notification-diagnostics.md)
- [SIM Link Architecture](./stateless-protocol.md)
- [Session Status Protocol](./session-status.md)
- [Failover Re-Authentication Flow](./failover-reauth-flow.md)

---

## Node-Side Failover Re-Authentication

> **Important:** When a miner switches to a failover node, the failover node has
> **no prior session** for that miner. The miner must perform a **full fresh Falcon
> authentication handshake**. The `DualConnectionManager` (above) tracks lane
> health from the miner's perspective; the node observes the failover via the
> `FailoverConnectionTracker` singleton and reports it in the Colin session state.
>
> See [failover-reauth-flow.md](./failover-reauth-flow.md) for the complete
> protocol description including the correct re-authentication sequence and how the
> Colin report's **Miner Session State** section shows `fresh_auth=true` for
> failover sessions.

### Protocol Summary

```
Miner → Failover Node: TCP connect (no session ID)
  Node: RecoverSessionByAddress() → nullopt
  Node: FailoverConnectionTracker::RecordConnection(IP)

Miner → Node: MINER_AUTH_INIT (full pubkey)
Node → Miner: MINER_AUTH_CHALLENGE (nonce)

Miner → Node: MINER_AUTH_RESPONSE (Falcon signature)
  Node: ProcessFalconResponse() → derives new nSessionId from pubkey hash
  Node: ChannelStateManager::NotifyFailoverConnection() [if failover detected]

Node → Miner: MINER_AUTH_RESULT (success + new nSessionId)
Miner → Node: STATELESS_MINER_READY (subscribe to templates)
  [Mining resumes on failover node]
```

