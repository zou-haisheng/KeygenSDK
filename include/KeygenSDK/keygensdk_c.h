#ifndef KEYGENSDK_C_API_H
#define KEYGENSDK_C_API_H

#include <stddef.h>

/*
 * KeygenSDK C ABI bridge.
 *
 * This header is available when the SDK is built with
 * -DKEYGENSDK_BUILD_C_API=ON. It exposes an opaque client handle plus
 * C-friendly config/result structs, and can be included from both C and C++.
 *
 * Typical usage:
 *
 *     KeygenSDK_Config config = {0};
 *     config.host = "https://example.com";
 *     config.account_id = "your-account-id";
 *     config.timeout_seconds = 15;
 *
 *     KeygenSDK_Client* client = KeygenSDK_Client_Create(&config);
 *     if (!client) { ... }
 *
 *     KeygenSDK_Result result;
 *     KeygenSDK_Client_ValidateOnline(client, "YOUR_LICENSE_KEY", &result);
 *     if (!result.ok) {
 *         printf("error %d: %s\n", (int)result.error, result.message);
 *     }
 *
 *     KeygenSDK_Client_Destroy(client);
 */

#ifdef _WIN32
#  if defined(KEYGENSDK_C_API_SHARED)
#    if defined(KEYGENSDK_C_API_EXPORTS)
#      define KEYGENSDK_C_API __declspec(dllexport)
#    else
#      define KEYGENSDK_C_API __declspec(dllimport)
#    endif
#  else
#    define KEYGENSDK_C_API
#  endif
#else
#  define KEYGENSDK_C_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Error codes. The explicit values mirror KeygenSDK::ErrorCode so that the
 * two enums can be mapped with a plain cast.
 */
typedef enum KeygenSDK_ErrorCode {
    KEYGENSDK_ERROR_NONE = 0,
    KEYGENSDK_ERROR_INVALID_CONFIGURATION = 1,
    KEYGENSDK_ERROR_NETWORK = 2,
    KEYGENSDK_ERROR_INVALID_LICENSE = 3,
    KEYGENSDK_ERROR_LOCAL_STATE_NOT_FOUND = 4,
    KEYGENSDK_ERROR_LOCAL_STORAGE = 5,
    KEYGENSDK_ERROR_LICENSE_EXPIRED = 6,
    KEYGENSDK_ERROR_LICENSE_SUSPENDED = 7,
    KEYGENSDK_ERROR_LICENSE_REVOKED = 8,
    KEYGENSDK_ERROR_LICENSE_OVERDUE = 9,
    KEYGENSDK_ERROR_MACHINE_LIMIT_REACHED = 10,
    KEYGENSDK_ERROR_ACTIVATION_FAILED = 11,
    KEYGENSDK_ERROR_OFFLINE_DATA_MISSING = 12,
    KEYGENSDK_ERROR_OFFLINE_VERIFICATION_FAILED = 13,
    KEYGENSDK_ERROR_INVALID_RESPONSE = 14,
    KEYGENSDK_ERROR_SERVER = 15,
    KEYGENSDK_ERROR_UNKNOWN = 16
} KeygenSDK_ErrorCode;

/* Result of an SDK operation. `message` is always NUL-terminated. */
typedef struct KeygenSDK_Result {
    int ok;
    KeygenSDK_ErrorCode error;
    char message[256];
} KeygenSDK_Result;

/* Configuration passed to KeygenSDK_Client_Create(). All string fields are UTF-8. */
typedef struct KeygenSDK_Config {
    const char* host;                /* required, https:// URL */
    const char* account_id;          /* required */
    const char* account_public_key;  /* hex Ed25519 public key, needed for verifyOffline */
    long timeout_seconds;            /* <= 0 means the default (15) */
    const char* local_license_path;  /* optional, UTF-8; NULL means the default path */
} KeygenSDK_Config;

/* Opaque client handle. */
typedef struct KeygenSDK_Client KeygenSDK_Client;

/* License metadata available after activation or offline verification. */
typedef struct KeygenSDK_LicenseInfo {
    char license_id[128];  /* license UUID; empty when unknown */
    char expiry[64];       /* ISO 8601 expiry; empty when unknown */
} KeygenSDK_LicenseInfo;

/*
 * Creates a client. On failure returns NULL and, when `out_error` is non-NULL, stores
 * the reason there: KEYGENSDK_ERROR_INVALID_CONFIGURATION for a NULL `config` or an
 * invalid UTF-8 `local_license_path`, otherwise KEYGENSDK_ERROR_UNKNOWN (for example,
 * allocation failure). `out_error` may be NULL.
 */
KEYGENSDK_C_API KeygenSDK_Client* KeygenSDK_Client_Create(
    const KeygenSDK_Config* config,
    KeygenSDK_ErrorCode* out_error);

/* Destroys a client created by KeygenSDK_Client_Create(). NULL is a no-op. */
KEYGENSDK_C_API void KeygenSDK_Client_Destroy(KeygenSDK_Client* client);

/* Fills `out` with the result. `out` must not be NULL. */
KEYGENSDK_C_API void KeygenSDK_Client_ValidateOnline(
    KeygenSDK_Client* client,
    const char* license_key,
    KeygenSDK_Result* out);

KEYGENSDK_C_API void KeygenSDK_Client_Activate(
    KeygenSDK_Client* client,
    const char* license_key,
    KeygenSDK_Result* out);

KEYGENSDK_C_API void KeygenSDK_Client_VerifyOffline(
    KeygenSDK_Client* client,
    KeygenSDK_Result* out);

KEYGENSDK_C_API void KeygenSDK_Client_Deactivate(
    KeygenSDK_Client* client,
    KeygenSDK_Result* out);

KEYGENSDK_C_API void KeygenSDK_Client_LoadLocalLicense(
    KeygenSDK_Client* client,
    KeygenSDK_Result* out);

/* Returns 1 if a local license is currently loaded, 0 otherwise. */
KEYGENSDK_C_API int KeygenSDK_Client_HasLocalLicense(const KeygenSDK_Client* client);

/* Returns a stable, NUL-terminated, human-readable error name. */
KEYGENSDK_C_API const char* KeygenSDK_ErrorCode_ToString(KeygenSDK_ErrorCode code);

/* Computes the machine fingerprint (hex SHA-256 of the Windows MachineGuid) and writes
 * it to `out` (up to `size` bytes, NUL-terminated). Returns KEYGENSDK_ERROR_NONE on
 * success. */
KEYGENSDK_C_API KeygenSDK_ErrorCode KeygenSDK_MachineFingerprint(
    char* out,
    size_t size);

/* Writes the checked-out machine file to `out` (up to `size` bytes). Returns the number
 * of bytes required including the NUL terminator, or 0 on error. Pass `out == NULL` to
 * query the required size first. */
KEYGENSDK_C_API size_t KeygenSDK_Client_ExportMachineFile(
    KeygenSDK_Client* client,
    char* out,
    size_t size);

/* Imports a checked-out machine file and persists it. Requires an already loaded or
 * activated local license state. */
KEYGENSDK_C_API void KeygenSDK_Client_ImportMachineFile(
    KeygenSDK_Client* client,
    const char* machine_file,
    KeygenSDK_Result* out);

/* Fills `out` with the loaded license metadata. Fields are NUL-terminated and empty when
 * unknown (`expiry` is only known after a successful offline verification). */
KEYGENSDK_C_API void KeygenSDK_Client_GetLicenseInfo(
    KeygenSDK_Client* client,
    KeygenSDK_LicenseInfo* out);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* KEYGENSDK_C_API_H */
