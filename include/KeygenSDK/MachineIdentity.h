#pragma once

#include <string>

#include "KeygenSDK/Error.h"

namespace KeygenSDK {

    class MachineIdentity {
    public:
        [[nodiscard]]
        static Result fingerprint(std::string& value);
    };

} // namespace KeygenSDK