/*
 * StreamIX TV — DEPRECATED.
 *
 * AppKeyInterceptor đã được thay bằng [FshareHeaderInterceptor] trong Wave M1
 * (migrate từ Open API Gateway sang Legacy Fshare API).
 *
 * Legacy API KHÔNG dùng `X-App-Key` header — `app_key` đi trong body /api/user/login.
 *
 * File này giữ rỗng để build không fail; sẽ xoá thực sự ở wave cleanup tiếp theo.
 */
package vn.streamix.tv.core.network.interceptor
