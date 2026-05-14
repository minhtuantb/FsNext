---
title: 05 — Player media nhúng (Embedded Player)
date: 2026-05-04
---

# 05. Player media nhúng — kiến trúc & quyết định kỹ thuật

## 5.1 Vì sao quan trọng

Bản desktop hiện tại **mở app ngoài** (VLC/MPC/KMPlayer) để phát video. Trên TV không thể làm vậy:
- Không phải TV nào cũng có VLC, MPC… Cần app cài trước.
- Trải nghiệm rời rạc: phải back, mở Files, mở app khác.
- Mất khả năng resume playback, mất context "đang xem phim trong Fshare".

→ TV bắt buộc phải có **embedded player** ngay trong app, được control bằng remote thống nhất với phần còn lại của UI.

## 5.2 Yêu cầu chức năng

| # | Yêu cầu | Ưu tiên |
|---|---------|---------|
| F1 | Phát video trực tiếp từ stream URL Fshare (không cần tải xong) | P0 |
| F2 | Tua lùi/tới ±10s, ±30s; jump to time | P0 |
| F3 | Play / pause bằng nút OK trên remote | P0 |
| F4 | Hiển thị progress bar, time elapsed/remaining | P0 |
| F5 | Resume từ vị trí xem dở (mỗi 5s lưu vào Room) | P0 |
| F6 | Hỗ trợ format phổ biến: MP4, MKV, WebM, FLV, AVI | P0 |
| F7 | Codec: H.264, HEVC, VP9, AV1 (nếu device hỗ trợ) | P0 |
| F8 | Audio: AAC, MP3, AC3, EAC3, DTS (passthrough qua HDMI) | P1 |
| F9 | Subtitle: SRT, ASS/SSA, VTT, embedded MKV subs | P0 |
| F10 | Chọn audio track nếu video có nhiều track | P1 |
| F11 | Chọn subtitle track | P1 |
| F12 | Tải subtitle ngoài (file .srt cùng tên trong cùng folder Fshare) | P2 |
| F13 | Cast nhận từ app mobile | P2 (V2) |
| F14 | Picture-in-picture (PiP) | P3 |
| F15 | Thay đổi tốc độ phát 0.5x – 2x | P2 |
| F16 | Hiển thị bitrate, codec info (debug menu) | P2 |
| F17 | Buffering chỉ báo (loading icon) | P0 |
| F18 | Recover lỗi mạng: tự retry 3 lần với exponential backoff trước khi báo lỗi | P0 |
| F19 | Network indicator (Wi-Fi yếu cảnh báo trước khi stream) | P1 |
| F20 | Force "use lower quality" cho Wi-Fi yếu (nếu Fshare cho phép multi-bitrate) | P2 |

## 5.3 Engine: ExoPlayer (Media3) là default

### 5.3.1 Vì sao ExoPlayer

- **Chính chủ Google**, hệ sinh thái Android TV/Compose-TV tích hợp sẵn.
- Hỗ trợ adaptive streaming (HLS, DASH), DRM, captions ra ngoài, timeline thumbnail.
- Có `PlayerView` cho View system và `Player` API thuần để ghép Compose.
- Audio passthrough cho AC3/EAC3/DTS qua AudioCapabilities.
- Cập nhật đều đặn, fix codec bug nhanh.

### 5.3.2 Hạn chế ExoPlayer

- **Codec lạ**: RMVB, một số biến thể XviD cũ → fail.
- **Subtitle ASS/SSA**: ExoPlayer text renderer hiện không render đầy đủ ASS effects (bold/italic OK; positioning, karaoke, fade — yếu).
- Một số file MKV với codec hiếm → ExoPlayer fail mở; libVLC mở được.

→ **Fallback chiến lược**: libVLC-Android. Khi ExoPlayer raise `ExoPlaybackException` với type `TYPE_RENDERER`, switch sang libVLC.

### 5.3.3 Tích hợp ExoPlayer vào Compose

```kotlin
@Composable
fun TvPlayerScreen(
    videoUrl: String,
    initialPosition: Long,
    onPositionUpdate: (Long) -> Unit,
    viewModel: PlayerViewModel = hiltViewModel(),
) {
    val context = LocalContext.current
    val exoPlayer = remember {
        ExoPlayer.Builder(context)
            .setMediaSourceFactory(
                DefaultMediaSourceFactory(context).setLiveTargetOffsetMs(5_000)
            )
            .setLoadControl(
                DefaultLoadControl.Builder()
                    .setBufferDurationsMs(15_000, 50_000, 1_500, 2_000)
                    .build()
            )
            .build()
            .apply {
                setMediaItem(MediaItem.fromUri(videoUrl))
                seekTo(initialPosition)
                prepare()
                playWhenReady = true
                addListener(object : Player.Listener {
                    override fun onPlayerError(error: PlaybackException) {
                        viewModel.onError(error)  // có thể switch sang libVLC tại đây
                    }
                })
            }
    }

    DisposableEffect(Unit) {
        val ticker = object : Runnable {
            override fun run() {
                onPositionUpdate(exoPlayer.currentPosition)
                Handler(Looper.getMainLooper()).postDelayed(this, 5_000)
            }
        }
        Handler(Looper.getMainLooper()).postDelayed(ticker, 5_000)
        onDispose {
            Handler(Looper.getMainLooper()).removeCallbacks(ticker)
            exoPlayer.release()
        }
    }

    AndroidView(
        factory = { ctx ->
            PlayerView(ctx).apply {
                player = exoPlayer
                useController = false  // tự build controls Compose để kiểm soát D-pad
                resizeMode = AspectRatioFrameLayout.RESIZE_MODE_FIT
            }
        },
        modifier = Modifier.fillMaxSize()
    )

    PlayerOverlayControls(
        player = exoPlayer,
        modifier = Modifier.fillMaxSize()
    )
}
```

`useController = false` rồi tự build overlay Compose là khuyến nghị bắt buộc — controls mặc định của ExoPlayer không hỗ trợ D-pad mượt; phải tự code focus model.

## 5.4 Nguồn stream URL từ Fshare

Bản desktop dùng API `/api/session/download` để lấy real-link tải file. Cùng endpoint này tạo URL có thể dùng làm **stream URL cho ExoPlayer**:

```kotlin
class GetStreamUrlUseCase @Inject constructor(
    private val api: FshareApiService,
    private val auth: AuthRepository,
) {
    suspend operator fun invoke(file: FileItem, password: String? = null): Result<String> =
        runCatching {
            val resp = api.getDownloadUrl(
                fcode = file.code,
                token = auth.session()!!.token,
                password = password,
                type = "stream"   // hoặc bỏ trống nếu Fshare API không có flag riêng cho stream
            )
            require(resp.code == 200) { "Server error ${resp.code}: ${resp.msg}" }
            resp.location
        }
}
```

**Lưu ý quan trọng**:
- URL stream có **TTL** (thường 6–24h tuỳ Fshare). Nếu user pause lâu rồi resume, có thể URL hết hạn → lỗi 403. Phải retry: gọi lại `getDownloadUrl` lấy URL mới rồi `setMediaItem(...)` + `seekTo(currentPosition)`.
- Cần header `User-Agent` và cookie `PHPSESSID` khi GET URL stream:
  ```kotlin
  val httpDataSourceFactory = DefaultHttpDataSource.Factory()
      .setUserAgent("FsNextTV/1.0")
      .setDefaultRequestProperties(mapOf("Cookie" to "PHPSESSID=$session"))
  ```
- Hỗ trợ **range request** là yêu cầu cứng cho seek. Test cẩn thận với CDN của Fshare.

## 5.5 Subtitle

### 5.5.1 Subtitle nhúng trong MKV

Hầu hết file MKV mà user Fshare lưu có subtitle nhúng (track #2, #3…). ExoPlayer phát hiện được → expose qua `Player.getCurrentTracks()`. UI cho user chọn:

```kotlin
val tracks by remember(player) { mutableStateOf(player.currentTracks) }
val textGroups = tracks.groups.filter { it.type == C.TRACK_TYPE_TEXT }
// Hiển thị danh sách textGroups trong overlay; click để select track
```

### 5.5.2 Subtitle ngoài (.srt cùng folder)

Với mỗi video, gọi `ListFiles` trên folder cha, tìm file `.srt`/`.ass` cùng tên. Nếu có:

```kotlin
val mediaItem = MediaItem.Builder()
    .setUri(streamUrl)
    .setSubtitleConfigurations(
        listOf(
            MediaItem.SubtitleConfiguration.Builder(srtUrl)
                .setMimeType(MimeTypes.APPLICATION_SUBRIP)
                .setLanguage("vi")
                .setSelectionFlags(C.SELECTION_FLAG_DEFAULT)
                .build()
        )
    )
    .build()
```

ExoPlayer tự fetch và sync. Cẩn thận: file SRT cũng cần auth header tương tự video.

### 5.5.3 ASS/SSA — fallback libVLC

Nếu user mở video có subtitle ASS với hiệu ứng phức tạp (anime fansub thường) và họ phàn nàn, switch sang libVLC. UI có toggle Settings → "Engine player" → ExoPlayer (mặc định) / libVLC.

## 5.6 Audio passthrough (HDMI bitstream)

Khi user nối TV qua AVR (Yamaha, Denon…) qua HDMI ARC/eARC, AC3/EAC3/DTS phải được truyền nguyên (passthrough), không decode trên TV. ExoPlayer hỗ trợ qua `DefaultAudioSink` + `AudioCapabilities`:

```kotlin
val audioSink = DefaultAudioSink.Builder(context)
    .setAudioCapabilities(AudioCapabilities.getCapabilities(context))
    .setEnableFloatOutput(false)
    .setEnableAudioTrackPlaybackParams(true)
    .build()

val renderersFactory = DefaultRenderersFactory(context)
    .setExtensionRendererMode(DefaultRenderersFactory.EXTENSION_RENDERER_MODE_OFF)

val player = ExoPlayer.Builder(context, renderersFactory)
    .setAudioAttributes(
        AudioAttributes.Builder()
            .setUsage(C.USAGE_MEDIA)
            .setContentType(C.AUDIO_CONTENT_TYPE_MOVIE)
            .build(),
        true  // handleAudioFocus
    )
    .build()
```

Test ma trận: TV → bridge AVR → loa. Nếu AVR không nhận DTS thì fall back decode trên app.

## 5.7 Resume playback

```kotlin
@Entity(tableName = "playback_position")
data class PlaybackPosition(
    @PrimaryKey val fileCode: String,
    val positionMs: Long,
    val durationMs: Long,
    val updatedAt: Long,
)

@Dao
interface PlaybackPositionDao {
    @Query("SELECT * FROM playback_position WHERE fileCode = :code")
    suspend fun get(code: String): PlaybackPosition?

    @Upsert
    suspend fun upsert(p: PlaybackPosition)

    // Continue Watching row trên Home
    @Query("""
        SELECT * FROM playback_position 
        WHERE positionMs > 30000 AND positionMs < durationMs - 60000 
        ORDER BY updatedAt DESC LIMIT 20
    """)
    fun continueWatching(): Flow<List<PlaybackPosition>>
}
```

Tick mỗi 5s → upsert. Khi mở lại file → check, nếu có position > 30s và < duration−60s → dialog "Tiếp tục từ {time}? — Tiếp tục / Bắt đầu lại".

## 5.8 Chất lượng & adaptive

Fshare hiện tại không cung cấp HLS/DASH manifest — file là single-bitrate. Nghĩa là V1 chỉ play raw file: chất lượng phụ thuộc Wi-Fi, không có ABR (Adaptive Bitrate).

V2: nếu Fshare backend triển khai transcode-on-demand → manifest HLS có 480p/720p/1080p → ExoPlayer tự ABR theo bandwidth. Trong V1 chỉ thêm:
- Network meter trước khi mở → cảnh báo "Wi-Fi 2.5 Mbps có thể không đủ cho 1080p".
- Buffer aggressive (15s prebuffer) cho Wi-Fi yếu.

## 5.9 PiP & background play

Android TV không hỗ trợ PiP cho video người dùng (chỉ cho voice assistant). Skip.

Background audio chỉ có ý nghĩa cho file audio, không phải video. V2 nếu cần: `MediaSessionService` foreground service, expose qua `MediaController` để remote control.

## 5.10 Test ma trận

| Format | Codec video | Codec audio | Subtitle | Status mong đợi |
|--------|-------------|-------------|----------|-----------------|
| MP4 | H.264 | AAC | none / SRT ngoài | Pass ExoPlayer |
| MP4 | HEVC | AAC | embedded | Pass ExoPlayer (TV phải hỗ trợ HEVC hardware) |
| MKV | H.264 | AC3 | embedded SRT | Pass ExoPlayer |
| MKV | HEVC 10-bit | EAC3 | embedded ASS | Pass ExoPlayer (subtitle có thể fallback VLC) |
| MKV | HEVC 10-bit | DTS-HD MA | embedded ASS | Có thể fail ExoPlayer; pass VLC |
| AVI | XviD/DivX | MP3 | none | Có thể fail ExoPlayer; pass VLC |
| FLV | H.264 | AAC | none | Pass ExoPlayer |
| RMVB | RV40 | Cook | none | Fail ExoPlayer; pass VLC (cố gắng) |

→ Phải có **switch engine** bằng tay trong UI; tự động switch khi ExoPlayer raise `ERROR_CODE_DECODER_INIT_FAILED`.

## 5.11 Kết luận chương 5

- **Default engine: Media3 ExoPlayer**, build controls Compose riêng (không dùng `PlayerView` controller mặc định).
- **Fallback engine: libVLC-Android** cho codec lạ và ASS effect.
- Stream URL có TTL — phải retry refresh URL khi 403.
- Resume playback bằng Room, tick 5s.
- Audio passthrough, subtitle nhúng, subtitle ngoài cùng folder — đều P0.
- ABR/HLS là V2, V1 chấp nhận single-bitrate.

Tiếp theo: UX 10-foot ([06](06_ux-10foot-dpad.md)).
