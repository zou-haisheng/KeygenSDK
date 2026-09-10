#include "KeygenSDK/MachineIdentity.h"

#ifdef _WIN32

#include <windows.h>
#include <bcrypt.h>

#include <array>
#include <string>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

namespace KeygenSDK {
    namespace {

        constexpr wchar_t kMachineGuidPath[] =
            L"SOFTWARE\\Microsoft\\Cryptography";

        constexpr wchar_t kMachineGuidValue[] =
            L"MachineGuid";

        Result readMachineGuid(std::string& value) {
            HKEY key = nullptr;

            const LSTATUS openStatus = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                kMachineGuidPath,
                0,
                KEY_READ | KEY_WOW64_64KEY,
                &key);

            if (openStatus != ERROR_SUCCESS) {
                return Result::failure(
                    ErrorCode::ActivationFailed,
                    "Failed to open Windows machine identity registry key.");
            }

            DWORD type = 0;
            DWORD size = 0;

            LSTATUS status = RegQueryValueExW(
                key,
                kMachineGuidValue,
                nullptr,
                &type,
                nullptr,
                &size);

            if (status != ERROR_SUCCESS ||
                type != REG_SZ ||
                size < sizeof(wchar_t)) {

                RegCloseKey(key);

                return Result::failure(
                    ErrorCode::ActivationFailed,
                    "Windows MachineGuid is unavailable.");
            }

            std::vector<wchar_t> buffer(
                size / sizeof(wchar_t) + 1,
                L'\0');

            status = RegQueryValueExW(
                key,
                kMachineGuidValue,
                nullptr,
                &type,
                reinterpret_cast<LPBYTE>(buffer.data()),
                &size);

            RegCloseKey(key);

            if (status != ERROR_SUCCESS ||
                type != REG_SZ) {

                return Result::failure(
                    ErrorCode::ActivationFailed,
                    "Failed to read Windows MachineGuid.");
            }

            const std::wstring wideGuid(buffer.data());

            if (wideGuid.empty()) {
                return Result::failure(
                    ErrorCode::ActivationFailed,
                    "Windows MachineGuid is empty.");
            }

            int required = WideCharToMultiByte(
                CP_UTF8,
                WC_ERR_INVALID_CHARS,
                wideGuid.data(),
                static_cast<int>(wideGuid.size()),
                nullptr,
                0,
                nullptr,
                nullptr);

            if (required <= 0) {
                return Result::failure(
                    ErrorCode::ActivationFailed,
                    "Failed to convert Windows MachineGuid.");
            }

            std::string utf8(required, '\0');

            if (WideCharToMultiByte(
                CP_UTF8,
                WC_ERR_INVALID_CHARS,
                wideGuid.data(),
                static_cast<int>(wideGuid.size()),
                utf8.data(),
                required,
                nullptr,
                nullptr) <= 0) {

                return Result::failure(
                    ErrorCode::ActivationFailed,
                    "Failed to convert Windows MachineGuid.");
            }

            value = std::move(utf8);
            return Result::successResult();
        }

        Result sha256(
            const std::string& input,
            std::array<unsigned char, 32>& digest) {

            BCRYPT_ALG_HANDLE algorithm = nullptr;
            BCRYPT_HASH_HANDLE hash = nullptr;

            NTSTATUS status = BCryptOpenAlgorithmProvider(
                &algorithm,
                BCRYPT_SHA256_ALGORITHM,
                nullptr,
                0);

            if (status < 0) {
                return Result::failure(
                    ErrorCode::ActivationFailed,
                    "Failed to initialize SHA-256.");
            }

            status = BCryptCreateHash(
                algorithm,
                &hash,
                nullptr,
                0,
                nullptr,
                0,
                0);

            if (status < 0) {
                BCryptCloseAlgorithmProvider(algorithm, 0);

                return Result::failure(
                    ErrorCode::ActivationFailed,
                    "Failed to create SHA-256 hash.");
            }

            status = BCryptHashData(
                hash,
                reinterpret_cast<PUCHAR>(
                    const_cast<char*>(input.data())),
                static_cast<ULONG>(input.size()),
                0);

            if (status < 0) {
                BCryptDestroyHash(hash);
                BCryptCloseAlgorithmProvider(algorithm, 0);

                return Result::failure(
                    ErrorCode::ActivationFailed,
                    "Failed to hash machine identity.");
            }

            status = BCryptFinishHash(
                hash,
                digest.data(),
                static_cast<ULONG>(digest.size()),
                0);

            BCryptDestroyHash(hash);
            BCryptCloseAlgorithmProvider(algorithm, 0);

            if (status < 0) {
                return Result::failure(
                    ErrorCode::ActivationFailed,
                    "Failed to finalize SHA-256 hash.");
            }

            return Result::successResult();
        }

        std::string toHex(
            const std::array<unsigned char, 32>& digest) {

            constexpr char hex[] = "0123456789abcdef";

            std::string result;
            result.reserve(digest.size() * 2);

            for (const auto byte : digest) {
                result.push_back(hex[(byte >> 4) & 0x0F]);
                result.push_back(hex[byte & 0x0F]);
            }

            return result;
        }

    } // namespace

    Result MachineIdentity::fingerprint(std::string& value) {
        value.clear();

        std::string machineGuid;

        auto result = readMachineGuid(machineGuid);

        if (!result.ok) {
            return result;
        }

        std::array<unsigned char, 32> digest{};

        result = sha256(machineGuid, digest);

        if (!result.ok) {
            return result;
        }

        value = toHex(digest);

        return Result::successResult();
    }

} // namespace KeygenSDK

#else

namespace KeygenSDK {

    Result MachineIdentity::fingerprint(std::string& value) {
        value.clear();

        return Result::failure(
            ErrorCode::ActivationFailed,
            "Machine fingerprint is only supported on Windows.");
    }

} // namespace KeygenSDK

#endif