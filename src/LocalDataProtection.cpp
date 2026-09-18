#include "KeygenSDK/LocalDataProtection.h"

#include <windows.h>
#include <wincrypt.h>

#include <string>

namespace KeygenSDK {
    namespace {

        Result makeProtectionError(const char* message) {
            return Result::failure(
                ErrorCode::LocalStorageError,
                message);
        }

    } // namespace

    Result LocalDataProtection::protect(
        const std::string& plaintext,
        std::string& protectedData) {

        if (plaintext.empty()) {
            return Result::failure(
                ErrorCode::LocalStorageError,
                "Plaintext cannot be empty.");
        }

        DATA_BLOB input{};
        input.pbData =
            reinterpret_cast<BYTE*>(
                const_cast<char*>(plaintext.data()));
        input.cbData =
            static_cast<DWORD>(plaintext.size());

        DATA_BLOB output{};

        if (!CryptProtectData(
            &input,
            L"KeygenSDK Local License Cache",
            nullptr,
            nullptr,
            nullptr,
            0,
            &output)) {

            return makeProtectionError(
                "Failed to protect local data.");
        }

        protectedData.assign(
            reinterpret_cast<const char*>(output.pbData),
            output.cbData);

        LocalFree(output.pbData);

        return Result::successResult(
            "Local data protected successfully.");
    }

    Result LocalDataProtection::unprotect(
        const std::string& protectedData,
        std::string& plaintext) {

        if (protectedData.empty()) {
            return Result::failure(
                ErrorCode::LocalStorageError,
                "Protected data cannot be empty.");
        }

        DATA_BLOB input{};
        input.pbData =
            reinterpret_cast<BYTE*>(
                const_cast<char*>(protectedData.data()));
        input.cbData =
            static_cast<DWORD>(protectedData.size());

        DATA_BLOB output{};

        if (!CryptUnprotectData(
            &input,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            0,
            &output)) {

            return makeProtectionError(
                "Failed to unprotect local data.");
        }

        plaintext.assign(
            reinterpret_cast<const char*>(output.pbData),
            output.cbData);

        LocalFree(output.pbData);

        return Result::successResult(
            "Local data unprotected successfully.");
    }

} // namespace KeygenSDK