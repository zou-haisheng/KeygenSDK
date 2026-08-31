#include <KeygenSDK/Client.h>
#include <KeygenSDK/HttpClient.h>

#include <cassert>
#include <string>
#include <utility>

namespace KeygenSDK {

    class ClientTestAccess {
    public:
        static Client create(
            Config config,
            IHttpClient& httpClient) {

            return Client(std::move(config), httpClient);
        }
    };

} // namespace KeygenSDK

namespace {

    class FakeHttpClient final : public KeygenSDK::IHttpClient {
    public:
        KeygenSDK::Result post(
            std::string_view url,
            std::string_view body,
            KeygenSDK::HttpResponse& response) const override {

            lastUrl = std::string(url);
            lastBody = std::string(body);

            response.statusCode = statusCode;
            response.body = responseBody;

            return result;
        }

        KeygenSDK::Result get(
            std::string_view,
            KeygenSDK::HttpResponse&) const override {

            return KeygenSDK::Result::failure(
                KeygenSDK::ErrorCode::Unknown,
                "Fake GET is not implemented.");
        }

        mutable std::string lastUrl;
        mutable std::string lastBody;

        long statusCode{ 200 };

        std::string responseBody;

        KeygenSDK::Result result =
            KeygenSDK::Result::successResult();
    };

    KeygenSDK::Config testConfig() {
        return {
            .host = "https://example.invalid",
            .accountId = "test-account",
            .timeoutSeconds = 5,
        };
    }

    void testValidLicense() {
        FakeHttpClient http;

        http.responseBody = R"({
        "meta": {
            "valid": true,
            "code": "VALID"
        }
    })";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto result =
            client.validateOnline("TEST-LICENSE-KEY");

        assert(result.ok);
        assert(result.error == KeygenSDK::ErrorCode::None);

        assert(
            http.lastUrl ==
            "https://example.invalid/v1/accounts/test-account/licenses/actions/validate-key");

        assert(
            http.lastBody ==
            R"({"meta":{"key":"TEST-LICENSE-KEY"}})");
    }

    void testExpiredLicense() {
        FakeHttpClient http;

        http.responseBody = R"({
        "meta": {
            "valid": false,
            "code": "EXPIRED"
        }
    })";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto result =
            client.validateOnline("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::LicenseExpired);
    }

    void testSuspendedLicense() {
        FakeHttpClient http;

        http.responseBody = R"({
        "meta": {
            "valid": false,
            "code": "SUSPENDED"
        }
    })";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto result =
            client.validateOnline("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::LicenseSuspended);
    }

    void testOverdueLicense() {
        FakeHttpClient http;

        http.responseBody = R"({
        "meta": {
            "valid": false,
            "code": "OVERDUE"
        }
    })";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto result =
            client.validateOnline("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::LicenseOverdue);
    }

    void testServerError() {
        FakeHttpClient http;

        http.statusCode = 500;
        http.responseBody = R"({
        "meta": {
            "valid": false,
            "code": "SERVER_ERROR"
        }
    })";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto result =
            client.validateOnline("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::ServerError);
    }

    void testInvalidJson() {
        FakeHttpClient http;

        http.responseBody = "this is not valid json";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto result =
            client.validateOnline("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::InvalidResponse);
    }

    void testMissingMeta() {
        FakeHttpClient http;

        http.responseBody = R"({
        "data": {}
    })";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto result =
            client.validateOnline("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::InvalidResponse);
    }

    void testMissingValid() {
        FakeHttpClient http;

        http.responseBody = R"({
        "meta": {
            "code": "VALID"
        }
    })";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto result =
            client.validateOnline("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::InvalidResponse);
    }

    void testMissingCode() {
        FakeHttpClient http;

        http.responseBody = R"({
        "meta": {
            "valid": true
        }
    })";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto result =
            client.validateOnline("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::InvalidResponse);
    }

    void testHttpClientFailure() {
        FakeHttpClient http;

        http.result = KeygenSDK::Result::failure(
            KeygenSDK::ErrorCode::NetworkError,
            "simulated network failure");

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto result =
            client.validateOnline("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::NetworkError);
    }

    void testHttp4xx() {
        FakeHttpClient http;

        http.statusCode = 400;
        http.responseBody = R"({
        "meta": {
            "valid": false,
            "code": "INVALID"
        }
    })";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto result =
            client.validateOnline("TEST-LICENSE-KEY");

        assert(!result.ok);
    }
    void testKeygenErrorResponse() {
        FakeHttpClient http;

        http.statusCode = 404;

        http.responseBody = R"({
        "errors": [
            {
                "title": "Not Found",
                "detail": "The requested resource was not found.",
                "code": "NOT_FOUND"
            }
        ]
    })";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto result =
            client.validateOnline("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::InvalidResponse);
    }

    void testInvalidHost() {
        KeygenSDK::Client client({
            .host = "http://example.invalid",
            .accountId = "test-account",
            .timeoutSeconds = 5,
            });

        const auto result =
            client.validateOnline("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::InvalidConfiguration);
    }

    void testEmptyHttpsHost() {
        KeygenSDK::Client client({
            .host = "https://",
            .accountId = "test-account",
            .timeoutSeconds = 5,
            });

        const auto result =
            client.validateOnline("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::InvalidConfiguration);
    }

} // namespace

int main() {
    using namespace KeygenSDK;

    static_assert(
        toString(ErrorCode::NetworkError) ==
        "network_error");

    const Result success =
        Result::successResult("ok");

    assert(success.ok);
    assert(success.success());
    assert(success.error == ErrorCode::None);

    const Result failure =
        Result::failure(
            ErrorCode::InvalidLicense,
            "bad key");

    assert(!failure.ok);
    assert(
        failure.error ==
        ErrorCode::InvalidLicense);

    assert(failure.message == "bad key");

    Client client({
        .host = "https://example.invalid",
        .accountId = "account",
        });

    const Result emptyKey =
        client.validateOnline("");

    assert(!emptyKey.ok);
    assert(
        emptyKey.error ==
        ErrorCode::InvalidLicense);

    assert(!client.hasLocalLicense());

    testValidLicense();
    testExpiredLicense();
    testSuspendedLicense();
    testOverdueLicense();
    testServerError();
    testInvalidJson();
    testMissingMeta();
    testMissingValid();
    testMissingCode();
    testHttpClientFailure();
    testHttp4xx();
    testKeygenErrorResponse();
    testInvalidHost();
    testEmptyHttpsHost();

    return 0;
}