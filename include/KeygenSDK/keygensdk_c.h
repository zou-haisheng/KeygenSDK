#ifndef KEYGENSDK_C_API_H
#define KEYGENSDK_C_API_H

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

/* Configuration passed to KeygenSDK_Client_Create(). */
typedef struct KeygenSDK_Config {
    const char* host;                /* required, https:// URL */
    const char* account_id;          /* required */
    const char* account_public_key;  /* hex Ed25519 public key, needed for verifyOffline */
    long timeout_seconds;            /* <= 0 means the default (15) */
    const char* local_license_path;  /* optional; NULL means the default path */
} KeygenSDK_Config;

/* Opaque client handle. */
typedef struct KeygenSDK_Client KeygenSDK_Client;

/*
 * Creates a client. Returns NULL only if `config` is NULL or allocation fails;
 * configuration problems are reported by the operation calls below.
 */
KEYGENSDK_C_API KeygenSDK_Client* KeygenSDK_Client_Create(const KeygenSDK_Config* config);

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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* KEYGENSDK_C_API_H */
