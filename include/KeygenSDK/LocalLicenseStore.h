#pragma once

#include "KeygenSDK/Error.h"
#include "KeygenSDK/LocalLicenseState.h"

namespace KeygenSDK {

    class ILocalLicenseStore {
    public:
        virtual ~ILocalLicenseStore() = default;

        [[nodiscard]] virtual Result load(
            LocalLicenseState& state) const = 0;

        [[nodiscard]] virtual Result save(
            const LocalLicenseState& state) = 0;

        [[nodiscard]] virtual Result remove() = 0;
    };

} // namespace KeygenSDK