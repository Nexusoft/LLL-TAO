# Copilot Build Instructions for Nexus LLL-TAO

## Overview
Nexus LLL-TAO is a blockchain node implementation written in C++17. It requires specific dependencies for compilation and testing.

## Build Environment

### Operating System
- **Primary:** Ubuntu 22.04 LTS
- **Minimum:** Ubuntu 20.04, Debian 11, or equivalent
- **Alternative:** macOS 12+ (requires Homebrew)

### Compiler Requirements
- **GCC:** 11.0 or later (recommended: 11.4)
- **Clang:** 14.0 or later
- **Standard:** C++17 with full support

### Core Dependencies

#### Required Libraries
| Library | Version | Package | Purpose |
|---------|---------|---------|---------|
| Berkeley DB | 5.3.x | `libdb5.3++-dev` | Blockchain database storage |
| OpenSSL | 1.1.1+ | `libssl-dev` | Cryptographic operations (hashing, signatures) |
| Boost | 1.71+ | `libboost-all-dev` | Utilities, threading, filesystem |
| MiniUPnP | 2.1+ | `libminiupnpc-dev` | NAT traversal for P2P networking |
| libevent | 2.1+ | `libevent-dev` | Asynchronous I/O and event handling |

#### Optional Dependencies
- `libgmp-dev` - Multi-precision arithmetic
- `libsodium-dev` - Additional cryptographic primitives
- `zlib1g-dev` - Compression support

## Installation

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libdb5.3++-dev \
    libssl-dev \
    libboost-all-dev \
    libminiupnpc-dev \
    libevent-dev \
    pkg-config
```

### macOS
```bash
brew install berkeley-db@5 openssl@1.1 boost miniupnpc libevent
```

## Building

### Standard Build
```bash
# Clean previous builds
make clean

# Build with all CPU cores
make -j$(nproc)
```

### Debug Build
```bash
make DEBUG=1 -j$(nproc)
```

### Verbose Build (see all commands)
```bash
make VERBOSE=1 -j$(nproc)
```

## Project Structure

### Key Directories
- `src/` - Source code
  - `LLP/` - Lower Level Protocol (networking)
  - `TAO/` - Tritium, API, Operations (core blockchain logic)
  - `LLC/` - Lower Level Crypto (cryptographic primitives)
  - `Legacy/` - Legacy protocol support
- `tests/` - Unit and integration tests
- `docs/` - Documentation
- `config/` - Configuration files

### Important Files
- `Makefile` - Main build configuration
- `makefile.cli` - Command-line build targets
- `.devcontainer/` - DevContainer configuration for consistent builds

## Testing

### Run All Tests
```bash
make test
```

### Run Specific Test Suite
```bash
# Unit tests only
./build/tests/unit_tests

# LLP tests
./build/tests/unit_tests --run_test=LLP/*

# Ledger tests
./build/tests/unit_tests --run_test=Ledger/*
```

## Common Build Issues

### Berkeley DB Not Found
**Error:** `fatal error: db_cxx.h: No such file or directory`

**Solution:**
```bash
sudo apt-get install libdb5.3++-dev
# Or specify DB location in Makefile
```

### Boost Version Mismatch
**Error:** `Boost version too old`

**Solution:**
```bash
sudo apt-get install libboost-all-dev
# Minimum version: 1.71
```

### OpenSSL Linking Error
**Error:** `undefined reference to EVP_*`

**Solution:**
```bash
sudo apt-get install libssl-dev
# Make sure OpenSSL 1.1.1+ is installed
```

## Code Modification Guidelines

### When Adding New Dependencies
1. Update `.devcontainer/setup.sh` with new package
2. Update this document's dependency table
3. Update README.md installation instructions
4. Test in clean DevContainer environment

### When Modifying Build Process
1. Update `Makefile` or relevant makefile.* files
2. Update build instructions in this document
3. Verify changes in DevContainer
4. Document any new build flags or options

## Continuous Integration

### GitHub Actions
Build and test workflows run automatically on:
- Push to main branches
- Pull request creation
- Pull request updates

### DevContainer Testing
Copilot Coding Agent uses the DevContainer configuration to:
- Verify PRs compile successfully
- Run automated tests
- Check for dependency issues
- Validate code changes before merge

## Performance Considerations

### Build Times
- **First build:** ~5-10 minutes (depends on CPU cores)
- **Incremental build:** ~30 seconds - 2 minutes
- **Clean rebuild:** ~3-5 minutes

### Optimization Flags
- Production: `-O3` (maximum optimization)
- Debug: `-g -O0` (no optimization, full debug symbols)
- Profile: `-O2 -g` (optimized with debug info)

## Architecture-Specific Notes

### Stateless Mining Protocol
- Located in `src/LLP/` directory
- Uses Falcon post-quantum cryptography
- Requires OpenSSL 1.1.1+ for ChaCha20-Poly1305
- Network protocol on ports 9323-9325

### Tritium Protocol
- Core blockchain logic in `src/TAO/Ledger/`
- Three mining channels: Proof-of-Stake (0), Prime (1), Hash (2)
- Uses Berkeley DB for block storage

### Lower Level Protocol (LLP)
- Custom P2P networking stack
- Event-driven architecture using libevent
- Supports both legacy (8-bit) and stateless (16-bit) opcodes

## Support

### Documentation
- Project README: `/README.md`
- API documentation: `/docs/`
- Protocol specification: `/docs/PROTOCOL.md`

### Common Commands
```bash
# Start node (after building)
./nexus

# Run with testnet
./nexus -testnet

# Enable verbose logging
./nexus -verbose=3

# Run API server
./nexus -api

# Mining configuration
./nexus -mining -generate=
```
