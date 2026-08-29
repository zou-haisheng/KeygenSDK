#pragma once

#include <string>

namespace KeygenSDK {

struct Config {
    std::string host;
    std::string accountId;
    long timeoutSeconds{15};
};

} // namespace KeygenSDK
