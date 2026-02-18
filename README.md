# Nexus

Nexus is a high performance Application Framework that provides a Peer to Peer electronic cash system with support for real-time smart contracts powered with a 64-bit register virtual machine. Nexus technology is developed to improve the overall security, scalability, and usability of Internet driven Applications.

## Branching

We use a very strict branching logic for our development. The branch 'merging' is the main development branch, which contains the most up-to-date code. The branch 'master' is the stable branch, that contains releases. If you are compiling from source, ensure you use the 'master' branch or pull from a release tag. The branch 'staging' is for pre-releases, so if you would like to test out new features before full release, but want to ensure they are mostly stable, use 'staging'.

## Building

We use Make to build our project to multiple platforms. Please read out build documentation for instructions and options.

[Build Options](docs/build-params-reference.md)

[Linux](docs/build-linux.md)

[Windows](docs/build-win.md)

[OSX](docs/build-osx.md)

[iPhone OS / Android OS](docs/build-mobile.md)

## Mining

Nexus supports both private solo mining and decentralized public mining pools using the enhanced Falcon Handshake protocol.

### Falcon Handshake & Stateless Mining

The LLL-TAO node implements a secure, post-quantum Falcon handshake for miner authentication with the following features:

#### Security Features
- **Post-Quantum Signatures**: Utilizes Falcon-512/1024 signature schemes for quantum-resistant authentication
- **ChaCha20 Encryption**: Optional ChaCha20 encryption for Falcon public key exchange (mandatory for public nodes, optional for localhost)
- **Session Key Generation**: Dynamic session keys tied to Tritium GenesisHash ownership
- **Replay Protection**: Timestamp-based validation prevents replay attacks

#### Node Cache & DDOS Protection
- **Cache Limit**: Hardcoded 500 entry maximum for DDOS protection
- **Keep-Alive Ping**: Miners must ping every 24 hours (or more frequently)
- **Purge Routine**: Inactive miners removed after 7 days (remote) or 30 days (localhost)
- **Localhost Exception**: Extended cache persistence for local solo miners

#### Mining Modes

**Private Solo Mining (Localhost)**
- Simplified validation with extended cache timeout
- ChaCha20 encryption optional (plaintext allowed)
- Direct reward payout to configured Tritium account

**Public Pool Mining (Internet)**
- Mandatory TLS 1.3 with ChaCha20-Poly1305-SHA256 cipher suite
- Required Falcon public key encryption during handshake
- Rewards tied to Tritium GenesisHash for stateful validation
- Session-based authentication with automatic purging

#### Quick Start: Node Pool Server `nexus.conf`

The minimum required settings to run a Nexus node as a mining pool server (`~/.Nexus/nexus.conf`):

```conf
# --- Autologin (required for unattended node operation) ---
autologin=1
username=YOUR_USERNAME
password=YOUR_PASSWORD
pin=YOUR_PIN

# --- API Authentication ---
apiuser=YOUR_API_USERNAME
apipassword=YOUR_STRONG_API_PASSWORD

# --- Servers ---
server=1

# --- Enable Mining (starts BOTH servers simultaneously) ---
mining=1
miningport=9323          # Stateless mining server  (PORT 9323)
legacyminingport=8323    # Legacy mining server     (PORT 8323, auto-starts)

# --- Network ---
listen=1
maxconnections=99
llpallowip=127.0.0.1    # Localhost testing
#llpallowip=0.0.0.0     # Public internet pool

# --- Post-Quantum Authentication ---
falcon=1
```

> **Localhost Testing:** Use `llpallowip=127.0.0.1` — miners on the same machine connect to `127.0.0.1:9323`  
> **Public Internet:** Switch to `llpallowip=0.0.0.0` — miners point to your public IP on port `9323`  
> **Security:** `chmod 600 ~/.Nexus/nexus.conf` to protect your credentials

The node runs **two mining servers simultaneously**:
| Server | Port | Protocol | Miners |
|--------|------|----------|--------|
| Stateless | `9323` | 16-bit framing (Phase 2) | NexusMiner (stateless config) |
| Legacy | `8323` | 8-bit framing (Phase 1) | Backward-compatible miners |

For a complete reference see [nexus.conf Reference](docs/reference/nexus.conf.md).

## Developing

Developing on Nexus has been designed to be powerful, yet simple to use. Tritium++ packages features from SQL Queries, filters, sorting, statistical operators, functions, variables, and much more. The API uses a RESTFul HTTP-JSON protocol, so that you can access the power of Smart Contracts on Nexus from very basic web experience to advanced capabilities. The API is always expanding, so if you find any bugs or wish to suggest improvements, please use the Issues tracker and submit your feedback.

## License

Nexus is released under the terms of the MIT license. See [COPYING](COPYING.MD) for more
information or see https://opensource.org/licenses/MIT.

## Documentation & Diagrams

Architecture diagrams, protocol visualizations, and onboarding guides:

- [Diagram Templates & Architecture Diagrams](docs/diagrams/README.md) - 15+ Mermaid and ASCII diagrams
- [AI-Assisted Developer Onboarding](docs/onboarding/ai-assisted-onboarding.md) - Learning pathways with AI guidance
- [AI-Human Advancement Thesis](docs/philosophy/ai-human-advancement.md) - Collaboration philosophy
- [Mining Debug Cheat Sheet](docs/onboarding/cheat-sheets/mining-debug.md)
- [Consensus Debug Cheat Sheet](docs/onboarding/cheat-sheets/consensus-debug.md)
- [Performance Tuning](docs/onboarding/cheat-sheets/performance-tuning.md)

## Contributing

If you would like to contribute as always submit a pull request. All code contributions should follow the comments and style guides. See [COMMENTS](contrib/COMMENTS.md) or [STYLEGUIDE](contrib/STYLEGUIDE.md) for more information.

### Working with GitHub Copilot Coding Agent

See [Coding Agent Best Practices](docs/CODING_AGENT_BEST_PRACTICES.md) for guidelines on creating effective PR descriptions that work within the 3,000 character limit.

Quick reference: [Cheat Sheet](docs/CODING_AGENT_CHEAT_SHEET.md)
