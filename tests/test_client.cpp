#include <KeygenSDK/KeygenSDK.h>

#include <cassert>
#include <string>

int main() {
    using namespace KeygenSDK;

    static_assert(toString(ErrorCode::NetworkError) == "network_error");

    const Result success = Result::successResult("ok");
    assert(success.ok);
    assert(success.success());
    assert(success.error == ErrorCode::None);

    const Result failure = Result::failure(ErrorCode::InvalidLicense, "bad key");
    assert(!failure.ok);
    assert(failure.error == ErrorCode::InvalidLicense);
    assert(failure.message == "bad key");

    Client client({
        .host = "https://example.invalid",
        .accountId = "account",
    });

    const Result emptyKey = client.validateOnline("");
    assert(!emptyKey.ok);
    assert(emptyKey.error == ErrorCode::InvalidLicense);

    const Result phase1 = client.validateOnline("REDACTED");
    assert(!phase1.ok);
    assert(phase1.error == ErrorCode::Unknown);

    assert(!client.hasLocalLicense());

    return 0;
}
