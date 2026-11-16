#pragma once

#include <memory>
#include <string>

#include "KeygenSDK/Error.h"
#include "KeygenSDK/Types.h"

namespace KeygenSDK {

    class IHttpClient;
    class ILocalLicenseStore;

    class Client {
        public:
            explicit Client(Config config);
            ~Client();

            Client(const Client&) = delete;
            Client& operator=(const Client&) = delete;
            Client(Client&&) noexcept;
            Client& operator=(Client&&) noexcept;

            [[nodiscard]] Result validateOnline(const std::string& licenseKey);
            [[nodiscard]] Result activate(const std::string& licenseKey);
            [[nodiscard]] Result verifyOffline();
            [[nodiscard]] Result deactivate();
            [[nodiscard]] Result loadLocalLicense();

            [[nodiscard]] bool hasLocalLicense() const noexcept;
            [[nodiscard]] std::string exportMachineFile() const;
            [[nodiscard]] Result importMachineFile(const std::string& machineFile);
            [[nodiscard]] std::string licenseId() const;
            [[nodiscard]] std::string licenseExpiry() const;

        private:
            class Impl;

            explicit Client(Config config, IHttpClient& httpClient);

            Client(
                Config config,
                IHttpClient& httpClient,
                std::unique_ptr<ILocalLicenseStore> localStore);

            friend class ClientTestAccess;

            Impl* impl_;
    };

} // namespace KeygenSDK
