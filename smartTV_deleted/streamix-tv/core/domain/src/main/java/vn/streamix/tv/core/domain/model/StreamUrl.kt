/*
 * StreamIX TV — Stream URL response model
 * Reference: 13 §3.3 + §10.0.4 — endpoint giả định, backend cần ship `v1/session.md`
 */
package vn.streamix.tv.core.domain.model

data class StreamUrl(
    val url: String,
    val expiresInSeconds: Long,
    val expiresAt: Long,            // epoch millis
)
