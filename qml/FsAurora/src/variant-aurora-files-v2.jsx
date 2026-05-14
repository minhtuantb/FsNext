// variant-aurora-files-v2.jsx — File Manager phương án 2: Column View (Miller)
// Layout: mỗi cấp folder mở ra một cột; cuộn ngang khi đi sâu.
// Khác với phương án 1 (3-panel tree+grid+detail), phương án này thiên về KHÁM PHÁ.

function AuroraFilesV2() {
  const t = useTA();

  return (
    <WinChrome title="Fshare · Duyệt file" theme={t}>
      <AuroraSidebar active="files"/>
      <div style={{ flex: 1, display: 'flex', flexDirection: 'column', background: t.bg, overflow: 'hidden' }}>
        {/* ── TOP BAR · breadcrumb + search ── */}
        <div style={{
          padding: '14px 24px', background: t.panel,
          borderBottom: `1px solid ${t.border}`,
          display: 'flex', alignItems: 'center', gap: 16,
        }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: 6, color: t.ink3 }}>
            <IconBtn theme={t}><Icons.chevRight size={14} style={{ transform: 'rotate(180deg)' }}/></IconBtn>
            <IconBtn theme={t}><Icons.chevRight size={14}/></IconBtn>
          </div>

          {/* path chips */}
          <div style={{ display: 'flex', alignItems: 'center', gap: 4, flex: 1 }}>
            <PathChip t={t} icon={Icons.cloud}>File của tôi</PathChip>
            <PathSep t={t}/>
            <PathChip t={t}>Phim</PathChip>
            <PathSep t={t}/>
            <PathChip t={t}>2025</PathChip>
            <PathSep t={t}/>
            <PathChip t={t} active>IMAX Remux</PathChip>
          </div>

          <div style={{
            display: 'flex', alignItems: 'center', gap: 8, padding: '7px 12px',
            background: t.bg, border: `1px solid ${t.border}`, borderRadius: 8,
            width: 240, color: t.ink3,
          }}>
            <Icons.search size={13}/>
            <input placeholder="Tìm trong thư mục này..."
              style={{ flex: 1, border: 'none', outline: 'none', background: 'transparent', fontSize: 12, fontFamily: t.font, color: t.ink }}/>
          </div>

          <div style={{ display: 'flex', gap: 6 }}>
            <IconBtn theme={t}><Icons.plus size={14}/></IconBtn>
            <IconBtn theme={t}><Icons.upload size={14}/></IconBtn>
            <button style={{
              padding: '7px 12px', background: t.grad, color: '#fff', border: 'none',
              borderRadius: 8, fontSize: 12, fontWeight: 600, cursor: 'pointer',
              display: 'flex', alignItems: 'center', gap: 5, fontFamily: t.font,
            }}><Icons.share size={12}/> Chia sẻ</button>
          </div>
        </div>

        {/* ── COLUMN VIEW ── */}
        <div style={{ flex: 1, display: 'flex', overflow: 'hidden', background: t.bg }}>
          {/* Column 1: Root */}
          <Col t={t} title="File của tôi" count="12 mục" width={240}>
            <ColRow t={t} icon={Icons.folder} label="Phim" sub="184 · 412 GB" selected hasChildren/>
            <ColRow t={t} icon={Icons.folder} label="Âm nhạc" sub="1.2K · 102 GB" hasChildren/>
            <ColRow t={t} icon={Icons.folder} label="Tài liệu" sub="342 · 14 GB" hasChildren/>
            <ColRow t={t} icon={Icons.folder} label="Ảnh" sub="8.4K · 48 GB" hasChildren/>
            <ColRow t={t} icon={Icons.folder} label="Backup" sub="24 · 238 GB" hasChildren/>
            <ColRow t={t} icon={Icons.folder} label="Phần mềm" sub="68 · 142 GB" hasChildren/>
            <div style={{ height: 1, background: t.border, margin: '8px 12px' }}/>
            <ColRow t={t} icon={Icons.archive} label="Backup_Photos_2024.rar" sub="8.1 GB"/>
            <ColRow t={t} icon={Icons.video} label="showreel_2025.mp4" sub="1.2 GB"/>
            <ColRow t={t} icon={Icons.doc} label="Đề án nghỉ dưỡng Q4.pdf" sub="2.4 MB" shared/>
          </Col>

          {/* Column 2: Phim */}
          <Col t={t} title="Phim" count="6 thư mục · 184 file" width={240}>
            <ColRow t={t} icon={Icons.folder} label="2026" sub="8" hasChildren/>
            <ColRow t={t} icon={Icons.folder} label="2025" sub="42" selected hasChildren/>
            <ColRow t={t} icon={Icons.folder} label="2024" sub="68" hasChildren/>
            <ColRow t={t} icon={Icons.folder} label="2023" sub="34" hasChildren/>
            <ColRow t={t} icon={Icons.folder} label="Classics" sub="32" hasChildren/>
            <ColRow t={t} icon={Icons.folder} label="Anime" sub="24" hasChildren/>
            <div style={{ height: 1, background: t.border, margin: '8px 12px' }}/>
            <ColRow t={t} icon={Icons.video} label="Top_10_Films_2025.mp4" sub="342 MB"/>
          </Col>

          {/* Column 3: 2025 */}
          <Col t={t} title="2025" count="3 thư mục · 42 file" width={240}>
            <ColRow t={t} icon={Icons.folder} label="Oscar winners" sub="12" hasChildren/>
            <ColRow t={t} icon={Icons.folder} label="Netflix rips" sub="24" hasChildren/>
            <ColRow t={t} icon={Icons.folder} label="IMAX Remux" sub="6" selected hasChildren/>
            <div style={{ height: 1, background: t.border, margin: '8px 12px' }}/>
            <ColRow t={t} icon={Icons.video} label="Oppenheimer_IMAX.mkv" sub="18.4 GB"/>
            <ColRow t={t} icon={Icons.video} label="Dune_Part_Two_4K.mkv" sub="24.1 GB"/>
            <ColRow t={t} icon={Icons.video} label="Interstellar_Rewatch.mp4" sub="8.2 GB"/>
          </Col>

          {/* Column 4: IMAX Remux — với grid thumbnails */}
          <Col t={t} title="IMAX Remux" count="6 file · 112 GB" width={340} noPadding>
            <div style={{ padding: '10px 14px 6px', display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
              <div style={{ fontSize: 10, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600 }}>Xem lưới</div>
              <div style={{ display: 'flex', gap: 2 }}>
                <div style={{ padding: 4, borderRadius: 4, background: t.chipBg, color: t.chipInk }}><Icons.grid size={11}/></div>
                <div style={{ padding: 4, color: t.ink4 }}><Icons.list size={11}/></div>
              </div>
            </div>
            <div style={{ padding: '4px 12px 12px', display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 8 }}>
              <Thumb t={t} title="Oppenheimer" sub="3h 0m · 18.4 GB" gradient="linear-gradient(135deg, #1a1a2e 0%, #E94560 100%)" selected/>
              <Thumb t={t} title="Dune Part Two" sub="2h 46m · 24.1 GB" gradient="linear-gradient(135deg, #4a3c1f 0%, #d4a554 100%)"/>
              <Thumb t={t} title="Mission Imp." sub="2h 43m · 19.2 GB" gradient="linear-gradient(135deg, #0f3460 0%, #16537e 100%)"/>
              <Thumb t={t} title="The Batman" sub="2h 56m · 22.8 GB" gradient="linear-gradient(135deg, #1a1a1a 0%, #4a1a1a 100%)"/>
              <Thumb t={t} title="Interstellar" sub="2h 49m · 16.8 GB" gradient="linear-gradient(135deg, #000 0%, #2a4a6e 100%)"/>
              <Thumb t={t} title="Tenet IMAX" sub="2h 30m · 14.2 GB" gradient="linear-gradient(135deg, #1f1f3a 0%, #7c5cff 100%)"/>
            </div>
          </Col>

          {/* Column 5: Preview */}
          <div style={{
            flex: 1, minWidth: 360, background: t.panel, borderLeft: `1px solid ${t.border}`,
            display: 'flex', flexDirection: 'column', overflow: 'auto',
          }}>
            {/* hero preview */}
            <div style={{
              aspectRatio: '16 / 9',
              background: 'linear-gradient(135deg, #1a1a2e 0%, #E94560 100%)',
              position: 'relative', overflow: 'hidden',
            }}>
              <div style={{ position: 'absolute', inset: 0, background: 'radial-gradient(circle at 30% 40%, rgba(255,255,255,.2), transparent 60%)' }}/>
              <div style={{
                position: 'absolute', top: 12, right: 12,
                padding: '4px 8px', background: 'rgba(0,0,0,.6)', color: '#fff',
                borderRadius: 6, fontSize: 10, fontFamily: t.mono, letterSpacing: '.08em',
              }}>4K · IMAX</div>
              <div style={{
                position: 'absolute', inset: 0, display: 'flex', alignItems: 'center', justifyContent: 'center',
              }}>
                <div style={{
                  width: 56, height: 56, borderRadius: '50%', background: 'rgba(255,255,255,.92)',
                  display: 'flex', alignItems: 'center', justifyContent: 'center',
                  boxShadow: '0 8px 24px rgba(0,0,0,.4)',
                }}>
                  <div style={{
                    width: 0, height: 0, marginLeft: 4,
                    borderLeft: '16px solid #0E0E12',
                    borderTop: '10px solid transparent',
                    borderBottom: '10px solid transparent',
                  }}/>
                </div>
              </div>
              <div style={{
                position: 'absolute', bottom: 12, left: 14, right: 14,
                fontFamily: t.serif, fontSize: 24, fontStyle: 'italic', color: '#fff',
                letterSpacing: '-.02em', lineHeight: 1,
              }}>Oppenheimer</div>
            </div>

            {/* file info */}
            <div style={{ padding: '18px 22px' }}>
              <div style={{ fontSize: 10, fontFamily: t.mono, color: t.accent, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600 }}>Chi tiết</div>
              <div style={{ fontSize: 16, fontWeight: 600, marginTop: 4, letterSpacing: '-.01em' }}>
                Oppenheimer_IMAX_70mm.2023.2160p.mkv
              </div>
              <div style={{ fontSize: 11.5, color: t.ink3, fontFamily: t.mono, marginTop: 4 }}>
                18.4 GB · Sửa 18/03/2026 · .mkv
              </div>

              {/* primary actions */}
              <div style={{ marginTop: 16, display: 'flex', gap: 8 }}>
                <button style={{
                  flex: 1, padding: '10px', background: t.grad, color: '#fff', border: 'none',
                  borderRadius: 10, fontSize: 12.5, fontWeight: 600, cursor: 'pointer',
                  display: 'flex', alignItems: 'center', justifyContent: 'center', gap: 6,
                  boxShadow: t.shadow, fontFamily: t.font,
                }}><Icons.download size={13}/> Tải về máy</button>
                <IconBtn theme={t}><Icons.share size={14}/></IconBtn>
                <IconBtn theme={t}><Icons.star size={14}/></IconBtn>
                <IconBtn theme={t}><Icons.more size={14}/></IconBtn>
              </div>

              {/* tags */}
              <div style={{ marginTop: 16, display: 'flex', gap: 6, flexWrap: 'wrap' }}>
                <Tag t={t}>4K</Tag>
                <Tag t={t}>IMAX</Tag>
                <Tag t={t}>HDR10+</Tag>
                <Tag t={t}>Đã xem</Tag>
              </div>

              {/* meta grid */}
              <div style={{ marginTop: 20, display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 14 }}>
                <MetaCard t={t} label="Độ phân giải" value="3840×2160"/>
                <MetaCard t={t} label="Thời lượng" value="3h 00m"/>
                <MetaCard t={t} label="Bitrate" value="18 Mbps"/>
                <MetaCard t={t} label="Codec" value="HEVC / AAC"/>
              </div>

              {/* share status */}
              <div style={{
                marginTop: 18, padding: '14px 16px',
                background: t.gradSoft, borderRadius: 12,
                border: `1px solid ${t.border}`,
              }}>
                <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
                  <div style={{
                    width: 30, height: 30, borderRadius: 8, background: t.grad,
                    display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff',
                  }}><Icons.link size={13}/></div>
                  <div style={{ flex: 1 }}>
                    <div style={{ fontSize: 12, fontWeight: 600, color: t.ink }}>Đã chia sẻ riêng tư</div>
                    <div style={{ fontSize: 10.5, color: t.ink3, fontFamily: t.mono, marginTop: 1 }}>
                      fshare.vn/s/Op7xK2m · 42 lượt xem
                    </div>
                  </div>
                  <Icons.chevRight size={13} style={{ color: t.ink4 }}/>
                </div>
              </div>

              {/* recent activity */}
              <div style={{ marginTop: 20 }}>
                <div style={{ fontSize: 10, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600, marginBottom: 10 }}>
                  Hoạt động
                </div>
                <ActLine t={t} dot={t.success} text="Đã chia sẻ với 3 người" time="Hôm nay · 14:22"/>
                <ActLine t={t} dot={t.accent} text="Đã tải về từ PC-Work" time="Hôm qua · 21:08"/>
                <ActLine t={t} dot={t.ink4} text="Upload từ Desktop · 4K IMAX" time="18/03/2026"/>
              </div>
            </div>
          </div>
        </div>

        {/* ── STATUS BAR ── */}
        <div style={{
          padding: '8px 20px', background: t.panel,
          borderTop: `1px solid ${t.border}`,
          display: 'flex', alignItems: 'center', gap: 20,
          fontSize: 11, color: t.ink3, fontFamily: t.mono,
        }}>
          <span style={{ display: 'flex', alignItems: 'center', gap: 6 }}>
            <div style={{ width: 7, height: 7, borderRadius: '50%', background: t.success }}/>
            Đã đồng bộ
          </span>
          <span>1 đã chọn · 18.4 GB</span>
          <span style={{ marginLeft: 'auto' }}>287 / 500 GB VIP</span>
          <span style={{ color: t.accent, fontWeight: 600 }}>88.3 MB/s</span>
        </div>
      </div>
    </WinChrome>
  );
}

// ─────────── sub components ───────────
function Col({ t, title, count, width, children, noPadding }) {
  return (
    <div style={{
      width, flexShrink: 0, background: t.panel,
      borderRight: `1px solid ${t.border}`,
      display: 'flex', flexDirection: 'column', overflow: 'hidden',
    }}>
      <div style={{ padding: '12px 16px 8px', borderBottom: `1px solid ${t.border}` }}>
        <div style={{ fontSize: 13, fontWeight: 600, letterSpacing: '-.01em', color: t.ink }}>{title}</div>
        <div style={{ fontSize: 10, fontFamily: t.mono, color: t.ink4, marginTop: 2, letterSpacing: '.08em' }}>{count}</div>
      </div>
      <div style={{ flex: 1, overflow: 'auto', padding: noPadding ? 0 : '6px 0' }}>
        {children}
      </div>
    </div>
  );
}

function ColRow({ t, icon, label, sub, selected, hasChildren, shared }) {
  const Ico = icon;
  const bg = selected ? t.grad : (icon === Icons.folder ? t.gradSoft : (t.mode === 'dark' ? 'rgba(255,255,255,.04)' : '#F5F4F1'));
  const iconColor = selected ? '#fff' : t.accent;
  return (
    <div style={{
      display: 'flex', alignItems: 'center', gap: 10, padding: '7px 12px',
      margin: '1px 8px', borderRadius: 8, cursor: 'pointer',
      background: selected ? (t.mode === 'dark' ? 'rgba(255,122,82,.15)' : 'rgba(255,91,46,.08)') : 'transparent',
      border: selected ? `1px solid ${t.accent}` : '1px solid transparent',
    }}>
      <div style={{
        width: 26, height: 26, borderRadius: 6, background: bg, flexShrink: 0,
        display: 'flex', alignItems: 'center', justifyContent: 'center', color: iconColor,
      }}><Ico size={13}/></div>
      <div style={{ flex: 1, minWidth: 0 }}>
        <div style={{ fontSize: 12, fontWeight: selected ? 600 : 500, color: t.ink, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis', display: 'flex', alignItems: 'center', gap: 5 }}>
          {label}
          {shared && <Icons.share size={9} style={{ color: t.accent }}/>}
        </div>
        <div style={{ fontSize: 9.5, color: t.ink4, fontFamily: t.mono, marginTop: 1 }}>{sub}</div>
      </div>
      {hasChildren && <Icons.chevRight size={11} style={{ color: t.ink4 }}/>}
    </div>
  );
}

function PathChip({ t, icon, active, children }) {
  const Ico = icon;
  return (
    <div style={{
      display: 'flex', alignItems: 'center', gap: 5,
      padding: '5px 10px', borderRadius: 6,
      background: active ? t.chipBg : 'transparent',
      color: active ? t.chipInk : t.ink3,
      fontSize: 12, fontWeight: active ? 600 : 500,
      cursor: 'pointer',
    }}>
      {Ico && <Ico size={12}/>}
      {children}
    </div>
  );
}

function PathSep({ t }) {
  return <Icons.chevRight size={11} style={{ color: t.ink4 }}/>;
}

function Thumb({ t, title, sub, gradient, selected }) {
  return (
    <div style={{
      borderRadius: 10, overflow: 'hidden', cursor: 'pointer',
      border: selected ? `2px solid ${t.accent}` : `1px solid ${t.border}`,
      boxShadow: selected ? '0 4px 14px rgba(255,91,46,.25)' : 'none',
    }}>
      <div style={{
        aspectRatio: '16 / 10', background: gradient, position: 'relative',
      }}>
        <div style={{
          position: 'absolute', inset: 0, display: 'flex', alignItems: 'center', justifyContent: 'center',
        }}>
          <div style={{
            width: 24, height: 24, borderRadius: '50%', background: 'rgba(255,255,255,.85)',
            display: 'flex', alignItems: 'center', justifyContent: 'center',
          }}>
            <div style={{
              width: 0, height: 0, marginLeft: 2,
              borderLeft: '7px solid #0E0E12',
              borderTop: '4.5px solid transparent',
              borderBottom: '4.5px solid transparent',
            }}/>
          </div>
        </div>
        <div style={{
          position: 'absolute', top: 6, right: 6, fontSize: 8,
          padding: '2px 5px', background: 'rgba(0,0,0,.5)', color: '#fff',
          borderRadius: 3, fontFamily: t.mono, letterSpacing: '.04em',
        }}>4K</div>
      </div>
      <div style={{ padding: '7px 9px', background: t.panel }}>
        <div style={{ fontSize: 11.5, fontWeight: 600, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis', color: t.ink }}>{title}</div>
        <div style={{ fontSize: 9.5, color: t.ink4, fontFamily: t.mono, marginTop: 1 }}>{sub}</div>
      </div>
    </div>
  );
}

function Tag({ t, children }) {
  return (
    <span style={{
      fontSize: 10.5, fontWeight: 600, padding: '3px 9px',
      background: t.chipBg, color: t.chipInk, borderRadius: 999,
      letterSpacing: '.02em',
    }}>{children}</span>
  );
}

function MetaCard({ t, label, value }) {
  return (
    <div style={{ padding: '10px 12px', background: t.bg, borderRadius: 10, border: `1px solid ${t.border}` }}>
      <div style={{ fontSize: 9.5, fontFamily: t.mono, color: t.ink4, letterSpacing: '.14em', textTransform: 'uppercase', fontWeight: 600 }}>{label}</div>
      <div style={{ fontSize: 13, fontWeight: 600, marginTop: 3, color: t.ink, fontFamily: t.mono }}>{value}</div>
    </div>
  );
}

function ActLine({ t, dot, text, time }) {
  return (
    <div style={{ display: 'flex', alignItems: 'flex-start', gap: 10, padding: '7px 0' }}>
      <div style={{ width: 6, height: 6, borderRadius: '50%', background: dot, marginTop: 5, flexShrink: 0 }}/>
      <div style={{ flex: 1 }}>
        <div style={{ fontSize: 12, color: t.ink2, fontWeight: 500 }}>{text}</div>
        <div style={{ fontSize: 10, color: t.ink4, fontFamily: t.mono, marginTop: 1 }}>{time}</div>
      </div>
    </div>
  );
}

Object.assign(window, { AuroraFilesV2 });
