// ds-table.jsx — Data grid / table component
// Variants: default, compact, selectable, sortable, sticky header

function TableCard() {
  const t = T_A_LIGHT;
  return (
    <div style={{ width:'100%', height:'100%', padding:'48px 56px', background:t.bg, color:t.ink, fontFamily:t.font, overflow:'auto', boxSizing:'border-box' }}>
      <DSHeader t={t}
        eyebrow="Components · 06 · Data"
        title={<>Bảng & <span style={{ fontStyle:'italic', color:t.accent }}>danh sách.</span></>}
        sub="Table chuẩn cho file list · sort · select · hover · resize · sticky header · empty row."
      />

      {/* Main example */}
      <DSFCard t={t} title="File list · default" sub="Header sticky · sort · multi-select · row hover · context actions">
        <div style={{ border:`1px solid ${t.border}`, borderRadius:12, overflow:'hidden', background:'#fff' }}>
          {/* toolbar */}
          <div style={{ padding:'12px 16px', borderBottom:`1px solid ${t.border}`, display:'flex', alignItems:'center', gap:10, background:t.bg }}>
            <span style={{ fontSize:12, fontWeight:700, padding:'4px 10px', background:t.gradSoft, color:t.accent, borderRadius:999 }}>3 đã chọn</span>
            <span style={{ fontSize:11.5, color:t.ink3 }}>· 18.4 GB tổng</span>
            <div style={{ flex:1 }}/>
            <TBtn t={t} ic="download">Tải xuống</TBtn>
            <TBtn t={t} ic="share">Chia sẻ</TBtn>
            <TBtn t={t} ic="trash" danger>Xoá</TBtn>
          </div>

          {/* header */}
          <div style={{ display:'grid', gridTemplateColumns:'44px 1fr 120px 140px 160px 100px', padding:'10px 16px', borderBottom:`1px solid ${t.border}`, background:t.bg, fontSize:10.5, fontFamily:t.mono, color:t.ink4, letterSpacing:'.08em', textTransform:'uppercase', fontWeight:700 }}>
            <div><DSCheck t={t} indet/></div>
            <Th t={t} sort="asc">Tên file</Th>
            <Th t={t}>Kích thước</Th>
            <Th t={t} sort="desc">Sửa đổi</Th>
            <Th t={t}>Chia sẻ</Th>
            <div style={{ textAlign:'right' }}>#</div>
          </div>

          {/* rows */}
          {[
            { sel:true, name:'Oppenheimer_4K_IMAX.mkv', type:'video', size:'18.2 GB', mod:'12/03/2026', share:'3 link' },
            { name:'BTS_Tour_2026.mov', type:'video', size:'3.2 GB', mod:'Hôm nay 14:22', share:'—' },
            { sel:true, name:'Dune_PartTwo_Remux.mkv', type:'video', size:'16.8 GB', mod:'08/03/2026', share:'1 link', hi:true },
            { name:'concert_recording.flac', type:'music', size:'620 MB', mod:'Hôm qua', share:'—' },
            { name:'Báo_cáo_quý4_v3.pdf', type:'pdf', size:'2.4 MB', mod:'15:08', share:'2 link' },
            { sel:true, name:'Portfolio_2026.zip', type:'archive', size:'1.2 GB', mod:'20/03/2026', share:'—' },
            { name:'Album_cưới_RAW.zip', type:'archive', size:'8.1 GB', mod:'02/03/2026', share:'4 link', shareHi:true },
          ].map((r,i)=>(
            <TRow key={i} t={t} {...r}/>
          ))}

          {/* footer */}
          <div style={{ padding:'12px 16px', background:t.bg, display:'flex', alignItems:'center', gap:12, fontSize:12 }}>
            <span style={{ color:t.ink3 }}>Hiện <b style={{ color:t.ink, fontFamily:t.mono }}>7</b> / 284 file · tổng <b style={{ color:t.ink, fontFamily:t.mono }}>48.5 GB</b></span>
            <div style={{ flex:1 }}/>
            <select style={{ border:`1px solid ${t.border}`, borderRadius:7, padding:'4px 8px', fontSize:11.5, fontFamily:t.mono, background:'#fff' }}>
              <option>20 / trang</option><option>50 / trang</option>
            </select>
            <div style={{ display:'flex', gap:2 }}>
              {['‹','1','2','3','›'].map((x,i)=>(
                <div key={i} style={{ minWidth:26, height:26, padding:'0 8px', borderRadius:6, background: x==='2'? t.grad : '#fff', border: x==='2'? 'none' : `1px solid ${t.border}`, color: x==='2'? '#fff' : t.ink2, display:'flex', alignItems:'center', justifyContent:'center', fontSize:11.5, fontWeight:600 }}>{x}</div>
              ))}
            </div>
          </div>
        </div>
      </DSFCard>

      <div style={{ marginTop:20, display:'grid', gridTemplateColumns:'1.3fr 1fr', gap:16 }}>
        {/* Compact + inline edit */}
        <DSFCard t={t} title="Compact · Inline edit" sub="Row cao 32px · dùng khi nhiều hàng">
          <div style={{ border:`1px solid ${t.border}`, borderRadius:10, overflow:'hidden', background:'#fff' }}>
            <div style={{ display:'grid', gridTemplateColumns:'1fr 80px 100px 30px', padding:'8px 12px', background:t.bg, fontSize:10, fontFamily:t.mono, color:t.ink4, fontWeight:700, textTransform:'uppercase', letterSpacing:'.08em', borderBottom:`1px solid ${t.border}` }}>
              <div>Link</div><div>Lượt tải</div><div>Hết hạn</div><div/>
            </div>
            {[
              ['fshare.vn/x8K2m','1,284','7 ngày',false],
              ['fshare.vn/a9P4q','342','1 ngày',false],
              ['fshare.vn/editing','—','—',true],
              ['fshare.vn/m2Z7w','88','30 ngày',false],
              ['fshare.vn/b4N8c','12','14 ngày',false],
            ].map((r,i)=>(
              <div key={i} style={{ display:'grid', gridTemplateColumns:'1fr 80px 100px 30px', padding:'7px 12px', fontSize:12, alignItems:'center', borderBottom: i<4? `1px solid ${t.border}` : 'none' }}>
                {r[3]
                  ? <input readOnly value="fshare.vn/nhap-link-moi" style={{ padding:'3px 6px', fontFamily:t.mono, fontSize:11.5, border:`1.5px solid ${t.accent}`, borderRadius:5, outline:'none' }}/>
                  : <span style={{ fontFamily:t.mono, color:t.accent }}>{r[0]}</span>}
                <span style={{ fontFamily:t.mono, color:t.ink2 }}>{r[1]}</span>
                <span style={{ color:t.ink3 }}>{r[2]}</span>
                <DSIcon name="more" size={14} stroke={t.ink4}/>
              </div>
            ))}
          </div>
        </DSFCard>

        {/* States */}
        <DSFCard t={t} title="Empty · Loading · Error" sub="Trạng thái đặc biệt trong table">
          <div style={{ display:'flex', flexDirection:'column', gap:10 }}>
            {/* empty */}
            <div style={{ padding:'22px 16px', background:'#fff', border:`1px dashed ${t.border}`, borderRadius:10, textAlign:'center' }}>
              <DSIcon name="folder" size={28} stroke={t.ink4}/>
              <div style={{ fontSize:13, fontWeight:700, marginTop:8 }}>Thư mục trống</div>
              <div style={{ fontSize:11, color:t.ink4, marginTop:3 }}>Kéo thả file vào đây để bắt đầu</div>
            </div>
            {/* loading */}
            <div style={{ padding:'12px 14px', background:'#fff', border:`1px solid ${t.border}`, borderRadius:10, display:'flex', flexDirection:'column', gap:8 }}>
              {[75,55,85].map((w,i)=>(
                <div key={i} style={{ display:'grid', gridTemplateColumns:'20px 1fr 50px', gap:10, alignItems:'center' }}>
                  <DSSkel t={t} w={16} h={16}/>
                  <DSSkel t={t} w={`${w}%`} h={10}/>
                  <DSSkel t={t} w={40} h={10}/>
                </div>
              ))}
            </div>
            {/* error */}
            <div style={{ padding:'14px 16px', background:t.dangerSoft, border:`1px solid ${t.danger}30`, borderRadius:10, display:'flex', alignItems:'center', gap:10 }}>
              <div style={{ width:28, height:28, borderRadius:'50%', background:t.danger, color:'#fff', display:'flex', alignItems:'center', justifyContent:'center' }}><DSIcon name="x" size={14} stroke="#fff"/></div>
              <div style={{ flex:1, fontSize:12 }}><b style={{ color:t.danger }}>Không tải được danh sách</b><div style={{ color:t.ink3, fontSize:11 }}>Kiểm tra kết nối mạng</div></div>
              <span style={{ fontSize:11.5, fontWeight:700, color:t.danger }}>Thử lại →</span>
            </div>
          </div>
        </DSFCard>
      </div>
    </div>
  );
}

function Th({ t, sort, children }) {
  return (
    <div style={{ display:'flex', alignItems:'center', gap:5, cursor:'pointer' }}>
      <span>{children}</span>
      {sort==='asc' && <span style={{ color:t.accent }}>▲</span>}
      {sort==='desc' && <span style={{ color:t.accent }}>▼</span>}
    </div>
  );
}
function TBtn({ t, ic, danger, children }) {
  return (
    <div style={{ padding:'5px 10px', borderRadius:7, background:'#fff', border:`1px solid ${t.border}`, color: danger? t.danger : t.ink2, fontSize:11.5, fontWeight:600, display:'flex', alignItems:'center', gap:5, cursor:'pointer' }}>
      <DSIcon name={ic} size={12} stroke={danger? t.danger : t.ink2}/> {children}
    </div>
  );
}
function TRow({ t, sel, hi, name, type, size, mod, share, shareHi }) {
  return (
    <div style={{ display:'grid', gridTemplateColumns:'44px 1fr 120px 140px 160px 100px', padding:'11px 16px', borderBottom:`1px solid ${t.border}`, background: hi? t.gradSoft : sel? 'rgba(255,91,46,.04)' : '#fff', alignItems:'center', fontSize:13 }}>
      <DSCheck t={t} on={sel}/>
      <div style={{ display:'flex', alignItems:'center', gap:10, minWidth:0 }}>
        <div style={{ width:28, height:28, borderRadius:7, background: hi? '#fff' : t.bg, color:t.accent, display:'flex', alignItems:'center', justifyContent:'center', flexShrink:0 }}>
          <DSIcon name={type} size={15} stroke={t.accent}/>
        </div>
        <span style={{ fontWeight:600, whiteSpace:'nowrap', overflow:'hidden', textOverflow:'ellipsis' }}>{name}</span>
      </div>
      <span style={{ fontFamily:t.mono, fontSize:12, color:t.ink2 }}>{size}</span>
      <span style={{ fontSize:12, color:t.ink3 }}>{mod}</span>
      <span>{share==='—' ? <span style={{ color:t.ink4 }}>—</span> : <span style={{ padding:'2px 8px', background: shareHi? t.grad : t.gradSoft, color: shareHi? '#fff' : t.accent, borderRadius:4, fontSize:11, fontWeight:700 }}>{share}</span>}</span>
      <div style={{ textAlign:'right' }}><DSIcon name="more" size={14} stroke={t.ink4}/></div>
    </div>
  );
}
function DSCheck({ t, on, indet }) {
  return (
    <div style={{ width:16, height:16, borderRadius:4, border:`1.5px solid ${on||indet? t.accent : t.borderStrong}`, background: on||indet? t.grad : '#fff', display:'flex', alignItems:'center', justifyContent:'center' }}>
      {on && <DSIcon name="check" size={11} stroke="#fff" sw={2.5}/>}
      {indet && <div style={{ width:8, height:2, background:'#fff' }}/>}
    </div>
  );
}

Object.assign(window, { TableCard });
