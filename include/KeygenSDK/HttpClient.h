#pragma once

#include <string>
#include <string_view>

#include "KeygenSDK/Error.h"

namespace KeygenSDK {

    struct HttpResponse {
        long statusCode{ 0 };
        std::string body;
    };

    class IHttpClient {
        public:
            virtual ~IHttpClient() = default;

            [[nodiscard]] virtual Result get(
                std::string_view url,
                HttpResponse& response) const = 0;

            [[nodiscard]] virtual Result post(
                std::string_view url,
                std::string_view body,
                HttpResponse& response) const = 0;
    };

    class HttpClient : public IHttpClient {
        public:
            explicit HttpClient(long timeoutSeconds = 15);
            ~HttpClient();

            HttpClient(const HttpClient&) = delete;
            HttpClient& operator=(const HttpClient&) = delete;
            HttpClient(HttpClient&&) = delete;
            HttpClient& operator=(HttpClient&&) = delete;

            [[nodiscard]] Result get(
                std::string_view url,
                HttpResponse& response) const override;

            [[nodiscard]] Result post(
                std::string_view url,
                std::string_view body,
                HttpResponse& response) const override;

        private:
            long timeoutSeconds_;
    };
} // namespace KeygenSDK
