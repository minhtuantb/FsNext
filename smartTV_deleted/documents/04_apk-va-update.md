---
title: 04 — APK build, ký & phân phối thủ công (no in-app update)
date: 2026-05-04
revision: 3 — rewrite cho Phương án A sau quyết định D-3.5 ở 12
supersedes: revision 2 (in-app update playbook đã bỏ)
---

# 04. APK Build, Sign, Distribute — Manual (Phương án A)

## 4.0 Bối cảnh sau quyết định D-3.5

Theo `12_scope-decisions-v1.md` D-3.5: V1 dùng **Phương án A — Manual notify + manual install**.

Hệ luỵ:
- App **KHÔNG** có cơ chế update tự động.
- Không có endpoint manifest, không có UpdateChecker/Downloader/Installer.
- Mỗi lần phát hành mới = nhân viên IT post Slack + email; user vào portal nội bộ tải APK + cài đè.
- Tài liệu này tập trung vào **build pipeline + signing + distribution channel + communication protocol**.

So với revision 2: đã bỏ ~70% nội dung (manifest schema, code Kotlin update, rollback, kill switch). Rev 2 vẫn được giữ làm reference nếu V2 cần in-app update.

---

## 4.1 Versioning — quy ước

```
versionName = MAJOR.MINOR.PATCH                        (semver)
versionCode = MAJOR*10000 + MINOR*100 + PATCH          (integer monotonic)

1.0.0 → 10000
1.0.1 → 10001
1.2.5 → 10205
2.0.0 → 20000
```

Quy tắc:
- `versionCode` chỉ tăng — không bao giờ giảm.
- Mỗi commit lên `main` được tag = 1 versionCode mới.
- Hot-fix: bump PATCH (1.0.5 → 1.0.6).

Tự động hoá lấy version từ git tag (giống rev 2):

```kotlin
fun getVersionFromGit(): Pair<String, Int> {
    val tag = "git describe --tags --abbrev=0".runCommand().trim()
    val (major, minor, patch) = tag.removePrefix("v").split(".").map { it.toInt() }
    return "$major.$minor.$patch" to (major * 10000 + minor * 100 + patch)
}
```

---

## 4.2 Build pipeline

### 4.2.1 Gradle config (rút gọn cho Phương án A)

```kotlin
android {
    namespace = "vn.streamix.tv"
    compileSdk = 34

    defaultConfig {
        applicationId = "vn.streamix.tv"            // ✅ chốt D-4
        minSdk = 21
        targetSdk = 34
        // versionCode + versionName từ getVersionFromGit()
    }

    signingConfigs {
        create("release") {
            storeFile = file(System.getenv("KEYSTORE_PATH") ?: "../signing/release.jks")
            storePassword = System.getenv("KEYSTORE_PASSWORD")
            keyAlias = System.getenv("KEY_ALIAS")
            keyPassword = System.getenv("KEY_PASSWORD")
            enableV1Signing = false
            enableV2Signing = true
            enableV3Signing = true
            enableV4Signing = false
        }
    }

    buildTypes {
        debug { applicationIdSuffix = ".debug" }
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            signingConfig = signingConfigs.getByName("release")
        }
    }
}
```

Lưu ý: **KHÔNG còn** `buildConfigField "UPDATE_BASE_URL"` hay `EXPECTED_SIGNATURE_SHA256` — vì app không tự update nên không cần.

### 4.2.2 Permissions trong AndroidManifest

So với revision 2, đã bỏ:

```xml
<!-- ❌ KHÔNG còn cần (Phương án A) -->
<!-- <uses-permission android:name="android.permission.REQUEST_INSTALL_PACKAGES" /> -->
<!-- <uses-permission android:name="android.permission.RECEIVE_BOOT_COMPLETED" /> -->
```

V1 chỉ cần các permission core:

```xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
<uses-permission android:name="android.permission.WAKE_LOCK" />        <!-- player -->
<uses-permission android:name="android.permission.FOREGROUND_SERVICE" /> <!-- player session -->
```

### 4.2.3 Build commands

```bash
# Local debug
./gradlew assembleDebug

# Local release (cần env vars hoặc keystore.properties)
./gradlew assembleRelease

# Output:
# app/build/outputs/apk/release/app-universal-release.apk     (~22 MB)
```

V1 chỉ phân phối **1 APK universal** — đơn giản hoá toàn bộ pipeline; user không phải biết về ABI.

---

## 4.3 Signing — keystore

Quy trình signing không đổi so với revision 2 (vì signing là thuộc tính của APK build, không phụ thuộc update mechanism). Tóm tắt:

### 4.3.1 Tạo keystore

```bash
keytool -genkeypair -v \
  -keystore streamix-tv-release.jks \
  -alias streamix-tv \
  -keyalg RSA -keysize 4096 \
  -sigalg SHA256withRSA \
  -validity 10000 \
  -dname "CN=StreamIX TV, OU=Platform, O=Fshare, L=Hanoi, S=Hanoi, C=VN" \
  -storetype PKCS12
```

### 4.3.2 Verify

```bash
$ANDROID_HOME/build-tools/34.0.0/apksigner verify \
  --verbose --print-certs \
  app-universal-release.apk

# Mong đợi:
# Verified using v2 scheme: true
# Verified using v3 scheme: true
# Number of signers: 1
```

### 4.3.3 Lưu trữ — internal version

| Bản sao | Vị trí | Người giữ |
|---------|--------|-----------|
| Master | USB key VeraCrypt trong két công ty | CTO/Tech Lead |
| Backup CI | env var trong CI/CD | DevOps lead |
| Backup vault | 1Password/Bitwarden encrypted attachment | Tech Lead + 1 backup |
| Recovery doc | Giấy in (alias + fingerprint, KHÔNG password) | Két công ty |

Quy tắc cứng:
- Không gửi keystore qua email/Slack/Teams.
- Không commit vào git.
- Không lưu shared drive cá nhân.

**Lưu ý nghiêm trọng cho Phương án A**: vì app không có cơ chế force update, nếu mất keystore → toàn bộ user phải gỡ và cài lại app (mất history local). Trong Phương án A đây càng nghiêm trọng vì user không tự nhận thông báo trong app — phải phụ thuộc IT comm.

### 4.3.4 SHA-256 fingerprint

Lưu lại sau `keytool -list -v`:

```
SHA256: A9:B8:C7:D6:...:88
```

Tuy nhiên với Phương án A, fingerprint này KHÔNG hard-code trong app code (vì không có in-app update để verify). Chỉ lưu làm reference cho:
- IT verify APK trước khi đẩy lên server.
- User power-user / IT có thể verify thủ công bằng `apksigner verify --print-certs APK`.

---

## 4.4 Hosting — Internal portal đơn giản

Vì không có client gọi manifest, hosting chỉ cần phục vụ **2 thứ**:

1. APK file để user tải.
2. Trang HTML hướng dẫn cài (xem §4.5).

### 4.4.1 Phương án hosting

Đơn giản nhất theo thứ tự:

#### A1 — Slack/Teams file attachment (cho < 30 user pilot)

- Post APK trực tiếp vào channel `#streamix-tv-releases`.
- User scan QR hoặc click link Slack từ phone → tải về phone → AirDrop/USB sang TV → cài.

Pros: 0 setup, 0 cost.
Cons: Slack có giới hạn file size (1 GB free, OK cho 22 MB APK); message dễ trôi; không versioning.

#### A2 — Confluence/Notion page với download link (cho 30–100 user)

- Tạo trang nội bộ "StreamIX TV — Release portal".
- Mỗi version có 1 section: download link, release date, notes, instructions.
- Upload APK lên Confluence attachment hoặc S3 + link.

Pros: dễ maintain, có history.
Cons: cần SSO/VPN; mobile UX để tải xuống không hoàn hảo.

#### A3 — Static nginx server (khuyến nghị cho V1)

```nginx
server {
    listen 443 ssl http2;
    server_name tv.streamix.vn;     # hoặc tv-update.fshare.local nếu nội bộ DNS

    ssl_certificate     /etc/ssl/streamix-tv.crt;
    ssl_certificate_key /etc/ssl/streamix-tv.key;

    root /var/www/streamix-tv;
    index install.html;

    location ~ ^/apk/.+\.apk$ {
        add_header Cache-Control "public, max-age=86400";
        add_header Content-Type "application/vnd.android.package-archive";
        add_header Content-Disposition "attachment";
    }

    location / {
        # phục vụ install.html + assets
        add_header Cache-Control "public, max-age=300";
    }

    access_log /var/log/nginx/streamix-tv-access.log;
}
```

Cấu trúc thư mục:

```
/var/www/streamix-tv/
├── install.html              ← landing page (xem §4.5)
├── apk/
│   ├── streamix-tv-1.0.0-10000.apk
│   ├── streamix-tv-1.0.1-10001.apk
│   ├── streamix-tv-latest.apk         ← symlink → version mới nhất
│   └── …
├── assets/
│   ├── streamix-logo.svg
│   └── qr-latest.png         ← QR pointing to /apk/streamix-tv-latest.apk
└── archive/                  ← versions cũ > 90 ngày
```

Khi release version mới:

```bash
# CI/CD post-build script
ssh deploy@tv.streamix.vn <<EOF
cd /var/www/streamix-tv/apk
mv streamix-tv-latest.apk archive/streamix-tv-${PREV_VERSION}.apk 2>/dev/null || true
cp /tmp/streamix-tv-${VERSION}.apk .
ln -sf streamix-tv-${VERSION}.apk streamix-tv-latest.apk
qrencode -o /var/www/streamix-tv/assets/qr-latest.png \
    "https://tv.streamix.vn/apk/streamix-tv-latest.apk"
EOF
```

### 4.4.2 Bảo mật server

- HTTPS bắt buộc (Let's Encrypt ổn cho domain public; cert nội bộ nếu chỉ intranet).
- Bind portal sau VPN nếu chỉ nhân viên dùng.
- Tắt directory listing (`autoindex off`).
- Log truy cập để theo dõi adoption (số user tải mỗi version).

---

## 4.5 First install + Re-install — trang nội bộ

Vì Phương án A coi mỗi update là "first install", trang `install.html` là **flow chính** chứ không phải edge case như rev 2.

### 4.5.1 Nội dung trang

```html
<!doctype html>
<html lang="vi">
<head>
  <meta charset="utf-8">
  <title>StreamIX TV — Cài đặt & cập nhật</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: system-ui; max-width: 720px; margin: 40px auto; padding: 0 20px; line-height: 1.6; background: #0F1218; color: #EEF1F6; }
    h1 { font-size: 32px; }
    .version { background: rgba(255,177,44,0.12); padding: 16px 20px; border-radius: 12px; margin: 16px 0; }
    .version code { background: rgba(255,255,255,0.06); padding: 2px 8px; border-radius: 6px; }
    .step { background: #1A1F2A; padding: 20px; border-radius: 12px; margin: 16px 0; }
    .step h2 { margin-top: 0; }
    .download-btn { display: inline-block; background: #FFB12C; color: #0F1218; padding: 16px 32px; border-radius: 12px; text-decoration: none; font-weight: 700; font-size: 18px; }
    code { background: rgba(255,255,255,0.06); padding: 2px 6px; border-radius: 3px; }
    .qr { display: inline-block; padding: 16px; background: #fff; border-radius: 12px; }
  </style>
</head>
<body>

<h1>StreamIX TV</h1>
<p>Cổng cài đặt & cập nhật phiên bản dành cho nhân viên Fshare.</p>

<div class="version">
  <strong>Phiên bản mới nhất:</strong> <code id="ver">1.0.2</code>
  &nbsp;·&nbsp; <strong>Phát hành:</strong> <span id="date">15/06/2026</span>
  &nbsp;·&nbsp; <strong>Kích thước:</strong> 22 MB
</div>

<h3>Có gì mới (1.0.2)</h3>
<ul>
  <li>Sửa lỗi tua phim trên TCL Android TV</li>
  <li>Hỗ trợ phụ đề SSA tốt hơn</li>
  <li>Tối ưu cold start</li>
</ul>

<div class="step">
  <h2>Bước 1 — Bật cài app từ nguồn lạ</h2>
  <p>Trên TV: <b>Cài đặt → Bảo mật → Nguồn không xác định</b> → bật.</p>
  <p>Sony Bravia: Settings → Device Preferences → Security & restrictions → Unknown sources<br>
  Xiaomi Mi Box: Settings → Apps → Security & restrictions → Unknown sources<br>
  FPT Play Box: Cài đặt → Bảo mật → Nguồn lạ</p>
  <p><i>(Bước này chỉ làm 1 lần. Nếu đã từng cài StreamIX TV trước, có thể bỏ qua.)</i></p>
</div>

<div class="step">
  <h2>Bước 2 — Tải APK</h2>
  <p>Cách 1 — nếu TV có browser: nhấn nút bên dưới.</p>
  <p><a class="download-btn" href="/apk/streamix-tv-latest.apk">Tải StreamIX TV 1.0.2 (22 MB)</a></p>

  <p>Cách 2 — quét QR bằng điện thoại, gửi APK sang TV qua Bluetooth/AirDrop/USB:</p>
  <p class="qr"><img src="/assets/qr-latest.png" alt="QR" width="200" height="200"></p>

  <p>Cách 3 — qua ADB (cho IT/dev):</p>
  <pre>adb connect &lt;ip-tv&gt;:5555
adb install -r /path/to/streamix-tv-latest.apk</pre>
</div>

<div class="step">
  <h2>Bước 3 — Cài đè</h2>
  <p>Trên TV, mở <b>File Manager → Downloads → streamix-tv-latest.apk</b> → cài.</p>
  <p>Android sẽ hỏi <i>"Cập nhật ứng dụng hiện có?"</i> → chọn <b>Cập nhật</b>. Toàn bộ dữ liệu local (vị trí xem dở, cài đặt) được giữ nguyên vì cùng signing key.</p>
  <p><i>Nếu lần đầu cài, sẽ là "Cài đặt" thay vì "Cập nhật".</i></p>
</div>

<div class="step">
  <h2>Bước 4 — Kiểm tra version</h2>
  <p>Mở app → Cài đặt → Giới thiệu → kiểm tra version hiển thị là <code>1.0.2</code>.</p>
</div>

<hr>

<h3>Lịch sử các phiên bản</h3>
<table>
  <tr><th>Version</th><th>Ngày</th><th>Tải</th></tr>
  <tr><td>1.0.2 ⭐ mới nhất</td><td>15/06/2026</td><td><a href="/apk/streamix-tv-1.0.2-10202.apk">tải</a></td></tr>
  <tr><td>1.0.1</td><td>01/06/2026</td><td><a href="/apk/streamix-tv-1.0.1-10101.apk">tải</a></td></tr>
  <tr><td>1.0.0</td><td>15/05/2026</td><td><a href="/apk/streamix-tv-1.0.0-10000.apk">tải</a></td></tr>
</table>

<h3>Có vấn đề?</h3>
<p>Liên hệ <a href="mailto:tv-support@fshare.vn">tv-support@fshare.vn</a> hoặc Slack <code>#streamix-tv-support</code>.</p>

</body>
</html>
```

CI/CD tự update các trường `<span id="ver">`, `<span id="date">`, "Có gì mới", và bảng lịch sử mỗi lần release.

### 4.5.2 Phương pháp 2: Companion mobile app push

App Fshare mobile (đã tồn tại) có thể thêm chức năng "Cài StreamIX TV":
- Mobile app gọi `tv.streamix.vn` → tải APK trên phone.
- Mobile dùng SSDP/Bonjour tìm TV cùng Wi-Fi.
- Push APK qua HTTP server tạm thời lên TV → TV mở App File Manager → cài.

Đây là roadmap V2 — V1 dùng trang download là đủ.

---

## 4.6 Communication protocol — quan trọng nhất ở Phương án A

Vì app **không alert user**, IT/admin phải có quy trình thông báo chặt chẽ. Đây là yếu tố sống còn của Phương án A.

### 4.6.1 Slack channel `#streamix-tv-releases`

Bắt buộc cho mọi release. Template message:

```
:tada: *StreamIX TV 1.0.2 đã phát hành*  ·  15/06/2026

*Có gì mới*
• Sửa lỗi tua phim trên TCL Android TV
• Hỗ trợ phụ đề SSA tốt hơn
• Tối ưu cold start (-30%)

*Cách cập nhật* (mất ~3 phút)
👉 https://tv.streamix.vn

*Bắt buộc?*
Không bắt buộc — phiên bản 1.0.0 và 1.0.1 vẫn hoạt động bình thường.

*Hỗ trợ*
Slack #streamix-tv-support  ·  Email tv-support@fshare.vn
```

Channel rules:
- Không spam — chỉ release notification.
- Pin message version mới nhất.
- Reply trong thread cho mỗi báo cáo bug version đó.

### 4.6.2 Email blast

Cho user không active trên Slack. Mỗi release gửi 1 email tới list `streamix-tv-users@fpt.com.vn`. Template ngắn hơn Slack, kèm download link và instruction.

### 4.6.3 Nhắc nhở định kỳ

- Mỗi 7 ngày sau release, IT check log nginx xem tỷ lệ download/cài.
- Nếu < 50% sau 14 ngày → reminder Slack thread.
- Nếu < 70% sau 30 ngày → email cá nhân hoá user còn ở version cũ (cần map device → user, có thể tự nguyện).

### 4.6.4 Force update — không có ở Phương án A

Phương án A chấp nhận rằng user có thể dùng version cũ. **KHÔNG có** cơ chế force update. Mitigation:
- Chấp nhận rủi ro R-S2 (xem `08_rui-ro-va-giam-thieu.md`).
- Nếu xảy ra security incident bắt buộc upgrade: PM + IT phối hợp gửi email khẩn + gọi từng user nếu pilot < 50 người.

### 4.6.5 Critical bug trong version đã release

Quy trình hot-fix:
1. Tech Lead xác nhận bug.
2. Hot-fix branch → commit → tag PATCH → CI build.
3. Release lên `tv.streamix.vn` trong < 4 giờ.
4. Slack message với prefix `🚨 [HOT-FIX]`.
5. Email blast với subject `[KHẨN] StreamIX TV 1.0.x — vui lòng cập nhật`.
6. Gọi điện cho user pilot key nếu cần.

---

## 4.7 CI/CD — Github Actions ví dụ (rút gọn)

```yaml
# .github/workflows/release.yml
name: Release Build

on:
  push:
    tags: ['v*']

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with: { fetch-depth: 0 }

      - uses: actions/setup-java@v4
        with: { distribution: temurin, java-version: '17' }

      - name: Decode keystore
        run: echo "${{ secrets.KEYSTORE_BASE64 }}" | base64 -d > $RUNNER_TEMP/release.jks

      - name: Build release APK
        env:
          KEYSTORE_PATH: ${{ runner.temp }}/release.jks
          KEYSTORE_PASSWORD: ${{ secrets.KEYSTORE_PASSWORD }}
          KEY_ALIAS: ${{ secrets.KEY_ALIAS }}
          KEY_PASSWORD: ${{ secrets.KEY_PASSWORD }}
        run: ./gradlew assembleRelease

      - name: Verify APK signature
        run: |
          $ANDROID_HOME/build-tools/34.0.0/apksigner verify \
            --print-certs app/build/outputs/apk/release/app-universal-release.apk

      - name: Compute SHA-256
        id: hash
        run: |
          SHA=$(sha256sum app/build/outputs/apk/release/app-universal-release.apk | awk '{print $1}')
          echo "sha=$SHA" >> $GITHUB_OUTPUT

      - name: Rename APK
        run: |
          VERSION=${GITHUB_REF#refs/tags/v}
          VERSION_CODE=$(./gradlew -q printVersionCode)
          mv app/build/outputs/apk/release/app-universal-release.apk \
             /tmp/streamix-tv-${VERSION}-${VERSION_CODE}.apk

      - name: Upload to portal
        env:
          DEPLOY_KEY: ${{ secrets.DEPLOY_KEY }}
        run: |
          mkdir -p ~/.ssh && echo "$DEPLOY_KEY" > ~/.ssh/id_ed25519
          chmod 600 ~/.ssh/id_ed25519
          # upload APK
          scp -i ~/.ssh/id_ed25519 -o StrictHostKeyChecking=no \
            /tmp/streamix-tv-*.apk \
            deploy@tv.streamix.vn:/var/www/streamix-tv/apk/
          # update symlink + QR + install.html metadata
          ssh -i ~/.ssh/id_ed25519 deploy@tv.streamix.vn \
            "cd /var/www/streamix-tv && ./update-portal.sh ${GITHUB_REF#refs/tags/v}"

      - name: Notify Slack
        run: |
          curl -X POST -H 'Content-type: application/json' \
            --data "{
              \"text\": \":tada: StreamIX TV ${GITHUB_REF#refs/tags/v} đã phát hành\",
              \"attachments\": [{
                \"text\": \"Cách cập nhật: https://tv.streamix.vn\nSHA-256: ${{ steps.hash.outputs.sha }}\"
              }]
            }" \
            ${{ secrets.SLACK_WEBHOOK }}
```

So với rev 2: bỏ `generate-manifest.sh` (không có manifest); thêm step rename APK theo convention; Slack notify tự động — vì giờ Slack là kênh comm chính.

CI/CD secrets:
- `KEYSTORE_BASE64`, `KEYSTORE_PASSWORD`, `KEY_ALIAS`, `KEY_PASSWORD` — signing.
- `DEPLOY_KEY` — SSH private key cho server.
- `SLACK_WEBHOOK` — channel `#streamix-tv-releases`.

---

## 4.8 Telemetry — đo adoption từ nginx log

Phương án A không có telemetry trong app. Adoption đo qua nginx access log:

```bash
# Số user tải mỗi version (1 IP = 1 user, gần đúng)
awk '/streamix-tv-.*\.apk/ {print $7}' /var/log/nginx/streamix-tv-access.log \
  | sort -u | sort | uniq -c | sort -rn

# Sample output:
#   42 /apk/streamix-tv-1.0.2-10202.apk     ← 42 unique IP tải bản 1.0.2
#   18 /apk/streamix-tv-1.0.1-10101.apk
#    3 /apk/streamix-tv-1.0.0-10000.apk     ← 3 user vẫn ở 1.0.0 → reach out
```

Dashboard đơn giản (Grafana hoặc cron + email weekly):
- Số download mỗi version trong 7/30/90 ngày qua.
- Tỷ lệ adopt latest sau 7/30 ngày.
- Top 5 user agents truy cập (biết loại TV phổ biến).

KPI:
- ≥ 70% user lên latest sau 30 ngày → tốt với Phương án A.
- < 50% sau 30 ngày → re-evaluate Phương án C ở V1.1.

---

## 4.9 Security checklist trước mỗi release

- [ ] APK ký bằng v2 + v3 (`apksigner verify` báo cả 2 = true)
- [ ] SHA-256 signature khớp với fingerprint sản phẩm gốc
- [ ] HTTPS portal + cert valid
- [ ] APK URL HTTPS
- [ ] `versionCode` tăng so với version trước (CI tự check)
- [ ] Release notes không lộ thông tin nhạy cảm
- [ ] ProGuard không strip class quan trọng (test bản release trước khi release)
- [ ] Permission trong manifest đã review — không có quyền thừa
- [ ] Đã test cài đè trên ≥ 1 thiết bị từ version trước
- [ ] DEPLOY_KEY còn valid; backup keystore còn truy cập được
- [ ] Slack message + email template đã chuẩn bị

So với rev 2: bỏ "Manifest URL HTTPS", "latest.sha256 đúng", "EXPECTED_SIGNATURE_SHA256 trong BuildConfig", "smoke-test rollback" — không còn áp dụng.

---

## 4.10 FAQ — lỗi thường gặp

**Q1. "App not installed" khi cài đè**
- Signature mới khác signature app cũ → user phải gỡ app cũ trước. Lý do thường gặp: ký bằng debug key thay release key.
- Fix: build lại với keystore release đúng; hoặc release thông báo user "vui lòng gỡ app cũ trước".

**Q2. `versionCode` thấp hơn version đang cài**
- Android chặn downgrade. Phải bump versionCode rồi build lại.

**Q3. Không thấy app trong File Manager → Downloads sau khi tải**
- Một số TV box (đặc biệt FPT Play Box) lưu Download ở thư mục không default. Hướng dẫn user dùng app "X-plore File Manager" hoặc "TV File Commander".

**Q4. APK tải về bị "damaged"**
- Network ngắt giữa chừng. Hướng dẫn user xoá file và tải lại.
- Hoặc CDN/server vấn đề — IT check nginx log và dung lượng disk server.

**Q5. User cài bằng QR scan từ phone, AirDrop sang TV không thành công**
- Tuỳ AirDrop/Send Files implementation. Workaround: tải về phone → kết nối USB OTG vào TV → mở File Manager TV.

**Q6. Cách verify APK chính chủ không bị tampering**
```bash
$ANDROID_HOME/build-tools/34.0.0/apksigner verify --print-certs streamix-tv-latest.apk
# So sánh SHA-256 với fingerprint chính chủ trong Confluence
```

**Q7. User đã cài thủ công xong, làm sao biết version đang chạy**
- Mở app → Cài đặt → Giới thiệu → version hiển thị.
- HOẶC qua ADB:
  ```bash
  adb shell dumpsys package vn.streamix.tv | grep versionName
  ```

---

## 4.11 Tóm tắt thay đổi từ revision 2 sang revision 3

| Mục | Rev 2 (in-app update) | Rev 3 (Phương án A — manual) |
|-----|----------------------|-------------------------------|
| Manifest JSON server-side | Có, schema phức tạp | **BỎ** |
| UpdateChecker/Downloader/Installer code | ~600 dòng Kotlin | **BỎ** |
| WorkManager periodic check | 6h/lần | **BỎ** |
| Rollback tự động | Có (crash 3 lần → revert) | **BỎ** |
| InstallTracker | DataStore + crash handler | **BỎ** |
| Permission `REQUEST_INSTALL_PACKAGES` | Có | **BỎ** |
| Permission `RECEIVE_BOOT_COMPLETED` | Có | **BỎ** |
| `EXPECTED_SIGNATURE_SHA256` trong BuildConfig | Có | **BỎ** |
| Build pipeline | Giữ | Giữ (rút gọn step manifest) |
| Signing v2+v3 | Giữ | Giữ |
| Hosting nginx | Có (manifest + apk) | Có (chỉ apk + html) |
| `install.html` | Edge case | **Flow chính** — mỗi update = first install |
| Communication protocol Slack/Email | Bonus | **Bắt buộc** — kênh comm chính |
| CI/CD Github Actions | ~80 dòng | ~60 dòng |
| Telemetry | App-side events qua endpoint | Server-side nginx log analysis |
| Effort triển khai | ~1.5 tuần | **~3 ngày** |
| Effort vận hành mỗi release | Auto | Manual: ~30 phút (Slack post + email) |

Tiết kiệm engineering: **~5 ngày**.

---

## 4.12 Kết luận

Phương án A đơn giản hoá phân phối tới mức tối đa. Đánh đổi: vận hành thủ công và rủi ro adoption thấp (R-S2).

Nguyên tắc cốt lõi cần giữ:
- **Versioning monotonic** — `versionCode` chỉ tăng.
- **Signing v2+v3** — cùng key cho mọi release để cài đè được.
- **Keystore bảo mật offline** — mất key = mất khả năng update.
- **HTTPS portal** — không serve APK qua HTTP.
- **Communication protocol chặt** — Slack channel + email blast bắt buộc cho mọi release.

Phần lớn rủi ro đã chấp nhận trong scope V1; nếu metric adoption sau 30 ngày kém (< 50%), V1.1 sẽ chuyển sang Phương án C (notification + manual install).

— Hết tài liệu APK distribution rev 3 —
