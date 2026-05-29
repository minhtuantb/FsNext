// aurora-upload-modal.jsx
// Màn hình "Tải lên" phiên bản có MODAL + FOLDER TREE picker.
// Ở trạng thái rỗng (0 file), nền xám mờ — modal "Tải lên file" ở giữa:
//   · Drag-drop zone (icon ↑ · "Kéo thả file vào đây" · "hoặc chọn từ máy tính")
//   · Dropdown "Tải vào:" — khi mở, xổ ra cây thư mục có lựa chọn highlighted
//   · Input mật khẩu bảo vệ
//   · Toggle file riêng tư
//   · 2 nút Hủy / Bắt đầu tải lên (disabled khi chưa có file)
// Trạng thái mở của dropdown điều khiển bằng state — click vào để xổ xuống.

function AuroraUploadModal() {
  const t = useTA();
  const [collapsed, setCollapsed] = React.useState(false);
  const [pickerOpen, setPickerOpen] = React.useState(true);       // demo: mở sẵn để thấy cây
  const [dest, setDest] = React.useState('/');                     // path chọn
  const [expanded, setExpanded] = React.useState({ '/': true, '/Fshare_Data': true, '/HoaSen': true });
  const [isPrivate, setIsPrivate] = React.useState(false);
  const [password, setPassword] = React.useState('');
  const [tab, setTab] = React.useState('active');                  // active | history
  const [dragOver, setDragOver] = React.useState(false);

  // Cây thư mục (giả) — theo screenshot: Cisco, Documents, Fshare_Data, Game, Gốc 1, HoaSen, HoaSen(1), HoaSen(2)…
  const tree = [
    { path: '/', name: '/ (Thư mục gốc)', isRoot: true, children: [
      { path: '/Cisco', name: 'Cisco', count: 12 },
      { path: '/Documents', name: 'Documents', count: 48, children: [
        { path: '/Documents/Reports', name: 'Reports', count: 8 },
        { path: '/Documents/Contracts', name: 'Contracts', count: 14 },
        { path: '/Documents/Invoices', name: 'Invoices', count: 22 },
      ]},
      { path: '/Fshare_Data', name: 'Fshare_Data', count: 186, children: [
        { path: '/Fshare_Data/Backup_2026', name: 'Backup_2026', count: 42 },
        { path: '/Fshare_Data/Client_Assets', name: 'Client_Assets', count: 68 },
        { path: '/Fshare_Data/Archive', name: 'Archive', count: 24 },
      ]},
      { path: '/Game', name: 'Game', count: 7 },
      { path: '/Goc_1', name: 'Gốc 1', count: 3 },
      { path: '/HoaSen', name: 'HoaSen', count: 94, children: [
        { path: '/HoaSen/2024', name: '2024', count: 18 },
        { path: '/HoaSen/2025', name: '2025', count: 32 },
        { path: '/HoaSen/2026_Q1', name: '2026_Q1', count: 16 },
      ]},
      { path: '/HoaSen(1)', name: 'HoaSen(1)', count: 12 },
      { path: '/HoaSen(2)', name: 'HoaSen(2)', count: 8 },
      { path: '/Phim', name: 'Phim', count: 52 },
      { path: '/RAW_Photos', name: 'RAW_Photos', count: 1240 },
    ]}
  ];

  const toggleExpand = (p) => setExpanded(s => ({ ...s, [p]: !s[p] }));
  const pick = (p) => { setDest(p); setPickerOpen(false); };
  const destLabel = dest === '/' ? '/ (Thư mục gốc)' : dest;

  return (
    <WinChrome title="Fshare · Tải lên" theme={t}>
      <AuroraSidebar active="uploads" collapsed={collapsed} onToggle={() => setCollapsed(v => !v)}/>
      <div style={{ flex: 1, background: t.bg, overflow: 'hidden', position: 'relative', display: 'flex', flexDirection: 'column' }}>

        {/* ── HEADER BAR ── */}
        <div style={{ padding: '22px 32px 18px', borderBottom: `1px solid ${t.border}`, background: t.panel, display: 'flex', alignItems: 'center', gap: 20, flexShrink: 0 }}>
          <div style={{ display: 'flex', alignItems: 'baseline', gap: 10 }}>
            <h1 style={{ margin: 0, fontFamily: t.serif, fontSize: 34, fontWeight: 400, letterSpacing: '-.02em' }}>Tải lên</h1>
            <span style={{ fontSize: 13, fontFamily: t.mono, color: t.ink4 }}>0 file</span>
          </div>

          {/* segmented tabs Đang tải / Lịch sử */}
          <div style={{ marginLeft: 24, display: 'flex', padding: 3, background: t.bg, borderRadius: 999, border: `1px solid ${t.border}` }}>
            <TabBtn t={t} active={tab==='active'} onClick={()=>setTab('active')}>Đang tải</TabBtn>
            <TabBtn t={t} active={tab==='history'} onClick={()=>setTab('history')}>Lịch sử</TabBtn>
          </div>

          <div style={{ marginLeft: 'auto', display: 'flex', gap: 10 }}>
            <button style={{ padding: '9px 14px', background: 'transparent', color: t.ink2, border: `1px solid ${t.border}`, borderRadius: 999, fontSize: 12.5, fontWeight: 600, cursor: 'pointer', fontFamily: t.font, display: 'flex', alignItems: 'center', gap: 6 }}>
              <Icons.folder size={13}/> Tải cả thư mục
            </button>
            <button style={{ padding: '9px 18px', background: t.grad, color: '#fff', border: 'none', borderRadius: 999, fontSize: 13, fontWeight: 700, cursor: 'pointer', fontFamily: t.font, boxShadow: t.shadow, display: 'flex', alignItems: 'center', gap: 6 }}>
              <Icons.plus size={13}/> Chọn file
            </button>
          </div>
        </div>

        {/* ── EMPTY CANVAS (under modal) ── */}
        <div style={{ flex: 1, position: 'relative', overflow: 'hidden', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
          {/* faint hint behind the modal */}
          <div style={{
            position: 'absolute', inset: 0,
            background: `
              radial-gradient(600px 400px at 20% 30%, ${t.accent}08, transparent 60%),
              radial-gradient(500px 400px at 80% 70%, ${t.accent3}08, transparent 60%)
            `,
            opacity: .8, pointerEvents: 'none',
          }}/>

          {/* subtle grid pattern */}
          <svg style={{ position: 'absolute', inset: 0, width: '100%', height: '100%', opacity: .35 }}>
            <defs>
              <pattern id="dotgrid" width="28" height="28" patternUnits="userSpaceOnUse">
                <circle cx="2" cy="2" r="1" fill={t.border}/>
              </pattern>
            </defs>
            <rect width="100%" height="100%" fill="url(#dotgrid)"/>
          </svg>

          {/* ── MODAL ── */}
          <div style={{
            position: 'relative', width: 560,
            background: t.panel, border: `1px solid ${t.border}`,
            borderRadius: 20, boxShadow: `0 40px 80px -20px rgba(0,0,0,.25), 0 0 0 1px ${t.border}`,
            fontFamily: t.font, overflow: 'visible',
          }}>
            {/* modal header */}
            <div style={{ padding: '22px 28px 18px', display: 'flex', alignItems: 'center', justifyContent: 'space-between', borderBottom: `1px solid ${t.border}` }}>
              <div style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
                <div style={{ width: 34, height: 34, borderRadius: 10, background: t.gradSoft, display: 'flex', alignItems: 'center', justifyContent: 'center', color: t.accent }}>
                  <Icons.upload size={16}/>
                </div>
                <div>
                  <div style={{ fontSize: 17, fontWeight: 700, letterSpacing: '-.01em' }}>Tải lên file</div>
                  <div style={{ fontSize: 11.5, color: t.ink4, marginTop: 1 }}>Kéo thả hoặc chọn từ máy · tối đa 50 GB mỗi file</div>
                </div>
              </div>
              <button style={{ width: 30, height: 30, borderRadius: 8, background: 'transparent', border: 'none', color: t.ink3, cursor: 'pointer', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
                <Icons.x size={15}/>
              </button>
            </div>

            {/* body */}
            <div style={{ padding: '22px 28px 24px' }}>
              {/* drop zone */}
              <div
                onDragOver={(e)=>{ e.preventDefault(); setDragOver(true); }}
                onDragLeave={()=>setDragOver(false)}
                onDrop={(e)=>{ e.preventDefault(); setDragOver(false); }}
                style={{
                  padding: '34px 20px', border: `1.5px dashed ${dragOver ? t.accent : t.border}`,
                  borderRadius: 14, background: dragOver ? t.gradSoft : t.bg,
                  textAlign: 'center', transition: 'all .18s cubic-bezier(.2,0,0,1)',
                }}>
                <div style={{
                  width: 52, height: 52, margin: '0 auto 12px', borderRadius: 14,
                  background: dragOver ? t.grad : t.panel,
                  color: dragOver ? '#fff' : t.ink2,
                  border: `1.5px solid ${dragOver ? 'transparent' : t.borderStrong}`,
                  display: 'flex', alignItems: 'center', justifyContent: 'center',
                  boxShadow: dragOver ? t.shadow : 'none',
                }}><Icons.upload size={22}/></div>
                <div style={{ fontSize: 14, fontWeight: 600, color: t.ink }}>Kéo thả file vào đây</div>
                <div style={{ fontSize: 12.5, color: t.ink3, marginTop: 6 }}>
                  hoặc <a style={{ color: t.accent, textDecoration: 'underline', textUnderlineOffset: 3, fontWeight: 600, cursor: 'pointer' }}>chọn từ máy tính</a>
                </div>
              </div>

              {/* destination picker */}
              <div style={{ marginTop: 18, display: 'flex', alignItems: 'center', gap: 14, position: 'relative' }}>
                <label style={{ fontSize: 12.5, color: t.ink3, flexShrink: 0, minWidth: 56 }}>Tải vào:</label>

                <button
                  onClick={() => setPickerOpen(v => !v)}
                  style={{
                    flex: 1, display: 'flex', alignItems: 'center', gap: 10,
                    padding: '10px 14px', background: t.bg,
                    border: `1.5px solid ${pickerOpen ? t.accent : t.border}`,
                    borderRadius: 10, fontSize: 13, fontWeight: 500, color: t.ink,
                    fontFamily: t.font, cursor: 'pointer', textAlign: 'left',
                    justifyContent: 'space-between',
                    boxShadow: pickerOpen ? `0 0 0 3px ${t.accent}22` : 'none',
                    transition: 'all .15s',
                  }}>
                  <span style={{ display: 'flex', alignItems: 'center', gap: 8, minWidth: 0 }}>
                    <Icons.folder size={14} stroke={t.accent}/>
                    <span style={{ fontFamily: t.mono, fontSize: 12.5, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>{destLabel}</span>
                  </span>
                  <Icons.chevDown size={13} style={{ color: t.ink3, transform: pickerOpen ? 'rotate(180deg)' : 'none', transition: 'transform .18s' }}/>
                </button>

                {/* FOLDER TREE DROPDOWN */}
                {pickerOpen && (
                  <div style={{
                    position: 'absolute', top: '100%', left: 70, right: 0, marginTop: 6, zIndex: 100,
                    background: t.panel, border: `1px solid ${t.border}`,
                    borderRadius: 12, boxShadow: `0 20px 50px -10px rgba(0,0,0,.25), 0 0 0 1px ${t.border}`,
                    padding: 8, maxHeight: 340, overflowY: 'auto',
                  }}>
                    {/* search bar inside dropdown */}
                    <div style={{ display: 'flex', alignItems: 'center', gap: 8, padding: '8px 10px', margin: '0 0 6px', background: t.bg, border: `1px solid ${t.border}`, borderRadius: 8 }}>
                      <Icons.search size={12} stroke={t.ink3}/>
                      <input
                        placeholder="Tìm thư mục…"
                        style={{ flex: 1, border: 'none', background: 'transparent', outline: 'none', fontSize: 12, color: t.ink, fontFamily: t.font }}/>
                      <span style={{ fontSize: 10, fontFamily: t.mono, color: t.ink4, padding: '2px 6px', background: t.panel, borderRadius: 4, border: `1px solid ${t.border}` }}>⌘ K</span>
                    </div>

                    {/* tree */}
                    {tree.map(n => <TreeNode key={n.path} node={n} depth={0} dest={dest} pick={pick} expanded={expanded} toggle={toggleExpand} t={t}/>)}

                    {/* footer actions */}
                    <div style={{ marginTop: 6, padding: '8px 10px', borderTop: `1px solid ${t.border}`, display: 'flex', alignItems: 'center', gap: 8 }}>
                      <button style={{ fontSize: 11.5, color: t.accent, background: 'transparent', border: 'none', cursor: 'pointer', fontWeight: 600, padding: 0, display: 'flex', alignItems: 'center', gap: 4, fontFamily: t.font }}>
                        <Icons.plus size={11}/> Tạo thư mục mới
                      </button>
                      <span style={{ marginLeft: 'auto', fontSize: 10.5, fontFamily: t.mono, color: t.ink4 }}>↑↓ chọn · ↵ xác nhận · Esc đóng</span>
                    </div>
                  </div>
                )}
              </div>

              {/* password */}
              <div style={{ marginTop: 14, position: 'relative' }}>
                <input
                  type="text"
                  value={password}
                  onChange={e=>setPassword(e.target.value)}
                  placeholder="Mật khẩu bảo vệ file (để trống nếu không cần)"
                  style={{
                    width: '100%', padding: '10px 40px 10px 14px', background: t.bg,
                    border: `1.5px solid ${t.border}`, borderRadius: 10,
                    fontSize: 13, color: t.ink, fontFamily: t.font, outline: 'none',
                  }}/>
                <Icons.lock size={13} style={{ position: 'absolute', right: 14, top: '50%', transform: 'translateY(-50%)', color: t.ink4 }}/>
              </div>

              {/* private toggle */}
              <div style={{ marginTop: 16, display: 'flex', alignItems: 'center', gap: 10 }}>
                <button
                  onClick={()=>setIsPrivate(v=>!v)}
                  style={{
                    width: 34, height: 20, padding: 2, border: 'none', cursor: 'pointer',
                    background: isPrivate ? t.grad : t.border,
                    borderRadius: 999, display: 'flex',
                    justifyContent: isPrivate ? 'flex-end' : 'flex-start',
                    transition: 'background .2s',
                  }}>
                  <div style={{ width: 16, height: 16, background: '#fff', borderRadius: '50%', boxShadow: '0 1px 3px rgba(0,0,0,.2)' }}/>
                </button>
                <div>
                  <div style={{ fontSize: 12.5, fontWeight: 600, color: t.ink }}>File riêng tư</div>
                  <div style={{ fontSize: 11, color: t.ink4 }}>Chỉ bạn xem được · không xuất hiện trong tìm kiếm công khai</div>
                </div>
              </div>
            </div>

            {/* footer buttons */}
            <div style={{ padding: '16px 28px', borderTop: `1px solid ${t.border}`, display: 'flex', justifyContent: 'flex-end', gap: 10 }}>
              <button style={{ padding: '10px 20px', background: 'transparent', color: t.ink2, border: `1.5px solid ${t.border}`, borderRadius: 999, fontSize: 13, fontWeight: 600, cursor: 'pointer', fontFamily: t.font }}>
                Hủy
              </button>
              <button
                disabled
                style={{
                  padding: '10px 22px', background: t.grad, color: '#fff', border: 'none',
                  borderRadius: 999, fontSize: 13, fontWeight: 700, cursor: 'not-allowed',
                  fontFamily: t.font, opacity: .45,
                }}>
                Bắt đầu tải lên
              </button>
            </div>
          </div>
        </div>

        {/* ── FOOTER HINT ── */}
        <div style={{ padding: '12px 32px', borderTop: `1px solid ${t.border}`, background: t.panel, display: 'flex', alignItems: 'center', gap: 20, fontSize: 11.5, color: t.ink3, flexShrink: 0 }}>
          <span style={{ display: 'flex', alignItems: 'center', gap: 6 }}><Icons.shield size={12} stroke={t.success}/> Mã hoá AES-256 end-to-end</span>
          <span style={{ display: 'flex', alignItems: 'center', gap: 6 }}><Icons.zap size={12} stroke={t.accent}/> Tốc độ VIP không giới hạn</span>
          <span style={{ display: 'flex', alignItems: 'center', gap: 6 }}><Icons.cloud size={12} stroke={t.ink3}/> Còn 213 GB / 500 GB</span>
          <span style={{ marginLeft: 'auto', fontFamily: t.mono, fontSize: 10.5, color: t.ink4 }}>Tự động resume khi mất mạng · tối đa 50 GB/file</span>
        </div>
      </div>
    </WinChrome>
  );
}

// ─── subcomponents ────────────────────────────────────────────────────────

function TabBtn({ t, active, onClick, children }) {
  return (
    <button onClick={onClick} style={{
      padding: '6px 14px', borderRadius: 999, border: 'none',
      background: active ? t.panel : 'transparent',
      color: active ? t.accent : t.ink3,
      fontSize: 12, fontWeight: 600, cursor: 'pointer', fontFamily: t.font,
      boxShadow: active ? `0 1px 3px rgba(0,0,0,.08)` : 'none',
      transition: 'all .15s',
    }}>{children}</button>
  );
}

function TreeNode({ node, depth, dest, pick, expanded, toggle, t }) {
  const hasChildren = node.children && node.children.length > 0;
  const isOpen = expanded[node.path];
  const selected = dest === node.path;

  return (
    <div>
      <div
        onClick={() => pick(node.path)}
        style={{
          display: 'flex', alignItems: 'center', gap: 8,
          padding: '7px 10px', paddingLeft: 10 + depth * 18,
          borderRadius: 8, cursor: 'pointer',
          background: selected ? t.gradSoft : 'transparent',
          color: selected ? t.accent : t.ink,
          fontSize: 12.5, fontWeight: selected ? 700 : 500,
          fontFamily: node.isRoot ? t.mono : t.font,
        }}
        onMouseEnter={e => { if (!selected) e.currentTarget.style.background = t.bg; }}
        onMouseLeave={e => { if (!selected) e.currentTarget.style.background = 'transparent'; }}
      >
        {hasChildren ? (
          <span
            onClick={(e) => { e.stopPropagation(); toggle(node.path); }}
            style={{ width: 14, height: 14, display: 'flex', alignItems: 'center', justifyContent: 'center', color: t.ink3 }}>
            <Icons.chevRight size={10} style={{ transform: isOpen ? 'rotate(90deg)' : 'none', transition: 'transform .15s' }}/>
          </span>
        ) : (
          <span style={{ width: 14 }}/>
        )}
        <Icons.folder size={13} stroke={selected ? t.accent : (node.isRoot ? t.ink3 : t.ink3)} fill={selected ? t.accent : 'none'}/>
        <span style={{ flex: 1, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>{node.name}</span>
        {!node.isRoot && node.count != null && (
          <span style={{ fontSize: 10, fontFamily: t.mono, color: t.ink4 }}>{node.count}</span>
        )}
        {selected && <Icons.check size={12} stroke={t.accent}/>}
      </div>
      {hasChildren && isOpen && node.children.map(c => (
        <TreeNode key={c.path} node={c} depth={depth + 1} dest={dest} pick={pick} expanded={expanded} toggle={toggle} t={t}/>
      ))}
    </div>
  );
}

Object.assign(window, { AuroraUploadModal });
