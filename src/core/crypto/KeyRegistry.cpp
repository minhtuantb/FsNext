// SPDX-License-Identifier: Proprietary
#include "core/crypto/KeyRegistry.h"

#include <json/json.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fsnext::crypto {

namespace fs = std::filesystem;

bool KeyRegistry::load(const std::string &path)
{
    entries_.clear();
    std::ifstream f(fs::u8path(path), std::ios::binary);
    if (!f)
        return false;
    std::ostringstream ss;
    ss << f.rdbuf();

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream is(ss.str());
    if (!Json::parseFromStream(builder, is, &root, &errs) || !root.isObject())
        return false;
    const Json::Value &arr = root["keys"];
    if (!arr.isArray())
        return false;
    for (const auto &k : arr) {
        KeyEntry e;
        e.id          = k.get("id", "").asString();
        e.label       = k.get("label", "").asString();
        e.fingerprint = k.get("fingerprint", "").asString();
        e.type        = k.get("type", "keyfile").asString();
        e.createdAt   = k.get("createdAt", Json::Value::Int64(0)).asInt64();
        if (!e.id.empty())
            entries_.push_back(e);
    }
    return true;
}

bool KeyRegistry::save(const std::string &path) const
{
    Json::Value root;
    Json::Value arr(Json::arrayValue);
    for (const auto &e : entries_) {
        Json::Value k;
        k["id"]          = e.id;
        k["label"]       = e.label;
        k["fingerprint"] = e.fingerprint;
        k["type"]        = e.type;
        k["createdAt"]   = static_cast<Json::Value::Int64>(e.createdAt);
        arr.append(k);
    }
    root["keys"] = arr;

    Json::StreamWriterBuilder writer;
    const std::string text = Json::writeString(writer, root);

    const fs::path p = fs::u8path(path);
    fs::path tmp = p;
    tmp += ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out)
            return false;
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        out.flush();
        if (!out)
            return false;
    }
    std::error_code ec;
    fs::rename(tmp, p, ec);
    if (ec) {
        fs::remove(p, ec);
        fs::rename(tmp, p, ec);
        if (ec)
            return false;
    }
    return true;
}

bool KeyRegistry::remove(const std::string &id)
{
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->id == id) {
            entries_.erase(it);
            return true;
        }
    }
    return false;
}

} // namespace fsnext::crypto
