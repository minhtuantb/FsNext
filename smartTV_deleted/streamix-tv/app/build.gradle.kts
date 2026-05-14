/*
 * StreamIX TV — :app module
 * Reference: 04 rev3 §4.2 Build pipeline; 13 §3 API config
 */
import java.util.Properties

plugins {
    id("streamix.android.application")
    alias(libs.plugins.kotlin.compose)
    alias(libs.plugins.kotlin.serialization)
    id("streamix.hilt")
}

val keystoreProperties = Properties().apply {
    val f = rootProject.file("keystore.properties")
    if (f.exists()) load(f.inputStream())
}

fun gitVersion(): Pair<String, Int> {
    return try {
        val tag = providers.exec {
            commandLine("git", "describe", "--tags", "--abbrev=0")
        }.standardOutput.asText.get().trim()
        val (major, minor, patch) = tag.removePrefix("v").split(".").map { it.toInt() }
        "$major.$minor.$patch" to (major * 10000 + minor * 100 + patch)
    } catch (_: Exception) {
        "0.0.1" to 1   // Phase 0 spike fallback
    }
}

android {
    namespace = "vn.streamix.tv"

    defaultConfig {
        applicationId = "vn.streamix.tv"   // D-4 đã chốt
        val (vName, vCode) = gitVersion()
        versionName = vName
        versionCode = vCode

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        // ─── Fshare LEGACY API (Wave M1) ───────────────────────────────
        val fshareAppKey = System.getenv("STREAMIX_FSHARE_APP_KEY")
            ?: keystoreProperties.getProperty("STREAMIX_FSHARE_APP_KEY")
            ?: project.findProperty("streamix.fshare.app.key") as? String
            ?: "tdf5ATEa3VUuGeSyw98D1YLSKvFr8pch"
        buildConfigField("String", "FSHARE_APP_KEY", "\"$fshareAppKey\"")

        val fshareBaseUrl = System.getenv("STREAMIX_FSHARE_API_BASE_URL")
            ?: project.findProperty("streamix.fshare.api.base.url") as? String
            ?: "https://api.fshare.vn/"
        buildConfigField("String", "FSHARE_API_BASE_URL", "\"$fshareBaseUrl\"")

        val fshareUserAgent = System.getenv("STREAMIX_FSHARE_USER_AGENT")
            ?: project.findProperty("streamix.fshare.user.agent") as? String
            ?: "FshareVideoDesktop_23052023"
        buildConfigField("String", "FSHARE_USER_AGENT", "\"$fshareUserAgent\"")

        val shareReferralId = System.getenv("STREAMIX_SHARE_REFERRAL_ID")
            ?: project.findProperty("streamix.share.referral.id") as? String
            ?: "8805984"
        buildConfigField("String", "FSHARE_SHARE_REFERRAL_ID", "\"$shareReferralId\"")

        // ─── Timfshare partner (M3) ────────────────────────────────────
        // Bearer JWT do partner cấp — đọc từ env hoặc keystore.properties.
        // Token không hết hạn (expires=0) nhưng vẫn coi là secret, KHÔNG commit.
        val timfshareBearer = System.getenv("STREAMIX_TIMFSHARE_BEARER")
            ?: keystoreProperties.getProperty("STREAMIX_TIMFSHARE_BEARER")
            ?: project.findProperty("streamix.timfshare.bearer") as? String
            ?: ""
        buildConfigField("String", "TIMFSHARE_BEARER", "\"$timfshareBearer\"")

        val timfshareUa = System.getenv("STREAMIX_TIMFSHARE_UA")
            ?: project.findProperty("streamix.timfshare.ua") as? String
            ?: "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36"
        buildConfigField("String", "TIMFSHARE_UA", "\"$timfshareUa\"")

        // ─── Google Sheets (M3) ────────────────────────────────────────
        buildConfigField("String", "SHEET_GOIY_FILE_ID", "\"1wDBrW0D7KhrLJW8HU1aaIwDLzrFNkeB0\"")
        buildConfigField("String", "SHEET_GOIY_GID", "\"1370676663\"")
        buildConfigField("String", "SHEET_XUHUONG_FILE_ID", "\"1r2e9W81ostp9PmosITlajdzKK4oY275Q\"")
        buildConfigField("String", "SHEET_XUHUONG_GID", "\"936805767\"")
        buildConfigField("String", "SHEET_TOP_IMDB_FILE_ID", "\"\"")
        buildConfigField("String", "SHEET_TOP_IMDB_GID", "\"\"")
        buildConfigField("String", "SHEET_TOP_RT_FILE_ID", "\"\"")
        buildConfigField("String", "SHEET_TOP_RT_GID", "\"\"")
        buildConfigField("String", "SHEET_TOP_FSHARE_FILE_ID", "\"\"")
        buildConfigField("String", "SHEET_TOP_FSHARE_GID", "\"\"")

        // ─── GA4 (M3 placeholder) ──────────────────────────────────────
        // Owner sẽ gửi production GA4_API_SECRET sau. Hiện tạm rỗng.
        val ga4Secret = System.getenv("STREAMIX_GA4_API_SECRET")
            ?: keystoreProperties.getProperty("STREAMIX_GA4_API_SECRET")
            ?: ""
        buildConfigField("String", "GA4_MEASUREMENT_ID", "\"G-252LMTWNB9\"")
        buildConfigField("String", "GA4_API_SECRET", "\"$ga4Secret\"")

        // SHA-256 fingerprint của signing cert (verify khi cài APK update)
        val expectedSig = System.getenv("STREAMIX_SIGNATURE_SHA256") ?: ""
        buildConfigField("String", "EXPECTED_SIGNATURE_SHA256", "\"$expectedSig\"")
    }

    signingConfigs {
        create("release") {
            storeFile = file(
                System.getenv("KEYSTORE_PATH")
                    ?: keystoreProperties.getProperty("storeFile")
                    ?: "../signing/release.jks"
            )
            storePassword = System.getenv("KEYSTORE_PASSWORD")
                ?: keystoreProperties.getProperty("storePassword")
            keyAlias = System.getenv("KEY_ALIAS")
                ?: keystoreProperties.getProperty("keyAlias")
                ?: "streamix-tv"
            keyPassword = System.getenv("KEY_PASSWORD")
                ?: keystoreProperties.getProperty("keyPassword")

            enableV1Signing = false
            enableV2Signing = true
            enableV3Signing = true
            enableV4Signing = false
        }
    }

    buildTypes {
        debug {
            applicationIdSuffix = ".debug"
            versionNameSuffix = "-DEBUG"
            isMinifyEnabled = false
            // Debug vẫn ký bằng release để cài đè khi switch
            // Tránh "App not installed" do signature mismatch
            // (Phương án A — ko dùng debug-only key)
        }
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            signingConfig = signingConfigs.getByName("release")
        }
    }

    splits {
        abi {
            isEnable = false   // Phương án A: chỉ universal APK
        }
    }

    buildFeatures {
        buildConfig = true
        compose = true
    }

    packaging {
        resources {
            excludes += "/META-INF/{AL2.0,LGPL2.1}"
            excludes += "/META-INF/INDEX.LIST"
        }
    }

    // Hỗ trợ TV cũ (Android 5/6): bật multidex
    defaultConfig.multiDexEnabled = true
}

dependencies {
    implementation(project(":core:common"))
    implementation(project(":core:domain"))
    implementation(project(":core:network"))
    implementation(project(":core:data"))
    implementation(project(":core:ui"))

    implementation(project(":feature:splash"))
    implementation(project(":feature:auth"))
    implementation(project(":feature:home"))
    implementation(project(":feature:browse"))
    implementation(project(":feature:player"))
    implementation(project(":feature:search"))
    implementation(project(":feature:settings"))
    implementation(project(":feature:onboarding"))

    // Compose
    implementation(platform(libs.androidx.compose.bom))
    implementation(libs.androidx.compose.ui)
    implementation(libs.androidx.compose.foundation)
    implementation(libs.androidx.compose.material3)
    implementation(libs.androidx.compose.ui.tooling.preview)
    debugImplementation(libs.androidx.compose.ui.tooling)

    implementation(libs.androidx.tv.material)

    // Activity / Lifecycle / Navigation
    implementation(libs.androidx.activity.compose)
    implementation(libs.androidx.lifecycle.runtime.compose)
    implementation(libs.androidx.navigation.compose)
    implementation(libs.kotlinx.serialization.json)

    // Hilt navigation
    implementation(libs.hilt.navigation.compose)

    // SplashScreen compat (backport postSplashScreenTheme attribute cho API < 31)
    implementation(libs.androidx.core.splashscreen)

    // Logging
    implementation(libs.timber)

    testImplementation(libs.junit)
}
