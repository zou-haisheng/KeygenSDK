#include "KeygenSDK/MachineFileVerifier.h"

namespace KeygenSDK {

    Result MachineFileVerifier::verify(
        std::string_view machineFile,
        std::string& verifiedPayload) {

        verifiedPayload.clear();

        if (machineFile.empty()) {
            return Result::failure(
                ErrorCode::OfflineDataMissing,
                "Machine file is empty.");
        }

        return Result::failure(
            ErrorCode::OfflineVerificationFailed,
            "Machine file verification is not implemented yet.");
    }

} // namespace KeygenSDK