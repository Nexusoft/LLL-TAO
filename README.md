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

#### Configuration

Mining can be configured through the node's API or configuration file:

```bash
# Enable mining pool mode
-miningpool=1

# Set Falcon handshake timeout (default: 60 seconds)
-falconhandshake.timeout=60

# Require encryption for all miners (default: false for localhost, true for remote)
-falconhandshake.requireencryption=1

# Set cache purge interval (default: 7 days)
-nodecache.purgetimeout=604800
```

For detailed mining setup instructions, see [Mining Documentation](docs/mining.md).

## Developing

Developing on Nexus has been designed to be powerful, yet simple to use. Tritium++ packages features from SQL Queries, filters, sorting, statistical operators, functions, variables, and much more. The API uses a RESTFul HTTP-JSON protocol, so that you can access the power of Smart Contracts on Nexus from very basic web experience to advanced capabilities. The API is always expanding, so if you find any bugs or wish to suggest improvements, please use the Issues tracker and submit your feedback.

## License

Nexus is released under the terms of the MIT license. See [COPYING](COPYING.MD) for more
information or see https://opensource.org/licenses/MIT.

## Contributing

If you would like to contribute as always submit a pull request. All code contributions should follow the comments and style guides. See [COMMENTS](contrib/COMMENTS.md) or [STYLEGUIDE](contrib/STYLEGUIDE.md) for more information.
