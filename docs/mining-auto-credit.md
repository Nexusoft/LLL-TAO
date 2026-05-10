# Mining Reward Auto-Credit (Both Lanes)

This document describes the optional **direct auto-credit** path for mining
rewards used by the Stateless mining lane (port 9323) and the Legacy mining
lane (port 8323), and the shared on-the-wire layout that both lanes use.

The pre-existing event-only behaviour is unchanged: legacy miners that send a
bare 32-byte reward payload continue to receive their reward as a coinbase
event that they must claim via the standard credit/claim flow.

---

## 1. Goal

Allow a miner to opt-in to **one-shot direct credit** of the block reward into
a specific account register (e.g. its `:default` NXS account) instead of
receiving an event that it must later claim.

The original ASCII sketch from the design discussion was:

> 1. At `MINER_SET_REWARD`, the miner sends both `(genesis, account-name string)`.
> 2. Node resolves the name index `genesis + "default"` → register address at
>    AUTH time and caches it on the session.
> 3. `OP::COINBASE` payload is split into two fields on the wire:
>    `recipient_genesis` (for `Verify`/events) + `recipient_account`
>    (for `Commit`'s direct credit).
> 4. `Commit` uses `recipient_account` for `ReadState`/`WriteState` and
>    `recipient_genesis` for `WriteEvent` — both succeed.

This is implemented in both lanes with a single shared parser and a single
shared `OP::COINBASE` extension.

---

## 2. Shared Wire Format

Both lanes route `MINER_SET_REWARD` through the same handler
(`StatelessMiner::ProcessSetReward`).  The decrypted ChaCha20-Poly1305
plaintext is parsed by the shared helper
[`LLP::RewardBindingPayload::ParsePayload`][rbp].

```
MINER_SET_REWARD plaintext  (after AEAD decryption)
┌───────────────────────────────────────────────────────────────────────────┐
│ byte[0..31]      recipient sigchain genesis hash (display order, BE)      │
│ byte[32]         (optional) account-name length N                          │
│ byte[33..32+N]   (optional) UTF-8 account-name string                      │
└───────────────────────────────────────────────────────────────────────────┘

  Legacy 32-byte form    → event-only (unchanged behaviour)
  Extended 33+N byte form → auto-credit: node resolves <genesis>:<name>
                            and binds the resulting register address.
```

[rbp]: ../src/LLP/include/reward_binding_payload.h

The matching extension in `OP::COINBASE` carries the resolved account address
on-chain so peers can replay deterministically:

```
OP::COINBASE contract  (extended layout)
┌────┬───────────────┬─────────────────┬──────────┬──────────────┐
│ OP │ recipient_gen │ recipient_acct  │ amount   │ extra_nonce  │
│ 1B │     32B       │      32B *      │   8B     │     8B       │
└────┴───────────────┴─────────────────┴──────────┴──────────────┘
                       * field is omitted in the legacy 65-byte layout

  Legacy producer:    OP | gen | amount | extra_nonce        =  49B
  Auto-credit producer: OP | gen | acct | amount | extra_nonce =  81B
```

`Coinbase::Verify()` accepts both lengths; `Coinbase::Commit()` writes the
event under `recipient_genesis` and, when `recipient_account != 0`, performs
an additional ReadState/WriteState credit against `recipient_account`.

---

## 3. End-to-end Flow

```mermaid
sequenceDiagram
    autonumber
    participant Miner as NexusMiner
    participant Lane as Mining Lane (8323 or 9323)
    participant Reward as StatelessMiner::ProcessSetReward
    participant Names as Names Index (LLD)
    participant Create as TAO::Ledger::CreateBlock
    participant Coinbase as TAO::Operation::Coinbase
    participant State as Register State

    Miner->>Lane: MINER_SET_REWARD<br/>{genesis ‖ name?}
    Lane->>Reward: decrypt + dispatch
    Note over Reward: RewardBindingPayload::ParsePayload<br/>(shared by both lanes)
    Reward->>Reward: ValidateRewardAddress(genesis)
    alt account-name provided
        Reward->>Names: ResolveRewardAccount(genesis, "default")
        Names-->>Reward: account register address
        Reward->>Reward: cache (hashRewardAddress,<br/>hashRewardAccount) on session
    else legacy 32-byte payload
        Reward->>Reward: cache hashRewardAddress only<br/>(event-only mode)
    end
    Reward-->>Lane: REWARD_RESULT (encrypted ok/fail)
    Lane-->>Miner: REWARD_RESULT

    Miner->>Lane: GET_BLOCK
    Lane->>Create: CreateBlockForStatelessMining(<br/>channel, nonce,<br/>hashRewardAddress,<br/>hashRewardAccount)
    Create->>Coinbase: build OP::COINBASE with optional<br/>recipient_account field
    Create-->>Lane: TritiumBlock template
    Lane-->>Miner: BLOCK_DATA

    Miner-->>Lane: SUBMIT_BLOCK
    Lane->>Coinbase: Commit(contract)
    Coinbase->>State: WriteEvent(recipient_genesis)
    alt recipient_account != 0
        Coinbase->>State: ReadState(recipient_account)
        Coinbase->>State: WriteState(recipient_account, +amount)
    end
```

---

## 4. Component Map

```
                      ┌────────────────────────────────────────────┐
                      │   src/LLP/include/reward_binding_payload.h │
                      │   LLP::RewardBindingPayload::ParsePayload  │
                      └─────────────────┬──────────────────────────┘
                                        │ shared header
            ┌───────────────────────────┴───────────────────────────┐
            ▼                                                       ▼
┌───────────────────────────┐                       ┌───────────────────────────┐
│  Stateless Lane (9323)    │                       │  Legacy Lane (8323)       │
│  src/LLP/                 │                       │  src/LLP/                 │
│   stateless_miner.cpp     │                       │   miner.cpp               │
│                           │                       │                           │
│   ProcessSetReward()  ────┼─────────┬─────────────┼─→ LegacyLaneHandler:     │
│                           │         │             │      SetRewardHandler →  │
│                           │         │             │      ProcessSetReward()  │
│                           │         │             │                           │
│   new_block() ────────────┼──┐      │      ┌──────┼─── new_block() ──────┐   │
└───────────────────────────┘  │      │      │      └─────────────────────────┘ │
                               │      │      │                                  │
                               ▼      ▼      ▼                                  │
                      ┌──────────────────────────────────┐                      │
                      │ TAO::Ledger::                    │                      │
                      │   CreateBlockForStatelessMining( │                      │
                      │     channel, extraNonce,         │                      │
                      │     hashRewardAddress,           │                      │
                      │     hashRewardAccount )          │                      │
                      └──────────────────────────────────┘                      │
                                       │                                        │
                                       ▼                                        │
                      ┌──────────────────────────────────┐                      │
                      │ TAO::Operation::Coinbase         │                      │
                      │   - extended OP::COINBASE        │                      │
                      │     carries recipient_account    │                      │
                      │   - Verify accepts both layouts  │                      │
                      │   - Commit performs WriteEvent + │                      │
                      │     optional direct credit       │                      │
                      └──────────────────────────────────┘
```

---

## 5. Backwards Compatibility

| Miner version           | `MINER_SET_REWARD` payload | Coinbase contract | Reward delivery |
| ----------------------- | -------------------------- | ----------------- | --------------- |
| Pre-auto-credit         | 32 bytes                   | 49 bytes (legacy) | event → claim   |
| Auto-credit capable     | 33 + N bytes               | 81 bytes (extended) | direct credit + event |

* Both wire forms are accepted by both lanes.
* Existing `Verify` / `Commit` / `Rollback` / `unpack` paths in
  `src/TAO/Operation/coinbase.cpp`, `src/TAO/Operation/execute.cpp`,
  `src/TAO/Register/{rollback,unpack,verify,build}.cpp` accept both
  contract sizes.
* Cache invalidation in `TAO::Ledger::CachedMiningTemplateRequiresProducerFinalization`
  treats different `hashRewardAccount` values as a producer mismatch so
  cached producers cannot leak across miners that requested different
  auto-credit accounts.

---

## 6. Operator Configuration

* `-rewardmustexist=1` (default) — node refuses to bind a reward genesis
  that does not exist on chain, preventing the silent "BLOCK_ACCEPTED with
  no NXS" failure mode.
* `-rewardmustexist=0` — opt-out for the narrow case of mining a brand-new
  sigchain's very first block, where `LLD::Ledger->HasFirst(genesis)` is
  still false.

---

## 7. Testing

The shared parser is unit tested by
[`tests/unit/LLP/reward_binding_payload_test.cpp`][rbpt] under the
`[reward_binding][shared]` Catch2 tags, covering:

* legacy 32-byte payload acceptance,
* extended payload acceptance and account-name extraction,
* short / mismatched / zero-length rejection paths,
* `ResultString` coverage for diagnostic logging.

Lane-level coverage continues to live in
`tests/unit/LLP/reward_routing.cpp` and
`tests/unit/LLP/reward_address_protocol.cpp`.

[rbpt]: ../tests/unit/LLP/reward_binding_payload_test.cpp
