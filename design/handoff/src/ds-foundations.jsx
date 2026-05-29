// ds-foundations.jsx — Foundations artboard (spacing, radius, shadow, motion, z-index)
// Uses Aurora light theme tokens. Pure visual reference cards.

function FoundationsCard() {
  const t = T_A_LIGHT;
  return (
    <div style={{
      width: '100%', height: '100%', padding: '48px 56px',
      background: t.bg, color: t.ink, fontFamily: t.font,
      overflow: 'auto', boxSizing: 'border-box',
    }}>
      <DSHeader t={t}
        eyebrow="Foundations · 01"
        title={<>Nền tảng hệ <span style={{ fontStyle:'italic', color:t.accent }}>thống.</span></>}
        sub="Spacing, bo góc, đổ bóng, chuyển động, z-index — tokens dùng chung cho mọi component."
      />

      <div style={{ display:'grid', gridTemplateColumns:'1.1fr 1fr', gap:24, marginTop:32 }}>
        {/* SPACING */}
        <DSFCard t={t} title="Spacing" sub="Thang 4px · 10 bậc · dùng cho padding, gap, margin">
          <div style={{ display:'flex', flexDirection:'column', gap:10 }}>
            {[
              ['xs',4],['sm',8],['md',12],['lg',16],['xl',20],
              ['2xl',24],['3xl',32],['4xl',40],['5xl',56],['6xl',72],
            ].map(([n,v]) => (
              <div key={n} style={{ display:'flex', alignItems:'center', gap:14 }}>
                <div style={{ width:52, fontSize:11, fontFamily:t.mono, color:t.ink3, fontWeight:600 }}>{n}</div>
                <div style={{ width:48, fontSize:11, fontFamily:t.mono, color:t.ink4 }}>{v}px</div>
                <div style={{ height:12, width:v*2.4, background:t.grad, borderRadius:3 }}/>
              </div>
            ))}
          </div>
        </DSFCard>

        {/* RADIUS */}
        <DSFCard t={t} title="Bo góc" sub="4 cấp · chip dùng pill">
          <div style={{ display:'grid', gridTemplateColumns:'repeat(4,1fr)', gap:12 }}>
            {[['sm',6],['md',10],['lg',14],['xl',20]].map(([n,v])=>(
              <div key={n} style={{ display:'flex', flexDirection:'column', alignItems:'center', gap:8 }}>
                <div style={{ width:72, height:72, background:t.gradSoft, border:`1.5px solid ${t.accent}`, borderRadius:v }}/>
                <div style={{ fontSize:11, fontFamily:t.mono, color:t.ink3, fontWeight:600 }}>{n} · {v}px</div>
              </div>
            ))}
          </div>
          <div style={{ marginTop:18, display:'flex', alignItems:'center', gap:12 }}>
            <div style={{ padding:'6px 14px', background:t.ink, color:'#fff', borderRadius:999, fontSize:11, fontWeight:600 }}>pill · 999px</div>
            <span style={{ fontSize:11, color:t.ink4 }}>chip · toggle · tag</span>
          </div>
        </DSFCard>

        {/* SHADOW */}
        <DSFCard t={t} title="Đổ bóng · Elevation" sub="4 cấp — resting → overlay">
          <div style={{ display:'grid', gridTemplateColumns:'repeat(4,1fr)', gap:16 }}>
            {[
              ['e0','0 1px 0 rgba(14,14,18,.04)','Flat'],
              ['e1','0 2px 6px rgba(14,14,18,.06), 0 0 0 1px rgba(14,14,18,.04)','Card'],
              ['e2','0 8px 24px rgba(14,14,18,.08), 0 0 0 1px rgba(14,14,18,.04)','Menu'],
              ['e3','0 24px 60px rgba(14,14,18,.16), 0 0 0 1px rgba(14,14,18,.05)','Modal'],
            ].map(([n,s,u])=>(
              <div key={n} style={{ display:'flex', flexDirection:'column', gap:8, alignItems:'center' }}>
                <div style={{ width:'100%', height:64, background:'#fff', borderRadius:10, boxShadow:s }}/>
                <div style={{ fontSize:11, fontFamily:t.mono, color:t.ink2, fontWeight:700 }}>{n}</div>
                <div style={{ fontSize:10, color:t.ink4 }}>{u}</div>
              </div>
            ))}
          </div>
        </DSFCard>

        {/* MOTION */}
        <DSFCard t={t} title="Chuyển động" sub="Duration × Easing · mặc định = base + standard">
          <div style={{ display:'grid', gridTemplateColumns:'1fr 1fr', gap:14 }}>
            <div>
              <DSSubLbl t={t}>Duration</DSSubLbl>
              <MotionRow t={t} name="instant" v="80ms" use="hover, focus"/>
              <MotionRow t={t} name="fast" v="140ms" use="toggle, chip"/>
              <MotionRow t={t} name="base" v="220ms" use="card, tab"/>
              <MotionRow t={t} name="slow" v="360ms" use="modal, drawer"/>
              <MotionRow t={t} name="stage" v="560ms" use="page transition" last/>
            </div>
            <div>
              <DSSubLbl t={t}>Easing</DSSubLbl>
              <EaseRow t={t} name="standard" v="cubic-bezier(.2,0,0,1)" use="default"/>
              <EaseRow t={t} name="emphasized" v="cubic-bezier(.3,0,0,1)" use="entrance"/>
              <EaseRow t={t} name="decelerate" v="cubic-bezier(0,0,.2,1)" use="exit"/>
              <EaseRow t={t} name="linear" v="linear" use="progress" last/>
            </div>
          </div>
        </DSFCard>

        {/* Z-INDEX */}
        <DSFCard t={t} title="Z-index" sub="Thang cố định — tránh chồng chéo">
          <div style={{ position:'relative', height:170 }}>
            {[
              ['base',0,'#F5F4F1','Nội dung',18],
              ['sticky',100,'#FFE8E0','DSHeader, sidebar',38],
              ['dropdown',1000,'#FFCEB8','Menu, popover',58],
              ['overlay',1400,'#FFB290','Scrim',78],
              ['modal',1500,'#FF8A5A','Dialog, drawer',98],
              ['toast',1800,'#FF5B2E','Snackbar',118],
              ['tooltip',2000,'#0E0E12','Tooltip',138],
            ].map(([n,v,c,u,top])=>(
              <div key={n} style={{
                position:'absolute', left:(v===0?0:v/20), top, width:320, height:24,
                background:c, color: n==='tooltip'?'#fff':t.ink, borderRadius:6,
                display:'flex', alignItems:'center', padding:'0 10px', gap:10, fontSize:11,
                border:`1px solid ${t.border}`,
              }}>
                <span style={{ fontFamily:t.mono, fontWeight:700, width:60 }}>{n}</span>
                <span style={{ fontFamily:t.mono, color: n==='tooltip'?'#FFAF1D':t.ink3, width:44 }}>{v}</span>
                <span style={{ opacity:.75 }}>{u}</span>
              </div>
            ))}
          </div>
        </DSFCard>

        {/* GRID */}
        <DSFCard t={t} title="Grid & Container" sub="Layout chuẩn cho màn hình desktop">
          <div style={{ display:'grid', gridTemplateColumns:'repeat(12,1fr)', gap:6, height:54 }}>
            {Array.from({length:12}).map((_,i)=>(
              <div key={i} style={{ background:t.gradSoft, border:`1px solid ${t.accent}`, borderRadius:4, display:'flex', alignItems:'center', justifyContent:'center', fontSize:10, fontFamily:t.mono, color:t.accent }}>{i+1}</div>
            ))}
          </div>
          <div style={{ marginTop:14, display:'grid', gridTemplateColumns:'repeat(3,1fr)', gap:10, fontSize:11, fontFamily:t.mono, color:t.ink3 }}>
            <div><div style={{ color:t.ink, fontWeight:700, fontSize:13 }}>12 cột</div>gutter 24</div>
            <div><div style={{ color:t.ink, fontWeight:700, fontSize:13 }}>1280 min</div>content width</div>
            <div><div style={{ color:t.ink, fontWeight:700, fontSize:13 }}>1440 max</div>padding 32 trái/phải</div>
          </div>
        </DSFCard>
      </div>
    </div>
  );
}

function DSFCard({ t, title, sub, children }) {
  return (
    <div style={{
      background:t.panel, border:`1px solid ${t.border}`, borderRadius:16,
      padding:'22px 24px',
    }}>
      <div style={{ marginBottom:16 }}>
        <div style={{ fontSize:15, fontWeight:700, letterSpacing:'-.01em' }}>{title}</div>
        <div style={{ fontSize:11.5, color:t.ink3, marginTop:3 }}>{sub}</div>
      </div>
      {children}
    </div>
  );
}

function DSHeader({ t, eyebrow, title, sub }) {
  return (
    <div>
      <div style={{ fontSize:11, fontFamily:t.mono, color:t.ink4, letterSpacing:'.14em', textTransform:'uppercase', marginBottom:10 }}>━━ {eyebrow}</div>
      <div style={{ fontFamily:t.serif, fontSize:64, fontWeight:400, letterSpacing:'-.03em', lineHeight:1 }}>{title}</div>
      <div style={{ fontSize:14, color:t.ink3, marginTop:12, maxWidth:680 }}>{sub}</div>
    </div>
  );
}

function DSSubLbl({ t, children }) {
  return <div style={{ fontSize:10.5, fontFamily:t.mono, color:t.ink4, letterSpacing:'.14em', textTransform:'uppercase', fontWeight:600, marginBottom:8 }}>{children}</div>;
}

function MotionRow({ t, name, v, use, last }) {
  return (
    <div style={{ display:'flex', alignItems:'center', gap:10, padding:'8px 0', borderBottom: last?'none':`1px dashed ${t.border}` }}>
      <div style={{ width:56, fontSize:12, fontWeight:600 }}>{name}</div>
      <div style={{ fontSize:11, fontFamily:t.mono, color:t.accent, width:52 }}>{v}</div>
      <div style={{ fontSize:11, color:t.ink4, flex:1 }}>{use}</div>
    </div>
  );
}
function EaseRow({ t, name, v, use, last }) {
  return (
    <div style={{ padding:'8px 0', borderBottom: last?'none':`1px dashed ${t.border}` }}>
      <div style={{ display:'flex', alignItems:'baseline', gap:8 }}>
        <div style={{ fontSize:12, fontWeight:600 }}>{name}</div>
        <div style={{ fontSize:10, color:t.ink4 }}>{use}</div>
      </div>
      <div style={{ fontSize:10, fontFamily:t.mono, color:t.accent, marginTop:2 }}>{v}</div>
    </div>
  );
}

Object.assign(window, { FoundationsCard });
