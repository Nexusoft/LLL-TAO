# 🟣 Kernel Correctness Tests

**Last Updated:** 2026-03-11

Kernel correctness tests should lock down deterministic inputs, expected outputs, overflow boundaries, and edge cases before any performance work begins. Consensus-sensitive logic must always be validated against the production C++ implementation.
