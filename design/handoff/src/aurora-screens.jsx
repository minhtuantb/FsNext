// aurora-screens.jsx — Upload, Account, Favorites, Sync
// Download đã có sẵn trong variant-aurora.jsx (AuroraDownloads)

// ═══════════════ UPLOAD ═══════════════
function AuroraUpload() {
  const t = useTA();
  const [collapsed, setCollapsed] = React.useState(false);
  return (
    <WinChrome title="Fshare · Tải lên" theme={t}>
      <AuroraSidebar active="uploads" collapsed={collapsed} onToggle={() => setCollapsed(v=>!v)}/>
      <div style={{ flex: 1, overflow: 'auto', background: t.bg }}>
        <div style={{ padding: '28px 32px 20px', background: t.panel, borderBottom: `1px solid ${t.border}` }}>
          <div style={{ fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', marginBottom: 8 }}>Tải lên · 4 đang xử lý</div>
          <div style={{ display: 'flex', alignItems: 'baseline', gap: 24 }}>
            <div style={{ fontFamily: t.serif, fontSize: 56, fontWeight: 400, letterSpacing: '-.03em', lineHeight: 1 }}>
              42.8 <span style={{ fontSize: 24, fontStyle: 'italic', color: t.ink3 }}>MB/s</span>
            </div>
            <div style={{ display: 'flex', gap: 28 }}>
              <MiniStat label="Đã upload" value="2.4 GB" mono={t.mono}/>
              <MiniStat label="Còn lại" value="5.1 GB" mono={t.mono}/>
              <MiniStat label="ETA" value="2m 08s" mono={t.mono}/>
            </div>
            <div style={{ marginLeft: 'auto', display: 'flex', gap: 8 }}>
              <button style={btnGhost(t)}><Icons.pause size={13}/> Tạm dừng tất cả</button>
              <button style={{ ...btnGhost(t), background: t.grad, color: '#fff', border: 'none', boxShadow: t.shadow }}><Icons.plus size={13}/> Thêm file</button>
            </div>
          </div>
        </div>

        {/* drag-drop zone */}
        <div style={{ padding: '24px 32px 12px' }}>
          <div style={{
            padding: '36px 28px', border: `2px dashed ${t.accent}`, borderRadius: 16,
            background: t.gradSoft, display: 'flex', alignItems: 'center', gap: 20,
          }}>
            <div style={{
              width: 64, height: 64, borderRadius: 16, background: t.grad,
              display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff',
              boxShadow: t.shadow,
            }}><Icons.upload size={28}/></div>
            <div style={{ flex: 1 }}>
              <div style={{ fontFamily: t.serif, fontSize: 28, fontWeight: 400, fontStyle: 'italic', color: t.ink, letterSpacing: '-.02em', lineHeight: 1 }}>
                Kéo thả bất kỳ vào đây.
              </div>
              <div style={{ fontSize: 13, color: t.ink3, marginTop: 6 }}>
                Hoặc <span style={{ color: t.accent, fontWeight: 600, cursor: 'pointer' }}>chọn từ máy</span> — tối đa 50 GB mỗi file · tự động chia gói và resume khi mất mạng
              </div>
            </div>
            <div style={{ display: 'flex', flexDirection: 'column', gap: 6, textAlign: 'right' }}>
              <ChipMini t={t}>Ưu tiên · VIP</ChipMini>
              <ChipMini t={t}>Mã hoá AES-256</ChipMini>
            </div>
          </div>
        </div>

        {/* destination selector */}
        <div style={{ padding: '16px 32px', display: 'flex', alignItems: 'center', gap: 14 }}>
          <div style={{ fontSize: 12, color: t.ink3 }}>Thư mục đích:</div>
          <div style={{ display: 'flex', alignItems: 'center', gap: 6, padding: '7px 12px', background: t.panel, border: `1px solid ${t.border}`, borderRadius: 8, fontSize: 12.5, fontWeight: 600 }}>
            <Icons.folder size={13} stroke={t.accent}/> /Phim/2025/IMAX Remux
            <Icons.chevDown size={12} style={{ color: t.ink4, marginLeft: 4 }}/>
          </div>
          <label style={{ display: 'flex', alignItems: 'center', gap: 6, fontSize: 12, color: t.ink2, cursor: 'pointer', marginLeft: 'auto' }}>
            <div style={{ width: 28, height: 16, background: t.grad, borderRadius: 999, padding: 2, display: 'flex', justifyContent: 'flex-end' }}>
              <div style={{ width: 12, height: 12, background: '#fff', borderRadius: '50%' }}/>
            </div>
            Tạo link chia sẻ tự động khi xong
          </label>
          <label style={{ display: 'flex', alignItems: 'center', gap: 6, fontSize: 12, color: t.ink2, cursor: 'pointer' }}>
            <div style={{ width: 28, height: 16, background: t.border, borderRadius: 999, padding: 2 }}>
              <div style={{ width: 12, height: 12, background: t.panel, borderRadius: '50%' }}/>
            </div>
            Nén .zip trước khi up
          </label>
        </div>

        {/* upload queue */}
        <div style={{ padding: '8px 32px 16px', fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600 }}>
          ━ Hàng chờ · 7 file (4 đang tải, 3 chờ)
        </div>

        <div style={{ padding: '0 32px 32px', display: 'flex', flexDirection: 'column', gap: 10 }}>
          <UpRowActive t={t} name="BTS_SummerTour_2026_4K.mov" pct={62} size="3.2 GB" done="2.0 GB" speed="18.4 MB/s" eta="38 giây" icon={Icons.video}/>
          <UpRowActive t={t} name="RAW_Photos_Canon_R5.zip" pct={24} size="1.8 GB" done="432 MB" speed="12.2 MB/s" eta="2 phút" icon={Icons.archive}/>
          <UpRowActive t={t} name="concert_recording.flac" pct={88} size="620 MB" done="545 MB" speed="8.8 MB/s" eta="8 giây" icon={Icons.music}/>
          <UpRowActive t={t} name="Báo cáo_quý4_final_v3.pdf" pct={3} size="48 MB" done="1.4 MB" speed="3.2 MB/s" eta="14 giây" icon={Icons.doc}/>
          <UpRowQueued t={t} name="homeVideo_family_2025.mp4" size="8.1 GB" icon={Icons.video}/>
          <UpRowQueued t={t} name="Portfolio_design_2026.zip" size="1.2 GB" icon={Icons.archive}/>
          <UpRowQueued t={t} name="studio_session_vol3.flac" size="440 MB" icon={Icons.music}/>
        </div>
      </div>
    </WinChrome>
  );
}

function ChipMini({ t, children }) {
  return (
    <span style={{ fontSize: 10, fontWeight: 600, padding: '3px 8px', background: t.panel, color: t.ink3, borderRadius: 999, border: `1px solid ${t.border}`, fontFamily: t.mono, letterSpacing: '.04em' }}>{children}</span>
  );
}

function UpRowActive({ t, name, pct, size, done, speed, eta, icon }) {
  const Ico = icon;
  return (
    <div style={{ background: t.panel, border: `1px solid ${t.border}`, borderRadius: 14, padding: '14px 18px', display: 'flex', alignItems: 'center', gap: 16 }}>
      <div style={{ width: 44, height: 44, borderRadius: 10, background: t.gradSoft, display: 'flex', alignItems: 'center', justifyContent: 'center', color: t.accent, flexShrink: 0 }}><Ico size={20}/></div>
      <div style={{ flex: 1, minWidth: 0 }}>
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'baseline' }}>
          <div style={{ fontSize: 13.5, fontWeight: 600, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis', maxWidth: 440 }}>{name}</div>
          <div style={{ fontSize: 11.5, fontFamily: t.mono, fontWeight: 600, color: t.accent, display: 'flex', alignItems: 'center', gap: 5 }}>
            <Icons.arrowUp size={11}/> {speed}
          </div>
        </div>
        <div style={{ fontSize: 11, color: t.ink4, fontFamily: t.mono, marginTop: 2, display: 'flex', gap: 10 }}>
          <span>{done} / {size}</span>·<span>còn {eta}</span>·<span style={{ color: t.success }}>● đang mã hoá</span>
        </div>
        <div style={{ marginTop: 10, height: 5, background: t.border, borderRadius: 4, overflow: 'hidden' }}>
          <div style={{ width: `${pct}%`, height: '100%', background: t.grad, borderRadius: 4 }}/>
        </div>
      </div>
      <div style={{ display: 'flex', gap: 6 }}>
        <IconBtn theme={t}><Icons.pause size={14}/></IconBtn>
        <IconBtn theme={t}><Icons.x size={14}/></IconBtn>
      </div>
    </div>
  );
}

function UpRowQueued({ t, name, size, icon }) {
  const Ico = icon;
  return (
    <div style={{ background: 'transparent', border: `1px dashed ${t.border}`, borderRadius: 12, padding: '10px 18px', display: 'flex', alignItems: 'center', gap: 14 }}>
      <div style={{ width: 32, height: 32, borderRadius: 8, background: t.bg, display: 'flex', alignItems: 'center', justifyContent: 'center', color: t.ink3 }}><Ico size={15}/></div>
      <div style={{ flex: 1 }}>
        <div style={{ fontSize: 12.5, fontWeight: 500, color: t.ink2 }}>{name}</div>
        <div style={{ fontSize: 10.5, color: t.ink4, fontFamily: t.mono, marginTop: 1 }}>{size} · chờ đến lượt</div>
      </div>
      <span style={{ fontSize: 10.5, color: t.ink4, fontFamily: t.mono, letterSpacing: '.1em', textTransform: 'uppercase' }}>Đang chờ</span>
      <Icons.x size={13} style={{ color: t.ink4, cursor: 'pointer' }}/>
    </div>
  );
}

// ═══════════════ ACCOUNT ═══════════════
function AuroraAccount() {
  const t = useTA();
  const [collapsed, setCollapsed] = React.useState(false);
  return (
    <WinChrome title="Fshare · Tài khoản" theme={t}>
      <AuroraSidebar active="account" collapsed={collapsed} onToggle={()=>setCollapsed(v=>!v)}/>
      <div style={{ flex: 1, overflow: 'auto', background: t.bg }}>
        {/* hero plan card */}
        <div style={{ padding: '28px 32px 24px' }}>
          <div style={{ fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', marginBottom: 10 }}>━━ Tài khoản</div>
          <h1 style={{ fontFamily: t.serif, fontSize: 48, fontWeight: 400, letterSpacing: '-.03em', margin: 0 }}>
            Nguyễn Minh, <span style={{ fontStyle: 'italic', color: t.accent }}>VIP.</span>
          </h1>

          <div style={{ marginTop: 22, display: 'grid', gridTemplateColumns: '2fr 1fr', gap: 18 }}>
            {/* VIP big card */}
            <div style={{ background: t.sidebar, color: '#fff', borderRadius: 18, padding: '28px 30px', position: 'relative', overflow: 'hidden' }}>
              <div style={{ position: 'absolute', width: 400, height: 400, borderRadius: '50%', background: 'radial-gradient(circle, #FF3D7F 0%, transparent 60%)', filter: 'blur(60px)', top: -100, right: -120, opacity: .45 }}/>
              <div style={{ position: 'relative', display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start' }}>
                <div>
                  <div style={{ fontSize: 10, fontFamily: t.mono, color: t.accent2, letterSpacing: '.18em', textTransform: 'uppercase', fontWeight: 700, display: 'flex', alignItems: 'center', gap: 6 }}>
                    <Icons.crown size={11} stroke={t.accent2}/> Gói VIP · 1TB
                  </div>
                  <div style={{ fontFamily: t.serif, fontSize: 42, fontWeight: 400, letterSpacing: '-.03em', marginTop: 8 }}>
                    287 / 500 <span style={{ fontSize: 20, color: 'rgba(255,255,255,.5)', fontStyle: 'italic' }}>GB đã dùng</span>
                  </div>
                  <div style={{ fontSize: 12, color: 'rgba(255,255,255,.65)', fontFamily: t.mono, marginTop: 8 }}>
                    Còn 213 GB · hết hạn 09/12/2026 (243 ngày)
                  </div>
                </div>
                <button style={{ padding: '10px 18px', background: t.grad, color: '#fff', border: 'none', borderRadius: 10, fontSize: 13, fontWeight: 600, cursor: 'pointer', fontFamily: t.font, boxShadow: t.shadow }}>Gia hạn +12 tháng</button>
              </div>

              {/* storage bar with segments */}
              <div style={{ marginTop: 24, height: 10, background: 'rgba(255,255,255,.08)', borderRadius: 10, overflow: 'hidden', display: 'flex' }}>
                <div style={{ width: '38%', background: t.accent3 }}/>
                <div style={{ width: '14%', background: t.accent }}/>
                <div style={{ width: '5%', background: t.accent2 }}/>
              </div>
              <div style={{ marginTop: 12, display: 'flex', gap: 20, fontSize: 11, color: 'rgba(255,255,255,.8)', fontFamily: t.mono }}>
                <LegDot color={t.accent3}>Video · 192 GB</LegDot>
                <LegDot color={t.accent}>Ảnh · 68 GB</LegDot>
                <LegDot color={t.accent2}>Tài liệu · 27 GB</LegDot>
                <LegDot color="rgba(255,255,255,.3)">Trống · 213 GB</LegDot>
              </div>
            </div>

            {/* speed + perks */}
            <div style={{ background: t.panel, border: `1px solid ${t.border}`, borderRadius: 18, padding: '20px 22px' }}>
              <div style={{ fontSize: 10, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600 }}>Quyền lợi</div>
              <PerkRow t={t} icon={Icons.zap} label="Tốc độ tải" value="Không giới hạn"/>
              <PerkRow t={t} icon={Icons.cloud} label="Dung lượng" value="500 GB"/>
              <PerkRow t={t} icon={Icons.share} label="Link đồng thời" value="Không giới hạn"/>
              <PerkRow t={t} icon={Icons.shield} label="Mã hoá" value="AES-256"/>
              <PerkRow t={t} icon={Icons.bell} label="Ưu tiên hỗ trợ" value="24/7" last/>
            </div>
          </div>
        </div>

        {/* profile + security grid */}
        <div style={{ padding: '8px 32px 32px', display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 18 }}>
          {/* profile */}
          <div style={{ background: t.panel, border: `1px solid ${t.border}`, borderRadius: 16, padding: '22px 24px' }}>
            <div style={{ fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600, marginBottom: 14 }}>Thông tin cá nhân</div>
            <div style={{ display: 'flex', alignItems: 'center', gap: 16, paddingBottom: 18, borderBottom: `1px solid ${t.border}` }}>
              <div style={{ width: 64, height: 64, borderRadius: 20, background: t.grad, display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff', fontWeight: 700, fontSize: 26, fontFamily: t.serif, fontStyle: 'italic', boxShadow: t.shadow }}>M</div>
              <div style={{ flex: 1 }}>
                <div style={{ fontSize: 17, fontWeight: 700, letterSpacing: '-.01em' }}>Nguyễn Minh</div>
                <div style={{ fontSize: 12, color: t.ink3, fontFamily: t.mono, marginTop: 2 }}>@minhng · Tham gia 03/2022</div>
              </div>
              <button style={btnGhost(t)}>Đổi ảnh</button>
            </div>
            <InfoRow t={t} label="Email" value="minh.nguyen@fpt.com" verified/>
            <InfoRow t={t} label="Điện thoại" value="+84 ••• ••• 342"/>
            <InfoRow t={t} label="Quốc gia" value="Việt Nam"/>
            <InfoRow t={t} label="Ngôn ngữ" value="Tiếng Việt" last/>
          </div>

          {/* security */}
          <div style={{ background: t.panel, border: `1px solid ${t.border}`, borderRadius: 16, padding: '22px 24px' }}>
            <div style={{ fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600, marginBottom: 14 }}>Bảo mật</div>
            <SecRow t={t} icon={Icons.lock} title="Mật khẩu" sub="Đổi 2 tháng trước" action="Đổi"/>
            <SecRow t={t} icon={Icons.shield} title="Xác minh 2 bước" sub="Qua Zalo · đã bật" state="on"/>
            <SecRow t={t} icon={Icons.users} title="Phiên đăng nhập" sub="3 thiết bị đang hoạt động" action="Xem"/>
            <SecRow t={t} icon={Icons.bell} title="Thông báo đăng nhập lạ" sub="Email + push" state="on" last/>

            <div style={{ marginTop: 18, padding: '12px 14px', background: t.dangerSoft, borderRadius: 10, display: 'flex', alignItems: 'center', gap: 10 }}>
              <Icons.shield size={15} stroke={t.danger}/>
              <div style={{ flex: 1, fontSize: 12, color: t.danger, fontWeight: 500 }}>Đăng xuất khỏi tất cả thiết bị</div>
              <Icons.chevRight size={14} stroke={t.danger}/>
            </div>
          </div>
        </div>

        {/* billing history */}
        <div style={{ padding: '0 32px 40px' }}>
          <div style={{ fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600, marginBottom: 12, display: 'flex', justifyContent: 'space-between' }}>
            <span>Lịch sử thanh toán</span>
            <a style={{ color: t.accent, fontWeight: 600, textTransform: 'none', letterSpacing: 0 }}>Tải hoá đơn →</a>
          </div>
          <div style={{ background: t.panel, border: `1px solid ${t.border}`, borderRadius: 14, overflow: 'hidden' }}>
            <BillRow t={t} date="12/03/2026" plan="VIP 12 tháng" amount="990.000 đ" method="VISA ···2847" status="success"/>
            <BillRow t={t} date="15/03/2025" plan="VIP 12 tháng" amount="880.000 đ" method="Momo" status="success"/>
            <BillRow t={t} date="18/03/2024" plan="VIP 12 tháng" amount="880.000 đ" method="VISA ···2847" status="success" last/>
          </div>
        </div>
      </div>
    </WinChrome>
  );
}

function LegDot({ color, children }) {
  return <span style={{ display: 'flex', alignItems: 'center', gap: 6 }}><div style={{ width: 7, height: 7, borderRadius: '50%', background: color }}/>{children}</span>;
}

function PerkRow({ t, icon, label, value, last }) {
  const Ico = icon;
  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: 12, padding: '11px 0', borderBottom: last ? 'none' : `1px solid ${t.border}` }}>
      <div style={{ width: 28, height: 28, borderRadius: 8, background: t.gradSoft, color: t.accent, display: 'flex', alignItems: 'center', justifyContent: 'center' }}><Ico size={14}/></div>
      <div style={{ flex: 1, fontSize: 12.5, color: t.ink2 }}>{label}</div>
      <div style={{ fontSize: 12.5, fontWeight: 600, color: t.ink }}>{value}</div>
    </div>
  );
}

function InfoRow({ t, label, value, verified, last }) {
  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: 10, padding: '12px 0', borderBottom: last ? 'none' : `1px dashed ${t.border}` }}>
      <div style={{ fontSize: 12, color: t.ink3, width: 100 }}>{label}</div>
      <div style={{ flex: 1, fontSize: 13, fontWeight: 500, display: 'flex', alignItems: 'center', gap: 6 }}>
        {value}
        {verified && <span style={{ fontSize: 9, fontWeight: 700, padding: '2px 6px', background: 'rgba(10,138,92,.15)', color: t.success, borderRadius: 4 }}>✓ ĐÃ XÁC MINH</span>}
      </div>
      <Icons.chevRight size={13} style={{ color: t.ink4 }}/>
    </div>
  );
}

function SecRow({ t, icon, title, sub, action, state, last }) {
  const Ico = icon;
  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: 12, padding: '12px 0', borderBottom: last ? 'none' : `1px solid ${t.border}` }}>
      <div style={{ width: 32, height: 32, borderRadius: 8, background: t.bg, color: t.ink2, display: 'flex', alignItems: 'center', justifyContent: 'center' }}><Ico size={14}/></div>
      <div style={{ flex: 1 }}>
        <div style={{ fontSize: 13, fontWeight: 600 }}>{title}</div>
        <div style={{ fontSize: 11.5, color: t.ink3, marginTop: 1 }}>{sub}</div>
      </div>
      {state === 'on' && <div style={{ width: 32, height: 18, background: t.grad, borderRadius: 999, padding: 2, display: 'flex', justifyContent: 'flex-end' }}><div style={{ width: 14, height: 14, background: '#fff', borderRadius: '50%' }}/></div>}
      {action && <span style={{ fontSize: 12, color: t.accent, fontWeight: 600, cursor: 'pointer' }}>{action}</span>}
    </div>
  );
}

function BillRow({ t, date, plan, amount, method, status, last }) {
  return (
    <div style={{ display: 'grid', gridTemplateColumns: '120px 1fr 1fr 140px 90px', padding: '14px 20px', borderBottom: last ? 'none' : `1px solid ${t.border}`, fontSize: 13, alignItems: 'center' }}>
      <span style={{ fontFamily: t.mono, color: t.ink3, fontSize: 12 }}>{date}</span>
      <span style={{ fontWeight: 600 }}>{plan}</span>
      <span style={{ fontFamily: t.mono, fontWeight: 600 }}>{amount}</span>
      <span style={{ fontSize: 12, color: t.ink3 }}>{method}</span>
      <span style={{ fontSize: 11, fontWeight: 600, color: t.success, padding: '3px 8px', background: 'rgba(10,138,92,.12)', borderRadius: 4, textAlign: 'center', justifySelf: 'end' }}>✓ Thành công</span>
    </div>
  );
}

// ═══════════════ FAVORITES ═══════════════
function AuroraFavorites() {
  const t = useTA();
  const [collapsed, setCollapsed] = React.useState(false);
  const [view, setView] = React.useState('grid');
  return (
    <WinChrome title="Fshare · Đã gắn sao" theme={t}>
      <AuroraSidebar active="starred" collapsed={collapsed} onToggle={()=>setCollapsed(v=>!v)}/>
      <div style={{ flex: 1, overflow: 'auto', background: t.bg }}>
        <div style={{ padding: '28px 32px 18px', background: t.panel, borderBottom: `1px solid ${t.border}` }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-end' }}>
            <div>
              <div style={{ fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', marginBottom: 8 }}>━━ Bộ sưu tập cá nhân</div>
              <h1 style={{ fontFamily: t.serif, fontSize: 48, fontWeight: 400, letterSpacing: '-.03em', margin: 0 }}>
                Yêu <span style={{ fontStyle: 'italic', color: t.accent }}>thích.</span>
              </h1>
              <div style={{ fontSize: 12, color: t.ink3, fontFamily: t.mono, marginTop: 10 }}>
                42 mục · 184 GB · gần nhất 2 giờ trước
              </div>
            </div>
            <div style={{ display: 'flex', gap: 6, alignItems: 'center' }}>
              <button style={btnGhost(t)}><Icons.filter size={13}/> Loại · Tất cả</button>
              <button style={btnGhost(t)}><Icons.arrowDown size={13}/> Mới thêm</button>
              <div style={{ width: 1, height: 20, background: t.border, margin: '0 4px' }}/>
              <div style={{ display: 'flex', background: t.bg, border: `1px solid ${t.border}`, borderRadius: 8, padding: 2 }}>
                <ViewTog t={t} active={view==='grid'} onClick={()=>setView('grid')}><Icons.grid size={13}/></ViewTog>
                <ViewTog t={t} active={view==='list'} onClick={()=>setView('list')}><Icons.list size={13}/></ViewTog>
              </div>
            </div>
          </div>

          {/* collections ribbon */}
          <div style={{ marginTop: 22, display: 'flex', gap: 10 }}>
            <CollChip t={t} active count="42" label="Tất cả" icon={Icons.star}/>
            <CollChip t={t} count="18" label="Phim" icon={Icons.video}/>
            <CollChip t={t} count="12" label="Nhạc" icon={Icons.music}/>
            <CollChip t={t} count="8" label="Tài liệu" icon={Icons.doc}/>
            <CollChip t={t} count="4" label="Ảnh" icon={Icons.image}/>
            <div style={{ marginLeft: 'auto' }}>
              <button style={{ ...btnGhost(t), border: `1px dashed ${t.border}`, color: t.ink3 }}><Icons.plus size={12}/> Tạo bộ sưu tập</button>
            </div>
          </div>
        </div>

        {/* grid */}
        <div style={{ padding: '22px 32px 32px', display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(220px, 1fr))', gap: 14 }}>
          <FavCard t={t} title="Oppenheimer" sub="18.2 GB · 4K IMAX" gradient="linear-gradient(135deg,#1a1a2e,#E94560)" tag="Phim"/>
          <FavCard t={t} title="Dua Lipa · Radical" sub="420 MB · FLAC" gradient="linear-gradient(135deg,#f0abfc,#f472b6)" tag="Nhạc"/>
          <FavCard t={t} title="Dune Part Two" sub="16.8 GB · 4K" gradient="linear-gradient(135deg,#4a3c1f,#d4a554)" tag="Phim"/>
          <FavCard t={t} title="Album ảnh cưới" sub="1.2 GB · 842 ảnh" gradient="linear-gradient(135deg,#fde68a,#f97316)" tag="Ảnh" shared/>
          <FavCard t={t} title="Tài liệu ACM 2026" sub="240 MB · 42 PDF" gradient="linear-gradient(135deg,#0369a1,#38bdf8)" tag="Tài liệu"/>
          <FavCard t={t} title="Interstellar OST" sub="820 MB · FLAC" gradient="linear-gradient(135deg,#1e293b,#64748b)" tag="Nhạc"/>
          <FavCard t={t} title="The Batman 4K" sub="22.8 GB · Remux" gradient="linear-gradient(135deg,#1a1a1a,#7f1d1d)" tag="Phim"/>
          <FavCard t={t} title="Design Playbook" sub="84 MB · Figma exports" gradient="linear-gradient(135deg,#7c3aed,#db2777)" tag="Tài liệu" shared/>
        </div>
      </div>
    </WinChrome>
  );
}

function ViewTog({ t, active, onClick, children }) {
  return <div onClick={onClick} style={{ padding: '5px 9px', borderRadius: 6, background: active ? t.panel : 'transparent', color: active ? t.accent : t.ink4, cursor: 'pointer', boxShadow: active ? `0 1px 3px rgba(0,0,0,.06)` : 'none' }}>{children}</div>;
}

function CollChip({ t, active, count, label, icon }) {
  const Ico = icon;
  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: 8, padding: '8px 14px', borderRadius: 999, background: active ? t.ink : t.panel, color: active ? '#fff' : t.ink2, border: active ? 'none' : `1px solid ${t.border}`, cursor: 'pointer', fontSize: 12.5, fontWeight: 600 }}>
      <Ico size={13}/> {label}
      <span style={{ fontSize: 10, padding: '1px 7px', background: active ? 'rgba(255,255,255,.15)' : t.bg, borderRadius: 999, fontFamily: t.mono, color: active ? '#fff' : t.ink4 }}>{count}</span>
    </div>
  );
}

function FavCard({ t, title, sub, gradient, tag, shared }) {
  return (
    <div style={{ background: t.panel, border: `1px solid ${t.border}`, borderRadius: 14, overflow: 'hidden', cursor: 'pointer' }}>
      <div style={{ aspectRatio: '16/10', background: gradient, position: 'relative' }}>
        <div style={{ position: 'absolute', top: 10, left: 10, padding: '3px 8px', background: 'rgba(0,0,0,.55)', backdropFilter: 'blur(8px)', borderRadius: 999, fontSize: 9.5, fontFamily: t.mono, color: '#fff', letterSpacing: '.08em', fontWeight: 600 }}>{tag}</div>
        <div style={{ position: 'absolute', top: 10, right: 10, width: 28, height: 28, borderRadius: '50%', background: 'rgba(255,255,255,.95)', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
          <Icons.star size={14} stroke={t.accent} fill={t.accent}/>
        </div>
        {shared && <div style={{ position: 'absolute', bottom: 10, left: 10, padding: '3px 8px', background: 'rgba(255,255,255,.95)', borderRadius: 999, fontSize: 9.5, fontWeight: 600, display: 'flex', alignItems: 'center', gap: 4 }}>
          <Icons.share size={9} stroke={t.accent}/> Đang chia sẻ
        </div>}
      </div>
      <div style={{ padding: '12px 14px 14px' }}>
        <div style={{ fontSize: 13.5, fontWeight: 600, letterSpacing: '-.01em', whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>{title}</div>
        <div style={{ fontSize: 11, color: t.ink4, fontFamily: t.mono, marginTop: 3 }}>{sub}</div>
      </div>
    </div>
  );
}

// ═══════════════ SYNC ═══════════════
function AuroraSync() {
  const t = useTA();
  const [collapsed, setCollapsed] = React.useState(false);
  return (
    <WinChrome title="Fshare · Đồng bộ" theme={t}>
      <AuroraSidebar active="sync" collapsed={collapsed} onToggle={()=>setCollapsed(v=>!v)}/>
      <div style={{ flex: 1, overflow: 'auto', background: t.bg }}>
        {/* hero */}
        <div style={{ padding: '28px 32px 20px', background: t.panel, borderBottom: `1px solid ${t.border}`, display: 'flex', alignItems: 'flex-end', justifyContent: 'space-between' }}>
          <div>
            <div style={{ fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', marginBottom: 8 }}>━━ Đồng bộ 2 chiều</div>
            <h1 style={{ fontFamily: t.serif, fontSize: 44, fontWeight: 400, letterSpacing: '-.03em', margin: 0 }}>
              3 folder <span style={{ fontStyle: 'italic', color: t.accent }}>đang sync.</span>
            </h1>
            <div style={{ marginTop: 10, display: 'flex', gap: 16, fontSize: 12, color: t.ink3, fontFamily: t.mono }}>
              <span style={{ display: 'flex', alignItems: 'center', gap: 5 }}>
                <div style={{ width: 7, height: 7, borderRadius: '50%', background: t.success, boxShadow: `0 0 8px ${t.success}` }}/>
                Hoạt động · 88.3 MB/s
              </span>
              <span>14,284 file tổng</span>
              <span>2 xung đột</span>
            </div>
          </div>
          <div style={{ display: 'flex', gap: 8 }}>
            <button style={btnGhost(t)}><Icons.pause size={13}/> Tạm dừng</button>
            <button style={{ ...btnGhost(t), background: t.grad, color: '#fff', border: 'none', boxShadow: t.shadow }}><Icons.plus size={13}/> Thêm folder sync</button>
          </div>
        </div>

        {/* global activity + speed */}
        <div style={{ padding: '18px 32px 8px', display: 'grid', gridTemplateColumns: '1fr 1fr 1fr', gap: 14 }}>
          <MetricBig t={t} label="Upload hôm nay" value="2.4 GB" sub="42 file" dir="up"/>
          <MetricBig t={t} label="Download hôm nay" value="1.1 GB" sub="18 file" dir="down"/>
          <MetricBig t={t} label="Tiết kiệm băng thông" value="-62%" sub="Dedup bật" sparkline/>
        </div>

        {/* folder pairs */}
        <div style={{ padding: '18px 32px 8px', fontSize: 11, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600 }}>
          ━ Cặp thư mục · 3
        </div>

        <div style={{ padding: '0 32px', display: 'flex', flexDirection: 'column', gap: 12 }}>
          <SyncPair t={t} active local="C:\Users\Minh\Documents\Công việc" cloud="/Tài liệu/Công việc" files={284} size="14.2 GB" state="syncing" progress="12 / 284 file · 62 MB/s"/>
          <SyncPair t={t} local="D:\Photos\2026" cloud="/Ảnh/2026" files={1842} size="68 GB" state="idle" progress="Đã sync · 6 phút trước"/>
          <SyncPair t={t} local="E:\Media\Phim 4K" cloud="/Phim/2026" files={12} size="142 GB" state="conflict" progress="2 xung đột cần xử lý"/>
        </div>

        {/* conflict resolution */}
        <div style={{ padding: '24px 32px 8px', fontSize: 11, fontFamily: t.mono, color: t.danger, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600 }}>
          ⚠ Xung đột · 2 mục
        </div>

        <div style={{ padding: '0 32px', display: 'flex', flexDirection: 'column', gap: 10 }}>
          <ConflictCard t={t} file="report_final.docx" path="/Tài liệu/Công việc/"
            local={{ time: 'Hôm nay 14:22', size: '2.4 MB', by: 'PC-Work' }}
            cloud={{ time: 'Hôm nay 15:08', size: '2.6 MB', by: 'Laptop-Home' }}/>
          <ConflictCard t={t} file="Season_04_E02.mkv" path="/Phim/2026/"
            local={{ time: 'Hôm qua 21:40', size: '8.1 GB', by: 'PC-Work' }}
            cloud={{ time: 'Hôm qua 22:14', size: '8.2 GB', by: 'Server' }}/>
        </div>

        {/* selective sync */}
        <div style={{ padding: '24px 32px 40px' }}>
          <div style={{ background: t.panel, border: `1px solid ${t.border}`, borderRadius: 16, padding: '20px 22px' }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 14 }}>
              <div>
                <div style={{ fontSize: 14, fontWeight: 700 }}>Sync có chọn lọc</div>
                <div style={{ fontSize: 11.5, color: t.ink3, marginTop: 2 }}>Chỉ kéo về máy các folder bạn tick — tiết kiệm dung lượng ổ cứng</div>
              </div>
              <span style={{ fontSize: 11, color: t.ink3, fontFamily: t.mono }}>Đã chọn 6 / 12 folder · 184 GB</span>
            </div>
            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: 8 }}>
              <SelFolder t={t} name="Phim" size="412 GB" on/>
              <SelFolder t={t} name="Âm nhạc" size="102 GB" on/>
              <SelFolder t={t} name="Tài liệu" size="14 GB" on/>
              <SelFolder t={t} name="Ảnh" size="48 GB"/>
              <SelFolder t={t} name="Backup" size="238 GB"/>
              <SelFolder t={t} name="Phần mềm" size="142 GB" on/>
            </div>
          </div>
        </div>
      </div>
    </WinChrome>
  );
}

function MetricBig({ t, label, value, sub, dir, sparkline }) {
  return (
    <div style={{ background: t.panel, border: `1px solid ${t.border}`, borderRadius: 14, padding: '18px 20px' }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start' }}>
        <div>
          <div style={{ fontSize: 10.5, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600 }}>{label}</div>
          <div style={{ fontFamily: t.serif, fontSize: 34, fontWeight: 400, letterSpacing: '-.02em', marginTop: 6, lineHeight: 1 }}>{value}</div>
          <div style={{ fontSize: 11, color: t.ink4, fontFamily: t.mono, marginTop: 4 }}>{sub}</div>
        </div>
        {dir && <div style={{ width: 32, height: 32, borderRadius: 10, background: t.gradSoft, color: t.accent, display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
          {dir === 'up' ? <Icons.arrowUp size={14}/> : <Icons.arrowDown size={14}/>}
        </div>}
        {sparkline && <svg width="80" height="32" viewBox="0 0 80 32">
          <path d="M 0 24 L 10 22 L 20 18 L 30 20 L 40 14 L 50 10 L 60 8 L 70 4 L 80 6" fill="none" stroke={t.accent} strokeWidth="2"/>
        </svg>}
      </div>
    </div>
  );
}

function SyncPair({ t, active, local, cloud, files, size, state, progress }) {
  const badges = {
    syncing: { c: t.accent, label: '↻ Đang sync', bg: t.gradSoft },
    idle: { c: t.success, label: '✓ Đã sync', bg: 'rgba(10,138,92,.1)' },
    conflict: { c: t.danger, label: '⚠ Xung đột', bg: t.dangerSoft },
  };
  const b = badges[state];
  return (
    <div style={{
      background: t.panel, border: `${active ? 1.5 : 1}px solid ${active ? t.accent : t.border}`,
      borderRadius: 14, padding: '16px 20px',
      boxShadow: active ? '0 4px 18px rgba(255,91,46,.12)' : 'none',
    }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: 18 }}>
        {/* local */}
        <div style={{ flex: 1, minWidth: 0 }}>
          <div style={{ fontSize: 10, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600 }}>Máy tính</div>
          <div style={{ display: 'flex', alignItems: 'center', gap: 8, marginTop: 4 }}>
            <Icons.folder size={15} stroke={t.ink2}/>
            <span style={{ fontSize: 13, fontFamily: t.mono, fontWeight: 500, color: t.ink, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>{local}</span>
          </div>
        </div>

        {/* arrows */}
        <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', color: state === 'syncing' ? t.accent : t.ink4, gap: 2 }}>
          <Icons.arrowUp size={12}/>
          <Icons.arrowDown size={12}/>
        </div>

        {/* cloud */}
        <div style={{ flex: 1, minWidth: 0 }}>
          <div style={{ fontSize: 10, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600 }}>Fshare Cloud</div>
          <div style={{ display: 'flex', alignItems: 'center', gap: 8, marginTop: 4 }}>
            <Icons.cloud size={15} stroke={t.accent}/>
            <span style={{ fontSize: 13, fontFamily: t.mono, fontWeight: 500, color: t.ink }}>{cloud}</span>
          </div>
        </div>

        {/* state badge */}
        <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'flex-end', gap: 5, minWidth: 160 }}>
          <span style={{ fontSize: 11, fontWeight: 700, padding: '4px 10px', background: b.bg, color: b.c, borderRadius: 999 }}>{b.label}</span>
          <span style={{ fontSize: 10.5, color: t.ink4, fontFamily: t.mono }}>{files} file · {size}</span>
        </div>

        <IconBtn theme={t}><Icons.more size={14}/></IconBtn>
      </div>

      {state === 'syncing' && (
        <div style={{ marginTop: 12, display: 'flex', alignItems: 'center', gap: 12 }}>
          <div style={{ flex: 1, height: 4, background: t.border, borderRadius: 4, overflow: 'hidden' }}>
            <div style={{ width: '42%', height: '100%', background: t.grad, borderRadius: 4 }}/>
          </div>
          <span style={{ fontSize: 11, color: t.accent, fontFamily: t.mono, fontWeight: 600, whiteSpace: 'nowrap' }}>{progress}</span>
        </div>
      )}
      {state !== 'syncing' && (
        <div style={{ marginTop: 8, fontSize: 11, color: state === 'conflict' ? t.danger : t.ink4, fontFamily: t.mono }}>{progress}</div>
      )}
    </div>
  );
}

function ConflictCard({ t, file, path, local, cloud }) {
  return (
    <div style={{ background: t.panel, border: `1px solid ${t.border}`, borderRadius: 14, padding: '14px 18px' }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: 10, marginBottom: 12 }}>
        <div style={{ width: 32, height: 32, borderRadius: 8, background: t.dangerSoft, color: t.danger, display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
          <Icons.shield size={15}/>
        </div>
        <div style={{ flex: 1 }}>
          <div style={{ fontSize: 13, fontWeight: 600 }}>{file}</div>
          <div style={{ fontSize: 11, color: t.ink4, fontFamily: t.mono, marginTop: 1 }}>{path}</div>
        </div>
      </div>

      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 10 }}>
        <ConflictOpt t={t} title="Bản máy tính" {...local} icon={Icons.folder}/>
        <ConflictOpt t={t} title="Bản cloud" {...cloud} icon={Icons.cloud} highlight/>
      </div>
      <div style={{ marginTop: 10, display: 'flex', gap: 6, alignItems: 'center', fontSize: 11.5, color: t.ink3 }}>
        <span style={{ flex: 1 }}>Hoặc:</span>
        <button style={{ ...btnGhost(t), padding: '6px 10px', fontSize: 11.5 }}>Giữ cả hai (đổi tên)</button>
        <button style={{ ...btnGhost(t), padding: '6px 10px', fontSize: 11.5, color: t.danger }}>Bỏ qua</button>
      </div>
    </div>
  );
}

function ConflictOpt({ t, title, time, size, by, icon, highlight }) {
  const Ico = icon;
  return (
    <div style={{
      padding: '10px 12px', borderRadius: 10,
      border: `1.5px solid ${highlight ? t.accent : t.border}`,
      background: highlight ? t.gradSoft : t.bg,
      cursor: 'pointer',
    }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: 8, marginBottom: 6 }}>
        <Ico size={13} stroke={highlight ? t.accent : t.ink3}/>
        <div style={{ fontSize: 12, fontWeight: 700, color: t.ink }}>{title}</div>
        {highlight && <span style={{ marginLeft: 'auto', fontSize: 9, fontWeight: 700, padding: '2px 6px', background: t.accent, color: '#fff', borderRadius: 4 }}>MỚI HƠN</span>}
      </div>
      <div style={{ fontSize: 10.5, color: t.ink3, fontFamily: t.mono, display: 'grid', gridTemplateColumns: '70px 1fr', rowGap: 3 }}>
        <span>Lúc</span><span style={{ color: t.ink }}>{time}</span>
        <span>Kích thước</span><span style={{ color: t.ink }}>{size}</span>
        <span>Từ</span><span style={{ color: t.ink }}>{by}</span>
      </div>
    </div>
  );
}

function SelFolder({ t, name, size, on }) {
  return (
    <div style={{
      padding: '10px 12px', display: 'flex', alignItems: 'center', gap: 10,
      background: on ? t.gradSoft : t.bg, borderRadius: 10,
      border: `1px solid ${on ? t.accent : t.border}`, cursor: 'pointer',
    }}>
      <div style={{
        width: 16, height: 16, borderRadius: 4, flexShrink: 0,
        background: on ? t.grad : 'transparent',
        border: on ? 'none' : `1.5px solid ${t.border}`,
        display: 'flex', alignItems: 'center', justifyContent: 'center',
      }}>{on && <Icons.check size={10} stroke="#fff" sw={3}/>}</div>
      <Icons.folder size={13} stroke={on ? t.accent : t.ink3}/>
      <div style={{ flex: 1 }}>
        <div style={{ fontSize: 12, fontWeight: 600, color: t.ink }}>{name}</div>
        <div style={{ fontSize: 10, color: t.ink4, fontFamily: t.mono }}>{size}</div>
      </div>
    </div>
  );
}

Object.assign(window, { AuroraUpload, AuroraAccount, AuroraFavorites, AuroraSync });
