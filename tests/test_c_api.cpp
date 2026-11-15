#include <cassert>
#include <cstring>

#include "KeygenSDK/keygensdk_c.h"

namespace KeygenSDKTests {

    namespace {

        void checkErrorString(KeygenSDK_ErrorCode code, const char* expected) {
            assert(
                std::strcmp(
                    KeygenSDK_ErrorCode_ToString(code),
                    expected) == 0);
        }

    } // namespace

    void testCApiCreateNullConfig() {
        KeygenSDK_ErrorCode error = KEYGENSDK_ERROR_NONE;
        KeygenSDK_Client* client =
            KeygenSDK_Client_Create(nullptr, &error);

        assert(client == nullptr);
        assert(error == KEYGENSDK_ERROR_INVALID_CONFIGURATION);
    }

    void testCApiCreateInvalidUtf8Path() {
        KeygenSDK_Config config = {};
        config.host = "https://example.com";
        config.account_id = "account";
        config.local_license_path = "\xFF\xFF";

        KeygenSDK_ErrorCode error = KEYGENSDK_ERROR_NONE;
        KeygenSDK_Client* client =
            KeygenSDK_Client_Create(&config, &error);

        assert(client == nullptr);
        assert(error == KEYGENSDK_ERROR_INVALID_CONFIGURATION);
    }

    void testCApiCreateChinesePath() {
        KeygenSDK_Config config = {};
        config.host = "https://example.com";
        config.account_id = "account";
        // UTF-8 bytes for "临时目录" (temporary directory).
        config.local_license_path =
            "C:\\" "\xE4\xB8\xB4" "\xE6\x97\xB6" "\xE7\x9B\xAE" "\xE5\xBD\x95"
            "\\license.dat";

        KeygenSDK_Client* client =
            KeygenSDK_Client_Create(&config, nullptr);

        assert(client != nullptr);
        KeygenSDK_Client_Destroy(client);
    }

    void testCApiErrorCodeToString() {
        checkErrorString(KEYGENSDK_ERROR_NONE, "none");
        checkErrorString(
            KEYGENSDK_ERROR_INVALID_CONFIGURATION,
            "invalid_configuration");
        checkErrorString(KEYGENSDK_ERROR_NETWORK, "network_error");
        checkErrorString(KEYGENSDK_ERROR_INVALID_LICENSE, "invalid_license");
        checkErrorString(
            KEYGENSDK_ERROR_LOCAL_STATE_NOT_FOUND,
            "local_state_not_found");
        checkErrorString(KEYGENSDK_ERROR_LOCAL_STORAGE, "local_storage_error");
        checkErrorString(KEYGENSDK_ERROR_LICENSE_EXPIRED, "license_expired");
        checkErrorString(
            KEYGENSDK_ERROR_LICENSE_SUSPENDED,
            "license_suspended");
        checkErrorString(KEYGENSDK_ERROR_LICENSE_REVOKED, "license_revoked");
        checkErrorString(KEYGENSDK_ERROR_LICENSE_OVERDUE, "license_overdue");
        checkErrorString(
            KEYGENSDK_ERROR_MACHINE_LIMIT_REACHED,
            "machine_limit_reached");
        checkErrorString(
            KEYGENSDK_ERROR_ACTIVATION_FAILED,
            "activation_failed");
        checkErrorString(
            KEYGENSDK_ERROR_OFFLINE_DATA_MISSING,
            "offline_data_missing");
        checkErrorString(
            KEYGENSDK_ERROR_OFFLINE_VERIFICATION_FAILED,
            "offline_verification_failed");
        checkErrorString(
            KEYGENSDK_ERROR_INVALID_RESPONSE,
            "invalid_response");
        checkErrorString(KEYGENSDK_ERROR_SERVER, "server_error");
        checkErrorString(KEYGENSDK_ERROR_UNKNOWN, "unknown");
    }

    void testCApiNullClientOperations() {
        KeygenSDK_Result result;

        KeygenSDK_Client_ValidateOnline(nullptr, "key", &result);
        assert(!result.ok);
        assert(result.error == KEYGENSDK_ERROR_INVALID_CONFIGURATION);

        KeygenSDK_Client_Activate(nullptr, "key", &result);
        assert(!result.ok);
        assert(result.error == KEYGENSDK_ERROR_INVALID_CONFIGURATION);

        KeygenSDK_Client_VerifyOffline(nullptr, &result);
        assert(!result.ok);
        assert(result.error == KEYGENSDK_ERROR_INVALID_CONFIGURATION);

        KeygenSDK_Client_Deactivate(nullptr, &result);
        assert(!result.ok);
        assert(result.error == KEYGENSDK_ERROR_INVALID_CONFIGURATION);

        KeygenSDK_Client_LoadLocalLicense(nullptr, &result);
        assert(!result.ok);
        assert(result.error == KEYGENSDK_ERROR_INVALID_CONFIGURATION);

        assert(KeygenSDK_Client_HasLocalLicense(nullptr) == 0);
        KeygenSDK_Client_Destroy(nullptr);
    }

} // namespace KeygenSDKTests
