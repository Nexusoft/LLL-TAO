# GitHub Copilot Coding Agent Best Practices

## The Critical Rule: 3,000 Character Limit

**⚠️ HARD LIMIT: PR descriptions are truncated at approximately 3,000 characters.**

If your PR description exceeds this limit, the Coding Agent never sees the full requirements. This is the #1 reason for failed PRs. In PR #214, a 7,000 character description was truncated at ~3,000 chars, causing the agent to miss critical implementation details.

**Character count matters more than completeness. Every character must earn its place.**

## PR Description Structure Template

```markdown
## Problem
[2-3 sentences: What's broken or missing, why it matters] (150 chars)

## Solution
[1-2 sentences: High-level approach] (100 chars)

## Implementation
[Detailed requirements - THIS IS WHERE YOU SPEND YOUR BUDGET] (2,000+ chars)

### File: src/path/to/file.cpp
- Method `MethodName()`: Add validation for X condition
- Line 142: Change Y to Z because [reason]
- Add new method `NewMethod()` that [specific behavior]

### File: src/path/to/other.cpp
- Update imports: Add `#include "new_header.h"`
- Method `ExistingMethod()`: Add parameter `Type param` and use it to [specific logic]

## Constraints
[What NOT to change] (200 chars)
- Don't modify existing tests in test/legacy/
- Preserve backward compatibility for API endpoints
- Keep existing error messages unchanged

## Testing
[Specific test requirements] (300 chars)
- Verify X behavior with Y input
- Test edge case Z
- Run existing tests: `make test`

## Acceptance
[Checklist of completion criteria] (250 chars)
```

**This template totals ~2,800 characters, leaving room for expansion.**

## What the Agent Needs vs. Doesn't Need

### ✅ What the Agent NEEDS

| Need | Why | Character Budget |
|------|-----|------------------|
| **Precise file paths** | `src/LLP/miner.cpp` not "the miner file" | Essential |
| **Exact method names** | `ProcessBlock()` not "the block processor" | Essential |
| **Specific line numbers** | "Line 142" when referencing existing code | High value |
| **Logic requirements** | "Validate that X < Y before calling Z()" | 60-70% of budget |
| **Edge cases** | "Handle empty string, null, negative values" | Medium |
| **Constraints** | "Don't change backward compatibility" | High value |
| **Expected behavior** | "Should return false when timeout occurs" | High value |

### ❌ What the Agent DOESN'T Need

| Don't Include | Why | Characters Wasted |
|---------------|-----|-------------------|
| **Code examples** | Agent knows C++ syntax, doesn't need lessons | 500-1,000+ chars |
| **Implementation details** | Let the agent decide HOW to implement | 300-500 chars |
| **Background context** | Keep it minimal, focus on requirements | 200-400 chars |
| **Explanations of syntax** | Agent understands language features | 200-300 chars |
| **Multiple solution options** | Pick one, tell the agent what to do | 300-500 chars |

### Example: Bad vs. Good

**❌ BAD - Wastes 1,200 characters on code examples:**
```
## Implementation

We need to add validation to the ProcessBlock method. Here's how you might do it:

```cpp
bool ProcessBlock(const Block& block) {
    // Add this validation
    if (block.nHeight < 0) {
        return LogicError("Invalid block height");
    }
    
    // Also check the timestamp
    if (block.nTime > GetCurrentTime() + 7200) {
        return LogicError("Block timestamp too far in future");
    }
    
    // Then continue with existing logic
    return ValidateBlock(block);
}
```

This shows how to add the validation before processing. Make sure to handle edge cases like...
```

**Character count: ~1,200 characters**

**✅ GOOD - Same requirements in 250 characters:**
```
## Implementation

### File: src/TAO/Ledger/block.cpp

Method `ProcessBlock()`: Add validation before line 145:
- Reject if `block.nHeight < 0` with error "Invalid block height"
- Reject if `block.nTime > current_time + 7200` with error "Block timestamp too far in future"
```

**Character count: ~250 characters**

**Savings: 950 characters (79% reduction)**

## Architecture Guidance: Diagrams Over Code

When explaining complex interactions, use ASCII diagrams, not code examples. **Diagrams are 70-90% more character-efficient than code.**

### Why Diagrams Beat Code Examples

| Approach | Characters | Clarity |
|----------|-----------|---------|
| Code example | 800-1,200 | Shows HOW (unnecessary) |
| ASCII diagram | 50-200 | Shows WHAT (essential) |
| **Savings** | **600-1,000 chars** | **Better focus** |

### Effective Diagram Types

**1. Data Flow Diagrams (50-100 chars)**

Simple linear flow:
```
Client → API → Validation → Database → Response
         ↓
    Error Handler
```

Multi-path flow:
```
Request → Router → Auth? → [Yes] → Handler → Response
                     ↓              
                   [No] → 401 Error
```

**2. Component Architecture (100-150 chars)**

```
┌─────────┐    ┌──────────┐    ┌─────────┐
│ Miner   │───→│  Node    │───→│ Ledger  │
│ Client  │←───│  Cache   │←───│ Manager │
└─────────┘    └──────────┘    └─────────┘
```

Layered architecture:
```
┌─────────────────────┐
│   API Layer         │
├─────────────────────┤
│   Business Logic    │
├─────────────────────┤
│   Data Access       │
└─────────────────────┘
```

**3. State Machines (80-120 chars)**

```
[Idle] → [Validating] → [Processing] → [Complete]
   ↑          ↓              ↓              ↓
   └─────[Error]←───────────┴──────────────┘
```

**4. Class/Module Relationships (70-100 chars)**

```
Connection
    ├─→ Timestamp (nLastActive)
    ├─→ Authentication
    └─→ PurgeLogic
```

Hierarchical:
```
MinerConnection (base)
    ├─→ PoolConnection
    └─→ SoloConnection
```

**5. Sequence Diagrams (120-180 chars)**

```
Miner         Node          Ledger
  │──Submit──→ │             │
  │            │──Validate─→ │
  │            │←───OK───────│
  │←─Accept────│             │
```

**6. Network Topology (100-150 chars)**

```
        [Load Balancer]
           /    |    \
       API1   API2   API3
          \     |     /
          [Shared DB]
```

### Using Diagrams in PR Descriptions

**Example: Adding Cache Purge Logic**

Instead of showing implementation code (800+ chars), use a diagram (120 chars):

```markdown
## Implementation

### Architecture:
```
Connection Objects
    ↓
[Timer: 1hr] → Check nLastActive
    ↓
Remote: age > 7d  → Purge
Local:  age > 30d → Purge
```

### File: src/LLP/miner.cpp
- Add `uint64_t nLastActive` member
- Update in Submit(), Ping() methods
- New method: PurgeInactiveConnections()
```

**Character count: ~300 chars (diagram + requirements)**
**vs. code example: ~1,200 chars**
**Savings: 900 chars (75%)**

### Quick Diagram Templates

**Copy-paste these and customize:**

**Flow:**
```
A → B → C → D
    ↓
    E
```

**Branching:**
```
Input → Check? → [Yes] → Process
          ↓
        [No] → Reject
```

**Layers:**
```
┌─────┐
│  A  │
├─────┤
│  B  │
└─────┘
```

**Bidirectional:**
```
Client ←──→ Server ←──→ Database
```

**Tree:**
```
Root
 ├─ Branch1
 │   ├─ Leaf1
 │   └─ Leaf2
 └─ Branch2
```

### When to Use Which Diagram

| Use Case | Best Diagram Type | Char Count |
|----------|------------------|------------|
| API request flow | Data flow | 50-100 |
| System components | Box diagram | 100-150 |
| State transitions | State machine | 80-120 |
| Class inheritance | Tree | 70-100 |
| Async operations | Sequence | 120-180 |
| Error handling | Branching flow | 60-100 |

**❌ BAD - Code example for architecture (800 chars):**
```cpp
class MinerConnection {
    Connection* base;
    uint64_t timestamp;
    
    void handleSubmit() {
        if (validate()) {
            timestamp = now();
            process();
        }
    }
    
    void purge() {
        for (auto& conn : connections) {
            if (isExpired(conn)) {
                remove(conn);
            }
        }
    }
};
```

**✅ GOOD - Diagram for architecture (120 chars):**
```
MinerConnection
    ├─→ timestamp
    ├─→ handleSubmit() → validate → update timestamp
    └─→ purge() → check expiry → remove
```

**Saved: 680 chars (85% reduction)**

## Precision in Specifying Changes

### Be Specific About Locations

**❌ Vague:**
- "Update the validation logic"
- "Fix the miner code"
- "Add error handling"

**✅ Precise:**
- "File `src/LLP/miner.cpp`, method `Submit()`, add null check before line 89"
- "File `src/TAO/API/types/users.cpp`, method `Login()`, return 401 status on invalid password"
- "Add new file `src/Util/validators.h` with function `ValidateAddress()`"

### Specify What NOT to Change

**Critical: The agent can accidentally modify working code if you don't set boundaries.**

```markdown
## Constraints
- Don't modify existing test files in `tests/legacy/`
- Preserve all backward-compatible API endpoints in `src/TAO/API/`
- Keep existing error messages in `src/Util/errors.cpp` unchanged
- Don't refactor `src/LLP/legacy_protocol.cpp` (scheduled for separate PR)
```

## Testing Guidance

Be specific about what needs testing, but brief.

**✅ GOOD:**
```markdown
## Testing
- Verify timeout triggers after 60 seconds: Set `config.timeout=60` and wait
- Test with empty database: Delete `~/.Nexus/testnet.db` and restart
- Run existing tests: `make test-ledger`
```

**❌ BAD - Too verbose:**
```markdown
## Testing
First, you'll need to set up a test environment. Create a test configuration 
file with the following settings... [500 more characters]
```

## Character Budget Allocation

Based on successful PR #215, optimal allocation:

| Section | Percentage | Characters | Purpose |
|---------|-----------|------------|---------|
| **Implementation details** | 67% | ~2,000 | File paths, methods, logic requirements |
| **Constraints** | 10% | ~300 | What NOT to change |
| **Testing** | 10% | ~300 | Verification steps |
| **Problem/Solution** | 8% | ~250 | Context (minimal) |
| **Acceptance criteria** | 5% | ~150 | Completion checklist |
| **Total** | 100% | ~3,000 | Maximum usable space |

**Spend 2/3 of your budget on implementation requirements. That's what the agent needs most.**

## Common Mistakes

### Mistake 1: Including Code Examples (500-1,000 chars wasted)

**Why it's wrong:** The agent is an expert C++ developer. It doesn't need syntax lessons.

**Fix:** Describe WHAT to do, WHERE to do it, and WHY. Skip HOW.

### Mistake 2: Explaining Language Features (200-400 chars wasted)

**❌ Don't:**
```
Use std::optional which is a C++17 feature that can contain a value or be empty...
```

**✅ Do:**
```
Return std::optional<Block> instead of raw pointer
```

### Mistake 3: Multiple Solution Options (300-500 chars wasted)

**❌ Don't:**
```
You could either use a mutex, or use atomic operations, or use a lock-free queue...
```

**✅ Do:**
```
Use std::mutex to protect shared state
```

### Mistake 4: Verbose Background Context (200-600 chars wasted)

**❌ Don't:**
```
The Nexus project started in 2014 and has evolved through many iterations. 
The mining system was redesigned in 2019 to support...
```

**✅ Do:**
```
Mining needs rate limiting. Current code has no limits.
```

## Dependency Management

If adding new dependencies, be concise:

```markdown
## Dependencies
- Add `<optional>` header to src/TAO/Ledger/block.cpp
- Link against pthread: Update makefile.cli line 89, add `-lpthread`
```

**Don't explain what libraries do. The agent knows.**

## Real Example: Good PR Description

**From successful PR #215 (2,850 characters):**

```markdown
## Problem
Miner connections stay in node cache indefinitely causing memory growth. 
No purge mechanism exists. PR #214 attempt was truncated.

## Solution  
Add time-based purge of inactive miner cache entries.

## Implementation

### File: src/LLP/miner.cpp

Add class member to Connection:
- `uint64_t nLastActive` - timestamp of last activity

Method `Connection::Constructor()` line 67:
- Initialize `nLastActive = runtime::timestamp()`

Method `Submit()` line 142:
- Update timestamp: `nLastActive = runtime::timestamp()`

Method `Ping()` line 178:  
- Update timestamp: `nLastActive = runtime::timestamp()`

### File: src/LLP/miner.cpp

Add new method `PurgeInactiveConnections()`:
- Iterate through `mapConnections`
- Remove entries where `current_time - nLastActive > purge_timeout`
- Use 604800 seconds (7 days) for remote connections
- Use 2592000 seconds (30 days) for localhost connections
- Log purged connections: "Purged inactive miner: [address]"

Method `Timer()` line 234:
- Call `PurgeInactiveConnections()` every 86400 seconds (24 hours)

### File: src/LLP/types/miner.h

Add to class definition line 45:
- `uint64_t nLastActive;`

Add method declaration:
- `void PurgeInactiveConnections();`

## Constraints
- Don't modify handshake logic in `Authorize()`
- Keep existing cache limit (500) unchanged
- Preserve localhost/remote detection logic

## Testing
- Start node with `-miningpool=1`
- Connect miner, verify entry in cache
- Wait 8 days, verify remote miner purged
- Verify localhost miner NOT purged after 8 days
- Check logs for "Purged inactive miner" messages

## Acceptance
- Inactive remote miners purged after 7 days
- Localhost miners purged after 30 days  
- No memory leaks
- Existing tests pass: `make test-llp`
```

**Character count: 2,850 - Fits comfortably under limit**

## Quick Reference Checklist

Before submitting your PR description:

- [ ] Character count < 3,000 (check in text editor)
- [ ] Used ASCII diagrams for architecture (not code examples)
- [ ] No code examples included (describe requirements instead)
- [ ] All file paths are absolute from repo root
- [ ] All method names are exact and quoted
- [ ] Specified what NOT to change (constraints)
- [ ] Testing is specific and brief
- [ ] 60-70% of characters spent on implementation details
- [ ] No explanations of programming concepts
- [ ] No verbose background context
- [ ] Single clear solution (no options presented)

## Agent Chat Commands

If the agent gets stuck or confused during implementation:

**Unstick the agent:**
```
Focus on the requirements: [restate the core requirement in 1 sentence]
```

**Clarify requirements:**
```  
For file X, method Y: [one specific action]
```

**Redirect from wrong path:**
```
Stop. Don't modify [wrong file]. Instead, modify [correct file].
```

**Emergency reset:**
```
Discard changes to [file]. Start over with: [single requirement]
```

## Pre-Flight Checklist

Before submitting PR description to agent:

1. **Character count:** Paste description into text editor, count chars
2. **Remove code examples:** Delete all code blocks showing implementation
3. **Be specific:** Replace vague descriptions with exact file paths and method names
4. **Add constraints:** List what NOT to change
5. **Verify template:** Does it follow the structure template?
6. **Budget check:** 60-70% on implementation details?

## Emergency Recovery

If the agent starts going off-track:

**Scenario 1: Agent modifying wrong files**
```
Stop. The requirement is only for src/LLP/miner.cpp. Don't modify src/TAO/ files.
```

**Scenario 2: Agent asking for missing info**
```
[Provide the specific info in 1-2 sentences]
```

**Scenario 3: Agent over-engineering**
```
Simple solution only: [describe minimal change needed]
```

**Scenario 4: Truncation suspected**
```
Create a new PR with description under 2,800 characters. Focus only on: [core requirement]
```

## Summary: The Golden Rules

1. **3,000 character hard limit** - Every character must justify its existence
2. **WHAT and WHERE, not HOW** - Requirements, not implementation
3. **Use ASCII diagrams** - 50-200 chars vs 800-1,200 for code examples (70-90% savings)
4. **Agent is an expert** - No syntax lessons, no code examples
5. **Spend wisely** - 67% of budget on implementation details
6. **Be surgical** - Precise file paths, exact method names, specific line numbers
7. **Set boundaries** - Explicitly state what NOT to change
8. **Diagrams for architecture** - Flow, components, states convey structure efficiently
9. **One solution** - Don't present options, make decisions
10. **Minimal context** - Background info is expensive
11. **Test concisely** - Verification steps, not tutorials

**Remember: A well-crafted 2,800 character description with diagrams beats a truncated 7,000 character description with code every time.**
