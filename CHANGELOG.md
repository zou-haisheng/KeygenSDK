# Changelog

All notable changes to KeygenSDK are documented in this file.

## [1.0.4] - 2026-11-15

### Added
- C++ `Client`: `exportMachineFile()`, `importMachineFile()`, `licenseId()`, and
  `licenseExpiry()` for machine-file transfer and license metadata.
- C API: `KeygenSDK_MachineFingerprint`, `KeygenSDK_Client_ExportMachineFile`,
  `KeygenSDK_Client_ImportMachineFile`, `KeygenSDK_Client_GetLicenseInfo`, and the
  `KeygenSDK_LicenseInfo` struct.
- Tests for the new fingerprint, machine-file export/import, and license-info APIs.

### Changed
- The C ABI bridge is now built by default (`KEYGENSDK_BUILD_C_API` defaults to `ON`), so
  the C header and C ABI symbols are part of a normal build.
- The C example target moved behind `KEYGENSDK_BUILD_C_API_EXAMPLE` (default `OFF`), so a
  default build no longer requires a C compiler.

## [1.0.3] - 2026-11-15

### Added
- `KeygenSDK_Client_Create` gained an optional `out_error` parameter so callers can
  distinguish a configuration error (`KEYGENSDK_ERROR_INVALID_CONFIGURATION`) from an
  allocation failure (`KEYGENSDK_ERROR_UNKNOWN`).

### Fixed
- `CMAKE_MSVC_RUNTIME_LIBRARY` is now only assigned when not already defined, so
  consumers that pull the SDK in via `add_subdirectory()` keep their own CRT setting.

### Testing
- Added `tests/test_c_api.cpp` covering: NULL config, invalid UTF-8 path, a Chinese path,
  the 17 error-code ↔ string mappings, and NULL-client operations.

## [1.0.2] - 2026-11-15

### Fixed
- The MSVC runtime library is now applied globally via `CMAKE_MSVC_RUNTIME_LIBRARY`, so
  the library, tests, and examples all use the same CRT, and it no longer overrides a
  parent project's setting when the SDK is consumed via `add_subdirectory()`.
- An invalid UTF-8 `local_license_path` now makes `KeygenSDK_Client_Create` fail with
  `KEYGENSDK_ERROR_INVALID_CONFIGURATION` instead of silently falling back to the
  default path. `Create` also gained an optional `out_error` parameter to report the
  failure reason.

## [1.0.1] - 2026-11-15

### Added
- Optional C ABI bridge: `keygensdk_c.h`, the `KEYGENSDK_BUILD_C_API` CMake option, and
  the `KeygenSDKCApiExample` C example target.
- C API tests covering NULL config, invalid UTF-8 path, a Chinese path, and the
  error-code ↔ string mapping.

### Fixed
- The C ABI bridge catches all exceptions internally (reported as
  `KEYGENSDK_ERROR_UNKNOWN`) instead of letting them cross the `extern "C"` boundary.
- `local_license_path` is converted from UTF-8 to a wide path on Windows, fixing garbled
  non-ASCII paths (e.g. Chinese).
- The MSVC runtime library is configurable via `KEYGENSDK_MSVC_RUNTIME_LIBRARY`
  (`/MD` vs `/MT`).

## [1.0.0] - 2026-11-08

### Added
- C++20 client SDK for Keygen CE licensing (Windows x86/x64).
- Online license validation (`Client::validateOnline`) via the Keygen `validate-key` endpoint.
- Machine activation and deactivation (`Client::activate`, `Client::deactivate`).
- Machine fingerprint generation (`MachineIdentity`) from the Windows MachineGuid (SHA-256).
- Local license state persistence (`LocalLicenseState`, `LocalLicenseSerializer`).
- Windows DPAPI protection of local license data (`LocalDataProtection`).
- Atomic-safe file persistence with temporary-file cleanup (`FileLocalLicenseStore`).
- Offline machine file verification (`MachineFileVerifier`, `Client::verifyOffline`) using
  Ed25519 signature verification and AES-256-GCM decryption, matching Keygen's
  `aes-256-gcm+ed25519` machine file format.
- CMake package export (`find_package(KeygenSDK CONFIG)` / `KeygenSDK::KeygenSDK`).
- Native lightweight test suite (no GoogleTest).

### Fixed
- CMake package config now declares `find_dependency` for `nlohmann_json` and `OpenSSL`
  (previously only `CURL`), so downstream `find_package(KeygenSDK)` resolves all
  transitive dependencies.

### Testing
- Debug and Release builds pass the native test suite via CTest
  (`100% tests passed, 0 tests failed`).

### Documentation
- README: corrected a code fence, documented the `openssl` dependency, updated the local
  license JSON schema version to `2`, documented `Config::accountPublicKey`, and
  clarified the MIT license and downstream vcpkg requirement.

### Platform Support
- Windows (x86 and x64). vcpkg manifest mode (`x64-windows` / `x86-windows` triplet).

### Known Limitations
- `Client::activate` does not clean up the remote machine if the machine-file checkout
  fails after machine creation.
- `ErrorCode::MachineLimitReached` is defined but not currently returned by validation.
- The HTTP client does not cap the response body size.
