plugins {
    id("streamix.android.library.compose")
}

android {
    namespace = "vn.streamix.tv.core.ui"
}

dependencies {
    api(libs.androidx.tv.material)
    api(libs.androidx.compose.material.icons)
    api(libs.androidx.lifecycle.runtime.compose)

    api(libs.coil.compose)
    api(libs.timber)
    api(libs.zxing.core)
}
