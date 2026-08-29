#include <KeygenSDK/KeygenSDK.h>

#include <iostream>

int main() {
    KeygenSDK::Client client({
        .host = "https://license.staredium.online",
        .accountId = "YOUR_ACCOUNT_ID",
    });

    // Phase 1 only validates the SDK shape. Actual Keygen endpoint binding starts in Phase 2.
    const auto result = client.validateOnline("YOUR_LICENSE_KEY");
    std::cout << "ok=" << std::boolalpha << result.ok
              << ", error=" << KeygenSDK::toString(result.error) << '\n';
    return 0;
}
