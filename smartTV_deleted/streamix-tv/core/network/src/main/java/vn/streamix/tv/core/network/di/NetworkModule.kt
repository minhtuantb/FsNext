/*
 * StreamIX TV — Network Hilt module (multi-host: Fshare + Timfshare + Sheets).
 *
 * Mỗi host có cấu hình riêng:
 *  - Fshare (api.fshare.vn): UA cố định + Cookie session_id + ShareReferral.
 *  - Timfshare (api.timfshare.com, timfshare.com): Chrome UA + Bearer (chỉ search).
 *  - Sheets (docs.google.com): Streamix/1.0 UA, no auth, response text/plain (CSV).
 *
 * Để Retrofit cùng-chia-sẻ-Moshi, dùng `@Named` qualifier cho client + retrofit.
 *
 * Reference:
 *  - 13 §4.1 (interceptor chain order).
 *  - API_REFERENCE_FOR_REUSE.md §3, §4, §5.
 */
package vn.streamix.tv.core.network.di

import com.squareup.moshi.Moshi
import com.squareup.moshi.kotlin.reflect.KotlinJsonAdapterFactory
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.components.SingletonComponent
import okhttp3.OkHttpClient
import okhttp3.logging.HttpLoggingInterceptor
import retrofit2.Retrofit
import retrofit2.converter.moshi.MoshiConverterFactory
import retrofit2.converter.scalars.ScalarsConverterFactory
import vn.streamix.tv.core.network.BuildConfig
import vn.streamix.tv.core.network.envelope.IntOrStringAdapter
import vn.streamix.tv.core.network.interceptor.AuthInterceptor
import vn.streamix.tv.core.network.interceptor.FshareHeaderInterceptor
import vn.streamix.tv.core.network.service.AuthApi
import vn.streamix.tv.core.network.service.FilesApi
import vn.streamix.tv.core.network.service.MeApi
import vn.streamix.tv.core.network.service.SessionApi
import vn.streamix.tv.core.network.service.SheetsApi
import vn.streamix.tv.core.network.service.TimfshareApi
import vn.streamix.tv.core.network.service.TimfshareTrendingApi
import java.util.concurrent.TimeUnit
import javax.inject.Named
import javax.inject.Singleton

@Module
@InstallIn(SingletonComponent::class)
object NetworkModule {

    // ─── Shared (Moshi, logging) ──────────────────────────────────────

    /** Moshi singleton. IntOrStringAdapter PHẢI register TRƯỚC KotlinJsonAdapterFactory. */
    @Provides @Singleton
    fun provideMoshi(): Moshi = Moshi.Builder()
        .add(IntOrStringAdapter())
        .add(KotlinJsonAdapterFactory())
        .build()

    @Provides @Singleton
    fun provideLoggingInterceptor(): HttpLoggingInterceptor =
        HttpLoggingInterceptor().apply {
            level = if (isDebugBuild()) HttpLoggingInterceptor.Level.BODY
                    else HttpLoggingInterceptor.Level.BASIC
            redactHeader("Authorization")
            redactHeader("Cookie")
        }

    // ─── Fshare host config ───────────────────────────────────────────

    @Provides @Singleton @Named("userAgent")
    fun provideUserAgent(): String = BuildConfig.FSHARE_USER_AGENT.ifBlank {
        "FshareVideoDesktop_23052023"
    }

    @Provides @Singleton @Named("appKey")
    fun provideAppKey(): String = BuildConfig.FSHARE_APP_KEY.ifBlank {
        "tdf5ATEa3VUuGeSyw98D1YLSKvFr8pch"
    }

    @Provides @Singleton @Named("shareReferralId")
    fun provideShareReferralId(): String = BuildConfig.FSHARE_SHARE_REFERRAL_ID.ifBlank {
        "8805984"
    }

    @Provides @Singleton @Named("fshareBaseUrl")
    fun provideFshareBaseUrl(): String = BuildConfig.FSHARE_API_BASE_URL.ifBlank {
        "https://api.fshare.vn/"
    }

    @Provides @Singleton @Named("fshareClient")
    fun provideFshareClient(
        fshareHeaderInterceptor: FshareHeaderInterceptor,
        authInterceptor: AuthInterceptor,
        loggingInterceptor: HttpLoggingInterceptor,
    ): OkHttpClient = OkHttpClient.Builder()
        .connectTimeout(15, TimeUnit.SECONDS)
        .readTimeout(30, TimeUnit.SECONDS)
        .writeTimeout(30, TimeUnit.SECONDS)
        .callTimeout(60, TimeUnit.SECONDS)
        // Order: Header trước (UA + cache-control), Auth (Cookie) sau, Logging cuối.
        .addInterceptor(fshareHeaderInterceptor)
        .addInterceptor(authInterceptor)
        .addInterceptor(loggingInterceptor)
        .retryOnConnectionFailure(true)
        .build()

    @Provides @Singleton @Named("fshareRetrofit")
    fun provideFshareRetrofit(
        @Named("fshareClient") client: OkHttpClient,
        moshi: Moshi,
        @Named("fshareBaseUrl") baseUrl: String,
    ): Retrofit = Retrofit.Builder()
        .baseUrl(baseUrl)
        .client(client)
        .addConverterFactory(MoshiConverterFactory.create(moshi))
        .build()

    @Provides @Singleton
    fun provideAuthApi(@Named("fshareRetrofit") retrofit: Retrofit): AuthApi =
        retrofit.create(AuthApi::class.java)

    @Provides @Singleton
    fun provideMeApi(@Named("fshareRetrofit") retrofit: Retrofit): MeApi =
        retrofit.create(MeApi::class.java)

    @Provides @Singleton
    fun provideFilesApi(@Named("fshareRetrofit") retrofit: Retrofit): FilesApi =
        retrofit.create(FilesApi::class.java)

    @Provides @Singleton
    fun provideSessionApi(@Named("fshareRetrofit") retrofit: Retrofit): SessionApi =
        retrofit.create(SessionApi::class.java)

    // ─── Timfshare host config ────────────────────────────────────────

    @Provides @Singleton @Named("timfshareUserAgent")
    fun provideTimfshareUserAgent(): String = BuildConfig.TIMFSHARE_UA.ifBlank {
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 " +
            "(KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36"
    }

    @Provides @Singleton @Named("timfshareBearer")
    fun provideTimfshareBearer(): String = BuildConfig.TIMFSHARE_BEARER

    @Provides @Singleton @Named("timfshareSearchBaseUrl")
    fun provideTimfshareSearchBaseUrl(): String = "https://api.timfshare.com/"

    @Provides @Singleton @Named("timfshareTrendingBaseUrl")
    fun provideTimfshareTrendingBaseUrl(): String = "https://timfshare.com/"

    /**
     * Client cho Timfshare search (cần Bearer + Chrome UA).
     * KHÔNG share Fshare auth interceptor — Timfshare server không nhận UA Fshare.
     */
    @Provides @Singleton @Named("timfshareSearchClient")
    fun provideTimfshareSearchClient(
        @Named("timfshareUserAgent") userAgent: String,
        @Named("timfshareBearer") bearer: String,
        loggingInterceptor: HttpLoggingInterceptor,
    ): OkHttpClient = OkHttpClient.Builder()
        .connectTimeout(15, TimeUnit.SECONDS)
        .readTimeout(30, TimeUnit.SECONDS)
        .callTimeout(45, TimeUnit.SECONDS)
        .addInterceptor { chain ->
            val req = chain.request().newBuilder()
                .header("User-Agent", userAgent)
                .apply {
                    if (bearer.isNotBlank()) header("Authorization", bearer)
                }
                .header("Accept", "application/json")
                .build()
            chain.proceed(req)
        }
        .addInterceptor(loggingInterceptor)
        .retryOnConnectionFailure(true)
        .build()

    /** Client cho Timfshare trending — không cần Bearer, chỉ UA Chrome. */
    @Provides @Singleton @Named("timfshareTrendingClient")
    fun provideTimfshareTrendingClient(
        @Named("timfshareUserAgent") userAgent: String,
        loggingInterceptor: HttpLoggingInterceptor,
    ): OkHttpClient = OkHttpClient.Builder()
        .connectTimeout(15, TimeUnit.SECONDS)
        .readTimeout(30, TimeUnit.SECONDS)
        .callTimeout(45, TimeUnit.SECONDS)
        .addInterceptor { chain ->
            val req = chain.request().newBuilder()
                .header("User-Agent", userAgent)
                .header("Accept", "application/json")
                .build()
            chain.proceed(req)
        }
        .addInterceptor(loggingInterceptor)
        .retryOnConnectionFailure(true)
        .build()

    @Provides @Singleton
    fun provideTimfshareApi(
        @Named("timfshareSearchClient") client: OkHttpClient,
        @Named("timfshareSearchBaseUrl") baseUrl: String,
        moshi: Moshi,
    ): TimfshareApi = Retrofit.Builder()
        .baseUrl(baseUrl)
        .client(client)
        .addConverterFactory(MoshiConverterFactory.create(moshi))
        .build()
        .create(TimfshareApi::class.java)

    @Provides @Singleton
    fun provideTimfshareTrendingApi(
        @Named("timfshareTrendingClient") client: OkHttpClient,
        @Named("timfshareTrendingBaseUrl") baseUrl: String,
        moshi: Moshi,
    ): TimfshareTrendingApi = Retrofit.Builder()
        .baseUrl(baseUrl)
        .client(client)
        .addConverterFactory(MoshiConverterFactory.create(moshi))
        .build()
        .create(TimfshareTrendingApi::class.java)

    // ─── Google Sheets CSV config ────────────────────────────────────

    @Provides @Singleton @Named("sheetsBaseUrl")
    fun provideSheetsBaseUrl(): String = "https://docs.google.com/"

    @Provides @Singleton @Named("sheetsClient")
    fun provideSheetsClient(
        loggingInterceptor: HttpLoggingInterceptor,
    ): OkHttpClient = OkHttpClient.Builder()
        .connectTimeout(15, TimeUnit.SECONDS)
        .readTimeout(30, TimeUnit.SECONDS)
        .callTimeout(45, TimeUnit.SECONDS)
        .followRedirects(true)
        .followSslRedirects(true)
        .addInterceptor { chain ->
            val req = chain.request().newBuilder()
                .header("User-Agent", "Streamix/1.0")
                .build()
            chain.proceed(req)
        }
        .addInterceptor(loggingInterceptor)
        .retryOnConnectionFailure(true)
        .build()

    @Provides @Singleton
    fun provideSheetsApi(
        @Named("sheetsClient") client: OkHttpClient,
        @Named("sheetsBaseUrl") baseUrl: String,
    ): SheetsApi = Retrofit.Builder()
        .baseUrl(baseUrl)
        .client(client)
        .addConverterFactory(ScalarsConverterFactory.create())  // CSV → String
        .build()
        .create(SheetsApi::class.java)

    // ─── Helpers ──────────────────────────────────────────────────────

    private fun isDebugBuild(): Boolean = try {
        Class.forName("vn.streamix.tv.BuildConfig")
            .getField("DEBUG").getBoolean(null)
    } catch (_: Throwable) { false }
}
