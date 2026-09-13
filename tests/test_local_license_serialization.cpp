#include <cassert>
#include <string>

#include "KeygenSDK/LocalLicenseSerialization.h"

namespace KeygenSDKTests {

    namespace {

        KeygenSDK::LocalLicenseState makeTestState() {
            return {
                .licenseId = "test-license-id",
                .licenseKey = "TEST-LICENSE-KEY",
                .machineId = "test-machine-id",
                .machineFingerprint = "test-fingerprint"
            };
        }

    } // namespace

    void testLocalLicenseSerializationRoundTrip() {
        const auto original = makeTestState();

        std::string json;

        const auto serializeResult =
            KeygenSDK::LocalLicenseSerializer::serialize(
                original,
                json);

        assert(serializeResult.ok);
        assert(!json.empty());

        KeygenSDK::LocalLicenseState restored;

        const auto deserializeResult =
            KeygenSDK::LocalLicenseSerializer::deserialize(
                json,
                restored);

        assert(deserializeResult.ok);

        assert(restored.licenseId == original.licenseId);
        assert(restored.licenseKey == original.licenseKey);
        assert(restored.machineId == original.machineId);
        assert(
            restored.machineFingerprint ==
            original.machineFingerprint);
    }

    void testLocalLicenseSerializationRejectsInvalidState() {
        const KeygenSDK::LocalLicenseState state{
            .licenseId = "",
            .licenseKey = "TEST-LICENSE-KEY",
            .machineId = "test-machine-id",
            .machineFingerprint = "test-fingerprint"
        };

        std::string json;

        const auto result =
            KeygenSDK::LocalLicenseSerializer::serialize(
                state,
                json);

        assert(!result.ok);
    }

    void testLocalLicenseDeserializationRejectsMalformedJson() {
        KeygenSDK::LocalLicenseState state;

        const auto result =
            KeygenSDK::LocalLicenseSerializer::deserialize(
                "{invalid json",
                state);

        assert(!result.ok);
    }

    void testLocalLicenseDeserializationRejectsMissingVersion() {
        const std::string json = R"({
        "license": {
            "id": "test-license-id",
            "key": "TEST-LICENSE-KEY"
        },
        "machine": {
            "id": "test-machine-id",
            "fingerprint": "test-fingerprint"
        }
    })";

        KeygenSDK::LocalLicenseState state;

        const auto result =
            KeygenSDK::LocalLicenseSerializer::deserialize(
                json,
                state);

        assert(!result.ok);
    }

    void testLocalLicenseDeserializationRejectsUnsupportedVersion() {
        const std::string json = R"({
        "version": 999,
        "license": {
            "id": "test-license-id",
            "key": "TEST-LICENSE-KEY"
        },
        "machine": {
            "id": "test-machine-id",
            "fingerprint": "test-fingerprint"
        }
    })";

        KeygenSDK::LocalLicenseState state;

        const auto result =
            KeygenSDK::LocalLicenseSerializer::deserialize(
                json,
                state);

        assert(!result.ok);
    }

    void testLocalLicenseDeserializationRejectsMissingLicense() {
        const std::string json = R"({
        "version": 1,
        "machine": {
            "id": "test-machine-id",
            "fingerprint": "test-fingerprint"
        }
    })";

        KeygenSDK::LocalLicenseState state;

        const auto result =
            KeygenSDK::LocalLicenseSerializer::deserialize(
                json,
                state);

        assert(!result.ok);
    }

    void testLocalLicenseDeserializationRejectsMissingMachine() {
        const std::string json = R"({
        "version": 1,
        "license": {
            "id": "test-license-id",
            "key": "TEST-LICENSE-KEY"
        }
    })";

        KeygenSDK::LocalLicenseState state;

        const auto result =
            KeygenSDK::LocalLicenseSerializer::deserialize(
                json,
                state);

        assert(!result.ok);
    }

    void testLocalLicenseDeserializationRejectsMissingRequiredField() {
        const std::string json = R"({
        "version": 1,
        "license": {
            "id": "test-license-id"
        },
        "machine": {
            "id": "test-machine-id",
            "fingerprint": "test-fingerprint"
        }
    })";

        KeygenSDK::LocalLicenseState state;

        const auto result =
            KeygenSDK::LocalLicenseSerializer::deserialize(
                json,
                state);

        assert(!result.ok);
    }

    void testLocalLicenseDeserializationRejectsInvalidFieldType() {
        const std::string json = R"({
        "version": 1,
        "license": {
            "id": 123,
            "key": "TEST-LICENSE-KEY"
        },
        "machine": {
            "id": "test-machine-id",
            "fingerprint": "test-fingerprint"
        }
    })";

        KeygenSDK::LocalLicenseState state;

        const auto result =
            KeygenSDK::LocalLicenseSerializer::deserialize(
                json,
                state);

        assert(!result.ok);
    }

    void testLocalLicenseDeserializationRejectsEmptyRequiredField() {
        const std::string json = R"({
        "version": 1,
        "license": {
            "id": "",
            "key": "TEST-LICENSE-KEY"
        },
        "machine": {
            "id": "test-machine-id",
            "fingerprint": "test-fingerprint"
        }
    })";

        KeygenSDK::LocalLicenseState state;

        const auto result =
            KeygenSDK::LocalLicenseSerializer::deserialize(
                json,
                state);

        assert(!result.ok);
    }

    void testLocalLicenseDeserializationAllowsUnknownFields() {
        const std::string json = R"({
        "version": 1,
        "futureField": "ignored",
        "license": {
            "id": "test-license-id",
            "key": "TEST-LICENSE-KEY",
            "futureLicenseField": true
        },
        "machine": {
            "id": "test-machine-id",
            "fingerprint": "test-fingerprint",
            "futureMachineField": 123
        }
    })";

        KeygenSDK::LocalLicenseState state;

        const auto result =
            KeygenSDK::LocalLicenseSerializer::deserialize(
                json,
                state);

        assert(result.ok);

        assert(state.licenseId == "test-license-id");
        assert(state.licenseKey == "TEST-LICENSE-KEY");
        assert(state.machineId == "test-machine-id");
        assert(state.machineFingerprint == "test-fingerprint");
    }

} // namespace KeygenSDKTests