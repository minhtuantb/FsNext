# StreamIX TV — ProGuard / R8 rules
# Reference: 13 §27

# Moshi
-keepclasseswithmembers class * {
    @com.squareup.moshi.* <methods>;
}
-keep @com.squareup.moshi.JsonClass class *
-keepnames class kotlin.jvm.internal.DefaultConstructorMarker

# Retrofit
-keepattributes Signature, Exceptions, *Annotation*, EnclosingMethod
-keep class retrofit2.** { *; }
-keep class * implements retrofit2.Call

# OkHttp
-dontwarn okhttp3.**
-dontwarn okio.**
-dontwarn javax.annotation.**

# Hilt
-keep class dagger.hilt.** { *; }
-keep class * extends dagger.hilt.android.lifecycle.HiltViewModel
-keep,allowobfuscation,allowshrinking @dagger.hilt.android.lifecycle.HiltViewModel class *

# Kotlinx Serialization
-keepattributes *Annotation*, InnerClasses
-dontnote kotlinx.serialization.AnnotationsKt

# Kotlin reflection (optional, cho debug)
-keep class kotlin.Metadata { *; }

# Compose: AGP 8.5+ tự lo ProGuard rules cho compose, không thêm
-dontwarn androidx.compose.**

# Media3 ExoPlayer
-keep class androidx.media3.** { *; }
-dontwarn androidx.media3.**

# Models cần giữ tên (cho Moshi adapter)
-keep class vn.streamix.tv.core.network.model.** { *; }
-keep class vn.streamix.tv.core.domain.model.** { *; }

# Timber
-dontwarn org.jetbrains.annotations.**
