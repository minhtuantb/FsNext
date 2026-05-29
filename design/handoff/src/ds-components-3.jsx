// ds-components-3.jsx — Component library · Part 3
// Modal, Drawer, Menu, Popover, Tabs, Breadcrumbs, Pagination, Empty state

function ComponentsCard3() {
  const t = T_A_LIGHT;
  return (
    <div style={{ width:'100%', height:'100%', padding:'48px 56px', background:t.bg, color:t.ink, fontFamily:t.font, overflow:'auto', boxSizing:'border-box' }}>
      <DSHeader t={t}
        eyebrow="Components · 04 · Overlays & Nav"
        title={<>Overlay & điều <span style={{ fontStyle:'italic', color:t.accent }}>hướng.</span></>}
        sub="Modal, drawer, menu, popover, tabs, breadcrumb, pagination, empty state — cấu trúc lớn hơn."
      />

      <div style={{ display:'grid', gridTemplateColumns:'1fr', gap:20, marginTop:32 }}>

        {/* MODAL + DRAWER */}
        <div style={{ display:'grid', gridTemplateColumns:'1.2fr 1fr', gap:20 }}>
          <DSFCard t={t} title="Modal · Dialog" sub="Confirmation · form · full flow">
            <div style={{ background:'rgba(14,14,18,.5)', borderRadius:12, padding:28, display:'flex', alignItems:'center', justifyContent:'center', minHeight:340 }}>
              <div style={{ width:'100%', maxWidth:460, background:'#fff', borderRadius:16, boxShadow:'0 24px 60px rgba(14,14,18,.22)', overflow:'hidden' }}>
                <div style={{ padding:'20px 24px 14px', borderBottom:`1px solid ${t.border}`, display:'flex', alignItems:'flex-start', gap:14 }}>
                  <div style={{ width:36, height:36, borderRadius:10, background:t.dangerSoft, color:t.danger, display:'flex', alignItems:'center', justifyContent:'center', fontSize:16, fontWeight:700 }}>⚠</div>
                  <div style={{ flex:1 }}>
                    <div style={{ fontFamily:t.serif, fontSize:22, fontWeight:400, letterSpacing:'-.02em', lineHeight:1.15 }}>Xoá vĩnh viễn <span style={{ fontStyle:'italic', color:t.danger }}>3 file?</span></div>
                    <div style={{ fontSize:12.5, color:t.ink3, marginTop:6 }}>Hành động này không thể hoàn tác. File sẽ được xoá khỏi cloud và mọi thiết bị đã đồng bộ.</div>
                  </div>
                  <span style={{ color:t.ink4, fontSize:16, cursor:'pointer' }}>✕</span>
                </div>
                <div style={{ padding:'14px 24px 16px' }}>
                  {['Oppenheimer_4K.mkv · 18.2 GB','Dune_PartTwo.mkv · 16.8 GB','Interstellar_OST.flac · 820 MB'].map(f=>(
                    <div key={f} style={{ padding:'8px 0', fontSize:12.5, fontFamily:t.mono, color:t.ink2, borderBottom:`1px dashed ${t.border}` }}>• {f}</div>
                  ))}
                </div>
                <div style={{ padding:'14px 24px 18px', display:'flex', gap:10, justifyContent:'flex-end', background:t.bg }}>
                  <button style={{ height:38, padding:'0 18px', borderRadius:10, border:`1.5px solid ${t.borderStrong}`, background:'#fff', fontSize:13, fontWeight:600 }}>Huỷ</button>
                  <button style={{ height:38, padding:'0 18px', borderRadius:10, border:'none', background:t.danger, color:'#fff', fontSize:13, fontWeight:600 }}>Xoá vĩnh viễn</button>
                </div>
              </div>
            </div>
            <div style={{ marginTop:10, fontSize:11, color:t.ink4, fontFamily:t.mono, textAlign:'center' }}>Backdrop: rgba(14,14,18,.5) · max-width 460 · radius 16 · shadow e3</div>
          </DSFCard>

          <DSFCard t={t} title="Drawer · Side panel" sub="Chi tiết file · cài đặt">
            <div style={{ height:340, background:t.bg, borderRadius:12, position:'relative', overflow:'hidden', border:`1px solid ${t.border}` }}>
              <div style={{ position:'absolute', inset:0, background:'rgba(14,14,18,.3)' }}/>
              <div style={{ position:'absolute', right:0, top:0, bottom:0, width:300, background:'#fff', boxShadow:'-20px 0 40px rgba(0,0,0,.12)', padding:'20px 22px' }}>
                <div style={{ display:'flex', justifyContent:'space-between', alignItems:'center' }}>
                  <div style={{ fontSize:13, fontWeight:700 }}>Chi tiết file</div>
                  <span style={{ color:t.ink4, cursor:'pointer' }}>✕</span>
                </div>
                <div style={{ width:'100%', aspectRatio:'16/10', marginTop:14, borderRadius:8, background:'linear-gradient(135deg,#1a1a2e,#E94560)' }}/>
                <div style={{ fontFamily:t.serif, fontSize:22, fontWeight:400, fontStyle:'italic', marginTop:12, letterSpacing:'-.02em' }}>Oppenheimer</div>
                <div style={{ fontSize:11, fontFamily:t.mono, color:t.ink4, marginTop:4 }}>18.2 GB · 4K IMAX · MKV</div>
                <div style={{ marginTop:14, display:'grid', gridTemplateColumns:'90px 1fr', rowGap:8, fontSize:11.5 }}>
                  <span style={{ color:t.ink4 }}>Tải lên</span><span style={{ color:t.ink2 }}>12/03/2026</span>
                  <span style={{ color:t.ink4 }}>Thuộc</span><span style={{ color:t.ink2 }}>/Phim/2026</span>
                  <span style={{ color:t.ink4 }}>Chia sẻ</span><span style={{ color:t.accent }}>3 link đang hoạt động</span>
                </div>
              </div>
            </div>
          </DSFCard>
        </div>

        {/* MENU + POPOVER + CONTEXT */}
        <DSFCard t={t} title="Menu · Popover · Context menu" sub="Dropdown · action menu · right-click">
          <div style={{ display:'grid', gridTemplateColumns:'1fr 1fr 1fr', gap:24 }}>
            {/* dropdown menu */}
            <div>
              <DSSubLbl t={t}>Dropdown menu</DSSubLbl>
              <div style={{ background:'#fff', border:`1px solid ${t.border}`, borderRadius:10, boxShadow:'0 8px 24px rgba(14,14,18,.08)', padding:'6px 0', width:240 }}>
                <MI t={t} ic="↓">Tải xuống<span style={{ marginLeft:'auto', fontSize:10, fontFamily:t.mono, color:t.ink4 }}>Ctrl+D</span></MI>
                <MI t={t} ic="⇧">Chia sẻ link<span style={{ marginLeft:'auto', fontSize:10, fontFamily:t.mono, color:t.ink4 }}>Ctrl+L</span></MI>
                <MI t={t} ic="★">Gắn sao</MI>
                <MI t={t} ic="✎">Đổi tên<span style={{ marginLeft:'auto', fontSize:10, fontFamily:t.mono, color:t.ink4 }}>F2</span></MI>
                <MIDiv t={t}/>
                <MI t={t} ic="📂">Di chuyển...</MI>
                <MI t={t} ic="⎘">Sao chép...</MI>
                <MIDiv t={t}/>
                <MI t={t} ic="✕" danger>Xoá<span style={{ marginLeft:'auto', fontSize:10, fontFamily:t.mono, color:t.danger }}>Del</span></MI>
              </div>
            </div>

            {/* popover */}
            <div>
              <DSSubLbl t={t}>Popover · Rich</DSSubLbl>
              <div style={{ background:'#fff', border:`1px solid ${t.border}`, borderRadius:12, boxShadow:'0 8px 24px rgba(14,14,18,.08)', padding:18, width:260 }}>
                <div style={{ fontSize:13, fontWeight:700 }}>Chia sẻ qua link</div>
                <div style={{ fontSize:11.5, color:t.ink3, marginTop:4 }}>Bất kỳ ai có link đều có thể xem</div>
                <div style={{ display:'flex', gap:6, marginTop:12, background:t.bg, border:`1px solid ${t.border}`, borderRadius:8, padding:'8px 10px' }}>
                  <span style={{ flex:1, fontSize:11, fontFamily:t.mono, color:t.ink2, whiteSpace:'nowrap', overflow:'hidden', textOverflow:'ellipsis' }}>fshare.vn/file/x8K2m</span>
                  <span style={{ fontSize:11, color:t.accent, fontWeight:700 }}>Copy</span>
                </div>
                <div style={{ display:'flex', gap:8, marginTop:12 }}>
                  <span style={{ flex:1, padding:'6px 10px', background:t.gradSoft, color:t.accent, borderRadius:6, fontSize:11, fontWeight:700, textAlign:'center' }}>⚙ Cài đặt</span>
                  <span style={{ flex:1, padding:'6px 10px', background:t.ink, color:'#fff', borderRadius:6, fontSize:11, fontWeight:700, textAlign:'center' }}>QR</span>
                </div>
              </div>
            </div>

            {/* context menu */}
            <div>
              <DSSubLbl t={t}>Context menu (right-click)</DSSubLbl>
              <div style={{ background:'#fff', border:`1px solid ${t.border}`, borderRadius:8, boxShadow:'0 8px 24px rgba(14,14,18,.1)', padding:'4px 0', width:220, fontSize:12 }}>
                <MI t={t} compact>Mở</MI>
                <MI t={t} compact>Mở bằng ▸</MI>
                <MI t={t} compact>Xem trước</MI>
                <MIDiv t={t}/>
                <MI t={t} compact>Cắt</MI>
                <MI t={t} compact>Sao chép</MI>
                <MI t={t} compact>Dán</MI>
                <MIDiv t={t}/>
                <MI t={t} compact danger>Xoá</MI>
                <MI t={t} compact>Thuộc tính</MI>
              </div>
            </div>
          </div>
        </DSFCard>

        {/* TABS + BREADCRUMB + PAGINATION */}
        <DSFCard t={t} title="Navigation" sub="Tabs · segmented · breadcrumb · pagination">
          <div style={{ display:'grid', gridTemplateColumns:'1fr 1fr', gap:24 }}>
            <div>
              <DSSubLbl t={t}>Tabs — underline</DSSubLbl>
              <div style={{ display:'flex', gap:0, borderBottom:`1px solid ${t.border}`, marginBottom:18 }}>
                {['Tất cả · 284','Phim · 42','Nhạc · 88','Tài liệu · 154'].map((tab,i)=>(
                  <div key={tab} style={{ padding:'10px 14px', fontSize:13, fontWeight: i===1? 700:500, color: i===1? t.ink : t.ink3, borderBottom: i===1? `2px solid ${t.accent}` : '2px solid transparent', marginBottom:-1, cursor:'pointer' }}>{tab}</div>
                ))}
              </div>

              <DSSubLbl t={t}>Tabs — pills</DSSubLbl>
              <div style={{ display:'flex', gap:4, padding:4, background:t.bg, borderRadius:10, marginBottom:18, width:'fit-content' }}>
                {['Ngày','Tuần','Tháng','Năm'].map((x,i)=>(
                  <div key={x} style={{ padding:'6px 14px', borderRadius:7, background: i===2? '#fff' : 'transparent', fontSize:12, fontWeight: i===2? 700:500, color: i===2? t.ink : t.ink3, boxShadow: i===2? '0 1px 3px rgba(0,0,0,.06)' : 'none', cursor:'pointer' }}>{x}</div>
                ))}
              </div>

              <DSSubLbl t={t}>Segmented</DSSubLbl>
              <div style={{ display:'inline-flex', border:`1.5px solid ${t.border}`, borderRadius:10, padding:2, background:'#fff' }}>
                {['Lưới','Danh sách','Cột'].map((x,i)=>(
                  <div key={x} style={{ padding:'6px 14px', fontSize:12, fontWeight: i===0?700:500, background: i===0? t.grad : 'transparent', color: i===0? '#fff' : t.ink2, borderRadius:7, cursor:'pointer' }}>{x}</div>
                ))}
              </div>
            </div>

            <div>
              <DSSubLbl t={t}>Breadcrumb</DSSubLbl>
              <div style={{ display:'flex', alignItems:'center', gap:6, fontSize:13, marginBottom:22, flexWrap:'wrap' }}>
                {['Fshare','Phim','2026','IMAX Remux'].map((x,i,a)=>(
                  <React.Fragment key={x}>
                    <span style={{ color: i===a.length-1? t.ink : t.ink3, fontWeight: i===a.length-1? 700:500, cursor:'pointer' }}>{i===0? '🏠 '+x : x}</span>
                    {i<a.length-1 && <span style={{ color:t.ink4 }}>›</span>}
                  </React.Fragment>
                ))}
              </div>

              <DSSubLbl t={t}>Pagination</DSSubLbl>
              <div style={{ display:'flex', gap:4, alignItems:'center', marginBottom:18 }}>
                <PBtn t={t}>‹</PBtn>
                <PBtn t={t}>1</PBtn>
                <PBtn t={t}>2</PBtn>
                <PBtn t={t} active>3</PBtn>
                <PBtn t={t}>4</PBtn>
                <PBtn t={t}>5</PBtn>
                <span style={{ padding:'0 6px', color:t.ink4 }}>…</span>
                <PBtn t={t}>24</PBtn>
                <PBtn t={t}>›</PBtn>
              </div>

              <DSSubLbl t={t}>Result count + Load more</DSSubLbl>
              <div style={{ padding:'12px 14px', background:t.bg, borderRadius:10, display:'flex', alignItems:'center', gap:12, fontSize:12.5 }}>
                <span style={{ color:t.ink3 }}>Hiện <b style={{ color:t.ink, fontFamily:t.mono }}>1–24</b> trong <b style={{ color:t.ink, fontFamily:t.mono }}>284</b> kết quả</span>
                <div style={{ flex:1 }}/>
                <span style={{ color:t.accent, fontWeight:700, cursor:'pointer' }}>Tải thêm ↓</span>
              </div>
            </div>
          </div>
        </DSFCard>

        {/* EMPTY STATE + ERROR + LOADING */}
        <DSFCard t={t} title="Empty · Error · Loading" sub="Patterns khi không có dữ liệu">
          <div style={{ display:'grid', gridTemplateColumns:'1fr 1fr 1fr', gap:18 }}>
            <EmptyState t={t}
              illus={<div style={{ fontSize:48, lineHeight:1 }}>🗂</div>}
              title="Chưa có file nào"
              desc="Kéo thả file từ máy tính hoặc bấm nút dưới để bắt đầu."
              cta="Tải lên file đầu tiên"
            />
            <EmptyState t={t}
              illus={<div style={{ width:72, height:72, borderRadius:'50%', background:t.dangerSoft, display:'flex', alignItems:'center', justifyContent:'center', fontSize:30, color:t.danger, fontWeight:700, margin:'0 auto' }}>✕</div>}
              title="Không tải được"
              desc="Kết nối mạng chập chờn. Đã thử 3 lần."
              cta="Thử lại"
              danger
            />
            <EmptyState t={t}
              illus={<div style={{ display:'flex', flexDirection:'column', gap:6, width:'80%', margin:'0 auto' }}>
                <DSSkel t={t} w="100%" h={10}/><DSSkel t={t} w="80%" h={10}/><DSSkel t={t} w="60%" h={10}/>
              </div>}
              title="Đang tải..."
              desc="Lấy danh sách file từ máy chủ"
              noCta
            />
          </div>
        </DSFCard>

      </div>
    </div>
  );
}

function MI({ t, ic, danger, compact, children }) {
  return (
    <div style={{ padding: compact? '6px 14px' : '8px 14px', display:'flex', alignItems:'center', gap:10, fontSize:13, color: danger? t.danger : t.ink2, cursor:'pointer' }}>
      {ic && <span style={{ fontSize:13, width:16, textAlign:'center', color: danger? t.danger : t.ink3 }}>{ic}</span>}
      {children}
    </div>
  );
}
function MIDiv({ t }) {
  return <div style={{ height:1, background:t.border, margin:'4px 0' }}/>;
}
function PBtn({ t, active, children }) {
  return (
    <div style={{
      minWidth:32, height:32, padding:'0 10px', borderRadius:8,
      background: active? t.grad : '#fff',
      border: active? 'none' : `1px solid ${t.border}`,
      color: active? '#fff' : t.ink2,
      display:'flex', alignItems:'center', justifyContent:'center',
      fontSize:12.5, fontWeight:600, cursor:'pointer',
      boxShadow: active? t.shadow : 'none',
    }}>{children}</div>
  );
}

function EmptyState({ t, illus, title, desc, cta, danger, noCta }) {
  return (
    <div style={{ padding:'28px 20px', border:`1.5px dashed ${t.border}`, borderRadius:14, textAlign:'center', background:t.panel }}>
      <div style={{ marginBottom:14 }}>{illus}</div>
      <div style={{ fontFamily:t.serif, fontSize:22, fontStyle:'italic', letterSpacing:'-.02em', lineHeight:1.2 }}>{title}</div>
      <div style={{ fontSize:12, color:t.ink3, marginTop:8, maxWidth:260, marginLeft:'auto', marginRight:'auto' }}>{desc}</div>
      {!noCta && <button style={{ marginTop:14, height:36, padding:'0 18px', borderRadius:10, border:'none', background: danger? t.danger : t.grad, color:'#fff', fontSize:12.5, fontWeight:700, cursor:'pointer', boxShadow: danger? 'none' : t.shadow }}>{cta}</button>}
    </div>
  );
}

function DSSkel({ t, w, h }) {
  return <div style={{ width:w, height:h, borderRadius:4, background:`linear-gradient(90deg, ${t.border} 0%, ${t.bg} 50%, ${t.border} 100%)`, backgroundSize:'200% 100%', animation:'skel2 1.4s ease-in-out infinite' }}>
    <style>{`@keyframes skel2 { 0%{background-position:200% 0} 100%{background-position:-200% 0} }`}</style>
  </div>;
}

Object.assign(window, { ComponentsCard3 });
