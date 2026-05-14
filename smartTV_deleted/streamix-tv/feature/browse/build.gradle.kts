plugins {
    id("streamix.android.library.compose")
    alias(libs.plugins.ksp)
    alias(libs.plugins.kotlin.serialization)
    id("streamix.hilt")
}

android {
    namespace = "vn.streamix.tv.feature.browse"
}

dependencies {
    api(project(":core:common"))
    api(project(":core:domain"))
    api(project(":core:data"))
    api(project(":core:ui"))

    implementation(libs.androidx.activity.compose)
    implementation(libs.androidx.lifecycle.runtime.compose)
    implementation(libs.androidx.lifecycle.viewmodel.compose)
    implementation(libs.androidx.navigation.compose)
    implementation(libs.kotlinx.serialization.json)
    implementation(libs.hilt.navigation.compose)

    implementation(libs.timber)
}
