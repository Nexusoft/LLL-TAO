# Nexus Node Documentation

This directory contains the active Nexus node documentation. Historical notes and superseded write-ups are preserved under [`archive/`](archive/); current references and invariants live under [`reference/`](reference/).

## Start here

- [Build guides](build/) - platform builds, build options, and RISC-V compilation.
- [Operator guides](guides/) - API, Docker, and mining operator workflows.
- [Current node docs](current/) - active mining, authentication, node, security, testing, and troubleshooting docs.
- [Architecture](architecture/) - design notes and system architecture references.
- [Protocol](protocol/) - mining protocol specifications.
- [Reference](reference/) - canonical config, opcodes, diagrams, and invariants.
- [Onboarding](onboarding/) - new contributor and AI-assisted onboarding.
- [Archive](archive/) - historical bug write-ups, superseded designs, and PR summaries.

## Build

- [Linux](build/build-linux.md)
- [Raspberry Pi](build/build-linux-rpi.md)
- [Windows](build/build-win.md)
- [OSX](build/build-osx.md)
- [Mobile](build/build-mobile.md)
- [RISC-V](build/riscv-build-guide.md)
- [Build parameters](build/build-params-reference.md)

## Guides

- [API guide](guides/how-to-api.md)
- [Docker guide](guides/how-to-docker.md)
- [Mining auto-credit guide](guides/mining-auto-credit.md)

## Philosophy

- [The AI-Human Advancement Thesis](philosophy/ai-human-advancement.md)
- [Computing Paradigms: Quantum vs Neural vs Classical](philosophy/computing-paradigms.md)
- [Garden of Eden as a Controlled Environment](philosophy/garden-of-eden-controlled-environment.md)
- [Created Intelligence and the Forbidden Operation](philosophy/created-intelligence-and-the-forbidden-operation.md)

## Architecture and design

- [RC13: Transactional chain-transition bug chain (SetBest teaching doc)](release/rc13-transactional-chain-transition-fixes.md)
- [Blockchain flow alignment](BLOCKCHAIN_FLOW_ALIGNMENT.md)
- [Block production flow](architecture/BLOCK_PRODUCTION_FLOW.md)
- [Mempool-only predecessor filter](architecture/MEMPOOL_ONLY_PREDECESSOR_FILTER.md)
- [Session architecture](architecture/SESSION_ARCHITECTURE.md)
- [Session freshness hardening](architecture/SESSION_FRESHNESS_HARDENING.md)
- [SIMLINK dual-lane architecture](architecture/SIMLINK_DUAL_LANE_ARCHITECTURE.md)
- [Template immutability and cache constraints](architecture/TEMPLATE_IMMUTABILITY_AND_CACHE_CONSTRAINTS.md)
- [RISC-V technical design](architecture/riscv-design.md)

## Current references

- [Hard-won invariants](reference/invariants.md)
- [Sigchain last resolution](reference/sigchain-last-resolution.md)
- [nexus.conf reference](reference/nexus.conf.md)
- [Opcodes reference](reference/opcodes-reference.md)
- [Stateless mining reference](reference/STATELESS_MINING_REFERENCE.md)
- [Stateless mining reference addendum](reference/STATELESS_MINING_REFERENCE_ADDENDUM.md)
- [Flow architecture diagrams](reference/Flow-Architecture-Diagram-REF.md)

## Mining and node operations

- [Stateless mining protocol](current/mining/stateless-protocol.md)
- [Mining server architecture](current/mining/mining-server.md)
- [Mining lanes cheat sheet](current/mining/mining-lanes-cheat-sheet.md)
- [Mining notification diagnostics](current/mining/mining-notification-diagnostics.md)
- [Mining server troubleshooting](current/troubleshooting/mining-server-issues.md)
- [Node architecture index](current/node/index.md)
- [Session container architecture](current/node/session-container-architecture.md)
- [Recovery merge model](current/node/recovery-merge-model.md)
- [Roadmap and upgrade path](current/node/roadmap-upgrade-path.md)
- [Test strategy](current/node/test-strategy.md)

## Security and authentication

- [Falcon verification](current/authentication/falcon-verification.md)
- [Quantum resistance](current/security/quantum-resistance.md)

## Diagrams

- [Diagram index](diagrams/README.md)
- [Architecture boxes](diagrams/architecture-boxes.md)
- [Upgrade-path diagrams](diagrams/upgrade-path/README.md)

## Coding agent guidance

- [Coding agent best practices](CODING_AGENT_BEST_PRACTICES.md)
- [Coding agent cheat sheet](CODING_AGENT_CHEAT_SHEET.md)

## Archive policy

Use [`archive/README.md`](archive/README.md) for the archive policy. Archive documents are retained for learning, but may describe behavior that has since been changed or reverted.
