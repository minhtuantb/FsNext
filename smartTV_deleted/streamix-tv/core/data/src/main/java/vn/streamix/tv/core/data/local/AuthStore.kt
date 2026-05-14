/*
 * StreamIX TV — AuthStore (EncryptedSharedPreferences) cho Fshare LEGACY API.
 *
 * Lưu `token` + `session_id` + `email` user. Implement [AuthTokenProvider]
 * để network layer truy cập credentials mà không phụ thuộc :core:data.
 *
 * Reference: 13 §4.1.1 + API_REFERENCE_FOR_REUSE.md §3.
 */
package vn.streamix.tv.core.data.local

import android.content.Context
import androidx.security.crypto.EncryptedSharedPreferences
import androidx.security.crypto.MasterKey
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.withContext
import vn.streamix.tv.core.domain.model.Session
import vn.streamix.tv.core.network.auth.AuthTokenProvider
import javax.inject.Inject
import javax.inject.Singleton

@Singleton
class AuthStore @Inject constructor(
    @ApplicationContext private val context: Context,
) : AuthTokenProvider {

    private val prefs by lazy {
        val masterKey = MasterKey.Builder(context)
            .setKeyScheme(MasterKey.KeyScheme.AES256_GCM)
            .build()
        EncryptedSharedPreferences.create(
            context, FILE_NAME, masterKey,
            EncryptedSharedPreferences.PrefKeyEncryptionScheme.AES256_SIV,
            EncryptedSharedPreferences.PrefValueEncryptionScheme.AES256_GCM,
        )
    }

    private val _sessionFlow = MutableStateFlow(loadSync())
    val sessionFlow: StateFlow<Session?> = _sessionFlow.asStateFlow()

    suspend fun session(): Session? = _sessionFlow.value

    suspend fun save(session: Session) = withContext(Dispatchers.IO) {
        prefs.edit().apply {
            putString(KEY_TOKEN, session.token)
            putString(KEY_SID, session.sessionId)
            putString(KEY_EMAIL, session.email)
        }.apply()
        _sessionFlow.value = session
    }

    private fun loadSync(): Session? {
        val token = prefs.getString(KEY_TOKEN, null) ?: return null
        val sid = prefs.getString(KEY_SID, null) ?: return null
        return Session(
            token = token,
            sessionId = sid,
            email = prefs.getString(KEY_EMAIL, "") ?: "",
        )
    }

    // ─── AuthTokenProvider impl (network layer dùng) ───────────────────

    override suspend fun token(): String? = session()?.token

    override suspend fun sessionId(): String? = session()?.sessionId

    override suspend fun saveCredentials(token: String, sessionId: String, email: String) {
        save(Session(token = token, sessionId = sessionId, email = email))
    }

    override suspend fun clear() = withContext(Dispatchers.IO) {
        prefs.edit().clear().apply()
        _sessionFlow.value = null
    }

    companion object {
        private const val FILE_NAME = "streamix_auth"
        private const val KEY_TOKEN = "token"
        private const val KEY_SID = "session_id"
        private const val KEY_EMAIL = "email"
    }
}
