// aurora-auth.jsx — Login screen (Aurora style, Windows 11)
// Không có sidebar · full window · split layout

function AuroraLogin() {
  const t = useTA();

  return (
    <WinChrome title="Fshare" theme={t}>
      <div style={{ flex: 1, display: 'flex', overflow: 'hidden', background: t.bg }}>
        {/* ─── LEFT · Brand panel (gradient hero) ─── */}
        <div style={{
          width: 560, flexShrink: 0, position: 'relative', overflow: 'hidden',
          background: t.mode === 'dark' ? '#050507' : '#0E0E12',
          color: '#fff', padding: '56px 52px',
          display: 'flex', flexDirection: 'column',
        }}>
          {/* gradient orbs */}
          <div style={{
            position: 'absolute', width: 520, height: 520, borderRadius: '50%',
            background: 'radial-gradient(circle, #FF3D7F 0%, #FF5B2E 40%, transparent 70%)',
            filter: 'blur(60px)', top: -120, right: -160, opacity: .55,
          }}/>
          <div style={{
            position: 'absolute', width: 420, height: 420, borderRadius: '50%',
            background: 'radial-gradient(circle, #FFAF1D 0%, transparent 70%)',
            filter: 'blur(80px)', bottom: -120, left: -120, opacity: .45,
          }}/>
          {/* grid */}
          <div style={{
            position: 'absolute', inset: 0,
            backgroundImage: 'linear-gradient(rgba(255,255,255,.03) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,.03) 1px, transparent 1px)',
            backgroundSize: '48px 48px',
          }}/>

          <div style={{ position: 'relative', display: 'flex', alignItems: 'center', gap: 12 }}>
            <FshareLogo size={36}/>
            <div style={{ fontSize: 18, fontWeight: 700, letterSpacing: '-.02em' }}>Fshare Desktop</div>
          </div>

          {/* huge editorial title */}
          <div style={{ position: 'relative', marginTop: 'auto' }}>
            <div style={{ fontSize: 11, fontFamily: t.mono, color: '#FFAF1D', letterSpacing: '.18em', textTransform: 'uppercase', marginBottom: 18 }}>
              ━━ Chào bạn trở lại
            </div>
            <h1 style={{
              fontFamily: t.serif, fontSize: 88, fontWeight: 400, lineHeight: .96,
              letterSpacing: '-.04em', margin: 0,
            }}>
              Tiếp tục <br/>
              <span style={{ fontStyle: 'italic', background: t.grad, WebkitBackgroundClip: 'text', WebkitTextFillColor: 'transparent' }}>
                nơi bạn dừng.
              </span>
            </h1>
            <div style={{ fontSize: 14, color: 'rgba(255,255,255,.7)', marginTop: 22, lineHeight: 1.55, maxWidth: 380, fontWeight: 300 }}>
              287 GB trong kho, 7 file đang chờ tải về, 14 link đang sống — mọi thứ của bạn, cách một lần đăng nhập.
            </div>
          </div>

          {/* bottom testimonial-ish */}
          <div style={{ position: 'relative', marginTop: 48, paddingTop: 24, borderTop: '1px solid rgba(255,255,255,.1)', display: 'flex', gap: 32 }}>
            <StatMini label="File đang lưu" value="287 GB"/>
            <StatMini label="Tốc độ VIP" value="Không giới hạn"/>
            <StatMini label="Hết hạn" value="243 ngày"/>
          </div>
        </div>

        {/* ─── RIGHT · Form ─── */}
        <div style={{
          flex: 1, display: 'flex', alignItems: 'center', justifyContent: 'center',
          padding: 40, position: 'relative',
        }}>
          {/* top right — signup link */}
          <div style={{ position: 'absolute', top: 24, right: 32, fontSize: 12.5, color: t.ink3 }}>
            Chưa có tài khoản? <a style={{ color: t.accent, fontWeight: 600, textDecoration: 'none', cursor: 'pointer' }}>Đăng ký ngay →</a>
          </div>

          <div style={{ width: 400 }}>
            <div style={{ fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.18em', textTransform: 'uppercase', marginBottom: 10 }}>
              ━━ Đăng nhập
            </div>
            <h2 style={{
              fontFamily: t.serif, fontSize: 48, fontWeight: 400, letterSpacing: '-.03em',
              margin: 0, lineHeight: 1, color: t.ink,
            }}>
              Chào <span style={{ fontStyle: 'italic', color: t.accent }}>Minh.</span>
            </h2>
            <div style={{ fontSize: 13.5, color: t.ink3, marginTop: 10 }}>
              Đăng nhập để tiếp tục đồng bộ 3 folder và quản lý 14 link đang hoạt động.
            </div>

            {/* social row */}
            <div style={{ marginTop: 28, display: 'grid', gridTemplateColumns: '1fr 1fr 1fr', gap: 8 }}>
              <SocialBtn t={t} brand="google" label="Google"/>
              <SocialBtn t={t} brand="facebook" label="Facebook"/>
              <SocialBtn t={t} brand="zalo" label="Zalo"/>
            </div>

            {/* divider */}
            <div style={{ display: 'flex', alignItems: 'center', gap: 12, margin: '24px 0 22px' }}>
              <div style={{ flex: 1, height: 1, background: t.border }}/>
              <div style={{ fontSize: 11, color: t.ink4, fontFamily: t.mono, letterSpacing: '.14em' }}>HOẶC</div>
              <div style={{ flex: 1, height: 1, background: t.border }}/>
            </div>

            {/* email / password */}
            <LoginField t={t} label="Email hoặc tên đăng nhập" value="minh.nguyen@fpt.com" icon={Icons.users}/>
            <div style={{ height: 14 }}/>
            <LoginField t={t} label="Mật khẩu" value="••••••••••••" icon={Icons.lock} password/>

            <div style={{ marginTop: 14, display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
              <label style={{ display: 'flex', alignItems: 'center', gap: 8, fontSize: 12.5, color: t.ink2, cursor: 'pointer' }}>
                <div style={{
                  width: 16, height: 16, borderRadius: 4, background: t.grad,
                  display: 'flex', alignItems: 'center', justifyContent: 'center',
                  boxShadow: t.shadow,
                }}><Icons.check size={10} stroke="#fff" sw={3}/></div>
                Giữ đăng nhập
              </label>
              <a style={{ fontSize: 12.5, color: t.accent, fontWeight: 600, cursor: 'pointer' }}>Quên mật khẩu?</a>
            </div>

            <button style={{
              width: '100%', marginTop: 22, padding: '13px', background: t.grad, color: '#fff',
              border: 'none', borderRadius: 10, fontSize: 14, fontWeight: 600, cursor: 'pointer',
              boxShadow: t.shadow, fontFamily: t.font,
              display: 'flex', alignItems: 'center', justifyContent: 'center', gap: 8,
            }}>
              Đăng nhập <Icons.chevRight size={15}/>
            </button>

            {/* secure footer */}
            <div style={{
              marginTop: 22, padding: '10px 12px', background: t.panel,
              border: `1px solid ${t.border}`, borderRadius: 10,
              display: 'flex', alignItems: 'center', gap: 10, fontSize: 11.5, color: t.ink3,
            }}>
              <Icons.shield size={14} stroke={t.success}/>
              Kết nối an toàn · TLS 1.3 · SRP authentication
            </div>
          </div>
        </div>
      </div>
    </WinChrome>
  );
}

function StatMini({ label, value }) {
  return (
    <div>
      <div style={{ fontSize: 10, color: 'rgba(255,255,255,.5)', fontFamily: "'Geist Mono', monospace", letterSpacing: '.14em', textTransform: 'uppercase' }}>{label}</div>
      <div style={{ fontSize: 14, fontWeight: 600, marginTop: 3, color: '#fff' }}>{value}</div>
    </div>
  );
}

function SocialBtn({ t, brand, label }) {
  const logos = {
    google: (
      <svg width="16" height="16" viewBox="0 0 24 24">
        <path fill="#4285F4" d="M22.56 12.25c0-.78-.07-1.53-.2-2.25H12v4.26h5.92c-.26 1.37-1.04 2.53-2.21 3.31v2.77h3.57c2.08-1.92 3.28-4.74 3.28-8.09z"/>
        <path fill="#34A853" d="M12 23c2.97 0 5.46-.98 7.28-2.66l-3.57-2.77c-.98.66-2.23 1.06-3.71 1.06-2.86 0-5.29-1.93-6.16-4.53H2.18v2.84C3.99 20.53 7.7 23 12 23z"/>
        <path fill="#FBBC04" d="M5.84 14.09c-.22-.66-.35-1.36-.35-2.09s.13-1.43.35-2.09V7.07H2.18C1.43 8.55 1 10.22 1 12s.43 3.45 1.18 4.93l2.85-2.22.81-.62z"/>
        <path fill="#EA4335" d="M12 5.38c1.62 0 3.06.56 4.21 1.64l3.15-3.15C17.45 2.09 14.97 1 12 1 7.7 1 3.99 3.47 2.18 7.07l3.66 2.84c.87-2.6 3.3-4.53 6.16-4.53z"/>
      </svg>
    ),
    facebook: (
      <svg width="16" height="16" viewBox="0 0 24 24" fill="#1877F2">
        <path d="M24 12a12 12 0 1 0-13.88 11.85v-8.38H7.08V12h3.04V9.36c0-3 1.79-4.67 4.53-4.67 1.31 0 2.68.23 2.68.23v2.95H15.83c-1.49 0-1.96.92-1.96 1.87V12h3.33l-.53 3.47h-2.8v8.38A12 12 0 0 0 24 12z"/>
      </svg>
    ),
    zalo: (
      <svg width="16" height="16" viewBox="0 0 24 24" fill="#0068FF">
        <path d="M12 2C6.48 2 2 6.14 2 11.25c0 2.88 1.43 5.44 3.67 7.14L5 22l3.89-2.07c.98.23 2.01.37 3.11.37 5.52 0 10-4.14 10-9.25S17.52 2 12 2z"/>
        <text x="12" y="14" fontFamily="sans-serif" fontSize="7" fontWeight="900" fill="#fff" textAnchor="middle">Zalo</text>
      </svg>
    ),
  };
  return (
    <button style={{
      padding: '11px 10px', background: t.panel, color: t.ink, border: `1px solid ${t.border}`,
      borderRadius: 10, fontSize: 12.5, fontWeight: 600, cursor: 'pointer',
      display: 'flex', alignItems: 'center', justifyContent: 'center', gap: 7,
      fontFamily: t.font,
    }}>
      {logos[brand]} {label}
    </button>
  );
}

function LoginField({ t, label, value, icon, password }) {
  const Ico = icon;
  return (
    <div>
      <div style={{ fontSize: 11, fontWeight: 600, color: t.ink3, marginBottom: 7, fontFamily: t.mono, letterSpacing: '.04em', textTransform: 'uppercase' }}>{label}</div>
      <div style={{
        display: 'flex', alignItems: 'center', gap: 10, padding: '11px 14px',
        background: t.panel, border: `1.5px solid ${t.border}`, borderRadius: 10,
      }}>
        <Ico size={15} stroke={t.ink3}/>
        <input defaultValue={value} type={password ? 'password' : 'text'}
          style={{ flex: 1, border: 'none', outline: 'none', background: 'transparent', fontSize: 13.5, fontFamily: t.font, color: t.ink }}/>
        {password && <Icons.eye size={15} stroke={t.ink3} style={{ cursor: 'pointer' }}/>}
      </div>
    </div>
  );
}

Object.assign(window, { AuroraLogin });
