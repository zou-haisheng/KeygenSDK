#include <cassert>

#include "KeygenSDK/LocalLicenseStore.h"

namespace KeygenSDKTests {

    class FakeLocalLicenseStore final: 
        public KeygenSDK::ILocalLicenseStore {
    public:
        [[nodiscard]] KeygenSDK::Result load(
            KeygenSDK::LocalLicenseState& state) const override {
            if (!hasState_) {
                return KeygenSDK::Result::failure(
                    KeygenSDK::ErrorCode::LocalStateNotFound,
                    "Local license state not found.");
            }

            state = state_;
            return KeygenSDK::Result::successResult();
        }

        [[nodiscard]] KeygenSDK::Result save(
            const KeygenSDK::LocalLicenseState& state) override {
            state_ = state;
            hasState_ = true;

            return KeygenSDK::Result::successResult();
        }

        [[nodiscard]] KeygenSDK::Result remove() override {
            state_ = {};
            hasState_ = false;

            return KeygenSDK::Result::successResult();
        }

    private:
        bool hasState_{ false };
        KeygenSDK::LocalLicenseState state_;
    };

    void testLocalLicenseStoreSaveAndLoad() {
        FakeLocalLicenseStore store;

        const KeygenSDK::LocalLicenseState expected{
            .licenseId = "test-license-id",
            .licenseKey = "TEST-LICENSE-KEY",
            .machineId = "test-machine-id",
            .machineFingerprint = "test-fingerprint"
        };

        const auto saveResult = store.save(expected);

        assert(saveResult.success());

        KeygenSDK::LocalLicenseState actual;

        const auto loadResult = store.load(actual);

        assert(loadResult.success());
        assert(actual.licenseId == expected.licenseId);
        assert(actual.licenseKey == expected.licenseKey);
        assert(actual.machineId == expected.machineId);
        assert(actual.machineFingerprint == expected.machineFingerprint);
    }

    void testLocalLicenseStoreLoadMissingState() {
        FakeLocalLicenseStore store;

        KeygenSDK::LocalLicenseState state;

        const auto result = store.load(state);

        assert(!result.success());
        assert(result.error == KeygenSDK::ErrorCode::LocalStateNotFound);
    }

    void testLocalLicenseStoreRemove() {
        FakeLocalLicenseStore store;

        const auto saveResult = store.save(
            KeygenSDK::LocalLicenseState{
                .licenseId = "test-license-id",
                .licenseKey = "TEST-LICENSE-KEY",
                .machineId = "test-machine-id",
                .machineFingerprint = "test-fingerprint"
            });

        assert(saveResult.success());

        const auto removeResult = store.remove();

        assert(removeResult.success());

        KeygenSDK::LocalLicenseState state;

        const auto loadResult = store.load(state);

        assert(!loadResult.success());
        assert(loadResult.error == KeygenSDK::ErrorCode::LocalStateNotFound);
    }

} // namespace KeygenSDKTests