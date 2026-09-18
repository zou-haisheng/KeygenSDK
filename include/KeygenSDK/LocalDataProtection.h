#pragma once

#include <string>

#include "KeygenSDK/Error.h"

namespace KeygenSDK {

    class LocalDataProtection {
    public:
        [[nodiscard]] static Result protect(
            const std::string& plaintext,
            std::string& protectedData);

        [[nodiscard]] static Result unprotect(
            const std::string& protectedData,
            std::string& plaintext);
    };

} // namespace KeygenSDK