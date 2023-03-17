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

## Developing

Developing on Nexus has been designed to be powerful, yet simple to use. Tritium++ packages features from SQL Queries, filters, sorting, statistical operators, functions, variables, and much more. The API uses a RESTFul HTTP-JSON protocol, so that you can access the power of Smart Contracts on Nexus from very basic web experience to advanced capabilities. The API is always expanding, so if you find any bugs or wish to suggest improvements, please use the Issues tracker and submit your feedback.

The developer documentation can be found here: https://wiki.nexus.io/en/tritium++

## License

Nexus is released under the terms of the MIT license. See [COPYING](COPYING.MD) for more
information or see https://opensource.org/licenses/MIT.

## Contributing

If you would like to contribute as always submit a pull request. All code contributions should follow the comments and style guides. See [COMMENTS](contrib/COMMENTS.md) or [STYLEGUIDE](contrib/STYLEGUIDE.md) for more information.
