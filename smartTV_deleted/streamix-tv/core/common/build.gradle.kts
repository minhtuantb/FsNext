plugins {
    id("streamix.android.library")
}

android {
    namespace = "vn.streamix.tv.core.common"
}

dependencies {
    implementation(libs.timber)
    testImplementation(libs.junit)
    testImplementation(libs.kotlinx.coroutines.test)
}
