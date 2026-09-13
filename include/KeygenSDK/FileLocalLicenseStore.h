#pragma once

#include <filesystem>

#include "KeygenSDK/LocalLicenseStore.h"

namespace KeygenSDK {

    class FileLocalLicenseStore final : public ILocalLicenseStore {
    public:
        explicit FileLocalLicenseStore(
            std::filesystem::path path);

        [[nodiscard]] Result load(
            LocalLicenseState& state) const override;

        [[nodiscard]] Result save(
            const LocalLicenseState& state) override;

        [[nodiscard]] Result remove() override;

    private:
        std::filesystem::path path_;
    };

} // namespace KeygenSDK