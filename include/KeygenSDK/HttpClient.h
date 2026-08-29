#pragma once

#include <string>
#include <string_view>

#include "KeygenSDK/Error.h"

namespace KeygenSDK {

struct HttpResponse {
    long statusCode{0};
    std::string body;
};

class HttpClient {
public:
    explicit HttpClient(long timeoutSeconds = 15);
    ~HttpClient();

    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient(HttpClient&&) = delete;
    HttpClient& operator=(HttpClient&&) = delete;

    [[nodiscard]] Result get(std::string_view url, HttpResponse& response) const;

private:
    long timeoutSeconds_;
};

} // namespace KeygenSDK
