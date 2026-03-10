# Nexus Node Documentation

Welcome to the comprehensive Nexus LLL-TAO node documentation. This documentation covers all aspects of running and operating a Nexus node, with a focus on the stateless mining protocol.

**Version:** LLL-TAO 5.1.0+  
**Last Updated:** 2026-02-18

---

## Quick Navigation

### 🎯 Getting Started

- **[Build Instructions](build-linux.md)** - Compile the node from source
- **[Configuration Guide](reference/nexus.conf.md)** - Complete nexus.conf reference
- **[Configuration Examples](reference/config-examples/)** - Ready-to-use configs

### ⛏️ Mining

- **[Stateless Mining Protocol](current/mining/stateless-protocol.md)** - Node implementation
- **[Mining Server Architecture](current/mining/mining-server.md)** - Server design
- **[Mining Lanes Cheat Sheet](current/mining/mining-lanes-cheat-sheet.md)** - Legacy vs stateless entrypoints
- **[Mining Notification Diagnostics](current/mining/mining-notification-diagnostics.md)** - Push notification debugging
- **[Troubleshooting](current/troubleshooting/mining-server-issues.md)** - Fix common issues

### 🏗️ Node Architecture & Refactor Roadmap

- **[Node Architecture Index](current/node/index.md)** - Per-session container model overview
- **[Session Container Architecture](current/node/session-container-architecture.md)** - Authoritative container, indexes, invariants
- **[Recovery Merge Model](current/node/recovery-merge-model.md)** - Reconnect, identity-first recovery, merge rules
- **[Roadmap & Upgrade Path](current/node/roadmap-upgrade-path.md)** - 15 remaining refactor items with priorities
- **[Test Strategy](current/node/test-strategy.md)** - First-block harness, multi-miner collision, preflight tests
- **[Upgrade-Path Diagrams (15 diagrams)](diagrams/upgrade-path/README.md)** - ASCII thick-box architecture diagrams

### 🔵 RISC-V Support

- **[RISC-V Node Overview](current/node/riscv/index.md)** - Build targets and portability guarantees
- **[Endianness & Serialisation](current/node/riscv/endianness-serialization.md)** - Byte-order invariants across architectures
- **[Atomic Operations & Locking](current/node/riscv/atomic-locking.md)** - RVWMO memory model and lock patterns
- **[RISC-V Diagnostics & Testing](current/node/riscv/diagnostics.md)** - Self-tests and cross-compile CI guide
- **[RISC-V Design (top-level)](riscv-design.md)** - Executive summary and design principles

### 🔐 Security & Authentication

- **[Falcon Verification](current/authentication/falcon-verification.md)** - Post-quantum auth
- **[Quantum Resistance](current/security/quantum-resistance.md)** - Security properties

### 🤖 Coding Agent Guidance

- **[Coding Agent Best Practices](CODING_AGENT_BEST_PRACTICES.md)** - Collaboration framework
- **[Coding Agent Cheat Sheet](CODING_AGENT_CHEAT_SHEET.md)** - Budget + case studies
- **[Diagram Templates](diagrams/README.md)** - ASCII reference diagrams

### 📚 Reference

- **[nexus.conf Reference](reference/nexus.conf.md)** - All configuration options
- **[Opcodes Reference](reference/opcodes-reference.md)** - LLP protocol opcodes
- **[Flow Diagrams](reference/Flow-Architecture-Diagram-REF.md)** - Architecture diagrams

---

## Documentation Structure

```
docs/
├── CODING_AGENT_BEST_PRACTICES.md # Agent collaboration framework
├── CODING_AGENT_CHEAT_SHEET.md    # Budget + case studies
├── diagrams/                      # ASCII diagram templates
│   ├── README.md
│   ├── architecture-boxes.md
│   ├── flow-chart.md
│   ├── state-machine.md
│   ├── data-pipeline.md
│   ├── push-notification-flow.md
│   └── upgrade-path/              # 15 upgrade-path refactor diagrams
│       ├── README.md              # Diagrams index
│       ├── 01-shared-session-binding.md
│       ├── 02-canonical-validate-consistency.md
│       ├── 03-stronger-state-machines.md
│       ├── 04-live-container-vs-recovery-snapshot.md
│       ├── 05-identity-first-recovery-keys.md
│       ├── 06-scoped-update-guard.md
│       ├── 07-reward-address-semantics.md
│       ├── 08-multi-miner-collision-tests.md
│       ├── 09-canonical-crypto-context.md
│       ├── 10-first-mined-block-acceptance.md
│       ├── 11-session-conflict-resolver.md
│       ├── 12-fast-vs-full-validation.md
│       ├── 13-per-session-event-journal.md
│       ├── 14-packet-ingress-preflight.md
│       └── 15-strong-semantic-id-types.md
├── protocol/                   # Protocol specifications
│   └── mining-protocol.md      # Legacy vs Stateless Tritium Protocol lanes
├── current/                    # Active node features
│   ├── mining/                 # Mining server & stateless protocol
│   ├── authentication/         # Falcon verification & sessions
│   ├── api/                    # RPC and API documentation
│   ├── security/               # Node security features
│   ├── troubleshooting/        # Problem solving guides
│   └── node/                   # Node architecture & refactor roadmap
│       ├── index.md             # Node section landing page
│       ├── session-container-architecture.md
│       ├── recovery-merge-model.md
│       ├── roadmap-upgrade-path.md
│       ├── test-strategy.md
│       └── riscv/              # RISC-V deployment guide
│           ├── index.md
│           ├── endianness-serialization.md
│           ├── atomic-locking.md
│           └── diagnostics.md
│
├── archive/                    # Historical documentation
│   ├── migration-guides/       # Legacy protocol migration
│   └── pr-summaries/           # Pull request summaries and rollups
│
├── reference/                  # Master references
│   ├── nexus.conf.md           # Complete config reference (800+ lines)
│   ├── opcodes-reference.md    # LLP protocol opcodes (700+ lines)
│   ├── Flow-Architecture-Diagram-REF.md  # Architecture diagrams
│   └── config-examples/        # Production-ready configs
│       ├── mining-node.conf
│       ├── mining-node-testnet.conf
│       ├── mining-node-whitelisted.conf
│       └── high-performance-mining.conf
│
└── upgrade-guides/             # Version migration guides
```

---

## Core Documents

### Mining Documentation

#### 1. [Stateless Mining Protocol](current/mining/stateless-protocol.md)
Complete node-side implementation of the stateless mining protocol introduced in LLL-TAO PR #170.

**Topics covered:**
- Mining server initialization
- Session management (24h default timeout)
- Template generation (<1ms target)
- Push notification flow (<10ms target)
- Block submission handling (20-30ms typical)
- Performance metrics

**Target audience:** Node operators, pool operators, developers

---

### Archived Summaries

Historical PR summaries, verification reports, and implementation notes now live under:
- `docs/archive/pr-summaries/`
- `docs/archive/`

---

#### 2. [Mining Server Architecture](current/mining/mining-server.md)
Deep dive into the LLP mining server architecture and internal components.

**Topics covered:**
- Component hierarchy
- Thread model (main, worker, notification threads)
- Connection lifecycle
- Session cache design
- Template generator
- Push notification broadcaster
- Performance characteristics
- Security features (DDoS, SSL, whitelisting)

**Target audience:** System administrators, developers

---

#### 3. [Troubleshooting Guide](current/troubleshooting/mining-server-issues.md)
Solutions for common mining server problems.

**Issues covered:**
- "Mining server not starting"
- "No miners connecting"
- "High block rejection rate"
- "Miners disconnecting frequently"
- "Slow template generation"
- "Memory usage growing"
- "SSL/TLS errors"

**Target audience:** Node operators, support staff

---

### Authentication Documentation

#### 4. [Falcon Verification](current/authentication/falcon-verification.md)
Node-side Falcon-1024 signature verification for post-quantum security.

**Topics covered:**
- Falcon-1024 background (NIST Level 5)
- Dual signature system (Disposable + Physical)
- Authentication architecture
- Step-by-step verification process
- Performance characteristics (<2ms typical)
- Security properties
- Troubleshooting

**Target audience:** Security engineers, developers

---

### Reference Documentation

#### 5. [nexus.conf Reference](reference/nexus.conf.md) (867 lines)
Comprehensive reference for all node configuration parameters.

**Sections:**
- Mining server settings
- Network configuration
- Authentication & security
- Performance tuning
- Database settings
- Logging configuration
- API/RPC settings
- 4 complete configuration examples

**Target audience:** All node operators

---

#### 6. [Opcodes Reference](reference/opcodes-reference.md) (774 lines)
Complete LLP protocol opcodes from node perspective.

**Sections:**
- Opcode allocation strategy
- Stateless mining opcodes (0xD000-0xDFFF)
- Authentication flow (MINER_AUTH, MINER_AUTH_RESPONSE)
- Configuration flow (MINER_SET_REWARD, SET_CHANNEL)
- Template delivery (GET_BLOCK, NEW_BLOCK)
- Solution submission (SUBMIT_BLOCK)
- Performance metrics
- Error handling

**Target audience:** Protocol developers, integrators

---

#### 7. [Flow Architecture Diagrams](reference/Flow-Architecture-Diagram-REF.md)
Visual diagrams of all protocol flows using Mermaid.

**Diagrams included:**
- Complete protocol flow (sequence diagram)
- Authentication flow (flowchart)
- Template generation (flowchart)
- Push notifications (flowchart)
- Block submission validation (flowchart)
- Session lifecycle (state diagram)
- Channel isolation (flowchart)
- Performance optimization (flowchart)
- Error handling (flowchart)

**Target audience:** Visual learners, architects

---

## Configuration Examples

Ready-to-use configuration files for common scenarios:

### 1. [Basic Mining Node](reference/config-examples/mining-node.conf)
Simple configuration for solo or small pool mining.

**Features:**
- Localhost mining enabled
- Basic security
- Standard performance
- Minimal configuration

**Use case:** Solo miners, local mining, learning

---

### 2. [Testnet Mining Node](reference/config-examples/mining-node-testnet.conf)
Configuration for Nexus testnet mining.

**Features:**
- Testnet mode enabled
- All testnet ports configured
- Verbose logging
- Reduced resource requirements

**Use case:** Testing, development, experimentation

---

### 3. [Whitelisted Mining Node](reference/config-examples/mining-node-whitelisted.conf)
Enhanced security with Falcon public key whitelisting.

**Features:**
- Key whitelisting enabled
- Only authorized miners can connect
- Network access controls
- Detailed key management workflow

**Use case:** Private pools, corporate mining, restricted access

---

### 4. [High-Performance Mining Pool](reference/config-examples/high-performance-mining.conf)
Optimized for large public mining pools with many concurrent miners.

**Features:**
- SSL/TLS encryption required
- DDoS protection enabled
- High thread counts (32+)
- Large database cache (4+ GB)
- API for pool frontend
- Production monitoring
- Scaling guidelines

**Use case:** Public mining pools (100-500+ miners)

---

## Cross-References with NexusMiner

This documentation is designed to complement the NexusMiner documentation, providing a complete ecosystem view.

### Node ↔ Miner Documentation Pairs

| Node Documentation | Miner Documentation |
|-------------------|---------------------|
| [Stateless Protocol (Node)](current/mining/stateless-protocol.md) | [Stateless Mining (Miner)](https://github.com/Nexusoft/NexusMiner/blob/main/docs/current/mining-protocols/stateless-mining.md) |
| [Opcodes Reference (Node)](reference/opcodes-reference.md) | [Opcodes Reference (Miner)](https://github.com/Nexusoft/NexusMiner/blob/main/docs/reference/opcodes-reference.md) |
| [Falcon Verification (Node)](current/authentication/falcon-verification.md) | [Falcon Authentication (Miner)](https://github.com/Nexusoft/NexusMiner/blob/main/docs/current/security/falcon-authentication.md) |
| [nexus.conf (Node)](reference/nexus.conf.md) | [nexus.conf (Miner)](https://github.com/Nexusoft/NexusMiner/blob/main/docs/reference/nexus.conf.md) |
| [Flow Diagrams (Node)](reference/Flow-Architecture-Diagram-REF.md) | [Flow Diagrams (Miner)](https://github.com/Nexusoft/NexusMiner/blob/main/docs/reference/Flow-Architecture-Diagram-REF.md) |
| [Mining Server Issues](current/troubleshooting/mining-server-issues.md) | [Connection Issues](https://github.com/Nexusoft/NexusMiner/blob/main/docs/current/troubleshooting/connection-issues.md) |

### NexusMiner Repository

**Repository:** https://github.com/Nexusoft/NexusMiner  
**Documentation:** https://github.com/Nexusoft/NexusMiner/tree/main/docs

**Key NexusMiner Docs:**
- [Getting Started](https://github.com/Nexusoft/NexusMiner/blob/main/docs/current/getting-started/setup.md)
- [Stateless Mining](https://github.com/Nexusoft/NexusMiner/blob/main/docs/current/mining-protocols/stateless-mining.md)
- [Auto-Negotiation](https://github.com/Nexusoft/NexusMiner/blob/main/docs/upgrade-guides/legacy-to-stateless.md)
- [Key Generation](https://github.com/Nexusoft/NexusMiner/blob/main/docs/current/getting-started/key-generation.md)

---

## Documentation Quality

This documentation reorganization mirrors the professional structure established in **NexusMiner PR #94**, achieving:

✅ **Comprehensive Coverage** - 2,400+ lines of documentation  
✅ **Professional Structure** - Organized by feature area  
✅ **Cross-Referenced** - Bidirectional links with NexusMiner  
✅ **Production-Ready** - Complete configuration examples  
✅ **Visual Aids** - Flow diagrams and architecture charts  
✅ **Troubleshooting** - Practical problem-solving guides  
✅ **Performance Metrics** - Real-world measurements  
✅ **Security Focus** - Post-quantum cryptography explained  

---

## Contributing

To contribute to this documentation:

1. Follow the established structure
2. Maintain consistency with NexusMiner docs
3. Include code examples where appropriate
4. Add cross-references liberally
5. Update version information
6. Test all commands and configurations

---

## Support & Community

- **Discord:** https://nexus.io/discord
- **Forum:** https://nexus.io/forum
- **GitHub Issues:** https://github.com/Nexusoft/LLL-TAO/issues
- **Website:** https://nexus.io

---

## Version History

**v1.0 (2026-01-13)**
- Initial comprehensive reorganization
- Mirrored NexusMiner PR #94 structure
- Created master reference documents
- Added configuration examples
- Established cross-references

---

## License

This documentation is part of the Nexus LLL-TAO project and is distributed under the MIT License.

Copyright (c) 2014-2026 The Nexus Developers

See [LICENSE](../LICENSE) for details.
