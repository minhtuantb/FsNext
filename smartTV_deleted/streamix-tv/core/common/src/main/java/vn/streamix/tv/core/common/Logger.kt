/*
 * StreamIX TV — Logger facade (Timber wrapper)
 *
 * Encapsulate Timber để rest of code không phụ thuộc trực tiếp vào Timber API.
 * Đầu Phase 5 Hardening sẽ thay backend (FileTree rotating).
 */
package vn.streamix.tv.core.common

import timber.log.Timber

object Log {
    fun d(message: String, vararg args: Any?) = Timber.d(message, *args)
    fun i(message: String, vararg args: Any?) = Timber.i(message, *args)
    fun w(message: String, vararg args: Any?) = Timber.w(message, *args)
    fun w(throwable: Throwable, message: String = "", vararg args: Any?) = Timber.w(throwable, message, *args)
    fun e(message: String, vararg args: Any?) = Timber.e(message, *args)
    fun e(throwable: Throwable, message: String = "", vararg args: Any?) = Timber.e(throwable, message, *args)
}
