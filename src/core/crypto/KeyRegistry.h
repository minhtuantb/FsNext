// SPDX-License-Identifier: Proprietary
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace fsnext::crypto {

/// One entry in the key registry — METADATA ONLY (no key material). Used by the
/// Key Manager page to list imported recovery keyfiles. Safe to store as plain
/// JSON (a fingerprint is one-way; the actual key never lives here).
struct KeyEntry {
    std::string  id;
    std::string  label;
    std::string  fingerprint;   // display hex, e.g. "9f3a 71c0 4e8d b21c"
    std::string  type;          // "keyfile" | "recovery" | "vault"
    std::int64_t createdAt = 0; // unix epoch seconds
};

/// Persistent list of imported keyfile entries (vault + recovery entries are
/// derived live by VaultViewModel and are NOT stored here). Pure (jsoncpp).
class KeyRegistry {
public:
    bool load(const std::string &path);   // false if file missing/corrupt (treated as empty)
    bool save(const std::string &path) const;

    const std::vector<KeyEntry> &entries() const { return entries_; }
    void add(const KeyEntry &e) { entries_.push_back(e); }
    bool remove(const std::string &id);
    void clear() { entries_.clear(); }

private:
    std::vector<KeyEntry> entries_;
};

} // namespace fsnext::crypto
