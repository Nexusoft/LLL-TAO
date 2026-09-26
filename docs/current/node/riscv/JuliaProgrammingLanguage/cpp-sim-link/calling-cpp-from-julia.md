# 🟣 Calling C++ from Julia

**Last Updated:** 2026-03-11

A Julia experiment should prefer narrow C-compatible entry points, deterministic buffers, and explicit type boundaries when calling compiled C++ libraries. Avoid binding broad internal node state directly into Julia until correctness and maintenance costs are well understood.
