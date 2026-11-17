# State Machine Diagrams

## Simple State Machine (85 chars)
```
[Idle] → [Validating] → [Processing]
   ↑          ↓              ↓
   └────────[Error]←────────┘
```

## Connection Lifecycle (120 chars)
```
[Disconnected] → [Connecting] → [Connected]
       ↑              ↓              ↓
       └───────[Timeout]      [Active/Idle]
                                     ↓
                               [Disconnecting]
```

## Transaction States (140 chars)
```
[Created] → [Validated] → [Pending] → [Confirmed]
                ↓            ↓            ↓
             [Invalid]   [Rejected]   [Complete]
                ↓            ↓
              [Failed] ←─────┘
```

## Session States (110 chars)
```
[New] → [Authenticated] → [Active]
           ↓                  ↓
        [Rejected]        [Expired]
                             ↓
                         [Cleanup]
```
