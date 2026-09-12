#pragma once

#include <string>

namespace KeygenSDK {

    struct LocalLicenseState {
        std::string licenseId;
        std::string licenseKey;
        std::string machineId;
        std::string machineFingerprint;

        [[nodiscard]] bool isValid() const noexcept {
            return !licenseId.empty() &&
                !licenseKey.empty() &&
                !machineId.empty() &&
                !machineFingerprint.empty();
        }
    };

} // namespace KeygenSDK