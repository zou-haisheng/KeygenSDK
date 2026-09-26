#include "KeygenSDK/FileLocalLicenseStore.h"
#include "KeygenSDK/LocalLicenseSerialization.h"
#include "KeygenSDK/LocalDataProtection.h"

#include <windows.h>

#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

namespace KeygenSDK {

    namespace {

        Result createTemporaryFilePath(
            const std::filesystem::path& targetPath,
            std::filesystem::path& temporaryPath) {

            const auto parentPath =
                targetPath.parent_path().empty()
                ? std::filesystem::current_path()
                : targetPath.parent_path();

            if (!std::filesystem::exists(parentPath)) {
                return Result::failure(
                    ErrorCode::LocalStorageError,
                    "Local license state directory does not exist.");
            }

            wchar_t temporaryFileName[MAX_PATH]{};

            const auto result =
                GetTempFileNameW(
                    parentPath.wstring().c_str(),
                    L"kgs",
                    0,
                    temporaryFileName);

            if (result == 0) {
                return Result::failure(
                    ErrorCode::LocalStorageError,
                    "Failed to create temporary local license state file.");
            }

            temporaryPath =
                std::filesystem::path(temporaryFileName);

            return Result::successResult();
        }

        Result replaceFile(
            const std::filesystem::path& temporaryPath,
            const std::filesystem::path& targetPath) {

            const BOOL result =
                MoveFileExW(
                    temporaryPath.wstring().c_str(),
                    targetPath.wstring().c_str(),
                    MOVEFILE_REPLACE_EXISTING |
                    MOVEFILE_WRITE_THROUGH);

            if (!result) {
                return Result::failure(
                    ErrorCode::LocalStorageError,
                    "Failed to replace local license state file.");
            }

            return Result::successResult();
        }

        void removeTemporaryFile(
            const std::filesystem::path& path) {

            if (path.empty()) {
                return;
            }

            std::error_code error;

            std::filesystem::remove(
                path,
                error);
        }

    } // namespace

    FileLocalLicenseStore::FileLocalLicenseStore(
        std::filesystem::path path)
        : path_(std::move(path)) {}

    Result FileLocalLicenseStore::load(
        LocalLicenseState& state) const {

        if (path_.empty()) {
            return Result::failure(
                ErrorCode::LocalStorageError,
                "Local license state path is empty.");
        }

        std::ifstream file(
            path_,
            std::ios::binary);

        if (!file.is_open()) {
            if (!std::filesystem::exists(path_)) {
                return Result::failure(
                    ErrorCode::LocalStateNotFound,
                    "Local license state file does not exist.");
            }

            return Result::failure(
                ErrorCode::LocalStorageError,
                "Failed to open local license state file.");
        }

        std::ostringstream buffer;

        buffer << file.rdbuf();

        if (file.fail() && !file.eof()) {
            return Result::failure(
                ErrorCode::LocalStorageError,
                "Failed to read local license state file.");
        }

        const std::string protectedData =
            buffer.str();

        if (protectedData.empty()) {
            return Result::failure(
                ErrorCode::LocalStorageError,
                "Local license state file is empty.");
        }

        std::string serialized;

        const auto unprotectResult =
            LocalDataProtection::unprotect(
                protectedData,
                serialized);

        if (!unprotectResult.ok) {
            return unprotectResult;
        }

        return LocalLicenseSerializer::deserialize(
            serialized,
            state);
    }

    Result FileLocalLicenseStore::save(
        const LocalLicenseState& state) {

        if (path_.empty()) {
            return Result::failure(
                ErrorCode::LocalStorageError,
                "Local license state path is empty.");
        }

        std::string serialized;

        const auto serializeResult =
            LocalLicenseSerializer::serialize(
                state,
                serialized);

        if (!serializeResult.ok) {
            return serializeResult;
        }

        std::string protectedData;

        const auto protectResult =
            LocalDataProtection::protect(
                serialized,
                protectedData);

        if (!protectResult.ok) {
            return protectResult;
        }

        const auto parentPath =
            path_.parent_path();

        if (!parentPath.empty()) {
            std::error_code directoryError;

            std::filesystem::create_directories(
                parentPath,
                directoryError);

            if (directoryError) {
                return Result::failure(
                    ErrorCode::LocalStorageError,
                    "Failed to create local license state directory.");
            }
        }

        std::filesystem::path temporaryPath;

        const auto temporaryPathResult =
            createTemporaryFilePath(
                path_,
                temporaryPath);

        if (!temporaryPathResult.ok) {
            return temporaryPathResult;
        }

        {
            std::ofstream file(
                temporaryPath,
                std::ios::binary |
                std::ios::trunc);

            if (!file.is_open()) {
                removeTemporaryFile(
                    temporaryPath);

                return Result::failure(
                    ErrorCode::LocalStorageError,
                    "Failed to open temporary local license state file.");
            }

            file.write(
                protectedData.data(),
                static_cast<std::streamsize>(
                    protectedData.size()));

            if (!file.good()) {
                removeTemporaryFile(
                    temporaryPath);

                return Result::failure(
                    ErrorCode::LocalStorageError,
                    "Failed to write temporary local license state file.");
            }

            file.flush();

            if (!file.good()) {
                removeTemporaryFile(
                    temporaryPath);

                return Result::failure(
                    ErrorCode::LocalStorageError,
                    "Failed to flush temporary local license state file.");
            }
        }

        const auto replaceResult =
            replaceFile(
                temporaryPath,
                path_);

        if (!replaceResult.ok) {
            removeTemporaryFile(
                temporaryPath);

            return replaceResult;
        }

        return Result::successResult(
            "Local license state saved successfully.");
    }

    Result FileLocalLicenseStore::remove() {

        if (path_.empty()) {
            return Result::failure(
                ErrorCode::LocalStorageError,
                "Local license state path is empty.");
        }

        std::error_code error;

        const bool removed =
            std::filesystem::remove(path_, error);

        if (error) {
            return Result::failure(
                ErrorCode::LocalStorageError,
                "Failed to remove local license state file.");
        }

        if (!removed) {
            return Result::failure(
                ErrorCode::LocalStateNotFound,
                "Local license state file does not exist.");
        }

        return Result::successResult(
            "Local license state removed successfully.");
    }

} // namespace KeygenSDK