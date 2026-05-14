// ds-components-1.jsx — Component library · Part 1
// Buttons, Inputs, Select, Toggle, Checkbox, Radio, Slider, Avatar

function ComponentsCard1() {
  const t = T_A_LIGHT;
  return (
    <div style={{
      width:'100%', height:'100%', padding:'48px 56px',
      background:t.bg, color:t.ink, fontFamily:t.font,
      overflow:'auto', boxSizing:'border-box',
    }}>
      <DSHeader t={t}
        eyebrow="Components · 02 · Inputs"
        title={<>Form <span style={{ fontStyle:'italic', color:t.accent }}>controls.</span></>}
        sub="Button, input, select, toggle, checkbox, radio, slider, avatar — mọi biến thể & trạng thái."
      />

      <div style={{ display:'grid', gridTemplateColumns:'1fr', gap:20, marginTop:32 }}>

        {/* BUTTONS */}
        <DSFCard t={t} title="Button" sub="4 variant · 3 size · states">
          <div style={{ display:'grid', gridTemplateColumns:'80px repeat(5, 1fr)', gap:14, alignItems:'center' }}>
            <div/>
            {['Primary','Secondary','Ghost','Danger','With icon'].map(h=>(
              <div key={h} style={{ fontSize:10.5, fontFamily:t.mono, color:t.ink4, letterSpacing:'.1em', textTransform:'uppercase', fontWeight:600 }}>{h}</div>
            ))}

            <DSLbl t={t}>sm · 32</DSLbl>
            <DSBtn t={t} variant="primary" size="sm">Tải xuống</DSBtn>
            <DSBtn t={t} variant="secondary" size="sm">Huỷ</DSBtn>
            <DSBtn t={t} variant="ghost" size="sm">Thêm</DSBtn>
            <DSBtn t={t} variant="danger" size="sm">Xoá</DSBtn>
            <DSBtn t={t} variant="primary" size="sm" icon="↓">Tải</DSBtn>

            <DSLbl t={t}>md · 38</DSLbl>
            <DSBtn t={t} variant="primary">Tải xuống</DSBtn>
            <DSBtn t={t} variant="secondary">Huỷ</DSBtn>
            <DSBtn t={t} variant="ghost">Thêm</DSBtn>
            <DSBtn t={t} variant="danger">Xoá vĩnh viễn</DSBtn>
            <DSBtn t={t} variant="primary" icon="↓">Tải ngay</DSBtn>

            <DSLbl t={t}>lg · 46</DSLbl>
            <DSBtn t={t} variant="primary" size="lg">Nâng cấp VIP</DSBtn>
            <DSBtn t={t} variant="secondary" size="lg">Quay lại</DSBtn>
            <DSBtn t={t} variant="ghost" size="lg">Bỏ qua</DSBtn>
            <DSBtn t={t} variant="danger" size="lg">Xoá tài khoản</DSBtn>
            <DSBtn t={t} variant="primary" size="lg" icon="↑">Tải lên</DSBtn>

            <DSLbl t={t}>states</DSLbl>
            <DSBtn t={t} variant="primary" state="hover">Hover</DSBtn>
            <DSBtn t={t} variant="primary" state="active">Active</DSBtn>
            <DSBtn t={t} variant="primary" state="loading">Đang tải</DSBtn>
            <DSBtn t={t} variant="primary" state="disabled">Disabled</DSBtn>
            <DSBtn t={t} variant="primary" state="focus">Focus</DSBtn>
          </div>

          {/* icon-only + FAB */}
          <div style={{ marginTop:22, display:'flex', gap:14, alignItems:'center' }}>
            <DSSubLbl t={t}>Icon only</DSSubLbl>
            <DSIconSq t={t} size={32}>↓</DSIconSq>
            <DSIconSq t={t} size={38}>⚙</DSIconSq>
            <DSIconSq t={t} size={46}>＋</DSIconSq>
            <div style={{ width:1, height:32, background:t.border, margin:'0 8px' }}/>
            <DSSubLbl t={t}>FAB</DSSubLbl>
            <div style={{ width:56, height:56, borderRadius:'50%', background:t.grad, display:'flex', alignItems:'center', justifyContent:'center', color:'#fff', fontSize:22, fontWeight:600, boxShadow:t.shadow }}>＋</div>
            <div style={{ padding:'0 22px', height:56, borderRadius:999, background:t.ink, color:'#fff', display:'flex', alignItems:'center', gap:10, fontWeight:600, fontSize:14 }}><span style={{ fontSize:18 }}>＋</span> Tạo mới</div>
          </div>
        </DSFCard>

        {/* INPUTS */}
        <DSFCard t={t} title="Input" sub="Text, password, search · với label, helper, error">
          <div style={{ display:'grid', gridTemplateColumns:'1fr 1fr 1fr', gap:18 }}>
            <FieldDemo t={t} label="Email" placeholder="ban@fpt.com" help="Dùng email công ty để xác minh"/>
            <FieldDemo t={t} label="Mật khẩu" type="password" value="••••••••••"/>
            <FieldDemo t={t} label="Tìm kiếm" icon="⌕" placeholder="Tìm file, thư mục..."/>
            <FieldDemo t={t} label="Số điện thoại" value="+84 906 123 456" state="valid" help="✓ Đã xác minh"/>
            <FieldDemo t={t} label="Mã OTP" value="42 8" state="error" help="Mã không đúng, thử lại"/>
            <FieldDemo t={t} label="Readonly" value="fshare.vn/abc123" readonly/>
          </div>
          <div style={{ marginTop:18 }}>
            <div style={{ fontSize:11.5, fontWeight:600, color:t.ink2, marginBottom:6 }}>Mô tả file</div>
            <textarea readOnly value="Bản 4K IMAX remux, âm thanh Atmos 7.1, phụ đề Việt + Anh đã tích hợp. Tổng dung lượng 18.2 GB." style={{
              width:'100%', minHeight:80, padding:'12px 14px', border:`1.5px solid ${t.border}`,
              borderRadius:10, fontFamily:t.font, fontSize:13, color:t.ink2, resize:'none', outline:'none',
            }}/>
          </div>
        </DSFCard>

        {/* SELECT + TOGGLES */}
        <div style={{ display:'grid', gridTemplateColumns:'1.2fr 1fr', gap:20 }}>
          <DSFCard t={t} title="Select & Dropdown" sub="Native-feel với custom chevron">
            <div style={{ display:'flex', flexDirection:'column', gap:12 }}>
              <FakeSelect t={t} label="Sắp xếp" value="Mới thêm"/>
              <FakeSelect t={t} label="Loại file" value="Tất cả" hasMulti/>
              <FakeSelect t={t} label="Disabled" value="Không có tuỳ chọn" disabled/>
              {/* open state */}
              <div>
                <div style={{ fontSize:11.5, fontWeight:600, color:t.ink2, marginBottom:6 }}>Đang mở</div>
                <div style={{ position:'relative' }}>
                  <div style={{ padding:'10px 14px', border:`1.5px solid ${t.accent}`, borderRadius:10, fontSize:13, fontWeight:500, display:'flex', alignItems:'center', justifyContent:'space-between', background:'#fff' }}>
                    <span>Thư mục đích: /Phim/2026</span><span style={{ color:t.ink4 }}>▾</span>
                  </div>
                  <div style={{ position:'absolute', top:'calc(100% + 4px)', left:0, right:0, background:'#fff', border:`1px solid ${t.border}`, borderRadius:10, boxShadow:'0 8px 24px rgba(14,14,18,.08)', overflow:'hidden', zIndex:1 }}>
                    {['/Phim/2026','/Phim/2025 ✓','/Tài liệu','/Ảnh/Gia đình'].map((x,i)=>(
                      <div key={i} style={{ padding:'10px 14px', fontSize:13, background: i===1? t.gradSoft : 'transparent', color: i===1? t.accent : t.ink2, fontWeight: i===1? 600 : 500 }}>{x}</div>
                    ))}
                  </div>
                </div>
              </div>
            </div>
          </DSFCard>

          <DSFCard t={t} title="Toggle · Checkbox · Radio" sub="Binary & multi-choice">
            <DSSubLbl t={t}>Toggle</DSSubLbl>
            <div style={{ display:'flex', flexDirection:'column', gap:10, marginBottom:18 }}>
              <ToggleRow t={t} label="Tự động đồng bộ" on/>
              <ToggleRow t={t} label="Thông báo khi xong"/>
              <ToggleRow t={t} label="Mã hoá đầu cuối" on disabled/>
            </div>

            <DSSubLbl t={t}>Checkbox</DSSubLbl>
            <div style={{ display:'flex', flexDirection:'column', gap:9, marginBottom:18 }}>
              <Check t={t} label="Gửi thông báo qua email" on/>
              <Check t={t} label="Gửi qua Zalo"/>
              <Check t={t} label="Áp dụng cho tất cả" indet/>
              <Check t={t} label="Không thể tắt" on disabled/>
            </div>

            <DSSubLbl t={t}>Radio</DSSubLbl>
            <div style={{ display:'flex', flexDirection:'column', gap:9 }}>
              <Radio t={t} label="Link công khai" on/>
              <Radio t={t} label="Yêu cầu mật khẩu"/>
              <Radio t={t} label="Chỉ người được mời"/>
            </div>
          </DSFCard>
        </div>

        {/* SLIDER + AVATAR */}
        <div style={{ display:'grid', gridTemplateColumns:'1.3fr 1fr', gap:20 }}>
          <DSFCard t={t} title="Slider" sub="Tốc độ · giới hạn băng thông">
            <div style={{ padding:'8px 4px' }}>
              <div style={{ display:'flex', justifyContent:'space-between', marginBottom:10, fontSize:12, color:t.ink3 }}>
                <span>Giới hạn tốc độ tải</span>
                <span style={{ fontFamily:t.mono, color:t.accent, fontWeight:700 }}>42 MB/s</span>
              </div>
              <Slider t={t} pct={62}/>
              <div style={{ display:'flex', justifyContent:'space-between', marginTop:6, fontSize:10.5, fontFamily:t.mono, color:t.ink4 }}>
                <span>0</span><span>20</span><span>40</span><span>60</span><span>80 MB/s</span>
              </div>
            </div>
            <div style={{ padding:'18px 4px 4px' }}>
              <div style={{ display:'flex', justifyContent:'space-between', marginBottom:10, fontSize:12, color:t.ink3 }}>
                <span>Thời gian lưu link</span>
                <span style={{ fontFamily:t.mono, color:t.accent, fontWeight:700 }}>7 ngày</span>
              </div>
              {/* range slider */}
              <div style={{ height:4, background:t.border, borderRadius:3, position:'relative' }}>
                <div style={{ position:'absolute', left:'15%', right:'40%', height:'100%', background:t.grad, borderRadius:3 }}/>
                <Thumb t={t} left="15%"/>
                <Thumb t={t} left="60%"/>
              </div>
            </div>
          </DSFCard>

          <DSFCard t={t} title="Avatar" sub="5 size · fallback · online indicator · group stack">
            <div style={{ display:'flex', alignItems:'center', gap:14, marginBottom:20 }}>
              <Ava t={t} size={24}>M</Ava>
              <Ava t={t} size={32}>M</Ava>
              <Ava t={t} size={40}>M</Ava>
              <Ava t={t} size={56}>M</Ava>
              <Ava t={t} size={72}>M</Ava>
            </div>

            <DSSubLbl t={t}>Variants</DSSubLbl>
            <div style={{ display:'flex', alignItems:'center', gap:14, marginBottom:20, flexWrap:'wrap' }}>
              <Ava t={t} size={40} grad>M</Ava>
              <Ava t={t} size={40} plain>TH</Ava>
              <Ava t={t} size={40} dark>LP</Ava>
              <div style={{ position:'relative' }}>
                <Ava t={t} size={40} grad>M</Ava>
                <div style={{ position:'absolute', bottom:-1, right:-1, width:12, height:12, borderRadius:'50%', background:t.success, border:'2px solid #fff' }}/>
              </div>
              <div style={{ position:'relative' }}>
                <Ava t={t} size={40} plain>KT</Ava>
                <div style={{ position:'absolute', bottom:-1, right:-1, width:12, height:12, borderRadius:'50%', background:'#c9c9d4', border:'2px solid #fff' }}/>
              </div>
            </div>

            <DSSubLbl t={t}>Group stack</DSSubLbl>
            <div style={{ display:'flex', alignItems:'center' }}>
              {['M','TH','LP','KT'].map((n,i)=>(
                <div key={n} style={{ marginLeft: i===0? 0 : -10 }}>
                  <Ava t={t} size={36} grad={i===0} plain={i===1} dark={i===2} ring>{n}</Ava>
                </div>
              ))}
              <div style={{ marginLeft:-10, width:36, height:36, borderRadius:'50%', background:'#fff', border:`2px solid #fff`, boxShadow:`inset 0 0 0 1px ${t.border}`, display:'flex', alignItems:'center', justifyContent:'center', fontSize:11, fontFamily:t.mono, color:t.ink3, fontWeight:700 }}>+8</div>
            </div>
          </DSFCard>
        </div>

      </div>
    </div>
  );
}

// ─── atoms ───
function DSLbl({ t, children }) {
  return <div style={{ fontSize:10.5, fontFamily:t.mono, color:t.ink4, letterSpacing:'.08em', fontWeight:600, textTransform:'uppercase' }}>{children}</div>;
}
function DSBtn({ t, variant='primary', size='md', state, icon, children }) {
  const sizes = { sm:{h:32,px:14,fs:12.5}, md:{h:38,px:18,fs:13.5}, lg:{h:46,px:22,fs:14.5} };
  const s = sizes[size];
  const styles = {
    primary:{ background: state==='hover'? 'linear-gradient(135deg,#FF2D75,#FF4B1F 50%,#FF9F0D)' : t.grad, color:'#fff', border:'none', boxShadow: state==='focus'? `0 0 0 3px rgba(255,91,46,.25)`: t.shadow },
    secondary:{ background:'#fff', color:t.ink, border:`1.5px solid ${t.borderStrong}` },
    ghost:{ background:'transparent', color:t.ink2, border:'none' },
    danger:{ background:t.danger, color:'#fff', border:'none' },
  };
  const extra = {};
  if (state==='disabled') Object.assign(extra, { opacity:.4, cursor:'not-allowed' });
  if (state==='active') extra.transform = 'translateY(1px)';
  return (
    <button style={{
      height:s.h, padding:`0 ${s.px}px`, borderRadius:10, fontSize:s.fs, fontWeight:600,
      display:'inline-flex', alignItems:'center', justifyContent:'center', gap:7,
      fontFamily:t.font, cursor:'pointer', whiteSpace:'nowrap',
      ...styles[variant], ...extra,
    }}>
      {state==='loading' && <span style={{ width:12, height:12, border:'2px solid rgba(255,255,255,.3)', borderTopColor:'#fff', borderRadius:'50%', animation:'spin 1s linear infinite' }}/>}
      {icon && <span>{icon}</span>}
      {children}
    </button>
  );
}
function DSIconSq({ t, size=38, children }) {
  return <button style={{ width:size, height:size, borderRadius:10, border:`1.5px solid ${t.borderStrong}`, background:'#fff', fontSize: size<36?13:15, color:t.ink2, display:'flex', alignItems:'center', justifyContent:'center', cursor:'pointer', fontWeight:600 }}>{children}</button>;
}

function FieldDemo({ t, label, value, placeholder, type='text', icon, help, state, readonly }) {
  const borderColor = state==='valid'? t.success : state==='error'? t.danger : t.borderStrong;
  return (
    <div>
      <div style={{ fontSize:11.5, fontWeight:600, color:t.ink2, marginBottom:6 }}>{label}</div>
      <div style={{
        display:'flex', alignItems:'center', gap:8, padding:'0 14px', height:40,
        background: readonly? t.bg : '#fff', border:`1.5px solid ${borderColor}`, borderRadius:10,
      }}>
        {icon && <span style={{ fontSize:14, color:t.ink4 }}>{icon}</span>}
        <input readOnly type={type==='password'?'text':type} value={value||''} placeholder={placeholder||''} style={{
          flex:1, border:'none', outline:'none', background:'transparent', fontSize:13, fontFamily:t.font, color: readonly? t.ink3 : t.ink,
        }}/>
      </div>
      {help && <div style={{ fontSize:10.5, color: state==='error'? t.danger : state==='valid'? t.success : t.ink4, marginTop:5, fontFamily: state? t.font : t.font }}>{help}</div>}
    </div>
  );
}

function FakeSelect({ t, label, value, hasMulti, disabled }) {
  return (
    <div style={{ opacity: disabled? .4 : 1 }}>
      <div style={{ fontSize:11.5, fontWeight:600, color:t.ink2, marginBottom:6 }}>{label}</div>
      <div style={{ padding:'10px 14px', border:`1.5px solid ${t.borderStrong}`, borderRadius:10, fontSize:13, fontWeight:500, display:'flex', alignItems:'center', gap:8, background: disabled? t.bg : '#fff' }}>
        <span style={{ flex:1 }}>{value}</span>
        {hasMulti && <span style={{ fontSize:10, padding:'2px 7px', background:t.gradSoft, color:t.accent, borderRadius:999, fontWeight:700 }}>3</span>}
        <span style={{ color:t.ink4 }}>▾</span>
      </div>
    </div>
  );
}

function ToggleRow({ t, label, on, disabled }) {
  return (
    <div style={{ display:'flex', alignItems:'center', gap:12, opacity: disabled? .5 : 1 }}>
      <div style={{ width:36, height:20, background: on? t.grad : t.border, borderRadius:999, padding:2, display:'flex', justifyContent: on? 'flex-end':'flex-start', transition:'.18s' }}>
        <div style={{ width:16, height:16, background:'#fff', borderRadius:'50%', boxShadow:'0 1px 2px rgba(0,0,0,.2)' }}/>
      </div>
      <span style={{ fontSize:13, color:t.ink2 }}>{label}</span>
    </div>
  );
}
function Check({ t, label, on, indet, disabled }) {
  return (
    <label style={{ display:'flex', alignItems:'center', gap:10, opacity: disabled? .5 : 1 }}>
      <div style={{ width:18, height:18, borderRadius:5, border:`1.5px solid ${on||indet? t.accent : t.borderStrong}`, background: on||indet? t.grad : '#fff', display:'flex', alignItems:'center', justifyContent:'center', color:'#fff', fontSize:12, fontWeight:700 }}>{on? '✓' : indet? '—' : ''}</div>
      <span style={{ fontSize:13, color:t.ink2 }}>{label}</span>
    </label>
  );
}
function Radio({ t, label, on }) {
  return (
    <label style={{ display:'flex', alignItems:'center', gap:10 }}>
      <div style={{ width:18, height:18, borderRadius:'50%', border:`1.5px solid ${on? t.accent : t.borderStrong}`, background:'#fff', display:'flex', alignItems:'center', justifyContent:'center' }}>
        {on && <div style={{ width:9, height:9, borderRadius:'50%', background:t.grad }}/>}
      </div>
      <span style={{ fontSize:13, color:t.ink2 }}>{label}</span>
    </label>
  );
}

function Slider({ t, pct }) {
  return (
    <div style={{ height:4, background:t.border, borderRadius:3, position:'relative' }}>
      <div style={{ width:`${pct}%`, height:'100%', background:t.grad, borderRadius:3 }}/>
      <Thumb t={t} left={`${pct}%`}/>
    </div>
  );
}
function Thumb({ t, left }) {
  return <div style={{ position:'absolute', left, top:'50%', transform:'translate(-50%,-50%)', width:16, height:16, borderRadius:'50%', background:'#fff', boxShadow:`0 0 0 2px ${t.accent}, 0 2px 4px rgba(0,0,0,.15)` }}/>;
}

function Ava({ t, size=40, grad, plain, dark, ring, children }) {
  const bg = grad? t.grad : plain? '#E6E3DC' : dark? t.ink : t.grad;
  const color = plain? t.ink : '#fff';
  return (
    <div style={{
      width:size, height:size, borderRadius:'50%', background:bg, color,
      display:'flex', alignItems:'center', justifyContent:'center',
      fontWeight:700, fontSize: size*0.38, fontFamily:t.serif, fontStyle:'italic',
      boxShadow: ring? `0 0 0 2px #fff` : 'none',
    }}>{children}</div>
  );
}

Object.assign(window, { ComponentsCard1 });
