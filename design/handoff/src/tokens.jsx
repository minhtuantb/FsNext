// tokens.jsx — Design tokens cho 3 variant
// Variant A — Aurora: gradient tinh tế, glass, modern bold
// Variant B — Monolith: typography-led, flat, editorial
// Variant C — Neon Velocity: dark-first, power-user, dense

const TOKENS = {
  // ═══════════════ AURORA ═══════════════
  // "Gradient nhưng thông minh" — bold accents, clean surfaces
  aurora: {
    name: 'Aurora',
    tagline: 'Gradient tinh tế · Glass · Modern bold',
    // surfaces
    bg: '#F5F4F1',            // warm off-white, lệch khỏi pure white
    bgElev: '#FFFFFF',
    sidebar: '#0E0E12',       // near-black sidebar — bold contrast
    sidebarInk: '#EDEDEF',
    sidebarInkMuted: '#8A8A94',
    border: '#E6E3DC',
    borderStrong: '#D4D0C7',

    // ink
    ink: '#0E0E12',
    ink2: '#2A2A30',
    ink3: '#5C5C66',
    ink4: '#8A8A94',

    // accent — signature gradient
    accent: '#FF5B2E',        // vermilion
    accent2: '#FFAF1D',       // mango
    accent3: '#FF3D7F',       // hot pink
    accentGrad: 'linear-gradient(135deg, #FF3D7F 0%, #FF5B2E 50%, #FFAF1D 100%)',
    accentGradSoft: 'linear-gradient(135deg, #FFE8E0 0%, #FFF3DA 100%)',

    // semantic
    success: '#0A8A5C',
    warn: '#C96A00',
    danger: '#D53030',
    info: '#2C6EF2',

    // fonts
    fontSans: "'Be Vietnam Pro', 'Geist', system-ui, sans-serif",
    fontDisplay: "'Instrument Serif', 'Be Vietnam Pro', serif",
    fontMono: "'Geist Mono', ui-monospace, monospace",

    radius: { sm: 6, md: 10, lg: 14, xl: 20, pill: 999 },
    shadow: {
      sm: '0 1px 2px rgba(14,14,18,.06), 0 0 0 1px rgba(14,14,18,.04)',
      md: '0 4px 16px rgba(14,14,18,.08), 0 0 0 1px rgba(14,14,18,.05)',
      lg: '0 20px 60px rgba(14,14,18,.12), 0 0 0 1px rgba(14,14,18,.05)',
    },
  },

  // ═══════════════ MONOLITH ═══════════════
  // Typography-forward, editorial, trust-focused
  monolith: {
    name: 'Monolith',
    tagline: 'Typography-led · Editorial · Calm confidence',
    bg: '#FAFAF7',
    bgElev: '#FFFFFF',
    sidebar: '#FAFAF7',
    sidebarInk: '#1A1A1A',
    sidebarInkMuted: '#6B6B6B',
    border: '#E8E6E0',
    borderStrong: '#CFCDC5',

    ink: '#0A0A0A',
    ink2: '#1A1A1A',
    ink3: '#4A4A4A',
    ink4: '#888884',

    accent: '#1E4D3E',        // pine green — trust, storage
    accent2: '#D4A554',       // ochre gold
    accent3: '#8B4513',
    accentGrad: 'linear-gradient(180deg, #1E4D3E 0%, #0E2F26 100%)',
    accentGradSoft: '#F0EDE3',

    success: '#1E4D3E',
    warn: '#B86B00',
    danger: '#B02E2E',
    info: '#2F5FB8',

    fontSans: "'Be Vietnam Pro', 'Geist', system-ui, sans-serif",
    fontDisplay: "'Instrument Serif', serif",
    fontMono: "'Geist Mono', ui-monospace, monospace",

    radius: { sm: 2, md: 4, lg: 6, xl: 10, pill: 999 },
    shadow: {
      sm: '0 1px 0 rgba(10,10,10,.04), 0 0 0 1px rgba(10,10,10,.05)',
      md: '0 2px 8px rgba(10,10,10,.06), 0 0 0 1px rgba(10,10,10,.06)',
      lg: '0 12px 32px rgba(10,10,10,.1), 0 0 0 1px rgba(10,10,10,.05)',
    },
  },

  // ═══════════════ NEON VELOCITY ═══════════════
  // Dark-first, power-user, dense, gamer-ish (phim, tốc độ cao = neon)
  neon: {
    name: 'Neon Velocity',
    tagline: 'Dark-first · Power user · Dense grid',
    bg: '#0A0A0F',
    bgElev: '#111118',
    bgElev2: '#16161F',
    sidebar: '#07070B',
    sidebarInk: '#E6E6EC',
    sidebarInkMuted: '#6B6B7A',
    border: '#1F1F2B',
    borderStrong: '#2B2B3C',

    ink: '#F5F5FA',
    ink2: '#C9C9D4',
    ink3: '#8E8E9E',
    ink4: '#5A5A6E',

    accent: '#00E5B8',        // signature cyan-mint
    accent2: '#7C5CFF',       // ultraviolet
    accent3: '#FFD43D',
    accentGrad: 'linear-gradient(135deg, #00E5B8 0%, #7C5CFF 100%)',
    accentGradSoft: 'linear-gradient(135deg, rgba(0,229,184,.1) 0%, rgba(124,92,255,.1) 100%)',

    success: '#00E5B8',
    warn: '#FFB020',
    danger: '#FF5470',
    info: '#5AC8FA',

    fontSans: "'Geist', 'Be Vietnam Pro', system-ui, sans-serif",
    fontDisplay: "'Geist', system-ui, sans-serif",
    fontMono: "'Geist Mono', ui-monospace, monospace",

    radius: { sm: 4, md: 6, lg: 10, xl: 14, pill: 999 },
    shadow: {
      sm: '0 1px 2px rgba(0,0,0,.5), inset 0 1px 0 rgba(255,255,255,.04)',
      md: '0 4px 16px rgba(0,0,0,.5), inset 0 1px 0 rgba(255,255,255,.05)',
      lg: '0 24px 60px rgba(0,0,0,.7), 0 0 0 1px rgba(0,229,184,.1)',
    },
  },
};

window.TOKENS = TOKENS;
