#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

TARGET_TRIPLET="${TARGET_TRIPLET:-x86_64-w64-mingw32}"
CXX="${CXX:-${TARGET_TRIPLET}-g++}"
DEPS_PREFIX="${DEPS_PREFIX:-}"

if ! command -v "${CXX}" >/dev/null 2>&1; then
    echo "MinGW compiler not found: ${CXX}" >&2
    echo "Set CXX or TARGET_TRIPLET to a Linux-hosted compiler, e.g. x86_64-w64-mingw32-g++ for MinGW-w64 or x86_64-w64-mingw32.static-g++ for MXE." >&2
    exit 1
fi

if [ -z "${DEPS_PREFIX}" ]; then
    cat >&2 <<EOF
DEPS_PREFIX is required.

Point it at the Windows-target dependency prefix that contains include/ and lib/.
For MXE this is usually similar to:
  DEPS_PREFIX=/opt/mxe/usr/x86_64-w64-mingw32.static

Static MXE triplets are preferred for release candidates. If you use a dynamic
triplet, package the required runtime DLLs with package-windows-node-release.sh.
EOF
    exit 1
fi

OPENSSL_PREFIX="${OPENSSL_PREFIX:-${DEPS_PREFIX}}"
BDB_PREFIX="${BDB_PREFIX:-${DEPS_PREFIX}}"
MINIUPNPC_PREFIX="${MINIUPNPC_PREFIX:-${DEPS_PREFIX}}"

MAKE_ARGS=(
    -f makefile.cli
    OS=Windows_NT
    CXX="${CXX}"
    OPENSSL_LIB_PATH="${OPENSSL_PREFIX}/lib"
    OPENSSL_INCLUDE_PATH="${OPENSSL_PREFIX}/include"
    BDB_LIB_PATH="${BDB_PREFIX}/lib"
    BDB_INCLUDE_PATH="${BDB_PREFIX}/include"
    MINIUPNPC_LIB_PATH="${MINIUPNPC_PREFIX}/lib"
    MINIUPNPC_INCLUDE_PATH="${MINIUPNPC_PREFIX}/include"
)

cd "${REPO_ROOT}"

if [ "${CLEAN:-1}" = "1" ]; then
    make "${MAKE_ARGS[@]}" clean || true
fi

make "${MAKE_ARGS[@]}" "$@"

if [ "${PACKAGE:-0}" = "1" ]; then
    "${REPO_ROOT}/scripts/package-windows-node-release.sh" "${REPO_ROOT}/release/nexus.exe"
fi
