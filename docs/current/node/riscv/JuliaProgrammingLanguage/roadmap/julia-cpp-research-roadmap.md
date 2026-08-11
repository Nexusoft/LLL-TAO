# 🟣 Julia + C++ Research Roadmap

**Last Updated:** 2026-03-11

## Focus Areas

- Integer-heavy kernels that map cleanly to existing C++ consensus rules.
- Batch/vector experiments that may benefit RISC-V or RVV exploration.
- Offline proof-of-concept work for mining, big integer, or serialization helpers.

## Research Constraints

- Never replace production consensus code with unverified Julia output.
- Preserve deterministic fixtures for every experiment.
- Prefer small, isolated interop surfaces over broad runtime coupling.

## Exit Criteria

A Julia experiment is ready for C++ consideration only when correctness, parity, and performance evidence all exist together.
