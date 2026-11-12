# Build KeygenSDK from the upstream GitHub repository as a vcpkg port.
#
# SHA512 of the v1.0.0 archive:
# https://github.com/zou-haisheng/KeygenSDK/archive/refs/tags/v1.0.0.tar.gz
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO zou-haisheng/KeygenSDK
    REF v1.0.0
    SHA512 b69474918903d26c9e22bb950a21f217c2b2a90e39d2540ae08db392172d957b91311b1c05955c8e210e1b01ae6eafdea4a8e3698fc89b616a44c370a9df5f07
    HEAD_REF master
)

# Respect the triplet's static/dynamic choice.
if(VCPKG_LIBRARY_LINKAGE STREQUAL "dynamic")
    set(KEYGENSDK_BUILD_SHARED ON)
else()
    set(KEYGENSDK_BUILD_SHARED OFF)
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DKEYGENSDK_BUILD_TESTS=OFF
        -DKEYGENSDK_BUILD_EXAMPLE=OFF
        -DKEYGENSDK_BUILD_SHARED=${KEYGENSDK_BUILD_SHARED}
)

vcpkg_cmake_install()

# Move the CMake package config from lib/cmake/KeygenSDK to share/KeygenSDK
# so that `find_package(KeygenSDK CONFIG REQUIRED)` works from the vcpkg tree.
vcpkg_cmake_config_fixup(
    CONFIG_PATH lib/cmake/KeygenSDK
    PACKAGE_NAME KeygenSDK
)

# Keep only one copy of headers/docs (Debug and Release installs overlap).
file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
