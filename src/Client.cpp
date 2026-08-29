#include "KeygenSDK/Client.h"

#include "KeygenSDK/HttpClient.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <stdexcept>

namespace KeygenSDK {

class Client::Impl {
public:
    explicit Impl(Config config)
        : config_(std::move(config)), http_(config_.timeoutSeconds) {}

    Config config_;
    HttpClient http_;
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
    return !host.empty() &&
           (host.rfind("https://", 0) == 0) &&
           !config.accountId.empty() &&
           config.timeoutSeconds > 0;
}

} // namespace

Client::Client(Config config)
    : impl_(new Impl(std::move(config))) {}

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
        return Result::failure(ErrorCode::InvalidConfiguration, "Client is not initialized.");
    }
    if (!validConfig(impl_->config_)) {
        return Result::failure(ErrorCode::InvalidConfiguration,
                               "host must use HTTPS, accountId must be set, and timeout must be positive.");
    }
    if (licenseKey.empty()) {
        return Result::failure(ErrorCode::InvalidLicense, "License key must not be empty.");
    }

    // Phase 1 deliberately stops before binding to a guessed Keygen API endpoint.
    // The HTTP layer is ready; endpoint/auth/JSON behavior belongs to Phase 2 after
    // verification against the actual Keygen CE 1.8 API documentation.
    return Result::failure(ErrorCode::Unknown,
                           "Online license validation is intentionally not implemented in Phase 1.");
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
