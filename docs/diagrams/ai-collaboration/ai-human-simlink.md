# AI-Human Symbiotic Link

Diagram showing the complementary strengths of AI and human developers working together on the Nexus codebase.

---

## AI-Human Strengths & Symbiotic Output

```mermaid
graph LR
    subgraph "AI Strengths (LLM)"
        A1[Pattern Recognition<br/>across 1M+ files]
        A2[Instant Code Navigation<br/>find any function]
        A3[Documentation Synthesis<br/>cross-reference all docs]
        A4[Consistency Checking<br/>API validation]
    end

    subgraph "Human Strengths"
        H1[Intuition & Design<br/>architecture decisions]
        H2[Context Understanding<br/>business requirements]
        H3[Testing Strategy<br/>edge case discovery]
        H4[Code Review Judgment<br/>quality assessment]
    end

    subgraph "Symbiotic Output"
        S1[Rapid Prototyping<br/>AI scaffolds, Human refines]
        S2[Bug-Free Code<br/>AI catches patterns, Human tests logic]
        S3[Complete Documentation<br/>AI generates, Human validates]
        S4[Architecture Evolution<br/>Human designs, AI implements]
    end

    A1 --> S1
    A2 --> S2
    A3 --> S3
    A4 --> S4
    H1 --> S1
    H2 --> S2
    H3 --> S3
    H4 --> S4

    S1 -.->|Learning Loop| H1
    S2 -.->|Learning Loop| A2
    S3 -.->|Learning Loop| A3
    S4 -.->|Learning Loop| H4
```

---

## Collaboration Workflow

```mermaid
flowchart TD
    TASK[New Development Task] --> PLAN{Who Plans?}

    PLAN -->|Architecture| HUMAN[Human: Design & Decide]
    PLAN -->|Implementation| AI[AI: Scaffold & Generate]

    HUMAN --> SPEC[Human writes spec]
    SPEC --> AIGEN[AI generates implementation]
    AIGEN --> REVIEW[Human reviews & refines]
    REVIEW --> AITEST[AI generates test coverage]
    AITEST --> HTEST[Human adds edge case tests]
    HTEST --> AIDOC[AI generates documentation]
    AIDOC --> HDOC[Human validates accuracy]
    HDOC --> DONE[Ship It]
```

---

## When to Use AI vs Human Judgment

| Task | AI | Human | Together |
|------|-----|-------|----------|
| Find all usages of a function | ✅ Fast & complete | ❌ Slow & error-prone | AI finds, Human analyzes |
| Design new API endpoint | ❌ Lacks context | ✅ Understands requirements | Human designs, AI implements |
| Write boilerplate code | ✅ Consistent & fast | ❌ Tedious & error-prone | AI generates, Human reviews |
| Security review | ⚠️ Pattern-based only | ✅ Contextual reasoning | AI flags, Human validates |
| Cross-reference documentation | ✅ Instant & thorough | ❌ Time-consuming | AI synthesizes, Human verifies |
| Debug complex race condition | ❌ Limited reasoning | ✅ Intuition & experience | AI traces, Human diagnoses |

---

## Cross-References

- [Learning Pathways](learning-pathways.md)
- [Common Tasks Cheat Sheet](cheat-sheets/common-tasks.md)
- [AI-Assisted Onboarding](../../onboarding/ai-assisted-onboarding.md)
- [AI-Human Advancement Thesis](../../philosophy/ai-human-advancement.md)
