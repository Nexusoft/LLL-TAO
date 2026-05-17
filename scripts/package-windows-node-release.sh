#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

EXE_PATH="${1:-${REPO_ROOT}/release/nexus.exe}"
DIST_DIR="${DIST_DIR:-${REPO_ROOT}/dist}"
VERSION="${VERSION:-}"

if [ ! -f "${EXE_PATH}" ]; then
    echo "nexus.exe not found at ${EXE_PATH}" >&2
    exit 1
fi

if [ -z "${VERSION}" ]; then
    VERSION="$(git -C "${REPO_ROOT}" describe --tags --always --dirty 2>/dev/null || date -u +%Y%m%d%H%M%S)"
fi

SAFE_VERSION="$(printf '%s' "${VERSION}" | tr -c 'A-Za-z0-9._-' '-')"
PACKAGE_NAME="${PACKAGE_NAME:-nexus-node-windows-x86_64-${SAFE_VERSION}}"
STAGE_DIR="${DIST_DIR}/${PACKAGE_NAME}"
ZIP_NAME="${PACKAGE_NAME}.zip"

rm -rf "${STAGE_DIR}"
mkdir -p "${STAGE_DIR}" "${DIST_DIR}"

cp "${EXE_PATH}" "${STAGE_DIR}/nexus.exe"
cp "${REPO_ROOT}/config/nexus.conf.windows-localhost-mining" "${STAGE_DIR}/nexus.conf.example"
cp "${REPO_ROOT}/docs/release/windows-node/README.md" "${STAGE_DIR}/README.md"

copy_runtime_dlls() {
    if [ -n "${DYNAMIC_DLL_DIR:-}" ] && [ -d "${DYNAMIC_DLL_DIR}" ]; then
        find "${DYNAMIC_DLL_DIR}" -maxdepth 1 -type f -name '*.dll' -exec cp -n {} "${STAGE_DIR}/" \;
    fi

    if ! command -v ldd >/dev/null 2>&1; then
        return
    fi

    while IFS= read -r dll; do
        dll="${dll//$'\\'//}"
        case "${dll}" in
            */mingw64/bin/*.dll|/mingw64/bin/*.dll|*/ucrt64/bin/*.dll|/ucrt64/bin/*.dll|*/clang64/bin/*.dll|/clang64/bin/*.dll)
                cp -n "${dll}" "${STAGE_DIR}/"
                ;;
        esac
    done < <(ldd "${EXE_PATH}" 2>/dev/null | awk '
        /=>/ && $3 ~ /\.dll$/ { print $3 }
        /^[[:space:]]*\/.*\.dll/ { print $1 }
    ' | sort -u)
}

copy_runtime_dlls

(
    cd "${STAGE_DIR}"
    find . -type f ! -name SHA256SUMS.txt -print0 \
        | sort -z \
        | xargs -0 sha256sum > SHA256SUMS.txt
)

(
    cd "${DIST_DIR}"
    rm -f "${ZIP_NAME}" "${ZIP_NAME}.sha256"
    zip -r "${ZIP_NAME}" "${PACKAGE_NAME}"
    sha256sum "${ZIP_NAME}" > "${ZIP_NAME}.sha256"
)

echo "Created ${DIST_DIR}/${ZIP_NAME}"
echo "Created ${DIST_DIR}/${ZIP_NAME}.sha256"
