# 🟣 Interop Models

**Last Updated:** 2026-03-11

## Preferred Order of Complexity

1. **Offline file/fixture exchange** for correctness research.
2. **Shared-library boundaries** for narrowly scoped kernels.
3. **Embedding Julia** only when process boundaries are unacceptable.

The simplest model that preserves reproducibility should be preferred.
