#pragma once

#include <string>
#include <string_view>

namespace KeygenSDK {

enum class ErrorCode {
    None = 0,
    InvalidConfiguration,
    NetworkError,
    InvalidLicense,
    LicenseExpired,
    LicenseSuspended,
    LicenseRevoked,
    LicenseOverdue,
    MachineLimitReached,
    ActivationFailed,
    OfflineDataMissing,
    OfflineVerificationFailed,
    InvalidResponse,
    ServerError,
    Unknown,
};

constexpr std::string_view toString(ErrorCode code) noexcept {
    switch (code) {
    case ErrorCode::None: return "none";
    case ErrorCode::InvalidConfiguration: return "invalid_configuration";
    case ErrorCode::NetworkError: return "network_error";
    case ErrorCode::InvalidLicense: return "invalid_license";
    case ErrorCode::LicenseExpired: return "license_expired";
    case ErrorCode::LicenseSuspended: return "license_suspended";
    case ErrorCode::LicenseOverdue: return "license_overdue";
    case ErrorCode::LicenseRevoked: return "license_revoked";
    case ErrorCode::MachineLimitReached: return "machine_limit_reached";
    case ErrorCode::ActivationFailed: return "activation_failed";
    case ErrorCode::OfflineDataMissing: return "offline_data_missing";
    case ErrorCode::OfflineVerificationFailed: return "offline_verification_failed";
    case ErrorCode::InvalidResponse: return "invalid_response";
    case ErrorCode::ServerError: return "server_error";
    case ErrorCode::Unknown: return "unknown";
    }
    return "unknown";
}

struct Result {
    bool ok{false};
    ErrorCode error{ErrorCode::Unknown};
    std::string message;

    [[nodiscard]] bool success() const noexcept { return ok; }
    explicit operator bool() const noexcept { return ok; }

    static Result successResult(std::string message = {}) {
        return {true, ErrorCode::None, std::move(message)};
    }

    static Result failure(ErrorCode error, std::string message) {
        return {false, error, std::move(message)};
    }
};

} // namespace KeygenSDK
