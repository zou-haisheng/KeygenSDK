cmake_minimum_required(VERSION 3.25)

project(KeygenSDK
    VERSION 1.0.0
    LANGUAGES CXX
)

option(KEYGENSDK_BUILD_TESTS "Build KeygenSDK tests" ON)
option(KEYGENSDK_BUILD_EXAMPLE "Build basic example" ON)
option(KEYGENSDK_BUILD_SHARED "Build KeygenSDK as a shared library" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(KEYGENSDK_LIBRARY_TYPE STATIC)
if(KEYGENSDK_BUILD_SHARED)
    set(KEYGENSDK_LIBRARY_TYPE SHARED)
endif()

find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_library(KeygenSDK ${KEYGENSDK_LIBRARY_TYPE}
    src/Client.cpp
    src/HttpClient.cpp
)

add_library(KeygenSDK::KeygenSDK ALIAS KeygenSDK)

target_compile_features(KeygenSDK PUBLIC cxx_std_20)

target_include_directories(KeygenSDK
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(KeygenSDK PRIVATE CURL::libcurl)

if(MSVC)
    target_compile_options(KeygenSDK PRIVATE /W4 /permissive-)
else()
    target_compile_options(KeygenSDK PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS KeygenSDK
    EXPORT KeygenSDKTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT KeygenSDKTargets
    FILE KeygenSDKTargets.cmake
    NAMESPACE KeygenSDK::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/KeygenSDK
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/KeygenSDKConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KeygenSDKConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/KeygenSDK
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/KeygenSDKConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/KeygenSDKConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/KeygenSDKConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/KeygenSDK
)

if(KEYGENSDK_BUILD_TESTS)
    enable_testing()
    add_executable(KeygenSDKTests tests/test_client.cpp)
    target_link_libraries(KeygenSDKTests PRIVATE KeygenSDK::KeygenSDK)
    add_test(NAME KeygenSDKTests COMMAND KeygenSDKTests)
endif()

if(KEYGENSDK_BUILD_EXAMPLE)
    add_executable(KeygenSDKBasicExample examples/basic/main.cpp)
    target_link_libraries(KeygenSDKBasicExample PRIVATE KeygenSDK::KeygenSDK)
endif()
