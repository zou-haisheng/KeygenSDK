#include <cassert>
#include <string>

#include "KeygenSDK/LocalDataProtection.h"

namespace KeygenSDKTests {

    void testLocalDataProtectionRoundTrip() {
        const std::string original =
            "KeygenSDK local license cache test data.";

        std::string protectedData;

        const auto protectResult =
            KeygenSDK::LocalDataProtection::protect(
                original,
                protectedData);

        assert(protectResult.ok);
        assert(!protectedData.empty());
        assert(protectedData != original);

        std::string restored;

        const auto unprotectResult =
            KeygenSDK::LocalDataProtection::unprotect(
                protectedData,
                restored);

        assert(unprotectResult.ok);
        assert(restored == original);
    }

    void testLocalDataProtectionRejectsEmptyPlaintext() {
        std::string protectedData;

        const auto result =
            KeygenSDK::LocalDataProtection::protect(
                "",
                protectedData);

        assert(!result.ok);
    }

    void testLocalDataProtectionRejectsEmptyProtectedData() {
        std::string plaintext;

        const auto result =
            KeygenSDK::LocalDataProtection::unprotect(
                "",
                plaintext);

        assert(!result.ok);
    }

    void testLocalDataProtectionRejectsInvalidProtectedData() {
        std::string plaintext;

        const std::string invalidData =
            "this-is-not-valid-dpapi-data";

        const auto result =
            KeygenSDK::LocalDataProtection::unprotect(
                invalidData,
                plaintext);

        assert(!result.ok);
    }

    void testLocalDataProtectionDoesNotModifyOutputOnProtectFailure() {
        std::string protectedData =
            "existing-value";

        const auto result =
            KeygenSDK::LocalDataProtection::protect(
                "",
                protectedData);

        assert(!result.ok);
        assert(protectedData == "existing-value");
    }

    void testLocalDataProtectionDoesNotModifyOutputOnUnprotectFailure() {
        std::string plaintext =
            "existing-value";

        const auto result =
            KeygenSDK::LocalDataProtection::unprotect(
                "invalid-data",
                plaintext);

        assert(!result.ok);
        assert(plaintext == "existing-value");
    }

} // namespace KeygenSDKTests