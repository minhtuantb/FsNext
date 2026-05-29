// app.jsx — Aurora-only exploration with light + dark mode toggle per artboard

const { useState } = React;

// Wrapper: each artboard can flip its own theme via a floating toggle
function ThemedArtboard({ defaultMode = 'light', children }) {
  const [mode, setMode] = useState(defaultMode);
  const t = mode === 'dark' ? T_A_DARK : T_A_LIGHT;
  return (
    <ThemeCtx.Provider value={t}>
      <div style={{ width: '100%', height: '100%', position: 'relative' }}>
        {children}
        {/* floating theme toggle */}
        <div style={{
          position: 'absolute', top: 10, right: 10, zIndex: 50,
          display: 'flex', gap: 2, padding: 3, background: 'rgba(0,0,0,.5)',
          backdropFilter: 'blur(8px)', borderRadius: 999,
          border: '1px solid rgba(255,255,255,.12)',
        }}>
          <ThemeBtn active={mode === 'light'} onClick={() => setMode('light')} label="☀" title="Light"/>
          <ThemeBtn active={mode === 'dark'} onClick={() => setMode('dark')} label="☾" title="Dark"/>
        </div>
      </div>
    </ThemeCtx.Provider>
  );
}

function ThemeBtn({ active, onClick, label, title }) {
  return (
    <button onClick={onClick} title={title} style={{
      width: 26, height: 26, borderRadius: '50%',
      background: active ? '#fff' : 'transparent',
      color: active ? '#0E0E12' : 'rgba(255,255,255,.7)',
      border: 'none', cursor: 'pointer', fontSize: 12, fontWeight: 600,
      display: 'flex', alignItems: 'center', justifyContent: 'center',
      transition: 'all .15s',
    }}>{label}</button>
  );
}

function App() {
  return (
    <DesignCanvas minScale={0.1} maxScale={2}>
      {/* ═════════ Section 1 · Intro ═════════ */}
      <DCSection id="intro" title="Fshare Desktop · Aurora" subtitle="Hướng thiết kế Aurora — light + dark mode, two file-manager layouts">
        <DCArtboard id="cover" label="Cover · Concept" width={1400} height={820}>
          <CoverCard/>
        </DCArtboard>
        <DCArtboard id="analysis" label="Analysis · Pains & Opportunities" width={1400} height={820}>
          <AnalysisCard/>
        </DCArtboard>
      </DCSection>

      {/* ═════════ Section 2 · Design System ═════════ */}
      <DCSection id="ds" title="Design System" subtitle="Type, color — hệ thống cho cả hai chế độ sáng / tối">
        <DCArtboard id="type" label="Typography" width={1400} height={820}>
          <TypeCard/>
        </DCArtboard>
        <DCArtboard id="colors" label="Colors · Aurora palette" width={1400} height={820}>
          <ColorCard/>
        </DCArtboard>
      </DCSection>

      {/* ═════════ Section 2.5 · Foundations + Components ═════════ */}
      <DCSection id="foundations" title="Foundations" subtitle="Spacing, bo góc, đổ bóng, motion, z-index, grid — tokens dùng chung">
        <DCArtboard id="f-tokens" label="Foundations · Tokens" width={1400} height={1100}>
          <FoundationsCard/>
        </DCArtboard>
      </DCSection>

      <DCSection id="components" title="Component Library" subtitle="Mọi atom và molecule — button, input, badge, modal, tabs…">
        <DCArtboard id="c-inputs" label="02 · Form controls" width={1400} height={1600}>
          <ComponentsCard1/>
        </DCArtboard>
        <DCArtboard id="c-feedback" label="03 · Feedback & state" width={1400} height={1600}>
          <ComponentsCard2/>
        </DCArtboard>
        <DCArtboard id="c-overlays" label="04 · Overlay & nav" width={1400} height={1500}>
          <ComponentsCard3/>
        </DCArtboard>
        <DCArtboard id="c-icons" label="05 · Icon library" width={1400} height={1400}>
          <IconLibraryCard/>
        </DCArtboard>
        <DCArtboard id="c-table" label="06 · Table / Data grid" width={1400} height={1600}>
          <TableCard/>
        </DCArtboard>
        <DCArtboard id="c-guidelines" label="07 · Guidelines, voice, tokens export, a11y" width={1400} height={2400}>
          <GuidelinesCard/>
        </DCArtboard>
      </DCSection>

      {/* ═════════ Section 3 · Core screens (light default) ═════════ */}
      <DCSection id="aurora-light" title="Core screens · Light" subtitle="Bật công tắc ☾ trên mỗi artboard để xem Dark mode ngay tại chỗ">
        <DCArtboard id="a-home" label="Home / Browse" width={1280} height={860}>
          <ThemedArtboard defaultMode="light"><AuroraHome/></ThemedArtboard>
        </DCArtboard>
        <DCArtboard id="a-dl" label="Download Manager" width={1280} height={860}>
          <ThemedArtboard defaultMode="light"><AuroraDownloads/></ThemedArtboard>
        </DCArtboard>
        <DCArtboard id="a-share" label="Share Link" width={1280} height={860}>
          <ThemedArtboard defaultMode="light"><AuroraShare/></ThemedArtboard>
        </DCArtboard>
      </DCSection>

      {/* ═════════ Section 4 · Core screens (dark default) ═════════ */}
      <DCSection id="aurora-dark" title="Core screens · Dark" subtitle="Cùng ba màn hình với dark mode — cho ban đêm, cho power users">
        <DCArtboard id="a-home-d" label="Home / Browse · Dark" width={1280} height={860}>
          <ThemedArtboard defaultMode="dark"><AuroraHome/></ThemedArtboard>
        </DCArtboard>
        <DCArtboard id="a-dl-d" label="Download Manager · Dark" width={1280} height={860}>
          <ThemedArtboard defaultMode="dark"><AuroraDownloads/></ThemedArtboard>
        </DCArtboard>
        <DCArtboard id="a-share-d" label="Share Link · Dark" width={1280} height={860}>
          <ThemedArtboard defaultMode="dark"><AuroraShare/></ThemedArtboard>
        </DCArtboard>
      </DCSection>

      {/* ═════════ Section 4.5 · Auth ═════════ */}
      <DCSection id="auth" title="Đăng nhập" subtitle="Split hero — bên trái là brand panel với Aurora gradient, bên phải là form email + 3 social (Google, Facebook, Zalo)">
        <DCArtboard id="a-login" label="Login · Light" width={1280} height={860}>
          <ThemedArtboard defaultMode="light"><AuroraLogin/></ThemedArtboard>
        </DCArtboard>
        <DCArtboard id="a-login-d" label="Login · Dark" width={1280} height={860}>
          <ThemedArtboard defaultMode="dark"><AuroraLogin/></ThemedArtboard>
        </DCArtboard>
      </DCSection>

      {/* ═════════ Section 4.6 · Upload ═════════ */}
      <DCSection id="upload" title="Tải lên" subtitle="Drag-drop zone lớn · destination picker · hàng chờ với per-file progress, pause/resume, tốc độ & ETA thời gian thực">
        <DCArtboard id="a-upload" label="Upload · Light" width={1280} height={900}>
          <ThemedArtboard defaultMode="light"><AuroraUpload/></ThemedArtboard>
        </DCArtboard>
        <DCArtboard id="a-upload-d" label="Upload · Dark" width={1280} height={900}>
          <ThemedArtboard defaultMode="dark"><AuroraUpload/></ThemedArtboard>
        </DCArtboard>
        <DCArtboard id="a-upload-modal" label="Upload · Modal + Folder tree picker" width={1280} height={900}>
          <ThemedArtboard defaultMode="light"><AuroraUploadModal/></ThemedArtboard>
        </DCArtboard>
        <DCArtboard id="a-upload-modal-d" label="Upload · Modal · Dark" width={1280} height={900}>
          <ThemedArtboard defaultMode="dark"><AuroraUploadModal/></ThemedArtboard>
        </DCArtboard>
      </DCSection>

      {/* ═════════ Section 4.7 · Account ═════════ */}
      <DCSection id="account" title="Tài khoản" subtitle="VIP plan card với breakdown dung lượng · profile · bảo mật (2FA, sessions) · lịch sử thanh toán">
        <DCArtboard id="a-account" label="Account · Light" width={1280} height={1200}>
          <ThemedArtboard defaultMode="light"><AuroraAccount/></ThemedArtboard>
        </DCArtboard>
        <DCArtboard id="a-account-d" label="Account · Dark" width={1280} height={1200}>
          <ThemedArtboard defaultMode="dark"><AuroraAccount/></ThemedArtboard>
        </DCArtboard>
      </DCSection>

      {/* ═════════ Section 4.8 · Favorites ═════════ */}
      <DCSection id="favs" title="Yêu thích" subtitle="Bộ sưu tập cá nhân — collections ribbon, grid thumbnails với gradient placeholder, tag loại file và trạng thái chia sẻ">
        <DCArtboard id="a-favs" label="Favorites · Light" width={1280} height={900}>
          <ThemedArtboard defaultMode="light"><AuroraFavorites/></ThemedArtboard>
        </DCArtboard>
        <DCArtboard id="a-favs-d" label="Favorites · Dark" width={1280} height={900}>
          <ThemedArtboard defaultMode="dark"><AuroraFavorites/></ThemedArtboard>
        </DCArtboard>
      </DCSection>

      {/* ═════════ Section 4.9 · Sync ═════════ */}
      <DCSection id="sync" title="Đồng bộ" subtitle="Cặp thư mục máy ↔ cloud · xung đột phiên bản với side-by-side compare · selective sync">
        <DCArtboard id="a-sync" label="Sync · Light" width={1280} height={1280}>
          <ThemedArtboard defaultMode="light"><AuroraSync/></ThemedArtboard>
        </DCArtboard>
        <DCArtboard id="a-sync-d" label="Sync · Dark" width={1280} height={1280}>
          <ThemedArtboard defaultMode="dark"><AuroraSync/></ThemedArtboard>
        </DCArtboard>
        <DCArtboard id="a-sync-auto" label="Sync Auto · Overview (max 5 thư mục)" width={1280} height={1100}>
          <ThemedArtboard defaultMode="light"><AuroraSyncAuto/></ThemedArtboard>
        </DCArtboard>
        <DCArtboard id="a-sync-auto-d" label="Sync Auto · Overview · Dark" width={1280} height={1100}>
          <ThemedArtboard defaultMode="dark"><AuroraSyncAuto/></ThemedArtboard>
        </DCArtboard>
        <DCArtboard id="a-sync-auto-detail" label="Sync Auto · Detail (HoaSen)" width={1280} height={1150}>
          <ThemedArtboard defaultMode="light"><AuroraSyncAutoDetail/></ThemedArtboard>
        </DCArtboard>
        <DCArtboard id="a-sync-auto-detail-d" label="Sync Auto · Detail · Dark" width={1280} height={1150}>
          <ThemedArtboard defaultMode="dark"><AuroraSyncAutoDetail/></ThemedArtboard>
        </DCArtboard>
      </DCSection>

      {/* ═════════ Section 5 · File Manager · Option A (3-panel) ═════════ */}
      <DCSection id="files-a" title="File Manager · A — 3-panel" subtitle="Tree trái · Grid giữa · Detail phải. Kiểu hiện đại, giống OneDrive / Google Drive desktop.">
        <DCArtboard id="a-files-light" label="Option A · Light" width={1440} height={900}>
          <ThemedArtboard defaultMode="light"><AuroraFiles/></ThemedArtboard>
        </DCArtboard>
        <DCArtboard id="a-files-dark" label="Option A · Dark" width={1440} height={900}>
          <ThemedArtboard defaultMode="dark"><AuroraFiles/></ThemedArtboard>
        </DCArtboard>
      </DCSection>

      {/* ═════════ Section 6 · File Manager · Option B (column view) ═════════ */}
      <DCSection id="files-b" title="File Manager · B — Column view" subtitle="Miller columns: mỗi cấp một cột, cuộn ngang để đi sâu. Nhanh để khám phá cây thư mục nhiều cấp.">
        <DCArtboard id="b-files-light" label="Option B · Light" width={1440} height={900}>
          <ThemedArtboard defaultMode="light"><AuroraFilesV2/></ThemedArtboard>
        </DCArtboard>
        <DCArtboard id="b-files-dark" label="Option B · Dark" width={1440} height={900}>
          <ThemedArtboard defaultMode="dark"><AuroraFilesV2/></ThemedArtboard>
        </DCArtboard>
      </DCSection>

      <DCPostIt x={60} y={60} color="#fef4a8">
        Aurora · Light + Dark. Click ☀ / ☾ trên mỗi artboard để đổi theme tại chỗ.
        Hai phương án File Manager ở cuối — chọn phương án bạn thích.
      </DCPostIt>
    </DesignCanvas>
  );
}

ReactDOM.createRoot(document.getElementById('root')).render(<App/>);
