#pragma once

#include <string>

#include "KeygenSDK/Error.h"
#include "KeygenSDK/Types.h"

namespace KeygenSDK {

    class IHttpClient;

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

            [[nodiscard]] bool hasLocalLicense() const noexcept;

        private:
            class Impl;

            explicit Client(Config config, IHttpClient& httpClient);

            friend class ClientTestAccess;

            Impl* impl_;
    };

} // namespace KeygenSDK
