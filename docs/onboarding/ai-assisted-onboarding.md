# AI-Assisted Developer Onboarding

## Why This Codebase is the APEX for AI-Human Collaboration

### The Nexus Architecture Advantage
1. **Modular Design** - AI can isolate components (LLP, TAO, Ledger)
2. **Extensive Documentation** - AI can cross-reference 100+ docs instantly
3. **Clear Abstractions** - AI understands mining/consensus/networking layers
4. **Test Coverage** - AI validates changes against test suites

### AI as Your Co-Pilot (Not Autopilot)
- **AI excels at:** Code navigation, pattern matching, boilerplate generation
- **Human excels at:** Design decisions, edge case intuition, security review
- **Together:** 10x development velocity with higher quality

---

## Learning Pathways

### Week 1: Mining Protocol (with AI)
1. Ask AI: "Explain LLP mining protocol flow"
2. Ask AI: "Show me push notification implementation"
3. Human: Test mining on testnet, verify AI's explanation
4. Together: Identify optimization opportunities

**Key diagrams to review:**
- [Mining Flow Complete](../diagrams/architecture/mining-flow-complete.md)
- [Push Notification Flow](../diagrams/push-notification-flow.md)
- [LLP Packet Anatomy](../diagrams/protocols/llp-packet-anatomy.md)

### Week 2: Consensus & Validation
1. AI: Generate flowchart of block validation
2. Human: Review for security edge cases
3. AI: Find all validation checks in codebase
4. Human: Add missing test cases

**Key diagrams to review:**
- [Consensus Validation Flow](../diagrams/architecture/consensus-validation-flow.md)
- [Ledger State Machine](../diagrams/architecture/ledger-state-machine.md)

### Week 3: Advanced Topics
1. AI: Map entire TAO register lifecycle
2. Human: Design new register type
3. AI: Generate boilerplate implementation
4. Human: Refine and optimize

**Key diagrams to review:**
- [Trust Network Topology](../diagrams/architecture/trust-network-topology.md)
- [Falcon Auth Sequence](../diagrams/protocols/falcon-auth-sequence.md)
- [ChaCha20 Session Lifecycle](../diagrams/protocols/chacha20-session-lifecycle.md)

---

## Codebase Map for AI Navigation

```
src/
├── LLP/          # Lower Level Protocol - networking & mining
│   ├── include/  # Headers: opcodes, falcon, push notifications
│   └── *.cpp     # Connection handling, packet processing
├── TAO/          # TAO Framework - registers, ledger, operations
│   ├── Register/ # Register types and state management
│   ├── Ledger/   # Block validation, consensus, chain state
│   └── Operation/# Register operations (WRITE, DEBIT, etc.)
├── Legacy/       # Legacy transaction support
└── Util/         # Utility functions and helpers
```

---

## AI Prompt Templates for Onboarding

### Day 1: Getting Oriented
```
"Give me an overview of the LLL-TAO repository structure and the purpose of each top-level directory"
"What are the main components: LLP, TAO, and Legacy?"
"How do I build LLL-TAO on Linux?"
```

### Day 2-3: Mining Deep Dive
```
"Trace the mining flow from MINER_AUTH to BLOCK_ACCEPTED"
"Compare Legacy Tritium (port 8323) and Stateless (port 9323) protocols"
"How does the push notification system work for new blocks?"
```

### Day 4-5: Consensus Understanding
```
"What validation checks does TritiumBlock::Check() perform?"
"How are the three consensus channels (PoS, Prime, Hash) different?"
"Explain the merkle root validation process"
```

---

## Cheat Sheets

- [Mining Debug](cheat-sheets/mining-debug.md) - Common mining issues and AI-assisted resolution
- [Consensus Debug](cheat-sheets/consensus-debug.md) - Validation failures and resolution
- [Performance Tuning](cheat-sheets/performance-tuning.md) - Optimization patterns

---

## Cross-References

- [AI-Human Simlink Diagram](../diagrams/ai-collaboration/ai-human-simlink.md)
- [Learning Pathways](../diagrams/ai-collaboration/learning-pathways.md)
- [Common Tasks Cheat Sheet](../diagrams/ai-collaboration/cheat-sheets/common-tasks.md)
- [AI-Human Advancement Thesis](../philosophy/ai-human-advancement.md)
- [Coding Agent Best Practices](../CODING_AGENT_BEST_PRACTICES.md)
