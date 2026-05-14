/*
 * StreamIX TV — Application class
 * Reference: 13 §26 Logging
 */
package vn.streamix.tv

import android.app.Application
import dagger.hilt.android.HiltAndroidApp
import timber.log.Timber

@HiltAndroidApp
class StreamIXApp : Application() {
    override fun onCreate() {
        super.onCreate()
        if (BuildConfig.DEBUG) {
            Timber.plant(Timber.DebugTree())
            Timber.i("StreamIX TV ${BuildConfig.VERSION_NAME} (${BuildConfig.VERSION_CODE}) — debug")
        } else {
            // Release: TODO plant FileTree (rotating log file 5MB cho user-driven debug)
            // Theo tài liệu 04 rev3 §4.11 — log file để user adb pull khi cần
        }
    }
}
