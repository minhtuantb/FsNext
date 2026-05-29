// ds-cards.jsx — Design system overview cards (tokens, type, components)

const { useState } = React;

// ─────────────────────────────────────────────────────────────
// Cover card — concept + brand positioning
// ─────────────────────────────────────────────────────────────
function CoverCard() {
  return (
    <div style={{
      width: 1400, height: 820, background: '#0E0E12', color: '#fff',
      fontFamily: "'Be Vietnam Pro', system-ui, sans-serif",
      position: 'relative', overflow: 'hidden',
    }}>
      {/* giant gradient orb */}
      <div style={{
        position: 'absolute', width: 900, height: 900, borderRadius: '50%',
        background: 'radial-gradient(circle, #FF3D7F 0%, #FF5B2E 40%, transparent 70%)',
        filter: 'blur(80px)', top: -200, right: -200, opacity: 0.6,
      }} />
      <div style={{
        position: 'absolute', width: 700, height: 700, borderRadius: '50%',
        background: 'radial-gradient(circle, #FFAF1D 0%, transparent 70%)',
        filter: 'blur(100px)', bottom: -300, left: -100, opacity: 0.4,
      }} />

      {/* grid overlay */}
      <div style={{
        position: 'absolute', inset: 0,
        backgroundImage: 'linear-gradient(rgba(255,255,255,.03) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,.03) 1px, transparent 1px)',
        backgroundSize: '60px 60px',
      }} />

      <div style={{ position: 'relative', padding: '64px 72px', height: '100%', display: 'flex', flexDirection: 'column' }}>
        {/* top bar */}
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: 14 }}>
            <FshareLogo size={40} />
            <div>
              <div style={{ fontSize: 16, fontWeight: 700, letterSpacing: '-.02em' }}>Fshare Desktop</div>
              <div style={{ fontSize: 11, color: 'rgba(255,255,255,.5)', fontFamily: "'Geist Mono', monospace", letterSpacing: '.12em', textTransform: 'uppercase', marginTop: 2 }}>Design Exploration · v0.1 · 2026</div>
            </div>
          </div>
          <div style={{ display: 'flex', gap: 8 }}>
            <Chip>Windows 11 · Fluent</Chip>
            <Chip>Light + Dark</Chip>
            <Chip>3 directions</Chip>
          </div>
        </div>

        {/* huge title */}
        <div style={{ marginTop: 'auto', marginBottom: 40 }}>
          <div style={{ fontSize: 13, fontFamily: "'Geist Mono', monospace", color: '#FFAF1D', letterSpacing: '.18em', textTransform: 'uppercase', marginBottom: 18 }}>
            ━━ Desktop client · All-in-one
          </div>
          <h1 style={{
            fontFamily: "'Instrument Serif', serif",
            fontSize: 148, fontWeight: 400, lineHeight: .92,
            letterSpacing: '-.04em', margin: 0,
          }}>
            Mọi file,<br/>
            <span style={{ fontStyle: 'italic', background: 'linear-gradient(135deg, #FF3D7F, #FF5B2E, #FFAF1D)', WebkitBackgroundClip: 'text', WebkitTextFillColor: 'transparent', backgroundClip: 'text' }}>một nơi.</span>
          </h1>
          <div style={{ fontSize: 19, color: 'rgba(255,255,255,.7)', maxWidth: 720, marginTop: 28, lineHeight: 1.5, fontWeight: 300 }}>
            Sync. Upload. Download ở tốc độ cao. Chia sẻ trong một cú click.
            Một ứng dụng desktop cho người Việt, thay thế hoàn toàn trải nghiệm web Fshare hiện tại.
          </div>
        </div>

        {/* bottom stats */}
        <div style={{ display: 'flex', gap: 48, borderTop: '1px solid rgba(255,255,255,.1)', paddingTop: 28 }}>
          <Stat label="Variations" value="3" />
          <Stat label="Key screens" value="15" />
          <Stat label="Components" value="40+" />
          <Stat label="Theme modes" value="Light · Dark" />
          <div style={{ marginLeft: 'auto', display: 'flex', alignItems: 'center', gap: 10, fontSize: 12, color: 'rgba(255,255,255,.5)', fontFamily: "'Geist Mono', monospace" }}>
            <div style={{ width: 8, height: 8, borderRadius: '50%', background: '#00E5B8', boxShadow: '0 0 12px #00E5B8' }} />
            EXPLORING
          </div>
        </div>
      </div>
    </div>
  );
}

function Chip({ children }) {
  return (
    <div style={{
      padding: '7px 12px', fontSize: 11, fontWeight: 500,
      background: 'rgba(255,255,255,.06)', border: '1px solid rgba(255,255,255,.12)',
      borderRadius: 999, color: 'rgba(255,255,255,.85)',
      fontFamily: "'Geist Mono', monospace", letterSpacing: '.04em',
    }}>{children}</div>
  );
}

function Stat({ label, value }) {
  return (
    <div>
      <div style={{ fontSize: 10, color: 'rgba(255,255,255,.4)', fontFamily: "'Geist Mono', monospace", letterSpacing: '.16em', textTransform: 'uppercase' }}>{label}</div>
      <div style={{ fontSize: 28, fontWeight: 600, marginTop: 4, fontFamily: "'Instrument Serif', serif", fontStyle: 'italic' }}>{value}</div>
    </div>
  );
}

function FshareLogo({ size = 40, mono = false }) {
  return (
    <div style={{
      width: size, height: size, borderRadius: size * 0.22,
      background: mono ? '#fff' : 'linear-gradient(135deg, #FF3D7F 0%, #FF5B2E 50%, #FFAF1D 100%)',
      display: 'flex', alignItems: 'center', justifyContent: 'center',
      color: mono ? '#0E0E12' : '#fff', fontWeight: 800, fontSize: size * 0.5,
      fontFamily: "'Instrument Serif', serif", fontStyle: 'italic',
      boxShadow: mono ? 'none' : '0 6px 20px rgba(255,91,46,.4)',
    }}>f</div>
  );
}

// ─────────────────────────────────────────────────────────────
// Analysis card — Fshare.vn insights
// ─────────────────────────────────────────────────────────────
function AnalysisCard() {
  const pains = [
    { k: '01', title: 'Tốc độ & giới hạn', desc: 'Non-VIP bị giới hạn tốc độ nghiêm trọng. Người dùng thường mở nhiều tab, phải chờ, không biết khi nào xong.' },
    { k: '02', title: 'Không có sync client', desc: 'Fshare hiện tại thuần web. Muốn tải file phải vào trang, paste link, chờ captcha, chờ countdown.' },
    { k: '03', title: 'Giao diện web nhiều quảng cáo', desc: 'Trang download có ad banner, pop-up, trải nghiệm không sạch sẽ cho người trả tiền.' },
    { k: '04', title: 'Quản lý file phân tán', desc: 'File cá nhân, link nhận từ bạn bè, thư mục sync — 3 use case khác nhau không có điểm hội tụ.' },
    { k: '05', title: 'Link share thiếu kiểm soát', desc: 'Ít tuỳ chọn: password, hạn ngày, theo dõi ai đã xem. Người chia sẻ muốn biết link có còn "sống" không.' },
  ];
  const opps = [
    { k: 'A', title: 'Trung tâm tải xuống tốc độ cao', desc: 'Queue, resume, multi-thread, tận dụng trọn tốc độ VIP mà không cần qua browser.' },
    { k: 'B', title: 'Sync folder như Dropbox', desc: 'Chọn folder local → auto 2-way sync. File mới upload, file xoá → di chuyển sang Thùng rác cloud.' },
    { k: 'C', title: 'Link share thông minh', desc: 'Password, expire, lượt xem, auto-revoke. Dashboard cho từng link đang "sống".' },
    { k: 'D', title: 'VIP dashboard', desc: 'Hiện rõ dung lượng còn lại, ngày hết hạn, tốc độ hiện tại, khuyến mại trong-app.' },
  ];

  return (
    <div style={{
      width: 1400, height: 820, background: '#F5F4F1', padding: '48px 56px',
      fontFamily: "'Be Vietnam Pro', system-ui, sans-serif", color: '#0E0E12',
      display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 48,
    }}>
      {/* left — pains */}
      <div>
        <div style={{ fontSize: 11, fontFamily: "'Geist Mono', monospace", color: '#D53030', letterSpacing: '.18em', textTransform: 'uppercase', marginBottom: 16 }}>
          ━━ Phân tích · Fshare.vn hiện tại
        </div>
        <h2 style={{ fontFamily: "'Instrument Serif', serif", fontSize: 56, fontWeight: 400, letterSpacing: '-.03em', lineHeight: 1, margin: 0, marginBottom: 32 }}>
          User pain<br/><span style={{ fontStyle: 'italic', color: '#D53030' }}>points.</span>
        </h2>
        <div style={{ display: 'flex', flexDirection: 'column', gap: 18 }}>
          {pains.map(p => (
            <div key={p.k} style={{ display: 'flex', gap: 18, alignItems: 'flex-start' }}>
              <div style={{
                fontFamily: "'Geist Mono', monospace", fontSize: 11, fontWeight: 600,
                color: '#8A8A94', width: 24, paddingTop: 3,
              }}>{p.k}</div>
              <div style={{ flex: 1, paddingBottom: 16, borderBottom: '1px solid #E6E3DC' }}>
                <div style={{ fontSize: 16, fontWeight: 600, marginBottom: 4 }}>{p.title}</div>
                <div style={{ fontSize: 13, color: '#5C5C66', lineHeight: 1.55 }}>{p.desc}</div>
              </div>
            </div>
          ))}
        </div>
      </div>

      {/* right — opps */}
      <div>
        <div style={{ fontSize: 11, fontFamily: "'Geist Mono', monospace", color: '#0A8A5C', letterSpacing: '.18em', textTransform: 'uppercase', marginBottom: 16 }}>
          ━━ Cơ hội · Desktop app có thể giải
        </div>
        <h2 style={{ fontFamily: "'Instrument Serif', serif", fontSize: 56, fontWeight: 400, letterSpacing: '-.03em', lineHeight: 1, margin: 0, marginBottom: 32 }}>
          Where we<br/><span style={{ fontStyle: 'italic', color: '#0A8A5C' }}>win.</span>
        </h2>
        <div style={{ display: 'flex', flexDirection: 'column', gap: 14 }}>
          {opps.map(o => (
            <div key={o.k} style={{
              background: '#fff', borderRadius: 12, padding: '18px 22px',
              border: '1px solid #E6E3DC', display: 'flex', gap: 18, alignItems: 'flex-start',
            }}>
              <div style={{
                width: 36, height: 36, borderRadius: 10, flexShrink: 0,
                background: 'linear-gradient(135deg, #FF3D7F 0%, #FFAF1D 100%)',
                display: 'flex', alignItems: 'center', justifyContent: 'center',
                color: '#fff', fontWeight: 700, fontSize: 15,
                fontFamily: "'Instrument Serif', serif", fontStyle: 'italic',
              }}>{o.k}</div>
              <div style={{ flex: 1 }}>
                <div style={{ fontSize: 15, fontWeight: 600, marginBottom: 3 }}>{o.title}</div>
                <div style={{ fontSize: 12.5, color: '#5C5C66', lineHeight: 1.55 }}>{o.desc}</div>
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}

// ─────────────────────────────────────────────────────────────
// Typography scale card
// ─────────────────────────────────────────────────────────────
function TypeCard() {
  const scale = [
    { px: 72, label: 'Display / Hero', weight: 400, ff: 'serif' },
    { px: 48, label: 'H1 · Page title', weight: 600, ff: 'sans' },
    { px: 32, label: 'H2 · Section', weight: 600, ff: 'sans' },
    { px: 22, label: 'H3 · Card title', weight: 600, ff: 'sans' },
    { px: 16, label: 'Body', weight: 400, ff: 'sans' },
    { px: 14, label: 'Body small', weight: 400, ff: 'sans' },
    { px: 12, label: 'Caption / Meta', weight: 500, ff: 'sans' },
    { px: 11, label: 'Mono · overline', weight: 500, ff: 'mono' },
  ];
  return (
    <div style={{
      width: 1400, height: 820, background: '#fff', padding: '56px 64px',
      fontFamily: "'Be Vietnam Pro', system-ui, sans-serif", color: '#0E0E12',
    }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'baseline', marginBottom: 8 }}>
        <div>
          <div style={{ fontSize: 11, fontFamily: "'Geist Mono', monospace", color: '#8A8A94', letterSpacing: '.18em', textTransform: 'uppercase', marginBottom: 10 }}>
            02 · Typography
          </div>
          <h2 style={{ fontFamily: "'Instrument Serif', serif", fontSize: 72, fontWeight: 400, letterSpacing: '-.03em', margin: 0 }}>
            Type, <span style={{ fontStyle: 'italic' }}>loud &amp; quiet.</span>
          </h2>
        </div>
        <div style={{ fontSize: 13, color: '#5C5C66', textAlign: 'right', maxWidth: 340, lineHeight: 1.55 }}>
          <b>Be Vietnam Pro</b> lo phần Việt hoá sạch sẽ. <b>Instrument Serif</b> mang chất editorial cho tiêu đề lớn. <b>Geist Mono</b> cho số liệu, mã link và speed counter.
        </div>
      </div>

      <div style={{ marginTop: 48, display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 48 }}>
        {/* left — font families sample */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: 20 }}>
          <FontSample
            family="'Be Vietnam Pro', sans-serif"
            name="Be Vietnam Pro"
            role="Sans · UI primary"
            weights="300 400 500 600 700 800"
            sample="Tải về ngay"
          />
          <FontSample
            family="'Instrument Serif', serif"
            name="Instrument Serif"
            role="Serif · Editorial"
            weights="400 · italic"
            sample="fshare."
            italic
          />
          <FontSample
            family="'Geist Mono', monospace"
            name="Geist Mono"
            role="Mono · Data &amp; code"
            weights="400 500 600"
            sample="47.3 MB/s"
          />
        </div>

        {/* right — scale */}
        <div style={{ borderLeft: '1px solid #E6E3DC', paddingLeft: 36 }}>
          <div style={{ fontSize: 11, fontFamily: "'Geist Mono', monospace", color: '#8A8A94', letterSpacing: '.16em', textTransform: 'uppercase', marginBottom: 16 }}>
            Scale
          </div>
          <div style={{ display: 'flex', flexDirection: 'column', gap: 14 }}>
            {scale.map(s => (
              <div key={s.px} style={{ display: 'flex', alignItems: 'baseline', gap: 16, borderBottom: '1px dashed #E6E3DC', paddingBottom: 10 }}>
                <div style={{ width: 40, fontSize: 10, fontFamily: "'Geist Mono', monospace", color: '#8A8A94' }}>{s.px}px</div>
                <div style={{
                  fontSize: Math.min(s.px, 36), fontWeight: s.weight, flex: 1,
                  fontFamily: s.ff === 'serif' ? "'Instrument Serif', serif" : s.ff === 'mono' ? "'Geist Mono', monospace" : "'Be Vietnam Pro', sans-serif",
                  letterSpacing: s.px > 30 ? '-.02em' : 0,
                }}>{s.label}</div>
              </div>
            ))}
          </div>
        </div>
      </div>
    </div>
  );
}

function FontSample({ family, name, role, weights, sample, italic }) {
  return (
    <div style={{ padding: '24px 28px', border: '1px solid #E6E3DC', borderRadius: 12 }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'baseline' }}>
        <div>
          <div style={{ fontSize: 14, fontWeight: 600 }}>{name}</div>
          <div style={{ fontSize: 11, color: '#8A8A94', fontFamily: "'Geist Mono', monospace", letterSpacing: '.1em', marginTop: 2 }} dangerouslySetInnerHTML={{ __html: role }}/>
        </div>
        <div style={{ fontSize: 10, color: '#8A8A94', fontFamily: "'Geist Mono', monospace" }} dangerouslySetInnerHTML={{ __html: weights }}/>
      </div>
      <div style={{
        fontFamily: family, fontSize: 56, marginTop: 18, lineHeight: 1,
        fontStyle: italic ? 'italic' : 'normal', fontWeight: italic ? 400 : 600,
        letterSpacing: '-.02em',
      }}>{sample}</div>
    </div>
  );
}

// ─────────────────────────────────────────────────────────────
// Color card — full palettes for all 3 variants
// ─────────────────────────────────────────────────────────────
function ColorCard() {
  return (
    <div style={{
      width: 1400, height: 820, background: '#F5F4F1', padding: '48px 56px',
      fontFamily: "'Be Vietnam Pro', system-ui, sans-serif", color: '#0E0E12',
    }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'baseline', marginBottom: 36 }}>
        <div>
          <div style={{ fontSize: 11, fontFamily: "'Geist Mono', monospace", color: '#8A8A94', letterSpacing: '.18em', textTransform: 'uppercase', marginBottom: 10 }}>
            03 · Colors
          </div>
          <h2 style={{ fontFamily: "'Instrument Serif', serif", fontSize: 72, fontWeight: 400, letterSpacing: '-.03em', margin: 0 }}>
            Ba ngôn ngữ, <span style={{ fontStyle: 'italic' }}>một app.</span>
          </h2>
        </div>
        <div style={{ fontSize: 13, color: '#5C5C66', maxWidth: 380, textAlign: 'right', lineHeight: 1.55 }}>
          Mỗi variant có palette riêng. Chia sẻ nguyên tắc: <b>1 neutral scale</b> + <b>1 gradient chính</b> + <b>4 semantic</b>.
        </div>
      </div>

      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr 1fr', gap: 24 }}>
        <PaletteCol title="Aurora" subtitle="Warm · Vibrant · Modern" bg="#FFFFFF" ink="#0E0E12" palette={[
          { n: 'Ink', v: '#0E0E12' }, { n: 'Ink-3', v: '#5C5C66' }, { n: 'Border', v: '#E6E3DC' }, { n: 'Bg', v: '#F5F4F1' },
          { n: 'Pink', v: '#FF3D7F', accent: true }, { n: 'Red', v: '#FF5B2E', accent: true }, { n: 'Mango', v: '#FFAF1D', accent: true },
        ]} gradient="linear-gradient(135deg, #FF3D7F 0%, #FF5B2E 50%, #FFAF1D 100%)" />

        <PaletteCol title="Monolith" subtitle="Editorial · Calm · Trust" bg="#FAFAF7" ink="#0A0A0A" palette={[
          { n: 'Ink', v: '#0A0A0A' }, { n: 'Ink-3', v: '#4A4A4A' }, { n: 'Border', v: '#E8E6E0' }, { n: 'Bg', v: '#FAFAF7' },
          { n: 'Pine', v: '#1E4D3E', accent: true }, { n: 'Ochre', v: '#D4A554', accent: true }, { n: 'Clay', v: '#8B4513', accent: true },
        ]} gradient="linear-gradient(180deg, #1E4D3E 0%, #0E2F26 100%)" />

        <PaletteCol title="Neon Velocity" subtitle="Dark · Dense · Speed" bg="#0A0A0F" ink="#F5F5FA" palette={[
          { n: 'Ink', v: '#F5F5FA', dark: true }, { n: 'Ink-3', v: '#8E8E9E', dark: true }, { n: 'Border', v: '#1F1F2B', dark: true }, { n: 'Bg', v: '#0A0A0F', dark: true },
          { n: 'Mint', v: '#00E5B8', accent: true, dark: true }, { n: 'UV', v: '#7C5CFF', accent: true, dark: true }, { n: 'Gold', v: '#FFD43D', accent: true, dark: true },
        ]} gradient="linear-gradient(135deg, #00E5B8 0%, #7C5CFF 100%)" dark />
      </div>
    </div>
  );
}

function PaletteCol({ title, subtitle, bg, ink, palette, gradient, dark }) {
  return (
    <div style={{
      background: bg, color: ink, borderRadius: 16, padding: 26,
      border: `1px solid ${dark ? '#1F1F2B' : '#E6E3DC'}`,
      boxShadow: '0 4px 20px rgba(0,0,0,.06)',
    }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', marginBottom: 20 }}>
        <div>
          <div style={{ fontSize: 20, fontWeight: 700, letterSpacing: '-.01em' }}>{title}</div>
          <div style={{ fontSize: 11, opacity: .5, fontFamily: "'Geist Mono', monospace", letterSpacing: '.12em', marginTop: 2 }}>{subtitle}</div>
        </div>
        <div style={{ width: 48, height: 48, borderRadius: 12, background: gradient, boxShadow: '0 4px 14px rgba(0,0,0,.2)' }}/>
      </div>

      {/* gradient hero */}
      <div style={{
        height: 120, borderRadius: 12, background: gradient, marginBottom: 18,
        position: 'relative', overflow: 'hidden',
      }}>
        <div style={{ position: 'absolute', bottom: 12, left: 14, right: 14, display: 'flex', justifyContent: 'space-between', color: '#fff', fontFamily: "'Geist Mono', monospace", fontSize: 10, letterSpacing: '.1em', textTransform: 'uppercase' }}>
          <span>Signature gradient</span>
          <span style={{ opacity: .7 }}>135°</span>
        </div>
      </div>

      {/* swatches */}
      <div style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
        {palette.map(s => (
          <div key={s.n} style={{
            display: 'flex', alignItems: 'center', gap: 12,
            padding: '8px 10px', borderRadius: 8,
            background: dark ? 'rgba(255,255,255,.02)' : 'rgba(0,0,0,.02)',
          }}>
            <div style={{
              width: 28, height: 28, borderRadius: 6, background: s.v, flexShrink: 0,
              border: `1px solid ${s.dark ? 'rgba(255,255,255,.1)' : 'rgba(0,0,0,.08)'}`,
              boxShadow: s.accent ? '0 2px 8px rgba(0,0,0,.12)' : 'none',
            }}/>
            <div style={{ fontSize: 12, fontWeight: 600, flex: 1 }}>{s.n}</div>
            <div style={{ fontSize: 10, fontFamily: "'Geist Mono', monospace", opacity: .6, textTransform: 'uppercase' }}>{s.v}</div>
          </div>
        ))}
      </div>
    </div>
  );
}

Object.assign(window, { CoverCard, AnalysisCard, TypeCard, ColorCard, FshareLogo });
