#include "KeygenSDK/keygensdk_c.h"

#include "KeygenSDK/Client.h"
#include "KeygenSDK/Error.h"
#include "KeygenSDK/MachineIdentity.h"

#include <cstring>
#include <filesystem>
#include <string>
#include <utility>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

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

    void copyString(char* dst, std::size_t dstSize, const char* src) {
        if (dstSize == 0) {
            return;
        }

        const std::size_t length = std::strlen(src);
        std::size_t count = length < dstSize - 1 ? length : dstSize - 1;

        /* Never cut a UTF-8 codepoint in half: back off continuation bytes. */
        while (count > 0 &&
               (static_cast<unsigned char>(src[count]) & 0xC0) == 0x80) {
            --count;
        }

        std::memcpy(dst, src, count);
        dst[count] = '\0';
    }

    void setError(
        KeygenSDK_Result* out,
        KeygenSDK_ErrorCode code,
        const char* message) {

        out->ok = 0;
        out->error = code;
        copyString(out->message, sizeof(out->message), message);
    }

    void fillResult(KeygenSDK_Result* out, const KeygenSDK::Result& result) {
        out->ok = result.ok ? 1 : 0;
        out->error = static_cast<KeygenSDK_ErrorCode>(result.error);
        copyString(out->message, sizeof(out->message), result.message.c_str());
    }

    bool utf8ToPath(const char* utf8, std::filesystem::path& out) {
        if (!utf8 || *utf8 == '\0') {
            out.clear();
            return true;
        }

        #ifdef _WIN32
            const int size = MultiByteToWideChar(
                CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, nullptr, 0);
            if (size <= 0) {
                return false;
            }

            std::wstring wide(static_cast<std::size_t>(size), L'\0');
            const int converted = MultiByteToWideChar(
                CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, wide.data(), size);
            if (converted != size) {
                return false;
            }

            wide.resize(static_cast<std::size_t>(size - 1));
            out = std::filesystem::path(std::move(wide));
        #else
            out = std::filesystem::path(utf8);
        #endif
            return true;
    }

    template <typename Fn>
    void callGuarded(KeygenSDK_Result* out, Fn&& fn) {
        try {
            fillResult(out, fn());
        } catch (...) {
            setError(out, KEYGENSDK_ERROR_UNKNOWN, "internal error");
        }
    }

    bool buildConfig(const KeygenSDK_Config& config, KeygenSDK::Config& out) {
        out.host = config.host ? config.host : "";
        out.accountId = config.account_id ? config.account_id : "";
        out.accountPublicKey =
            config.account_public_key ? config.account_public_key : "";
        out.timeoutSeconds =
            config.timeout_seconds > 0 ? config.timeout_seconds : 15;

        if (config.local_license_path && *config.local_license_path != '\0') {
            std::filesystem::path path;
            if (!utf8ToPath(config.local_license_path, path)) {
                return false;
            }
            out.localLicensePath = std::move(path);
        }

        return true;
    }

} // namespace

extern "C" {

KeygenSDK_Client* KeygenSDK_Client_Create(
    const KeygenSDK_Config* config,
    KeygenSDK_ErrorCode* out_error) {

    if (!config) {
        if (out_error) {
            *out_error = KEYGENSDK_ERROR_INVALID_CONFIGURATION;
        }
        return nullptr;
    }

    try {
        KeygenSDK::Config built;
        if (!buildConfig(*config, built)) {
            if (out_error) {
                *out_error = KEYGENSDK_ERROR_INVALID_CONFIGURATION;
            }
            return nullptr;
        }
        return new KeygenSDK_Client(std::move(built));
    } catch (...) {
        if (out_error) {
            *out_error = KEYGENSDK_ERROR_UNKNOWN;
        }
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

    callGuarded(out, [&] {
        return client->client.validateOnline(license_key ? license_key : "");
    });
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

    callGuarded(out, [&] {
        return client->client.activate(license_key ? license_key : "");
    });
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

    callGuarded(out, [&] { return client->client.verifyOffline(); });
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

    callGuarded(out, [&] { return client->client.deactivate(); });
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

    callGuarded(out, [&] { return client->client.loadLocalLicense(); });
}

int KeygenSDK_Client_HasLocalLicense(const KeygenSDK_Client* client) {
    if (!client) {
        return 0;
    }

    try {
        return client->client.hasLocalLicense() ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

const char* KeygenSDK_ErrorCode_ToString(KeygenSDK_ErrorCode code) {
    return KeygenSDK::toString(static_cast<KeygenSDK::ErrorCode>(code)).data();
}

KeygenSDK_ErrorCode KeygenSDK_MachineFingerprint(char* out, size_t size) {
    if (!out || size == 0) {
        return KEYGENSDK_ERROR_INVALID_CONFIGURATION;
    }

    try {
        std::string fingerprint;
        const KeygenSDK::Result result =
            KeygenSDK::MachineIdentity::fingerprint(fingerprint);

        if (!result.ok) {
            return static_cast<KeygenSDK_ErrorCode>(result.error);
        }

        copyString(out, size, fingerprint.c_str());
        return KEYGENSDK_ERROR_NONE;
    } catch (...) {
        return KEYGENSDK_ERROR_UNKNOWN;
    }
}

size_t KeygenSDK_Client_ExportMachineFile(
    KeygenSDK_Client* client,
    char* out,
    size_t size) {

    if (!client) {
        return 0;
    }

    try {
        const std::string machineFile =
            client->client.exportMachineFile();

        if (machineFile.empty()) {
            return 0;
        }

        const std::size_t required = machineFile.size() + 1;

        if (out != nullptr && size >= required) {
            std::memcpy(out, machineFile.c_str(), required);
        }

        return required;
    } catch (...) {
        return 0;
    }
}

void KeygenSDK_Client_ImportMachineFile(
    KeygenSDK_Client* client,
    const char* machine_file,
    KeygenSDK_Result* out) {

    if (!out) {
        return;
    }
    clearResult(out);

    if (!client) {
        setError(out, KEYGENSDK_ERROR_INVALID_CONFIGURATION, "client is null");
        return;
    }

    callGuarded(out, [&] {
        return client->client.importMachineFile(
            machine_file ? machine_file : "");
    });
}

void KeygenSDK_Client_GetLicenseInfo(
    KeygenSDK_Client* client,
    KeygenSDK_LicenseInfo* out) {

    if (!out) {
        return;
    }

    out->license_id[0] = '\0';
    out->expiry[0] = '\0';

    if (!client) {
        return;
    }

    try {
        copyString(
            out->license_id,
            sizeof(out->license_id),
            client->client.licenseId().c_str());
        copyString(
            out->expiry,
            sizeof(out->expiry),
            client->client.licenseExpiry().c_str());
    } catch (...) {
        // Leave fields empty on unexpected failure.
    }
}

} // extern "C"
