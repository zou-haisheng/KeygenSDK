#include "KeygenSDK/HttpClient.h"

#include <curl/curl.h>

#include <utility>

namespace KeygenSDK {
namespace {

size_t writeCallback(char* ptr, size_t size, size_t count, void* userdata) {
    const auto total = size * count;
    auto* body = static_cast<std::string*>(userdata);
    body->append(ptr, total);
    return total;
}

} // namespace

HttpClient::HttpClient(long timeoutSeconds) : timeoutSeconds_(timeoutSeconds) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

HttpClient::~HttpClient() {
    curl_global_cleanup();
}

Result HttpClient::get(std::string_view url, HttpResponse& response) const {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return Result::failure(ErrorCode::NetworkError, "Failed to initialize libcurl.");
    }

    response = {};
    std::string urlString(url);

    curl_easy_setopt(curl, CURLOPT_URL, urlString.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSeconds_);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeoutSeconds_);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    const CURLcode code = curl_easy_perform(curl);
    if (code != CURLE_OK) {
        const std::string message = curl_easy_strerror(code);
        curl_easy_cleanup(curl);
        return Result::failure(ErrorCode::NetworkError, message);
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.statusCode);
    curl_easy_cleanup(curl);

    if (response.statusCode >= 500) {
        return Result::failure(ErrorCode::ServerError, "HTTP server error.");
    }

    if (response.statusCode >= 400) {
        return Result::failure(ErrorCode::InvalidResponse, "HTTP request returned a client error.");
    }

    return Result::successResult();
}

Result HttpClient::post(
    std::string_view url,
    std::string_view body,
    HttpResponse& response) const {

    CURL* curl = curl_easy_init();
    if (!curl) {
        return Result::failure(
            ErrorCode::NetworkError,
            "Failed to initialize libcurl.");
    }

    response = {};

    std::string urlString(url);
    std::string bodyString(body);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(
        headers,
        "Content-Type: application/vnd.api+json");
    headers = curl_slist_append(
        headers,
        "Accept: application/vnd.api+json");

    curl_easy_setopt(curl, CURLOPT_URL, urlString.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyString.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
        static_cast<long>(bodyString.size()));

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSeconds_);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeoutSeconds_);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);

    // TLS certificate and hostname verification must remain enabled.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    const CURLcode code = curl_easy_perform(curl);

    if (code != CURLE_OK) {
        const std::string message = curl_easy_strerror(code);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return Result::failure(
            ErrorCode::NetworkError,
            message);
    }

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &response.statusCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (response.statusCode >= 500) {
        return Result::failure(
            ErrorCode::ServerError,
            "HTTP server error.");
    }

    if (response.statusCode >= 400) {
        return Result::failure(
            ErrorCode::InvalidResponse,
            "HTTP request returned a client error.");
    }

    return Result::successResult();
}

} // namespace KeygenSDK
