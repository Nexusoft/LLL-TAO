# New Coder Repo Guide

## 1. Start With the Actual Shape of the Repository

Treat the repo as a set of operating systems for one node, not as a single C++
application:

- `src/LLP/` - networking, mining, session management, failover, connection lifecycles
- `src/TAO/` - consensus, ledger, contracts, registers, API behavior
- `src/LLD/` - persistent storage and database access
- `src/Legacy/` - backward-compatible wallet and RPC surface
- `tests/unit`, `tests/smoke`, `tests/live`, `tests/bench` - test layers for different risk levels
- `makefile.cli` - the active build entrypoint in this branch

The practical onboarding path is:

```bash
sudo bash contrib/devtools/install-build-deps.sh
make -f makefile.cli -j$(nproc)
make -f makefile.cli UNIT_TESTS=1 -j$(nproc)
```

## 2. Current Automation Reality

Before proposing new automation, understand what already exists:

- `.devcontainer/devcontainer.json` sets up the developer container and calls `.devcontainer/setup.sh`
- `.devcontainer/setup.sh` delegates dependency installation to `contrib/devtools/install-build-deps.sh`
- `.github/workflows/copilot-setup-steps.yml` currently validates setup/install only
- As of 2026-04-15, no in-repo references to `AEP-agent-element-protocol` or `dynAEP-dynamic-agent-element-protocol` were found in the repository search across `src/`, `docs/`, `.github/`, and the top-level docs

Conclusion: AEP/dynAEP should currently be treated as optional orchestration around
the repository, not as a core runtime or consensus dependency.

## 3. The Architectural Truth to Learn Early

The most expensive failures here are cross-layer correctness failures:

- networking ↔ mining
- mining ↔ sync
- sync ↔ shutdown
- shutdown ↔ thread ownership

This means "performance" and "correctness" often collapse into the same problem.
Slow, ambiguous, or under-specified state transitions can become stale templates,
dropped sessions, blocked shutdown, or fragile failover.

Recommended reading order:

1. `README.md`
2. `docs/onboarding/ai-assisted-onboarding.md`
3. `src/LLP/` mining and session paths
4. `src/TAO/Ledger/` consensus and sync paths

## 4. Recently Completed Hardening and Remaining Architectural Risks

These should drive workflow design and refactoring order.

### Recently Completed LLP Hardening

1. **Sync failover is now bounded instead of recursive**
   - `src/LLP/tritium.cpp`
   - `TritiumNode::SwitchNode()` now uses bounded retry logic with optional session exclusion instead of recursively calling itself from inside the exception path
   - Effect: lower stack risk, clearer retry behavior, better failure containment

2. **Shutdown now keeps thread ownership local**
   - `src/LLP/data.cpp`
   - `DataThread` shutdown now releases triggers, disconnects connections cooperatively, and joins worker threads directly
   - Effect: avoids detached waiter lifetime hazards and keeps thread/object ownership aligned

3. **Sync batching now separates block budget from inventory batch size**
   - `src/LLP/tritium.cpp`
   - block-response control is now tracked with `nBlockBudget`, while transaction/inventory reads keep their own batch size semantics
   - Effect: easier reasoning about sync response size and fewer budget-coupling regressions

### Remaining Architectural Risks

1. **Dual mining server lifecycle cost**
   - `src/LLP/include/global.h`
   - `src/LLP/global.cpp`
   - node runs both `MINING_SERVER` and `STATELESS_MINER_SERVER`
   - Risk: duplicated threads, lifecycle management, memory, and operational complexity

2. **Onboarding / CI command drift**
   - contributor-facing docs and automation need to stay aligned with `make -f makefile.cli ...`
   - Risk: new contributors use the wrong command path and misdiagnose the build

## 5. Optimization Paths That Reduce Risk

Prioritize changes that make system behavior easier to reason about:

1. Preserve the completed LLP hardening by adding focused regression coverage around retry, shutdown, and sync budgeting behavior
2. Keep shared mining abstractions, but only unify server internals where protocol differences are small
3. Keep docs, devcontainer instructions, and CI commands aligned
4. Treat `.devcontainer` + GitHub Actions as the first orchestration surface for any future external automation

## 6. Using AEP / dynAEP With `.devcontainer` Automated Testing

### Good Uses

- Test orchestration or scenario specification around existing repo commands
- Structured onboarding prompts for new contributors
- Failure triage workflows
- Repeatable agent tasks for bootstrap, smoke build, and unit-test build

### Concrete In-Repo Integration Points

- `.devcontainer/devcontainer.json`
  - `postCreateCommand`
  - `updateContentCommand`
- `.devcontainer/setup.sh`
  - shared dependency/bootstrap entrypoint
- `.github/workflows/copilot-setup-steps.yml`
  - setup validation workflow with `workflow_dispatch`
- `.github/workflows/devcontainer-validate.yml`
  - smoke-build and unit-test-build workflow with `workflow_dispatch`

These are orchestration edges around the repository. No current AEP/dynAEP
references were found in runtime, consensus, networking, or mining code paths.

### Bad Use Right Now

- Making node runtime or consensus correctness depend on an external orchestration repo
- Making CI success depend on AEP/dynAEP before security, maintenance, and ownership fit are proven

## 7. Practical Workflow Additions

Recommended baseline automation:

1. setup validation: run `contrib/devtools/install-build-deps.sh`
2. smoke build: run `make -f makefile.cli -j2`
3. unit-test build: run `make -f makefile.cli UNIT_TESTS=1 -j2`
4. later: add targeted execution for the most stable test tags/suites

## 8. One Thing to Remember

In this repo, performance bugs often become correctness bugs. The best early
improvements are the ones that make thread ownership, retry behavior, and state
transitions obvious.
