# Nexus LLL-TAO Windows Node Release

This portable package contains `nexus.exe`, an example localhost mining
configuration, and dependency DLLs when the build is dynamically linked.

## Quick start

1. Extract the ZIP to a folder such as `C:\NexusNode`.
2. Create the Nexus config directory under your Windows APPDATA folder if it
   does not already exist, then copy `nexus.conf.example` there:

   ```powershell
   New-Item -ItemType Directory -Force "$env:APPDATA\Nexus"
   Copy-Item .\nexus.conf.example "$env:APPDATA\Nexus\nexus.conf"
   ```

   This usually maps to
   `C:\Users\YourUsername\AppData\Roaming\Nexus\nexus.conf`.
3. Edit the placeholder RPC/API credentials and any autologin settings.
4. Start the node:

   ```powershell
   .\nexus.exe
   ```

5. Start `NexusMiner.exe` on the same machine and point it at
   `127.0.0.1:9323` for stateless mining.

The example config enables both mining lanes for localhost use:

- Stateless mining: `127.0.0.1:9323`
- Legacy mining: `127.0.0.1:8323`

## Files

- `nexus.exe` - Nexus LLL-TAO node binary.
- `nexus.conf.example` - localhost node config for pairing with
  `NexusMiner.exe`.
- `SHA256SUMS.txt` - checksums for files inside this package.
- `*.dll` - runtime libraries copied from the build environment when required.

## Validation checklist

- Verify the ZIP checksum before extracting.
- Run `nexus.exe` on a clean Windows host to confirm no DLLs are missing.
- Confirm `%APPDATA%\Nexus\nexus.conf` is loaded and the mining ports bind.
- Confirm `NexusMiner.exe` can connect to `127.0.0.1:9323`.
