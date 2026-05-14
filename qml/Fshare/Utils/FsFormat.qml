// SPDX-License-Identifier: Proprietary
// FsFormat — Centralized human-readable formatters for QML.
//
// Usage:
//   import Fshare.Utils
//   Text { text: FsFormat.bytes(fileSize) }
//   Text { text: FsFormat.dateTime(modified) }
//
// Mirrors src/core/util/FormatUtil (C++). Keep algorithms in sync.

pragma Singleton
import QtQuick

QtObject {
    // ── Bytes ──────────────────────────────────────────────────────────
    // Human-readable size. `emptyOnZero = true` → returns "" for 0/negative
    // (useful for folders where size is meaningless).
    function bytes(b, emptyOnZero) {
        if (b === undefined || b === null || b <= 0)
            return emptyOnZero ? "" : "0 B";
        const units = ["B", "KB", "MB", "GB", "TB", "PB"];
        let i = 0;
        let v = +b;
        while (v >= 1024 && i < units.length - 1) { v /= 1024; i++; }
        let decimals;
        if (i === 0)          decimals = 0;      // bytes: integer
        else if (v >= 100)    decimals = 0;
        else if (v >= 10)     decimals = 1;
        else                  decimals = 2;
        return v.toFixed(decimals) + " " + units[i];
    }

    // Bytes per second, e.g. "1.5 MB/s". Empty string for 0/negative.
    function speed(bps) {
        if (bps === undefined || bps === null || bps <= 0) return "";
        return bytes(bps) + "/s";
    }

    // ── Timestamps ─────────────────────────────────────────────────────
    // Parse Fshare API timestamp (unix epoch seconds string, or ISO-8601)
    // → JS Date. Returns null for invalid/empty.
    function _parse(s) {
        if (s === undefined || s === null || s === "") return null;
        // Fast path: numeric epoch seconds.
        const n = Number(s);
        if (Number.isFinite(n) && String(n) === String(s).trim()) {
            if (n <= 0) return null;
            return new Date(n * 1000);
        }
        const d = new Date(s);
        return isNaN(d.getTime()) ? null : d;
    }

    // "dd/MM/yyyy HH:mm" (fixed format — consistent with C++ FormatUtil).
    function dateTime(s, fallback) {
        const d = _parse(s);
        if (!d) return fallback !== undefined ? fallback : "—";
        return Qt.formatDateTime(d, "dd/MM/yyyy HH:mm");
    }

    // Date only, localized short format.
    function date(s, fallback) {
        const d = _parse(s);
        if (!d) return fallback !== undefined ? fallback : "—";
        return Qt.formatDate(d, Locale.ShortFormat);
    }
}
