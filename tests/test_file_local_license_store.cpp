#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>

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

    void testFileLocalLicenseStoreAtomicReplacement() {
        const auto path =
            makeTestPath(
                "keygensdk-test-atomic-replacement.dat");

        std::error_code error;
        std::filesystem::remove(path, error);

        KeygenSDK::FileLocalLicenseStore store(path);

        const auto firstState =
            KeygenSDK::LocalLicenseState{
                .licenseId = "first-license-id",
                .licenseKey = "FIRST-LICENSE-KEY",
                .machineId = "first-machine-id",
                .machineFingerprint = "first-fingerprint"
        };

        const auto secondState =
            KeygenSDK::LocalLicenseState{
                .licenseId = "second-license-id",
                .licenseKey = "SECOND-LICENSE-KEY",
                .machineId = "second-machine-id",
                .machineFingerprint = "second-fingerprint"
        };

        const auto firstSaveResult =
            store.save(firstState);

        assert(firstSaveResult.ok);
        assert(std::filesystem::exists(path));

        const auto secondSaveResult =
            store.save(secondState);

        assert(secondSaveResult.ok);
        assert(std::filesystem::exists(path));

        KeygenSDK::LocalLicenseState actual;

        const auto loadResult =
            store.load(actual);

        assert(loadResult.ok);

        assert(
            actual.licenseId ==
            secondState.licenseId);

        assert(
            actual.licenseKey ==
            secondState.licenseKey);

        assert(
            actual.machineId ==
            secondState.machineId);

        assert(
            actual.machineFingerprint ==
            secondState.machineFingerprint);

        std::filesystem::remove(path, error);
    }

    void testFileLocalLicenseStoreDoesNotLeaveTemporaryFile() {
        const auto path =
            makeTestPath(
                "keygensdk-test-no-temporary-file.dat");

        std::error_code error;
        std::filesystem::remove(path, error);

        KeygenSDK::FileLocalLicenseStore store(path);

        const auto result =
            store.save(makeTestState());

        assert(result.ok);
        assert(std::filesystem::exists(path));

        const auto parentPath =
            path.parent_path();

        bool foundTemporaryFile = false;

        for (const auto& entry :
            std::filesystem::directory_iterator(parentPath)) {

            if (!entry.is_regular_file()) {
                continue;
            }

            const auto filename =
                entry.path().filename().wstring();

            const auto targetFilename =
                path.filename().wstring();

            if (filename != targetFilename &&
                filename.rfind(L"kgs", 0) == 0) {

                foundTemporaryFile = true;
                break;
            }
        }

        assert(!foundTemporaryFile);

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

    void testFileLocalLicenseStoreDoesNotStorePlaintext() {
        const auto path =
            makeTestPath(
                "keygensdk-test-protected-license-state.json");

        std::error_code error;
        std::filesystem::remove(path, error);

        const auto state = makeTestState();

        KeygenSDK::FileLocalLicenseStore store(path);

        const auto saveResult =
            store.save(state);

        assert(saveResult.ok);

        std::ifstream file(
            path,
            std::ios::binary);

        assert(file.is_open());

        std::ostringstream buffer;
        buffer << file.rdbuf();

        const std::string contents =
            buffer.str();

        assert(!contents.empty());
        assert(
            contents.find(state.licenseKey) ==
            std::string::npos);

        assert(
            contents.find(state.licenseId) ==
            std::string::npos);

        assert(
            contents.find(state.machineId) ==
            std::string::npos);

        std::filesystem::remove(path, error);
    }

    void testFileLocalLicenseStoreRejectsTamperedData() {
        const auto path =
            makeTestPath(
                "keygensdk-test-tampered-license-state.dat");

        {
            std::ofstream file(
                path,
                std::ios::binary);

            file << "tampered-data";
        }

        KeygenSDK::FileLocalLicenseStore store(path);

        KeygenSDK::LocalLicenseState state;

        const auto result =
            store.load(state);

        assert(!result.ok);

        std::error_code error;
        std::filesystem::remove(path, error);
    }

} // namespace KeygenSDKTests