
#pragma once

#include <string>
#include <string_view>

#include "KeygenSDK/Error.h"

namespace KeygenSDK {

    class MachineFileVerifier {
    public:
        [[nodiscard]] static Result verify(
            std::string_view machineFile,
            std::string_view accountPublicKey,
            std::string_view licenseKey,
            std::string& verifiedPayload);
    };

} // namespace KeygenSDK