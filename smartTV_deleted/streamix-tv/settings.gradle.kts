/*
 * StreamIX TV — Settings (Gradle multi-module)
 * Reference: D:\Work\FsNext\smartTV\documents\02_khuyen-nghi-kien-truc.md §2.4
 */
@file:Suppress("UnstableApiUsage")

pluginManagement {
    includeBuild("build-logic")
    repositories {
        google {
            content {
                includeGroupByRegex("com\\.android.*")
                includeGroupByRegex("com\\.google.*")
                includeGroupByRegex("androidx.*")
            }
        }
        mavenCentral()
        gradlePluginPortal()
    }
}
plugins {
    id("org.gradle.toolchains.foojay-resolver-convention") version "0.10.0"
}

dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
    }
}

rootProject.name = "streamix-tv"

include(":app")

include(":core:common")
include(":core:domain")
include(":core:network")
include(":core:data")
include(":core:ui")

include(":feature:splash")
include(":feature:auth")
include(":feature:home")
include(":feature:browse")
include(":feature:player")
include(":feature:search")
include(":feature:settings")
include(":feature:onboarding")
