# KeygenSDK vcpkg port

This directory contains a [vcpkg](https://vcpkg.io) port for KeygenSDK, so it can be
consumed through vcpkg instead of a manual CMake build.

## Using the port (overlay)

Reference the port directly from a checkout of this repository:

```powershell
vcpkg install keygensdk --overlay-ports=<path-to-this-repo>\ports
```

## Using the self-hosted registry

This repository also acts as a vcpkg git registry (`ports/` + `versions/`). Add it to the
consuming project's `vcpkg-configuration.json`:

```json
{
  "registries": [
    {
      "kind": "git",
      "repository": "https://github.com/zou-haisheng/KeygenSDK",
      "baseline": "<commit-that-contains-versions-and-ports>",
      "packages": [ "keygensdk" ]
    }
  ]
}
```

Then in the consuming project's `vcpkg.json`:

```json
{
  "dependencies": [ "keygensdk" ]
}
```

and in CMake:

```cmake
find_package(KeygenSDK CONFIG REQUIRED)
target_link_libraries(MyApp PRIVATE KeygenSDK::KeygenSDK)
```

## Regenerating the version entry

After changing `ports/keygensdk/`, regenerate the version database entry and the
`git-tree` hash with:

```powershell
vcpkg x-add-version keygensdk --overlay-ports=<path-to-this-repo>\ports
```

The port is Windows-only (`"supports": "windows"`), which is why vcpkg will skip it on
non-Windows triples.
