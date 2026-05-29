// SPDX-License-Identifier: Proprietary
// Sparkline — tiny inline chart for the Transfer HUD's speed history.
//
// Renders a 60-sample × 1Hz time series as a smooth line over a soft
// gradient fill underneath.  No axes, no labels — it's a glance affordance,
// not a precise chart.
//
// Usage:
//   Sparkline {
//       width: 320; height: 32
//       data: transferHudViewModel.speedHistory   // QVariantList of doubles
//       lineColor: AuroraTheme.accent
//   }
//
// Implementation notes:
//   • Canvas (not Shape) because the data updates once per second; the
//     redraw cost is negligible and Canvas's drawing API is simpler than
//     ShapePath for smooth bezier paths.
//   • Y-axis auto-scales to the buffer's local max.  If the local max is
//     zero (idle), render a faint dashed baseline instead of an empty
//     canvas — a flat zero-line is "I'm here, just nothing to show".
//   • Smoothing: Catmull-Rom-ish cubic between consecutive midpoints.
//     Faster than computing real Catmull-Rom and visually identical at
//     32-px height.

import QtQuick
import FsAurora.Theme 1.0

Item {
    id: spark

    // Public API
    property var    data: []                      // QVariantList of doubles
    property color  lineColor: AuroraTheme.accent
    property color  fillColor: AuroraTheme.accent
    property real   lineWidth: 1.6
    property real   bottomPad: 2                  // px padding between line low and bottom edge
    property real   topPad: 4                     // px padding between line peak and top edge

    onDataChanged:       canvas.requestPaint()
    onLineColorChanged:  canvas.requestPaint()
    onFillColorChanged:  canvas.requestPaint()
    onWidthChanged:      canvas.requestPaint()
    onHeightChanged:     canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true
        // CPU-rendered image target instead of FBO. The FBO path is faster
        // but has a long tail of "blank canvas on translucent windows"
        // issues across Qt 6.x — and the mini HUD is exactly that case
        // (frameless transparent Window + translucent card fill). At
        // 60 samples × 1 Hz the redraw cost is trivial, so we trade peak
        // GPU efficiency for "actually renders every time". Cooperative
        // strategy keeps the work on the GUI thread which avoids the
        // race that lost first-paint requests on Threaded.
        renderTarget:   Canvas.Image
        renderStrategy: Canvas.Cooperative

        // Canvas.available flips to true only after the backend has set up
        // the surface. requestPaint() calls fired before that point are
        // silently dropped, so first-paint can be lost if the data binding
        // changes during construction (very common when the host window
        // appears before vm.speedHistory has emitted). Force a fresh paint
        // the instant the canvas becomes available, plus once on completion,
        // plus a 120 ms safety-net kick (the canvas occasionally needs a
        // post-layout tick before width/height settle on the real values).
        onAvailableChanged: if (available) requestPaint();
        Component.onCompleted: requestPaint();
        Timer {
            interval: 120; repeat: false; running: true
            onTriggered: canvas.requestPaint()
        }

        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();
            const w = width;
            const h = height;
            if (w <= 0 || h <= 0) return;

            const n = (spark.data && spark.data.length) ? spark.data.length : 0;
            if (n < 2) {
                _drawBaseline(ctx, w, h);
                return;
            }

            // Find local max for y-scaling.  Floor at 1 so a buffer of
            // very small numbers still renders rather than dividing by 0.
            let maxV = 0;
            for (let i = 0; i < n; ++i) {
                const v = spark.data[i] || 0;
                if (v > maxV) maxV = v;
            }
            if (maxV <= 0) {
                _drawBaseline(ctx, w, h);
                return;
            }

            // Build screen-space points.  X spreads samples evenly across
            // the width; Y maps 0..maxV onto (h-bottomPad)..topPad (inverted
            // because Canvas y grows downward).
            const xs = new Array(n);
            const ys = new Array(n);
            const usableH = h - spark.topPad - spark.bottomPad;
            const dx = w / (n - 1);
            for (let i = 0; i < n; ++i) {
                const v = spark.data[i] || 0;
                xs[i] = i * dx;
                ys[i] = h - spark.bottomPad - (v / maxV) * usableH;
            }

            // ── Fill (Aurora vertical gradient — accent 25% → 0%) ────
            // Soft warm wash underneath the curve. The single-colour fill
            // (lineColor.r/g/b) keeps the visual focus on the curve while
            // grounding it on the surface.
            const fillGrad = ctx.createLinearGradient(0, spark.topPad, 0, h);
            fillGrad.addColorStop(0.0, Qt.rgba(spark.fillColor.r, spark.fillColor.g,
                                                spark.fillColor.b, 0.28));
            fillGrad.addColorStop(1.0, Qt.rgba(spark.fillColor.r, spark.fillColor.g,
                                                spark.fillColor.b, 0.00));
            ctx.beginPath();
            ctx.moveTo(xs[0], h);
            ctx.lineTo(xs[0], ys[0]);
            _drawSmoothPath(ctx, xs, ys);
            ctx.lineTo(xs[n - 1], h);
            ctx.closePath();
            ctx.fillStyle = fillGrad;
            ctx.fill();

            // ── Line stroke (Aurora horizontal gradient: pink → orange
            //                 → mango) ───────────────────────────────────
            // Mirrors the brand mark + progress bar gradient so the
            // waveform reads as "this app's signature chart" instead of
            // a generic accent line. Stops match AuroraColors.gradientStops.
            const strokeGrad = ctx.createLinearGradient(0, 0, w, 0);
            strokeGrad.addColorStop(0.0, AuroraTheme.accent3);   // pink
            strokeGrad.addColorStop(0.5, AuroraTheme.accent);    // orange
            strokeGrad.addColorStop(1.0, AuroraTheme.accent2);   // mango
            ctx.beginPath();
            ctx.moveTo(xs[0], ys[0]);
            _drawSmoothPath(ctx, xs, ys);
            ctx.lineCap   = "round";
            ctx.lineJoin  = "round";
            ctx.lineWidth = spark.lineWidth + 0.4;   // 2.0 default — visible on glass
            ctx.strokeStyle = strokeGrad;
            ctx.stroke();

            // ── Live-sample dot at the right edge ────────────────────
            // Two-layer affordance: a faint accent halo ring (3px alpha
            // 0.20) backed by a saturated solid dot. Reads as "this is
            // the live tip of the curve". Tracks the latest y so the dot
            // sits ON the curve, not floating.
            const cx = xs[n - 1];
            const cy = ys[n - 1];
            ctx.beginPath();
            ctx.arc(cx, cy, spark.lineWidth + 4.5, 0, 2 * Math.PI);
            ctx.fillStyle = Qt.rgba(spark.lineColor.r, spark.lineColor.g,
                                     spark.lineColor.b, 0.20);
            ctx.fill();
            ctx.beginPath();
            ctx.arc(cx, cy, spark.lineWidth + 1.6, 0, 2 * Math.PI);
            ctx.fillStyle = spark.lineColor;
            ctx.fill();
        }

        // Smooth-through-midpoints — fast bezier approximation.  For each
        // pair (i, i+1) we pick the midpoint as the end of a quadratic
        // segment whose control point is the next data point.  Visually
        // identical to Catmull-Rom at our scale but ~half the math.
        function _drawSmoothPath(ctx, xs, ys) {
            const n = xs.length;
            for (let i = 1; i < n - 1; ++i) {
                const mx = (xs[i] + xs[i + 1]) / 2;
                const my = (ys[i] + ys[i + 1]) / 2;
                ctx.quadraticCurveTo(xs[i], ys[i], mx, my);
            }
            // Final segment lands on the last point exactly so the dot
            // marker overlays the line end cleanly.
            ctx.lineTo(xs[n - 1], ys[n - 1]);
        }

        // Idle baseline — clearly visible dashed line so the chart band
        // never looks "broken / blank". Tuned aggressively against the
        // translucent glass surface where lighter tones disappeared:
        // ink3 (not ink4), 0.85 alpha, 2px line. A brand-accent dot at the
        // right edge mirrors the live curve's end-dot so the chart area
        // reads as the SAME element in both states.
        function _drawBaseline(ctx, w, h) {
            const y = h - spark.bottomPad - 1;
            ctx.beginPath();
            ctx.setLineDash([4, 4]);
            ctx.lineWidth = 2.0;
            ctx.strokeStyle = AuroraTheme.ink3;
            ctx.globalAlpha = 0.85;
            ctx.moveTo(0, y);
            ctx.lineTo(w, y);
            ctx.stroke();
            ctx.setLineDash([]);
            // Brand-accent latest-slot dot — "data lands here" affordance.
            ctx.beginPath();
            ctx.arc(w - 2, y, 2.5, 0, 2 * Math.PI);
            ctx.fillStyle = spark.lineColor;
            ctx.globalAlpha = 0.85;
            ctx.fill();
            ctx.globalAlpha = 1.0;
        }
    }
}
