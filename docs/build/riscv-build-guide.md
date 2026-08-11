# RISC-V Build Guide

This document is the practical build guide for compiling the Nexus LLL-TAO node on RISC-V 64-bit systems or cross-compiling it for RISC-V from another Linux host.

For architecture rationale and implementation notes, see [`../architecture/riscv-design.md`](../architecture/riscv-design.md). For runtime diagnostics, see [`../current/node/riscv/diagnostics.md`](../current/node/riscv/diagnostics.md).

## Supported target

The supported baseline target is **RV64GC**:

- `rv64i`: 64-bit base integer ISA
- `m`: integer multiply/divide
- `a`: atomic instructions
- `f`/`d`: floating-point extensions
- `c`: compressed instructions

The build system enables this baseline with:

```sh
make -f makefile.cli RISCV64=1
```

Optional RISC-V Vector (RVV) builds are available for hardware that supports vector instructions:

```sh
make -f makefile.cli RISCV64=1 RISCV_VECTOR=1
```

Do not use `RISCV_VECTOR=1` on boards that do not support RVV; the resulting binary may fail with an illegal-instruction error.

## Dependencies

### Native RISC-V build host

On a RISC-V Debian/Ubuntu system:

```sh
sudo apt-get update
sudo apt-get install -y git make build-essential libssl-dev libdb5.3-dev libdb5.3++-dev libminiupnpc-dev libevent-dev
```

On Fedora RISC-V:

```sh
sudo dnf install git make gcc-c++ openssl-devel libdb-cxx-devel miniupnpc-devel libevent-devel
```

### x86_64 host cross-compiling for RISC-V

On a Debian/Ubuntu x86_64 host:

```sh
sudo dpkg --add-architecture riscv64
sudo apt-get update
sudo apt-get install -y git make build-essential gcc-riscv64-linux-gnu g++-riscv64-linux-gnu crossbuild-essential-riscv64
# Berkeley DB base + C++ wrapper are both needed for wallet-enabled builds.
sudo apt-get install -y libssl-dev:riscv64 libdb5.3-dev:riscv64 libdb5.3++-dev:riscv64 libminiupnpc-dev:riscv64 libevent-dev:riscv64
```

`libdb5.3-dev` provides the Berkeley DB base headers and libraries; `libdb5.3++-dev` provides the C++ wrapper headers used by wallet-enabled builds.

If your distribution does not provide all `:riscv64` development packages, use a RISC-V sysroot and pass explicit include/library paths as shown below.

## Native build on RISC-V hardware

```sh
git clone https://github.com/Nexusoft/LLL-TAO
cd LLL-TAO
make -f makefile.cli RISCV64=1 -j"$(nproc)"
```

To build without the legacy Berkeley DB wallet dependency:

```sh
make -f makefile.cli RISCV64=1 NO_WALLET=1 -j"$(nproc)"
```

To build for RVV-capable hardware:

```sh
make -f makefile.cli RISCV64=1 RISCV_VECTOR=1 -j"$(nproc)"
```

## Cross-compile from x86_64 Linux

Set the RISC-V compiler explicitly and pass the RISC-V library paths when they are not already the defaults on your host:

```sh
git clone https://github.com/Nexusoft/LLL-TAO
cd LLL-TAO

export CC=riscv64-linux-gnu-gcc
export CXX=riscv64-linux-gnu-g++

make -f makefile.cli RISCV64=1 \
    OPENSSL_LIB_PATH=/usr/lib/riscv64-linux-gnu \
    OPENSSL_INCLUDE_PATH=/usr/include/openssl \
    BDB_LIB_PATH=/usr/lib/riscv64-linux-gnu \
    -j"$(nproc)"
```

For a cross-compiled build without Berkeley DB wallet support:

```sh
make -f makefile.cli RISCV64=1 NO_WALLET=1 \
    OPENSSL_LIB_PATH=/usr/lib/riscv64-linux-gnu \
    OPENSSL_INCLUDE_PATH=/usr/include/openssl \
    -j"$(nproc)"
```

## QEMU smoke test

Install QEMU user-mode emulation on the build host:

```sh
sudo apt-get install -y qemu-user-static
```

After cross-compiling, run a basic binary smoke test:

```sh
qemu-riscv64-static -L /usr/riscv64-linux-gnu ./nexus --help
```

If your sysroot lives somewhere else, replace `/usr/riscv64-linux-gnu` with that sysroot path.

## Build options

| Make variable | Purpose |
|---|---|
| `RISCV64=1` | Selects the RISC-V 64-bit build path and enables the RV64GC baseline flags. |
| `RISCV_VECTOR=1` | Adds RVV build flags for vector-capable hardware. |
| `NO_WALLET=1` | Builds without the legacy Berkeley DB wallet dependency. |
| `OPENSSL_LIB_PATH=...` | Overrides the OpenSSL library directory. |
| `OPENSSL_INCLUDE_PATH=...` | Overrides the OpenSSL header directory. |
| `BDB_LIB_PATH=...` | Overrides the Berkeley DB library directory. |
| `BDB_INCLUDE_PATH=...` | Overrides the Berkeley DB header directory. |

The RISC-V defaults in `makefile.cli` use Debian/Ubuntu multiarch library paths such as `/usr/lib/riscv64-linux-gnu`.

## Verify the build

Check that the resulting binary is for RISC-V:

```sh
file ./nexus
```

Expected output should identify an ELF 64-bit RISC-V executable.

Run the built binary on native hardware:

```sh
./nexus --help
```

For unit-test builds, compile with the repository's existing test option and run the generated test binary according to the active test command for your branch.

## Troubleshooting

### `riscv64-linux-gnu-g++: command not found`

Install the cross compiler:

```sh
sudo apt-get install -y gcc-riscv64-linux-gnu g++-riscv64-linux-gnu
```

### `cannot find -lssl` or OpenSSL headers are missing

Install RISC-V OpenSSL development files or pass the sysroot paths explicitly:

```sh
sudo apt-get install -y libssl-dev:riscv64
make -f makefile.cli RISCV64=1 OPENSSL_LIB_PATH=/usr/lib/riscv64-linux-gnu OPENSSL_INCLUDE_PATH=/usr/include/openssl
```

### Berkeley DB is unavailable for RISC-V

Install the RISC-V Berkeley DB packages if your distribution provides them:

```sh
sudo apt-get install -y libdb5.3-dev:riscv64 libdb5.3++-dev:riscv64
```

If you do not need wallet support, build without Berkeley DB:

```sh
make -f makefile.cli RISCV64=1 NO_WALLET=1
```

### `undefined reference to __atomic_*`

Some RISC-V toolchains require linking `libatomic` explicitly:

```sh
make -f makefile.cli RISCV64=1 LIBS+="-latomic"
```

### Illegal instruction on startup

Rebuild without vector instructions unless the target CPU supports RVV:

```sh
make clean
make -f makefile.cli RISCV64=1
```

## Related documentation

- [`build-linux.md`](build-linux.md) — general Linux build flow
- [`build-params-reference.md`](build-params-reference.md) — common make variables
- [`../architecture/riscv-design.md`](../architecture/riscv-design.md) — RISC-V architecture and implementation rationale
- [`../current/node/riscv/index.md`](../current/node/riscv/index.md) — RISC-V node deployment notes
