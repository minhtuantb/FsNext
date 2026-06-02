// SPDX-License-Identifier: Proprietary
#include "core/crypto/VaultMetadata.h"

#include <sodium.h>
#include <json/json.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fsnext::crypto {

namespace {

namespace fs = std::filesystem;

std::string b64(const std::uint8_t *d, std::size_t n)
{
    if (n == 0)
        return std::string();
    std::string s;
    s.resize(sodium_base64_encoded_len(n, sodium_base64_VARIANT_ORIGINAL));
    sodium_bin2base64(s.data(), s.size(), d, n, sodium_base64_VARIANT_ORIGINAL);
    s.resize(std::strlen(s.c_str()));  // drop the trailing NUL
    return s;
}

std::string b64(const std::vector<std::uint8_t> &v)
{
    return b64(v.data(), v.size());
}

// Decode base64 into `out`. If `expected` != 0, the decoded length must match.
bool unb64(const std::string &s, std::vector<std::uint8_t> &out, std::size_t expected)
{
    out.assign(s.size(), 0);  // upper bound
    std::size_t outlen = 0;
    if (sodium_base642bin(out.data(), out.size(), s.data(), s.size(), nullptr, &outlen, nullptr,
                          sodium_base64_VARIANT_ORIGINAL) != 0)
        return false;
    out.resize(outlen);
    return expected == 0 || outlen == expected;
}

} // namespace

CryptoError VaultMetadata::load(const std::string &path, VaultMetadata &out)
{
    std::ifstream f(fs::u8path(path), std::ios::binary);
    if (!f)
        return CryptoError::IoError;
    std::ostringstream ss;
    ss << f.rdbuf();
    const std::string text = ss.str();

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream is(text);
    if (!Json::parseFromStream(builder, is, &root, &errs) || !root.isObject())
        return CryptoError::InvalidFormat;

    out = VaultMetadata{};
    out.version    = root.get("version", 1).asInt();
    out.vaultId    = root.get("vaultId", "").asString();
    out.name       = root.get("name", "").asString();
    out.createdAt  = root.get("createdAt", Json::Value::Int64(0)).asInt64();
    out.autoLockMin = root.get("autoLockMin", 15).asInt();

    const Json::Value &kdf = root["kdf"];
    out.kdfOps      = kdf.get("ops", 3u).asUInt();
    out.kdfMemKb    = kdf.get("memKb", 65536u).asUInt();
    out.kdfParallel = static_cast<std::uint8_t>(kdf.get("parallel", 1u).asUInt());

    std::vector<std::uint8_t> salt;
    if (!unb64(kdf.get("salt", "").asString(), salt, 16))
        return CryptoError::InvalidFormat;
    std::memcpy(out.kdfSalt, salt.data(), 16);

    if (!unb64(root.get("wrappedMasterKey", "").asString(), out.wrappedMasterKey, 48))
        return CryptoError::InvalidFormat;
    if (!unb64(root.get("wrapNonce", "").asString(), out.wrapNonce, 24))
        return CryptoError::InvalidFormat;

    out.hasRecoveryKey = root.get("hasRecoveryKey", false).asBool();
    if (out.hasRecoveryKey) {
        if (!unb64(root.get("wrappedMasterKeyRecovery", "").asString(),
                   out.wrappedMasterKeyRecovery, 48))
            return CryptoError::InvalidFormat;
        if (!unb64(root.get("recoveryNonce", "").asString(), out.recoveryNonce, 24))
            return CryptoError::InvalidFormat;
    }

    out.storageLevel = root.get("storageLevel", 2).asInt();
    return CryptoError::Ok;
}

CryptoError VaultMetadata::save(const std::string &path) const
{
    Json::Value root;
    root["version"]    = version;
    root["vaultId"]    = vaultId;
    root["name"]       = name;
    root["createdAt"]  = static_cast<Json::Value::Int64>(createdAt);
    root["autoLockMin"] = autoLockMin;

    Json::Value kdf;
    kdf["algo"]     = "argon2id";
    kdf["ops"]      = static_cast<Json::UInt>(kdfOps);
    kdf["memKb"]    = static_cast<Json::UInt>(kdfMemKb);
    kdf["parallel"] = static_cast<Json::UInt>(kdfParallel);
    kdf["salt"]     = b64(kdfSalt, 16);
    root["kdf"]     = kdf;

    root["wrappedMasterKey"] = b64(wrappedMasterKey);
    root["wrapNonce"]        = b64(wrapNonce);
    root["hasRecoveryKey"]   = hasRecoveryKey;
    if (hasRecoveryKey) {
        root["wrappedMasterKeyRecovery"] = b64(wrappedMasterKeyRecovery);
        root["recoveryNonce"]            = b64(recoveryNonce);
    }
    root["storageLevel"] = storageLevel;

    Json::StreamWriterBuilder writer;
    const std::string text = Json::writeString(writer, root);

    const fs::path p   = fs::u8path(path);
    fs::path       tmp = p;
    tmp += ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f)
            return CryptoError::IoError;
        f.write(text.data(), static_cast<std::streamsize>(text.size()));
        f.flush();
        if (!f)
            return CryptoError::IoError;
    }
    std::error_code ec;
    fs::rename(tmp, p, ec);
    if (ec) {
        // Windows rename fails if the destination exists — remove then retry.
        fs::remove(p, ec);
        fs::rename(tmp, p, ec);
        if (ec)
            return CryptoError::IoError;
    }
    return CryptoError::Ok;
}

} // namespace fsnext::crypto
