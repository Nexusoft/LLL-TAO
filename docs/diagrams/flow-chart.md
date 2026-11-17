# Flow Chart Diagrams

## Simple Linear Flow (65 chars)
```
Client → API → Validate → DB
          ↓
       Error Log
```

## Decision Flow (120 chars)
```
[Start] → [Check Valid?]
            ├─ Yes → [Process] → [End]
            └─ No  → [Error] → [Log]
```

## Parallel Processing (150 chars)
```
[Input]
   ├──→ [Worker 1] ──┐
   ├──→ [Worker 2] ──┤
   └──→ [Worker 3] ──┴→ [Aggregate] → [Output]
```

## Conditional Branches (180 chars)
```
[Request]
    ↓
[Validate]
    ├─→ [Valid] → [Process] → [Success]
    ├─→ [Invalid] → [Reject] → [Error]
    └─→ [Timeout] → [Retry] → [Queue]
```

## Loop Flow (100 chars)
```
[Start] → [Process] → [Check Done?]
             ↑            ├─ No ──┘
             └─ Yes ──→ [End]
```

## Error Handling (140 chars)
```
[Try Execute]
    ├─→ [Success] → [Return]
    └─→ [Catch Error]
           ├─→ [Log]
           ├─→ [Notify]
           └─→ [Rollback]
```
