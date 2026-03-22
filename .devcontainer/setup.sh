#!/bin/bash
set -e

echo "================================================"
echo "Setting up Nexus LLL-TAO Development Environment"
echo "================================================"

# Prevent interactive prompts during package installation
export DEBIAN_FRONTEND=noninteractive

# Update package lists
echo "Updating package lists..."
apt-get update

# Install essential build tools
echo "Installing build essentials..."
apt-get install -y \
    build-essential \
    gcc \
    g++ \
    make \
    cmake \
    autoconf \
    automake \
    libtool \
    pkg-config \
    git \
    wget \
    curl \
    vim \
    gdb

# Install Berkeley DB 5.3 (required for blockchain database)
echo "Installing Berkeley DB 5.3..."
apt-get install -y libdb5.3-dev libdb5.3++-dev

# Install OpenSSL (required for cryptographic operations)
echo "Installing OpenSSL..."
apt-get install -y libssl-dev

# Install Boost libraries (required for various utilities)
echo "Installing Boost libraries..."
apt-get install -y \
    libboost-all-dev \
    libboost-system-dev \
    libboost-filesystem-dev \
    libboost-thread-dev \
    libboost-program-options-dev

# Install MiniUPnP (required for NAT traversal)
echo "Installing MiniUPnP..."
apt-get install -y miniupnpc libminiupnpc-dev

# Install libevent (required for async I/O)
echo "Installing libevent..."
apt-get install -y libevent-dev

# Install additional utilities
echo "Installing additional utilities..."
apt-get install -y \
    libgmp-dev \
    libsodium-dev \
    zlib1g-dev

# Clean up apt cache to reduce image size
echo "Cleaning up..."
apt-get clean
rm -rf /var/lib/apt/lists/*

# Set proper permissions for workspace
chown -R vscode:vscode /workspace || true

echo ""
echo "================================================"
echo "Environment Setup Complete!"
echo "================================================"
echo ""
echo "To build the project:"
echo "  cd /workspace"
echo "  make clean"
echo "  make -j\$(nproc)"
echo ""
echo "To run tests:"
echo "  make test"
echo ""
echo "Nexus ports forwarded:"
echo "  8323 - Tritium Protocol (Legacy Mining)"
echo "  9323 - Tritium Protocol (Stateless Mining)"
echo "  9325 - Testnet"
echo "  9336 - API Server"
echo ""
