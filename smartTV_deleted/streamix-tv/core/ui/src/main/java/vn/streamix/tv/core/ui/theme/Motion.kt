/*
 * StreamIX TV — Motion tokens
 * Reference: design package motion-specs.md
 */
package vn.streamix.tv.core.ui.theme

import androidx.compose.animation.core.CubicBezierEasing
import androidx.compose.animation.core.Easing

object FsMotion {
    // motion.quick — 100ms cubic-bezier(0.2,0,0,1)
    const val DurationQuickMs = 100
    val EaseQuick: Easing = CubicBezierEasing(0.2f, 0f, 0f, 1f)

    // motion.standard — 200ms
    const val DurationStandardMs = 200
    val EaseStandard: Easing = CubicBezierEasing(0.2f, 0f, 0f, 1f)

    // motion.emphasized — 300ms
    const val DurationEmphasizedMs = 300
    val EaseEmphasized: Easing = CubicBezierEasing(0.2f, 0f, 0f, 1f)

    // skeleton-pulse — 1500ms ease-in-out, repeat infinite
    const val DurationSkeletonMs = 1500
    val EaseInOut: Easing = CubicBezierEasing(0.4f, 0f, 0.6f, 1f)
}
