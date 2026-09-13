#pragma once

#include <string>
#include <string_view>

#include "KeygenSDK/Error.h"
#include "KeygenSDK/LocalLicenseState.h"

namespace KeygenSDK {

    class LocalLicenseSerializer {
    public:
        [[nodiscard]] static Result serialize(
            const LocalLicenseState& state,
            std::string& output);

        [[nodiscard]] static Result deserialize(
            std::string_view input,
            LocalLicenseState& state);
    };

} // namespace KeygenSDK