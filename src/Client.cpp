#include "KeygenSDK/Client.h"

#include "KeygenSDK/HttpClient.h"

#include "KeygenSDK/MachineIdentity.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <memory>
#include <stdexcept>

namespace KeygenSDK {

    class Client::Impl {
        public:
            explicit Impl(Config config)
                : config_(std::move(config)),
                http_(config_.timeoutSeconds),
                httpClient_(&http_) {}

            Impl(Config config, IHttpClient& httpClient)
                : config_(std::move(config)),
                http_(config_.timeoutSeconds),
                httpClient_(&httpClient) {}

            Config config_;
            HttpClient http_;
            IHttpClient* httpClient_;
            bool hasLocalLicense{ false };
            std::string machineId;
            std::string licenseKey;
    };

    namespace {

        std::string normalizeHost(std::string host) {
            while (!host.empty() && std::isspace(static_cast<unsigned char>(host.back()))) {
                host.pop_back();
            }
            while (!host.empty() && std::isspace(static_cast<unsigned char>(host.front()))) {
                host.erase(host.begin());
            }
            while (!host.empty() && host.back() == '/') {
                host.pop_back();
            }
            return host;
        }

        bool validConfig(const Config& config) {
            const auto host = normalizeHost(config.host);

            if (host.empty() ||
                host.rfind("https://", 0) != 0) {
                return false;
            }

            const auto authority = host.substr(8);

            return !authority.empty() &&
                authority.find('/') == std::string::npos &&
                !config.accountId.empty() &&
                config.timeoutSeconds > 0;
        }

        std::string buildValidationUrl(const Config& config) {
            const auto host = normalizeHost(config.host);

            return host +
                "/v1/accounts/" +
                config.accountId +
                "/licenses/actions/validate-key";
        }

        std::string mapValidationCode(
            const nlohmann::json& meta,
            ErrorCode& errorCode) {

            if (!meta.contains("code") || !meta["code"].is_string()) {
                errorCode = ErrorCode::InvalidResponse;
                return "Keygen validation response is missing a valid code.";
            }

            const std::string code = meta["code"].get<std::string>();

            if (code == "EXPIRED") {
                errorCode = ErrorCode::LicenseExpired;
                return "License has expired.";
            }

            if (code == "SUSPENDED") {
                errorCode = ErrorCode::LicenseSuspended;
                return "License is suspended.";
            }

            if (code == "VALID") {
                errorCode = ErrorCode::None;
                return {};
            }

            if (code == "OVERDUE") {
                errorCode = ErrorCode::LicenseOverdue;
                return "License check-in is overdue.";
            }

            if (code == "NO_MACHINE" ||
                code == "NO_MACHINES" ||
                code == "TOO_MANY_MACHINES" ||
                code == "TOO_MANY_CORES" ||
                code == "TOO_MUCH_MEMORY" ||
                code == "TOO_MUCH_DISK" ||
                code == "TOO_MANY_PROCESSES" ||
                code == "TOO_MANY_USERS") {

                errorCode = ErrorCode::InvalidLicense;
                return "License validation failed.";
            }

            errorCode = ErrorCode::InvalidLicense;
            return "License validation failed.";
        }

        struct ValidationResult {
            Result result;
            std::string licenseId;
        };

        ValidationResult validateLicense(
            IHttpClient& httpClient,
            const Config& config,
            const std::string& licenseKey) {

            if (licenseKey.empty()) {
                return {
                    Result::failure(
                        ErrorCode::InvalidLicense,
                        "License key must not be empty."),
                    {}
                };
            }

            const auto url = buildValidationUrl(config);

            nlohmann::json requestJson = {
                {
                    "meta",
                    {
                        {"key", licenseKey}
                    }
                }
            };

            const std::string requestBody = requestJson.dump();

            HttpResponse response;

            const Result httpResult =
                httpClient.post(url, requestBody, response);

            if (!httpResult.ok) {
                return { httpResult, {} };
            }

            if (response.statusCode >= 500) {
                return {
                    Result::failure(
                        ErrorCode::ServerError,
                        "Keygen server returned a server error."),
                    {}
                };
            }

            try {
                const auto json =
                    nlohmann::json::parse(response.body);

                if (!json.contains("meta") ||
                    !json["meta"].is_object()) {

                    return {
                        Result::failure(
                            ErrorCode::InvalidResponse,
                            "Keygen response is missing meta."),
                        {}
                    };
                }

                const auto& meta = json["meta"];

                if (!meta.contains("valid") ||
                    !meta["valid"].is_boolean()) {

                    return {
                        Result::failure(
                            ErrorCode::InvalidResponse,
                            "Keygen response is missing meta.valid."),
                        {}
                    };
                }

                ErrorCode mappedError = ErrorCode::Unknown;

                const std::string mappedMessage =
                    mapValidationCode(meta, mappedError);

                if (mappedError != ErrorCode::None) {
                    return {
                        Result::failure(
                            mappedError,
                            mappedMessage),
                        {}
                    };
                }

                if (!meta["valid"].get<bool>()) {
                    return {
                        Result::failure(
                            ErrorCode::InvalidLicense,
                            "License validation failed."),
                        {}
                    };
                }

                if (!json.contains("data") ||
                    !json["data"].is_object()) {

                    return {
                        Result::failure(
                            ErrorCode::InvalidResponse,
                            "Keygen validation response is missing data."),
                        {}
                    };
                }

                const auto& data = json["data"];

                if (!data.contains("id") ||
                    !data["id"].is_string() ||
                    data["id"].get<std::string>().empty()) {

                    return {
                        Result::failure(
                            ErrorCode::InvalidResponse,
                            "Keygen validation response is missing license id."),
                        {}
                    };
                }

                return {
                    Result::successResult(
                        "License validation succeeded."),
                    data["id"].get<std::string>()
                };
            }
            catch (const nlohmann::json::exception&) {
                return {
                    Result::failure(
                        ErrorCode::InvalidResponse,
                        "Keygen server returned invalid JSON."),
                    {}
                };
            }
        }

        // Machine URL helper
        std::string buildMachineUrl(const Config& config) {
            const auto host = normalizeHost(config.host);

            return host +
                "/v1/accounts/" +
                config.accountId +
                "/machines";
        }

        // activation request helper
        struct ActivationResult {
            Result result;
            std::string machineId;
        };

        ActivationResult createMachine(
            IHttpClient& httpClient,
            const Config& config,
            const std::string& licenseKey,
            const std::string& licenseId,
            const std::string& fingerprint) {

            nlohmann::json requestJson = {
                {
                    "data",
                    {
                        {"type", "machines"},
                        {
                            "attributes",
                            {
                                {"fingerprint", fingerprint}
                            }
                        },
                        {
                            "relationships",
                            {
                                {
                                    "license",
                                    {
                                        {
                                            "data",
                                            {
                                                {"type", "licenses"},
                                                {"id", licenseId}
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            };

            HttpResponse response;

            const HttpHeaders headers{
                "Authorization: License " + licenseKey
            };

            const Result httpResult =
                httpClient.post(
                    buildMachineUrl(config),
                    requestJson.dump(),
                    response,
                    headers);

            if (!httpResult.ok) {
                return { httpResult, {} };
            }

            if (response.statusCode >= 500) {
                return {
                    Result::failure(
                        ErrorCode::ServerError,
                        "Keygen server returned a server error."),
                    {}
                };
            }

            if (response.statusCode < 200 ||
                response.statusCode >= 300) {

                return {
                    Result::failure(
                        ErrorCode::ActivationFailed,
                        "Keygen machine activation request failed."),
                    {}
                };
            }

            try {
                const auto json =
                    nlohmann::json::parse(response.body);

                if (!json.contains("data") ||
                    !json["data"].is_object()) {

                    return {
                        Result::failure(
                            ErrorCode::InvalidResponse,
                            "Keygen machine response is missing data."),
                        {}
                    };
                }

                const auto& data = json["data"];

                if (!data.contains("id") ||
                    !data["id"].is_string() ||
                    data["id"].get<std::string>().empty()) {

                    return {
                        Result::failure(
                            ErrorCode::InvalidResponse,
                            "Keygen machine response is missing machine id."),
                        {}
                    };
                }

                if (!data.contains("type") ||
                    !data["type"].is_string() ||
                    data["type"].get<std::string>() != "machines") {

                    return {
                        Result::failure(
                            ErrorCode::InvalidResponse,
                            "Keygen machine response has an invalid type."),
                        {}
                    };
                }

                if (!data.contains("attributes") ||
                    !data["attributes"].is_object()) {

                    return {
                        Result::failure(
                            ErrorCode::InvalidResponse,
                            "Keygen machine response is missing attributes."),
                        {}
                    };
                }

                const auto& attributes = data["attributes"];

                if (!attributes.contains("fingerprint") ||
                    !attributes["fingerprint"].is_string() ||
                    attributes["fingerprint"].get<std::string>().empty()) {

                    return {
                        Result::failure(
                            ErrorCode::InvalidResponse,
                            "Keygen machine response is missing fingerprint."),
                        {}
                    };
                }

                return {
                    Result::successResult(
                        "Machine activation succeeded."),
                    data["id"].get<std::string>()
                };
            }
            catch (const nlohmann::json::exception&) {
                return {
                    Result::failure(
                        ErrorCode::InvalidResponse,
                        "Keygen server returned invalid JSON."),
                    {}
                };
            }
        }

    } // namespace

    Client::Client(Config config)
        : impl_(new Impl(std::move(config))) {}

    Client::Client(Config config, IHttpClient& httpClient)
        : impl_(new Impl(std::move(config), httpClient)) {}

    Client::~Client() {
        delete impl_;
    }

    Client::Client(Client&& other) noexcept : impl_(other.impl_) {
        other.impl_ = nullptr;
    }

    Client& Client::operator=(Client&& other) noexcept {
        if (this != &other) {
            delete impl_;
            impl_ = other.impl_;
            other.impl_ = nullptr;
        }
        return *this;
    }

    Result Client::validateOnline(
        const std::string& licenseKey) {

        if (!impl_) {
            return Result::failure(
                ErrorCode::InvalidConfiguration,
                "Client is not initialized.");
        }

        if (!validConfig(impl_->config_)) {
            return Result::failure(
                ErrorCode::InvalidConfiguration,
                "host must use HTTPS, accountId must be set, and timeout must be positive.");
        }

        return validateLicense(
            *impl_->httpClient_,
            impl_->config_,
            licenseKey).result;
    }

    Result Client::activate(
        const std::string& licenseKey) {

        if (!impl_) {
            return Result::failure(
                ErrorCode::InvalidConfiguration,
                "Client is not initialized.");
        }

        if (!validConfig(impl_->config_)) {
            return Result::failure(
                ErrorCode::InvalidConfiguration,
                "host must use HTTPS, accountId must be set, and timeout must be positive.");
        }

        const auto validation =
            validateLicense(
                *impl_->httpClient_,
                impl_->config_,
                licenseKey);

        if (!validation.result.ok) {
            return validation.result;
        }

        std::string fingerprint;

        const Result fingerprintResult =
            MachineIdentity::fingerprint(fingerprint);

        if (!fingerprintResult.ok) {
            return fingerprintResult;
        }

        const auto activation =
            createMachine(
                *impl_->httpClient_,
                impl_->config_,
                licenseKey,
                validation.licenseId,
                fingerprint);

        if (!activation.result.ok) {
            return activation.result;
        }

        impl_->machineId = activation.machineId;
        impl_->licenseKey = licenseKey;
        impl_->hasLocalLicense = true;

        return activation.result;
    }

    Result Client::verifyOffline() {
        return Result::failure(
            ErrorCode::OfflineDataMissing,
            "Offline licensing is not implemented yet.");
    }

    Result Client::deactivate() {
        if (!impl_->hasLocalLicense) {
            return Result::failure(
                ErrorCode::InvalidLicense,
                "No active local license.");
        }

        if (impl_->machineId.empty()) {
            return Result::failure(
                ErrorCode::InvalidResponse,
                "Local machine ID is missing.");
        }

        if (impl_->licenseKey.empty()) {
            return Result::failure(
                ErrorCode::InvalidLicense,
                "Local license key is missing.");
        }

        const std::string url =
            buildMachineUrl(impl_->config_) + "/" + impl_->machineId;

        HttpResponse response;

        const HttpHeaders headers = {
            "Authorization: License " + impl_->licenseKey
        };

        const Result httpResult =
            impl_->httpClient_->deleteResource(
                url,
                response,
                headers);

        if (!httpResult.ok) {
            return httpResult;
        }

        if (response.statusCode != 204) {
            return Result::failure(
                ErrorCode::InvalidResponse,
                "Unexpected machine deletion response.");
        }

        impl_->machineId.clear();
        impl_->licenseKey.clear();
        impl_->hasLocalLicense = false;

        return Result::successResult(
            "Machine deactivation succeeded.");
    }

    bool Client::hasLocalLicense() const noexcept {
        return impl_->hasLocalLicense;
    }

} // namespace KeygenSDK
