# PR Template for Coding Agents

## Problem
[2-3 sentences max, ~150 chars. What breaks or is missing?]

## Solution
[1-2 sentences, ~100 chars. High-level approach.]

## Architecture Diagram
[Optional: ASCII diagram of components/relationships]

```
┌─────────┐    ┌──────────┐    ┌─────────┐
│ Component│───→│ Component│───→│Component│
│    A    │    │    B     │    │    C    │
└─────────┘    └──────────┘    └─────────┘
```

## Implementation (60-70% of character budget)

### File: src/exact/path/file.cpp
- Method `ExactName()` line X: [specific action]
- Add validation: [precise requirement]

### File: src/another/file.cpp
- Import: Add `#include "header.h"`
- Method `OtherMethod()`: [specific behavior]

## Flow Diagram
[Optional: Decision flow or state machine]

```
[Start] → [Validate] → [Process] → [End]
             ↓              ↓
         [Error]       [Success]
```

## Constraints (What NOT to change)
- Don't modify [specific files/methods]
- Preserve [specific behavior]

## Testing
- Verify [specific test] with [specific input]
- Run: `make test-component`

## Acceptance Criteria
- [ ] Behavior X works as specified
- [ ] All tests pass
- [ ] No regressions in [area]

---

**⚠️ CRITICAL: Keep total description under 3,000 characters. Check with `wc -c`**
