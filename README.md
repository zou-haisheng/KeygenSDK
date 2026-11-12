[English](README.md) | [简体中文](README.zh-CN.md)

# KeygenSDK

A lightweight, modern C++ 20 client SDK for [Keygen CE](https://keygen.sh) licensing on
Windows. It lets C++ applications validate licenses, activate and deactivate machines,
persist license state locally, and verify licenses offline — using CMake and vcpkg for
clean integration.

## Features

- **Online license validation** — validate a license key against the Keygen API.
- **Machine activation / deactivation** — activate a machine, then deactivate it later.
- **Local license persistence** — save and restore activation state across process
  lifetimes, protected with Windows DPAPI.
- **Offline license verification** — verify a checked-out machine file without any
  network access, using Ed25519 signatures and AES-256-GCM.

## Requirements

- **C++20** compiler (MSVC; the project is developed with Visual Studio 2026).
- **CMake 3.25+** (Visual Studio 2026 bundles a compatible CMake 4.x).
- **vcpkg** in manifest mode (`x64-windows` or `x86-windows` triplet).
- **Windows (x86 and x64)**.

## Dependencies

Managed through the vcpkg manifest ([`vcpkg.json`](vcpkg.json)):

- `curl` — HTTP client.
- `nlohmann-json` — JSON parsing and serialization.
- `openssl` — Ed25519 signature verification and AES-256-GCM decryption for offline
  machine files.

Windows builds additionally use the system APIs `Crypt32` (DPAPI), `bcrypt`, and
`advapi32` for machine identity and local data protection.

## Building

Configure and build with the provided CMake presets:

```powershell
# Point vcpkg at your vcpkg installation
$env:VCPKG_ROOT = "C:\path\to\vcpkg"

# Configure (Visual Studio generator, x64)
cmake --preset vs2026-x64

# Build Debug and Release
cmake --build build-vs --config Debug
cmake --build build-vs --config Release

# Run tests
ctest --test-dir .\build-vs -C Debug --output-on-failure

# 32-bit (x86) build
cmake --preset vs2026-x86
cmake --build build-vs-x86 --config Debug
ctest --test-dir .\build-vs-x86 -C Debug --output-on-failure
```

A successful test run reports:

```text
100% tests passed, 0 tests failed
```

## Configuration

All SDK options are passed through the `KeygenSDK::Config` struct:

| Field | Type | Description |
|---|---|---|
| `host` | `std::string` | Keygen server URL. Must use `https://`. |
| `accountId` | `std::string` | Your Keygen account ID. |
| `accountPublicKey` | `std::string` | Hex-encoded Ed25519 account public key. Required only for `verifyOffline()`. |
| `timeoutSeconds` | `long` | HTTP timeout in seconds. Defaults to `15`. |
| `localLicensePath` | `std::filesystem::path` | Where to persist local license state. Empty means the default path. |

The default local license path is:

```text
%LOCALAPPDATA%\KeygenSDK\license.dat
```

```cpp
KeygenSDK::Config config{
    .host = "https://example.com",
    .accountId = "your-account-id",
    .accountPublicKey = "your-account-ed25519-public-key-hex",
    .timeoutSeconds = 15,
    .localLicensePath = "C:/MyApplication/license.dat"
};
```

`accountPublicKey` is only needed when using offline verification; it can be omitted for
online-only workflows. License keys and account-specific values are supplied at runtime
and are never hardcoded into the SDK.

## Quick Start

```cpp
#include <KeygenSDK/KeygenSDK.h>

#include <iostream>

int main() {
    KeygenSDK::Config config{
        .host = "https://example.com",
        .accountId = "your-account-id",
        .accountPublicKey = "your-account-ed25519-public-key-hex",
    };

    KeygenSDK::Client client(std::move(config));

    const KeygenSDK::Result result =
        client.validateOnline("YOUR_LICENSE_KEY");

    if (result.ok) {
        std::cout << "License is valid\n";
    } else {
        std::cout << "Validation failed: "
                  << KeygenSDK::toString(result.error) << '\n';
    }

    return 0;
}
```

## API

### Client

The primary entry point is `KeygenSDK::Client`:

```cpp
class Client {
public:
    explicit Client(Config config);
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;

    [[nodiscard]] Result validateOnline(const std::string& licenseKey);
    [[nodiscard]] Result activate(const std::string& licenseKey);
    [[nodiscard]] Result verifyOffline();
    [[nodiscard]] Result deactivate();
    [[nodiscard]] Result loadLocalLicense();
    [[nodiscard]] bool hasLocalLicense() const noexcept;
};
```

### Result

Every operation returns a `KeygenSDK::Result`:

```cpp
struct Result {
    bool ok{false};
    ErrorCode error{ErrorCode::Unknown};
    std::string message;

    [[nodiscard]] bool success() const noexcept { return ok; }
    explicit operator bool() const noexcept { return ok; }
};
```

Check `result.ok` (or use `result` directly in a boolean context) to determine success.
On failure, `result.error` and `result.message` describe the cause.

### Error codes

```cpp
enum class ErrorCode {
    None,
    InvalidConfiguration,
    NetworkError,
    InvalidLicense,
    LocalStateNotFound,
    LocalStorageError,
    LicenseExpired,
    LicenseSuspended,
    LicenseRevoked,
    LicenseOverdue,
    MachineLimitReached,
    ActivationFailed,
    OfflineDataMissing,
    OfflineVerificationFailed,
    InvalidResponse,
    ServerError,
    Unknown,
};
```

Use `KeygenSDK::toString(code)` to get a stable, human-readable name for an error code.

### Advanced types

The following lower-level types are also installed as public headers for advanced use:
`HttpClient` / `IHttpClient`, `MachineIdentity`, `MachineFileVerifier`,
`LocalLicenseState`, `ILocalLicenseStore`, `FileLocalLicenseStore`,
`LocalLicenseSerializer`, and `LocalDataProtection`.

## Usage

### Online license validation

Validates a license key against the Keygen `validate-key` endpoint:

```text
POST /v1/accounts/<accountId>/licenses/actions/validate-key
```

```cpp
const KeygenSDK::Result result =
    client.validateOnline("YOUR_LICENSE_KEY");
```

License-specific error conditions (expired, suspended, overdue, and so on) are mapped to
their corresponding `ErrorCode` values.

### Machine activation

`activate()` validates the license, generates a machine fingerprint, creates a machine on
the Keygen server, checks out the machine file, and persists the resulting local license
state:

```cpp
const KeygenSDK::Result result =
    client.activate("YOUR_LICENSE_KEY");

if (result.ok && client.hasLocalLicense()) {
    // The machine is activated and the state is persisted locally.
}
```

The machine fingerprint is derived from the Windows `MachineGuid` (hashed with SHA-256)
and is not raw hardware data.

### Machine deactivation

`deactivate()` removes the remote machine and clears the local state:

```cpp
const KeygenSDK::Result result = client.deactivate();
```

If the remote deactivation succeeds but local cleanup fails, the in-memory activation
state is still cleared.

### Offline license verification

`verifyOffline()` verifies the locally stored machine file without making any HTTP
request:

```cpp
const KeygenSDK::Result result = client.verifyOffline();

if (result.ok) {
    // The offline machine file is valid and unexpired.
}
```

It checks the Ed25519 signature, decrypts and validates the machine payload, verifies its
metadata and machine identity, and returns `LicenseExpired` for expired files.

### Restoring a local license

Restore previously persisted activation state in a later process lifetime:

```cpp
const KeygenSDK::Result result = client.loadLocalLicense();

if (result.ok && client.hasLocalLicense()) {
    // Local license state was restored.
}
```

## Local License Persistence

The persisted state contains the license ID, license key, machine ID, machine
fingerprint, and (after activation) the checked-out machine file.

### Schema

The state uses a versioned JSON representation. The current schema version is `2`:

```json
{
    "version": 2,
    "license": {
        "id": "...",
        "key": "..."
    },
    "machine": {
        "id": "...",
        "fingerprint": "...",
        "file": "..."
    }
}
```

Version `1` (without `machine.file`) is still accepted for backward compatibility with
states persisted before offline machine files were introduced.

### DPAPI protection

The serialized state is protected with the Windows Data Protection API (DPAPI) in the
current-user scope before being written to disk:

```text
LocalLicenseState → JSON → DPAPI → license.dat
```

The SDK does not store the license key or other local license state as plaintext on disk.

### Atomic writes

Saves go through a temporary file that is flushed and then atomically moved over the
target using `MoveFileExW(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)`. Temporary
files are cleaned up on failure, reducing the risk of leaving a partially written license
file.

## Offline Verification Details

The SDK verifies and decrypts Keygen machine files using the configured account public
key, the local license key, and the machine fingerprint.

The verifier:

- Requires the `MACHINE FILE` wrapper and the `aes-256-gcm+ed25519` algorithm.
- Verifies the Ed25519 signature over `machine/` followed by the encrypted payload.
- Derives the AES-256-GCM key as `SHA256(licenseKey + machineFingerprint)`.
- Decrypts the payload using AES-256-GCM (12-byte IV, 16-byte authentication tag).
- Validates the decrypted JSON:API machine resource, including its non-empty ID and
  `machines` type, plus its `meta.issued`, `meta.expiry`, and `meta.ttl` metadata.
- Reports expired files with `LicenseExpired`.

## Install and Consume

The project exports a CMake package. Install it and consume it from another CMake
project:

```cmake
find_package(KeygenSDK CONFIG REQUIRED)

target_link_libraries(
    MyApplication
    PRIVATE
        KeygenSDK::KeygenSDK
)
```

Then include the public header:

```cpp
#include <KeygenSDK/KeygenSDK.h>
```

The installed package declares `find_dependency` for `curl`, `nlohmann-json`, and
`openssl`, so the consuming project must provide those packages through vcpkg (with the
same `x64-windows` triplet) or otherwise make them available to CMake.

## Testing

The project uses a lightweight custom test harness (no GoogleTest) and a fake HTTP client,
so the suite runs without a live Keygen server.

Run the suite with:

```powershell
ctest --test-dir .\build-vs -C Debug --output-on-failure
```

The suite covers online validation (valid, expired, suspended, overdue, invalid licenses;
HTTP and server failures; invalid responses), machine activation and deactivation
(success and failure paths), local persistence (serialization, schema versions, DPAPI,
atomic replacement, temporary-file cleanup, tampered data), and offline machine-file
verification (Ed25519 and AES-256-GCM, timestamps, TTL, fingerprint and machine-ID
matching).

## Security Notes

The SDK does not hardcode license keys, account credentials, authentication tokens, or
private keys. Local license state is protected with Windows DPAPI, and machine
identification uses a hashed fingerprint rather than raw hardware data. Client-side
verification is not a substitute for server-side license enforcement.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
