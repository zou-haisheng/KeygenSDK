#include <cassert>
#include <filesystem>
#include <fstream>

#include "KeygenSDK/FileLocalLicenseStore.h"

namespace KeygenSDKTests {

    namespace {

        std::filesystem::path makeTestPath(
            const char* name) {

            return std::filesystem::temp_directory_path() / name;
        }

        KeygenSDK::LocalLicenseState makeTestState() {
            return {
                .licenseId = "test-license-id",
                .licenseKey = "TEST-LICENSE-KEY",
                .machineId = "test-machine-id",
                .machineFingerprint = "test-fingerprint"
            };
        }

    } // namespace

    void testFileLocalLicenseStoreSaveAndLoad() {
        const auto path =
            makeTestPath("keygensdk-test-license-state.json");

        std::error_code error;
        std::filesystem::remove(path, error);

        KeygenSDK::FileLocalLicenseStore store(path);

        const auto expected = makeTestState();

        const auto saveResult = store.save(expected);

        assert(saveResult.ok);
        assert(std::filesystem::exists(path));

        KeygenSDK::LocalLicenseState actual;

        const auto loadResult = store.load(actual);

        assert(loadResult.ok);
        assert(actual.licenseId == expected.licenseId);
        assert(actual.licenseKey == expected.licenseKey);
        assert(actual.machineId == expected.machineId);
        assert(
            actual.machineFingerprint ==
            expected.machineFingerprint);

        std::filesystem::remove(path, error);
    }

    void testFileLocalLicenseStoreLoadMissingFile() {
        const auto path =
            makeTestPath("keygensdk-test-missing-license-state.json");

        std::error_code error;
        std::filesystem::remove(path, error);

        KeygenSDK::FileLocalLicenseStore store(path);

        KeygenSDK::LocalLicenseState state;

        const auto result = store.load(state);

        assert(!result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::LocalStateNotFound);
    }

    void testFileLocalLicenseStoreRemove() {
        const auto path =
            makeTestPath("keygensdk-test-remove-license-state.json");

        std::error_code error;
        std::filesystem::remove(path, error);

        KeygenSDK::FileLocalLicenseStore store(path);

        const auto saveResult =
            store.save(makeTestState());

        assert(saveResult.ok);
        assert(std::filesystem::exists(path));

        const auto removeResult = store.remove();

        assert(removeResult.ok);
        assert(!std::filesystem::exists(path));
    }

    void testFileLocalLicenseStoreMalformedJson() {
        const auto path =
            makeTestPath("keygensdk-test-malformed-license-state.json");

        {
            std::ofstream file(path);
            file << "{ invalid json";
        }

        KeygenSDK::FileLocalLicenseStore store(path);

        KeygenSDK::LocalLicenseState state;

        const auto result = store.load(state);

        assert(!result.ok);

        std::error_code error;
        std::filesystem::remove(path, error);
    }

} // namespace KeygenSDKTests