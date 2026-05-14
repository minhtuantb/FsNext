plugins {
    id("streamix.android.library")
    alias(libs.plugins.kotlin.serialization)
}

android {
    namespace = "vn.streamix.tv.core.domain"
}

dependencies {
    api(project(":core:common"))

    // Coroutines + Flow (cho repository observe* + UseCase)
    api(libs.kotlinx.coroutines.core)

    // javax.inject @Inject annotation cho UseCases (không depend Hilt ở core/domain)
    api(libs.javax.inject)

    implementation(libs.kotlinx.serialization.json)

    testImplementation(libs.junit)
    testImplementation(libs.kotlinx.coroutines.test)
}
