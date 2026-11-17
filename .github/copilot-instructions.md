# Copilot Build Instructions for Nexus LLL-TAO

## Overview
Nexus LLL-TAO is a blockchain node implementation written in C++20. It requires specific dependencies for compilation and testing.

## Build Environment

### Operating System
- **Primary:** Ubuntu 22.04 LTS
- **Minimum:** Ubuntu 20.04, Debian 11, or equivalent
- **Alternative:** macOS 12+ (requires Homebrew)

### Compiler Requirements
- **GCC:** 11.0 or later (recommended: 11.4)
- **Clang:** 14.0 or later
- **Standard:** C++20 with full support

### Core Dependencies

#### Required Libraries
| Library | Version | Package | Purpose |
|---------|---------|---------|---------|
| Berkeley DB | 5.3.x | `libdb5.3++-dev` | Legacy wallet database storage |
| OpenSSL | 1.1.1+ | `libssl-dev` | Cryptographic operations (hashing, signatures) |

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
make -f makefile.cli clean

# Build with all CPU cores
make -f makefile.cli -j$(nproc)
```

### Debug Build
```bash
make -f makefile.cli clean
make -f makefile.cli ENABLE_DEBUG=1 -j$(nproc)
```

### Verbose Build (see all commands)
```bash
make -f makefile.cli VERBOSE=1 -j$(nproc)
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
- `makefile.cli` - Command-line build targets
- `.devcontainer/` - DevContainer configuration for consistent builds

## Testing

### Run All Tests
On Unix, use a fresh isolated HOME for each test process; bootstrap clears its data directory before processing Catch arguments.

```bash
make -f makefile.cli clean
make -f makefile.cli UNIT_TESTS=1 -j$(nproc)
test_home="$(mktemp -d)" && HOME="$test_home" ./nexus
```

### Run Specific Test Suite
```bash
# Build unit-test binary
make -f makefile.cli clean
make -f makefile.cli UNIT_TESTS=1 -j$(nproc)

# Run LLP-focused tests from the produced nexus binary
test_home="$(mktemp -d)" && HOME="$test_home" ./nexus "[llp]"

# Run ledger-focused tests
test_home="$(mktemp -d)" && HOME="$test_home" ./nexus "[ledger]"
```

## Common Build Issues

### Berkeley DB Not Found
**Error:** `fatal error: db_cxx.h: No such file or directory`

**Solution:**
```bash
sudo apt-get install libdb5.3++-dev
# Or run the repo bootstrap:
bash .devcontainer/setup.sh
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
1. Update `contrib/devtools/install-build-deps.sh` with new package
2. Update this document's dependency table
3. Update README.md installation instructions
4. Test in clean DevContainer environment

### When Modifying Build Process
1. Update relevant makefile.* files
2. Update build instructions in this document
3. Verify changes in DevContainer
4. Document any new build flags or options

## Continuous Integration

### GitHub Actions
Build and test workflows run automatically for changes matching their path filters on:
- Push to main branches
- Pull request creation
- Pull request updates

### Copilot Setup
Copilot Coding Agent uses the Copilot setup workflow plus the shared devcontainer bootstrap to:
- Check for dependency issues

Copilot code review uses `.github/workflows/copilot-code-review.yml` for its
runner and build dependency setup. This configures its environment, not
automatic review requests.

## Performance Considerations

### Build Times
- **First build:** ~5-10 minutes (depends on CPU cores)
- **Incremental build:** ~30 seconds - 2 minutes
- **Clean rebuild:** ~3-5 minutes

### Optimization Flags
- Production: `-O3` (maximum optimization)
- Profile: `-O2 -g` (optimized with debug info)

## Architecture-Specific Notes

### Stateless Mining Protocol
- Located in `src/LLP/` directory
- Uses Falcon post-quantum cryptography
- Requires OpenSSL 1.1.1+ for ChaCha20-Poly1305

### Tritium Protocol
- Core blockchain logic in `src/TAO/Ledger/`
- Three mining channels: Proof-of-Stake (0), Prime (1), Hash (2)

### Lower Level Protocol (LLP)
- Custom P2P networking stack
- Supports both legacy (8-bit) and stateless (16-bit) opcodes

## Support

### Documentation
- Project README: `/README.md`
- API documentation: `/docs/`

### Common Commands
```bash
# Start node (after building)
./nexus

# Run with testnet
./nexus -testnet=1

# Enable verbose logging
./nexus -verbose=3

# Run API server (set apiuser and apipassword in nexus.conf first)
./nexus

# Mining configuration
./nexus -mining -generate=
```
