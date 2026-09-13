#include "KeygenSDK/FileLocalLicenseStore.h"

#include <fstream>
#include <sstream>
#include <utility>

#include "KeygenSDK/LocalLicenseSerialization.h"

namespace KeygenSDK {

    FileLocalLicenseStore::FileLocalLicenseStore(
        std::filesystem::path path)
        : path_(std::move(path)) {}

    Result FileLocalLicenseStore::load(
        LocalLicenseState& state) const {

        std::ifstream file(path_, std::ios::binary);

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

        const std::string contents = buffer.str();

        return LocalLicenseSerializer::deserialize(
            contents,
            state);
    }

    Result FileLocalLicenseStore::save(
        const LocalLicenseState& state) {

        std::string serialized;

        const auto serializeResult =
            LocalLicenseSerializer::serialize(
                state,
                serialized);

        if (!serializeResult.ok) {
            return serializeResult;
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

        file << serialized;

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