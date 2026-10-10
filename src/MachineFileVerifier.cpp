#include "KeygenSDK/MachineFileVerifier.h"

#include <nlohmann/json.hpp>

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <algorithm>
#include <cctype>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace KeygenSDK {
    namespace {

        using Json = nlohmann::json;

        using EvpPkeyPtr =
            std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;

        using EvpMdCtxPtr =
            std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

        using EvpCipherCtxPtr =
            std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

        Result verificationFailure(std::string message) {
            return Result::failure(
                ErrorCode::OfflineVerificationFailed,
                std::move(message));
        }

        bool base64Decode(
            std::string_view input,
            std::vector<unsigned char>& output) {

            output.clear();

            std::string compact;
            compact.reserve(input.size());

            for (const unsigned char ch : input) {
                if (std::isspace(ch)) {
                    continue;
                }

                compact.push_back(static_cast<char>(ch));
            }

            if (compact.empty() || compact.size() % 4 != 0) {
                return false;
            }

            for (std::size_t i = 0; i < compact.size(); ++i) {
                const unsigned char ch =
                    static_cast<unsigned char>(compact[i]);

                const bool isBase64Character =
                    (ch >= 'A' && ch <= 'Z') ||
                    (ch >= 'a' && ch <= 'z') ||
                    (ch >= '0' && ch <= '9') ||
                    ch == '+' || ch == '/';

                if (ch == '=') {
                    if (i < compact.size() - 2) {
                        return false;
                    }
                }
                else if (!isBase64Character) {
                    return false;
                }
            }

            const auto inputLength =
                static_cast<int>(compact.size());

            output.resize((compact.size() / 4) * 3);

            const int decodedLength = EVP_DecodeBlock(
                output.data(),
                reinterpret_cast<const unsigned char*>(compact.data()),
                inputLength);

            if (decodedLength < 0) {
                output.clear();
                return false;
            }

            std::size_t padding = 0;

            if (!compact.empty() && compact.back() == '=') {
                ++padding;
            }

            if (compact.size() >= 2 &&
                compact[compact.size() - 2] == '=') {
                ++padding;
            }

            if (static_cast<std::size_t>(decodedLength) < padding) {
                output.clear();
                return false;
            }

            output.resize(
                static_cast<std::size_t>(decodedLength) - padding);

            return true;
        }

        std::string trim(std::string_view value) {
            std::size_t first = 0;
            std::size_t last = value.size();

            while (first < last &&
                std::isspace(static_cast<unsigned char>(value[first]))) {
                ++first;
            }

            while (last > first &&
                std::isspace(static_cast<unsigned char>(value[last - 1]))) {
                --last;
            }

            return std::string(value.substr(first, last - first));
        }

        bool decodeHexPublicKey(
            std::string_view hex,
            std::vector<unsigned char>& key) {

            key.clear();

            if (hex.size() != 64) {
                return false;
            }

            const auto hexValue = [](char ch) -> int {
                if (ch >= '0' && ch <= '9') {
                    return ch - '0';
                }

                if (ch >= 'a' && ch <= 'f') {
                    return ch - 'a' + 10;
                }

                if (ch >= 'A' && ch <= 'F') {
                    return ch - 'A' + 10;
                }

                return -1;
                };

            key.reserve(32);

            for (std::size_t i = 0; i < hex.size(); i += 2) {
                const int high = hexValue(hex[i]);
                const int low = hexValue(hex[i + 1]);

                if (high < 0 || low < 0) {
                    key.clear();
                    return false;
                }

                key.push_back(
                    static_cast<unsigned char>((high << 4) | low));
            }

            return key.size() == 32;
        }

        bool splitEncryptedData(
            std::string_view encoded,
            std::string_view& ciphertext,
            std::string_view& iv,
            std::string_view& tag) {

            const auto firstDot = encoded.find('.');

            if (firstDot == std::string_view::npos) {
                return false;
            }

            const auto secondDot =
                encoded.find('.', firstDot + 1);

            if (secondDot == std::string_view::npos ||
                encoded.find('.', secondDot + 1) != std::string_view::npos) {
                return false;
            }

            ciphertext = encoded.substr(0, firstDot);
            iv = encoded.substr(
                firstDot + 1,
                secondDot - firstDot - 1);
            tag = encoded.substr(secondDot + 1);

            return !ciphertext.empty() &&
                !iv.empty() &&
                !tag.empty();
        }

        bool verifyEd25519Signature(
            std::string_view encodedPublicKey,
            std::string_view encodedSignature,
            std::string_view message) {

            std::vector<unsigned char> publicKeyBytes;
            std::vector<unsigned char> signatureBytes;

            if (!decodeHexPublicKey(
                encodedPublicKey,
                publicKeyBytes)) {
                return false;
            }

            if (!base64Decode(
                encodedSignature,
                signatureBytes) ||
                signatureBytes.size() != 64) {
                return false;
            }

            EvpPkeyPtr publicKey(
                EVP_PKEY_new_raw_public_key(
                    EVP_PKEY_ED25519,
                    nullptr,
                    publicKeyBytes.data(),
                    publicKeyBytes.size()),
                EVP_PKEY_free);

            if (!publicKey) {
                return false;
            }

            EvpMdCtxPtr context(
                EVP_MD_CTX_new(),
                EVP_MD_CTX_free);

            if (!context) {
                return false;
            }

            if (EVP_DigestVerifyInit(
                context.get(),
                nullptr,
                nullptr,
                nullptr,
                publicKey.get()) != 1) {
                return false;
            }

            return EVP_DigestVerify(
                context.get(),
                signatureBytes.data(),
                signatureBytes.size(),
                reinterpret_cast<const unsigned char*>(message.data()),
                message.size()) == 1;
        }

        bool decryptAes256Gcm(
            std::string_view encodedData,
            std::string_view licenseKey,
            std::string& plaintext) {

            plaintext.clear();

            std::string_view encodedCiphertext;
            std::string_view encodedIv;
            std::string_view encodedTag;

            if (!splitEncryptedData(
                encodedData,
                encodedCiphertext,
                encodedIv,
                encodedTag)) {
                return false;
            }

            std::vector<unsigned char> ciphertext;
            std::vector<unsigned char> iv;
            std::vector<unsigned char> tag;

            if (!base64Decode(encodedCiphertext, ciphertext) ||
                !base64Decode(encodedIv, iv) ||
                !base64Decode(encodedTag, tag)) {
                return false;
            }

            // Keygen derives the AES-256 key as SHA-256(license key).
            unsigned char aesKey[SHA256_DIGEST_LENGTH]{};

            if (SHA256(
                reinterpret_cast<const unsigned char*>(licenseKey.data()),
                licenseKey.size(),
                aesKey) == nullptr) {
                return false;
            }

            // AES-GCM uses a 12-byte IV and a 16-byte authentication tag.
            if (iv.size() != 12 || tag.size() != 16) {
                return false;
            }

            EvpCipherCtxPtr context(
                EVP_CIPHER_CTX_new(),
                EVP_CIPHER_CTX_free);

            if (!context) {
                return false;
            }

            if (EVP_DecryptInit_ex(
                context.get(),
                EVP_aes_256_gcm(),
                nullptr,
                nullptr,
                nullptr) != 1) {
                return false;
            }

            if (EVP_CIPHER_CTX_ctrl(
                context.get(),
                EVP_CTRL_GCM_SET_IVLEN,
                static_cast<int>(iv.size()),
                nullptr) != 1) {
                return false;
            }

            if (EVP_DecryptInit_ex(
                context.get(),
                nullptr,
                nullptr,
                aesKey,
                iv.data()) != 1) {
                return false;
            }

            std::vector<unsigned char> combinedCiphertext =
                std::move(ciphertext);

            combinedCiphertext.insert(
                combinedCiphertext.end(),
                tag.begin(),
                tag.end());

            plaintext.resize(combinedCiphertext.size());

            int plaintextLength = 0;

            if (EVP_DecryptUpdate(
                context.get(),
                reinterpret_cast<unsigned char*>(plaintext.data()),
                &plaintextLength,
                combinedCiphertext.data(),
                static_cast<int>(combinedCiphertext.size())) != 1) {
                plaintext.clear();
                return false;
            }

            // GCM authentication is completed here. Never expose plaintext
            // unless the authentication tag has been accepted.
            int finalLength = 0;

            if (EVP_DecryptFinal_ex(
                context.get(),
                reinterpret_cast<unsigned char*>(plaintext.data()) +
                plaintextLength,
                &finalLength) != 1) {
                plaintext.clear();
                return false;
            }

            plaintext.resize(
                static_cast<std::size_t>(plaintextLength + finalLength));

            return true;
        }

    } // namespace

    Result MachineFileVerifier::verify(
        std::string_view machineFile,
        std::string_view accountPublicKey,
        std::string_view licenseKey,
        std::string& verifiedPayload) {

        verifiedPayload.clear();

        if (machineFile.empty()) {
            return Result::failure(
                ErrorCode::OfflineDataMissing,
                "Machine file is empty.");
        }

        if (accountPublicKey.empty() || licenseKey.empty()) {
            return verificationFailure(
                "Account public key and license key are required.");
        }

        constexpr std::string_view beginMarker =
            "-----BEGIN MACHINE FILE-----";
        constexpr std::string_view endMarker =
            "-----END MACHINE FILE-----";

        const std::string normalizedFile = trim(machineFile);

        if (!normalizedFile.starts_with(beginMarker) ||
            !normalizedFile.ends_with(endMarker)) {
            return verificationFailure(
                "Machine file wrapper is invalid.");
        }

        const std::string_view encodedCertificate(
            normalizedFile.data() + beginMarker.size(),
            normalizedFile.size() -
            beginMarker.size() -
            endMarker.size());

        std::vector<unsigned char> certificateBytes;

        if (!base64Decode(encodedCertificate, certificateBytes)) {
            return verificationFailure(
                "Machine file certificate is not valid Base64.");
        }

        Json certificate;

        try {
            certificate = Json::parse(
                certificateBytes.begin(),
                certificateBytes.end());
        }
        catch (const Json::exception&) {
            return verificationFailure(
                "Machine file certificate is not valid JSON.");
        }

        if (!certificate.is_object() ||
            !certificate.contains("enc") ||
            !certificate["enc"].is_string() ||
            !certificate.contains("sig") ||
            !certificate["sig"].is_string() ||
            !certificate.contains("alg") ||
            !certificate["alg"].is_string()) {
            return verificationFailure(
                "Machine file certificate is missing required fields.");
        }

        const std::string enc =
            certificate["enc"].get<std::string>();

        const std::string signature =
            certificate["sig"].get<std::string>();

        const std::string algorithm =
            certificate["alg"].get<std::string>();

        if (algorithm != "aes-256-gcm+ed25519") {
            return verificationFailure(
                "Machine file algorithm is not supported.");
        }

        // Keygen signs the exact string "machine/" + certificate.enc.
        const std::string signedMessage = "machine/" + enc;

        if (!verifyEd25519Signature(
            accountPublicKey,
            signature,
            signedMessage)) {
            return verificationFailure(
                "Machine file signature verification failed.");
        }

        std::string plaintext;

        if (!decryptAes256Gcm(
            enc,
            licenseKey,
            plaintext)) {
            return verificationFailure(
                "Machine file decryption or authentication failed.");
        }

        // Commit output only after signature verification and authenticated
        // decryption have both succeeded.
        verifiedPayload = std::move(plaintext);

        return Result::successResult(
            "Machine file signature verified and payload decrypted.");
    }

} // namespace KeygenSDK