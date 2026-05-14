// SPDX-License-Identifier: Proprietary
#pragma once
#include <QString>

namespace fsnext::FileNameSanitizer {

// Produce a filesystem-safe filename from an untrusted string (typically a
// filename derived from a server-returned URL). Replaces path separators,
// strips control characters, substitutes Windows-reserved characters, trims
// trailing dots/spaces, and escapes Windows DOS device names (CON, PRN, AUX,
// NUL, COM1-9, LPT1-9). Returns "download" if the resulting string is empty.
//
// The output is clamped to 200 bytes/chars — well under the 255-code-unit
// filename cap enforced by NTFS, APFS, and most Linux filesystems — with
// extension (last `.ext`, if present) preserved when truncating.
QString sanitize(const QString &name);

// Conflict resolution policy — see ADR 003 D7.
enum class ConflictPolicy : int {
    Rename    = 0,   // append " (1)", " (2)", … until unique  (default)
    Overwrite = 1,
    Skip      = 2,
    Ask       = 3,   // caller is responsible for prompting; this util returns
                     // a path that *would* be the rename outcome so the
                     // prompt can show the user the rename suggestion.
};

// Given a target directory + desired filename and the active policy, returns
// the absolute path the caller should write to.
//
//   • policy == Overwrite → returns dir + "/" + sanitized(name) directly.
//   • policy == Skip      → returns empty QString when the file exists.
//   • policy == Rename    → appends " (N)" before the extension until the
//                           candidate doesn't exist on disk.
//   • policy == Ask       → returns the same path Rename would (caller can
//                           still choose Overwrite/Skip after prompting).
//
// `name` is sanitized internally so callers can pass server-provided names
// directly. Caller-provided directory must already exist.
QString resolveConflict(const QString &dir, const QString &name, ConflictPolicy policy);

} // namespace fsnext::FileNameSanitizer
