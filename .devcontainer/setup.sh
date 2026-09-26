#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
if [ "${EUID:-$(id -u)}" -eq 0 ]; then
    SUDO=""
elif command -v sudo >/dev/null 2>&1; then
    SUDO="sudo"
else
    echo "Error: root or sudo is required to install build dependencies." >&2
    exit 1
fi

echo "================================================"
echo "Setting up Nexus LLL-TAO Development Environment"
echo "================================================"

echo "Installing shared build dependencies..."
$SUDO bash "${REPO_ROOT}/contrib/devtools/install-build-deps.sh"

echo "Verifying Berkeley DB headers..."
TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT
cat >"$TMPDIR/dbcheck.cpp" <<'EOF'
#include <db_cxx.h>
int main() { return 0; }
EOF
g++ -std=c++20 -c "$TMPDIR/dbcheck.cpp" -o "$TMPDIR/dbcheck.o"

# Clean up apt cache to reduce image size
echo "Cleaning up..."
$SUDO apt-get clean
$SUDO rm -rf /var/lib/apt/lists/*

# Set proper permissions for workspace
if [ -d /workspace ] && id -u vscode >/dev/null 2>&1; then
    $SUDO chown -R vscode:vscode /workspace
fi

echo ""
echo "================================================"
echo "Environment Setup Complete!"
echo "================================================"
echo ""
echo "To build the project:"
echo "  cd /workspace"
echo "  make -f makefile.cli clean"
echo "  make -f makefile.cli -j\$(nproc)"
echo ""
echo "To run tests:"
echo "  make -f makefile.cli UNIT_TESTS=1 -j\$(nproc)"
echo "  ./nexus \"[llp]\""
echo ""
echo "Nexus ports forwarded:"
echo "  8323 - Tritium Protocol (Legacy Mining)"
echo "  9323 - Tritium Protocol (Stateless Mining)"
echo "  9325 - Testnet"
echo "  9336 - API Server"
echo ""
