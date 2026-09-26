# Data Pipeline Diagrams

## Simple Pipeline (80 chars)
```
[Input] → [Transform] → [Filter] → [Validate] → [Output]
```

## Multi-Stage Processing (120 chars)
```
[Raw Data] → [Parse] → [Normalize] → [Validate]
                ↓          ↓            ↓
            [Errors]   [Cache]      [Database]
```

## Fan-Out Pattern (140 chars)
```
[Source]
   ├──→ [Processor A] → [Sink A]
   ├──→ [Processor B] → [Sink B]
   └──→ [Processor C] → [Sink C]
```

## ETL Pipeline (160 chars)
```
[Extract]
   ↓
[Transform]
   ├─→ [Clean]
   ├─→ [Enrich]
   └─→ [Aggregate]
        ↓
     [Load]
        ├─→ [Primary DB]
        └─→ [Archive]
```
