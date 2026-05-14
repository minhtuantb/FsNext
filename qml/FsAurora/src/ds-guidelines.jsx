// ds-guidelines.jsx — Content style + Do/Don't + Tokens export

function GuidelinesCard() {
  const t = T_A_LIGHT;
  return (
    <div style={{ width:'100%', height:'100%', padding:'48px 56px', background:t.bg, color:t.ink, fontFamily:t.font, overflow:'auto', boxSizing:'border-box' }}>
      <DSHeader t={t}
        eyebrow="Guidelines · 07"
        title={<>Cách dùng & <span style={{ fontStyle:'italic', color:t.accent }}>giọng điệu.</span></>}
        sub="Do / Don't cho component, format dữ liệu, copywriting voice, tokens export cho dev."
      />

      {/* DO / DONT */}
      <div style={{ marginTop:30, fontSize:11, fontFamily:t.mono, color:t.ink4, letterSpacing:'.14em', textTransform:'uppercase', fontWeight:600, marginBottom:12 }}>━ Do / Don't</div>
      <div style={{ display:'grid', gridTemplateColumns:'1fr 1fr', gap:14 }}>
        <GdDo t={t} ok title="Button — 1 primary mỗi view" desc="Chỉ có một hành động chính trong mỗi màn hình hoặc modal">
          <div style={{ display:'flex', gap:8 }}>
            <button style={{ padding:'8px 16px', background:t.grad, color:'#fff', border:'none', borderRadius:8, fontSize:12.5, fontWeight:600 }}>Nâng cấp VIP</button>
            <button style={{ padding:'8px 16px', background:'#fff', color:t.ink, border:`1.5px solid ${t.border}`, borderRadius:8, fontSize:12.5, fontWeight:600 }}>Sau</button>
          </div>
        </GdDo>
        <GdDo t={t} title="Nhiều primary cạnh nhau" desc="Người dùng bối rối khi có 2+ nút gradient — trật tự thông tin bị phá">
          <div style={{ display:'flex', gap:8 }}>
            <button style={{ padding:'8px 16px', background:t.grad, color:'#fff', border:'none', borderRadius:8, fontSize:12.5, fontWeight:600 }}>Nâng cấp</button>
            <button style={{ padding:'8px 16px', background:t.grad, color:'#fff', border:'none', borderRadius:8, fontSize:12.5, fontWeight:600 }}>Sau</button>
          </div>
        </GdDo>

        <GdDo t={t} ok title="Icon dùng stroke 1.5" desc="Mọi icon đều cùng độ đậm — tạo nhịp visual thống nhất">
          <div style={{ display:'flex', gap:14, alignItems:'center' }}>
            <DSIcon name="upload" stroke={t.ink}/>
            <DSIcon name="download" stroke={t.ink}/>
            <DSIcon name="share" stroke={t.ink}/>
          </div>
        </GdDo>
        <GdDo t={t} title="Trộn stroke 1px · 2px · fill" desc="Độ đậm không đều khiến UI 'ồn'">
          <div style={{ display:'flex', gap:14, alignItems:'center' }}>
            <DSIcon name="upload" stroke={t.ink} sw={1}/>
            <DSIcon name="download" stroke={t.ink} sw={2.5}/>
            <DSIcon name="share" stroke={t.ink} fill={t.ink}/>
          </div>
        </GdDo>

        <GdDo t={t} ok title="Lỗi — nói lý do, đưa cách xử lý" desc='"Mất kết nối mạng. Thử lại sau 3 giây…"'>
          <div style={{ padding:'10px 14px', background:t.dangerSoft, border:`1px solid ${t.danger}30`, borderRadius:8, fontSize:12.5, color:t.danger, fontWeight:500 }}>Mất kết nối mạng. Đang thử lại sau 3 giây…</div>
        </GdDo>
        <GdDo t={t} title="Lỗi mơ hồ · mã lỗi không có ngữ cảnh" desc='"Error 500. Please try again."'>
          <div style={{ padding:'10px 14px', background:t.dangerSoft, border:`1px solid ${t.danger}30`, borderRadius:8, fontSize:12.5, color:t.danger, fontWeight:500, fontFamily:t.mono }}>Error 500. Unknown state.</div>
        </GdDo>
      </div>

      {/* COPY VOICE */}
      <div style={{ marginTop:30, fontSize:11, fontFamily:t.mono, color:t.ink4, letterSpacing:'.14em', textTransform:'uppercase', fontWeight:600, marginBottom:12 }}>━ Voice & Tone</div>
      <div style={{ display:'grid', gridTemplateColumns:'1fr 1fr 1fr', gap:14 }}>
        <DSFCard t={t} title="Thân thiện" sub="Dùng 'bạn', câu ngắn, không kỹ thuật">
          <div style={{ fontSize:13, color:t.ink2, lineHeight:1.5 }}>
            "Kéo file vào đây để tải lên nhé."<br/>
            "Bạn đã dùng 287 GB."<br/>
            "Xong rồi! Link đã được sao chép."
          </div>
        </DSFCard>
        <DSFCard t={t} title="Rõ ràng" sub="Một câu = một ý, có con số cụ thể">
          <div style={{ fontSize:13, color:t.ink2, lineHeight:1.5 }}>
            "Còn 2 phút 08 giây"<br/>
            "3 file · 18.4 GB tổng"<br/>
            "Upload 62% · 18.4 MB/s"
          </div>
        </DSFCard>
        <DSFCard t={t} title="Tránh" sub="Không viết thế này">
          <div style={{ fontSize:13, color:t.ink3, lineHeight:1.5, textDecoration:'line-through' }}>
            "Hệ thống đang xử lý yêu cầu của Quý khách."<br/>
            "Vui lòng chờ trong ít phút."<br/>
            "Thao tác thành công."
          </div>
        </DSFCard>
      </div>

      {/* FORMATTING */}
      <div style={{ marginTop:30, fontSize:11, fontFamily:t.mono, color:t.ink4, letterSpacing:'.14em', textTransform:'uppercase', fontWeight:600, marginBottom:12 }}>━ Format số & ngày tháng</div>
      <div style={{ background:t.panel, border:`1px solid ${t.border}`, borderRadius:12, overflow:'hidden' }}>
        {[
          ['Kích thước','420 MB · 1.2 GB · 18.2 GB','1 số thập phân khi ≥ 1GB · dấu cách trước đơn vị','420MB · 1.234 GB · 1,200 MB'],
          ['Tốc độ','18.4 MB/s · 62 KB/s','1 số thập phân · dấu "/s"','18.40 MB/sec'],
          ['Thời gian','2 phút 08 giây · 14 giây · 1 giờ 22 phút','Viết tắt tiếng Việt, số cụ thể','2m 08s · 00:02:08'],
          ['Ngày','12/03/2026 · Hôm nay · Hôm qua 14:22','Tương đối trong 2 ngày, tuyệt đối sau đó','March 12, 2026 · 2026-03-12'],
          ['Tiền','990.000 đ · 79.000 đ/tháng','Dấu chấm phân cách · "đ" viết thường','990,000 VND · 990.000,00đ'],
          ['Số đếm lớn','1.284 lượt tải · 42K · 1.2M','Dấu chấm cho Việt · tắt gọn từ 10k','1,284 downloads'],
        ].map((r,i)=>(
          <div key={i} style={{ display:'grid', gridTemplateColumns:'140px 1.3fr 1.3fr 1fr', padding:'14px 18px', borderBottom: i<5? `1px solid ${t.border}`:'none', fontSize:12.5, alignItems:'center' }}>
            <div style={{ fontWeight:700, color:t.ink }}>{r[0]}</div>
            <div style={{ fontFamily:t.mono, color:t.success, fontWeight:600 }}>✓ {r[1]}</div>
            <div style={{ color:t.ink3, fontSize:11.5 }}>{r[2]}</div>
            <div style={{ fontFamily:t.mono, color:t.danger, fontSize:11, textDecoration:'line-through' }}>✕ {r[3]}</div>
          </div>
        ))}
      </div>

      {/* TOKENS EXPORT */}
      <div style={{ marginTop:30, fontSize:11, fontFamily:t.mono, color:t.ink4, letterSpacing:'.14em', textTransform:'uppercase', fontWeight:600, marginBottom:12 }}>━ Tokens export (cho dev)</div>
      <div style={{ display:'grid', gridTemplateColumns:'1fr 1fr', gap:14 }}>
        <DSFCard t={t} title="JSON · design-tokens.json" sub="Nguồn gốc — dùng cho Style Dictionary, Figma Tokens">
          <pre style={{ margin:0, padding:'14px 16px', background:'#0E0E12', color:'#EDEDEF', borderRadius:10, fontSize:11, fontFamily:t.mono, lineHeight:1.55, overflow:'auto', maxHeight:280 }}>
{`{
  "color": {
    "accent":  { "value": "#FF5B2E" },
    "accent2": { "value": "#FFAF1D" },
    "accent3": { "value": "#FF3D7F" },
    "ink":     { "value": "#0E0E12" },
    "bg":      { "value": "#F5F4F1" },
    "success": { "value": "#0A8A5C" },
    "warn":    { "value": "#C96A00" },
    "danger":  { "value": "#D53030" }
  },
  "space":  { "xs":4, "sm":8, "md":12, "lg":16, "xl":20, "2xl":24, "3xl":32 },
  "radius": { "sm":6, "md":10, "lg":14, "xl":20, "pill":999 },
  "motion": {
    "fast": "140ms",
    "base": "220ms",
    "slow": "360ms",
    "ease-std": "cubic-bezier(.2,0,0,1)"
  }
}`}
          </pre>
        </DSFCard>

        <DSFCard t={t} title="CSS · :root variables" sub="Dán vào stylesheet, dùng ngay với var(--fs-*)">
          <pre style={{ margin:0, padding:'14px 16px', background:'#0E0E12', color:'#EDEDEF', borderRadius:10, fontSize:11, fontFamily:t.mono, lineHeight:1.55, overflow:'auto', maxHeight:280 }}>
{`:root {
  --fs-accent:   #FF5B2E;
  --fs-accent-2: #FFAF1D;
  --fs-accent-3: #FF3D7F;
  --fs-ink:      #0E0E12;
  --fs-bg:       #F5F4F1;
  --fs-border:   #E6E3DC;

  --fs-space-sm: 8px;
  --fs-space-md: 12px;
  --fs-space-lg: 16px;

  --fs-radius-md: 10px;
  --fs-radius-lg: 14px;

  --fs-motion-base: 220ms cubic-bezier(.2,0,0,1);
  --fs-shadow-md: 0 4px 14px rgba(255,91,46,.3);
}
[data-theme="dark"] {
  --fs-bg:     #0E0E12;
  --fs-ink:    #F5F4F1;
  --fs-border: #25252E;
}`}
          </pre>
        </DSFCard>
      </div>

      {/* ACCESSIBILITY */}
      <div style={{ marginTop:30, fontSize:11, fontFamily:t.mono, color:t.ink4, letterSpacing:'.14em', textTransform:'uppercase', fontWeight:600, marginBottom:12 }}>━ Accessibility audit</div>
      <div style={{ display:'grid', gridTemplateColumns:'1.2fr 1fr', gap:14 }}>
        <DSFCard t={t} title="Contrast · WCAG" sub="Ratio tối thiểu: 4.5:1 cho body, 3:1 cho large text">
          <div style={{ display:'flex', flexDirection:'column', gap:0 }}>
            <CRow t={t} fg="#0E0E12" bg="#F5F4F1" label="Ink / BG" ratio="17.2" pass="AAA"/>
            <CRow t={t} fg="#5C5C66" bg="#F5F4F1" label="Ink3 / BG" ratio="6.8" pass="AA"/>
            <CRow t={t} fg="#8A8A94" bg="#F5F4F1" label="Ink4 / BG" ratio="3.4" pass="AA-lg" warn/>
            <CRow t={t} fg="#FFFFFF" bg="#FF5B2E" label="White / Accent" ratio="3.5" pass="AA-lg"/>
            <CRow t={t} fg="#FF5B2E" bg="#F5F4F1" label="Accent / BG" ratio="4.6" pass="AA"/>
            <CRow t={t} fg="#F5F4F1" bg="#0E0E12" label="BG / Ink (dark)" ratio="17.2" pass="AAA"/>
            <CRow t={t} fg="#A0A0AC" bg="#0E0E12" label="Ink3 / BG (dark)" ratio="8.4" pass="AA"/>
          </div>
        </DSFCard>

        <DSFCard t={t} title="Focus · Keyboard" sub="Ring 2px accent · offset 2px · mọi interactive">
          <div style={{ display:'flex', gap:12, flexWrap:'wrap', marginBottom:16 }}>
            <button style={{ padding:'8px 16px', background:t.grad, color:'#fff', border:'none', borderRadius:8, fontSize:12.5, fontWeight:600, boxShadow:`0 0 0 2px #fff, 0 0 0 4px ${t.accent}` }}>Primary · focus</button>
            <div style={{ padding:'8px 14px', background:'#fff', border:`1.5px solid ${t.border}`, borderRadius:8, fontSize:12.5, fontWeight:600, boxShadow:`0 0 0 2px #fff, 0 0 0 4px ${t.accent}` }}>Input · focus</div>
          </div>
          <div style={{ fontSize:12, color:t.ink2, lineHeight:1.6 }}>
            ✓ Tab order theo thứ tự visual<br/>
            ✓ Esc đóng modal / menu / drawer<br/>
            ✓ Enter = primary action<br/>
            ✓ Space = toggle / checkbox<br/>
            ✓ Arrow = di chuyển trong list / menu<br/>
            ✓ Cmd+K mở command palette
          </div>
        </DSFCard>
      </div>

      {/* SCREEN READER */}
      <div style={{ marginTop:20 }}>
        <DSFCard t={t} title="Screen reader — label patterns" sub="Mọi icon-only button phải có aria-label tiếng Việt">
          <div style={{ display:'grid', gridTemplateColumns:'1fr 1fr', gap:20 }}>
            <div>
              <div style={{ fontSize:11, fontFamily:t.mono, color:t.success, fontWeight:700, marginBottom:6 }}>✓ DO</div>
              <pre style={{ margin:0, padding:'10px 12px', background:'#0E0E12', color:'#EDEDEF', borderRadius:8, fontSize:11, fontFamily:t.mono, lineHeight:1.5 }}>
{`<button aria-label="Tải xuống file">
  <DSIcon name="download"/>
</button>

<img src="..." alt="Ảnh đại diện của Minh"/>

<div role="alert" aria-live="polite">
  Link đã được sao chép
</div>`}
              </pre>
            </div>
            <div>
              <div style={{ fontSize:11, fontFamily:t.mono, color:t.danger, fontWeight:700, marginBottom:6 }}>✕ DON'T</div>
              <pre style={{ margin:0, padding:'10px 12px', background:'#0E0E12', color:'#EDEDEF', borderRadius:8, fontSize:11, fontFamily:t.mono, lineHeight:1.5 }}>
{`<button>
  <DSIcon name="download"/>
</button>
// không có label — SR đọc "button"

<img src="..." alt=""/>
// user mù không biết đây là gì

<div>Link đã được sao chép</div>
// không có role — SR bỏ qua`}
              </pre>
            </div>
          </div>
        </DSFCard>
      </div>
    </div>
  );
}

function GdDo({ t, ok, title, desc, children }) {
  return (
    <div style={{ padding:'16px 18px', background:t.panel, border:`1px solid ${t.border}`, borderLeft:`3px solid ${ok? t.success : t.danger}`, borderRadius:10 }}>
      <div style={{ fontSize:10.5, fontFamily:t.mono, color: ok? t.success : t.danger, letterSpacing:'.12em', textTransform:'uppercase', fontWeight:700, marginBottom:4 }}>{ok? '✓ DO' : '✕ DON\'T'}</div>
      <div style={{ fontSize:13.5, fontWeight:700 }}>{title}</div>
      <div style={{ fontSize:11.5, color:t.ink3, marginTop:2, marginBottom:12 }}>{desc}</div>
      {children}
    </div>
  );
}

function CRow({ t, fg, bg, label, ratio, pass, warn }) {
  return (
    <div style={{ display:'grid', gridTemplateColumns:'36px 1fr 70px 70px', padding:'8px 0', borderBottom:`1px dashed ${t.border}`, alignItems:'center', gap:12 }}>
      <div style={{ width:32, height:32, background:bg, borderRadius:6, display:'flex', alignItems:'center', justifyContent:'center', border:`1px solid ${t.border}` }}>
        <span style={{ color:fg, fontSize:14, fontWeight:700 }}>Aa</span>
      </div>
      <div>
        <div style={{ fontSize:12, fontWeight:600 }}>{label}</div>
        <div style={{ fontSize:10.5, fontFamily:t.mono, color:t.ink4 }}>{fg} / {bg}</div>
      </div>
      <span style={{ fontFamily:t.mono, fontSize:13, fontWeight:700, color: warn? t.warn : t.ink }}>{ratio}:1</span>
      <span style={{ fontSize:10, fontWeight:700, padding:'3px 7px', background: warn? 'rgba(201,106,0,.12)' : 'rgba(10,138,92,.12)', color: warn? t.warn : t.success, borderRadius:4, textAlign:'center' }}>{pass}</span>
    </div>
  );
}

Object.assign(window, { GuidelinesCard });
