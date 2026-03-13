# PR #363 & PR #385 — ValidateConsistency Flow Diagram

**Section:** Node Architecture  
**Version:** LLL-TAO 5.1.0+ (post-PR #385)  
**Last Updated:** 2026-03-13  
**PRs:** [#363 — MergeContext sticky auth](https://github.com/NamecoinGithub/LLL-TAO/pull/363) · [#385 — ValidateConsistency gates + write-ahead fix](https://github.com/NamecoinGithub/LLL-TAO/pull/385)

---

## 1️⃣  Protocol Flow Layer — Full Handshake with All Three ValidateConsistency Gates

```
  🔌  MINER                                    🖥️  NODE  (port 9323 / 8325)
  ─────────────────────────────────────────────────────────────────────────
                                                  │
  ══════════════════  PHASE 1: AUTH HANDSHAKE  ══════════════════════════
                                                  │
  │──── MINER_AUTH_INIT (0xD0D0) ──────────────►│
  │                                               │  Generate Falcon challenge
  │◄─── MINER_AUTH_CHALLENGE (0xD0D1) ───────────│
  │                                               │
  │──── MINER_AUTH_RESPONSE (0xD0D3) ───────────►│
  │     [Falcon pubkey + signed challenge]        │  1. Verify Falcon signature
  │                                               │  2. Derive ChaCha20 key from genesis
  │                                               │  3. Register in NodeSessionRegistry
  │                                               │  4. SaveSession() to recovery cache
  │                                               │
  │                                               │  ┌──────────────────────────────────┐
  │                                               │  │ 🛡️  GATE #1 — MINER_AUTH        │
  │                                               │  │  ValidateConsistency()           │
  │                                               │  │  ✅ PR #363 (existing)           │
  │                                               │  │                                  │
  │                                               │  │  Checks:                         │
  │                                               │  │  • fAuthenticated → nSessionId≠0 │
  │                                               │  │  • fAuthenticated → hashGenesis≠0│
  │                                               │  │  • fAuthenticated → hashKeyID≠0  │
  │                                               │  │  • fRewardBound → hashReward≠0   │
  │                                               │  │  • fEncryptionReady → key≠empty  │
  │                                               │  │                                  │
  │                                               │  │  FAIL → AUTH_RESULT(0x00)        │
  │                                               │  └──────────────────────────────────┘
  │◄─── SESSION_START (0xD0D4) ──────────────────│
  │     [session_id | timeout | genesis_hash]     │
  │                                               │
  ══════════════════  PHASE 2: REWARD BINDING  ══════════════════════════
                                                  │
  │──── SET_REWARD (0xD0?) ─────────────────────►│  (optional — stateless or legacy port)
  │     [encrypted reward address payload]        │  1. Auth check
  │                                               │  2. Genesis check
  │                                               │
  │                                               │  ┌──────────────────────────────────┐
  │                                               │  │ 🛡️  GATE #2 — MINER_SET_REWARD  │
  │                                               │  │  ValidateConsistency()           │
  │                                               │  │  ✅ PR #385 (NEW)                │
  │                                               │  │                                  │
  │                                               │  │  Conditioned on hashKeyID≠0      │
  │                                               │  │  (legacy lane: hashKeyID lives   │
  │                                               │  │   in StatelessMinerManager, not  │
  │                                               │  │   in local context — skip gate)  │
  │                                               │  │                                  │
  │                                               │  │  FAIL → AUTH_RESULT(0x00)        │
  │                                               │  └──────────────────────────────────┘
  │                                               │  3. ChaCha20 key derivation
  │                                               │  4. Decrypt + bind reward address
  │◄─── AUTH_RESULT(0x01) ───────────────────────│
  │                                               │
  ══════════════════  PHASE 3: SUBSCRIBE + MINE  ════════════════════════
                                                  │
  │──── STATELESS_MINER_READY (0xD0D8) ─────────►│
  │                                               │
  │                                               │  ╔══════════════════════════════════╗
  │                                               │  ║ ⏱️  WRITE-AHEAD (PR #385 fix)   ║
  │                                               │  ║                                  ║
  │                                               │  ║  BEFORE: notification sent THEN  ║
  │                                               │  ║          SaveSession()           ║
  │                                               │  ║  ❌ Race: drop after notify but  ║
  │                                               │  ║     before save → stale height   ║
  │                                               │  ║                                  ║
  │                                               │  ║  AFTER (PR #385):                ║
  │                                               │  ║  1. Lookup authoritative context ║
  │                                               │  ║  2. Stamp nLastTemplateChannel-  ║
  │                                               │  ║     Height with current chain    ║
  │                                               │  ║  3. SaveSession() ← write-ahead  ║
  │                                               │  ║  4. THEN SendChannelNotification ║
  │                                               │  ╚══════════════════════════════════╝
  │◄─── BLOCK_DATA (template push) ─────────────│
  │                                               │
  ──── ⟳ MINING LOOP ───────────────────────────────────────────────────
  │                                               │
  │──── GET_BLOCK (0xD081) ─────────────────────►│
  │◄─── BLOCK_DATA (0xD082) [216-byte template] ─│
  │                                               │
  │──── SUBMIT_BLOCK (0xD089) ──────────────────►│
  │     [encrypted Falcon-signed solution]        │  1. Auth check
  │                                               │  2. Channel check
  │                                               │  3. ChaCha20 encryption check
  │                                               │
  │                                               │  ┌──────────────────────────────────┐
  │                                               │  │ 🛡️  GATE #3 — SUBMIT_BLOCK      │
  │                                               │  │  ValidateConsistency()           │
  │                                               │  │  ✅ PR #385 (NEW)                │
  │                                               │  │                                  │
  │                                               │  │  Stateless port (9323):          │
  │                                               │  │  context.ValidateConsistency()   │
  │                                               │  │  FAIL → STATELESS_BLOCK_REJECTED │
  │                                               │  │                                  │
  │                                               │  │  Legacy port (8325):             │
  │                                               │  │  Looks up authoritative context  │
  │                                               │  │  from StatelessMinerManager      │
  │                                               │  │  (has real hashKeyID)            │
  │                                               │  │  FAIL → BLOCK_REJECTED           │
  │                                               │  └──────────────────────────────────┘
  │                                               │  4. ChaCha20 decrypt
  │                                               │  5. Falcon signature verify
  │                                               │  6. Block validate + accept
  │◄─── STATELESS_BLOCK_ACCEPTED (0xD08B) ───────│
  │   or STATELESS_BLOCK_REJECTED (0xD08A) ───────│
  │                                               │
  ──── ⟳ SESSION KEEPALIVE ─────────────────────────────────────────────
  │                                               │
  │──── SESSION_STATUS (0xD0DB) ────────────────►│  [session_id | status_flags]
  │◄─── SESSION_STATUS_ACK (0xD0DC) ─────────────│  [session_id | lane_health | uptime | echo]
  │                                               │
  ─────────────────────────────────────────────────────────────────────
```

---

## 2️⃣  PR #363 Layer — MergeContext Before/After & Partial-Merge Regression

### 🔄 MergeContext Diff: Sticky Authentication

```
  ╔══════════════════════════════════════════════════════════════════════╗
  ║  🔴  BEFORE PR #363 — MergeContext (BUG)                           ║
  ╠══════════════════════════════════════════════════════════════════════╣
  ║                                                                      ║
  ║  void MinerSessionContainer::MergeContext(const MiningContext& ctx) ║
  ║  {                                                                   ║
  ║      ...                                                             ║
  ║      fAuthenticated = context.fAuthenticated;  ← UNCONDITIONAL      ║
  ║      ...                                                             ║
  ║  }                                                                   ║
  ║                                                                      ║
  ║  PROBLEM: A partial refresh or channel-switch carrying               ║
  ║  fAuthenticated=false silently de-authenticates a recovered session. ║
  ╚══════════════════════════════════════════════════════════════════════╝

  ╔══════════════════════════════════════════════════════════════════════╗
  ║  🟢  AFTER PR #363 — MergeContext (FIXED)                          ║
  ╠══════════════════════════════════════════════════════════════════════╣
  ║                                                                      ║
  ║  void MinerSessionContainer::MergeContext(const MiningContext& ctx) ║
  ║  {                                                                   ║
  ║      ...                                                             ║
  ║      if(context.fAuthenticated)     ← STICKY: only set true        ║
  ║          fAuthenticated = true;     ← NEVER cleared by merge        ║
  ║      ...                                                             ║
  ║  }                                                                   ║
  ║                                                                      ║
  ║  FIX: Authentication is sticky recovery state.                       ║
  ║  Only explicit session teardown / expiry clears fAuthenticated.      ║
  ╚══════════════════════════════════════════════════════════════════════╝
```

### 📋 Partial-Merge Sequence Table

```
  ┌──────────────────────────────────────────────────────────────────────┐
  │  Scenario: Miner reconnects mid-session (e.g. channel switch)        │
  ├─────────────┬────────────────────┬────────────────────┬─────────────┤
  │  Step       │  Incoming Context  │  BEFORE PR #363    │  AFTER #363 │
  ├─────────────┼────────────────────┼────────────────────┼─────────────┤
  │  1. Initial │ fAuthenticated=✅  │ container: auth=✅ │ auth=✅     │
  │  SaveSession│ nSessionId=12345   │                    │             │
  ├─────────────┼────────────────────┼────────────────────┼─────────────┤
  │  2. Channel │ fAuthenticated=❌  │ ❌ container:      │ ✅ auth     │
  │  switch     │ (fresh ctx default)│    auth=❌         │    stays ✅ │
  │  UpdateSess │ nChannel=2         │    SILENT BUG      │             │
  ├─────────────┼────────────────────┼────────────────────┼─────────────┤
  │  3. Miner   │ SUBMIT_BLOCK       │ ❌ rejected as     │ ✅ block    │
  │  submits    │ fAuthenticated=?   │    unauthenticated │    proceeds │
  │  block      │                    │    (no error log)  │             │
  └─────────────┴────────────────────┴────────────────────┴─────────────┘
```

### 🧪 PR #363 Regression Test (session_recovery.cpp)

```
  TEST_CASE: "SessionRecoveryData Basic Tests"
  SECTION:   (implicit in MergeContext validation chain)

  // Arrange: initial authenticated session
  MiningContext authCtx = MiningContext()
      .WithAuth(true)
      .WithSession(12345)
      .WithKeyId(testKeyId)
      .WithGenesis(testGenesis);

  SessionRecoveryData data(authCtx);
  REQUIRE(data.fAuthenticated == true);   // ← initial state

  // Act: partial merge with fAuthenticated=false (channel-switch refresh)
  MiningContext refreshCtx = MiningContext()
      .WithAuth(false)                     // ← fresh default context
      .WithChannel(2);                     // ← only channel changes

  data.MergeContext(refreshCtx);

  // Assert: auth state must be preserved (sticky)
  REQUIRE(data.fAuthenticated == true);   // ← MUST remain true after PR #363
  REQUIRE(data.nChannel == 2);            // ← channel update applied
```

---

## 3️⃣  PR #385 Layer — Three Gates + Port Differences + Write-Ahead Fix

### 🛡️ All Three Gate Call Sites

```
  ╔══════════════════════════════════════════════════════════════════════╗
  ║  🔵  GATE #1 — MINER_AUTH (PR #363, existing)                      ║
  ╠══════════════════════════════════════════════════════════════════════╣
  ║                                                                      ║
  ║  File: src/LLP/stateless_miner.cpp                                  ║
  ║  Function: StatelessMiner::ProcessFalconResponse()                  ║
  ║                                                                      ║
  ║  const SessionConsistencyResult consistency =                        ║
  ║      context.ValidateConsistency();                                  ║
  ║  if(consistency != SessionConsistencyResult::Ok)                     ║
  ║  {                                                                   ║
  ║      // send AUTH_RESULT(0x00) failure                               ║
  ║      return AUTH_FAILED_RESULT;                                      ║
  ║  }                                                                   ║
  ║                                                                      ║
  ║  Port: 9323 (stateless) only                                         ║
  ║  Conditioned on: post-auth, full context available                   ║
  ╚══════════════════════════════════════════════════════════════════════╝

  ╔══════════════════════════════════════════════════════════════════════╗
  ║  🔵  GATE #2 — MINER_SET_REWARD (PR #385, NEW)                     ║
  ╠══════════════════════════════════════════════════════════════════════╣
  ║                                                                      ║
  ║  File: src/LLP/stateless_miner.cpp                                  ║
  ║  Function: StatelessMiner::ProcessSetReward()                       ║
  ║                                                                      ║
  ║  // ⚠️  Port-specific: legacy lane hashKeyID=0 by design            ║
  ║  // (real hashKeyID lives in StatelessMinerManager, not here)        ║
  ║  if(context.hashKeyID != 0)   ← skip gate for legacy lane           ║
  ║  {                                                                   ║
  ║      const SessionConsistencyResult consistency =                    ║
  ║          context.ValidateConsistency();                              ║
  ║      if(consistency != SessionConsistencyResult::Ok)                 ║
  ║      {                                                               ║
  ║          debug::log(0, FUNCTION,                                     ║
  ║              "Session consistency violation at MINER_SET_REWARD: ",  ║
  ║              SessionConsistencyResultString(consistency));           ║
  ║          StatelessPacket authFailed(AUTH_RESULT);                   ║
  ║          authFailed.DATA.push_back(0x00);  // failure byte          ║
  ║          return ProcessResult::Success(context, authFailed);         ║
  ║      }                                                               ║
  ║  }                                                                   ║
  ║                                                                      ║
  ║  Port: 9323 stateless → full gate; 8325 legacy → guarded by keyID≠0 ║
  ╚══════════════════════════════════════════════════════════════════════╝

  ╔══════════════════════════════════════════════════════════════════════╗
  ║  🔵  GATE #3 — SUBMIT_BLOCK (PR #385, NEW)                         ║
  ╠══════════════════════════════════════════════════════════════════════╣
  ║                                                                      ║
  ║  --- Stateless port (9323) ---                                        ║
  ║  File: src/LLP/stateless_miner_connection.cpp                        ║
  ║                                                                      ║
  ║  const SessionConsistencyResult consistency =                        ║
  ║      context.ValidateConsistency();  ← uses LOCAL context            ║
  ║  if(consistency != SessionConsistencyResult::Ok)                     ║
  ║  {                                                                   ║
  ║      StatelessPacket response(STATELESS_BLOCK_REJECTED);            ║
  ║      respond(response);                                              ║
  ║      return true;   // packet consumed                               ║
  ║  }                                                                   ║
  ║                                                                      ║
  ║  --- Legacy port (8325) ---                                           ║
  ║  File: src/LLP/miner.cpp                                             ║
  ║                                                                      ║
  ║  // ⚠️  Must look up AUTHORITATIVE context from StatelessMinerManager║
  ║  // because legacy port's local context has hashKeyID=0 by design.  ║
  ║  auto authCtx = StatelessMinerManager::Get()                         ║
  ║                     .GetMinerContext(strLookupAddr);                 ║
  ║  const SessionConsistencyResult consistency =                        ║
  ║      authCtx->ValidateConsistency();  ← authoritative hashKeyID     ║
  ║  if(consistency != SessionConsistencyResult::Ok)                     ║
  ║  {                                                                   ║
  ║      respond(BLOCK_REJECTED);                                        ║
  ║      return true;                                                    ║
  ║  }                                                                   ║
  ╚══════════════════════════════════════════════════════════════════════╝
```

### ⏱️ Write-Ahead Race Window — Before and After

```
  ╔══════════════════════════════════════════════════════════════════════╗
  ║  🔴  BEFORE PR #385 — MINER_READY Write-Ahead Bug (Legacy Port)    ║
  ╠══════════════════════════════════════════════════════════════════════╣
  ║                                                                      ║
  ║  handle_miner_ready_stateless()                                      ║
  ║  {                                                                   ║
  ║      ...                                                             ║
  ║      SendChannelNotification();  ← notification sent FIRST           ║
  ║      SaveSession(context);       ← save happens AFTER               ║
  ║  }                                                                   ║
  ║                                                                      ║
  ║  ⚠️  RACE WINDOW:                                                   ║
  ║      If TCP drops between SendChannelNotification() and SaveSession, ║
  ║      the recovery cache holds stale channel height → on reconnect    ║
  ║      stale-height throttle suppresses template push.                 ║
  ║                                                                      ║
  ║  Timeline:                                                           ║
  ║  ──────────────────────────────────────────────────────────────────  ║
  ║  t₀  MINER_READY received                                           ║
  ║  t₁  SendChannelNotification() → miner gets template push            ║
  ║  t₂  💥 TCP DROP HERE (before save)                                  ║
  ║  t₃  Recovery cache: stale channelHeight from BEFORE this session    ║
  ║  t₄  Miner reconnects → MINER_READY again                           ║
  ║  t₅  Stale-height throttle fires → no template push! 🔇             ║
  ╚══════════════════════════════════════════════════════════════════════╝

  ╔══════════════════════════════════════════════════════════════════════╗
  ║  🟢  AFTER PR #385 — Write-Ahead Fixed                             ║
  ╠══════════════════════════════════════════════════════════════════════╣
  ║                                                                      ║
  ║  handle_miner_ready_stateless()                                      ║
  ║  {                                                                   ║
  ║      // 1. Lookup authoritative context from StatelessMinerManager   ║
  ║      auto optCtx = StatelessMinerManager::Get()                      ║
  ║                        .GetMinerContext(strLookupAddr);              ║
  ║      if(optCtx.has_value() && optCtx->fAuthenticated               ║
  ║         && optCtx->hashKeyID != 0)                                  ║
  ║      {                                                               ║
  ║          MiningContext updatedCtx = optCtx.value();                  ║
  ║          // 2. Stamp current chain channel height                    ║
  ║          updatedCtx = updatedCtx                                     ║
  ║              .WithLastTemplateChannelHeight(nChannelHeight);         ║
  ║          // 3. ✅ SaveSession BEFORE notification                    ║
  ║          SessionRecoveryManager::Get().SaveSession(updatedCtx);      ║
  ║      }                                                               ║
  ║      // 4. NOW send notification (safe: cache is up to date)         ║
  ║      SendChannelNotification();                                       ║
  ║  }                                                                   ║
  ║                                                                      ║
  ║  Timeline:                                                           ║
  ║  ──────────────────────────────────────────────────────────────────  ║
  ║  t₀  MINER_READY received                                           ║
  ║  t₁  SaveSession(fresh channelHeight) ← write-ahead ✅              ║
  ║  t₂  SendChannelNotification() → miner gets template push            ║
  ║  t₃  💥 TCP DROP HERE (after save)                                   ║
  ║  t₄  Recovery cache: FRESH channelHeight ✅                          ║
  ║  t₅  Miner reconnects → template push served immediately 📤          ║
  ╚══════════════════════════════════════════════════════════════════════╝
```

---

## 4️⃣  Test Coverage Matrix — 15 Assertions Across Both Test Cases

### 📁 File: `tests/unit/LLP/session_status_tests.cpp`

```
  ┌──────────────────────────────────────────────────────────────────────────────────┐
  │  TEST_CASE: "ValidateConsistency rejects structurally inconsistent contexts"     │
  │  Tag:       [llp][session_status][consistency]                                   │
  ├──────┬──────────────────────────────────────────────┬──────────────┬────────────┤
  │  #   │  SECTION                                      │  REQUIRE     │  Assertion │
  ├──────┼──────────────────────────────────────────────┼──────────────┼────────────┤
  │  1   │ Authenticated ctx missing session ID          │ ==           │ Missing-   │
  │      │                                               │              │ SessionId  │
  ├──────┼──────────────────────────────────────────────┼──────────────┼────────────┤
  │  2   │ Authenticated ctx missing session ID          │ !=           │ Not Ok     │
  ├──────┼──────────────────────────────────────────────┼──────────────┼────────────┤
  │  3   │ Authenticated ctx missing genesis hash        │ ==           │ Missing-   │
  │      │                                               │              │ Genesis    │
  ├──────┼──────────────────────────────────────────────┼──────────────┼────────────┤
  │  4   │ Authenticated ctx missing Falcon key ID       │ ==           │ Missing-   │
  │      │                                               │              │ FalconKey  │
  ├──────┼──────────────────────────────────────────────┼──────────────┼────────────┤
  │  5   │ Reward-bound ctx with zero reward address     │ ==           │ Reward-    │
  │      │                                               │              │ BoundMiss  │
  ├──────┼──────────────────────────────────────────────┼──────────────┼────────────┤
  │  6   │ Fully valid authenticated context             │ ==           │ Ok         │
  ├──────┼──────────────────────────────────────────────┼──────────────┼────────────┤
  │  7   │ Unauthenticated context always passes         │ ==           │ Ok         │
  ├──────┴──────────────────────────────────────────────┴──────────────┴────────────┤
  │  Subtotal: 7 assertions                                                          │
  └──────────────────────────────────────────────────────────────────────────────────┘

  ┌──────────────────────────────────────────────────────────────────────────────────┐
  │  TEST_CASE: "ValidateConsistency gate: SUBMIT_BLOCK rejected on inconsistent     │
  │              session"                                                            │
  │  Tag:       [llp][session_status][consistency]                                   │
  ├──────┬──────────────────────────────────────────────┬──────────────┬────────────┤
  │  #   │  SECTION                                      │  REQUIRE     │  Assertion │
  ├──────┼──────────────────────────────────────────────┼──────────────┼────────────┤
  │  8   │ Good context passes the gate                  │ == false     │ Not rej.   │
  ├──────┼──────────────────────────────────────────────┼──────────────┼────────────┤
  │  9   │ Zero session ID triggers rejection            │ == true      │ Rejected   │
  ├──────┼──────────────────────────────────────────────┼──────────────┼────────────┤
  │  10  │ SessionConsistencyResultString non-empty      │ .size() > 0  │ Missing-   │
  │      │ for MissingSessionId                          │              │ SessionId  │
  ├──────┼──────────────────────────────────────────────┼──────────────┼────────────┤
  │  11  │ SessionConsistencyResultString non-empty      │ .size() > 0  │ Missing-   │
  │      │ for MissingGenesis                            │              │ Genesis    │
  ├──────┼──────────────────────────────────────────────┼──────────────┼────────────┤
  │  12  │ SessionConsistencyResultString non-empty      │ .size() > 0  │ Missing-   │
  │      │ for MissingFalconKey                          │              │ FalconKey  │
  ├──────┼──────────────────────────────────────────────┼──────────────┼────────────┤
  │  13  │ SessionConsistencyResultString non-empty      │ .size() > 0  │ RewardBound│
  │      │ for RewardBoundMissingHash                    │              │ MissingH.  │
  ├──────┼──────────────────────────────────────────────┼──────────────┼────────────┤
  │  14  │ SessionConsistencyResultString non-empty      │ .size() > 0  │ Encryption-│
  │      │ for EncryptionReadyMissingKey                 │              │ Missing    │
  ├──────┼──────────────────────────────────────────────┼──────────────┼────────────┤
  │  15  │ SessionConsistencyResultString non-empty      │ .size() > 0  │ Ok string  │
  │      │ for Ok                                        │              │            │
  ├──────┴──────────────────────────────────────────────┴──────────────┴────────────┤
  │  Subtotal: 8 assertions                                                          │
  └──────────────────────────────────────────────────────────────────────────────────┘

  ════════════════════════════════════════════════════════════
   🎯  TOTAL: 15 assertions across both test cases  ✅
  ════════════════════════════════════════════════════════════
```

---

## 5️⃣  Dependency Chain — Why PR #363 Must Land Before PR #385

```
  ╔══════════════════════════════════════════════════════════════════════╗
  ║  🔗  Dependency: #363 → #385                                        ║
  ╠══════════════════════════════════════════════════════════════════════╣
  ║                                                                      ║
  ║  PR #385 adds ValidateConsistency() gates at MINER_SET_REWARD and   ║
  ║  SUBMIT_BLOCK. These gates call context.ValidateConsistency() or    ║
  ║  recoveredData.ValidateConsistency().                                ║
  ║                                                                      ║
  ║  PR #363 introduces the sticky-auth fix in MergeContext():           ║
  ║      if(context.fAuthenticated) fAuthenticated = true;              ║
  ║                                                                      ║
  ║  WITHOUT PR #363 MERGED:                                             ║
  ║  ──────────────────────────────────────────────────────────────────  ║
  ║  A legitimate reconnecting miner CAN have fAuthenticated=false in   ║
  ║  the recovery container after a channel-switch partial merge.        ║
  ║                                                                      ║
  ║  The #385 gate at SUBMIT_BLOCK sees:                                 ║
  ║      context.fAuthenticated == false                                 ║
  ║      → ValidateConsistency() returns Ok  (unauthenticated = no      ║
  ║        invariants apply; no nSessionId check triggered)             ║
  ║                                                                      ║
  ║  RESULT: The gate silently passes a de-authenticated session         ║
  ║  that should be rejected. PR #385 gates are only trustworthy        ║
  ║  AFTER #363 ensures fAuthenticated is never falsely cleared.         ║
  ║                                                                      ║
  ╠══════════════════════════════════════════════════════════════════════╣
  ║  Silent Failures Each PR Independently Prevents                      ║
  ╠══════════════════════════════════════════════════════════════════════╣
  ║                                                                      ║
  ║  PR #363 alone prevents:                                             ║
  ║    🔇 Silent de-authentication: a fresh-context partial merge        ║
  ║       carries fAuthenticated=false and silently degrades a           ║
  ║       valid recovered session to unauthenticated state. The miner   ║
  ║       receives no error — it simply stops getting templates.         ║
  ║                                                                      ║
  ║  PR #385 alone prevents (given #363 already merged):                 ║
  ║    🔓 Corrupted container accepted at key-material boundary:         ║
  ║       a container with fAuthenticated=true but nSessionId=0 or      ║
  ║       hashGenesis=0 would pass all pre-gate checks and reach         ║
  ║       ChaCha20 key access / Falcon verify with invalid state.        ║
  ║    ⏱️  Stale height on reconnect: the legacy port MINER_READY        ║
  ║       handler sent the push notification before persisting the       ║
  ║       updated channel height, leaving a race window.                 ║
  ║                                                                      ║
  ╠══════════════════════════════════════════════════════════════════════╣
  ║  Merge Order                                                          ║
  ╠══════════════════════════════════════════════════════════════════════╣
  ║                                                                      ║
  ║     ┌─────────────────────────────────────────────────────────┐     ║
  ║     │                                                         │     ║
  ║     │  1️⃣  Merge PR #363 first (sticky auth)                 │     ║
  ║     │       ↓                                                 │     ║
  ║     │  2️⃣  Now PR #385 gates are trustworthy                 │     ║
  ║     │       (fAuthenticated is never falsely cleared)          │     ║
  ║     │       ↓                                                 │     ║
  ║     │  3️⃣  Merge PR #385 (ValidateConsistency at all         │     ║
  ║     │       security boundaries + write-ahead fix)            │     ║
  ║     │                                                         │     ║
  ║     └─────────────────────────────────────────────────────────┘     ║
  ║                                                                      ║
  ║  Both PRs have been merged to STATELESS-NODE ✅                      ║
  ╚══════════════════════════════════════════════════════════════════════╝
```

---

## Summary Reference

| Layer | PR | Key Change | Silent Failure Prevented |
|---|---|---|---|
| 🔄 MergeContext sticky auth | #363 | `if(ctx.fAuthenticated) fAuthenticated=true` | Silent de-authentication on partial refresh |
| 🛡️ Gate #1 MINER_AUTH | #363 | `ValidateConsistency()` at end of auth handler | Corrupt container proceeds past auth |
| 🛡️ Gate #2 SET_REWARD | #385 | `ValidateConsistency()` before ChaCha20 key use | Inconsistent container reaches key material |
| 🛡️ Gate #3 SUBMIT_BLOCK | #385 | `ValidateConsistency()` before decrypt/verify | Inconsistent container reaches block accept |
| ⏱️ Write-ahead | #385 | `SaveSession()` before `SendChannelNotification()` | Stale channel height on reconnect suppresses template |
| 🧪 Regression (auth sticky) | #363 | `session_recovery.cpp` test case | Regression of sticky-auth behaviour |
| 🧪 Gate tests (15 assertions) | #385 | `session_status_tests.cpp` two test cases | Gate rejection logic not exercised in CI |

---

*See also: [`02-canonical-validate-consistency.md`](../../diagrams/upgrade-path/02-canonical-validate-consistency.md) for the upgrade-path roadmap item tracking these gates.*
