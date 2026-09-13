#include "KeygenSDK/LocalLicenseSerialization.h"

#include <nlohmann/json.hpp>
#include <utility>

namespace KeygenSDK {

    namespace {

        constexpr int kCurrentVersion = 1;

    } // namespace

    Result LocalLicenseSerializer::serialize(
        const LocalLicenseState& state,
        std::string& output) {

        if (!state.isValid()) {
            return Result::failure(
                ErrorCode::InvalidResponse,
                "Local license state is invalid.");
        }

        try {
            const nlohmann::json json = {
                {"version", kCurrentVersion},
                {
                    "license",
                    {
                        {"id", state.licenseId},
                        {"key", state.licenseKey}
                    }
                },
                {
                    "machine",
                    {
                        {"id", state.machineId},
                        {"fingerprint", state.machineFingerprint}
                    }
                }
            };

            output = json.dump();

            return Result::successResult(
                "Local license state serialized successfully.");
        }
        catch (const nlohmann::json::exception&) {
            return Result::failure(
                ErrorCode::LocalStorageError,
                "Failed to serialize local license state.");
        }
    }

    Result LocalLicenseSerializer::deserialize(
        std::string_view input,
        LocalLicenseState& state) {

        try {
            const auto json =
                nlohmann::json::parse(input);

            if (!json.is_object()) {
                return Result::failure(
                    ErrorCode::InvalidResponse,
                    "Local license state must be a JSON object.");
            }

            if (!json.contains("version") ||
                !json["version"].is_number_integer()) {

                return Result::failure(
                    ErrorCode::InvalidResponse,
                    "Local license state is missing a valid version.");
            }

            const int version =
                json["version"].get<int>();

            if (version != kCurrentVersion) {
                return Result::failure(
                    ErrorCode::InvalidResponse,
                    "Unsupported local license state version.");
            }

            if (!json.contains("license") ||
                !json["license"].is_object()) {

                return Result::failure(
                    ErrorCode::InvalidResponse,
                    "Local license state is missing license.");
            }

            if (!json.contains("machine") ||
                !json["machine"].is_object()) {

                return Result::failure(
                    ErrorCode::InvalidResponse,
                    "Local license state is missing machine.");
            }

            const auto& license = json["license"];
            const auto& machine = json["machine"];

            if (!license.contains("id") ||
                !license["id"].is_string()) {

                return Result::failure(
                    ErrorCode::InvalidResponse,
                    "Local license state is missing license id.");
            }

            if (!license.contains("key") ||
                !license["key"].is_string()) {

                return Result::failure(
                    ErrorCode::InvalidResponse,
                    "Local license state is missing license key.");
            }

            if (!machine.contains("id") ||
                !machine["id"].is_string()) {

                return Result::failure(
                    ErrorCode::InvalidResponse,
                    "Local license state is missing machine id.");
            }

            if (!machine.contains("fingerprint") ||
                !machine["fingerprint"].is_string()) {

                return Result::failure(
                    ErrorCode::InvalidResponse,
                    "Local license state is missing machine fingerprint.");
            }

            LocalLicenseState parsedState{
                .licenseId = license["id"].get<std::string>(),
                .licenseKey = license["key"].get<std::string>(),
                .machineId = machine["id"].get<std::string>(),
                .machineFingerprint = machine["fingerprint"].get<std::string>()
            };

            if (!parsedState.isValid()) {
                return Result::failure(
                    ErrorCode::InvalidResponse,
                    "Local license state contains empty required fields.");
            }

            state = std::move(parsedState);

            return Result::successResult(
                "Local license state deserialized successfully.");
        }
        catch (const nlohmann::json::exception&) {
            return Result::failure(
                ErrorCode::InvalidResponse,
                "Local license state contains invalid JSON.");
        }
    }

} // namespace KeygenSDK