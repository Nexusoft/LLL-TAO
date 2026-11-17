#!/usr/bin/env bash
set -euo pipefail

export DEBIAN_FRONTEND="${DEBIAN_FRONTEND:-noninteractive}"

log() {
    echo "[install-build-deps] $*"
}

ensure_apt_exists() {
    if ! command -v apt-get >/dev/null 2>&1; then
        echo "This dependency bootstrap currently supports apt-based environments only." >&2
        exit 1
    fi
}

package_exists() {
    apt-cache show "$1" >/dev/null 2>&1
}

verify_db_headers() {
    local tmp_src tmp_obj tmp_err
    tmp_src="$(mktemp /tmp/db-header-check-XXXXXX.cpp)"
    tmp_obj="$(mktemp /tmp/db-header-check-XXXXXX.o)"
    tmp_err="$(mktemp /tmp/db-header-check-XXXXXX.log)"
    trap 'rm -f "${tmp_src:-}" "${tmp_obj:-}" "${tmp_err:-}"' RETURN

    cat >"$tmp_src" <<'EOF'
#include <db_cxx.h>
int main()
{
    DbEnv env(0);
    return 0;
}
EOF

    if g++ -std=c++20 -c "$tmp_src" -o "$tmp_obj" >/dev/null 2>"$tmp_err"; then
        return 0
    fi

    cat "$tmp_err" >&2
    return 1
}

main() {
    ensure_apt_exists

    log "Updating package lists"
    apt-get update

    local -a db_packages
    if package_exists libdb5.3++-dev && package_exists libdb5.3-dev; then
        db_packages=(libdb5.3-dev libdb5.3++-dev)
    else
        db_packages=(libdb-dev libdb++-dev)
    fi

    log "Installing build dependencies"
    apt-get install -y --no-install-recommends \
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
        gdb \
        "${db_packages[@]}" \
        libssl-dev \
        libboost-all-dev \
        libboost-system-dev \
        libboost-filesystem-dev \
        libboost-thread-dev \
        libboost-program-options-dev \
        miniupnpc \
        libminiupnpc-dev \
        libevent-dev \
        libgmp-dev \
        libsodium-dev \
        zlib1g-dev

    log "Verifying Berkeley DB C++ headers"
    if ! verify_db_headers; then
        echo "Failed to compile a minimal translation unit including <db_cxx.h>." >&2
        echo "Installed Berkeley DB packages:" >&2
        dpkg-query -W "${db_packages[@]}" >&2 || true
        echo "Known db_cxx.h candidates:" >&2
        dpkg -L "${db_packages[@]}" 2>/dev/null | grep '/db_cxx\.h$' >&2 || true
        exit 1
    fi

    log "Dependency bootstrap complete"
}

main "$@"
