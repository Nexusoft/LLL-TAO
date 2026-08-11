# 🟣 Phased Delivery Plan

**Last Updated:** 2026-03-11

## Phase 1 — Documentation and Tooling

- Define the folder structure and operating rules.
- Document local Julia installation and binary options.
- Capture the first interop and testing diagrams.

## Phase 2 — Kernel Research

- Prototype deterministic math kernels in Julia.
- Add correctness fixtures and parity expectations.
- Record RISC-V-specific observations.

## Phase 3 — Interoperability Review

- Evaluate embedding Julia in C++ only when justified.
- Compare `ccall`/library boundaries with standalone process workflows.
- Document rollback paths if complexity outweighs benefit.

## Phase 4 — Evidence-Based Porting

- Require reproducible benchmarks.
- Port only validated ideas into production C++.
- Preserve a parity test trail for future regressions.
