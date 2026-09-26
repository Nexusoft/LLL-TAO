# Quantum Resistance

**Section:** Security & Authentication  
**Version:** LLL-TAO 5.1.0+  
**Last Updated:** 2026-03-11

---

## Overview

Nexus LLL-TAO relies on Falcon-1024 for post-quantum mining authentication. This page captures the high-level security posture behind that choice and explains how quantum-resistance claims should be interpreted in the context of the node.

---

## Current Scope

- Falcon-1024 provides the repository's primary post-quantum signature story for mining authentication.
- Quantum-resistance claims apply to the cryptographic primitive, not to every surrounding operational risk.
- Production guarantees still depend on correct implementation, key management, and protocol validation.

---

## Operational Notes

- Prefer existing node authentication documentation for implementation details.
- Treat quantum-resistance as one layer of the overall security model.
- Keep security claims aligned with the documented Falcon verification path.

---

## Related Pages

- [Falcon Verification](../authentication/falcon-verification.md)
- [RISC-V Node Overview](../node/riscv/index.md)
