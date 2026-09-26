#include "KeygenSDK/FileLocalLicenseStore.h"
#include "KeygenSDK/LocalLicenseSerialization.h"
#include "KeygenSDK/LocalDataProtection.h"

#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

namespace KeygenSDK {

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

        std::ofstream file(
            path_,
            std::ios::binary |
            std::ios::trunc);

        if (!file.is_open()) {
            return Result::failure(
                ErrorCode::LocalStorageError,
                "Failed to open local license state file for writing.");
        }

        file.write(
            protectedData.data(),
            static_cast<std::streamsize>(
                protectedData.size()));

        if (!file.good()) {
            return Result::failure(
                ErrorCode::LocalStorageError,
                "Failed to write local license state file.");
        }

        file.flush();

        if (!file.good()) {
            return Result::failure(
                ErrorCode::LocalStorageError,
                "Failed to flush local license state file.");
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