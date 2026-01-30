# Coding Agent Cheat Sheet

## 🚨 CRITICAL RULES

1. **3,000 CHARACTER HARD LIMIT** - Descriptions truncate at ~3k. Agent never sees the rest.
2. **NO CODE EXAMPLES** - Agent knows C++. Code examples waste 500-1,000+ chars.
3. **WHAT/WHERE, NOT HOW** - Describe requirements, not implementation.

## Perfect PR Template (2,800 chars)

```markdown
## Problem
[2-3 sentences max] (150 chars)

## Solution
[1-2 sentences] (100 chars)

## Implementation

### File: src/exact/path/file.cpp
- Method `ExactName()` line X: [specific action]
- Add validation: [precise requirement]

### File: src/another/file.cpp  
- Import: Add `#include "header.h"`
- Method `OtherMethod()`: [specific behavior]

## Constraints
- Don't modify [specific files/methods]
- Preserve [specific behavior]

## Testing
- Verify [specific test] with [specific input]
- Run: `make test-component`

## Acceptance
- [ ] Behavior X works
- [ ] Tests pass
```

## Character Budget Table

| Section | % | Chars | Use For |
|---------|---|-------|---------|
| Implementation | 67% | 2,000 | File paths, methods, logic |
| Constraints | 10% | 300 | What NOT to change |
| Testing | 10% | 300 | Verification steps |
| Problem/Solution | 8% | 250 | Minimal context |
| Acceptance | 5% | 150 | Checklist |

**Spend 2/3 of budget on implementation requirements.**

## Do's and Don'ts

| ❌ DON'T | ✅ DO |
|---------|-------|
| Include code examples | Describe requirements |
| "Update the validation" | "`src/api.cpp` line 89: Add null check" |
| Explain language features | Use feature names (`std::optional`) |
| Present multiple options | Choose one solution |
| Add verbose context | 2-3 sentence problem statement |
| Write implementation tutorials | State specific actions |

## Precision Examples

**❌ Vague:**
- "Fix the miner code"
- "Add error handling"
- "Update validation"

**✅ Precise:**
- "`src/LLP/miner.cpp` method `Submit()` line 89: Add null check for `pBlock`"
- "Return HTTP 401 in `src/TAO/API/users.cpp` method `Login()` when password invalid"
- "`src/Util/validators.h`: Add function `ValidateAddress()` that checks length > 0"

## Agent Chat Commands

**Unstick:**
```
Focus: [restate core requirement in 1 sentence]
```

**Clarify:**
```
For file X, method Y: [one specific action]
```

**Redirect:**
```
Stop. Don't modify [wrong file]. Instead: [correct action]
```

**Simplify:**
```
Simple solution only: [minimal change]
```

## Pre-Flight Checklist

- [ ] Char count < 3,000 (paste in text editor)
- [ ] Zero code examples (only requirements)
- [ ] File paths are absolute from repo root  
- [ ] Method names exact and quoted
- [ ] Constraints listed (what NOT to change)
- [ ] Testing specific, not tutorial
- [ ] 60-70% chars on implementation
- [ ] No programming concept explanations

## Emergency Recovery

**Wrong files being modified:**
```
Stop. Only modify [correct file].
```

**Over-engineering:**
```
Minimal change: [single requirement]
```

**Suspected truncation:**
```
New PR under 2,800 chars: [core requirement only]
```

## Example: Character Waste

**❌ BAD (1,200 chars):**
```markdown
Here's how to add validation:

```cpp
bool ProcessBlock(const Block& block) {
    if (block.nHeight < 0) {
        return LogicError("Invalid height");
    }
    // ... more code ...
}
```
```

**✅ GOOD (250 chars):**
```markdown
`src/TAO/block.cpp` method `ProcessBlock()` line 145:
- Reject if `block.nHeight < 0` error "Invalid height"
```

**Saved: 950 chars (79%)**

## Golden Rules Summary

1. **3k limit** - No exceptions
2. **WHAT/WHERE** - Not HOW
3. **Expert agent** - Skip syntax lessons  
4. **67% implementation** - Requirements focused
5. **Surgical precision** - Exact paths/methods
6. **Set boundaries** - Say what NOT to change
7. **Diagrams > code** - Architecture simplified
8. **One solution** - Decide, don't present options
9. **Minimal context** - Every char counts
10. **Concise tests** - Steps, not tutorials

## Quick Calculation

**Check your description:**
- Paste into text editor
- Word count feature → multiply by ~5 = char estimate
- Or use: `echo "your text" | wc -c`
- **Must be < 3,000**

---

**Remember: 2,800 well-crafted characters beats 7,000 truncated characters every time.**
