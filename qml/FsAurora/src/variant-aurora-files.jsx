// variant-aurora-files.jsx — Aurora File Manager (multi-level folders)
// Menu chính có thể thu gọn (mặc định collapsed trong context này để có nhiều diện tích cho 3 panel)

function AuroraFiles() {
  const t = useTA();
  const [navCollapsed, setNavCollapsed] = React.useState(true);
  return (
    <WinChrome title="Fshare · File của tôi" theme={t}>
      <AuroraSidebar active="files" collapsed={navCollapsed} onToggle={() => setNavCollapsed(v => !v)}/>
      <div style={{ flex: 1, display: 'flex', overflow: 'hidden', background: t.bg }}>
        {/* ── FOLDER TREE PANEL ── */}
        <div style={{
          width: 260, background: t.panel, borderRight: `1px solid ${t.border}`,
          display: 'flex', flexDirection: 'column', overflow: 'hidden',
        }}>
          <div style={{ padding: '14px 16px 10px', borderBottom: `1px solid ${t.border}` }}>
            <div style={{ fontSize: 10, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600, marginBottom: 8 }}>Thư mục</div>
            <div style={{ display: 'flex', gap: 6 }}>
              <button style={{
                flex: 1, padding: '6px 8px', background: t.gradSoft, color: t.accent,
                border: `1px solid ${t.border}`, borderRadius: 6, fontSize: 11, fontWeight: 600,
                cursor: 'pointer', display: 'flex', alignItems: 'center', gap: 4, fontFamily: t.font,
                justifyContent: 'center',
              }}><Icons.plus size={11}/> Folder mới</button>
              <button style={{
                padding: '6px 8px', background: 'transparent', color: t.ink3,
                border: `1px solid ${t.border}`, borderRadius: 6, cursor: 'pointer',
              }}><Icons.upload size={12}/></button>
            </div>
          </div>
          <div style={{ flex: 1, overflow: 'auto', padding: '10px 8px' }}>
            <Tree theme={t} label="Root · File của tôi" icon={Icons.cloud} open>
              <Tree theme={t} label="Phim" count="184" size="412 GB" open>
                <Tree theme={t} label="2026" count="8"/>
                <Tree theme={t} label="2025" count="42" open>
                  <Tree theme={t} label="Oscar winners" count="12"/>
                  <Tree theme={t} label="Netflix rips" count="24"/>
                  <Tree theme={t} label="IMAX" count="6"/>
                </Tree>
                <Tree theme={t} label="2024" count="68"/>
                <Tree theme={t} label="2023" count="34" active/>
                <Tree theme={t} label="Classics" count="32"/>
              </Tree>
              <Tree theme={t} label="Âm nhạc" count="1.2K" size="102 GB">
                <Tree theme={t} label="Vpop"/>
                <Tree theme={t} label="FLAC Collection"/>
                <Tree theme={t} label="Playlists"/>
              </Tree>
              <Tree theme={t} label="Tài liệu" count="342" size="14 GB" open>
                <Tree theme={t} label="Công việc" open>
                  <Tree theme={t} label="Q1 2026"/>
                  <Tree theme={t} label="Q4 2025"/>
                </Tree>
                <Tree theme={t} label="Học tập"/>
                <Tree theme={t} label="Cá nhân"/>
              </Tree>
              <Tree theme={t} label="Ảnh" count="4.8K" size="68 GB"/>
              <Tree theme={t} label="Phần mềm" count="24" size="84 GB"/>
              <Tree theme={t} label="Backup" count="12" size="128 GB"/>
            </Tree>

            <div style={{ marginTop: 18, padding: '0 10px', fontSize: 10, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600, marginBottom: 6 }}>Chia sẻ với tôi</div>
            <Tree theme={t} label="Shared · 6 folder" icon={Icons.users}>
              <Tree theme={t} label="Team Design 2026"/>
              <Tree theme={t} label="Tài liệu nhóm"/>
            </Tree>
          </div>

          {/* storage footer */}
          <div style={{ padding: '12px 16px', borderTop: `1px solid ${t.border}` }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: 10.5, fontFamily: t.mono, color: t.ink4, marginBottom: 4 }}>
              <span>287 GB dùng</span><span>57%</span>
            </div>
            <div style={{ height: 3, background: t.border, borderRadius: 2 }}>
              <div style={{ width: '57%', height: '100%', background: t.grad, borderRadius: 2 }}/>
            </div>
          </div>
        </div>

        {/* ── MAIN FILE AREA ── */}
        <div style={{ flex: 1, display: 'flex', flexDirection: 'column', overflow: 'hidden' }}>
          {/* top toolbar */}
          <div style={{
            padding: '14px 22px', background: t.panel, borderBottom: `1px solid ${t.border}`,
            display: 'flex', alignItems: 'center', gap: 14,
          }}>
            {/* breadcrumb */}
            <div style={{ display: 'flex', alignItems: 'center', gap: 6, fontSize: 13, flex: 1, minWidth: 0, flexWrap: 'wrap' }}>
              <Crumb theme={t} icon={Icons.cloud}>File của tôi</Crumb>
              <Icons.chevRight size={13} style={{ color: t.ink4 }}/>
              <Crumb theme={t}>Phim</Crumb>
              <Icons.chevRight size={13} style={{ color: t.ink4 }}/>
              <Crumb theme={t}>2023</Crumb>
              <Icons.chevRight size={13} style={{ color: t.ink4 }}/>
              <Crumb theme={t} active>IMAX Remux</Crumb>
            </div>

            <div style={{ display: 'flex', alignItems: 'center', gap: 6 }}>
              {/* view toggle */}
              <div style={{ display: 'flex', background: t.bg, borderRadius: 8, padding: 2, border: `1px solid ${t.border}` }}>
                <ViewBtn theme={t}><Icons.list size={13}/></ViewBtn>
                <ViewBtn theme={t} active><Icons.grid size={13}/></ViewBtn>
              </div>
              <div style={{ width: 1, height: 20, background: t.border, margin: '0 4px' }}/>
              <button style={btnGhost(t)}><Icons.filter size={13}/> Lọc</button>
              <button style={btnGhost(t)}><Icons.arrowDown size={13}/> Sắp xếp</button>
              <button style={{
                padding: '8px 14px', background: t.grad, color: '#fff', border: 'none', borderRadius: 8,
                fontSize: 12.5, fontWeight: 600, cursor: 'pointer', display: 'flex', alignItems: 'center',
                gap: 6, fontFamily: t.font, boxShadow: '0 4px 14px rgba(255,91,46,.25)',
              }}><Icons.upload size={13}/> Tải lên</button>
            </div>
          </div>

          {/* sub header — path info strip */}
          <div style={{ padding: '16px 22px 8px', display: 'flex', alignItems: 'baseline', justifyContent: 'space-between' }}>
            <div>
              <h1 style={{ fontFamily: t.serif, fontSize: 40, fontWeight: 400, letterSpacing: '-.03em', margin: 0, lineHeight: 1 }}>
                IMAX <span style={{ fontStyle: 'italic', color: t.accent }}>Remux.</span>
              </h1>
              <div style={{ fontSize: 12, color: t.ink4, fontFamily: t.mono, marginTop: 8, display: 'flex', gap: 14 }}>
                <span>6 folder · 18 file</span><span>·</span>
                <span>142 GB tổng</span><span>·</span>
                <span>Cập nhật 2 giờ trước</span><span>·</span>
                <span style={{ color: t.accent, fontWeight: 600 }}>● Đang sync</span>
              </div>
            </div>
            {/* selection badge */}
            <div style={{
              padding: '6px 12px', background: t.gradSoft, border: `1px solid ${t.border}`,
              borderRadius: 999, fontSize: 11.5, fontWeight: 600, color: t.accent,
              display: 'flex', alignItems: 'center', gap: 8,
            }}>
              <Icons.check size={12}/> 3 mục đã chọn · 18.4 GB
              <span style={{ marginLeft: 8, fontSize: 10, color: t.ink4, cursor: 'pointer' }}>Bỏ</span>
            </div>
          </div>

          {/* file content — grid view */}
          <div style={{ flex: 1, overflow: 'auto', padding: '16px 22px 22px' }}>
            {/* folders section */}
            <div style={{ fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600, marginBottom: 10 }}>━ Thư mục con · 6</div>
            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(180px, 1fr))', gap: 10, marginBottom: 22 }}>
              <FolderTile theme={t} name="Oppenheimer 2023" count="4 file" size="28 GB" selected/>
              <FolderTile theme={t} name="Dune Part 2" count="3 file" size="24 GB"/>
              <FolderTile theme={t} name="Avatar WOW" count="5 file" size="38 GB" shared/>
              <FolderTile theme={t} name="Mission Impossible 7" count="2 file" size="18 GB"/>
              <FolderTile theme={t} name="Spider-Man ATSV" count="3 file" size="14 GB" starred/>
              <FolderTile theme={t} name="John Wick 4" count="1 file" size="6 GB"/>
            </div>

            {/* files section */}
            <div style={{ fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600, marginBottom: 10, display: 'flex', justifyContent: 'space-between' }}>
              <span>━ File · 18</span>
              <span style={{ textTransform: 'none', letterSpacing: 0 }}>Sắp xếp theo <b style={{ color: t.ink2 }}>Thời gian ↓</b></span>
            </div>

            {/* data grid */}
            <div style={{ background: t.panel, borderRadius: 14, border: `1px solid ${t.border}`, overflow: 'hidden' }}>
              {/* header row */}
              <div style={{
                display: 'grid', gridTemplateColumns: '28px 1fr 100px 90px 130px 70px',
                padding: '10px 16px', borderBottom: `1px solid ${t.border}`,
                fontSize: 10, fontFamily: t.mono, color: t.ink4, letterSpacing: '.1em',
                textTransform: 'uppercase', fontWeight: 700, background: t.bg,
              }}>
                <span></span>
                <span>Tên</span>
                <span style={{ textAlign: 'right' }}>Kích thước</span>
                <span style={{ textAlign: 'center' }}>Loại</span>
                <span>Chỉnh sửa</span>
                <span style={{ textAlign: 'right' }}>Thao tác</span>
              </div>
              {[
                { sel: true, n: 'Oppenheimer_2023_IMAX_4K_HDR.mkv', s: '18.2 GB', ty: 'MKV · 4K', t: '2 giờ trước', i: Icons.video, tag: 'VIP' },
                { n: 'Dune_Part_Two_2024_2160p.mkv', s: '16.8 GB', ty: 'MKV · 4K', t: 'Hôm qua', i: Icons.video, star: true },
                { sel: true, n: 'Avatar_Way_of_Water_4K.mp4', s: '14.2 GB', ty: 'MP4 · 4K', t: '2 ngày trước', i: Icons.video, shared: true },
                { n: 'Mission_Impossible_DR1_1080p.mkv', s: '12.4 GB', ty: 'MKV · 1080p', t: '3 ngày trước', i: Icons.video },
                { n: 'Spider-Man_ATSV_1080p_VietSub.mkv', s: '4.2 GB', ty: 'MKV · 1080p', t: '4 ngày trước', i: Icons.video },
                { sel: true, n: 'Barbie_2023_1080p.mp4', s: '2.1 GB', ty: 'MP4 · 1080p', t: '1 tuần trước', i: Icons.video },
                { n: 'subtitles_pack_vi.rar', s: '14 MB', ty: 'RAR', t: '1 tuần trước', i: Icons.archive },
                { n: 'poster_collection.zip', s: '84 MB', ty: 'ZIP', t: '2 tuần trước', i: Icons.archive },
                { n: 'BTS_Oppenheimer_making.mp4', s: '842 MB', ty: 'MP4 · 1080p', t: '2 tuần trước', i: Icons.video },
                { n: 'soundtrack_collection.flac', s: '1.2 GB', ty: 'FLAC', t: '3 tuần trước', i: Icons.music },
                { n: 'review_notes_IMAX.pdf', s: '2.4 MB', ty: 'PDF', t: '1 tháng trước', i: Icons.doc },
                { n: 'thumb_gallery.jpg', s: '18 MB', ty: 'JPG', t: '1 tháng trước', i: Icons.image },
              ].map((f, i, a) => <DataRow key={i} theme={t} {...f} last={i === a.length - 1}/>)}
            </div>

            {/* drag-drop hint */}
            <div style={{
              marginTop: 14, padding: '18px 24px', border: `1.5px dashed ${t.border}`,
              borderRadius: 12, display: 'flex', alignItems: 'center', gap: 14,
              background: t.panel,
            }}>
              <div style={{
                width: 44, height: 44, borderRadius: 12, background: t.gradSoft,
                display: 'flex', alignItems: 'center', justifyContent: 'center', color: t.accent,
              }}><Icons.upload size={20}/></div>
              <div>
                <div style={{ fontSize: 14, fontWeight: 600 }}>Kéo thả file vào đây để tải lên</div>
                <div style={{ fontSize: 12, color: t.ink3, marginTop: 2 }}>Hoặc <span style={{ color: t.accent, fontWeight: 600, cursor: 'pointer' }}>chọn từ máy</span> · Tối đa 50 GB/file · Tự động sync với folder này</div>
              </div>
            </div>
          </div>

          {/* status bar */}
          <div style={{
            padding: '8px 22px', background: t.panel, borderTop: `1px solid ${t.border}`,
            display: 'flex', alignItems: 'center', gap: 20, fontSize: 11, fontFamily: t.mono, color: t.ink4,
          }}>
            <span>24 mục</span>
            <span style={{ color: t.accent }}>3 đã chọn · 18.4 GB</span>
            <span style={{ marginLeft: 'auto', display: 'flex', alignItems: 'center', gap: 6 }}>
              <div style={{ width: 6, height: 6, borderRadius: 999, background: t.success }}/>
              Đồng bộ trực tiếp · 88.3 MB/s
            </span>
          </div>
        </div>

        {/* ── RIGHT DETAIL PANE ── */}
        <div style={{ width: 300, background: t.panel, borderLeft: `1px solid ${t.border}`, overflow: 'auto' }}>
          {/* preview */}
          <div style={{
            aspectRatio: '16/10', background: 'linear-gradient(135deg, #2a1a0a 0%, #4a2818 100%)',
            position: 'relative', overflow: 'hidden',
          }}>
            <div style={{ position: 'absolute', inset: 0, backgroundImage: 'radial-gradient(circle at 70% 30%, rgba(255,91,46,.35), transparent 60%)' }}/>
            <div style={{ position: 'absolute', top: 12, left: 12, padding: '3px 8px', background: 'rgba(0,0,0,.5)', backdropFilter: 'blur(10px)', borderRadius: 999, fontSize: 10, fontFamily: t.mono, color: '#fff', letterSpacing: '.08em', fontWeight: 600 }}>
              IMAX · 4K HDR
            </div>
            <div style={{
              position: 'absolute', bottom: 0, left: 0, right: 0, padding: 16,
              background: 'linear-gradient(180deg, transparent, rgba(0,0,0,.8))',
              color: '#fff',
            }}>
              <div style={{ fontSize: 11, fontFamily: t.mono, opacity: .7, letterSpacing: '.08em' }}>VIDEO · 2023</div>
              <div style={{ fontFamily: t.serif, fontSize: 24, fontStyle: 'italic', lineHeight: 1.1, marginTop: 4 }}>Oppenheimer</div>
            </div>
            <button style={{
              position: 'absolute', top: '50%', left: '50%', transform: 'translate(-50%, -50%)',
              width: 54, height: 54, borderRadius: 999, background: 'rgba(255,255,255,.95)',
              border: 'none', cursor: 'pointer', display: 'flex', alignItems: 'center', justifyContent: 'center',
            }}><Icons.play size={22} stroke="#0E0E12" fill="#0E0E12"/></button>
          </div>

          <div style={{ padding: 18 }}>
            <div style={{ fontSize: 15, fontWeight: 600, marginBottom: 3 }}>Oppenheimer_2023_IMAX_4K_HDR.mkv</div>
            <div style={{ fontSize: 11.5, color: t.ink4, fontFamily: t.mono }}>/Phim/2023/IMAX Remux/</div>

            <div style={{ marginTop: 18, paddingBottom: 16, borderBottom: `1px solid ${t.border}`, display: 'flex', gap: 8 }}>
              <button style={{ flex: 1, padding: '9px', background: t.grad, color: '#fff', border: 'none', borderRadius: 8, fontSize: 12, fontWeight: 600, cursor: 'pointer', display: 'flex', alignItems: 'center', justifyContent: 'center', gap: 5, fontFamily: t.font }}>
                <Icons.download size={12}/> Tải xuống
              </button>
              <button style={{ padding: '9px 12px', background: 'transparent', color: t.ink, border: `1px solid ${t.border}`, borderRadius: 8, fontSize: 12, fontWeight: 500, cursor: 'pointer', display: 'flex', alignItems: 'center', gap: 5, fontFamily: t.font }}>
                <Icons.share size={12}/>
              </button>
              <button style={{ padding: '9px 12px', background: 'transparent', color: t.ink, border: `1px solid ${t.border}`, borderRadius: 8, fontSize: 12, cursor: 'pointer', display: 'flex', alignItems: 'center' }}>
                <Icons.more size={13}/>
              </button>
            </div>

            <DetailRow theme={t} label="Kích thước" val="18.2 GB"/>
            <DetailRow theme={t} label="Định dạng" val="MKV · H.265"/>
            <DetailRow theme={t} label="Độ phân giải" val="3840 × 2160"/>
            <DetailRow theme={t} label="Thời lượng" val="3h 00m 42s"/>
            <DetailRow theme={t} label="Âm thanh" val="DTS-HD 5.1 · 2 track"/>
            <DetailRow theme={t} label="Tạo lúc" val="15/07/2023"/>
            <DetailRow theme={t} label="Sửa đổi" val="2 giờ trước"/>
            <DetailRow theme={t} label="Đã tải xuống" val="3 lần"/>

            <div style={{ marginTop: 20, padding: '14px', background: t.gradSoft, borderRadius: 10 }}>
              <div style={{ fontSize: 11, fontFamily: t.mono, color: t.accent, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 700, marginBottom: 8 }}>Đã chia sẻ</div>
              <div style={{ fontSize: 12, fontFamily: t.mono, color: t.ink2, wordBreak: 'break-all' }}>fshare.vn/s/op9K3mX</div>
              <div style={{ display: 'flex', gap: 12, marginTop: 8, fontSize: 11, color: t.ink3 }}>
                <span style={{ display: 'flex', alignItems: 'center', gap: 4 }}><Icons.eye size={10}/> 42 lượt</span>
                <span style={{ display: 'flex', alignItems: 'center', gap: 4 }}><Icons.clock size={10}/> còn 14 ngày</span>
              </div>
            </div>

            <div style={{ marginTop: 18, fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600, marginBottom: 8 }}>━ Hoạt động</div>
            {[
              { t: '2 giờ trước', e: 'Minh đã sửa đổi' },
              { t: 'Hôm qua', e: 'Tải xuống từ Windows PC' },
              { t: '3 ngày', e: 'Chia sẻ công khai · 14 ngày' },
              { t: '1 tuần', e: 'Tạo từ folder Phim / 2023' },
            ].map((a, i) => (
              <div key={i} style={{ display: 'flex', gap: 10, padding: '8px 0', borderBottom: i === 3 ? 'none' : `1px dashed ${t.border}` }}>
                <div style={{ width: 5, height: 5, borderRadius: 999, background: t.accent, marginTop: 6 }}/>
                <div style={{ flex: 1 }}>
                  <div style={{ fontSize: 12, color: t.ink2 }}>{a.e}</div>
                  <div style={{ fontSize: 10.5, color: t.ink4, fontFamily: t.mono, marginTop: 1 }}>{a.t}</div>
                </div>
              </div>
            ))}
          </div>
        </div>
      </div>
    </WinChrome>
  );
}

// ─────────────────────────────────────────────────────────────
function Tree({ theme, label, icon, count, size, active, open, children }) {
  const Ico = icon || Icons.folder;
  const hasChildren = !!children;
  return (
    <div>
      <div style={{
        padding: '6px 10px', borderRadius: 7, display: 'flex', alignItems: 'center', gap: 7,
        background: active ? theme.gradSoft : 'transparent', cursor: 'pointer',
        fontSize: 12.5, fontWeight: active ? 600 : 500,
        color: active ? theme.accent : theme.ink2,
        position: 'relative',
      }}>
        {hasChildren && (
          <Icons.chevRight size={9} style={{
            color: theme.ink4,
            transform: open ? 'rotate(90deg)' : 'rotate(0)',
            transition: 'transform .15s',
          }}/>
        )}
        {!hasChildren && <div style={{ width: 9 }}/>}
        <Ico size={13} stroke={active ? theme.accent : theme.ink3}/>
        <span style={{ flex: 1, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>{label}</span>
        {count && <span style={{ fontSize: 10, color: theme.ink4, fontFamily: theme.mono }}>{count}</span>}
      </div>
      {hasChildren && open && (
        <div style={{ marginLeft: 14, paddingLeft: 8, borderLeft: `1px dashed ${theme.border}` }}>
          {children}
        </div>
      )}
    </div>
  );
}

function Crumb({ theme, children, icon, active }) {
  const Ico = icon;
  return (
    <div style={{
      padding: '5px 10px', borderRadius: 6, display: 'flex', alignItems: 'center', gap: 5,
      background: active ? theme.gradSoft : 'transparent', cursor: 'pointer',
      fontWeight: active ? 600 : 500, color: active ? theme.accent : theme.ink2, fontSize: 12.5,
    }}>
      {Ico && <Ico size={12}/>}
      {children}
    </div>
  );
}

function ViewBtn({ theme, active, children }) {
  return (
    <div style={{
      padding: '5px 8px', borderRadius: 6, cursor: 'pointer',
      background: active ? theme.panel : 'transparent',
      color: active ? theme.ink : theme.ink4,
      boxShadow: active ? '0 1px 2px rgba(0,0,0,.06)' : 'none',
    }}>{children}</div>
  );
}

function FolderTile({ theme, name, count, size, selected, shared, starred }) {
  return (
    <div style={{
      position: 'relative', padding: '14px 16px 14px 14px',
      background: selected ? theme.gradSoft : theme.panel,
      border: `1.5px solid ${selected ? theme.accent : theme.border}`,
      borderRadius: 12, cursor: 'pointer', display: 'flex', alignItems: 'center', gap: 11,
    }}>
      {/* checkbox (hover state shown for selected) */}
      {selected && (
        <div style={{
          position: 'absolute', top: 8, right: 8, width: 18, height: 18,
          borderRadius: 5, background: theme.accent, color: '#fff',
          display: 'flex', alignItems: 'center', justifyContent: 'center',
        }}><Icons.check size={11} sw={3}/></div>
      )}
      <div style={{
        width: 34, height: 34, borderRadius: 9, background: theme.gradSoft,
        display: 'flex', alignItems: 'center', justifyContent: 'center', color: theme.accent,
        flexShrink: 0,
      }}><Icons.folder size={17}/></div>
      <div style={{ flex: 1, minWidth: 0 }}>
        <div style={{ fontSize: 12.5, fontWeight: 600, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis', display: 'flex', alignItems: 'center', gap: 4 }}>
          {name}
          {starred && <Icons.star size={10} fill={theme.accent2} stroke={theme.accent2}/>}
        </div>
        <div style={{ fontSize: 10.5, color: theme.ink4, fontFamily: theme.mono, marginTop: 2, display: 'flex', gap: 6, alignItems: 'center' }}>
          {count} · {size}
          {shared && <Icons.users size={10}/>}
        </div>
      </div>
    </div>
  );
}

function DataRow({ theme, sel, n, s, ty, t, i, tag, shared, star, last }) {
  const Ico = i;
  return (
    <div style={{
      display: 'grid', gridTemplateColumns: '28px 1fr 100px 90px 130px 70px',
      padding: '10px 16px', borderBottom: last ? 'none' : `1px solid ${theme.border}`,
      alignItems: 'center', background: sel ? theme.gradSoft : 'transparent',
      cursor: 'pointer', fontSize: 13,
    }}>
      <div style={{
        width: 16, height: 16, borderRadius: 4,
        background: sel ? theme.accent : 'transparent',
        border: `1.5px solid ${sel ? theme.accent : theme.border}`,
        display: 'flex', alignItems: 'center', justifyContent: 'center',
        color: '#fff',
      }}>{sel && <Icons.check size={10} sw={3}/>}</div>

      <div style={{ display: 'flex', alignItems: 'center', gap: 10, minWidth: 0 }}>
        <div style={{
          width: 28, height: 28, borderRadius: 7, background: theme.gradSoft,
          display: 'flex', alignItems: 'center', justifyContent: 'center', color: theme.accent, flexShrink: 0,
        }}><Ico size={14}/></div>
        <span style={{ fontWeight: 500, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>{n}</span>
        {tag && <span style={{ fontSize: 9.5, padding: '2px 6px', background: theme.grad, color: '#fff', borderRadius: 3, fontWeight: 700, fontFamily: theme.mono, letterSpacing: '.04em' }}>{tag}</span>}
        {shared && <Icons.users size={12} style={{ color: theme.ink4, flexShrink: 0 }}/>}
        {star && <Icons.star size={12} fill={theme.accent2} stroke={theme.accent2} style={{ flexShrink: 0 }}/>}
      </div>

      <div style={{ fontSize: 12, fontFamily: theme.mono, color: theme.ink3, textAlign: 'right' }}>{s}</div>
      <div style={{ textAlign: 'center' }}>
        <span style={{ fontSize: 10, padding: '2px 7px', background: theme.bg, color: theme.ink3, borderRadius: 4, fontFamily: theme.mono, fontWeight: 600, letterSpacing: '.04em' }}>{ty}</span>
      </div>
      <div style={{ fontSize: 11.5, color: theme.ink4 }}>{t}</div>
      <div style={{ display: 'flex', gap: 10, color: theme.ink3, justifyContent: 'flex-end' }}>
        <Icons.download size={14}/>
        <Icons.share size={14}/>
        <Icons.more size={14}/>
      </div>
    </div>
  );
}

function DetailRow({ theme, label, val }) {
  return (
    <div style={{ display: 'flex', padding: '7px 0', fontSize: 12, borderBottom: `1px dashed ${theme.border}` }}>
      <span style={{ color: theme.ink4, width: 100, fontFamily: theme.mono, fontSize: 11 }}>{label}</span>
      <span style={{ flex: 1, fontWeight: 500, color: theme.ink2, textAlign: 'right', fontFamily: theme.mono }}>{val}</span>
    </div>
  );
}

Object.assign(window, { AuroraFiles });
