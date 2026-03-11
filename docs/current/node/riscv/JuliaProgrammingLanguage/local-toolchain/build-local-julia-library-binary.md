# 🟣 Build Local Julia Library Binary

**Last Updated:** 2026-03-11

A local Julia library build is appropriate only when a narrow, well-tested interop boundary is needed. Document exported symbols, ABI assumptions, test fixtures, and cleanup steps before any downstream consumer is allowed to depend on the library.

For the Falcon QTV prototype, the safe handoff boundary is the deterministic fixture surface:

- `deterministic_laser_fixture(...)`
- `encode_laser_tunnel(...)`
- `decode_laser_tunnel(...)`

Those entry points keep the Julia/C++ exchange limited to byte vectors, swap-log tuples, and SHA-512 digests. Any local `PackageCompiler.create_library(...)` experiment should preserve that boundary so the C++ parity test can continue to replay the same vectors byte-for-byte.
