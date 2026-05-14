# StreamIX TV — Consumer ProGuard rules
# Rules này tự áp dụng cho mọi consumer dùng module này.

# Giữ tên class @JsonClass + @Json (Moshi codegen)
-keep,allowobfuscation,allowshrinking @com.squareup.moshi.JsonClass class *

# Giữ class data + sealed interface domain (cho Moshi/Hilt) 
-keep class **.model.** { *; }
