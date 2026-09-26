# 🟣 Cross-Language Parity Tests

**Last Updated:** 2026-03-11

Cross-language parity tests compare Julia outputs with C++ outputs across identical fixtures. Any mismatch should block promotion of a Julia-derived optimisation idea until the discrepancy is explained and resolved.

Current recorded example:

- Julia fixture: `docs/current/node/riscv/JuliaProgrammingLanguage/tests/falcon_qtv_tests.jl`
- C++ replay: `tests/unit/LLC/falcon_qtv_parity_test.cpp`
- Shared evidence: fixed `encoded`, `swap_log`, and `working_vector_digest` values for the Falcon QTV laser-tunnel experiment
