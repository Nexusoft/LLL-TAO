# Common Tasks: AI Prompts Cheat Sheet

Ready-to-use AI prompts for frequent development operations on the Nexus LLL-TAO codebase.

---

## Code Navigation

| Task | AI Prompt |
|------|-----------|
| Find a function | "Where is `[function_name]` defined in the codebase?" |
| Trace a call chain | "Trace the call chain from `[entry_point]` to `[target]`" |
| Find all callers | "List all files that call `[function_name]`" |
| Understand a class | "Explain the purpose and methods of `[ClassName]` in `src/[path]`" |
| Find related tests | "Find test files for `[component_name]`" |

---

## Mining Protocol

| Task | AI Prompt |
|------|-----------|
| Understand mining flow | "Trace the mining flow from MINER_AUTH to BLOCK_ACCEPTED in LLL-TAO" |
| Debug notification | "How does push notification work in `src/LLP/include/push_notification.h`?" |
| Compare protocols | "Compare Legacy Tritium (8-bit) vs Stateless (16-bit) packet formats" |
| Find opcode | "What is the opcode for `[OPERATION]` in both Legacy and Stateless protocols?" |
| Template generation | "How is a block template generated for miners?" |

---

## Consensus & Validation

| Task | AI Prompt |
|------|-----------|
| Validation pipeline | "What checks does `TritiumBlock::Check()` perform?" |
| Channel differences | "Compare PoS, Prime, and Hash mining channels" |
| Merkle verification | "How is the merkle root validated during block acceptance?" |
| Transaction types | "What transaction types exist and how are they verified?" |

---

## TAO Registers

| Task | AI Prompt |
|------|-----------|
| Register types | "List all register types in `src/TAO/Register/include/enum.h`" |
| State transitions | "Explain PRESTATE and POSTSTATE in register operations" |
| Trust accounts | "How does the Trust register (0x14) lifecycle work?" |
| Operations | "What operations can be performed on registers (WRITE, APPEND, DEBIT, etc.)?" |

---

## Authentication & Security

| Task | AI Prompt |
|------|-----------|
| Falcon handshake | "Walk through the Falcon authentication handshake step by step" |
| Session management | "How does the session cache work? What are the purge timeouts?" |
| ChaCha20 encryption | "When is ChaCha20 required vs optional in mining connections?" |
| DDOS protection | "How does the 500-entry cache limit protect against DDOS?" |

---

## Build & Configuration

| Task | AI Prompt |
|------|-----------|
| Build the project | "What are the build steps for Linux in LLL-TAO?" |
| Configuration | "What mining-related options are available in nexus.conf?" |
| Debug build | "How do I build with debug symbols enabled?" |
| Dependencies | "What are the build dependencies for LLL-TAO?" |

---

## Documentation

| Task | AI Prompt |
|------|-----------|
| Find related docs | "What documentation exists for `[topic]`?" |
| Cross-reference | "Cross-reference the mining protocol docs with the source code" |
| Generate diagrams | "Create a Mermaid diagram for `[workflow/architecture]`" |
| Write PR description | "Summarize the changes in `[files]` for a PR description" |

---

## Cross-References

- [AI-Human Simlink](../ai-human-simlink.md)
- [Learning Pathways](../learning-pathways.md)
- [Mining Debug Cheat Sheet](../../../onboarding/cheat-sheets/mining-debug.md)
