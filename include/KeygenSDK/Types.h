#pragma once

#include <filesystem>
#include <string>

namespace KeygenSDK {

    struct Config {
        std::string host;
        std::string accountId;
        long timeoutSeconds{ 15 };

        // Empty means use the default per-user local license path.
        std::filesystem::path localLicensePath;
    };

} // namespace KeygenSDK