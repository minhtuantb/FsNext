// ds-components-2.jsx — Component library · Part 2
// Badge, DSChipFilter, Tag, Tooltip, Alert, Toast, Notification, Progress

function ComponentsCard2() {
  const t = T_A_LIGHT;
  return (
    <div style={{ width:'100%', height:'100%', padding:'48px 56px', background:t.bg, color:t.ink, fontFamily:t.font, overflow:'auto', boxSizing:'border-box' }}>
      <DSHeader t={t}
        eyebrow="Components · 03 · Feedback"
        title={<>Trạng thái & <span style={{ fontStyle:'italic', color:t.accent }}>phản hồi.</span></>}
        sub="Badge, chip, tag, tooltip, alert, toast, progress — những mảnh nhỏ truyền tải trạng thái."
      />

      <div style={{ display:'grid', gridTemplateColumns:'1fr', gap:20, marginTop:32 }}>

        {/* BADGES + CHIPS + TAGS */}
        <div style={{ display:'grid', gridTemplateColumns:'1fr 1fr', gap:20 }}>
          <DSFCard t={t} title="Badge · Status" sub="Dot + filled + outline · semantic colors">
            <DSSubLbl t={t}>Filled</DSSubLbl>
            <div style={{ display:'flex', gap:8, flexWrap:'wrap', marginBottom:16 }}>
              <Bdg t={t} color={t.success}>✓ Thành công</Bdg>
              <Bdg t={t} color={t.danger}>✕ Lỗi</Bdg>
              <Bdg t={t} color={t.warn}>⚠ Cảnh báo</Bdg>
              <Bdg t={t} color="#2C6EF2">ⓘ Thông tin</Bdg>
              <Bdg t={t} color={t.accent}>VIP</Bdg>
            </div>
            <DSSubLbl t={t}>Soft (tint)</DSSubLbl>
            <div style={{ display:'flex', gap:8, flexWrap:'wrap', marginBottom:16 }}>
              <BdgSoft t={t} c={t.success} bg="rgba(10,138,92,.12)">Đã upload</BdgSoft>
              <BdgSoft t={t} c={t.danger} bg={t.dangerSoft}>Thất bại</BdgSoft>
              <BdgSoft t={t} c={t.warn} bg="rgba(201,106,0,.12)">Tạm dừng</BdgSoft>
              <BdgSoft t={t} c="#2C6EF2" bg="rgba(44,110,242,.1)">Đang chờ</BdgSoft>
              <BdgSoft t={t} c={t.accent} bg={t.gradSoft}>Đang tải 42%</BdgSoft>
            </div>
            <DSSubLbl t={t}>Dot indicator</DSSubLbl>
            <div style={{ display:'flex', gap:16, flexWrap:'wrap' }}>
              <DotRow t={t} c={t.success} label="Đang online"/>
              <DotRow t={t} c={t.warn} label="Idle"/>
              <DotRow t={t} c={t.danger} label="Offline"/>
              <DotRow t={t} c="#c9c9d4" label="Không xác định"/>
            </div>
            <DSSubLbl t={t} style={{ marginTop:16 }}>Number badge</DSSubLbl>
            <div style={{ display:'flex', gap:18, alignItems:'center', marginTop:8 }}>
              {[3, 12, 99, '99+'].map(n=>(
                <div key={n} style={{ position:'relative' }}>
                  <div style={{ width:36, height:36, borderRadius:10, background:t.bg, border:`1.5px solid ${t.border}`, display:'flex', alignItems:'center', justifyContent:'center', fontSize:14 }}>🔔</div>
                  <div style={{ position:'absolute', top:-6, right:-6, minWidth:18, height:18, padding:'0 5px', borderRadius:999, background:t.accent, color:'#fff', fontSize:10, fontWeight:700, display:'flex', alignItems:'center', justifyContent:'center', border:'2px solid #fff' }}>{n}</div>
                </div>
              ))}
            </div>
          </DSFCard>

          <DSFCard t={t} title="DSChipFilter · Tag" sub="Removable, selectable, with icon">
            <DSSubLbl t={t}>Filter chip</DSSubLbl>
            <div style={{ display:'flex', gap:8, flexWrap:'wrap', marginBottom:16 }}>
              <DSChipFilter t={t} selected>Tất cả · 42</DSChipFilter>
              <DSChipFilter t={t}>Phim · 18</DSChipFilter>
              <DSChipFilter t={t}>Nhạc · 12</DSChipFilter>
              <DSChipFilter t={t}>Tài liệu · 8</DSChipFilter>
              <DSChipFilter t={t}>Ảnh · 4</DSChipFilter>
            </div>

            <DSSubLbl t={t}>Input chip (removable)</DSSubLbl>
            <div style={{ display:'flex', gap:8, flexWrap:'wrap', marginBottom:16 }}>
              <ChipRemove t={t} ava="M">Minh Nguyễn</ChipRemove>
              <ChipRemove t={t} ava="TH">Thu Hương</ChipRemove>
              <ChipRemove t={t} ava="LP">Long Phạm</ChipRemove>
            </div>

            <DSSubLbl t={t}>Tag</DSSubLbl>
            <div style={{ display:'flex', gap:6, flexWrap:'wrap' }}>
              {['4K','IMAX','HDR','Atmos','Phụ đề Việt','Remux','2026'].map(tag=>(
                <span key={tag} style={{ padding:'3px 9px', background:t.bg, border:`1px solid ${t.border}`, borderRadius:4, fontSize:10.5, fontFamily:t.mono, fontWeight:600, color:t.ink2, letterSpacing:'.02em' }}>{tag}</span>
              ))}
            </div>
          </DSFCard>
        </div>

        {/* TOOLTIP + KEYBOARD */}
        <DSFCard t={t} title="Tooltip · Keyboard hint" sub="4 vị trí · với shortcut">
          <div style={{ display:'grid', gridTemplateColumns:'repeat(4,1fr)', gap:24, padding:'24px 8px' }}>
            {[
              ['top','↓ Tải xuống','Ctrl+D'],
              ['right','Chia sẻ link','Ctrl+Shift+S'],
              ['bottom','Xoá','Delete'],
              ['left','Thông tin file','I'],
            ].map(([pos,label,kbd])=>(
              <div key={pos} style={{ display:'flex', flexDirection:'column', alignItems:'center', gap:14, position:'relative' }}>
                <TooltipDemo t={t} pos={pos} label={label} kbd={kbd}>
                  <DSIconSq t={t} size={38}>{pos==='top'?'↓':pos==='right'?'⇧':pos==='bottom'?'✕':'ⓘ'}</DSIconSq>
                </TooltipDemo>
                <div style={{ fontSize:10.5, fontFamily:t.mono, color:t.ink4 }}>{pos}</div>
              </div>
            ))}
          </div>
          <div style={{ marginTop:10 }}>
            <DSSubLbl t={t}>Keyboard shortcut — inline</DSSubLbl>
            <div style={{ display:'flex', gap:12, alignItems:'center', fontSize:13, color:t.ink2 }}>
              <span>Bật tìm kiếm nhanh</span>
              <Kbd t={t}>Ctrl</Kbd><span style={{ color:t.ink4 }}>+</span><Kbd t={t}>K</Kbd>
              <span style={{ marginLeft:20 }}>Chuyển màn hình</span>
              <Kbd t={t}>⌘</Kbd><span style={{ color:t.ink4 }}>+</span><Kbd t={t}>1..9</Kbd>
              <span style={{ marginLeft:20 }}>Upload</span>
              <Kbd t={t}>U</Kbd>
            </div>
          </div>
        </DSFCard>

        {/* ALERT */}
        <DSFCard t={t} title="Alert · Inline banner" sub="4 loại · với action · dismissible">
          <div style={{ display:'flex', flexDirection:'column', gap:10 }}>
            <Alert t={t} type="success" title="Upload hoàn tất" desc="3 file đã được tải lên /Phim/2026 · 4.2 GB tổng cộng." actions={['Xem','Sao chép link']}/>
            <Alert t={t} type="info" title="Bản cập nhật mới" desc="Fshare 4.2 đã sẵn sàng với tốc độ upload nhanh hơn 30%." actions={['Cập nhật ngay']}/>
            <Alert t={t} type="warn" title="Sắp hết dung lượng" desc="Còn 24 GB trống (5% gói VIP). Nâng cấp để tiếp tục upload." actions={['Nâng cấp','Dọn rác']}/>
            <Alert t={t} type="danger" title="Không thể kết nối server" desc="Đang thử lại sau 3 giây… (lần 2/5)" actions={['Thử lại ngay']}/>
          </div>
        </DSFCard>

        {/* TOAST + NOTIFICATION */}
        <div style={{ display:'grid', gridTemplateColumns:'1fr 1.1fr', gap:20 }}>
          <DSFCard t={t} title="Toast · Snackbar" sub="Tạm thời · 1 dòng · auto-dismiss 4s">
            <div style={{ display:'flex', flexDirection:'column', gap:12 }}>
              <Toast t={t} icon="✓" c={t.success} bg="#0E0E12">Link đã được sao chép vào clipboard</Toast>
              <Toast t={t} icon="⚠" c={t.warn} bg="#0E0E12">Mất kết nối · đang thử lại</Toast>
              <Toast t={t} icon="✕" c={t.danger} bg="#0E0E12" action="Hoàn tác">Đã xoá 3 file</Toast>
              <Toast t={t} icon="↑" c={t.accent} bg="#0E0E12" progress={62}>Đang upload · 62%</Toast>
            </div>
          </DSFCard>

          <DSFCard t={t} title="Notification" sub="Cố định · có thumbnail · action button">
            <div style={{ display:'flex', flexDirection:'column', gap:12 }}>
              <Notif t={t} type="share"
                title="Thu Hương đã chia sẻ 1 thư mục"
                body="Tài liệu dự án Q4 · 42 file"
                time="2 phút trước"
                ava="TH"
              />
              <Notif t={t} type="done"
                title="Upload hoàn tất"
                body="concert_recording.flac · 620 MB"
                time="14 phút trước"
                icon="✓"
              />
              <Notif t={t} type="storage"
                title="Sắp hết dung lượng VIP"
                body="Còn 24 GB / 500 GB"
                time="Hôm nay"
                icon="⚠"
              />
            </div>
          </DSFCard>
        </div>

        {/* PROGRESS */}
        <DSFCard t={t} title="Progress" sub="Linear, circular, step · với tốc độ & ETA">
          <div style={{ display:'grid', gridTemplateColumns:'1.5fr 1fr 1.3fr', gap:24 }}>
            <div>
              <DSSubLbl t={t}>Linear</DSSubLbl>
              <div style={{ display:'flex', flexDirection:'column', gap:14 }}>
                <LinP t={t} label="Upload" pct={62} speed="18.4 MB/s" eta="38 giây"/>
                <LinP t={t} label="Đang xác minh" pct={100} done/>
                <LinP t={t} label="Indeterminate" indeterminate/>
                <LinP t={t} label="Nhiều đoạn · resumable" segments/>
              </div>
            </div>

            <div>
              <DSSubLbl t={t}>Circular</DSSubLbl>
              <div style={{ display:'flex', gap:18, flexWrap:'wrap', alignItems:'center' }}>
                <Circ t={t} pct={24} size={52}/>
                <Circ t={t} pct={62} size={72} label="62%"/>
                <Circ t={t} pct={88} size={52} ind/>
                <Circ t={t} pct={100} size={52} done/>
              </div>
            </div>

            <div>
              <DSSubLbl t={t}>Step progress</DSSubLbl>
              <Stepper t={t} steps={['Chọn file','Thiết lập','Upload','Chia sẻ']} current={2}/>
              <div style={{ marginTop:22 }}>
                <DSSubLbl t={t}>Skeleton</DSSubLbl>
                <div style={{ display:'flex', flexDirection:'column', gap:8 }}>
                  <DSSkel t={t} w="60%" h={14}/>
                  <DSSkel t={t} w="90%" h={10}/>
                  <DSSkel t={t} w="80%" h={10}/>
                </div>
              </div>
            </div>
          </div>
        </DSFCard>

      </div>
    </div>
  );
}

// ─── atoms ───
function Bdg({ t, color, children }) {
  return <span style={{ padding:'3px 9px', background:color, color:'#fff', borderRadius:4, fontSize:10.5, fontWeight:700, letterSpacing:'.02em' }}>{children}</span>;
}
function BdgSoft({ t, c, bg, children }) {
  return <span style={{ padding:'3px 9px', background:bg, color:c, borderRadius:4, fontSize:10.5, fontWeight:700, letterSpacing:'.02em' }}>{children}</span>;
}
function DotRow({ t, c, label }) {
  return <span style={{ display:'flex', alignItems:'center', gap:6, fontSize:12, color:t.ink2 }}><span style={{ width:8, height:8, borderRadius:'50%', background:c, boxShadow:`0 0 0 3px ${c}22` }}/>{label}</span>;
}
function DSChipFilter({ t, selected, children }) {
  return <span style={{ padding:'6px 12px', borderRadius:999, background: selected? t.ink : '#fff', color: selected? '#fff' : t.ink2, border: selected? 'none' : `1px solid ${t.border}`, fontSize:12, fontWeight:600, cursor:'pointer' }}>{children}</span>;
}
function ChipRemove({ t, ava, children }) {
  return (
    <span style={{ display:'inline-flex', alignItems:'center', gap:6, padding:'4px 6px 4px 4px', background:t.bg, border:`1px solid ${t.border}`, borderRadius:999, fontSize:12, fontWeight:600 }}>
      <span style={{ width:20, height:20, borderRadius:'50%', background:t.grad, color:'#fff', display:'inline-flex', alignItems:'center', justifyContent:'center', fontSize:9, fontWeight:700 }}>{ava}</span>
      {children}
      <span style={{ width:14, height:14, borderRadius:'50%', background:'rgba(0,0,0,.08)', display:'inline-flex', alignItems:'center', justifyContent:'center', fontSize:9, color:t.ink3, cursor:'pointer', marginLeft:2 }}>✕</span>
    </span>
  );
}

function DSIconSq({ t, size=38, children }) {
  return <button style={{ width:size, height:size, borderRadius:10, border:`1.5px solid ${t.borderStrong}`, background:'#fff', fontSize:15, color:t.ink2, display:'flex', alignItems:'center', justifyContent:'center', cursor:'pointer' }}>{children}</button>;
}

function TooltipDemo({ t, pos, label, kbd, children }) {
  const offsets = {
    top: { bottom:'calc(100% + 10px)', left:'50%', transform:'translateX(-50%)' },
    right: { left:'calc(100% + 10px)', top:'50%', transform:'translateY(-50%)' },
    bottom: { top:'calc(100% + 10px)', left:'50%', transform:'translateX(-50%)' },
    left: { right:'calc(100% + 10px)', top:'50%', transform:'translateY(-50%)' },
  };
  return (
    <div style={{ position:'relative' }}>
      {children}
      <div style={{
        position:'absolute', ...offsets[pos], zIndex:10,
        background:'#0E0E12', color:'#fff', padding:'6px 10px', borderRadius:6,
        fontSize:11.5, display:'inline-flex', alignItems:'center', gap:8, whiteSpace:'nowrap',
        boxShadow:'0 6px 16px rgba(0,0,0,.25)',
      }}>
        <span>{label}</span>
        <span style={{ padding:'1px 5px', background:'rgba(255,255,255,.1)', borderRadius:3, fontSize:10, fontFamily:t.mono, color:'#FFAF1D' }}>{kbd}</span>
      </div>
    </div>
  );
}
function Kbd({ t, children }) {
  return <span style={{ padding:'2px 7px', background:t.bg, border:`1px solid ${t.border}`, borderBottomWidth:2, borderRadius:5, fontSize:11, fontFamily:t.mono, fontWeight:600, color:t.ink2 }}>{children}</span>;
}

function Alert({ t, type, title, desc, actions=[] }) {
  const palette = {
    success:{ c:t.success, bg:'rgba(10,138,92,.08)', ic:'✓' },
    info:{ c:'#2C6EF2', bg:'rgba(44,110,242,.06)', ic:'ⓘ' },
    warn:{ c:t.warn, bg:'rgba(201,106,0,.08)', ic:'⚠' },
    danger:{ c:t.danger, bg:t.dangerSoft, ic:'✕' },
  }[type];
  return (
    <div style={{ padding:'14px 18px', background:palette.bg, border:`1px solid ${palette.c}30`, borderLeft:`3px solid ${palette.c}`, borderRadius:10, display:'flex', alignItems:'flex-start', gap:14 }}>
      <div style={{ width:24, height:24, borderRadius:'50%', background:palette.c, color:'#fff', display:'flex', alignItems:'center', justifyContent:'center', fontSize:13, fontWeight:700, flexShrink:0 }}>{palette.ic}</div>
      <div style={{ flex:1 }}>
        <div style={{ fontSize:13.5, fontWeight:700, color:t.ink }}>{title}</div>
        <div style={{ fontSize:12.5, color:t.ink2, marginTop:3 }}>{desc}</div>
        {actions.length>0 && <div style={{ marginTop:10, display:'flex', gap:14 }}>
          {actions.map(a=><a key={a} style={{ fontSize:12, color:palette.c, fontWeight:700, cursor:'pointer' }}>{a} →</a>)}
        </div>}
      </div>
      <span style={{ fontSize:14, color:t.ink4, cursor:'pointer' }}>✕</span>
    </div>
  );
}

function Toast({ t, icon, c, bg, action, progress, children }) {
  return (
    <div style={{ background:bg, color:'#fff', borderRadius:12, padding:'12px 16px', display:'flex', alignItems:'center', gap:12, boxShadow:'0 12px 32px rgba(0,0,0,.18)', position:'relative', overflow:'hidden' }}>
      <div style={{ width:22, height:22, borderRadius:'50%', background: c, color:'#fff', display:'flex', alignItems:'center', justifyContent:'center', fontSize:11, fontWeight:700 }}>{icon}</div>
      <span style={{ flex:1, fontSize:13, fontWeight:500 }}>{children}</span>
      {action && <a style={{ fontSize:12, color:'#FFAF1D', fontWeight:700, cursor:'pointer' }}>{action}</a>}
      <span style={{ fontSize:13, color:'rgba(255,255,255,.4)', cursor:'pointer' }}>✕</span>
      {progress!=null && <div style={{ position:'absolute', bottom:0, left:0, right:0, height:2, background:'rgba(255,255,255,.08)' }}>
        <div style={{ width:`${progress}%`, height:'100%', background:t.grad }}/>
      </div>}
    </div>
  );
}

function Notif({ t, title, body, time, ava, icon }) {
  return (
    <div style={{ padding:'12px 14px', background:'#fff', border:`1px solid ${t.border}`, borderRadius:12, display:'flex', alignItems:'flex-start', gap:12 }}>
      <div style={{ width:36, height:36, borderRadius:10, background: ava? t.grad : t.gradSoft, color: ava? '#fff' : t.accent, display:'flex', alignItems:'center', justifyContent:'center', fontWeight:700, fontSize: ava? 12 : 15, fontFamily: ava? t.serif : t.font, fontStyle: ava? 'italic' : 'normal', flexShrink:0 }}>{ava||icon}</div>
      <div style={{ flex:1 }}>
        <div style={{ fontSize:13, fontWeight:700, color:t.ink }}>{title}</div>
        <div style={{ fontSize:12, color:t.ink3, marginTop:2 }}>{body}</div>
        <div style={{ fontSize:10.5, fontFamily:t.mono, color:t.ink4, marginTop:6 }}>{time}</div>
      </div>
      <div style={{ width:7, height:7, borderRadius:'50%', background:t.accent, flexShrink:0, marginTop:6 }}/>
    </div>
  );
}

function LinP({ t, label, pct, speed, eta, done, indeterminate, segments }) {
  return (
    <div>
      <div style={{ display:'flex', justifyContent:'space-between', alignItems:'baseline', marginBottom:6 }}>
        <span style={{ fontSize:12, fontWeight:600, color:t.ink2 }}>{label}</span>
        {pct!=null && <span style={{ fontSize:10.5, fontFamily:t.mono, color: done? t.success : t.accent, fontWeight:700 }}>{done? '✓ 100%' : `${pct}%`}</span>}
      </div>
      <div style={{ height:6, background:t.border, borderRadius:4, overflow:'hidden', position:'relative' }}>
        {indeterminate
          ? <div style={{ position:'absolute', inset:0, background:t.grad, width:'40%', borderRadius:4, animation:'slide 1.4s ease-in-out infinite' }}/>
          : segments
            ? <div style={{ display:'flex', height:'100%', gap:2 }}>
                {[100,100,100,60,0,0].map((v,i)=>(<div key={i} style={{ flex:1, background: v===0? 'transparent' : v<100? t.accent2 : t.accent, opacity:v<100?.9:1 }}/>))}
              </div>
            : <div style={{ width:`${pct}%`, height:'100%', background: done? t.success : t.grad, borderRadius:4 }}/>
        }
      </div>
      {speed && <div style={{ fontSize:10.5, fontFamily:t.mono, color:t.ink4, marginTop:4 }}>{speed} · còn {eta}</div>}
      <style>{`@keyframes slide { 0%{left:-40%} 100%{left:100%} }`}</style>
    </div>
  );
}

function Circ({ t, pct=0, size=60, label, ind, done }) {
  const r = (size-8)/2;
  const c = 2*Math.PI*r;
  const dash = c * (pct/100);
  return (
    <div style={{ position:'relative', width:size, height:size }}>
      <svg width={size} height={size}>
        <circle cx={size/2} cy={size/2} r={r} fill="none" stroke={t.border} strokeWidth="4"/>
        <defs>
          <linearGradient id={`g${size}`} x1="0" y1="0" x2="1" y2="1">
            <stop offset="0%" stopColor="#FF3D7F"/>
            <stop offset="50%" stopColor="#FF5B2E"/>
            <stop offset="100%" stopColor="#FFAF1D"/>
          </linearGradient>
        </defs>
        <circle cx={size/2} cy={size/2} r={r} fill="none"
          stroke={done? t.success : `url(#g${size})`} strokeWidth="4"
          strokeDasharray={`${dash} ${c}`} strokeDashoffset={0} strokeLinecap="round"
          transform={`rotate(-90 ${size/2} ${size/2})`}
          style={ind? { animation:'spin 1.2s linear infinite', transformOrigin:'center', transformBox:'fill-box' } : {}}/>
      </svg>
      {label && <div style={{ position:'absolute', inset:0, display:'flex', alignItems:'center', justifyContent:'center', fontSize:12, fontWeight:700, fontFamily:t.mono, color:t.ink }}>{label}</div>}
      <style>{`@keyframes spin { to { transform: rotate(360deg); } }`}</style>
    </div>
  );
}

function Stepper({ t, steps, current }) {
  return (
    <div style={{ display:'flex', flexDirection:'column', gap:0 }}>
      {steps.map((s,i)=>{
        const done = i<current, active = i===current;
        return (
          <div key={s} style={{ display:'flex', alignItems:'center', gap:12, position:'relative' }}>
            <div style={{ width:22, height:22, borderRadius:'50%', background: done? t.grad : active? '#fff' : t.border, border: active? `2px solid ${t.accent}` : 'none', color: done? '#fff' : active? t.accent : t.ink4, display:'flex', alignItems:'center', justifyContent:'center', fontSize:11, fontWeight:700, flexShrink:0, zIndex:1 }}>
              {done? '✓' : i+1}
            </div>
            <div style={{ padding:'8px 0', fontSize:12.5, fontWeight: active? 700 : 500, color: done||active? t.ink : t.ink3 }}>{s}</div>
            {i<steps.length-1 && <div style={{ position:'absolute', left:10, top:22, bottom:-10, width:2, background: done? t.accent : t.border }}/>}
          </div>
        );
      })}
    </div>
  );
}

function DSSkel({ t, w, h }) {
  return <div style={{ width:w, height:h, borderRadius:4, background:`linear-gradient(90deg, ${t.border} 0%, ${t.bg} 50%, ${t.border} 100%)`, backgroundSize:'200% 100%', animation:'skel 1.4s ease-in-out infinite' }}>
    <style>{`@keyframes skel { 0%{background-position:200% 0} 100%{background-position:-200% 0} }`}</style>
  </div>;
}

Object.assign(window, { ComponentsCard2 });
