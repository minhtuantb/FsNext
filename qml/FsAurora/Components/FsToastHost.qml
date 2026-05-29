// SPDX-License-Identifier: Proprietary
// FsToastHost — Toast queue manager (Aurora).
//
// Replaces the previous "anchor offset by hand" pattern in Main.qml. Drop one
// instance into Main.qml (or any ApplicationWindow); call show() from any
// Connections handler:
//
//   toastHost.show({
//       title:       qsTr("Đã sao chép link"),
//       desc:        "https://fshare.vn/file/ABC123",
//       variant:     "success",       // info | success | warning | error
//       autoCloseMs: 3500
//   })
//
// Behavior:
//   • Up to `maxVisible` (default 3) toasts visible at once, stacked top-down
//     from the top-right corner.
//   • Queued show() calls beyond maxVisible are held in `_pending` and pop
//     in FIFO order as visible toasts auto-dismiss.
//   • Each toast auto-removes after its autoCloseMs; clicking the close glyph
//     fires the same dismissal path.
//   • Top-margin tracks AuroraTheme.sp4 so the stack lines up with other
//     window chrome (the sessionExpired toast was anchored the same way).
//
// Reduce motion is respected by the inner FsToast (opacity animation gated
// on AuroraTheme.reduceMotion).

import QtQuick
import QtQuick.Layouts
import Fshare.Components 1.0
import FsAurora.Theme 1.0

Item {
    id: host

    // Tunables.
    property int maxVisible: 3
    property int defaultAutoCloseMs: 3500
    property int gap: AuroraTheme.sp2

    // Internal: monotonic id generator so multiple show() calls in the same
    // ms are distinguishable in the queue.
    property int _nextId: 1

    // Visible toasts (rendered as a Column). Keep this an Object-typed
    // ListModel so we can hold both strings and ints without QML's strict
    // role typing tripping us up.
    ListModel { id: _visible }

    // Pending queue. Same shape as _visible. When a visible toast closes,
    // we shift the first entry from here into _visible.
    ListModel { id: _pending }

    // ── Public API ────────────────────────────────────────────────────
    // payload: { title, desc, variant, autoCloseMs }
    // Returns the toast id (int) so callers could dismiss specific toasts
    // later if we ever expose host.dismiss(id).
    function show(payload) {
        const item = {
            _id:         _nextId++,
            title:       payload.title       || "",
            desc:        payload.desc        || "",
            variant:     payload.variant     || "info",
            autoCloseMs: payload.autoCloseMs > 0 ? payload.autoCloseMs : defaultAutoCloseMs
        };
        if (_visible.count < maxVisible) {
            _visible.append(item);
        } else {
            _pending.append(item);
        }
        return item._id;
    }

    function _dismiss(id) {
        // Remove from _visible by id (count is small — 3 max — linear is fine).
        for (let i = 0; i < _visible.count; ++i) {
            if (_visible.get(i)._id === id) {
                _visible.remove(i);
                break;
            }
        }
        // Promote one pending toast if any.
        if (_pending.count > 0 && _visible.count < maxVisible) {
            const next = _pending.get(0);
            const item = {
                _id:         next._id,
                title:       next.title,
                desc:        next.desc,
                variant:     next.variant,
                autoCloseMs: next.autoCloseMs
            };
            _pending.remove(0);
            _visible.append(item);
        }
    }

    // Stack visible toasts in a Column anchored top-right of the host's
    // parent (typically the ApplicationWindow). Host is sized 0×0 itself.
    width: 0
    height: 0

    Column {
        anchors.top: parent ? parent.top : undefined
        anchors.right: parent ? parent.right : undefined
        anchors.topMargin: AuroraTheme.sp4
        anchors.rightMargin: AuroraTheme.sp4
        spacing: host.gap

        Repeater {
            model: _visible

            // Each row reads the model's _id / title / desc / variant /
            // autoCloseMs roles and emits onClosed → host._dismiss(_id).
            FsToast {
                required property int    _id
                required property string title
                required property string desc
                required property string variant
                required property int    autoCloseMs

                title:       _id ? title : ""    // bind via role
                desc:        desc
                variant:     variant
                autoCloseMs: autoCloseMs
                autoClose:   true
                onClosed:    host._dismiss(_id)
            }
        }
    }
}
