# Changelog

All notable changes to KeygenSDK are documented in this file.

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
