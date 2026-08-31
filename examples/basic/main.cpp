#include <KeygenSDK/KeygenSDK.h>

#include <iostream>

int main() {
    KeygenSDK::Client client({
        .host = "https://license.staredium.online",
        .accountId = "YOUR_ACCOUNT_ID",
    });

    // Phase 2: validate a license key against the configured Keygen server.
    const auto result = client.validateOnline("YOUR_LICENSE_KEY");
    std::cout << "ok=" << std::boolalpha << result.ok
              << ", error=" << KeygenSDK::toString(result.error) << '\n';
    return 0;
}
