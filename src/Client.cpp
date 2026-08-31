#include "KeygenSDK/Client.h"

#include "KeygenSDK/HttpClient.h"

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

    Result Client::validateOnline(const std::string& licenseKey) {
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

        if (licenseKey.empty()) {
            return Result::failure(
                ErrorCode::InvalidLicense,
                "License key must not be empty.");
        }

        const auto url = buildValidationUrl(impl_->config_);

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
            impl_->httpClient_->post(url, requestBody, response);

        if (!httpResult.ok) {
            return httpResult;
        }

        if (response.statusCode >= 500) {
            return Result::failure(
                ErrorCode::ServerError,
                "Keygen server returned a server error.");
        }

        try {
            const auto json = nlohmann::json::parse(response.body);

            if (!json.contains("meta") || !json["meta"].is_object()) {
                return Result::failure(
                    ErrorCode::InvalidResponse,
                    "Keygen response is missing meta.");
            }

            const auto& meta = json["meta"];

            if (!meta.contains("valid") || !meta["valid"].is_boolean()) {
                return Result::failure(
                    ErrorCode::InvalidResponse,
                    "Keygen response is missing meta.valid.");
            }

            ErrorCode mappedError = ErrorCode::Unknown;
            const std::string mappedMessage =
                mapValidationCode(meta, mappedError);

            if (mappedError == ErrorCode::None) {
                if (!meta["valid"].get<bool>()) {
                    return Result::failure(
                        ErrorCode::InvalidLicense,
                        "License validation failed.");
                }

                return Result::successResult(
                    "License validation succeeded.");
            }

            if (mappedError == ErrorCode::InvalidResponse) {
                return Result::failure(
                    mappedError,
                    mappedMessage);
            }

            return Result::failure(
                mappedError,
                mappedMessage);
        }
        catch (const nlohmann::json::exception&) {
            return Result::failure(
                ErrorCode::InvalidResponse,
                "Keygen server returned invalid JSON.");
        }
    }

    Result Client::activate(const std::string& licenseKey) {
        if (!impl_) {
            return Result::failure(ErrorCode::InvalidConfiguration, "Client is not initialized.");
        }
        if (licenseKey.empty()) {
            return Result::failure(ErrorCode::InvalidLicense, "License key must not be empty.");
        }
        return Result::failure(ErrorCode::Unknown,
                               "License activation is intentionally not implemented in Phase 1.");
    }

    Result Client::verifyOffline() {
        return Result::failure(ErrorCode::OfflineDataMissing,
                               "Offline licensing is intentionally not implemented in Phase 1.");
    }

    Result Client::deactivate() {
        return Result::failure(ErrorCode::Unknown,
                               "Deactivation is intentionally not implemented in Phase 1.");
    }

    bool Client::hasLocalLicense() const noexcept {
        return false;
    }

} // namespace KeygenSDK
