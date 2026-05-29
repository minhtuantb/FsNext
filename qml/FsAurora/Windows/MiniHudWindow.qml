// SPDX-License-Identifier: Proprietary
// MiniHudWindow — floating, always-on-top, draggable HUD shown when the
// user closes the main window into the tray with at least one active
// transfer running.
//
// Two modes via `pinned`: pinned (tray-click, stays until toggled/closed)
// and auto (minimize + active transfer, auto-hides after 30s idle).
// Supports drag + snap + position persist; embeds TransferHudPanel with
// compact=false (sparkline + pause-all + close buttons).
//
// Public API:
//   showAtDefault()              — show at saved position (or default bottom-right)
//   dismissWithFade()            — fade out and hide
//   bindToVisibility(showFlag)   — react to hudVM.shouldShowMini changes
//                                   • flag=true  : show immediately, cancel idle timer
//                                   • flag=false : start 30s idle timer; hide when it fires
//
// Drag math: we capture window.x/y + mouse local pos at press time, then
// on positionChanged update window pos by the cursor delta. Once window
// moves, MouseArea-local cursor settles to the press point ⇒ stable.
//
// Snap: on mouse release, if window edge is within 24 px of an available
// screen edge, snap to (avail - 8 px gap). Smooth animated snap unless
// reduce-motion is set.

import QtQuick
import QtQuick.Window
import FsAurora.Theme 1.0
import FsAurora.Components 1.0

Window {
    id: mini

    // CRITICAL: decouple from the main window's lifecycle. A Window declared
    // as a child of ApplicationWindow inherits transientParent = root by
    // default — and Windows hides/minimises transient children whenever the
    // parent is hidden or minimised. Since the mini is shown PRECISELY when
    // the main window goes away (hide-to-tray or minimise), a transient
    // relationship makes it invisible exactly when we need it. Setting
    // transientParent: null makes the mini a fully independent top-level
    // window that survives the main window disappearing.
    transientParent: null

    // Aurora widget size tier — only two canonical states per the v6.1
    // refresh: "expanded" (full hero + sparkline + file list) and
    // "collapsed" (compact peek with just header + inline speed). The
    // intermediate "normal" tier was confusing — three click-to-cycle
    // states bury the user in a 3-state machine for a glance affordance.
    // Two states make the intent unambiguous: shrunk vs. open.
    //
    // The widget auto-collapses when the surface fades to its idle alpha
    // and auto-expands on hover, so the manual cycle button is now an
    // explicit override of that automatic behaviour.
    property string widgetSize: "expanded"
    width: widgetSize === "collapsed" ? AuroraTheme.widgetWidthCollapsed
                                      : AuroraTheme.widgetWidthExpanded
    height: panel.implicitHeight
    // Width transitions smoothly so the auto collapse/expand on hover
    // feels like the widget breathing rather than snapping between states.
    Behavior on width { enabled: !AuroraTheme.reduceMotion
        NumberAnimation { duration: AuroraTheme.durFast; easing.type: AuroraTheme.easingStd } }

    // Cycle button now flips between the two canonical sizes.
    function _cycleSize() {
        widgetSize = (widgetSize === "expanded") ? "collapsed" : "expanded";
        // Manual override: cancel the auto-collapse pause so the user's
        // explicit choice sticks until they hover again.
        _userOverride = true;
        userOverrideClearTimer.restart();
    }

    // After a manual cycle, hold the override for 4 seconds before letting
    // hover-auto-expand resume. Otherwise leaving the cursor mid-hover
    // would un-do the user's manual collapse on the next mouse-leave.
    property bool _userOverride: false
    Timer {
        id: userOverrideClearTimer
        interval: 4000
        repeat: false
        onTriggered: mini._userOverride = false
    }

    visible: false
    color: "transparent"
    title: ""     // never displayed (frameless) but kept empty for clarity

    // Always-on-top tool window. Qt.Tool keeps us out of the taskbar and
    // Alt+Tab. WindowDoesNotAcceptFocus would block keyboard interactions
    // entirely — we WANT focus so close/expand buttons respond, so we
    // leave that flag OFF here (unlike the tray popup which is short-lived
    // and benefits from focus-out auto-dismiss).
    flags: Qt.FramelessWindowHint
         | Qt.Tool
         | Qt.WindowStaysOnTopHint
         | Qt.NoDropShadowWindowHint

    // External signal forwarding — Main.qml listens to wire expand → show
    // main, close → dismissMini on hudVM (user intent: stop auto-popping
    // the mini until they ask for it again).
    signal expandRequested()
    signal closeRequested()
    signal rowFocused(string taskId)
    // Footer "+ Dán link" CTA — Main.qml routes this to the existing
    // clipboard-paste flow (downloadViewModel.clipboardText →
    // routePastedText) so the widget can kick off a download without
    // opening the main UI.
    signal pasteLinkRequested()

    // Tunables.
    readonly property int kSnapMargin: 24
    readonly property int kEdgeGap:    8
    readonly property int kIdleHideMs: 30 * 1000
    // Drag damping factor — the per-frame lerp coefficient. Lower = heavier,
    // more visible trail behind the cursor. 1.0 = 1:1 instant follow.  0.16
    // gives a deliberate "weighty / slow" feel matching the UX brief: "cảm
    // giác kéo không đi hoặc đi chậm".
    readonly property real kDragFollow: 0.16

    // Surface alpha tier — v6.1 refresh per user feedback ("trong suốt
    // nhanh hơn, sâu hơn"). The widget aggressively recedes when the user
    // isn't looking at it, then firms up the moment the cursor approaches.
    //
    //   dragging       : 0.99  (firm while in motion)
    //   hover (any)    : 0.98  (cursor present, full legibility)
    //   pinned + idle  : 0.55  (persistent widget, well-recessed)
    //   auto   + idle  : 0.35  (transient peek, melts into desktop)
    //
    // The fade transition uses durFast (140 ms) instead of durBase (220 ms)
    // so the response feels reactive rather than lazy.
    property real _surfaceAlpha: {
        if (_isDragging)      return 0.99;
        if (hudHover.hovered) return 0.98;
        return pinned ? 0.55 : 0.35;
    }
    Behavior on _surfaceAlpha {
        enabled: !AuroraTheme.reduceMotion
        NumberAnimation { duration: AuroraTheme.durFast; easing.type: AuroraTheme.easingStd }
    }

    // Auto-collapse on idle, auto-expand on hover — implements the "khi
    // hover sẽ tự động open max size widget" rule. The _userOverride flag
    // (set by manual cycle button) suspends this for 4 s so a user choice
    // isn't immediately undone by their cursor leaving the widget.
    //
    // Reads `_isDragging` so a drag-in-progress keeps the widget in its
    // current state instead of toggling under the cursor; reads
    // `hudHover.hovered` for the actual hover signal.
    Connections {
        target: hudHover
        function onHoveredChanged() { mini._maybeAutoResize() }
    }
    function _maybeAutoResize() {
        if (_userOverride || _isDragging) return;
        widgetSize = hudHover.hovered ? "expanded" : "collapsed";
    }

    // Glass rim — 2× the original alpha so the card edge reads cleanly
    // against a hover-translucent surface. Bumped from 0.10 → 0.22 (dark)
    // and 0.08 → 0.18 (light) per user feedback ("border đậm hơn"). Still
    // translucent enough to read as glass rather than a hard outline.
    readonly property color _glassBorder: AuroraTheme.isDark
        ? Qt.rgba(1, 1, 1, 0.22)
        : Qt.rgba(0, 0, 0, 0.18)

    // Pinned = user summoned this via the tray icon and wants it to STAY
    // (Idea A — a persistent desktop widget floating over Word/Excel/etc.).
    // While pinned, the 30s idle auto-hide is disabled; the widget only
    // goes away on an explicit tray-toggle, ✕, or when the main window comes
    // to the foreground. Auto mode (minimize + transfer) sets pinned=false
    // so the idle timer reclaims it.
    property bool pinned: false

    // Keyboard a11y: Esc dismisses the mini exactly like the ✕ button.
    // Fires once the window has focus (we don't grab focus on show, so a
    // keyboard user presses Esc after clicking into the mini) — the correct
    // behaviour for a non-activating tool window.
    Shortcut {
        sequence: "Escape"
        onActivated: mini.closeRequested()
    }

    // ── Idle auto-hide ─────────────────────────────────────────────────
    // When hudVM.shouldShowMini flips to false (everything idle), we don't
    // hide immediately — give the user 30 s to see "done!" then fade out.
    Timer {
        id: idleHideTimer
        interval: mini.kIdleHideMs
        repeat: false
        onTriggered: dismissWithFade()
    }

    // AUTO mode (Idea B): driven by hudVM.shouldShowMini on minimize+transfer.
    function bindToVisibility(shouldShow) {
        if (shouldShow) {
            idleHideTimer.stop();
            if (!mini.visible) { mini.pinned = false; showAtDefault(); }
        } else {
            // Don't insta-hide; let the idle timer expire so the user has
            // a beat to read the "everything done" state. BUT never auto-hide
            // a pinned widget — the user explicitly asked it to stay.
            if (mini.visible && !mini.pinned) idleHideTimer.restart();
        }
    }

    // PINNED mode (Idea A): summoned from the tray icon. Stays put (no idle
    // auto-hide) until the user toggles it off or opens the main window.
    function showPinned() {
        mini.pinned = true;
        idleHideTimer.stop();
        if (!mini.visible) showAtDefault();
        else { raise(); requestActivate(); }
    }

    // Tray single-click handler: hide if showing, else show pinned.
    function togglePinned() {
        if (mini.visible) dismissWithFade();
        else              showPinned();
    }

    // ── Position restore on first show ──────────────────────────────────
    function showAtDefault() {
        const savedX = (typeof settingsViewModel !== "undefined" && settingsViewModel)
                        ? settingsViewModel.savedMiniWindowX() : -1;
        const savedY = (typeof settingsViewModel !== "undefined" && settingsViewModel)
                        ? settingsViewModel.savedMiniWindowY() : -1;
        const savedScreen = (typeof settingsViewModel !== "undefined" && settingsViewModel)
                        ? settingsViewModel.savedMiniWindowScreen() : "";
        _placeAt(savedX, savedY, savedScreen);
        show();
        raise();
        // Don't requestActivate() — we don't want to steal focus from the
        // user's current app.  Buttons inside still receive clicks because
        // Qt routes pointer events to top-most windows even unfocused.
    }

    // Work-area rect of a QML Screen object.  CRITICAL: the QML Screen type
    // (what Qt.application.screens returns) does NOT expose availableGeometry
    // — that's a C++ QScreen API.  QML gives virtualX/Y + desktopAvailable
    // Width/Height instead.  Using .availableGeometry returned undefined and
    // threw "Cannot read property 'x' of undefined", which silently broke
    // every show/snap/position path.  This helper builds the rect correctly.
    function _availOf(s) {
        if (!s) return Qt.rect(0, 0, 1280, 720);   // last-ditch fallback
        const w = s.desktopAvailableWidth  > 0 ? s.desktopAvailableWidth  : s.width;
        const h = s.desktopAvailableHeight > 0 ? s.desktopAvailableHeight : s.height;
        // virtualX/Y is the screen's top-left in the virtual desktop. Work
        // area top-left matches it for the common bottom/right taskbar.
        return Qt.rect(s.virtualX, s.virtualY, w, h);
    }

    function _placeAt(savedX, savedY, savedScreenName) {
        const screens = Qt.application.screens;
        let target = screens[0];
        // Restore the screen the window lived on; fall back to primary if
        // that monitor has been unplugged.
        if (savedScreenName && savedScreenName.length > 0) {
            for (let i = 0; i < screens.length; ++i) {
                if (screens[i].name === savedScreenName) { target = screens[i]; break; }
            }
        }
        const avail = _availOf(target);

        let x, y;
        if (savedX >= 0 && savedY >= 0) {
            // Verify the saved point still falls inside the target screen.
            // If not (monitor topology changed), fall back to default.
            if (savedX >= avail.x &&
                savedX + mini.width  <= avail.x + avail.width &&
                savedY >= avail.y &&
                savedY + mini.height <= avail.y + avail.height) {
                x = savedX; y = savedY;
            } else {
                x = -1; y = -1;
            }
        } else {
            x = -1; y = -1;
        }
        if (x < 0) {
            // Default: TOP-right of the primary screen's available area
            // (user preference — widget floats top-right like a desktop
            // gadget, clear of the taskbar/tray at the bottom).
            const primary = _availOf(screens[0]);
            x = primary.x + primary.width - mini.width - kEdgeGap;
            y = primary.y + kEdgeGap;
        }
        mini.x = Math.round(x);
        mini.y = Math.round(y);
    }

    // ── Drag handling — damped (heavy) follow ───────────────────────────
    // Drag model: mouse press captures the grab offset (local cursor point
    // inside the drag area). Every move event recomputes the IDEAL window
    // position (_targetX/_targetY) — where the window would sit RIGHT NOW
    // to keep the grabbed point under the cursor. A FrameAnimation then
    // glides mini.x/y toward that target at kDragFollow per frame, so the
    // window visibly trails the cursor — the "rất nặng / đi chậm" feel.
    //
    // Stability under lag: the target computation uses `mini.x + mouse.x`
    // (the true screen-space cursor, since the drag area is anchored at
    // window origin) minus the press grab offset. Because that quantity
    // doesn't depend on the lagging mini.x reaching the target, the math
    // stays correct regardless of how far behind the window is.
    property real _grabX
    property real _grabY
    property real _targetX: 0
    property real _targetY: 0
    property bool _isDragging: false

    function _onDragPress(mouse) {
        _grabX = mouse.x;
        _grabY = mouse.y;
        _targetX = mini.x;
        _targetY = mini.y;
        _isDragging = true;
    }
    function _onDragMove(mouse) {
        if (!_isDragging) return;
        // True screen cursor = mini.x + mouse.x (drag area is anchored at
        // window origin). Target keeps the grab offset under that cursor.
        _targetX = mini.x + mouse.x - _grabX;
        _targetY = mini.y + mouse.y - _grabY;
        // Reduce-motion users get instant 1:1 — the lerp IS the motion
        // effect, so honour the accessibility preference by skipping it.
        if (AuroraTheme.reduceMotion) {
            mini.x = Math.round(_targetX);
            mini.y = Math.round(_targetY);
        }
    }
    function _onDragRelease() {
        if (!_isDragging) return;
        _isDragging = false;
        _snapToEdges();
        _persistPosition();
    }

    // Per-frame lerp toward the drag target — the source of the "heavy"
    // feel. Runs only while a drag is active and motion is permitted.
    // Math.round prevents int-truncation drift (Window.x/y are int).  A
    // sub-pixel threshold snaps to target so the window doesn't asymptote
    // a few pixels short when the user releases on a slow trail.
    FrameAnimation {
        id: dragFollow
        running: mini._isDragging && !AuroraTheme.reduceMotion
        onTriggered: {
            if (!mini._isDragging) return;
            const dx = mini._targetX - mini.x;
            const dy = mini._targetY - mini.y;
            if (Math.abs(dx) < 1 && Math.abs(dy) < 1) {
                mini.x = Math.round(mini._targetX);
                mini.y = Math.round(mini._targetY);
                return;
            }
            mini.x = Math.round(mini.x + dx * mini.kDragFollow);
            mini.y = Math.round(mini.y + dy * mini.kDragFollow);
        }
    }

    function _snapToEdges() {
        const screens = Qt.application.screens;
        // Pick screen by the INTENDED window center (_targetX/_targetY).
        // Using mini.x/y while the window may still be lagging mid-trail
        // would let snap chase the visual position onto the wrong monitor.
        const cx = _targetX + mini.width  / 2;
        const cy = _targetY + mini.height / 2;
        let target = screens[0];
        let bestDist = Number.MAX_VALUE;
        for (let i = 0; i < screens.length; ++i) {
            const s  = screens[i];
            const sx = s.virtualX + s.width / 2;
            const sy = s.virtualY + s.height / 2;
            const d  = (sx - cx) * (sx - cx) + (sy - cy) * (sy - cy);
            if (d < bestDist) { bestDist = d; target = s; }
        }
        const avail = _availOf(target);

        let nx = _targetX, ny = _targetY;
        if (nx - avail.x < kSnapMargin) nx = avail.x + kEdgeGap;
        else if (avail.x + avail.width - (nx + mini.width) < kSnapMargin)
            nx = avail.x + avail.width  - mini.width  - kEdgeGap;
        if (ny - avail.y < kSnapMargin) ny = avail.y + kEdgeGap;
        else if (avail.y + avail.height - (ny + mini.height) < kSnapMargin)
            ny = avail.y + avail.height - mini.height - kEdgeGap;

        // Update the logical target so persist + any continuing math
        // refer to the snapped position, then glide the visible window
        // (which may be lagging behind) into that final spot — gives the
        // heavy drag a clean "settles into the edge" finish.
        _targetX = nx;
        _targetY = ny;

        if (AuroraTheme.reduceMotion) {
            mini.x = nx; mini.y = ny;
        } else {
            snapAnimX.from = mini.x; snapAnimX.to = nx; snapAnimX.start();
            snapAnimY.from = mini.y; snapAnimY.to = ny; snapAnimY.start();
        }
    }

    function _persistPosition() {
        if (typeof settingsViewModel === "undefined" || !settingsViewModel) return;
        const screens = Qt.application.screens;
        // Persist the LOGICAL final position (_targetX/_targetY) — what the
        // user dragged to, not the still-gliding visible position.
        const cx = _targetX + mini.width / 2;
        const cy = _targetY + mini.height / 2;
        let screenName = "";
        for (let i = 0; i < screens.length; ++i) {
            const s = screens[i];
            if (cx >= s.virtualX && cx < s.virtualX + s.width &&
                cy >= s.virtualY && cy < s.virtualY + s.height) {
                screenName = s.name;
                break;
            }
        }
        settingsViewModel.saveMiniWindowPosition(_targetX, _targetY, screenName);
    }

    NumberAnimation { id: snapAnimX; target: mini; property: "x"; duration: 150; easing.type: AuroraTheme.easingStd }
    NumberAnimation { id: snapAnimY; target: mini; property: "y"; duration: 150; easing.type: AuroraTheme.easingStd }

    // ── Fade dismiss ────────────────────────────────────────────────────
    function dismissWithFade() {
        if (!mini.visible) return;
        idleHideTimer.stop();
        mini.pinned = false;   // reset so the next auto-show is auto mode
        if (AuroraTheme.reduceMotion) { mini.hide(); return; }
        fadeOut.start();
    }
    NumberAnimation {
        id: fadeOut
        target: mini.contentItem
        property: "opacity"
        from: 1.0; to: 0.0
        duration: AuroraTheme.durBase
        easing.type: AuroraTheme.easingStd
        onStopped: { mini.hide(); mini.contentItem.opacity = 1.0; }
    }
    onVisibleChanged: {
        if (visible && !AuroraTheme.reduceMotion) {
            contentItem.opacity = 0.0;
            fadeIn.start();
        }
    }
    NumberAnimation {
        id: fadeIn
        target: mini.contentItem
        property: "opacity"
        from: 0.0; to: 1.0
        duration: AuroraTheme.durFast
        easing.type: AuroraTheme.easingStd
    }

    // ── Content ─────────────────────────────────────────────────────────
    Item {
        anchors.fill: parent

        // NOTE on outer shadows: the Aurora spec calls for an outside
        // warm halo (0 24px 60px rgba(255,91,46,0.18)). Rendering that in
        // a QML frameless Window requires the window viewport to be
        // larger than the card so children with negative anchor margins
        // can paint into the padding. Our window is sized to the card
        // exactly (so the user grabs the card's visible edge for drag),
        // so any shadow Rectangle with margin -N gets clipped off-screen.
        // Aurora's warm identity is therefore carried by (a) the brand-
        // mark gradient, (b) the radial halo Canvas painted INSIDE the
        // card top, (c) the gradient hero number, and (d) the orange
        // glass border. Outer drop shadow is a future enhancement that
        // would require a Window-enlargement refactor + per-side shadow
        // pad constants — tracked but deferred to keep drag/snap math
        // simple and predictable.

        Rectangle {
            id: card
            anchors.fill: parent
            // TRANSLUCENT Aurora surface — single point where state-driven
            // opacity becomes visible. RGB stays AuroraTheme.panel so the
            // tint matches dark/light mode; alpha shifts per pinned/hover/
            // drag tier. Panel content composites crisp on top because the
            // panel has drawSurface=false (no second opaque layer below).
            color: {
                const c = AuroraTheme.panel;
                return Qt.rgba(c.r, c.g, c.b, mini._surfaceAlpha);
            }
            // Aurora widget radius (18px) — larger than the generic card
            // radiusLg (14) per the design spec for the floating overlay.
            radius: AuroraTheme.radiusOuter
            // Glass rim — translucent hairline. See mini._glassBorder.
            border.color: mini._glassBorder
            border.width: 1
            // Clip the warm halo canvas to the rounded card shape — Qt's
            // rectangular clip approximates this well enough at the top
            // edge since the halo gradient already fades to transparent
            // before reaching the rounded corner area.
            clip: true

            // ── Aurora warm halo ─────────────────────────────────────
            // GPU-rasterised vertical gradient (Rectangle.gradient) —
            // rasterised once per size change and cached by Qt's scene
            // graph, so it survives QML engine activity like page swaps
            // in the main window without re-painting. The earlier Canvas
            // implementation painted two RADIAL gradients (pink-left,
            // mango-right) for stronger Aurora feel, but Canvas.onPaint
            // re-fires whenever Qt invalidates the parent scene graph
            // layer — and the user observed mini repainting on every
            // page change in the main window, which led to crash
            // reports. Vertical Rectangle gradient costs zero on
            // invalidation. We pick stops that read as a warm aurora
            // wash across the full top band even without the radial.
            Rectangle {
                id: auroraHalo
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 1   // sit under the card's 1px border
                anchors.leftMargin: 1
                anchors.rightMargin: 1
                height: 110
                radius: AuroraTheme.radiusOuter
                // Per-corner radii: round only the top corners to follow
                // the card; let the bottom fade naturally without a
                // visible edge.
                topLeftRadius:     AuroraTheme.radiusOuter
                topRightRadius:    AuroraTheme.radiusOuter
                bottomLeftRadius:  0
                bottomRightRadius: 0
                gradient: Gradient {
                    // Stops: warm pink top → cam → fade to transparent.
                    // Dark mode raises the top alpha so the glow reads
                    // against the dark panel.
                    GradientStop {
                        position: 0.0
                        color: Qt.rgba(AuroraTheme.accent3.r,
                                       AuroraTheme.accent3.g,
                                       AuroraTheme.accent3.b,
                                       AuroraTheme.isDark ? 0.28 : 0.20)
                    }
                    GradientStop {
                        position: 0.35
                        color: Qt.rgba(AuroraTheme.accent.r,
                                       AuroraTheme.accent.g,
                                       AuroraTheme.accent.b,
                                       AuroraTheme.isDark ? 0.16 : 0.10)
                    }
                    GradientStop {
                        position: 1.0
                        color: Qt.rgba(AuroraTheme.accent.r,
                                       AuroraTheme.accent.g,
                                       AuroraTheme.accent.b, 0.0)
                    }
                }
            }

            // Whole-widget hover detector. Drives _surfaceAlpha firm-up so
            // the entire surface opacifies the moment the cursor enters
            // ANY part of the card — header, body, footer — not just on
            // a button. HoverHandler is passive: it observes hover without
            // grabbing events, so buttons + the drag layer keep working
            // normally underneath.
            HoverHandler {
                id: hudHover
            }

            // Drag layer — covers ONLY the header band, declared BEFORE
            // the panel so it sits beneath the panel in z order. Empty
            // header areas (brand text + spacer + speed readout — none of
            // which grab mouse events) fall through to this drag area;
            // the ⏸/⛶/✕ ToolButtons inside the panel grab their own
            // events because they're on top. Net effect: drag fires on
            // header background, buttons keep their hover/click — exactly
            // the "kéo chỉ ở header, trừ buttons" requirement. No
            // propagateComposedEvents trickery needed.
            MouseArea {
                id: dragArea
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                // Header row + the breathing gap above the sparkline.
                // Doesn't extend into the sparkline itself so the chart
                // area isn't a stealth drag handle.
                height: 44 + AuroraTheme.sp2
                cursorShape: Qt.SizeAllCursor
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton
                onPressed: function(mouse) { mini._onDragPress(mouse); }
                onPositionChanged: function(mouse) { mini._onDragMove(mouse); }
                onReleased: function(mouse) { mini._onDragRelease(); }
            }

            TransferHudPanel {
                id: panel
                anchors.fill: parent
                compact: false
                // Host paints the translucent surface; panel paints ONLY
                // content so the glass effect isn't undone by a second
                // opaque fill layer beneath the text.
                drawSurface: false
                // expand/close come from the panel's own header buttons.
                // Mirror the host's size tier down — panel reads this to
                // hide/show the speedmeter + file list.
                widgetSize:         mini.widgetSize
                onExpandClicked:    mini.expandRequested()
                onCloseClicked:     mini.closeRequested()
                onPasteLinkClicked: mini.pasteLinkRequested()
                onCycleSizeRequested: mini._cycleSize()
                onRowClicked: function(taskId) {
                    mini.rowFocused(taskId);
                    // Don't auto-dismiss on row click for the mini window:
                    // user might want to keep watching the queue.  This is
                    // the key UX difference from the popup, which DOES
                    // dismiss on row click (popup = "peek then leave").
                }
            }

            // ── Brand-gradient accent strip ─────────────────────────────
            // 3px horizontal seal at the top edge.  Inset by 1px from each
            // side and the top so the strip sits INSIDE the card's 1px
            // glass rim — previously the strip ran flush to the card edge
            // and bled past the rounded top corners ("border top bị đưa
            // ra ngoài"). Per-corner radii match the card's INNER rounded
            // shape (parent.radius - 1) so the strip melts cleanly into
            // the curve.
            Rectangle {
                id: accentStrip
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: 1
                anchors.rightMargin: 1
                anchors.topMargin: 1
                height: 3
                topLeftRadius: Math.max(0, parent.radius - 1)
                topRightRadius: Math.max(0, parent.radius - 1)
                bottomLeftRadius: 0
                bottomRightRadius: 0
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.00; color: AuroraTheme.accent  }
                    GradientStop { position: 0.60; color: AuroraTheme.accent3 }
                    GradientStop { position: 1.00; color: AuroraTheme.accent2 }
                }
                opacity: (typeof transferHudViewModel !== "undefined"
                          && transferHudViewModel
                          && transferHudViewModel.runState === "running")
                         ? 0.95 : 0.50
                Behavior on opacity {
                    enabled: !AuroraTheme.reduceMotion
                    NumberAnimation { duration: AuroraTheme.durBase; easing.type: AuroraTheme.easingStd }
                }
            }
        }
    }
}
