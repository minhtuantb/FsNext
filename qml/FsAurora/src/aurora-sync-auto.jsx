// aurora-sync-auto.jsx
// Hai màn đồng bộ tự động (theo phác thảo FsNext):
//   · AuroraSyncAuto        — Overview grid: max 5 thư mục, mỗi card là 1 pair local↔cloud
//   · AuroraSyncAutoDetail  — Chi tiết 1 thư mục: chip tabs + toolbar + list file với trạng thái

// ═══════════════════════════════════════════════════════════════════════════
// OVERVIEW
// ═══════════════════════════════════════════════════════════════════════════
function AuroraSyncAuto() {
  const t = useTA();
  const [collapsed, setCollapsed] = React.useState(false);

  // Tối đa 5 pair — card cuối cùng là "Thêm thư mục" nếu chưa đủ
  const pairs = [
    { id: 'hoasen', name: 'HoaSen', local: 'D:/Projects/HoaSen', cloud: '/HoaSen',
      files: 12, done: 8, size: '284 MB', speed: '5 MB/s',
      status: 'syncing', lastSync: 'vừa xong', delLocal: true, enabled: true },
    { id: 'qml', name: 'qml', local: 'D:/Dev/qml', cloud: '/Fshare_Data/qml',
      files: 1293, done: 1293, size: '8.4 GB', speed: null,
      status: 'synced', lastSync: '2 phút trước', delLocal: false, enabled: true },
    { id: 'platforms', name: 'platforms', local: 'D:/Build/platforms', cloud: '/Build/platforms',
      files: 1, done: 1, size: '142 MB', speed: null,
      status: 'synced', lastSync: '14 phút trước', delLocal: false, enabled: true },
    { id: 'photos', name: 'RAW_Photos', local: 'E:/Canon_R5/2026', cloud: '/RAW_Photos/2026',
      files: 340, done: 312, size: '28.4 GB', speed: '12.8 MB/s',
      status: 'syncing', lastSync: 'vừa xong', delLocal: false, enabled: true },
    { id: 'backup', name: 'Docs_Backup', local: 'D:/Users/minh/Documents', cloud: '/Backup_2026/Docs',
      files: 842, done: 842, size: '1.8 GB', speed: null,
      status: 'paused', lastSync: 'hôm qua 18:20', delLocal: false, enabled: false },
  ];
  const canAdd = pairs.length < 5;

  return (
    <WinChrome title="Fshare · Đồng bộ" theme={t}>
      <AuroraSidebar active="sync" collapsed={collapsed} onToggle={() => setCollapsed(v => !v)}/>
      <div style={{ flex: 1, background: t.bg, overflow: 'auto' }}>

        {/* HEADER */}
        <div style={{ padding: '26px 32px 20px', background: t.panel, borderBottom: `1px solid ${t.border}` }}>
          <div style={{ display: 'flex', alignItems: 'baseline', gap: 12 }}>
            <h1 style={{ margin: 0, fontFamily: t.serif, fontSize: 36, fontWeight: 400, letterSpacing: '-.02em' }}>
              Đồng bộ
            </h1>
            <span style={{ fontSize: 12, fontFamily: t.mono, color: t.ink4 }}>
              {pairs.length}/5 thư mục · tối đa 5
            </span>
          </div>
          <div style={{ fontSize: 13, color: t.ink3, marginTop: 4 }}>
            Tự động sao lưu tối đa 5 thư mục quan trọng lên Fshare — file mới hoặc file sửa đổi sẽ được tải lên âm thầm.
          </div>
        </div>

        {/* EXPLAINER BANNER */}
        <div style={{ padding: '20px 32px 4px' }}>
          <div style={{
            display: 'flex', gap: 14, alignItems: 'flex-start',
            padding: '16px 20px', background: t.gradSoft,
            border: `1px solid ${t.accent}22`, borderRadius: 14,
          }}>
            <div style={{
              width: 36, height: 36, borderRadius: 10, background: t.panel,
              display: 'flex', alignItems: 'center', justifyContent: 'center',
              color: t.accent, flexShrink: 0, border: `1px solid ${t.border}`,
            }}>
              <Icons.sync size={18}/>
            </div>
            <div style={{ flex: 1 }}>
              <div style={{ fontSize: 13.5, fontWeight: 700, color: t.ink }}>Đồng bộ là gì?</div>
              <div style={{ fontSize: 12.5, color: t.ink2, marginTop: 4, lineHeight: 1.55 }}>
                Đồng bộ tự động tải các file trong thư mục bạn chọn lên Fshare, đặt ở chế độ riêng tư và giới hạn tốc độ 5 MB/s để không ảnh hưởng tới mạng. Khi phát hiện file mới hoặc file đã thay đổi, FsNext sẽ âm thầm sao lưu. Bạn có thể chọn tự động xoá bản local sau khi tải lên thành công.
              </div>
            </div>
            <button style={{ padding: '6px 12px', fontSize: 11.5, fontWeight: 600, background: 'transparent', border: `1px solid ${t.accent}44`, borderRadius: 999, color: t.accent, cursor: 'pointer', fontFamily: t.font, flexShrink: 0 }}>
              Tìm hiểu thêm
            </button>
          </div>
        </div>

        {/* GLOBAL STATUS STRIP */}
        <div style={{ padding: '16px 32px 8px', display: 'flex', alignItems: 'center', gap: 20, flexWrap: 'wrap' }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
            <div style={{ width: 8, height: 8, borderRadius: '50%', background: t.success, boxShadow: `0 0 0 3px ${t.success}22` }}/>
            <span style={{ fontSize: 12.5, fontWeight: 600 }}>Dịch vụ đồng bộ đang chạy</span>
          </div>
          <MiniStat label="Đang đồng bộ" value="2" mono={t.mono}/>
          <MiniStat label="Đã hoàn tất" value="2" mono={t.mono}/>
          <MiniStat label="Tạm dừng" value="1" mono={t.mono}/>
          <MiniStat label="Tổng file" value="2.488" mono={t.mono}/>
          <MiniStat label="Tốc độ hiện tại" value="17.8 MB/s" mono={t.mono}/>
          <div style={{ marginLeft: 'auto', display: 'flex', gap: 8 }}>
            <button style={saBtnGhostSm(t)}><Icons.pause size={12}/> Tạm dừng tất cả</button>
            <button style={saBtnGhostSm(t)}><Icons.settings size={12}/> Cài đặt</button>
          </div>
        </div>

        {/* GRID CARDS */}
        <div style={{ padding: '18px 32px 32px', display: 'grid', gridTemplateColumns: 'repeat(2, 1fr)', gap: 16 }}>
          {pairs.map(p => <SAPairCard key={p.id} p={p} t={t}/>)}
          {canAdd && <SAAddPairCard t={t}/>}
        </div>
      </div>
    </WinChrome>
  );
}

function saBtnGhostSm(t) {
  return {
    padding: '6px 12px', background: 'transparent',
    border: `1px solid ${t.border}`, borderRadius: 8, color: t.ink2,
    fontSize: 11.5, fontWeight: 600, cursor: 'pointer', fontFamily: t.font,
    display: 'flex', alignItems: 'center', gap: 6,
  };
}

function SAPairCard({ p, t }) {
  const statusBadge = {
    syncing: { bg: t.gradSoft, color: t.accent, label: 'Đang đồng bộ', dot: t.accent },
    synced:  { bg: 'rgba(10,138,92,.1)', color: t.success, label: 'Đã đồng bộ', dot: t.success },
    paused:  { bg: t.bg, color: t.ink3, label: 'Tạm dừng', dot: t.ink4 },
  }[p.status];
  const pct = Math.round((p.done / p.files) * 100);

  return (
    <div style={{
      background: t.panel, border: `1px solid ${t.border}`, borderRadius: 16,
      padding: '18px 20px', opacity: p.enabled ? 1 : .65,
      position: 'relative', overflow: 'hidden',
    }}>
      {/* top row: name + status + toggle */}
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', gap: 12 }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: 10, minWidth: 0 }}>
          <div style={{
            width: 36, height: 36, borderRadius: 10, background: t.gradSoft,
            display: 'flex', alignItems: 'center', justifyContent: 'center', color: t.accent,
            flexShrink: 0,
          }}><Icons.folder size={17}/></div>
          <div style={{ minWidth: 0 }}>
            <div style={{ fontSize: 15, fontWeight: 700, letterSpacing: '-.01em', whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>{p.name}</div>
            <div style={{ fontSize: 10.5, fontFamily: t.mono, color: t.ink4, marginTop: 1 }}>{p.done.toLocaleString()} / {p.files.toLocaleString()} file · {p.size}</div>
          </div>
        </div>
        <div style={{ display: 'flex', alignItems: 'center', gap: 10, flexShrink: 0 }}>
          <span style={{
            display: 'inline-flex', alignItems: 'center', gap: 5,
            padding: '3px 9px', background: statusBadge.bg, color: statusBadge.color,
            fontSize: 10.5, fontWeight: 700, borderRadius: 999,
            textTransform: 'uppercase', letterSpacing: '.06em', fontFamily: t.mono,
          }}>
            <span style={{ width: 6, height: 6, borderRadius: '50%', background: statusBadge.dot }}/>
            {statusBadge.label}
          </span>
          <SASwitchMini on={p.enabled} t={t}/>
        </div>
      </div>

      {/* pair path */}
      <div style={{ marginTop: 14, padding: '10px 12px', background: t.bg, borderRadius: 10, border: `1px dashed ${t.border}`, display: 'flex', alignItems: 'center', gap: 8, fontSize: 11.5, fontFamily: t.mono, color: t.ink2 }}>
        <Icons.folder size={11} stroke={t.ink3}/>
        <span style={{ whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis', maxWidth: 180 }}>{p.local}</span>
        <Icons.arrowUp size={11} stroke={t.accent} style={{ transform: 'rotate(90deg)', flexShrink: 0 }}/>
        <Icons.cloud size={11} stroke={t.accent}/>
        <span style={{ color: t.accent, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis', fontWeight: 600 }}>{p.cloud}</span>
      </div>

      {/* progress */}
      {p.status === 'syncing' ? (
        <div style={{ marginTop: 12 }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: 11, fontFamily: t.mono, color: t.ink3, marginBottom: 5 }}>
            <span>{pct}% · {p.speed}</span>
            <span>còn {p.files - p.done} file</span>
          </div>
          <div style={{ height: 5, background: t.border, borderRadius: 4, overflow: 'hidden' }}>
            <div style={{ width: `${pct}%`, height: '100%', background: t.grad, borderRadius: 4 }}/>
          </div>
        </div>
      ) : (
        <div style={{ marginTop: 12, display: 'flex', alignItems: 'center', gap: 8, fontSize: 11.5, color: t.ink3 }}>
          {p.status === 'synced' ? <Icons.check size={12} stroke={t.success}/> : <Icons.pause size={11}/>}
          {p.status === 'synced' ? `Đã đồng bộ · lần cuối ${p.lastSync}` : `Tạm dừng từ ${p.lastSync}`}
        </div>
      )}

      {/* bottom actions */}
      <div style={{ marginTop: 14, paddingTop: 12, borderTop: `1px solid ${t.border}`, display: 'flex', alignItems: 'center', gap: 14, fontSize: 11.5 }}>
        <span style={{ display: 'flex', alignItems: 'center', gap: 6, color: p.delLocal ? t.warn : t.ink4 }}>
          <Icons.trash size={11}/> {p.delLocal ? 'Xoá local sau khi tải' : 'Giữ local'}
        </span>
        <div style={{ marginLeft: 'auto', display: 'flex', gap: 4 }}>
          <SAActionIcon t={t} title="Mở thư mục local"><Icons.folder size={13}/></SAActionIcon>
          <SAActionIcon t={t} title="Mở trên cloud"><Icons.cloud size={13}/></SAActionIcon>
          <SAActionIcon t={t} title="Xem chi tiết"><Icons.chevRight size={13}/></SAActionIcon>
        </div>
      </div>
    </div>
  );
}

function SAActionIcon({ t, title, children }) {
  return (
    <button title={title} style={{
      width: 28, height: 28, borderRadius: 8, border: 'none',
      background: 'transparent', color: t.ink3, cursor: 'pointer',
      display: 'flex', alignItems: 'center', justifyContent: 'center',
    }}>{children}</button>
  );
}

function SASwitchMini({ on, t }) {
  return (
    <div style={{
      width: 30, height: 18, padding: 2,
      background: on ? t.grad : t.border,
      borderRadius: 999, display: 'flex',
      justifyContent: on ? 'flex-end' : 'flex-start',
      transition: 'background .2s',
    }}>
      <div style={{ width: 14, height: 14, background: '#fff', borderRadius: '50%', boxShadow: '0 1px 2px rgba(0,0,0,.2)' }}/>
    </div>
  );
}

function SAAddPairCard({ t }) {
  return (
    <button style={{
      background: 'transparent', border: `2px dashed ${t.border}`, borderRadius: 16,
      padding: '28px 20px', display: 'flex', flexDirection: 'column', alignItems: 'center',
      justifyContent: 'center', gap: 10, cursor: 'pointer', color: t.ink3, fontFamily: t.font,
      minHeight: 180,
    }}>
      <div style={{
        width: 44, height: 44, borderRadius: 12, background: t.gradSoft,
        display: 'flex', alignItems: 'center', justifyContent: 'center', color: t.accent,
      }}><Icons.plus size={22}/></div>
      <div style={{ fontSize: 14, fontWeight: 700, color: t.ink }}>Thêm thư mục đồng bộ</div>
      <div style={{ fontSize: 11.5, color: t.ink4, textAlign: 'center', maxWidth: 280 }}>
        Chọn thư mục trên máy để tự động sao lưu · còn trống {5 - 5 + 1} / 5 slot
      </div>
    </button>
  );
}


// ═══════════════════════════════════════════════════════════════════════════
// DETAIL — theo phác thảo
// ═══════════════════════════════════════════════════════════════════════════
function AuroraSyncAutoDetail() {
  const t = useTA();
  const [collapsed, setCollapsed] = React.useState(false);
  const [active, setActive] = React.useState('HoaSen');
  const [syncOn, setSyncOn] = React.useState(true);
  const [delLocal, setDelLocal] = React.useState(true);

  const chips = [
    { id: 'HoaSen', name: 'HoaSen', done: 8, total: 12, status: 'syncing' },
    { id: 'qml', name: 'qml', done: 1293, total: 1293, status: 'synced' },
    { id: 'platforms', name: 'platforms', done: 1, total: 1, status: 'synced' },
  ];

  const files = [
    { name: 'POC_GlamSatPTVMSmart_HoaSen.xlsx', size: '38.6 KB', mtime: '4/17/2026 4:04 PM', backup: '4/21/2026 3:55 PM', state: 'synced', type: 'doc' },
    { name: 'Qt6Core.dll', size: '5.87 MB', mtime: '3/17/2025 9:18 PM', backup: '4/22/2026 5:46 PM', state: 'synced', type: 'file' },
    { name: 'Qt6Gui.dll', size: '8.86 MB', mtime: '3/17/2025 9:18 PM', backup: '4/22/2026 5:46 PM', state: 'synced', type: 'file' },
    { name: 'Qt6LabsFolderListModel.dll', size: '108 KB', mtime: '3/18/2025 10:36 PM', backup: '4/22/2026 5:46 PM', state: 'synced', type: 'file' },
    { name: 'Qt6LabsSettings.dll', size: '59.1 KB', mtime: '3/18/2025 10:36 PM', backup: '4/22/2026 5:46 PM', state: 'synced', type: 'file' },
    { name: 'Qt6QuickEffects.dll', size: '380 KB', mtime: '3/18/2025 10:39 PM', backup: '4/22/2026 5:47 PM', state: 'removed', type: 'file' },
    { name: 'Qt6QuickLayouts.dll', size: '201 KB', mtime: '3/18/2025 10:39 PM', backup: '4/22/2026 5:47 PM', state: 'removed', type: 'file' },
    { name: 'Qt6Widgets.dll', size: '6.14 MB', mtime: '3/18/2025 10:41 PM', backup: '—', state: 'uploading', pct: 64, type: 'file' },
    { name: 'd3dcompiler_47.dll', size: '4.10 MB', mtime: '3/18/2025 10:42 PM', backup: '—', state: 'queued', type: 'file' },
  ];

  return (
    <WinChrome title="Fshare · Đồng bộ · HoaSen" theme={t}>
      <AuroraSidebar active="sync" collapsed={collapsed} onToggle={() => setCollapsed(v => !v)}/>
      <div style={{ flex: 1, background: t.bg, overflow: 'auto' }}>

        {/* HEADER */}
        <div style={{ padding: '26px 32px 18px', background: t.panel, borderBottom: `1px solid ${t.border}` }}>
          <div style={{ fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', marginBottom: 6 }}>
            ━━ Đồng bộ
          </div>
          <h1 style={{ margin: 0, fontFamily: t.serif, fontSize: 36, fontWeight: 400, letterSpacing: '-.02em' }}>
            Đồng bộ
          </h1>
          <div style={{ fontSize: 13, color: t.ink3, marginTop: 4 }}>
            Tự động sao lưu tối đa 5 thư mục quan trọng lên Fshare
          </div>
        </div>

        {/* EXPLAINER BANNER */}
        <div style={{ padding: '18px 32px 4px' }}>
          <div style={{
            display: 'flex', gap: 14, alignItems: 'flex-start',
            padding: '14px 18px', background: t.gradSoft,
            border: `1px solid ${t.accent}22`, borderRadius: 14,
          }}>
            <div style={{ width: 30, height: 30, borderRadius: 9, background: t.panel, display: 'flex', alignItems: 'center', justifyContent: 'center', color: t.accent, flexShrink: 0, border: `1px solid ${t.border}` }}>
              <Icons.sync size={15}/>
            </div>
            <div style={{ flex: 1 }}>
              <div style={{ fontSize: 13, fontWeight: 700 }}>Đồng bộ là gì?</div>
              <div style={{ fontSize: 11.5, color: t.ink2, marginTop: 3, lineHeight: 1.5 }}>
                Đồng bộ tự động tải các file trong thư mục bạn chọn lên Fshare, đặt ở chế độ riêng tư và giới hạn tốc độ 5 MB/s để không ảnh hưởng tới mạng. Khi phát hiện file mới hoặc file đã thay đổi, FsNext sẽ âm thầm sao lưu. Bạn có thể chọn tự động xoá bản local sau khi tải lên thành công.
              </div>
            </div>
          </div>
        </div>

        {/* CHIPS — folder tabs */}
        <div style={{ padding: '16px 32px 12px', display: 'flex', alignItems: 'center', gap: 10, flexWrap: 'wrap' }}>
          {chips.map(c => (
            <SAFolderChip key={c.id} t={t} active={active === c.id} onClick={() => setActive(c.id)} chip={c}/>
          ))}
          <button style={{
            width: 40, height: 40, borderRadius: 999, border: 'none',
            background: t.grad, color: '#fff', cursor: 'pointer',
            display: 'flex', alignItems: 'center', justifyContent: 'center',
            marginLeft: 'auto', boxShadow: t.shadow,
          }}><Icons.plus size={16}/></button>
        </div>

        {/* TOOLBAR */}
        <div style={{
          margin: '0 32px', padding: '10px 14px',
          background: t.panel, border: `1px solid ${t.border}`,
          borderRadius: 12, display: 'flex', alignItems: 'center', gap: 8,
        }}>
          <SAToolBtn t={t} title="Đồng bộ ngay"><Icons.sync size={13}/></SAToolBtn>
          <SAToolBtn t={t} title="Mở thư mục"><Icons.folder size={13}/></SAToolBtn>
          <SAToolBtn t={t} title="Lịch sử"><Icons.clock size={13}/></SAToolBtn>
          <SAToolBtn t={t} title="Gỡ thư mục"><Icons.trash size={13}/></SAToolBtn>

          <div style={{ width: 1, height: 20, background: t.border, margin: '0 6px' }}/>

          <label style={{ display: 'flex', alignItems: 'center', gap: 8, fontSize: 12.5, fontWeight: 500, cursor: 'pointer' }}>
            <Icons.zap size={12} stroke={syncOn ? t.accent : t.ink4}/>
            Bật đồng bộ
            <SASwitchMd on={syncOn} onClick={() => setSyncOn(v => !v)} t={t}/>
          </label>

          <label style={{ display: 'flex', alignItems: 'center', gap: 8, fontSize: 12.5, fontWeight: 500, cursor: 'pointer', marginLeft: 16 }}>
            <Icons.trash size={12} stroke={delLocal ? t.accent : t.ink4}/>
            Xoá local sau khi tải
            <SASwitchMd on={delLocal} onClick={() => setDelLocal(v => !v)} t={t}/>
          </label>

          <div style={{ marginLeft: 'auto', display: 'flex', alignItems: 'center', gap: 10, fontSize: 11, fontFamily: t.mono, color: t.ink4 }}>
            <span>Giới hạn tốc độ</span>
            <span style={{ padding: '3px 8px', background: t.bg, borderRadius: 6, border: `1px solid ${t.border}`, color: t.ink2 }}>5 MB/s</span>
          </div>
        </div>

        {/* PAIR HEADER */}
        <div style={{ margin: '16px 32px 0', padding: '14px 18px', background: t.panel, border: `1px solid ${t.border}`, borderRadius: 12, display: 'flex', alignItems: 'center', gap: 14 }}>
          <div style={{ width: 40, height: 40, borderRadius: 10, background: t.gradSoft, display: 'flex', alignItems: 'center', justifyContent: 'center', color: t.accent, flexShrink: 0 }}>
            <Icons.folder size={18}/>
          </div>
          <div style={{ flex: 1, minWidth: 0 }}>
            <div style={{ fontSize: 15, fontWeight: 700, fontFamily: t.mono }}>D:/Projects/HoaSen</div>
            <div style={{ fontSize: 11.5, color: t.ink3, marginTop: 2, display: 'flex', alignItems: 'center', gap: 10, fontFamily: t.mono }}>
              <span style={{ display: 'flex', alignItems: 'center', gap: 4 }}>
                <Icons.arrowUp size={10} style={{ transform: 'rotate(90deg)' }} stroke={t.accent}/> /HoaSen
              </span>
              <span>·</span>
              <span style={{ color: t.accent, fontWeight: 600 }}>5 MB/s</span>
              <span>·</span>
              <span>12 file</span>
              <span>·</span>
              <span>284 MB</span>
            </div>
          </div>
          <span style={{ padding: '4px 10px', fontSize: 10.5, fontWeight: 700, background: t.gradSoft, color: t.accent, borderRadius: 999, textTransform: 'uppercase', letterSpacing: '.08em', fontFamily: t.mono }}>
            Đang đồng bộ · 8/12
          </span>
          <span style={{ fontSize: 11, color: t.ink4, fontFamily: t.mono }}>Trạng thái</span>
        </div>

        {/* FILE LIST */}
        <div style={{ margin: '0 32px 32px', background: t.panel, border: `1px solid ${t.border}`, borderTop: 'none', borderRadius: '0 0 14px 14px' }}>
          {files.map((f, i) => <SAFileRow key={i} f={f} t={t}/>)}
        </div>
      </div>
    </WinChrome>
  );
}

function SAFolderChip({ t, active, chip, onClick }) {
  return (
    <button onClick={onClick} style={{
      display: 'flex', alignItems: 'center', gap: 8,
      padding: '8px 14px', borderRadius: 10,
      background: active ? t.grad : t.panel,
      color: active ? '#fff' : t.ink,
      border: active ? 'none' : `1px solid ${t.border}`,
      fontSize: 13, fontWeight: 700, cursor: 'pointer', fontFamily: t.font,
      boxShadow: active ? t.shadow : 'none',
    }}>
      <Icons.folder size={13} fill={active ? '#fff' : 'none'} stroke={active ? '#fff' : t.ink2}/>
      {chip.name}
      <span style={{
        padding: '2px 8px', borderRadius: 999,
        background: active ? 'rgba(255,255,255,.22)' : t.bg,
        color: active ? '#fff' : t.ink3,
        fontSize: 10.5, fontFamily: t.mono, fontWeight: 600,
      }}>
        {chip.done === chip.total ? `${chip.total}` : `${chip.done}/${chip.total}`}
      </span>
    </button>
  );
}

function SAToolBtn({ t, title, children }) {
  return (
    <button title={title} style={{
      width: 30, height: 30, borderRadius: 8, border: 'none',
      background: 'transparent', color: t.ink2, cursor: 'pointer',
      display: 'flex', alignItems: 'center', justifyContent: 'center',
    }}>{children}</button>
  );
}

function SASwitchMd({ on, onClick, t }) {
  return (
    <button onClick={onClick} style={{
      width: 34, height: 20, padding: 2, border: 'none', cursor: 'pointer',
      background: on ? t.grad : t.border, borderRadius: 999, display: 'flex',
      justifyContent: on ? 'flex-end' : 'flex-start', transition: 'background .2s',
    }}>
      <div style={{ width: 16, height: 16, background: '#fff', borderRadius: '50%', boxShadow: '0 1px 2px rgba(0,0,0,.2)' }}/>
    </button>
  );
}

function SAFileRow({ f, t }) {
  const stateInfo = {
    synced:    { label: 'Đã đồng bộ', color: t.success, bg: 'rgba(10,138,92,.08)', icon: <Icons.check size={13} stroke={t.success}/> },
    removed:   { label: 'Đã xoá local', color: t.warn, bg: 'rgba(201,106,0,.08)', icon: <Icons.folder size={13} stroke={t.warn}/> },
    uploading: { label: `Đang tải · ${f.pct}%`, color: t.accent, bg: t.gradSoft, icon: <Icons.arrowUp size={13} stroke={t.accent}/> },
    queued:    { label: 'Chờ đến lượt', color: t.ink3, bg: t.bg, icon: <Icons.clock size={13} stroke={t.ink3}/> },
  }[f.state];

  return (
    <div style={{
      display: 'flex', alignItems: 'center', gap: 14, padding: '12px 18px',
      borderBottom: `1px solid ${t.border}`,
    }}>
      <div style={{ width: 22, height: 22, borderRadius: 6, background: t.bg, display: 'flex', alignItems: 'center', justifyContent: 'center', color: t.ink3, flexShrink: 0 }}>
        {f.state === 'synced' ? <Icons.check size={12} stroke={t.success}/> : (f.state === 'removed' ? <Icons.folder size={12} stroke={t.ink4}/> : stateInfo.icon)}
      </div>

      <div style={{ flex: 1, minWidth: 0 }}>
        <div style={{ fontSize: 13, fontWeight: 600, color: t.ink, fontFamily: t.mono, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>{f.name}</div>
        <div style={{ fontSize: 10.5, color: t.ink4, fontFamily: t.mono, marginTop: 2 }}>
          {f.size} · {f.mtime}
          {f.state === 'synced' && <> · đã sao lưu {f.backup}</>}
          {f.state === 'removed' && <> · đã sao lưu {f.backup}</>}
        </div>
        {f.state === 'uploading' && (
          <div style={{ marginTop: 6, height: 3, background: t.border, borderRadius: 2, overflow: 'hidden' }}>
            <div style={{ width: `${f.pct}%`, height: '100%', background: t.grad, borderRadius: 2 }}/>
          </div>
        )}
      </div>

      <span style={{
        display: 'inline-flex', alignItems: 'center', gap: 5,
        padding: '4px 10px', background: stateInfo.bg, color: stateInfo.color,
        fontSize: 11, fontWeight: 700, borderRadius: 999, flexShrink: 0,
      }}>
        {stateInfo.label}
      </span>

      <button style={{ width: 26, height: 26, borderRadius: 7, border: 'none', background: 'transparent', color: t.ink3, cursor: 'pointer', display: 'flex', alignItems: 'center', justifyContent: 'center' }} title="Sao chép link">
        <Icons.copy size={13}/>
      </button>
      <button style={{ width: 26, height: 26, borderRadius: 7, border: 'none', background: 'transparent', color: t.ink3, cursor: 'pointer', display: 'flex', alignItems: 'center', justifyContent: 'center' }} title="Mở">
        <Icons.external size={13}/>
      </button>
    </div>
  );
}

Object.assign(window, { AuroraSyncAuto, AuroraSyncAutoDetail });
