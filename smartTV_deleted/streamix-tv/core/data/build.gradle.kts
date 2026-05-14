plugins {
    id("streamix.android.library")
    alias(libs.plugins.ksp)
    id("streamix.hilt")
}

android {
    namespace = "vn.streamix.tv.core.data"
}

dependencies {
    api(project(":core:domain"))
    api(project(":core:network"))

    implementation(libs.androidx.room.runtime)
    implementation(libs.androidx.room.ktx)
    ksp(libs.androidx.room.compiler)

    implementation(libs.androidx.datastore.preferences)
    implementation(libs.androidx.security.crypto)

    implementation(libs.timber)

    testImplementation(libs.junit)
    testImplementation(libs.kotlinx.coroutines.test)
    testImplementation(libs.mockk)
    testImplementation(libs.turbine)
    testImplementation(libs.mockwebserver)
}
