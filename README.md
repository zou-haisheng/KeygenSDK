# KeygenSDK

A small, reusable C++20 client SDK for Keygen CE licensing on Windows, designed for CMake/vcpkg integration.

## Phase 1 status

Phase 1 establishes the library boundary, C++20 build, vcpkg manifest, libcurl HTTP foundation, result/error model, tests, and CMake package export.

The Keygen API endpoints and JSON contract are intentionally **not guessed** in this phase. Phase 2 will bind the HTTP layer to the verified Keygen CE 1.8 API documentation/source.

## Dependencies

Manifest mode dependencies:

- `curl`
- `nlohmann-json`

## Build with vcpkg Manifest Mode

Configure with your vcpkg toolchain file:

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

> The generator name may differ depending on the Visual Studio installation. Run `cmake --help` to see the installed generator names.

## Install and consume

```cmake
find_package(KeygenSDK CONFIG REQUIRED)
target_link_libraries(MyApplication PRIVATE KeygenSDK::KeygenSDK)
```

Then:

```cpp
#include <KeygenSDK/KeygenSDK.h>
```

No license key, token, secret, or private key is written by the SDK's Phase 1 example.
