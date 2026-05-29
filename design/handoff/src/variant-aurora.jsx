// variant-aurora.jsx — Variant A: Aurora (warm gradient, modern bold)
// Supports LIGHT + DARK mode via ThemeContext. All screens adapt.

const T_A_LIGHT = {
  mode: 'light',
  bg: '#F5F4F1', panel: '#FFFFFF', sidebar: '#0E0E12',
  sidebarInk: '#EDEDEF', sidebarInkMut: '#8A8A94',
  border: '#E6E3DC', borderStrong: '#D4D0C7',
  ink: '#0E0E12', ink2: '#2A2A30', ink3: '#5C5C66', ink4: '#8A8A94',
  accent: '#FF5B2E', accent2: '#FFAF1D', accent3: '#FF3D7F',
  grad: 'linear-gradient(135deg, #FF3D7F 0%, #FF5B2E 50%, #FFAF1D 100%)',
  gradSoft: 'linear-gradient(135deg, #FFE8E0 0%, #FFF3DA 100%)',
  softInk: '#FF5B2E',          // text color on gradSoft chips
  rowHover: 'rgba(0,0,0,.03)',
  success: '#0A8A5C', warn: '#C96A00', danger: '#D53030',
  dangerSoft: '#FFE8E0',
  chipBg: '#FFE8E0', chipInk: '#FF5B2E',
  shadow: '0 4px 14px rgba(255,91,46,.3)',
  font: "'Be Vietnam Pro', system-ui, sans-serif",
  serif: "'Instrument Serif', serif",
  mono: "'Geist Mono', monospace",
};

const T_A_DARK = {
  mode: 'dark',
  bg: '#0E0E12', panel: '#17171C', sidebar: '#050507',
  sidebarInk: '#EDEDEF', sidebarInkMut: '#7A7A85',
  border: '#25252E', borderStrong: '#32323D',
  ink: '#F5F4F1', ink2: '#D9D8D3', ink3: '#A0A0AC', ink4: '#6E6E7A',
  accent: '#FF7A52', accent2: '#FFBF45', accent3: '#FF5A97',
  grad: 'linear-gradient(135deg, #FF5A97 0%, #FF7A52 50%, #FFBF45 100%)',
  gradSoft: 'linear-gradient(135deg, rgba(255,91,46,.18) 0%, rgba(255,175,29,.14) 100%)',
  softInk: '#FFAA7A',
  rowHover: 'rgba(255,255,255,.03)',
  success: '#34D399', warn: '#FBBF24', danger: '#F87171',
  dangerSoft: 'rgba(248,113,113,.15)',
  chipBg: 'rgba(255,122,82,.18)', chipInk: '#FFAA7A',
  shadow: '0 4px 20px rgba(255,91,46,.4)',
  font: "'Be Vietnam Pro', system-ui, sans-serif",
  serif: "'Instrument Serif', serif",
  mono: "'Geist Mono', monospace",
};

// Theme context — any screen can read the active theme via useTA().
const ThemeCtx = React.createContext(T_A_LIGHT);
const useTA = () => React.useContext(ThemeCtx);

// Wrapper to set a mode for a subtree
function Themed({ mode, children }) {
  const t = mode === 'dark' ? T_A_DARK : T_A_LIGHT;
  return <ThemeCtx.Provider value={t}>{children}</ThemeCtx.Provider>;
}

// Legacy reference — some places may still use T_A directly; default to light.
const T_A = T_A_LIGHT;

// Windows 11 window chrome
function WinChrome({ title, children, theme }) {
  const ctx = useTA();
  theme = theme || ctx;
  return (
    <div style={{
      width: '100%', height: '100%', background: theme.bg,
      fontFamily: theme.font, color: theme.ink,
      display: 'flex', flexDirection: 'column', overflow: 'hidden',
      borderRadius: 10, boxShadow: '0 24px 80px rgba(0,0,0,.35)',
    }}>
      {/* title bar */}
      <div style={{
        height: 36, display: 'flex', alignItems: 'center',
        background: theme.sidebar, color: theme.sidebarInk,
        paddingLeft: 16, fontSize: 12, fontWeight: 500,
        borderBottom: `1px solid ${theme.mode === 'dark' ? 'rgba(255,255,255,.04)' : '#000'}`,
      }}>
        <FshareLogo size={16} />
        <span style={{ marginLeft: 10, letterSpacing: '-.005em' }}>{title}</span>
        <div style={{ marginLeft: 'auto', display: 'flex', height: '100%' }}>
          <WinBtn><Icons.minWin stroke={theme.sidebarInk}/></WinBtn>
          <WinBtn><Icons.maxWin stroke={theme.sidebarInk}/></WinBtn>
          <WinBtn close><Icons.closeWin stroke={theme.sidebarInk}/></WinBtn>
        </div>
      </div>
      <div style={{ flex: 1, display: 'flex', overflow: 'hidden' }}>{children}</div>
    </div>
  );
}

function WinBtn({ children, close }) {
  return (
    <div style={{
      width: 46, height: 36, display: 'flex', alignItems: 'center', justifyContent: 'center',
      cursor: 'pointer', transition: 'background .12s',
    }} onMouseEnter={e => e.currentTarget.style.background = close ? '#E81123' : 'rgba(255,255,255,.1)'}
      onMouseLeave={e => e.currentTarget.style.background = 'transparent'}>
      {children}
    </div>
  );
}

// Sidebar item — supports collapsed (icon-only) state
function SideItem({ icon, label, active, badge, theme, sub, collapsed, onClick }) {
  const Ico = icon;
  if (collapsed) {
    return (
      <div
        onClick={onClick}
        title={label}
        style={{
          width: 40, height: 40, borderRadius: 10, margin: '2px auto',
          display: 'flex', alignItems: 'center', justifyContent: 'center',
          background: active ? 'rgba(255,255,255,.12)' : 'transparent',
          color: active ? theme.sidebarInk : theme.sidebarInkMut,
          cursor: 'pointer', position: 'relative',
          border: active ? `1px solid rgba(255,255,255,.1)` : '1px solid transparent',
        }}
      >
        {active && <div style={{ position: 'absolute', left: -9, top: 9, bottom: 9, width: 3, background: theme.accent, borderRadius: 3 }}/>}
        <Ico size={17}/>
        {badge && (
          <div style={{
            position: 'absolute', top: 4, right: 4, minWidth: 14, height: 14,
            padding: '0 4px', borderRadius: 999, background: theme.accent,
            color: '#fff', fontSize: 9, fontWeight: 700,
            display: 'flex', alignItems: 'center', justifyContent: 'center',
          }}>{badge}</div>
        )}
      </div>
    );
  }
  return (
    <div
      onClick={onClick}
      style={{
        padding: '9px 14px', borderRadius: 8, display: 'flex', alignItems: 'center', gap: 11,
        background: active ? 'rgba(255,255,255,.08)' : 'transparent',
        color: active ? theme.sidebarInk : theme.sidebarInkMut,
        fontSize: 13, fontWeight: active ? 600 : 500, cursor: 'pointer',
        position: 'relative',
      }}
    >
      {active && <div style={{ position: 'absolute', left: -10, top: 8, bottom: 8, width: 3, background: theme.accent, borderRadius: 3 }}/>}
      <Ico size={16}/>
      <span>{label}</span>
      {badge && <span style={{
        marginLeft: 'auto', fontSize: 10, fontWeight: 600, padding: '2px 7px',
        borderRadius: 999, background: theme.accent, color: '#fff',
      }}>{badge}</span>}
      {sub && !badge && <span style={{ marginLeft: 'auto', fontSize: 10, color: theme.sidebarInkMut, fontFamily: theme.mono }}>{sub}</span>}
    </div>
  );
}

function AuroraSidebar({ active = 'home', collapsed = false, onToggle }) {
  const t = useTA();
  if (collapsed) {
    return (
      <div style={{
        width: 60, background: t.sidebar, color: t.sidebarInk,
        display: 'flex', flexDirection: 'column', padding: '14px 0',
        borderRight: '1px solid rgba(255,255,255,.06)', alignItems: 'center',
      }}>
        {/* toggle expand */}
        <div
          onClick={onToggle}
          title="Mở rộng menu"
          style={{
            width: 40, height: 40, borderRadius: 10, marginBottom: 8,
            display: 'flex', alignItems: 'center', justifyContent: 'center',
            color: t.sidebarInk, cursor: 'pointer', background: 'rgba(255,255,255,.04)',
            border: '1px solid rgba(255,255,255,.06)',
          }}
        >
          <Icons.menu size={17}/>
        </div>

        {/* avatar pill */}
        <div style={{
          width: 40, height: 40, borderRadius: 12, background: t.grad,
          display: 'flex', alignItems: 'center', justifyContent: 'center',
          color: '#fff', fontWeight: 700, fontSize: 15, marginBottom: 14,
          position: 'relative', boxShadow: t.shadow,
        }}>
          M
          <div style={{
            position: 'absolute', bottom: -3, right: -3, width: 14, height: 14, borderRadius: '50%',
            background: t.accent2, border: `2px solid ${t.sidebar}`,
            display: 'flex', alignItems: 'center', justifyContent: 'center',
          }}>
            <Icons.crown size={7} stroke={t.sidebar}/>
          </div>
        </div>

        <div style={{ width: 28, height: 1, background: 'rgba(255,255,255,.08)', marginBottom: 8 }}/>

        <SideItem icon={Icons.home} label="Trang chủ" active={active === 'home'} theme={t} collapsed/>
        <SideItem icon={Icons.folder} label="File của tôi" active={active === 'files'} theme={t} collapsed/>
        <SideItem icon={Icons.clock} label="Gần đây" theme={t} active={active === 'recent'} collapsed/>
        <SideItem icon={Icons.star} label="Đã gắn sao" theme={t} collapsed/>
        <SideItem icon={Icons.share} label="Đã chia sẻ" active={active === 'shared'} theme={t} collapsed/>
        <SideItem icon={Icons.trash} label="Thùng rác" theme={t} collapsed/>

        <div style={{ width: 28, height: 1, background: 'rgba(255,255,255,.08)', margin: '8px 0' }}/>

        <SideItem icon={Icons.arrowDown} label="Tải xuống" active={active === 'downloads'} theme={t} badge="7" collapsed/>
        <SideItem icon={Icons.arrowUp} label="Tải lên" theme={t} collapsed/>
        <SideItem icon={Icons.sync} label="Đồng bộ" active={active === 'sync'} theme={t} collapsed/>

        <div style={{ marginTop: 'auto', display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 4 }}>
          <div
            title="Nâng cấp VIP Pro"
            style={{
              width: 40, height: 40, borderRadius: 12,
              background: 'linear-gradient(135deg, #FFAF1D, #FF5B2E)',
              display: 'flex', alignItems: 'center', justifyContent: 'center',
              color: '#fff', cursor: 'pointer',
              boxShadow: '0 4px 14px rgba(255,175,29,.35)',
            }}
          >
            <Icons.crown size={17} stroke="#fff"/>
          </div>
          <SideItem icon={Icons.settings} label="Cài đặt" theme={t} collapsed/>
        </div>
      </div>
    );
  }

  return (
    <div style={{
      width: 240, background: t.sidebar, color: t.sidebarInk,
      display: 'flex', flexDirection: 'column', padding: '18px 14px',
      borderRight: '1px solid rgba(255,255,255,.06)',
    }}>
      {/* collapse button */}
      {onToggle && (
        <div style={{ display: 'flex', justifyContent: 'flex-end', marginBottom: 8 }}>
          <div
            onClick={onToggle}
            title="Thu gọn menu"
            style={{
              width: 28, height: 28, borderRadius: 7, cursor: 'pointer',
              display: 'flex', alignItems: 'center', justifyContent: 'center',
              color: t.sidebarInkMut, background: 'rgba(255,255,255,.04)',
            }}
          >
            <Icons.menu size={14}/>
          </div>
        </div>
      )}
      {/* account card */}
      <div style={{
        background: t.grad, borderRadius: 12, padding: '14px 14px 12px',
        position: 'relative', overflow: 'hidden',
      }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
          <div style={{
            width: 34, height: 34, borderRadius: 10, background: 'rgba(255,255,255,.2)',
            display: 'flex', alignItems: 'center', justifyContent: 'center',
            fontWeight: 700, fontSize: 14, border: '1.5px solid rgba(255,255,255,.3)',
          }}>M</div>
          <div style={{ flex: 1, minWidth: 0 }}>
            <div style={{ fontSize: 12.5, fontWeight: 600, color: '#fff', whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>Nguyễn Minh</div>
            <div style={{ fontSize: 10, color: 'rgba(255,255,255,.8)', fontFamily: t.mono, letterSpacing: '.04em', display: 'flex', alignItems: 'center', gap: 5 }}>
              <Icons.crown size={10} stroke="#fff"/> VIP · 243 ngày
            </div>
          </div>
        </div>
        <div style={{ marginTop: 12, fontSize: 10, color: 'rgba(255,255,255,.85)', fontFamily: t.mono, display: 'flex', justifyContent: 'space-between', marginBottom: 5 }}>
          <span>287 GB</span><span>/ 500 GB</span>
        </div>
        <div style={{ height: 4, background: 'rgba(0,0,0,.2)', borderRadius: 4, overflow: 'hidden' }}>
          <div style={{ width: '57%', height: '100%', background: 'rgba(255,255,255,.9)' }}/>
        </div>
      </div>

      {/* nav */}
      <div style={{ marginTop: 20, display: 'flex', flexDirection: 'column', gap: 2 }}>
        <SideLabel theme={t}>Không gian</SideLabel>
        <SideItem icon={Icons.home} label="Trang chủ" active={active === 'home'} theme={t}/>
        <SideItem icon={Icons.folder} label="File của tôi" active={active === 'files'} theme={t} sub="2.1K"/>
        <SideItem icon={Icons.clock} label="Gần đây" theme={t} active={active === 'recent'}/>
        <SideItem icon={Icons.star} label="Đã gắn sao" theme={t}/>
        <SideItem icon={Icons.share} label="Đã chia sẻ" active={active === 'shared'} theme={t} sub="14"/>
        <SideItem icon={Icons.trash} label="Thùng rác" theme={t}/>

        <SideLabel theme={t} style={{marginTop: 18}}>Hoạt động</SideLabel>
        <SideItem icon={Icons.arrowDown} label="Tải xuống" active={active === 'downloads'} theme={t} badge="7"/>
        <SideItem icon={Icons.arrowUp} label="Tải lên" theme={t}/>
        <SideItem icon={Icons.sync} label="Đồng bộ" active={active === 'sync'} theme={t} sub="ON"/>
      </div>

      {/* bottom — upgrade */}
      <div style={{ marginTop: 'auto', padding: '14px 14px', background: 'rgba(255,255,255,.04)', borderRadius: 12, border: '1px solid rgba(255,255,255,.06)' }}>
        <div style={{ fontSize: 11, fontFamily: t.mono, color: t.accent2, letterSpacing: '.1em', textTransform: 'uppercase', fontWeight: 600 }}>Giảm 40%</div>
        <div style={{ fontSize: 13, fontWeight: 600, marginTop: 4, color: '#fff' }}>Nâng cấp VIP Pro</div>
        <div style={{ fontSize: 11, color: t.sidebarInkMut, marginTop: 2 }}>1TB · tốc độ không giới hạn</div>
        <button style={{
          marginTop: 10, width: '100%', padding: '7px 10px', background: t.grad,
          color: '#fff', border: 'none', borderRadius: 8, fontSize: 12, fontWeight: 600,
          cursor: 'pointer', fontFamily: t.font,
        }}>Xem ưu đãi</button>
      </div>
    </div>
  );
}

function SideLabel({ children, theme, style }) {
  return (
    <div style={{
      fontSize: 10, fontFamily: theme.mono, color: theme.sidebarInkMut,
      letterSpacing: '.18em', textTransform: 'uppercase', fontWeight: 600,
      padding: '0 14px 6px', ...style,
    }}>{children}</div>
  );
}

// ═══════════════ SCREEN 1: HOME / BROWSE ═══════════════
function AuroraHome() {
  const t = useTA();
  return (
    <WinChrome title="Fshare" theme={t}>
      <AuroraSidebar active="home"/>
      <div style={{ flex: 1, overflow: 'auto', background: t.bg }}>
        {/* top bar */}
        <div style={{
          display: 'flex', alignItems: 'center', gap: 16, padding: '14px 28px',
          borderBottom: `1px solid ${t.border}`, background: t.panel,
        }}>
          <div style={{
            flex: 1, maxWidth: 440, display: 'flex', alignItems: 'center', gap: 10,
            padding: '8px 14px', background: t.bg, borderRadius: 10, border: `1px solid ${t.border}`,
          }}>
            <Icons.search size={15} stroke={t.ink3}/>
            <input placeholder="Tìm file, folder, hoặc dán link Fshare..."
              style={{ flex: 1, border: 'none', background: 'transparent', outline: 'none', fontSize: 13, fontFamily: t.font, color: t.ink }}/>
            <span style={{ fontSize: 10, fontFamily: t.mono, color: t.ink4, padding: '2px 6px', border: `1px solid ${t.border}`, borderRadius: 4 }}>⌘K</span>
          </div>
          <div style={{ marginLeft: 'auto', display: 'flex', alignItems: 'center', gap: 10 }}>
            <IconBtn theme={t}><Icons.bell size={16}/></IconBtn>
            <IconBtn theme={t}><Icons.settings size={16}/></IconBtn>
            <button style={{
              padding: '8px 16px', background: t.grad, color: '#fff', border: 'none',
              borderRadius: 10, fontSize: 13, fontWeight: 600, cursor: 'pointer',
              display: 'flex', alignItems: 'center', gap: 6, fontFamily: t.font,
              boxShadow: '0 4px 14px rgba(255,91,46,.3)',
            }}><Icons.upload size={14}/> Tải lên</button>
          </div>
        </div>

        {/* hero */}
        <div style={{ padding: '32px 28px 20px' }}>
          <div style={{ fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', marginBottom: 8 }}>
            Chiều thứ hai · 18:42
          </div>
          <h1 style={{ fontFamily: t.serif, fontSize: 52, fontWeight: 400, letterSpacing: '-.03em', margin: 0, lineHeight: 1 }}>
            Chào Minh, <span style={{ fontStyle: 'italic', background: t.grad, WebkitBackgroundClip: 'text', WebkitTextFillColor: 'transparent' }}>tiếp tục?</span>
          </h1>

          {/* quick actions */}
          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: 12, marginTop: 28 }}>
            <QuickCard theme={t} icon={Icons.link} title="Dán link & tải" sub="Paste link Fshare"/>
            <QuickCard theme={t} icon={Icons.upload} title="Tải file lên" sub="Kéo-thả hoặc chọn"/>
            <QuickCard theme={t} icon={Icons.sync} title="Thêm folder sync" sub="2-way đồng bộ"/>
            <QuickCard theme={t} icon={Icons.share} title="Tạo link chia sẻ" sub="Password + hết hạn"/>
          </div>
        </div>

        {/* continue section */}
        <div style={{ padding: '20px 28px 12px', display: 'flex', alignItems: 'baseline', justifyContent: 'space-between' }}>
          <h2 style={{ fontSize: 18, fontWeight: 700, margin: 0, letterSpacing: '-.01em' }}>Tiếp tục đang tải</h2>
          <span style={{ fontSize: 12, color: t.ink4, fontFamily: t.mono }}>3 đang hoạt động · 1.2 GB/s tổng</span>
        </div>

        <div style={{ padding: '0 28px', display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: 14 }}>
          <DownloadingCard theme={t} name="Spider-Man_BTSV_2023_1080p.mkv" pct={73} size="4.2 GB" speed="47.3 MB/s" eta="18 giây" type="video"/>
          <DownloadingCard theme={t} name="PTS_CC_2025_Full.rar" pct={31} size="8.1 GB" speed="22.8 MB/s" eta="4 phút" type="archive"/>
          <DownloadingCard theme={t} name="Playlist_Vpop_2026.zip" pct={91} size="820 MB" speed="18.2 MB/s" eta="4 giây" type="music"/>
        </div>

        {/* recent */}
        <div style={{ padding: '28px 28px 12px', display: 'flex', alignItems: 'baseline', justifyContent: 'space-between' }}>
          <h2 style={{ fontSize: 18, fontWeight: 700, margin: 0, letterSpacing: '-.01em' }}>File gần đây</h2>
          <div style={{ display: 'flex', gap: 4 }}>
            <Tab theme={t} active>Tất cả</Tab>
            <Tab theme={t}>Video</Tab>
            <Tab theme={t}>Tài liệu</Tab>
            <Tab theme={t}>Ảnh</Tab>
          </div>
        </div>

        <div style={{ padding: '0 28px 32px' }}>
          <div style={{ background: t.panel, borderRadius: 14, border: `1px solid ${t.border}`, overflow: 'hidden' }}>
            <FileRow theme={t} icon={Icons.video} name="Avatar 2 - The Way of Water.mp4" size="12.4 GB" time="2 giờ trước" iconBg="#FFE8E0" iconColor={t.accent3}/>
            <FileRow theme={t} icon={Icons.folder} name="Tài liệu khoá 2025-2026" size="34 mục" time="Hôm qua" iconBg="#FFF3DA" iconColor={t.warn} folder/>
            <FileRow theme={t} icon={Icons.archive} name="Backup_Photos_2024.rar" size="8.1 GB" time="2 ngày trước" iconBg={t.gradSoft} iconColor={t.accent}/>
            <FileRow theme={t} icon={Icons.doc} name="Đề án nghỉ dưỡng Q4.pdf" size="2.4 MB" time="3 ngày trước" iconBg="#E6F0FF" iconColor="#2C6EF2" shared/>
            <FileRow theme={t} icon={Icons.music} name="Dua Lipa - Radical Optimism.zip" size="420 MB" time="Tuần trước" iconBg="#E7F1EC" iconColor={t.success} last/>
          </div>
        </div>
      </div>
    </WinChrome>
  );
}

function IconBtn({ children, theme }) {
  return (
    <button style={{
      width: 34, height: 34, background: 'transparent', border: `1px solid ${theme.border}`,
      borderRadius: 8, cursor: 'pointer', display: 'flex', alignItems: 'center', justifyContent: 'center',
      color: theme.ink2,
    }}>{children}</button>
  );
}

function Tab({ children, active, theme }) {
  return (
    <div style={{
      padding: '6px 12px', fontSize: 12, fontWeight: active ? 600 : 500,
      color: active ? theme.ink : theme.ink4, cursor: 'pointer',
      borderBottom: active ? `2px solid ${theme.accent}` : '2px solid transparent',
    }}>{children}</div>
  );
}

function QuickCard({ theme, icon, title, sub }) {
  const Ico = icon;
  return (
    <div style={{
      background: theme.panel, border: `1px solid ${theme.border}`, borderRadius: 14,
      padding: '18px 18px 20px', cursor: 'pointer', position: 'relative',
      transition: 'transform .2s, box-shadow .2s',
    }}>
      <div style={{
        width: 38, height: 38, borderRadius: 10, background: theme.gradSoft,
        display: 'flex', alignItems: 'center', justifyContent: 'center', color: theme.accent,
        marginBottom: 14,
      }}><Ico size={18}/></div>
      <div style={{ fontSize: 14, fontWeight: 600, marginBottom: 2 }}>{title}</div>
      <div style={{ fontSize: 11.5, color: theme.ink3 }}>{sub}</div>
      <Icons.chevRight size={14} style={{ position: 'absolute', top: 18, right: 18, color: theme.ink4 }}/>
    </div>
  );
}

function DownloadingCard({ theme, name, pct, size, speed, eta, type }) {
  const iconMap = { video: Icons.video, archive: Icons.archive, music: Icons.music };
  const Ico = iconMap[type] || Icons.file;
  return (
    <div style={{
      background: theme.panel, border: `1px solid ${theme.border}`,
      borderRadius: 14, padding: '16px 16px 14px', overflow: 'hidden',
    }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: 10, marginBottom: 12 }}>
        <div style={{
          width: 34, height: 34, borderRadius: 8, background: theme.gradSoft,
          display: 'flex', alignItems: 'center', justifyContent: 'center', color: theme.accent,
        }}><Ico size={16}/></div>
        <div style={{ flex: 1, minWidth: 0 }}>
          <div style={{ fontSize: 12.5, fontWeight: 600, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>{name}</div>
          <div style={{ fontSize: 10.5, color: theme.ink4, fontFamily: theme.mono, marginTop: 2 }}>{size} · còn {eta}</div>
        </div>
        <Icons.pause size={14} style={{ color: theme.ink3 }}/>
      </div>
      <div style={{
        height: 4, background: theme.border, borderRadius: 4, overflow: 'hidden', position: 'relative',
      }}>
        <div style={{
          position: 'absolute', inset: 0, width: `${pct}%`,
          background: theme.grad, borderRadius: 4,
        }}/>
      </div>
      <div style={{ display: 'flex', justifyContent: 'space-between', marginTop: 8, fontSize: 11, fontFamily: theme.mono }}>
        <span style={{ color: theme.accent, fontWeight: 600 }}>{speed}</span>
        <span style={{ color: theme.ink3 }}>{pct}%</span>
      </div>
    </div>
  );
}

function FileRow({ theme, icon, name, size, time, iconBg, iconColor, shared, folder, last }) {
  const Ico = icon;
  return (
    <div style={{
      display: 'flex', alignItems: 'center', gap: 14, padding: '12px 18px',
      borderBottom: last ? 'none' : `1px solid ${theme.border}`,
      cursor: 'pointer', transition: 'background .12s',
    }}>
      <div style={{
        width: 36, height: 36, borderRadius: 10, background: iconBg,
        display: 'flex', alignItems: 'center', justifyContent: 'center', color: iconColor,
      }}><Ico size={17}/></div>
      <div style={{ flex: 1, minWidth: 0 }}>
        <div style={{ fontSize: 13.5, fontWeight: 500, display: 'flex', alignItems: 'center', gap: 8 }}>
          {name}
          {shared && <span style={{ fontSize: 10, padding: '2px 6px', background: theme.gradSoft, color: theme.accent, borderRadius: 4, fontWeight: 600 }}>Đã chia sẻ</span>}
          {folder && <Icons.chevRight size={12} style={{ color: theme.ink4 }}/>}
        </div>
        <div style={{ fontSize: 11, color: theme.ink4, fontFamily: theme.mono, marginTop: 2 }}>{size} · {time}</div>
      </div>
      <div style={{ display: 'flex', gap: 14, color: theme.ink3 }}>
        <Icons.download size={15}/>
        <Icons.share size={15}/>
        <Icons.more size={15}/>
      </div>
    </div>
  );
}

// ═══════════════ SCREEN 2: DOWNLOAD MANAGER ═══════════════
function AuroraDownloads() {
  const t = useTA();
  return (
    <WinChrome title="Fshare · Tải xuống" theme={t}>
      <AuroraSidebar active="downloads"/>
      <div style={{ flex: 1, overflow: 'auto', background: t.bg }}>
        {/* top summary */}
        <div style={{
          background: t.panel, padding: '28px 28px 24px',
          borderBottom: `1px solid ${t.border}`,
        }}>
          <div style={{ display: 'flex', alignItems: 'baseline', gap: 20 }}>
            <div>
              <div style={{ fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase' }}>Đang tải · 3 file</div>
              <div style={{ fontFamily: t.serif, fontSize: 60, fontWeight: 400, letterSpacing: '-.03em', lineHeight: 1, marginTop: 4 }}>
                88.3 <span style={{ fontSize: 28, fontStyle: 'italic', color: t.ink3 }}>MB/s</span>
              </div>
            </div>
            <div style={{ marginLeft: 'auto', display: 'flex', gap: 28 }}>
              <MiniStat label="Đã tải" value="4 GB" mono={t.mono}/>
              <MiniStat label="Còn lại" value="7.3 GB" mono={t.mono}/>
              <MiniStat label="ETA" value="2m 18s" mono={t.mono}/>
              <MiniStat label="Luồng" value="6 / 8" mono={t.mono}/>
            </div>
            <div style={{ display: 'flex', gap: 8 }}>
              <button style={btnGhost(t)}><Icons.pause size={13}/> Tạm dừng tất cả</button>
              <button style={btnSolid(t)}><Icons.plus size={13}/> Thêm URL</button>
            </div>
          </div>

          {/* speed graph */}
          <div style={{ marginTop: 20, height: 60, background: t.bg, borderRadius: 10, padding: 8, position: 'relative' }}>
            <SpeedGraph theme={t}/>
          </div>
        </div>

        {/* active downloads */}
        <div style={{ padding: '24px 28px 16px', fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600 }}>
          Đang hoạt động
        </div>

        <div style={{ padding: '0 28px', display: 'flex', flexDirection: 'column', gap: 10 }}>
          <DLRowActive theme={t} name="Spider-Man_Across_the_Spider-Verse_2023_1080p_VietSub.mkv" folder="Phim 2023" pct={73} size="4.2 GB" done="3.1 GB" speed="47.3 MB/s" eta="18 giây" threads={4}/>
          <DLRowActive theme={t} name="Photoshop_2025_Full_Crack.rar" folder="Phần mềm" pct={31} size="8.1 GB" done="2.5 GB" speed="22.8 MB/s" eta="4 phút 12 giây" threads={2}/>
          <DLRowActive theme={t} name="Playlist_Vpop_2026_FLAC.zip" folder="Nhạc" pct={91} size="820 MB" done="745 MB" speed="18.2 MB/s" eta="4 giây" threads={2}/>
        </div>

        <div style={{ padding: '28px 28px 16px', fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600, display: 'flex', justifyContent: 'space-between' }}>
          <span>Hoàn tất hôm nay · 6</span>
          <a style={{ color: t.accent, fontWeight: 600, textTransform: 'none', letterSpacing: 0 }}>Xem tất cả →</a>
        </div>

        <div style={{ padding: '0 28px 32px', display: 'flex', flexDirection: 'column', gap: 8 }}>
          <DLRowDone theme={t} name="Chi Pu - Dancing Monkey.mp3" size="18.4 MB" time="15:32" iconBg="#E7F1EC" iconColor={t.success}/>
          <DLRowDone theme={t} name="CV_NguyenMinh_2026.pdf" size="2.1 MB" time="14:18" iconBg="#E6F0FF" iconColor="#2C6EF2"/>
          <DLRowDone theme={t} name="Tailwind_Config_Snippets.zip" size="44 KB" time="11:02" iconBg="#F3E8FF" iconColor="#8B5CF6"/>
        </div>
      </div>
    </WinChrome>
  );
}

function MiniStat({ label, value, mono }) {
  const t = useTA();
  return (
    <div>
      <div style={{ fontSize: 10, fontFamily: mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600 }}>{label}</div>
      <div style={{ fontSize: 18, fontWeight: 600, marginTop: 2, fontFamily: mono, color: t.ink }}>{value}</div>
    </div>
  );
}

function btnGhost(t) {
  return {
    padding: '9px 14px', background: t.panel, color: t.ink, border: `1px solid ${t.border}`,
    borderRadius: 10, fontSize: 12.5, fontWeight: 500, cursor: 'pointer',
    display: 'flex', alignItems: 'center', gap: 6, fontFamily: t.font,
  };
}
function btnSolid(t) {
  return {
    padding: '9px 14px', background: t.ink, color: '#fff', border: 'none',
    borderRadius: 10, fontSize: 12.5, fontWeight: 600, cursor: 'pointer',
    display: 'flex', alignItems: 'center', gap: 6, fontFamily: t.font,
  };
}

function SpeedGraph({ theme }) {
  const vals = [14, 28, 22, 36, 44, 58, 52, 66, 74, 68, 82, 76, 88, 84, 92, 88, 74, 68, 82, 88];
  return (
    <svg viewBox="0 0 400 44" preserveAspectRatio="none" style={{ width: '100%', height: '100%' }}>
      <defs>
        <linearGradient id="sgrad" x1="0" x2="0" y1="0" y2="1">
          <stop offset="0" stopColor="#FF5B2E" stopOpacity=".3"/>
          <stop offset="1" stopColor="#FF5B2E" stopOpacity="0"/>
        </linearGradient>
      </defs>
      <path
        d={`M 0 44 ${vals.map((v, i) => `L ${i * (400/(vals.length-1))} ${44 - v*0.4}`).join(' ')} L 400 44 Z`}
        fill="url(#sgrad)"/>
      <path
        d={`M 0 ${44 - vals[0]*0.4} ${vals.map((v, i) => `L ${i * (400/(vals.length-1))} ${44 - v*0.4}`).join(' ')}`}
        fill="none" stroke={theme.accent} strokeWidth="1.5"/>
    </svg>
  );
}

function DLRowActive({ theme, name, folder, pct, size, done, speed, eta, threads }) {
  return (
    <div style={{
      background: theme.panel, border: `1px solid ${theme.border}`,
      borderRadius: 14, padding: '14px 18px', display: 'flex', alignItems: 'center', gap: 16,
    }}>
      <div style={{
        width: 44, height: 44, borderRadius: 10, background: theme.gradSoft, flexShrink: 0,
        display: 'flex', alignItems: 'center', justifyContent: 'center', color: theme.accent,
      }}><Icons.video size={20}/></div>
      <div style={{ flex: 1, minWidth: 0 }}>
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'baseline' }}>
          <div style={{ fontSize: 13.5, fontWeight: 600, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis', maxWidth: 440 }}>{name}</div>
          <div style={{ fontSize: 11.5, fontFamily: theme.mono, fontWeight: 600, color: theme.accent }}>{speed}</div>
        </div>
        <div style={{ fontSize: 11, color: theme.ink4, fontFamily: theme.mono, marginTop: 2, display: 'flex', gap: 10 }}>
          <span>{folder}</span>·
          <span>{done} / {size}</span>·
          <span>{threads} luồng</span>·
          <span>còn {eta}</span>
        </div>
        <div style={{ marginTop: 10, height: 5, background: theme.border, borderRadius: 4, overflow: 'hidden' }}>
          <div style={{ width: `${pct}%`, height: '100%', background: theme.grad, borderRadius: 4 }}/>
        </div>
      </div>
      <div style={{ display: 'flex', gap: 6, color: theme.ink3 }}>
        <IconBtn theme={theme}><Icons.pause size={14}/></IconBtn>
        <IconBtn theme={theme}><Icons.x size={14}/></IconBtn>
      </div>
    </div>
  );
}

function DLRowDone({ theme, name, size, time, iconBg, iconColor }) {
  return (
    <div style={{
      background: theme.panel, border: `1px solid ${theme.border}`,
      borderRadius: 12, padding: '10px 16px', display: 'flex', alignItems: 'center', gap: 14,
    }}>
      <div style={{
        width: 32, height: 32, borderRadius: 8, background: iconBg, flexShrink: 0,
        display: 'flex', alignItems: 'center', justifyContent: 'center', color: iconColor,
      }}><Icons.check size={15}/></div>
      <div style={{ flex: 1 }}>
        <div style={{ fontSize: 13, fontWeight: 500 }}>{name}</div>
        <div style={{ fontSize: 10.5, color: theme.ink4, fontFamily: theme.mono, marginTop: 1 }}>{size} · hoàn tất {time}</div>
      </div>
      <span style={{ fontSize: 11, color: theme.success, fontWeight: 600 }}>✓ Đã tải</span>
      <Icons.folder size={14} style={{ color: theme.ink3 }}/>
    </div>
  );
}

// ═══════════════ SCREEN 3: SHARE LINK ═══════════════
function AuroraShare() {
  const t = useTA();
  return (
    <WinChrome title="Fshare · Chia sẻ" theme={t}>
      <AuroraSidebar active="shared"/>
      <div style={{ flex: 1, background: t.bg, display: 'flex', flexDirection: 'column' }}>
        <div style={{ padding: '28px 28px 20px', borderBottom: `1px solid ${t.border}`, background: t.panel }}>
          <div style={{ fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', marginBottom: 6 }}>Đã chia sẻ · 14 link đang hoạt động</div>
          <h1 style={{ fontFamily: t.serif, fontSize: 40, fontWeight: 400, letterSpacing: '-.03em', margin: 0 }}>Link của bạn, <span style={{ fontStyle: 'italic', color: t.accent }}>trong tầm tay.</span></h1>
        </div>

        <div style={{ padding: 28, display: 'grid', gridTemplateColumns: '1fr 380px', gap: 24, flex: 1, overflow: 'hidden' }}>
          {/* list */}
          <div style={{ display: 'flex', flexDirection: 'column', gap: 10, overflow: 'auto', paddingBottom: 20 }}>
            <ShareCard theme={t} active name="Avatar 2.mp4" link="fshare.vn/s/aB3k9Xm" views={142} limit={200} password expire="còn 3 ngày"/>
            <ShareCard theme={t} name="Album ảnh cưới.zip" link="fshare.vn/s/wd7Hp2q" views={28} limit={null} password={false} expire="Vĩnh viễn"/>
            <ShareCard theme={t} warn name="Báo cáo Q3 2026.pdf" link="fshare.vn/s/Rp4mK1n" views={8} limit={10} password expire="HẾT HẠN sau 6h"/>
            <ShareCard theme={t} name="Playlist Tết.zip" link="fshare.vn/s/tx9Nr6W" views={56} limit={null} password={false} expire="còn 12 ngày"/>
          </div>

          {/* detail / new link panel */}
          <div style={{
            background: t.panel, borderRadius: 16, border: `1px solid ${t.border}`, overflow: 'hidden', display: 'flex', flexDirection: 'column',
          }}>
            <div style={{ padding: '20px 22px 18px', borderBottom: `1px solid ${t.border}`, background: t.gradSoft }}>
              <div style={{ fontSize: 11, fontFamily: t.mono, color: t.accent, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600, marginBottom: 8 }}>Chi tiết link · Avatar 2.mp4</div>
              <div style={{ display: 'flex', alignItems: 'center', gap: 10, padding: '10px 14px', background: t.panel, borderRadius: 10, border: `1px solid ${t.border}` }}>
                <Icons.link size={14} stroke={t.accent}/>
                <div style={{ fontFamily: t.mono, fontSize: 12.5, fontWeight: 600, flex: 1, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap', color: t.ink }}>
                  fshare.vn/s/aB3k9Xm
                </div>
                <button style={{ padding: '5px 10px', background: t.grad, color: '#fff', border: 'none', borderRadius: 6, fontSize: 11, fontWeight: 600, cursor: 'pointer', display: 'flex', alignItems: 'center', gap: 4 }}>
                  <Icons.copy size={11}/> Copy
                </button>
              </div>
            </div>

            <div style={{ padding: '18px 22px', flex: 1, overflow: 'auto' }}>
              <OptionRow theme={t} icon={Icons.lock} label="Mật khẩu" value="••••••••" action="Đổi"/>
              <OptionRow theme={t} icon={Icons.clock} label="Hết hạn" value="15/05/2026 · còn 3 ngày"/>
              <OptionRow theme={t} icon={Icons.eye} label="Lượt xem" value="142 / 200"/>
              <OptionRow theme={t} icon={Icons.download} label="Cho phép tải" value="Có · tối đa 500"/>
              <OptionRow theme={t} icon={Icons.shield} label="Chỉ người có link" value="Bật"/>

              {/* visitors graph */}
              <div style={{ marginTop: 18, padding: '14px 14px 10px', background: t.bg, borderRadius: 12 }}>
                <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: 10, fontSize: 11, fontFamily: t.mono, color: t.ink4 }}>
                  <span style={{ fontWeight: 600, color: t.ink }}>Lượt xem · 7 ngày</span>
                  <span>142 tổng</span>
                </div>
                <div style={{ display: 'flex', alignItems: 'flex-end', gap: 6, height: 50 }}>
                  {[12, 28, 18, 34, 22, 16, 12].map((v, i) => (
                    <div key={i} style={{ flex: 1, height: `${v*2}%`, background: i === 3 ? t.grad : t.border, borderRadius: 3 }}/>
                  ))}
                </div>
              </div>

              <div style={{ marginTop: 16, display: 'flex', gap: 8 }}>
                <button style={{ flex: 1, padding: '10px', background: t.dangerSoft, color: t.danger, border: 'none', borderRadius: 10, fontSize: 12.5, fontWeight: 600, cursor: 'pointer', fontFamily: t.font }}>Thu hồi link</button>
                <button style={{ flex: 1, padding: '10px', background: t.grad, color: '#fff', border: 'none', borderRadius: 10, fontSize: 12.5, fontWeight: 600, cursor: 'pointer', fontFamily: t.font }}>Gia hạn +30d</button>
              </div>
            </div>
          </div>
        </div>
      </div>
    </WinChrome>
  );
}

function ShareCard({ theme, name, link, views, limit, password, expire, active, warn }) {
  return (
    <div style={{
      background: theme.panel, border: `1.5px solid ${active ? theme.accent : theme.border}`,
      borderRadius: 14, padding: '16px 20px', display: 'flex', alignItems: 'center', gap: 16,
      boxShadow: active ? '0 6px 20px rgba(255,91,46,.12)' : 'none',
    }}>
      <div style={{
        width: 42, height: 42, borderRadius: 10, background: theme.gradSoft,
        display: 'flex', alignItems: 'center', justifyContent: 'center', color: theme.accent,
      }}><Icons.link size={18}/></div>
      <div style={{ flex: 1 }}>
        <div style={{ fontSize: 14, fontWeight: 600, marginBottom: 3 }}>{name}</div>
        <div style={{ fontSize: 11.5, color: theme.ink3, fontFamily: theme.mono }}>{link}</div>
      </div>
      <div style={{ display: 'flex', alignItems: 'center', gap: 14, fontSize: 11.5 }}>
        {password && <span style={{ display: 'flex', alignItems: 'center', gap: 4, color: theme.ink3 }}><Icons.lock size={11}/> Bảo vệ</span>}
        <span style={{ display: 'flex', alignItems: 'center', gap: 4, color: theme.ink3, fontFamily: theme.mono }}><Icons.eye size={11}/> {views}{limit ? `/${limit}` : ''}</span>
        {warn ? <span style={{
          color: theme.danger, fontWeight: 600,
          padding: '3px 8px', background: theme.dangerSoft, borderRadius: 4,
        }}>{expire}</span> : <span style={{ color: theme.ink3, fontWeight: 500 }}>{expire}</span>}
      </div>
      <Icons.chevRight size={16} style={{ color: theme.ink4 }}/>
    </div>
  );
}

function OptionRow({ theme, icon, label, value, action }) {
  const Ico = icon;
  return (
    <div style={{
      display: 'flex', alignItems: 'center', gap: 12, padding: '11px 0',
      borderBottom: `1px dashed ${theme.border}`,
    }}>
      <Ico size={14} stroke={theme.ink3}/>
      <div style={{ fontSize: 12.5, color: theme.ink3, width: 110 }}>{label}</div>
      <div style={{ flex: 1, fontSize: 12.5, fontWeight: 500 }}>{value}</div>
      {action && <span style={{ fontSize: 11, color: theme.accent, fontWeight: 600, cursor: 'pointer' }}>{action}</span>}
    </div>
  );
}

Object.assign(window, { AuroraHome, AuroraDownloads, AuroraShare });
