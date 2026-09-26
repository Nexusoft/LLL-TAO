# Peer Reputation & Trust Network Topology

Diagrams showing how the Nexus peer trust system evaluates and ranks network peers.

---

## Trust Score Calculation

```mermaid
flowchart TD
    subgraph "Positive Factors"
        CON[Connected Count<br/>Weight: 100.0]
        SES[Session Count<br/>Weight: 80.0]
        LAT[Low Latency<br/>Weight: 10.0]
    end

    subgraph "Negative Factors"
        FAI[Failed Connections<br/>Weight: -20.0]
        FLS[Connection Fails<br/>Weight: -10.0]
        DRP[Dropped Connections<br/>Weight: -5.0]
    end

    CON --> CALC[Trust Score<br/>Calculation]
    SES --> CALC
    LAT --> CALC
    FAI --> CALC
    FLS --> CALC
    DRP --> CALC

    CALC --> SCORE[Final Trust Score]
    SCORE --> HIGH{Score Level}
    HIGH -->|High| PREF[Preferred Peer<br/>Priority connections]
    HIGH -->|Medium| NORM[Normal Peer<br/>Standard routing]
    HIGH -->|Low| DEPRI[Deprioritized<br/>Last resort]
    HIGH -->|Negative| BAN[Banned<br/>Connection refused]
```

---

## Trust Score Formula

```
Score = (100.0 × min(connected, max_connected))
      + ( 80.0 × session_count)
      + ( 10.0 × (max_latency - actual_latency))
      - ( 20.0 × min(failed, max_failed))
      - ( 10.0 × min(fails, max_fails))
      - (  5.0 × min(dropped, max_dropped))
```

---

## Peer Discovery & Connection Lifecycle

```mermaid
sequenceDiagram
    participant N as New Peer
    participant L as Local Node
    participant DB as TrustDB

    N->>L: Connection Request
    L->>DB: Lookup Trust Score
    DB->>L: Score (or new entry)
    alt Score >= 0
        L->>N: Accept Connection
        L->>DB: Increment session_count
        Note over L,N: Active session
        L->>DB: Update latency, connected
    else Score < 0
        L->>N: Reject Connection
        L->>DB: Increment failed
    end
    Note over N,L: On disconnect
    L->>DB: Update final metrics
```

---

## Network Topology View

```mermaid
flowchart TD
    subgraph "Trusted Peers (High Score)"
        T1[Peer A<br/>Score: 950]
        T2[Peer B<br/>Score: 870]
    end

    subgraph "Normal Peers (Medium Score)"
        N1[Peer C<br/>Score: 450]
        N2[Peer D<br/>Score: 320]
        N3[Peer E<br/>Score: 280]
    end

    subgraph "New/Unknown Peers"
        U1[Peer F<br/>Score: 0]
        U2[Peer G<br/>Score: 0]
    end

    LOCAL[Local Node] <-->|Priority| T1
    LOCAL <-->|Priority| T2
    LOCAL <-->|Standard| N1
    LOCAL <-->|Standard| N2
    LOCAL <-->|Standard| N3
    LOCAL <-->|Probation| U1
    LOCAL <-->|Probation| U2

    T1 <-->|Relay| T2
    N1 <-->|Relay| N2
```

---

## Cross-References

- [Consensus Validation](consensus-validation-flow.md)
- [Architecture Boxes](../architecture-boxes.md)
- Source: `src/LLP/trust_address.cpp`
