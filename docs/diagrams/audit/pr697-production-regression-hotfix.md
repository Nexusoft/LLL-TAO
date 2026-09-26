# PR #697 Production Regression Hotfix

This diagram captures the patched recovery path for the production orphan/BESTCHAIN
doom-loop class observed after PR #697.

## Extracted incomplete orphan + no-progress backoff

```mermaid
flowchart TD
    BEST["BESTCHAIN foreign tip"] --> WALK["AttemptPeerBestChainRecovery walkback"]
    WALK --> EXTRACT["Extract connectable orphan from pool"]
    EXTRACT --> PROC["Process(extracted orphan)"]
    PROC -->|ACCEPTED| DRAIN["BFS orphan drain advances chain"]
    PROC -->|INCOMPLETE| MISS["Immediate GET BLOCK+TRANSACTIONS for hashMissing"]
    MISS --> RETAIN["Retain incomplete orphan for redelivery"]
    RETAIN --> RETRY["Next BESTCHAIN arrival"]
    RETRY --> PROGRESS{"Local best advanced?"}
    PROGRESS -->|Yes| RECOVER["Allow branch LIST recovery"]
    PROGRESS -->|No| BACKOFF["No-progress backoff suppresses repeated LIST churn"]
    RECOVER --> DRAIN
```

### Operational effect

- Incomplete extracted connectable orphans now trigger immediate missing-transaction
  recovery instead of waiting for unrelated redelivery.
- Repeated far-tip recovery LIST traffic is suppressed when the local best hash/height
  have not advanced between attempts, reducing reconnect/orphan-limit loop pressure.
