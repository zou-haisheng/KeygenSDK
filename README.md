# KeygenSDK

A small, reusable C++20 client SDK for Keygen CE licensing on Windows, designed for CMake/vcpkg integration.

## Project Status

The project is developed incrementally in phases.

The current implementation has completed:

- SDK foundation
- Online license validation
- Machine activation and deactivation
- Local license cache and persistence

Offline license verification is intentionally not implemented yet and is planned for Phase 5.

---

### Phase 1 — Foundation

Phase 1 establishes the library boundary, C++20 build, vcpkg manifest, libcurl HTTP foundation, result/error model, tests, and CMake package export.

It includes:

- C++20 project structure
- CMake build system
- vcpkg manifest
- libcurl HTTP foundation
- Result and error model
- Public SDK headers
- CMake package export
- Basic test infrastructure

The Keygen API endpoints and JSON contract were intentionally not guessed in this phase.

---

### Phase 2 — Online License Validation

Phase 2 binds the SDK to the verified Keygen CE 1.8 API for online license validation.

The SDK supports:

- Online license-key validation
- JSON:API request/response handling
- Keygen license error mapping
- Configuration validation
- HTTP and server error handling
- License ID extraction from successful validation responses
- Unit tests using a fake HTTP client

The validation endpoint is:

```text
POST /v1/accounts/<accountId>/licenses/actions/validate-key
```

The license key is sent in the JSON:API request body rather than being hardcoded into the SDK.

### Phase 3 — Machine Activation

Phase 3 adds machine activation and deactivation based on the verified Keygen machine API.

The SDK supports:

- Machine fingerprint generation through `MachineIdentity`
- License validation before activation
- Machine creation through the Keygen Machines API
- License-key authorization for machine creation and deletion
- Validation of the returned machine resource
- Local activation state tracking
- Machine deactivation
- Safe preservation of local activation state when deactivation fails
- Unit tests covering activation and deactivation success and failure paths

Machine activation uses:

```text
POST /v1/accounts/<accountId>/machines
```

Machine deactivation uses:

```text
DELETE /v1/accounts/<accountId>/machines/<machineId>
```

The current Phase 3 implementation keeps the machine ID and license key in the in-memory client state. Persistent local license storage is intentionally deferred to Phase 4.

---

## Phase 4 — Local License Cache and Persistence
Phase 4 adds persistent local storage for the activation state.

The goal of this phase is to allow an activated client to safely persist its local license state and restore it in a later process lifetime.

Phase 4 is now complete.

### Local License State

The persisted state contains:

- License ID
- License key
- Machine ID
- Machine fingerprint

The SDK validates that all required fields are present and non-empty before accepting a local license state.

### JSON Serialization

The local license state uses a versioned JSON representation.

Current schema version:

```text
1
```

The logical structure is:

```json
{
    "version": 1,
    "license": {
        "id": "...",
        "key": "..."
    },
    "machine": {
        "id": "...",
        "fingerprint": "..."
    }
}
```
The serializer validates:

- JSON object structure
- Schema version
- Required sections
- Required field types
- Empty required fields
- Unsupported schema versions

Unknown fields are currently allowed so that the schema can evolve without unnecessarily rejecting additional data.

### Windows DPAPI Protection

Persistent local license data is protected using Windows Data Protection API (DPAPI).

The storage flow is:

```text
LocalLicenseState
       ↓
JSON serialization
       ↓
Windows DPAPI encryption
       ↓
license.dat
```

The SDK does not intentionally store the local license key or other local license state as plaintext on disk.

The current implementation uses the Windows current-user DPAPI scope rather than machine-wide protection.

### FileLocalLicenseStore

Persistent storage is separated behind the ILocalLicenseStore interface.

The current file-based implementation is:

```cpp
KeygenSDK::FileLocalLicenseStore
```

It provides:

```cpp
Result load(LocalLicenseState& state) const;
Result save(const LocalLicenseState& state);
Result remove();
```

The store handles:

- Loading persisted state
- Saving persisted state
- Removing persisted state
- Missing-state detection
- Directory creation
- DPAPI protection and unprotection
- JSON serialization and deserialization
- Invalid or tampered data detection

### Atomic-Safe Persistence

Saving local state does not directly truncate and overwrite the final license file.

The current save flow is:

```text
serialize
    ↓
DPAPI protect
    ↓
create temporary file
    ↓
write protected data
    ↓
flush
    ↓
replace target file
```

On Windows, the final replacement uses:

```text
MoveFileExW(
    ...,
    MOVEFILE_REPLACE_EXISTING |
    MOVEFILE_WRITE_THROUGH
)
```

Temporary files are cleaned up when a save operation fails.

This reduces the risk of leaving a partially written final license file when persistence is interrupted.

### Default Local License Path

If no explicit local license path is configured, the SDK uses:

```text
%LOCALAPPDATA%\KeygenSDK\license.dat
```

The path can also be customized through:

```cpp
KeygenSDK::Config::localLicensePath
```

For example:

```cpp
KeygenSDK::Config config{
    .host = "https://example.com",
    .accountId = "your-account-id",
    .timeoutSeconds = 15,
    .localLicensePath = "C:/MyApplication/license.dat"
};
```

The parent directory is created automatically when saving local license state.

### Client Lifecycle Integration

The persistent store is integrated into the Client lifecycle.

Activation now follows:

```text
validate license online
        ↓
generate machine fingerprint
        ↓
create machine remotely
        ↓
create local license state
        ↓
save local license state
        ↓
mark client as locally activated
```

If persistent storage fails, the client does not mark the local license as active.

Local license state can be restored with:

```cpp
client.loadLocalLicense();
```

A missing local state is treated as a normal "no local license state" condition rather than as a corrupted state.

Deactivation follows:

```text
delete remote machine
        ↓
remove local license state
        ↓
clear in-memory activation state
```

If remote deactivation succeeds but local file removal fails, the in-memory activation state is still cleared because the remote deactivation has already succeeded.

## Phase 4 Testing

Phase 4 includes tests for:

- Local license state validation
- JSON serialization and deserialization
- Unsupported schema versions
- Missing required fields
- Invalid field types
- Empty required fields
- Unknown JSON fields
- DPAPI protection and unprotection
- Invalid protected data
- Protection failure behavior
- File-based save and load
- Missing local state
- Local state removal
- Tampered persisted data
- Plaintext storage checks
- Atomic replacement
- Temporary file cleanup
- Client local-state loading
- Activation persistence failures
- Real client persistence lifecycle

The end-to-end persistence test covers:

```text
Client
  ↓
FileLocalLicenseStore
  ↓
LocalLicenseSerializer
  ↓
Windows DPAPI
  ↓
license.dat
```

and the reverse loading path.

---

## Current API

The public client API currently includes:

```cpp
class Client {
public:
    explicit Client(Config config);
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;

    [[nodiscard]] Result validateOnline(
        const std::string& licenseKey);

    [[nodiscard]] Result activate(
        const std::string& licenseKey);

    [[nodiscard]] Result verifyOffline();
    [[nodiscard]] Result deactivate();
    [[nodiscard]] Result loadLocalLicense();
    [[nodiscard]] bool hasLocalLicense() const noexcept;
};
```

`verifyOffline()` is currently a placeholder.

Offline verification is not implemented yet. It remains part of a later phase.

---

## Configuration

The SDK requires runtime configuration for the Keygen server and account:

```cpp
KeygenSDK::Config config{
    .host = "https://example.com",
    .accountId = "your-account-id",
    .timeoutSeconds = 15
};
```

Optional local license storage configuration:

```xpp
KeygenSDK::Config config{
    .host = "https://example.com",
    .accountId = "your-account-id",
    .timeoutSeconds = 15,
    .localLicensePath = "C:/MyApplication/license.dat"
};
```

If `localLicensePath` is empty, the SDK uses:

```text
%LOCALAPPDATA%\KeygenSDK\license.dat
```

License keys and account-specific values are supplied at runtime and are not hardcoded into the SDK.

---

## Dependencies

Manifest mode dependencies:

- `curl`
- `nlohmann-json`

Windows builds also use:

- Windows Cryptography API (Crypt32)
- Windows system APIs for machine identity and local data protection

---

## Build with Visual Studio and CMake

The project is intended to be configured and built through Visual Studio's built-in CMake integration.

After configuring and building the project in Visual Studio, tests can be run with:

```powershell
ctest --test-dir .\build-vs -C Debug --output-on-failure
```

A successful test run should report:

```text
100% tests passed, 0 tests failed
```

---

## Install and Consume

The project exports a CMake package:

```cmake
find_package(KeygenSDK CONFIG REQUIRED)

target_link_libraries(
    MyApplication
    PRIVATE
        KeygenSDK::KeygenSDK
)
```

Then include the public SDK header:

```cpp
#include <KeygenSDK/KeygenSDK.h>
```

---

## Testing

The project uses a lightweight custom test harness rather than GoogleTest.

The test suite uses a fake HTTP client to exercise client behavior without requiring a live Keygen server.

The current tests cover:

### Online Validation
- Valid licenses
- Expired licenses
- Suspended licenses
- Overdue licenses
- Invalid licenses
- HTTP failures
- Server failures
- Invalid JSON
- Invalid API responses
- Configuration errors
- License error mapping

### Machine Activation
- Successful activation
- License validation failures
- Machine creation failures
- Invalid machine responses
- Invalid machine types
- Missing machine fingerprints
- HTTP failures
- Server failures
- Local activation state updates
- Local persistence failures

### Machine Deactivation
- Successful deactivation
- HTTP failures
- Invalid response status
- Missing local license
- Missing machine ID
- Missing license key
- Local persistence removal failures
- Preservation and clearing of local state according to the operation result

### Local License Persistence
- Local license state validation
- JSON serialization
- JSON deserialization
- Schema version validation
- Invalid JSON
- Missing required fields
- Invalid field types
- Unknown fields
- DPAPI round-trip
- Invalid protected data
- File save/load/remove
- Missing state
- Tampered data
- Plaintext storage checks
- Atomic file replacement
- Temporary file cleanup
- Client persistence lifecycle

---

## Security Notes

The SDK does not hardcode:

- License keys
- Account credentials
- Authentication tokens
- Private keys

Persistent local license state is protected using Windows DPAPI.

The local license cache is intended to protect the stored activation state from being kept as plaintext on disk.

Machine identification is represented by a fingerprint generated by `MachineIdentity`.

The current implementation does not require collecting unnecessary hardware information for the activation flow.

Local persistence is implemented in Phase 4.

Offline license verification is intentionally not implemented yet and will be addressed separately in Phase 5.

---

## Development Roadmap

The completed phases are:

- [x] Phase 1 — SDK foundation
- [x] Phase 2 — Online license validation
- [x] Phase 3 — Machine activation and deactivation
- [x] Phase 4 — Local license cache and persistence
- [ ] Phase 5 — Offline license verification
- [ ] Phase 6 — Final hardening, documentation, and release preparation

Each phase is implemented and tested incrementally before moving to the next phase.

---

## Phase 5 — Offline License Verification

Phase 5 will address offline license verification.

The design and implementation are intentionally kept separate from Phase 4.

Phase 4 provides the persistent local activation state required by later functionality, but it does not itself make any offline validity decision.

Phase 5 will determine how the persisted license state, machine identity, and other required information can be used for offline verification.

No offline verification behavior should be assumed from the current Phase 4 implementation.

---

## License

This project is currently under active development.
