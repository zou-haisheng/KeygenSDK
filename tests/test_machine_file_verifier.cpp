
#include <KeygenSDK/MachineFileVerifier.h>

#include <nlohmann/json.hpp>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace KeygenSDKTests {
    namespace {

        using Json = nlohmann::json;

        using EvpPkeyPtr =
            std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;

        using EvpPkeyCtxPtr =
            std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;

        using EvpMdCtxPtr =
            std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

        using EvpCipherCtxPtr =
            std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

        constexpr std::string_view kLicenseKey =
            "TEST-MACHINE-FILE-LICENSE-KEY";

        std::string base64Encode(
            const unsigned char* data,
            std::size_t size) {

            std::string encoded(4 * ((size + 2) / 3), '\0');

            const int length = EVP_EncodeBlock(
                reinterpret_cast<unsigned char*>(encoded.data()),
                data,
                static_cast<int>(size));

            assert(length >= 0);
            encoded.resize(static_cast<std::size_t>(length));

            return encoded;
        }

        std::string hexEncode(
            const unsigned char* data,
            std::size_t size) {

            constexpr char digits[] = "0123456789abcdef";

            std::string output;
            output.reserve(size * 2);

            for (std::size_t i = 0; i < size; ++i) {
                output.push_back(digits[data[i] >> 4]);
                output.push_back(digits[data[i] & 0x0F]);
            }

            return output;
        }

        struct TestKeyPair {
            EvpPkeyPtr privateKey{ nullptr, EVP_PKEY_free };
            std::string publicKeyHex;
        };

        TestKeyPair makeKeyPair() {
            EvpPkeyCtxPtr context(
                EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr),
                EVP_PKEY_CTX_free);

            assert(context);
            assert(EVP_PKEY_keygen_init(context.get()) == 1);

            EVP_PKEY* rawPrivateKey = nullptr;

            assert(
                EVP_PKEY_keygen(context.get(), &rawPrivateKey) == 1);

            TestKeyPair result;
            result.privateKey.reset(rawPrivateKey);

            unsigned char publicKey[32]{};
            std::size_t publicKeyLength = sizeof(publicKey);

            assert(
                EVP_PKEY_get_raw_public_key(
                    result.privateKey.get(),
                    publicKey,
                    &publicKeyLength) == 1);

            assert(publicKeyLength == sizeof(publicKey));

            result.publicKeyHex =
                hexEncode(publicKey, publicKeyLength);

            return result;
        }

        std::string signMessage(
            EVP_PKEY* privateKey,
            std::string_view message) {

            EvpMdCtxPtr context(
                EVP_MD_CTX_new(),
                EVP_MD_CTX_free);

            assert(context);

            assert(
                EVP_DigestSignInit(
                    context.get(),
                    nullptr,
                    nullptr,
                    nullptr,
                    privateKey) == 1);

            std::size_t signatureLength = 0;

            assert(
                EVP_DigestSign(
                    context.get(),
                    nullptr,
                    &signatureLength,
                    reinterpret_cast<const unsigned char*>(message.data()),
                    message.size()) == 1);

            std::vector<unsigned char> signature(signatureLength);

            assert(
                EVP_DigestSign(
                    context.get(),
                    signature.data(),
                    &signatureLength,
                    reinterpret_cast<const unsigned char*>(message.data()),
                    message.size()) == 1);

            signature.resize(signatureLength);

            return base64Encode(
                signature.data(),
                signature.size());
        }

        std::string encryptPayload(
            std::string_view licenseKey,
            std::string_view payload) {

            unsigned char aesKey[SHA256_DIGEST_LENGTH]{};

            assert(
                SHA256(
                    reinterpret_cast<const unsigned char*>(licenseKey.data()),
                    licenseKey.size(),
                    aesKey) != nullptr);

            unsigned char iv[12]{};
            assert(RAND_bytes(iv, sizeof(iv)) == 1);

            EvpCipherCtxPtr context(
                EVP_CIPHER_CTX_new(),
                EVP_CIPHER_CTX_free);

            assert(context);

            assert(
                EVP_EncryptInit_ex(
                    context.get(),
                    EVP_aes_256_gcm(),
                    nullptr,
                    nullptr,
                    nullptr) == 1);

            assert(
                EVP_CIPHER_CTX_ctrl(
                    context.get(),
                    EVP_CTRL_GCM_SET_IVLEN,
                    sizeof(iv),
                    nullptr) == 1);

            assert(
                EVP_EncryptInit_ex(
                    context.get(),
                    nullptr,
                    nullptr,
                    aesKey,
                    iv) == 1);

            std::vector<unsigned char> ciphertext(payload.size() + 16);

            int ciphertextLength = 0;

            assert(
                EVP_EncryptUpdate(
                    context.get(),
                    ciphertext.data(),
                    &ciphertextLength,
                    reinterpret_cast<const unsigned char*>(payload.data()),
                    static_cast<int>(payload.size())) == 1);

            int finalLength = 0;

            assert(
                EVP_EncryptFinal_ex(
                    context.get(),
                    ciphertext.data() + ciphertextLength,
                    &finalLength) == 1);

            ciphertext.resize(
                static_cast<std::size_t>(ciphertextLength + finalLength));

            unsigned char tag[16]{};

            assert(
                EVP_CIPHER_CTX_ctrl(
                    context.get(),
                    EVP_CTRL_GCM_GET_TAG,
                    sizeof(tag),
                    tag) == 1);

            return base64Encode(ciphertext.data(), ciphertext.size()) + "." +
                base64Encode(iv, sizeof(iv)) + "." +
                base64Encode(tag, sizeof(tag));
        }

        std::string makeMachineFile(
            EVP_PKEY* privateKey,
            std::string_view licenseKey,
            std::string_view payload,
            std::string_view algorithm = "aes-256-gcm+ed25519") {

            const std::string enc =
                encryptPayload(licenseKey, payload);

            const std::string message = "machine/" + enc;

            Json certificate = {
                { "enc", enc },
                { "sig", signMessage(privateKey, message) },
                { "alg", algorithm }
            };

            const std::string certificateJson = certificate.dump();

            return "-----BEGIN MACHINE FILE-----\n" +
                base64Encode(
                    reinterpret_cast<const unsigned char*>(
                        certificateJson.data()),
                    certificateJson.size()) +
                "\n-----END MACHINE FILE-----";
        }

        void testValidMachineFile() {
            auto keyPair = makeKeyPair();

            const std::string payload =
                R"({"data":{"id":"machine-123","type":"machines"},"meta":{"ttl":3600}})";

            const std::string machineFile = makeMachineFile(
                keyPair.privateKey.get(),
                kLicenseKey,
                payload);

            std::string verifiedPayload = "must be replaced";

            const auto result = KeygenSDK::MachineFileVerifier::verify(
                machineFile,
                keyPair.publicKeyHex,
                kLicenseKey,
                verifiedPayload);

            assert(result.ok);
            assert(result.error == KeygenSDK::ErrorCode::None);
            assert(verifiedPayload == payload);
        }

        void testMachineFileRejectsWrongPublicKey() {
            auto signingKey = makeKeyPair();
            auto unrelatedKey = makeKeyPair();

            const std::string machineFile = makeMachineFile(
                signingKey.privateKey.get(),
                kLicenseKey,
                R"({"data":{"id":"machine-123","type":"machines"}})");

            std::string verifiedPayload;

            const auto result = KeygenSDK::MachineFileVerifier::verify(
                machineFile,
                unrelatedKey.publicKeyHex,
                kLicenseKey,
                verifiedPayload);

            assert(!result.ok);
            assert(
                result.error ==
                KeygenSDK::ErrorCode::OfflineVerificationFailed);
            assert(verifiedPayload.empty());
        }

        void testMachineFileRejectsWrongLicenseKey() {
            auto keyPair = makeKeyPair();

            const std::string machineFile = makeMachineFile(
                keyPair.privateKey.get(),
                kLicenseKey,
                R"({"data":{"id":"machine-123","type":"machines"}})");

            std::string verifiedPayload;

            const auto result = KeygenSDK::MachineFileVerifier::verify(
                machineFile,
                keyPair.publicKeyHex,
                "WRONG-LICENSE-KEY",
                verifiedPayload);

            assert(!result.ok);
            assert(
                result.error ==
                KeygenSDK::ErrorCode::OfflineVerificationFailed);
            assert(verifiedPayload.empty());
        }

        void testMachineFileRejectsInvalidWrapper() {
            std::string verifiedPayload = "must be cleared";

            const auto result = KeygenSDK::MachineFileVerifier::verify(
                "not a machine file",
                std::string(64, '0'),
                kLicenseKey,
                verifiedPayload);

            assert(!result.ok);
            assert(
                result.error ==
                KeygenSDK::ErrorCode::OfflineVerificationFailed);
            assert(verifiedPayload.empty());
        }

        void testMachineFileRejectsUnsupportedAlgorithm() {
            auto keyPair = makeKeyPair();

            const std::string machineFile = makeMachineFile(
                keyPair.privateKey.get(),
                kLicenseKey,
                R"({"data":{"id":"machine-123","type":"machines"}})",
                "aes-256-gcm+unsupported");

            std::string verifiedPayload;

            const auto result = KeygenSDK::MachineFileVerifier::verify(
                machineFile,
                keyPair.publicKeyHex,
                kLicenseKey,
                verifiedPayload);

            assert(!result.ok);
            assert(
                result.error ==
                KeygenSDK::ErrorCode::OfflineVerificationFailed);
            assert(verifiedPayload.empty());
        }

        void testMachineFileRejectsTamperedSignature() {
            auto keyPair = makeKeyPair();

            std::string machineFile = makeMachineFile(
                keyPair.privateKey.get(),
                kLicenseKey,
                R"({"data":{"id":"machine-123","type":"machines"}})");

            const auto endMarker =
                machineFile.find("-----END MACHINE FILE-----");

            assert(endMarker != std::string::npos);

            // Alter a character in the Base64-encoded certificate body.
            // The outer Base64 is intentionally corrupted, so parsing must fail.
            const auto bodyPosition = machineFile.find('\n') + 1;

            assert(bodyPosition > 0 && bodyPosition < endMarker);

            machineFile[bodyPosition] =
                machineFile[bodyPosition] == 'A' ? 'B' : 'A';

            std::string verifiedPayload;

            const auto result = KeygenSDK::MachineFileVerifier::verify(
                machineFile,
                keyPair.publicKeyHex,
                kLicenseKey,
                verifiedPayload);

            assert(!result.ok);
            assert(
                result.error ==
                KeygenSDK::ErrorCode::OfflineVerificationFailed);
            assert(verifiedPayload.empty());
        }

    } // namespace

    void testMachineFileVerifier() {
        testValidMachineFile();
        testMachineFileRejectsWrongPublicKey();
        testMachineFileRejectsWrongLicenseKey();
        testMachineFileRejectsInvalidWrapper();
        testMachineFileRejectsUnsupportedAlgorithm();
        testMachineFileRejectsTamperedSignature();
    }

} // namespace KeygenSDKTests