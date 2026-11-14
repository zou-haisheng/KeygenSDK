#include "KeygenSDK/keygensdk_c.h"

#include "KeygenSDK/Client.h"
#include "KeygenSDK/Error.h"

#include <cstring>
#include <string>
#include <utility>

/* Keep the C error codes in sync with the C++ KeygenSDK::ErrorCode enum. */
static_assert(
    static_cast<int>(KEYGENSDK_ERROR_NONE) ==
        static_cast<int>(KeygenSDK::ErrorCode::None),
    "C error code NONE is out of sync with KeygenSDK::ErrorCode::None");
static_assert(
    static_cast<int>(KEYGENSDK_ERROR_OFFLINE_VERIFICATION_FAILED) ==
        static_cast<int>(KeygenSDK::ErrorCode::OfflineVerificationFailed),
    "C error codes are out of sync with KeygenSDK::ErrorCode");
static_assert(
    static_cast<int>(KEYGENSDK_ERROR_UNKNOWN) ==
        static_cast<int>(KeygenSDK::ErrorCode::Unknown),
    "C error code UNKNOWN is out of sync with KeygenSDK::ErrorCode::Unknown");

/* Opaque handle definition: wraps the C++ client. */
struct KeygenSDK_Client {
    KeygenSDK::Client client;

    explicit KeygenSDK_Client(KeygenSDK::Config config)
        : client(std::move(config)) {}
};

namespace {

    void clearResult(KeygenSDK_Result* out) {
        out->ok = 0;
        out->error = KEYGENSDK_ERROR_UNKNOWN;
        out->message[0] = '\0';
    }

    void setError(
        KeygenSDK_Result* out,
        KeygenSDK_ErrorCode code,
        const char* message) {

        out->ok = 0;
        out->error = code;
        std::strncpy(out->message, message, sizeof(out->message) - 1);
        out->message[sizeof(out->message) - 1] = '\0';
    }

    void fillResult(KeygenSDK_Result* out, const KeygenSDK::Result& result) {
        out->ok = result.ok ? 1 : 0;
        out->error = static_cast<KeygenSDK_ErrorCode>(result.error);
        std::strncpy(
            out->message,
            result.message.c_str(),
            sizeof(out->message) - 1);
        out->message[sizeof(out->message) - 1] = '\0';
    }

    KeygenSDK::Config toConfig(const KeygenSDK_Config& config) {
        KeygenSDK::Config result;
        result.host = config.host ? config.host : "";
        result.accountId = config.account_id ? config.account_id : "";
        result.accountPublicKey =
            config.account_public_key ? config.account_public_key : "";
        result.timeoutSeconds =
            config.timeout_seconds > 0 ? config.timeout_seconds : 15;
        if (config.local_license_path) {
            result.localLicensePath = config.local_license_path;
        }
        return result;
    }

} // namespace

extern "C" {

KeygenSDK_Client* KeygenSDK_Client_Create(const KeygenSDK_Config* config) {
    if (!config) {
        return nullptr;
    }

    try {
        return new KeygenSDK_Client(toConfig(*config));
    } catch (...) {
        return nullptr;
    }
}

void KeygenSDK_Client_Destroy(KeygenSDK_Client* client) {
    delete client;
}

void KeygenSDK_Client_ValidateOnline(
    KeygenSDK_Client* client,
    const char* license_key,
    KeygenSDK_Result* out) {

    if (!out) {
        return;
    }
    clearResult(out);

    if (!client) {
        setError(out, KEYGENSDK_ERROR_INVALID_CONFIGURATION, "client is null");
        return;
    }

    fillResult(
        out,
        client->client.validateOnline(license_key ? license_key : ""));
}

void KeygenSDK_Client_Activate(
    KeygenSDK_Client* client,
    const char* license_key,
    KeygenSDK_Result* out) {

    if (!out) {
        return;
    }
    clearResult(out);

    if (!client) {
        setError(out, KEYGENSDK_ERROR_INVALID_CONFIGURATION, "client is null");
        return;
    }

    fillResult(
        out,
        client->client.activate(license_key ? license_key : ""));
}

void KeygenSDK_Client_VerifyOffline(
    KeygenSDK_Client* client,
    KeygenSDK_Result* out) {

    if (!out) {
        return;
    }
    clearResult(out);

    if (!client) {
        setError(out, KEYGENSDK_ERROR_INVALID_CONFIGURATION, "client is null");
        return;
    }

    fillResult(out, client->client.verifyOffline());
}

void KeygenSDK_Client_Deactivate(
    KeygenSDK_Client* client,
    KeygenSDK_Result* out) {

    if (!out) {
        return;
    }
    clearResult(out);

    if (!client) {
        setError(out, KEYGENSDK_ERROR_INVALID_CONFIGURATION, "client is null");
        return;
    }

    fillResult(out, client->client.deactivate());
}

void KeygenSDK_Client_LoadLocalLicense(
    KeygenSDK_Client* client,
    KeygenSDK_Result* out) {

    if (!out) {
        return;
    }
    clearResult(out);

    if (!client) {
        setError(out, KEYGENSDK_ERROR_INVALID_CONFIGURATION, "client is null");
        return;
    }

    fillResult(out, client->client.loadLocalLicense());
}

int KeygenSDK_Client_HasLocalLicense(const KeygenSDK_Client* client) {
    return (client && client->client.hasLocalLicense()) ? 1 : 0;
}

const char* KeygenSDK_ErrorCode_ToString(KeygenSDK_ErrorCode code) {
    return KeygenSDK::toString(static_cast<KeygenSDK::ErrorCode>(code)).data();
}

} // extern "C"
