// ds-icon-library.jsx — Unified 24×24 icon system · stroke 1.5 · round caps

// Single-source icon defs. Each icon = a <g> of paths drawn on a 24x24 grid.
const ICON_DEFS = {
  // navigation
  home: <path d="M4 10.5 12 4l8 6.5V20a1 1 0 0 1-1 1h-4v-6h-6v6H5a1 1 0 0 1-1-1z"/>,
  folder: <path d="M3 7a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>,
  file: <><path d="M14 3H6a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z"/><path d="M14 3v6h6"/></>,
  files: <><path d="M15 4H8a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V9z"/><path d="M15 4v5h5M3 15V5a2 2 0 0 1 2-2h6"/></>,
  grid: <><rect x="3" y="3" width="7" height="7" rx="1"/><rect x="14" y="3" width="7" height="7" rx="1"/><rect x="3" y="14" width="7" height="7" rx="1"/><rect x="14" y="14" width="7" height="7" rx="1"/></>,
  list: <><path d="M8 6h13M8 12h13M8 18h13"/><circle cx="4" cy="6" r="1"/><circle cx="4" cy="12" r="1"/><circle cx="4" cy="18" r="1"/></>,

  // actions
  upload: <><path d="M12 3v13M7 8l5-5 5 5"/><path d="M4 17v2a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-2"/></>,
  download: <><path d="M12 3v13M17 11l-5 5-5-5"/><path d="M4 17v2a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-2"/></>,
  share: <><circle cx="18" cy="5" r="3"/><circle cx="6" cy="12" r="3"/><circle cx="18" cy="19" r="3"/><path d="M8.6 10.5 15.4 6.5M8.6 13.5l6.8 4"/></>,
  link: <><path d="M10 14a5 5 0 0 0 7 0l3-3a5 5 0 1 0-7-7l-1.5 1.5"/><path d="M14 10a5 5 0 0 0-7 0l-3 3a5 5 0 1 0 7 7l1.5-1.5"/></>,
  plus: <path d="M12 5v14M5 12h14"/>,
  minus: <path d="M5 12h14"/>,
  x: <path d="M6 6l12 12M18 6 6 18"/>,
  check: <path d="M5 12.5 10 17 20 7"/>,
  edit: <><path d="M4 20h4L19 9l-4-4L4 16z"/><path d="m14 6 4 4"/></>,
  trash: <><path d="M4 7h16M10 11v6M14 11v6"/><path d="M6 7v12a2 2 0 0 0 2 2h8a2 2 0 0 0 2-2V7M9 7V4h6v3"/></>,

  // status
  star: <path d="M12 3l2.8 6 6.2.7-4.7 4.3L17.6 20 12 16.8 6.4 20l1.3-6L3 9.7 9.2 9z"/>,
  heart: <path d="M12 20s-7-4.5-7-10a4 4 0 0 1 7-2.5A4 4 0 0 1 19 10c0 5.5-7 10-7 10z"/>,
  bell: <><path d="M6 8a6 6 0 1 1 12 0c0 5 2 7 2 7H4s2-2 2-7z"/><path d="M10 20a2 2 0 0 0 4 0"/></>,
  info: <><circle cx="12" cy="12" r="9"/><path d="M12 8v.5M12 11v5"/></>,
  warn: <><path d="M12 3 2 20h20z"/><path d="M12 10v5M12 18.5v.5"/></>,
  help: <><circle cx="12" cy="12" r="9"/><path d="M9.5 9a2.5 2.5 0 1 1 3.5 2.5c-1 .5-1 1-1 2M12 17v.5"/></>,

  // media & files
  image: <><rect x="3" y="5" width="18" height="14" rx="2"/><circle cx="8.5" cy="10" r="1.5"/><path d="m4 17 5-5 4 4 3-3 4 4"/></>,
  video: <><rect x="3" y="5" width="18" height="14" rx="2"/><path d="m10 9 6 3-6 3z"/></>,
  music: <><path d="M9 18V6l10-2v12"/><circle cx="7" cy="18" r="2"/><circle cx="17" cy="16" r="2"/></>,
  doc: <><path d="M14 3H6a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z"/><path d="M14 3v6h6M8 13h8M8 17h5"/></>,
  archive: <><rect x="3" y="4" width="18" height="5" rx="1"/><path d="M5 9v10a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V9M10 13h4"/></>,
  pdf: <><path d="M14 3H6a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z"/><path d="M14 3v6h6"/><path d="M8 14h1a1.5 1.5 0 0 1 0 3H8zM12 14v3M12 14h2M12 15.5h1.5M16 17v-3h2"/></>,
  zip: <><path d="M14 3H6a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z"/><path d="M14 3v6h6M11 5v2M11 9v2M11 13v2M11 17v2"/></>,

  // system
  search: <><circle cx="11" cy="11" r="7"/><path d="m16 16 5 5"/></>,
  settings: <><circle cx="12" cy="12" r="3"/><path d="M12 3v3M12 18v3M3 12h3M18 12h3M5.6 5.6l2.1 2.1M16.3 16.3l2.1 2.1M5.6 18.4l2.1-2.1M16.3 7.7l2.1-2.1"/></>,
  user: <><circle cx="12" cy="8" r="4"/><path d="M4 20c0-4 4-6 8-6s8 2 8 6"/></>,
  users: <><circle cx="9" cy="9" r="3.5"/><path d="M3 19c0-3 3-5 6-5s6 2 6 5"/><path d="M16 6a3.5 3.5 0 0 1 0 7M16 14c2.5 0 5 1.5 5 4"/></>,
  cloud: <path d="M7 19a4 4 0 0 1-.4-8 6 6 0 0 1 11.4-.8A4 4 0 0 1 17 19z"/>,
  shield: <path d="M12 3 4 6v5c0 5 3.5 8.5 8 10 4.5-1.5 8-5 8-10V6z"/>,
  lock: <><rect x="4" y="11" width="16" height="10" rx="2"/><path d="M8 11V8a4 4 0 0 1 8 0v3"/></>,
  key: <><circle cx="8" cy="14" r="4"/><path d="M11 14h10M17 11v3M20 14v3"/></>,

  // motion & states
  arrowUp: <path d="M12 4v16M6 10l6-6 6 6"/>,
  arrowDown: <path d="M12 4v16M6 14l6 6 6-6"/>,
  arrowRight: <path d="M4 12h16M14 6l6 6-6 6"/>,
  arrowLeft: <path d="M20 12H4M10 6l-6 6 6 6"/>,
  refresh: <><path d="M20 11a8 8 0 0 0-14-4M4 13a8 8 0 0 0 14 4"/><path d="M20 4v4h-4M4 20v-4h4"/></>,
  play: <path d="M6 4v16l14-8z"/>,
  pause: <><rect x="6" y="5" width="4" height="14" rx="1"/><rect x="14" y="5" width="4" height="14" rx="1"/></>,
  more: <><circle cx="6" cy="12" r="1.3"/><circle cx="12" cy="12" r="1.3"/><circle cx="18" cy="12" r="1.3"/></>,
};

function DSIcon({ name, size=24, stroke='currentColor', fill='none', sw=1.5 }) {
  const def = ICON_DEFS[name];
  if (!def) return null;
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill={fill} stroke={stroke} strokeWidth={sw} strokeLinecap="round" strokeLinejoin="round" aria-hidden="true" style={{ display:'inline-block', flexShrink:0 }}>
      {def}
    </svg>
  );
}

function IconLibraryCard() {
  const t = T_A_LIGHT;
  const groups = [
    ['Navigation', ['home','folder','file','files','grid','list']],
    ['Actions', ['upload','download','share','link','plus','minus','x','check','edit','trash']],
    ['Status & feedback', ['star','heart','bell','info','warn','help']],
    ['Media & files', ['image','video','music','doc','archive','pdf','zip']],
    ['System & security', ['search','settings','user','users','cloud','shield','lock','key']],
    ['Motion & arrows', ['arrowUp','arrowDown','arrowLeft','arrowRight','refresh','play','pause','more']],
  ];
  return (
    <div style={{ width:'100%', height:'100%', padding:'48px 56px', background:t.bg, color:t.ink, fontFamily:t.font, overflow:'auto', boxSizing:'border-box' }}>
      <DSHeader t={t}
        eyebrow="Icon library · 05"
        title={<>45 icon · <span style={{ fontStyle:'italic', color:t.accent }}>một hệ.</span></>}
        sub="Thống nhất grid 24 · stroke 1.5px · round cap. Dùng hệ này cho tất cả component — không trộn lẫn emoji hay icon từ nguồn khác."
      />

      {/* spec card */}
      <div style={{ marginTop:32, display:'grid', gridTemplateColumns:'1fr 2fr', gap:20 }}>
        <DSFCard t={t} title="Spec" sub="Kỹ thuật vẽ một icon chuẩn">
          <div style={{ display:'flex', justifyContent:'center', padding:'20px 0', background:t.bg, borderRadius:10, position:'relative' }}>
            <svg width="180" height="180" viewBox="0 0 24 24" style={{ display:'block' }}>
              {/* grid */}
              {Array.from({length:7}).map((_,i)=><line key={'v'+i} x1={2+i*3.33} y1="2" x2={2+i*3.33} y2="22" stroke={t.border} strokeWidth=".2"/>)}
              {Array.from({length:7}).map((_,i)=><line key={'h'+i} x1="2" y1={2+i*3.33} x2="22" y2={2+i*3.33} stroke={t.border} strokeWidth=".2"/>)}
              {/* safe area */}
              <rect x="2" y="2" width="20" height="20" fill="none" stroke={t.accent} strokeWidth=".3" strokeDasharray="1 .5"/>
              {/* icon: cloud */}
              <path d="M7 19a4 4 0 0 1-.4-8 6 6 0 0 1 11.4-.8A4 4 0 0 1 17 19z" fill="none" stroke={t.ink} strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round"/>
            </svg>
          </div>
          <div style={{ marginTop:14, display:'grid', gridTemplateColumns:'70px 1fr', rowGap:8, fontSize:11.5 }}>
            <span style={{ color:t.ink4, fontFamily:t.mono }}>Grid</span><span>24 × 24 · safe area 20</span>
            <span style={{ color:t.ink4, fontFamily:t.mono }}>Stroke</span><span>1.5px · không scale theo size</span>
            <span style={{ color:t.ink4, fontFamily:t.mono }}>Cap</span><span>round · round joins</span>
            <span style={{ color:t.ink4, fontFamily:t.mono }}>Fill</span><span>none (trừ filled variant)</span>
            <span style={{ color:t.ink4, fontFamily:t.mono }}>Color</span><span>currentColor · theo context</span>
          </div>
        </DSFCard>

        <DSFCard t={t} title="Size scale" sub="5 kích cỡ · không đệ quy scale">
          <div style={{ display:'flex', alignItems:'flex-end', gap:32, padding:'22px 8px' }}>
            {[12,14,16,20,24].map(s=>(
              <div key={s} style={{ display:'flex', flexDirection:'column', alignItems:'center', gap:10 }}>
                <DSIcon name="cloud" size={s} stroke={t.ink}/>
                <span style={{ fontSize:10.5, fontFamily:t.mono, color:t.ink4, fontWeight:600 }}>{s}</span>
              </div>
            ))}
            <div style={{ width:1, height:40, background:t.border, margin:'0 8px' }}/>
            <div style={{ display:'flex', alignItems:'center', gap:14 }}>
              <DSIcon name="cloud" size={24} stroke={t.accent}/>
              <DSIcon name="cloud" size={24} stroke={t.success}/>
              <DSIcon name="cloud" size={24} stroke={t.danger}/>
              <DSIcon name="cloud" size={24} stroke={t.ink3}/>
              <DSIcon name="cloud" size={24} fill={t.accent} stroke={t.accent}/>
            </div>
          </div>
          <div style={{ display:'flex', justifyContent:'space-between', fontSize:10.5, fontFamily:t.mono, color:t.ink4, marginTop:6, padding:'0 8px' }}>
            <span>tiny</span><span>inline</span><span>default</span><span>ui</span><span>accent</span>
            <span style={{ color:t.ink }}>· currentColor ·</span>
          </div>
        </DSFCard>
      </div>

      {/* GROUPS */}
      {groups.map(([name, icons])=>(
        <div key={name} style={{ marginTop:28 }}>
          <div style={{ fontSize:11, fontFamily:t.mono, color:t.ink4, letterSpacing:'.14em', textTransform:'uppercase', fontWeight:600, marginBottom:12 }}>━ {name} · {icons.length}</div>
          <div style={{ display:'grid', gridTemplateColumns:'repeat(8, 1fr)', gap:10 }}>
            {icons.map(ic=>(
              <div key={ic} style={{ padding:'18px 10px 12px', background:t.panel, border:`1px solid ${t.border}`, borderRadius:12, display:'flex', flexDirection:'column', alignItems:'center', gap:10, cursor:'pointer' }}>
                <DSIcon name={ic} size={24} stroke={t.ink}/>
                <span style={{ fontSize:10.5, fontFamily:t.mono, color:t.ink4, fontWeight:600 }}>{ic}</span>
              </div>
            ))}
          </div>
        </div>
      ))}

      {/* DO / DONT */}
      <div style={{ marginTop:32, display:'grid', gridTemplateColumns:'1fr 1fr', gap:16 }}>
        <DoDont t={t} ok title="Do — dùng currentColor">
          <div style={{ display:'flex', gap:16, alignItems:'center', marginTop:8 }}>
            <div style={{ padding:'6px 12px', borderRadius:8, background:t.grad, color:'#fff', display:'flex', alignItems:'center', gap:6, fontSize:12, fontWeight:600 }}><DSIcon name="download" size={14}/> Tải</div>
            <div style={{ padding:'6px 12px', borderRadius:8, background:'#fff', border:`1.5px solid ${t.border}`, color:t.ink, display:'flex', alignItems:'center', gap:6, fontSize:12, fontWeight:600 }}><DSIcon name="upload" size={14}/> Up</div>
          </div>
        </DoDont>
        <DoDont t={t} title="Don't — trộn emoji với icon line">
          <div style={{ display:'flex', gap:16, alignItems:'center', marginTop:8 }}>
            <div style={{ padding:'6px 12px', borderRadius:8, background:t.grad, color:'#fff', display:'flex', alignItems:'center', gap:6, fontSize:12, fontWeight:600 }}>📥 Tải</div>
            <div style={{ padding:'6px 12px', borderRadius:8, background:'#fff', border:`1.5px solid ${t.border}`, color:t.ink, display:'flex', alignItems:'center', gap:6, fontSize:12, fontWeight:600 }}>☁️ Up</div>
          </div>
        </DoDont>
      </div>
    </div>
  );
}

function DoDont({ t, ok, title, children }) {
  return (
    <div style={{ padding:'18px 22px', background:t.panel, border:`1.5px solid ${ok? t.success : t.danger}40`, borderLeft:`4px solid ${ok? t.success : t.danger}`, borderRadius:12 }}>
      <div style={{ fontSize:11, fontFamily:t.mono, color: ok? t.success : t.danger, letterSpacing:'.1em', textTransform:'uppercase', fontWeight:700 }}>{ok? '✓ DO' : '✕ DON\'T'}</div>
      <div style={{ fontSize:14, fontWeight:700, marginTop:4 }}>{title}</div>
      {children}
    </div>
  );
}

Object.assign(window, { DSIcon, IconLibraryCard, ICON_DEFS });
