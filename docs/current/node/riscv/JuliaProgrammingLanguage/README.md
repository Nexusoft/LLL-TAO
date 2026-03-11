# 🟣 Julia Programming Language

**Section:** Node Architecture → RISC-V → Julia Programming Language  
**Version:** LLL-TAO 5.1.0+  
**Last Updated:** 2026-03-11

---

## Overview

This section treats Julia as a self-contained research and prototyping lab for math-heavy, vector-aware, and kernel-oriented experiments that may later inform the production C++ node.

Julia is documented here as a companion environment for:

- research tooling
- math and kernel experimentation
- Julia ↔ C++ SIM-LINK interoperability planning
- local binary and toolchain support
- tests-first validation workflows
- RISC-V portability research

---

## Documentation Rules

- Julia is a **research and prototyping environment first**.
- C++ remains the **production miner and runtime**.
- No Julia-derived optimization is trusted without Julia tests, C++ parity tests, and reproducible benchmark evidence.
- Local binary and toolchain workflows must be documented and reproducible.
- Embedding Julia is optional and must be justified before adoption.

---

## Sections

| Section | Purpose |
|---|---|
| **[Roadmap](roadmap/README.md)** | Delivery phases, C++ research sequencing, remaining optimization plan |
| **[Glossary](glossary/README.md)** | Shared Julia and Julia/C++ terminology |
| **[History](history/README.md)** | Context on Julia language evolution and scientific-computing relevance |
| **[Julia ↔ C++ SIM-LINK](cpp-sim-link/README.md)** | Interop models, embedding, calling conventions, offline kernel workflow |
| **[Local Toolchain](local-toolchain/README.md)** | Local Julia install, binary strategy, app/library build guidance |
| **[Tests](tests/README.md)** | Kernel correctness, cross-language parity, and performance regression gates |
| **[Diagrams](diagrams/README.md)** | Screenshot-friendly architecture and workflow diagrams |
| **[Crypto Experiments](crypto/crypto-scope-and-boundaries.md)** | Boundaries for safe cryptographic prototyping |
| **[References](references/official-julia-references.md)** | Official Julia material and external references |

---

## Recommended Workflow

1. Prototype a kernel or math idea in Julia.
2. Define deterministic Julia-side tests first.
3. Record parity expectations against the C++ implementation.
4. Capture benchmark evidence and RISC-V portability notes.
5. Port only validated changes into production C++.

---

## Related Pages

- [RISC-V Node Deployment — Overview](../index.md)
- [Diagnostics & Testing Notes (RISC-V)](../diagnostics.md)
- [Node Architecture Index](../../index.md)
