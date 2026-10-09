#pragma once

#include <filesystem>
#include <string>

namespace KeygenSDK {

    struct Config {
        std::string host;
        std::string accountId;

        // Trusted Keygen account public key, encoded as hexadecimal.
        std::string accountPublicKey;

        long timeoutSeconds{ 15 };

        // Empty means use the default per-user local license path.
        std::filesystem::path localLicensePath;
    };

} // namespace KeygenSDK