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
#include <charconv>
#include <chrono>
#include <optional>

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

        std::optional<std::chrono::sys_time<std::chrono::milliseconds>>
            parseIso8601Timestamp(std::string_view value) {
            using namespace std::chrono;

            const auto parseDigits = [](
                std::string_view text,
                std::size_t position,
                std::size_t count,
                int& result) -> bool {
                    if (position + count > text.size()) {
                        return false;
                    }

                    for (std::size_t i = position; i < position + count; ++i) {
                        if (text[i] < '0' || text[i] > '9') {
                            return false;
                        }
                    }

                    const char* first = text.data() + position;
                    const char* end = first + count;
                    const auto parsed = std::from_chars(first, end, result);

                    return parsed.ec == std::errc{} &&
                        parsed.ptr == end;
                };

            if (value.size() < 20 ||
                value[4] != '-' ||
                value[7] != '-' ||
                (value[10] != 'T' && value[10] != 't') ||
                value[13] != ':' ||
                value[16] != ':') {
                return std::nullopt;
            }

            int parsedYear = 0;
            int parsedMonth = 0;
            int parsedDay = 0;
            int parsedHour = 0;
            int parsedMinute = 0;
            int parsedSecond = 0;

            if (!parseDigits(value, 0, 4, parsedYear) ||
                !parseDigits(value, 5, 2, parsedMonth) ||
                !parseDigits(value, 8, 2, parsedDay) ||
                !parseDigits(value, 11, 2, parsedHour) ||
                !parseDigits(value, 14, 2, parsedMinute) ||
                !parseDigits(value, 17, 2, parsedSecond)) {
                return std::nullopt;
            }

            const year_month_day date{
                year{ parsedYear },
                month{ static_cast<unsigned>(parsedMonth) },
                day{ static_cast<unsigned>(parsedDay) }
            };

            if (!date.ok() ||
                parsedHour > 23 ||
                parsedMinute > 59 ||
                parsedSecond > 59) {
                return std::nullopt;
            }

            std::size_t position = 19;
            int fractionalMilliseconds = 0;

            if (position < value.size() && value[position] == '.') {
                ++position;

                std::size_t fractionDigits = 0;

                while (position < value.size() &&
                    value[position] >= '0' &&
                    value[position] <= '9') {
                    if (fractionDigits < 3) {
                        fractionalMilliseconds =
                            fractionalMilliseconds * 10 +
                            (value[position] - '0');
                    }

                    ++fractionDigits;
                    ++position;
                }

                if (fractionDigits == 0) {
                    return std::nullopt;
                }

                if (fractionDigits == 1) {
                    fractionalMilliseconds *= 100;
                }
                else if (fractionDigits == 2) {
                    fractionalMilliseconds *= 10;
                }

            }

            int offsetMinutes = 0;

            if (position < value.size() &&
                (value[position] == 'Z' || value[position] == 'z')) {
                ++position;
            }
            else if (position < value.size() &&
                (value[position] == '+' || value[position] == '-')) {
                const int sign = value[position] == '+' ? 1 : -1;

                if (position + 6 != value.size() ||
                    value[position + 3] != ':') {
                    return std::nullopt;
                }

                int offsetHours = 0;
                int offsetRemainderMinutes = 0;

                if (!parseDigits(value, position + 1, 2, offsetHours) ||
                    !parseDigits(
                        value,
                        position + 4,
                        2,
                        offsetRemainderMinutes) ||
                    offsetHours > 23 ||
                    offsetRemainderMinutes > 59) {
                    return std::nullopt;
                }

                offsetMinutes =
                    sign * (offsetHours * 60 + offsetRemainderMinutes);
                position += 6;
            }
            else {
                return std::nullopt;
            }

            if (position != value.size()) {
                return std::nullopt;
            }

            const auto localTime =
                sys_days{ date } +
                hours{ parsedHour } +
                minutes{ parsedMinute } +
                seconds{ parsedSecond } +
                std::chrono::milliseconds{ fractionalMilliseconds };

            return time_point_cast<milliseconds>(
                localTime - minutes{ offsetMinutes });
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
            std::string_view machineFingerprint,
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


            const std::string secret =
                std::string(licenseKey) + std::string(machineFingerprint);

            unsigned char aesKey[SHA256_DIGEST_LENGTH]{};

            if (SHA256(
                reinterpret_cast<const unsigned char*>(secret.data()),
                secret.size(),
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

            // Decrypt only the ciphertext. The GCM authentication tag must
            // be supplied separately through EVP_CTRL_GCM_SET_TAG.
            plaintext.resize(ciphertext.size());

            int plaintextLength = 0;

            if (EVP_DecryptUpdate(
                context.get(),
                reinterpret_cast<unsigned char*>(plaintext.data()),
                &plaintextLength,
                ciphertext.data(),
                static_cast<int>(ciphertext.size())) != 1) {
                plaintext.clear();
                return false;
            }

            // Configure the expected authentication tag before finalization.
            if (EVP_CIPHER_CTX_ctrl(
                context.get(),
                EVP_CTRL_GCM_SET_TAG,
                static_cast<int>(tag.size()),
                tag.data()) != 1) {
                plaintext.clear();
                return false;
            }

            // Authentication is verified here. Do not expose plaintext if it fails.
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
        std::string_view machineFingerprint,
        std::string& verifiedPayload) {
        verifiedPayload.clear();

        if (machineFile.empty()) {
            return Result::failure(
                ErrorCode::OfflineDataMissing,
                "Machine file is empty.");
        }

        if (accountPublicKey.empty() ||
            licenseKey.empty() ||
            machineFingerprint.empty()) {
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
            machineFingerprint,
            plaintext)) {
            return verificationFailure(
                "Machine file decryption or authentication failed.");
        }

        Json payload;

        try {
            payload = Json::parse(plaintext);
        }
        catch (const Json::exception&) {
            return verificationFailure(
                "Machine file payload is not valid JSON.");
        }

        if (!payload.is_object() ||
            !payload.contains("data") ||
            !payload["data"].is_object()) {
            return verificationFailure(
                "Machine file payload must contain a data object.");
        }

        const Json& machine = payload["data"];

        if (!machine.contains("id") ||
            !machine["id"].is_string() ||
            machine["id"].get_ref<const std::string&>().empty() ||
            !machine.contains("type") ||
            !machine["type"].is_string() ||
            machine["type"] != "machines") {
            return verificationFailure(
                "Machine file payload does not contain a valid machine resource.");
        }

        if ((machine.contains("attributes") &&
            !machine["attributes"].is_object()) ||
            (machine.contains("relationships") &&
                !machine["relationships"].is_object()) ||
            (payload.contains("meta") &&
                !payload["meta"].is_object()) ||
            (payload.contains("included") &&
                !payload["included"].is_array())) {
            return verificationFailure(
                "Machine file payload contains invalid JSON:API structures.");
        }

        if (!payload.contains("meta") ||
            !payload["meta"].is_object() ||
            !payload["meta"].contains("issued") ||
            !payload["meta"]["issued"].is_string() ||
            !payload["meta"].contains("expiry")) {
            return verificationFailure(
                "Machine file payload is missing TTL metadata.");
        }

        const auto issued = parseIso8601Timestamp(
            payload["meta"]["issued"].get_ref<const std::string&>());

        if (!issued) {
            return verificationFailure(
                "Machine file issued timestamp is invalid.");
        }

        const auto now =
            std::chrono::time_point_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now());

        if (*issued > now) {
            return verificationFailure(
                "Machine file was issued in the future.");
        }

        if (!payload["meta"].contains("ttl")) {
            return verificationFailure(
                "Machine file payload is missing TTL metadata.");
        }

        const Json& ttl = payload["meta"]["ttl"];

        if (!ttl.is_null() &&
            (!ttl.is_number_integer() ||
                ttl.get<std::int64_t>() < 3600)) {
            return verificationFailure(
                "Machine file TTL must be null or an integer of at least 3600 seconds.");
        }

        const Json& expiry = payload["meta"]["expiry"];

        if (ttl.is_null() != expiry.is_null()) {
            return verificationFailure(
                "Machine file TTL and expiry metadata are inconsistent.");
        }

        if (expiry.is_string()) {
            const auto parsedExpiry = parseIso8601Timestamp(
                expiry.get_ref<const std::string&>());

            if (!parsedExpiry) {
                return verificationFailure(
                    "Machine file expiry timestamp is invalid.");
            }

            if (*parsedExpiry < *issued) {
                return verificationFailure(
                    "Machine file expiry precedes its issued timestamp.");
            }

            if (*parsedExpiry < now) {
                return Result::failure(
                    ErrorCode::LicenseExpired,
                    "Machine file has expired.");
            }
        }
        else if (!expiry.is_null()) {
            return verificationFailure(
                "Machine file expiry must be a timestamp or null.");
        }

        if (machine.contains("attributes")) {
            const Json& attributes = machine["attributes"];

            if (attributes.contains("fingerprint") &&
                (!attributes["fingerprint"].is_string() ||
                    std::string_view(
                        attributes["fingerprint"].get_ref<const std::string&>()) !=
                    machineFingerprint)) {
                return verificationFailure(
                    "Machine file fingerprint does not match this machine.");
            }
        }

        // Preserve the original plaintext after its JSON structure is valid.
        verifiedPayload = std::move(plaintext);

        return Result::successResult(
            "Machine file signature verified and payload decrypted.");
    }

} // namespace KeygenSDK