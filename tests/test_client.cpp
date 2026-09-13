#include <KeygenSDK/Client.h>
#include <KeygenSDK/HttpClient.h>
#include <KeygenSDK/MachineIdentity.h>
#include <KeygenSDK/LocalLicenseState.h>
#include <KeygenSDK/LocalLicenseStore.h>

#include <cassert>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>
#include <iostream>

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

namespace KeygenSDKTests {

    void testLocalLicenseStoreSaveAndLoad();
    void testLocalLicenseStoreLoadMissingState();
    void testLocalLicenseStoreRemove();

    void testLocalLicenseSerializationRoundTrip();
    void testLocalLicenseSerializationRejectsInvalidState();
    void testLocalLicenseDeserializationRejectsMalformedJson();
    void testLocalLicenseDeserializationRejectsMissingVersion();
    void testLocalLicenseDeserializationRejectsUnsupportedVersion();
    void testLocalLicenseDeserializationRejectsMissingLicense();
    void testLocalLicenseDeserializationRejectsMissingMachine();
    void testLocalLicenseDeserializationRejectsMissingRequiredField();
    void testLocalLicenseDeserializationRejectsInvalidFieldType();
    void testLocalLicenseDeserializationRejectsEmptyRequiredField();
    void testLocalLicenseDeserializationAllowsUnknownFields();

    void testFileLocalLicenseStoreSaveAndLoad();
    void testFileLocalLicenseStoreLoadMissingFile();
    void testFileLocalLicenseStoreRemove();
    void testFileLocalLicenseStoreMalformedJson();

}

namespace {

    class FakeHttpClient final : public KeygenSDK::IHttpClient {
    public:
        KeygenSDK::Result post(
            std::string_view url,
            std::string_view body,
            KeygenSDK::HttpResponse& response,
            const KeygenSDK::HttpHeaders& headers) const override {

            ++postCallCount;

            lastUrl = std::string(url);
            lastBody = std::string(body);
            lastHeaders = headers;

            response.statusCode = statusCode;
            response.body = responseBody;

            if (postCallCount == 1 && !validationResponseBody.empty()) {
                response.statusCode = validationStatusCode;
                response.body = validationResponseBody;
                return validationResult;
            }
            else if (postCallCount >= 2 && !activationResponseBody.empty()) {
                response.statusCode = activationStatusCode;
                response.body = activationResponseBody;
                return activationResult;
            }
            else {
                response.statusCode = statusCode;
                response.body = responseBody;
                return result;
            }
        }

        KeygenSDK::Result get(
            std::string_view,
            KeygenSDK::HttpResponse&) const override {

            return KeygenSDK::Result::failure(
                KeygenSDK::ErrorCode::Unknown,
                "Fake GET is not implemented.");
        }

        KeygenSDK::Result deleteResource(
            std::string_view url,
            KeygenSDK::HttpResponse& response,
            const KeygenSDK::HttpHeaders& headers = {}) const override {

            lastDeleteUrl = std::string(url);
            lastDeleteHeaders = headers;
            ++deleteCallCount;

            response.statusCode = deleteStatusCode;
            response.body = deleteResponseBody;

            return deleteResult;
        }

        mutable std::string lastUrl;
        mutable std::string lastBody;
        mutable KeygenSDK::HttpHeaders lastHeaders;
        mutable int postCallCount{ 0 };
        mutable int deleteCallCount{ 0 };
        mutable std::string lastDeleteUrl;
        mutable KeygenSDK::HttpHeaders lastDeleteHeaders;

        long statusCode{ 200 };

        std::string responseBody;

        long validationStatusCode{ 200 };
        long activationStatusCode{ 201 };
        long deleteStatusCode{ 204 };
        
        std::string validationResponseBody;
        std::string activationResponseBody;
        std::string deleteResponseBody;
        
        KeygenSDK::Result result =
            KeygenSDK::Result::successResult();

        KeygenSDK::Result validationResult =
            KeygenSDK::Result::successResult();

        KeygenSDK::Result activationResult =
            KeygenSDK::Result::successResult();

        KeygenSDK::Result deleteResult = KeygenSDK::Result::successResult();
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
            },
            "data": {
                "id": "test-license-id",
                "type": "licenses"
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

    void testMissingLicenseId() {
        FakeHttpClient http;

        http.responseBody = R"({
        "meta": {
            "valid": true,
            "code": "VALID"
        },
        "data": {
            "type": "licenses"
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

    // Activation Success Test
    void testActivationSuccess() {
        FakeHttpClient http;

        http.validationResponseBody = R"({
        "meta": {
            "valid": true,
            "code": "VALID"
        },
        "data": {
            "id": "test-license-id",
            "type": "licenses"
        }
    })";

        http.activationResponseBody = R"({
        "data": {
            "id": "test-machine-id",
            "type": "machines",
            "attributes": {
                "fingerprint": "test-fingerprint"
            }
        }
    })";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);
        const auto result =
            client.activate("TEST-LICENSE-KEY");
        // 测试代码
        if (!result.ok) {
            std::cerr
                << "Activation failed. error="
                << static_cast<int>(result.error)
                << ", message="
                << result.message
                << ", postCallCount="
                << http.postCallCount
                << std::endl;
        }

        assert(result.ok);
        assert(result.error == KeygenSDK::ErrorCode::None);

        assert(http.postCallCount == 2);

        assert(
            http.lastUrl ==
            "https://example.invalid/v1/accounts/test-account/machines");

        assert(
            http.lastHeaders.size() == 1);

        assert(
            http.lastHeaders[0] ==
            "Authorization: License TEST-LICENSE-KEY");

        const auto request =
            nlohmann::json::parse(http.lastBody);

        assert(request.contains("data"));
        assert(request["data"].is_object());

        const auto& data = request["data"];

        assert(
            data["type"] == "machines");

        assert(
            data["attributes"].is_object());

        assert(
            data["attributes"]["fingerprint"].is_string());

        assert(
            !data["attributes"]["fingerprint"]
            .get<std::string>()
            .empty());

        assert(
            data["relationships"].is_object());

        assert(
            data["relationships"]["license"].is_object());

        const auto& license =
            data["relationships"]["license"]["data"];

        assert(
            license["type"] == "licenses");

        assert(
            license["id"] == "test-license-id");

        assert(
            http.lastBody.find("TEST-LICENSE-KEY") ==
            std::string::npos);
    }

    void testActivationStopsWhenLicenseIsInvalid() {
        FakeHttpClient http;

        http.validationResponseBody = R"({
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
            client.activate("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::InvalidLicense);

        // License validation failed, so machine creation must not happen.
        assert(http.postCallCount == 1);
    }

    void testActivationHttpFailure() {
        FakeHttpClient http;

        http.validationResponseBody = R"({
        "meta": {
            "valid": true,
            "code": "VALID"
        },
        "data": {
            "id": "test-license-id",
            "type": "licenses"
        }
    })";

        http.activationStatusCode = 422;
        http.activationResponseBody = R"({
        "errors": [
            {
                "code": "MACHINE_LIMIT_EXCEEDED"
            }
        ]
    })";

        auto client = KeygenSDK::ClientTestAccess::create(testConfig(), http);
        const auto result = client.activate("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(result.error == KeygenSDK::ErrorCode::ActivationFailed);
        assert(http.postCallCount == 2);
    }

    void testActivationServerError() {
        FakeHttpClient http;

        http.validationResponseBody = R"({
        "meta": {
            "valid": true,
            "code": "VALID"
        },
        "data": {
            "id": "test-license-id",
            "type": "licenses"
        }
    })";

        http.activationStatusCode = 500;
        http.activationResponseBody = R"({
        "errors": [
            {
                "code": "INTERNAL_ERROR"
            }
        ]
    })";

        auto client = KeygenSDK::ClientTestAccess::create(testConfig(), http);
        const auto result = client.activate("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(result.error == KeygenSDK::ErrorCode::ServerError);
        assert(http.postCallCount == 2);
    }

    void testActivationInvalidJson() {
        FakeHttpClient http;

        http.validationResponseBody = R"({
        "meta": {
            "valid": true,
            "code": "VALID"
        },
        "data": {
            "id": "test-license-id",
            "type": "licenses"
        }
    })";

        http.activationResponseBody = "not valid json";

        auto client = KeygenSDK::ClientTestAccess::create(testConfig(), http);
        const auto result = client.activate("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(result.error == KeygenSDK::ErrorCode::InvalidResponse);
        assert(http.postCallCount == 2);
    }

    void testActivationInvalidResponse() {
        FakeHttpClient http;

        http.validationResponseBody = R"({
        "meta": {
            "valid": true,
            "code": "VALID"
        },
        "data": {
            "id": "test-license-id",
            "type": "licenses"
        }
    })";

        http.activationResponseBody = R"({
        "data": {
            "type": "machines",
            "attributes": {
                "fingerprint": "test-fingerprint"
            }
        }
    })";

        auto client = KeygenSDK::ClientTestAccess::create(testConfig(), http);
        const auto result = client.activate("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(result.error == KeygenSDK::ErrorCode::InvalidResponse);
        assert(http.postCallCount == 2);
    }

    void testActivationInvalidType() {
        FakeHttpClient http;

        http.validationResponseBody = R"({
        "meta": {
            "valid": true,
            "code": "VALID"
        },
        "data": {
            "id": "test-license-id",
            "type": "licenses"
        }
    })";

        http.activationResponseBody = R"({
        "data": {
            "id": "test-machine-id",
            "type": "licenses",
            "attributes": {
                "fingerprint": "test-fingerprint"
            }
        }
    })";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto result =
            client.activate("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(result.error ==
            KeygenSDK::ErrorCode::InvalidResponse);

        assert(http.postCallCount == 2);
    }

    void testActivationHttpClientFailure() {
        FakeHttpClient http;

        http.validationResponseBody = R"({
        "meta": {
            "valid": true,
            "code": "VALID"
        },
        "data": {
            "id": "test-license-id",
            "type": "licenses"
        }
    })";

        http.activationResponseBody = R"({
        "data": {
            "id": "test-machine-id",
            "type": "machines",
            "attributes": {
                "fingerprint": "test-fingerprint"
            }
        }
    })";

        http.activationResult =
            KeygenSDK::Result::failure(
                KeygenSDK::ErrorCode::NetworkError,
                "simulated activation network failure");

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto result =
            client.activate("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(result.error ==
            KeygenSDK::ErrorCode::NetworkError);
        assert(result.message ==
            "simulated activation network failure");

        assert(http.postCallCount == 2);
    }

    void testActivationMissingFingerprint() {
        FakeHttpClient http;

        http.validationResponseBody = R"({
        "meta": {
            "valid": true,
            "code": "VALID"
        },
        "data": {
            "id": "test-license-id",
            "type": "licenses"
        }
    })";

        http.activationResponseBody = R"({
        "data": {
            "id": "test-machine-id",
            "type": "machines",
            "attributes": {}
        }
    })";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto result =
            client.activate("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(result.error ==
            KeygenSDK::ErrorCode::InvalidResponse);

        assert(http.postCallCount == 2);
    }

    void testActivationSetsLocalLicenseState() {
        FakeHttpClient http;

        http.validationResponseBody = R"({
        "meta": {
            "valid": true,
            "code": "VALID"
        },
        "data": {
            "id": "test-license-id",
            "type": "licenses"
        }
    })";

        http.activationResponseBody = R"({
        "data": {
            "id": "test-machine-id",
            "type": "machines",
            "attributes": {
                "fingerprint": "test-fingerprint"
            }
        }
    })";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        assert(!client.hasLocalLicense());

        const auto result =
            client.activate("TEST-LICENSE-KEY");

        assert(result.ok);
        assert(client.hasLocalLicense());
    }

    void testActivationFailureDoesNotSetLocalLicenseState() {
        FakeHttpClient http;

        http.validationResponseBody = R"({
        "meta": {
            "valid": true,
            "code": "VALID"
        },
        "data": {
            "id": "test-license-id",
            "type": "licenses"
        }
    })";

        http.activationResponseBody = R"({
        "errors": [
            {
                "title": "Machine Limit Exceeded",
                "code": "MACHINE_LIMIT_EXCEEDED"
            }
        ]
    })";

        http.activationStatusCode = 422;

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        assert(!client.hasLocalLicense());

        const auto result =
            client.activate("TEST-LICENSE-KEY");

        assert(!result.ok);
        assert(result.error ==
            KeygenSDK::ErrorCode::ActivationFailed);

        assert(!client.hasLocalLicense());
        assert(http.postCallCount == 2);
    }

    void testDeactivateWithoutLocalLicense() {
        FakeHttpClient http;

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        assert(!client.hasLocalLicense());

        const auto result =
            client.deactivate();

        assert(!result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::InvalidLicense);

        assert(http.deleteCallCount == 0);
    }


    void testDeactivateHttpFailureKeepsLocalLicense() {
        FakeHttpClient http;

        http.validationResponseBody = R"({
        "meta": {
            "valid": true,
            "code": "VALID"
        },
        "data": {
            "id": "test-license-id",
            "type": "licenses"
        }
    })";

        http.activationResponseBody = R"({
        "data": {
            "id": "test-machine-id",
            "type": "machines",
            "attributes": {
                "fingerprint": "test-fingerprint"
            }
        }
    })";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto activation =
            client.activate("TEST-LICENSE-KEY");

        assert(activation.ok);
        assert(client.hasLocalLicense());

        http.deleteResult =
            KeygenSDK::Result::failure(
                KeygenSDK::ErrorCode::NetworkError,
                "simulated delete network failure");

        const auto result =
            client.deactivate();

        assert(!result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::NetworkError);

        assert(
            result.message ==
            "simulated delete network failure");

        assert(client.hasLocalLicense());
        assert(http.deleteCallCount == 1);

        assert(
            http.lastDeleteUrl ==
            "https://example.invalid/v1/accounts/test-account/machines/test-machine-id");

        assert(
            http.lastDeleteHeaders.size() == 1);

        assert(
            http.lastDeleteHeaders[0] ==
            "Authorization: License TEST-LICENSE-KEY");
    }


    void testDeactivateInvalidResponseKeepsLocalLicense() {
        FakeHttpClient http;

        http.validationResponseBody = R"({
        "meta": {
            "valid": true,
            "code": "VALID"
        },
        "data": {
            "id": "test-license-id",
            "type": "licenses"
        }
    })";

        http.activationResponseBody = R"({
        "data": {
            "id": "test-machine-id",
            "type": "machines",
            "attributes": {
                "fingerprint": "test-fingerprint"
            }
        }
    })";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto activation =
            client.activate("TEST-LICENSE-KEY");

        assert(activation.ok);
        assert(client.hasLocalLicense());

        http.deleteStatusCode = 200;

        const auto result =
            client.deactivate();

        assert(!result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::InvalidResponse);

        assert(client.hasLocalLicense());
        assert(http.deleteCallCount == 1);
    }


    void testDeactivateSuccessClearsLocalLicense() {
        FakeHttpClient http;

        http.validationResponseBody = R"({
        "meta": {
            "valid": true,
            "code": "VALID"
        },
        "data": {
            "id": "test-license-id",
            "type": "licenses"
        }
    })";

        http.activationResponseBody = R"({
        "data": {
            "id": "test-machine-id",
            "type": "machines",
            "attributes": {
                "fingerprint": "test-fingerprint"
            }
        }
    })";

        auto client =
            KeygenSDK::ClientTestAccess::create(
                testConfig(),
                http);

        const auto activation =
            client.activate("TEST-LICENSE-KEY");

        assert(activation.ok);
        assert(client.hasLocalLicense());

        http.deleteStatusCode = 204;

        const auto result =
            client.deactivate();

        assert(result.ok);
        assert(
            result.error ==
            KeygenSDK::ErrorCode::None);

        assert(!client.hasLocalLicense());
        assert(http.deleteCallCount == 1);

        assert(
            http.lastDeleteUrl ==
            "https://example.invalid/v1/accounts/test-account/machines/test-machine-id");

        assert(
            http.lastDeleteHeaders.size() == 1);

        assert(
            http.lastDeleteHeaders[0] ==
            "Authorization: License TEST-LICENSE-KEY");
    }

    void testLocalLicenseStateValid() {
        KeygenSDK::LocalLicenseState state{
            .licenseId = "test-license-id",
            .licenseKey = "TEST-LICENSE-KEY",
            .machineId = "test-machine-id",
            .machineFingerprint = "test-fingerprint"
        };

        assert(state.isValid());
    }

    void testLocalLicenseStateMissingLicenseId() {
        KeygenSDK::LocalLicenseState state{
            .licenseId = "",
            .licenseKey = "TEST-LICENSE-KEY",
            .machineId = "test-machine-id",
            .machineFingerprint = "test-fingerprint"
        };

        assert(!state.isValid());
    }

    void testLocalLicenseStateMissingLicenseKey() {
        KeygenSDK::LocalLicenseState state{
            .licenseId = "test-license-id",
            .licenseKey = "",
            .machineId = "test-machine-id",
            .machineFingerprint = "test-fingerprint"
        };

        assert(!state.isValid());
    }

    void testLocalLicenseStateMissingMachineId() {
        KeygenSDK::LocalLicenseState state{
            .licenseId = "test-license-id",
            .licenseKey = "TEST-LICENSE-KEY",
            .machineId = "",
            .machineFingerprint = "test-fingerprint"
        };

        assert(!state.isValid());
    }

    void testLocalLicenseStateMissingMachineFingerprint() {
        KeygenSDK::LocalLicenseState state{
            .licenseId = "test-license-id",
            .licenseKey = "TEST-LICENSE-KEY",
            .machineId = "test-machine-id",
            .machineFingerprint = ""
        };

        assert(!state.isValid());
    }

    void testMachineFingerprint() {
        std::string fingerprint;

        const auto result =
            KeygenSDK::MachineIdentity::fingerprint(fingerprint);

        assert(result.ok);
        assert(result.error == KeygenSDK::ErrorCode::None);

        assert(fingerprint.size() == 64);

        for (const char ch : fingerprint) {
            assert(
                (ch >= '0' && ch <= '9') ||
                (ch >= 'a' && ch <= 'f'));
        }
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
    testMissingLicenseId();
    testHttpClientFailure();
    testHttp4xx();
    testKeygenErrorResponse();
    testInvalidHost();
    testEmptyHttpsHost();
    testMachineFingerprint();
    testLocalLicenseStateValid();
    testLocalLicenseStateMissingLicenseId();
    testLocalLicenseStateMissingLicenseKey();
    testLocalLicenseStateMissingMachineId();
    testLocalLicenseStateMissingMachineFingerprint();
    testActivationSuccess();
    testActivationStopsWhenLicenseIsInvalid();
    testActivationHttpFailure();
    testActivationServerError();
    testActivationInvalidJson();
    testActivationInvalidResponse();
    testActivationInvalidType();
    testActivationHttpClientFailure();
    testActivationMissingFingerprint();
    testActivationSetsLocalLicenseState();
    testActivationFailureDoesNotSetLocalLicenseState();
    testDeactivateWithoutLocalLicense();
    testDeactivateHttpFailureKeepsLocalLicense();
    testDeactivateInvalidResponseKeepsLocalLicense();
    testDeactivateSuccessClearsLocalLicense();

    KeygenSDKTests::testLocalLicenseStoreSaveAndLoad();
    KeygenSDKTests::testLocalLicenseStoreLoadMissingState();
    KeygenSDKTests::testLocalLicenseStoreRemove();

    KeygenSDKTests::testLocalLicenseSerializationRoundTrip();
    KeygenSDKTests::testLocalLicenseSerializationRejectsInvalidState();
    KeygenSDKTests::testLocalLicenseDeserializationRejectsMalformedJson();
    KeygenSDKTests::testLocalLicenseDeserializationRejectsMissingVersion();
    KeygenSDKTests::testLocalLicenseDeserializationRejectsUnsupportedVersion();
    KeygenSDKTests::testLocalLicenseDeserializationRejectsMissingLicense();
    KeygenSDKTests::testLocalLicenseDeserializationRejectsMissingMachine();
    KeygenSDKTests::testLocalLicenseDeserializationRejectsMissingRequiredField();
    KeygenSDKTests::testLocalLicenseDeserializationRejectsInvalidFieldType();
    KeygenSDKTests::testLocalLicenseDeserializationRejectsEmptyRequiredField();
    KeygenSDKTests::testLocalLicenseDeserializationAllowsUnknownFields();

    KeygenSDKTests::testFileLocalLicenseStoreSaveAndLoad();
    KeygenSDKTests::testFileLocalLicenseStoreLoadMissingFile();
    KeygenSDKTests::testFileLocalLicenseStoreRemove();
    KeygenSDKTests::testFileLocalLicenseStoreMalformedJson();
    return 0;
}