plugins {
    id("streamix.android.library.compose")
    alias(libs.plugins.ksp)
    id("streamix.hilt")
}

android {
    namespace = "vn.streamix.tv.feature.player"
}

dependencies {
    api(project(":core:common"))
    api(project(":core:domain"))
    api(project(":core:data"))
    api(project(":core:ui"))

    implementation(libs.androidx.lifecycle.runtime.compose)
    implementation(libs.androidx.lifecycle.viewmodel.compose)
    implementation(libs.hilt.navigation.compose)

    // Media3 ExoPlayer
    implementation(libs.androidx.media3.exoplayer)
    implementation(libs.androidx.media3.exoplayer.hls)
    implementation(libs.androidx.media3.exoplayer.dash)
    implementation(libs.androidx.media3.ui)
    implementation(libs.androidx.media3.session)
    implementation(libs.androidx.media3.common)

    implementation(libs.timber)
}
