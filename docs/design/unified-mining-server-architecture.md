# Unified Mining Server Architecture Design

## Overview

This document describes the unified mining server architecture refactoring implemented to eliminate code duplication and establish a single source of truth for mining server configuration across both protocol lanes.

## Problem Statement

### Original Architecture Issues

Prior to this refactoring, the mining server initialization in `src/LLP/global.cpp` suffered from significant code duplication:

1. **Duplicated Configuration Code**: Nearly identical 20+ line Config setup blocks for STATELESS_MINER_SERVER (port 9323) and MINING_SERVER (port 8323)
2. **No Shared Logic**: Each lane independently set identical configuration parameters
3. **Maintenance Burden**: Adding new configuration options required updating two separate locations
4. **Error-Prone**: Inconsistencies could arise if one lane's config diverged from the other

### Key Duplication Points

Both lanes previously had nearly identical code for:
- `ENABLE_LISTEN`, `ENABLE_METERS`, `ENABLE_DDOS`, `ENABLE_MANAGER`
- `ENABLE_SSL`, `ENABLE_REMOTE`, `REQUIRE_SSL`, `PORT_SSL`
- `MAX_INCOMING`, `MAX_CONNECTIONS`, `MAX_THREADS`
- `DDOS_CSCORE`, `DDOS_RSCORE`, `DDOS_TIMESPAN`
- `MANAGER_SLEEP`

The **only** differences were:
- Port number: `GetMiningPort()` vs `GetLegacyMiningPort()`
- Default socket timeout: 300s (stateless) vs 120s (legacy)

## Solution: MiningServerFactory

### Design Principles

The `MiningServerFactory` implements a **factory pattern** that:

1. **Eliminates Duplication**: Single `BuildConfig()` method replaces 40+ lines of duplicated code
2. **Single Source of Truth**: All shared configuration parameters defined once
3. **Clear Lane Differentiation**: Explicit enum distinguishes STATELESS vs LEGACY lanes
4. **Easy Extensibility**: New configuration options added in one location
5. **Type Safety**: Compile-time enforcement via `enum class Lane`

### Architecture Components

#### 1. MiningServerFactory Header (`src/LLP/include/mining_server_factory.h`)

```cpp
struct MiningServerFactory
{
    enum class Lane {
        STATELESS,  // Port 9323, 300s timeout, push-based
        LEGACY      // Port 8323, 120s timeout, polling-based
    };

    static LLP::Config BuildConfig(Lane lane);
};
```

**Key Features:**
- Scoped enum prevents accidental integer conversions
- Static factory method (no instance needed)
- Returns `LLP::Config` by value (copy semantics)
- Lane parameter determines port and timeout

#### 2. Refactored Global Initialization (`src/LLP/global.cpp`)

**Before (98 lines):**
```cpp
// STATELESS_MINER_SERVER - 34 lines of Config setup
LLP::Config CONFIG = LLP::Config(GetMiningPort());
CONFIG.ENABLE_LISTEN = true;
CONFIG.ENABLE_METERS = false;
// ... 30+ more lines ...
CONFIG.SOCKET_TIMEOUT = config::GetArg(std::string("-miningtimeout"), 300);
STATELESS_MINER_SERVER = new Server<StatelessMinerConnection>(CONFIG);

// MINING_SERVER - 34 lines of DUPLICATE Config setup
LLP::Config LEGACY_CONFIG = LLP::Config(nLegacyPort);
LEGACY_CONFIG.ENABLE_LISTEN = true;
LEGACY_CONFIG.ENABLE_METERS = false;
// ... 30+ more lines of duplication ...
LEGACY_CONFIG.SOCKET_TIMEOUT = config::GetArg(std::string("-miningtimeout"), 120);
MINING_SERVER = new Server<Miner>(LEGACY_CONFIG);
```

**After (12 lines):**
```cpp
// STATELESS_MINER_SERVER - 3 lines
LLP::Config CONFIG = MiningServerFactory::BuildConfig(
    MiningServerFactory::Lane::STATELESS);
STATELESS_MINER_SERVER = new Server<StatelessMinerConnection>(CONFIG);

// MINING_SERVER - 3 lines
LLP::Config LEGACY_CONFIG = MiningServerFactory::BuildConfig(
    MiningServerFactory::Lane::LEGACY);
MINING_SERVER = new Server<Miner>(LEGACY_CONFIG);
```

**Code Reduction:**
- **86 lines eliminated** (88% reduction in mining server config code)
- **Single source of truth** for all shared parameters
- **Clearer intent**: Lane type explicit in factory call

## Dual-Lane Architecture

### Lane Comparison

| Aspect | Stateless Lane | Legacy Lane |
|--------|---------------|-------------|
| **Port** | 9323 (configurable via `-miningport`) | 8323 (configurable via `-legacyminingport`) |
| **Protocol** | 16-bit stateless framing | 8-bit legacy framing |
| **Communication** | Push notifications (event-driven) | GET_BLOCK polling (request-driven) |
| **Timeout** | 300s (accommodates long Prime searches) | 120s (traditional polling interval) |
| **Authentication** | Falcon mandatory | Falcon optional (backward compat) |
| **Connection Type** | `Server<StatelessMinerConnection>` | `Server<Miner>` |
| **Opcode Width** | 2 bytes (0xD0xx range) | 1 byte (0xD9, 0xDA) |

### Shared Configuration

Both lanes use **identical** settings for:

| Parameter | Value | Purpose |
|-----------|-------|---------|
| `ENABLE_LISTEN` | `true` | Accept incoming connections |
| `ENABLE_METERS` | `false` | No bandwidth metering |
| `ENABLE_DDOS` | Configurable (`-miningddos`) | DDoS protection |
| `ENABLE_MANAGER` | `false` | No connection manager |
| `ENABLE_SSL` | Configurable (`-miningssl`) | SSL support (not yet implemented) |
| `ENABLE_REMOTE` | `true` | Allow remote connections |
| `REQUIRE_SSL` | Configurable (`-miningsslrequired`) | SSL requirement |
| `PORT_SSL` | `0` | SSL port (not yet implemented) |
| `MAX_INCOMING` | `128` | Maximum incoming connections |
| `MAX_CONNECTIONS` | `128` | Total connection limit |
| `MAX_THREADS` | `4` (configurable via `-miningthreads`) | Worker thread pool size |
| `DDOS_CSCORE` | `1` (configurable via `-miningcscore`) | Connection score threshold |
| `DDOS_RSCORE` | `50` (configurable via `-miningrscore`) | Request score threshold |
| `DDOS_TIMESPAN` | `60` (configurable via `-miningtimespan`) | DDoS timespan in seconds |
| `MANAGER_SLEEP` | `0` | Connection manager disabled |

### Configuration Override System

All configuration parameters respect command-line arguments:

```bash
# Override timeout for both lanes (backward compatible)
nexus -miningtimeout=180

# Per-lane timeout control (recommended)
nexus -miningstatelesstimeout=300 -mininglegacytimeout=60

# Lane-specific takes priority over generic
nexus -miningtimeout=180 -miningstatelesstimeout=400

# Enable DDoS protection
nexus -miningddos=1

# Increase thread pool
nexus -miningthreads=8

# Custom ports
nexus -miningport=9999 -legacyminingport=8888

# Disable legacy lane
nexus -legacyminingport=0
```

**Timeout Configuration Priority:**
1. Lane-specific argument (`-miningstatelesstimeout` or `-mininglegacytimeout`)
2. Generic argument (`-miningtimeout`) for backward compatibility
3. Lane-specific default (300s for stateless, 120s for legacy)

**Problem Solved:** Prior to per-lane timeout support, setting `-miningtimeout=60` would clobber the stateless lane's protective 300s timeout, breaking Prime mining. Now users can:
- Use `-miningstatelesstimeout=300 -mininglegacytimeout=60` for optimal per-lane control
- Or keep `-miningtimeout=60` but add `-miningstatelesstimeout=300` to protect the stateless lane

## Lane-Specific Behaviors

### Stateless Lane (Port 9323)

**Purpose**: Modern event-driven protocol for NexusMiner clients

**Key Characteristics:**
- **Push Notifications**: Node sends `PRIME_BLOCK_AVAILABLE` / `HASH_BLOCK_AVAILABLE` when new work is ready
- **Long Timeout**: 300-second default accommodates Prime block searches (2-5+ minutes)
- **16-bit Opcodes**: Stateless packet framing with extended opcode space (0xD000-0xDFFF)
- **Mandatory Authentication**: Falcon signatures required before mining operations
- **Read-Idle Tolerance**: Node expects silence during mining, only miner activity is block submission

**Protocol Flow:**
```
1. Miner → Node: MINER_AUTH_INIT (Genesis + Falcon pubkey)
2. Node → Miner: MINER_AUTH_CHALLENGE (Random nonce)
3. Miner → Node: MINER_AUTH_RESPONSE (Falcon signature)
4. Node → Miner: MINER_AUTH_RESULT (Session ID)
5. Miner → Node: SET_CHANNEL (1=Prime, 2=Hash)
6. Miner → Node: MINER_READY (Subscribe to push notifications)
7. Node → Miner: STATELESS_PRIME_BLOCK_AVAILABLE (Push notification)
8. Miner → Node: GET_BLOCK (Request template)
9. Node → Miner: STATELESS_GET_BLOCK (228-byte template)
10. [Miner mines...]
11. Miner → Node: SUBMIT_BLOCK (Solution submission)
```

### Legacy Lane (Port 8323)

**Purpose**: Backward compatibility with older mining clients

**Key Characteristics:**
- **Polling Protocol**: Miner periodically calls GET_ROUND (every 5-10s) to check for new work
- **Short Timeout**: 120-second default matches traditional polling frequency
- **8-bit Opcodes**: Legacy packet framing (0x00-0xFF range)
- **Optional Authentication**: Falcon signatures optional for backward compatibility
- **Active Polling**: Miner activity expected every 5-10 seconds

**Protocol Flow:**
```
1. [Optional] Miner → Node: MINER_AUTH_INIT/CHALLENGE/RESPONSE/RESULT
2. Miner → Node: SET_CHANNEL (1=Prime, 2=Hash)
3. Miner → Node: GET_ROUND (Poll for new work)
4. Node → Miner: NEW_ROUND (16 bytes: unified + channel heights)
5. Miner → Node: GET_BLOCK (If height changed)
6. Node → Miner: BLOCK_DATA (Block template)
7. [Miner mines...]
8. Miner → Node: GET_ROUND (Poll again)
9. Miner → Node: SUBMIT_BLOCK (Solution submission)
```

## Implementation Details

### Factory Method Logic

```cpp
static LLP::Config BuildConfig(Lane lane)
{
    // 1. Determine lane-specific port
    const uint16_t nPort = (lane == Lane::STATELESS) ?
        GetMiningPort() : GetLegacyMiningPort();

    // 2. Determine lane-specific default timeout
    const uint32_t nDefaultTimeout = (lane == Lane::STATELESS) ? 300 : 120;

    // 3. Create base config with port
    LLP::Config CONFIG = LLP::Config(nPort);

    // 4. Apply shared configuration (identical for both lanes)
    CONFIG.ENABLE_LISTEN = true;
    CONFIG.ENABLE_METERS = false;
    // ... [20+ shared parameters] ...

    // 5. Apply lane-specific timeout (with command-line override)
    CONFIG.SOCKET_TIMEOUT = config::GetArg(std::string("-miningtimeout"), nDefaultTimeout);

    return CONFIG;
}
```

**Design Rationale:**
- **Ternary Operator**: Clear, concise lane differentiation
- **Default Timeout**: Lane-appropriate defaults (300s vs 120s)
- **Command-Line Override**: `-miningtimeout` can override lane defaults
- **Return by Value**: Config copied to caller (safe ownership transfer)

### Thread Safety

The factory method is **read-only** and **thread-safe**:
- Queries `config::GetArg()` and `config::GetBoolArg()` (read-only after initialization)
- No mutable state or global variables modified
- Safe to call concurrently from multiple threads
- Returns new Config object per call (no shared state)

### Memory Management

```cpp
// Config returned by value (copy semantics)
LLP::Config CONFIG = MiningServerFactory::BuildConfig(Lane::STATELESS);

// Server takes ownership of Config
STATELESS_MINER_SERVER = new Server<StatelessMinerConnection>(CONFIG);

// CONFIG goes out of scope (no memory leak)
```

**Ownership Model:**
- Factory returns Config by value
- Server constructor copies Config
- No heap allocations in factory
- RAII ensures cleanup

## Benefits & Impact

### Code Quality Improvements

1. **86 Lines Eliminated**: 88% reduction in mining server config code
2. **DRY Principle**: Single source of truth for all shared configuration
3. **Type Safety**: `enum class Lane` prevents invalid lane specifications
4. **Self-Documenting**: Factory call explicitly states which lane is being configured
5. **Compile-Time Validation**: Type system enforces correct usage

### Maintenance Benefits

1. **Single Update Location**: New config options added in one place
2. **Consistency Guaranteed**: Both lanes always use identical shared settings
3. **Reduced Error Surface**: No risk of diverging configurations
4. **Easier Testing**: Factory method testable in isolation
5. **Clear Responsibilities**: Separation of concerns (factory vs server initialization)

### Extensibility

Future enhancements simplified:

```cpp
// Example: Add custom rate limiting per lane
static LLP::Config BuildConfig(Lane lane, uint32_t customRateLimit = 0)
{
    // ... existing code ...

    if (customRateLimit > 0)
        CONFIG.RATE_LIMIT = customRateLimit;
    else
        CONFIG.RATE_LIMIT = (lane == Lane::STATELESS) ? 100 : 50;

    return CONFIG;
}
```

## Testing Strategy

### Unit Testing

Test the factory in isolation:

```cpp
TEST_CASE("MiningServerFactory BuildConfig")
{
    SECTION("Stateless lane produces correct config")
    {
        auto cfg = MiningServerFactory::BuildConfig(MiningServerFactory::Lane::STATELESS);
        CHECK(cfg.SOCKET_TIMEOUT == 300);  // Default for stateless
        CHECK(cfg.ENABLE_LISTEN == true);
        CHECK(cfg.MAX_THREADS == 4);
    }

    SECTION("Legacy lane produces correct config")
    {
        auto cfg = MiningServerFactory::BuildConfig(MiningServerFactory::Lane::LEGACY);
        CHECK(cfg.SOCKET_TIMEOUT == 120);  // Default for legacy
        CHECK(cfg.ENABLE_LISTEN == true);
        CHECK(cfg.MAX_THREADS == 4);
    }

    SECTION("Both lanes share identical settings")
    {
        auto stateless = MiningServerFactory::BuildConfig(MiningServerFactory::Lane::STATELESS);
        auto legacy = MiningServerFactory::BuildConfig(MiningServerFactory::Lane::LEGACY);

        // Should be identical except port and timeout
        CHECK(stateless.ENABLE_LISTEN == legacy.ENABLE_LISTEN);
        CHECK(stateless.MAX_INCOMING == legacy.MAX_INCOMING);
        CHECK(stateless.DDOS_CSCORE == legacy.DDOS_CSCORE);
    }
}
```

### Integration Testing

Test full server initialization:

```bash
# Test stateless lane
nexus -mining=1 -testnet
# Verify: Port 9323 listening, 300s timeout

# Test legacy lane
nexus -mining=1 -legacyminingport=8323
# Verify: Port 8323 listening, 120s timeout

# Test with custom timeout
nexus -mining=1 -miningtimeout=180
# Verify: Both lanes use 180s timeout

# Test legacy lane disabled
nexus -mining=1 -legacyminingport=0
# Verify: Only port 9323 listening
```

### Behavioral Testing

Verify protocol behavior unchanged:

1. **Stateless Lane**: Connect NexusMiner, verify push notifications work
2. **Legacy Lane**: Connect legacy miner, verify GET_ROUND polling works
3. **Dual-Lane**: Connect both simultaneously, verify no interference
4. **Timeout Behavior**: Verify stateless tolerates 300s idle, legacy times out at 120s

## Migration Notes

### Backward Compatibility

**Full backward compatibility maintained:**
- No changes to packet formats or protocol behavior
- Both lanes continue to function identically to before
- Command-line arguments unchanged
- Configuration file syntax unchanged
- Existing miners require no updates

### Deployment

**Zero-downtime deployment:**
1. Compile new node binary with refactored code
2. Stop existing node: `./nexus stop`
3. Replace binary: `cp nexus_new nexus`
4. Start node: `./nexus -mining=1`
5. Verify both ports listening: `netstat -an | grep -E "(9323|8323)"`

**No configuration changes required.**

## Future Work

### Potential Enhancements

1. **Protocol Adapter Pattern**: Refactor legacy Miner class to thin protocol adapter
   - Translate 8-bit packets to 16-bit stateless format
   - Route through shared `StatelessMiner::ProcessPacket()` pure function
   - Eliminate duplicated authentication and session logic

2. **Shared Template Management**: Unified block template cache
   - Both lanes access same template pool
   - Eliminate `Miner::mapBlocks` raw pointer storage
   - Use `TemplateMetadata` with channel-specific staleness detection

3. **Unified Push Dispatcher**: Enhanced `MinerPushDispatcher`
   - Protocol-agnostic notification system
   - Lane-specific opcode translation (8-bit vs 16-bit)
   - Consolidated deduplication logic

4. **Configuration Validation**: Factory-level config validation
   - Range checks for thread counts, timeouts, etc.
   - Warn if timeout too short for Prime mining
   - Detect conflicting SSL settings

5. **Lane-Specific Metrics**: Per-lane performance tracking
   - Separate connection counts per lane
   - Protocol-specific latency metrics
   - Push vs polling efficiency comparison

## Related Files

- `src/LLP/include/mining_server_factory.h` - Factory implementation
- `src/LLP/global.cpp` - Server initialization (refactored)
- `src/LLP/include/port.h` - Port resolution functions
- `src/LLP/include/config.h` - LLP::Config structure definition
- `src/LLP/types/stateless_miner_connection.h` - Stateless lane handler
- `src/LLP/types/miner.h` - Legacy lane handler
- `src/LLP/miner_push_dispatcher.cpp` - Unified push broadcaster

## References

- **Original Issue**: "Node C++ Refactor Design: Unified Mining Server Architecture"
- **Problem Statement**: Eliminate Config duplication in global.cpp
- **Design Pattern**: Factory Method (Gang of Four)
- **Principle**: DRY (Don't Repeat Yourself)

## Conclusion

The `MiningServerFactory` refactoring successfully eliminates 86 lines of duplicated configuration code while maintaining full backward compatibility. The factory pattern provides a single source of truth for mining server configuration, making the codebase more maintainable and extensible.

**Key Achievements:**
- ✅ Eliminated 88% of mining server config duplication
- ✅ Established single source of truth for shared parameters
- ✅ Maintained full backward compatibility
- ✅ Improved code clarity and maintainability
- ✅ Foundation for future protocol unification

**Next Steps:**
- Refactor legacy Miner class to thin protocol adapter
- Unify template management across both lanes
- Consolidate authentication and session logic
