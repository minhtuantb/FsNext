plugins {
    id("streamix.android.library")
    alias(libs.plugins.ksp)
    alias(libs.plugins.kotlin.serialization)
    id("streamix.hilt")
}

android {
    namespace = "vn.streamix.tv.core.network"
    buildFeatures.buildConfig = true
    defaultConfig {
        // Stub mặc định — :app override khi build (đặt giá trị thật ở app/build.gradle.kts).
        // Migrate sang Fshare LEGACY API ở Wave M1, thêm Timfshare/Sheets/GA4 ở Wave M3.
        buildConfigField("String", "FSHARE_API_BASE_URL", "\"https://api.fshare.vn/\"")
        buildConfigField("String", "FSHARE_APP_KEY", "\"\"")
        buildConfigField("String", "FSHARE_USER_AGENT", "\"FshareVideoDesktop_23052023\"")
        buildConfigField("String", "FSHARE_SHARE_REFERRAL_ID", "\"8805984\"")

        // Timfshare partner (M3)
        buildConfigField("String", "TIMFSHARE_BEARER", "\"\"")
        buildConfigField("String", "TIMFSHARE_UA", "\"\"")

        // Google Sheets (M3)
        buildConfigField("String", "SHEET_GOIY_FILE_ID", "\"1wDBrW0D7KhrLJW8HU1aaIwDLzrFNkeB0\"")
        buildConfigField("String", "SHEET_GOIY_GID", "\"1370676663\"")
        buildConfigField("String", "SHEET_XUHUONG_FILE_ID", "\"1r2e9W81ostp9PmosITlajdzKK4oY275Q\"")
        buildConfigField("String", "SHEET_XUHUONG_GID", "\"936805767\"")

        // Top sheets — owner sẽ cung cấp file IDs sau. Empty → repo trả empty list.
        buildConfigField("String", "SHEET_TOP_IMDB_FILE_ID", "\"\"")
        buildConfigField("String", "SHEET_TOP_IMDB_GID", "\"\"")
        buildConfigField("String", "SHEET_TOP_RT_FILE_ID", "\"\"")
        buildConfigField("String", "SHEET_TOP_RT_GID", "\"\"")
        buildConfigField("String", "SHEET_TOP_FSHARE_FILE_ID", "\"\"")
        buildConfigField("String", "SHEET_TOP_FSHARE_GID", "\"\"")

        // GA4 (M3 placeholder — owner sẽ gửi production key sau)
        buildConfigField("String", "GA4_MEASUREMENT_ID", "\"G-252LMTWNB9\"")
        buildConfigField("String", "GA4_API_SECRET", "\"\"")
    }
}

dependencies {
    api(project(":core:common"))
    api(project(":core:domain"))

    // api thay implementation — :core:data cần Moshi/Retrofit cho repository impls inject
    api(libs.retrofit)
    api(libs.retrofit.converter.moshi)
    api(libs.retrofit.converter.scalars)
    api(libs.okhttp)
    api(libs.moshi)
    api(libs.moshi.kotlin)

    implementation(libs.okhttp.logging)
    ksp(libs.moshi.codegen)

    implementation(libs.timber)

    testImplementation(libs.junit)
    testImplementation(libs.mockk)
    testImplementation(libs.kotlinx.coroutines.test)
    testImplementation(libs.mockwebserver)
}
