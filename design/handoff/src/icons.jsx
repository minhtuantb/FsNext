// icons.jsx — Lucide-style stroke icons, sized via size prop

const Icon = ({ d, size = 16, stroke = 'currentColor', sw = 1.75, fill = 'none', style, ...p }) => (
  <svg width={size} height={size} viewBox="0 0 24 24" fill={fill} stroke={stroke}
    strokeWidth={sw} strokeLinecap="round" strokeLinejoin="round" style={style} {...p}>
    {d}
  </svg>
);

const Icons = {
  folder: (p) => <Icon {...p} d={<path d="M3 7a2 2 0 0 1 2-2h3.5l2 2H19a2 2 0 0 1 2 2v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>} />,
  file: (p) => <Icon {...p} d={<><path d="M14 3H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8z"/><path d="M14 3v5h5"/></>} />,
  video: (p) => <Icon {...p} d={<><rect x="3" y="6" width="18" height="12" rx="2"/><path d="m10 10 5 3-5 3z" fill="currentColor"/></>} />,
  image: (p) => <Icon {...p} d={<><rect x="3" y="3" width="18" height="18" rx="2"/><circle cx="9" cy="9" r="2"/><path d="m21 15-5-5L5 21"/></>} />,
  music: (p) => <Icon {...p} d={<><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/></>} />,
  archive: (p) => <Icon {...p} d={<><rect x="3" y="4" width="18" height="4" rx="1"/><path d="M5 8v11a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V8"/><path d="M10 12h4"/></>} />,
  doc: (p) => <Icon {...p} d={<><path d="M14 3H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8z"/><path d="M14 3v5h5M9 13h6M9 17h4"/></>} />,
  upload: (p) => <Icon {...p} d={<><path d="M12 3v14"/><path d="m6 9 6-6 6 6"/><path d="M4 21h16"/></>} />,
  download: (p) => <Icon {...p} d={<><path d="M12 3v14"/><path d="m6 13 6 6 6-6"/><path d="M4 21h16"/></>} />,
  cloud: (p) => <Icon {...p} d={<path d="M17.5 19a4.5 4.5 0 1 0-1.5-8.74 6 6 0 0 0-11.7 1.52A4 4 0 0 0 5 19z"/>} />,
  sync: (p) => <Icon {...p} d={<><path d="M21 12a9 9 0 0 0-15.5-6.3L3 8"/><path d="M3 3v5h5"/><path d="M3 12a9 9 0 0 0 15.5 6.3L21 16"/><path d="M21 21v-5h-5"/></>} />,
  share: (p) => <Icon {...p} d={<><circle cx="18" cy="5" r="3"/><circle cx="6" cy="12" r="3"/><circle cx="18" cy="19" r="3"/><path d="m8.6 13.5 6.8 4M15.4 6.5l-6.8 4"/></>} />,
  link: (p) => <Icon {...p} d={<><path d="M10 13a5 5 0 0 0 7.5.5l3-3a5 5 0 0 0-7-7l-1.5 1.5"/><path d="M14 11a5 5 0 0 0-7.5-.5l-3 3a5 5 0 0 0 7 7l1.5-1.5"/></>} />,
  search: (p) => <Icon {...p} d={<><circle cx="11" cy="11" r="7"/><path d="m21 21-4.3-4.3"/></>} />,
  filter: (p) => <Icon {...p} d={<path d="M3 4h18l-7 9v7l-4-2v-5z"/>} />,
  star: (p) => <Icon {...p} d={<path d="m12 3 2.7 6.2L21 10l-4.8 4.3L17.5 21 12 17.5 6.5 21l1.3-6.7L3 10l6.3-.8z"/>} />,
  trash: (p) => <Icon {...p} d={<><path d="M3 6h18M8 6V4a1 1 0 0 1 1-1h6a1 1 0 0 1 1 1v2"/><path d="M19 6v14a1 1 0 0 1-1 1H6a1 1 0 0 1-1-1V6"/><path d="M10 11v6M14 11v6"/></>} />,
  external: (p) => <Icon {...p} d={<><path d="M15 3h6v6"/><path d="M10 14 21 3"/><path d="M19 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V7a2 2 0 0 1 2-2h6"/></>} />,
  home: (p) => <Icon {...p} d={<><path d="m3 10 9-7 9 7v10a2 2 0 0 1-2 2h-4v-7h-6v7H5a2 2 0 0 1-2-2z"/></>} />,
  clock: (p) => <Icon {...p} d={<><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/></>} />,
  users: (p) => <Icon {...p} d={<><path d="M17 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2"/><circle cx="9" cy="7" r="4"/><path d="M23 21v-2a4 4 0 0 0-3-3.87M16 3.13a4 4 0 0 1 0 7.75"/></>} />,
  settings: (p) => <Icon {...p} d={<><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09a1.65 1.65 0 0 0-1-1.51 1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09a1.65 1.65 0 0 0 1.51-1 1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"/></>} />,
  bell: (p) => <Icon {...p} d={<><path d="M6 8a6 6 0 0 1 12 0c0 7 3 9 3 9H3s3-2 3-9M10.3 21a1.94 1.94 0 0 0 3.4 0"/></>} />,
  plus: (p) => <Icon {...p} d={<><path d="M12 5v14M5 12h14"/></>} />,
  check: (p) => <Icon {...p} d={<path d="m5 12 5 5L20 7"/>} />,
  x: (p) => <Icon {...p} d={<><path d="M18 6 6 18M6 6l12 12"/></>} />,
  chevRight: (p) => <Icon {...p} d={<path d="m9 6 6 6-6 6"/>} />,
  chevDown: (p) => <Icon {...p} d={<path d="m6 9 6 6 6-6"/>} />,
  more: (p) => <Icon {...p} d={<><circle cx="12" cy="12" r="1" fill="currentColor"/><circle cx="5" cy="12" r="1" fill="currentColor"/><circle cx="19" cy="12" r="1" fill="currentColor"/></>} />,
  play: (p) => <Icon {...p} d={<path d="M6 4v16l14-8z" fill="currentColor"/>} />,
  pause: (p) => <Icon {...p} d={<><rect x="6" y="4" width="4" height="16" fill="currentColor"/><rect x="14" y="4" width="4" height="16" fill="currentColor"/></>} />,
  zap: (p) => <Icon {...p} d={<path d="M13 2 3 14h7l-1 8 10-12h-7z"/>} />,
  shield: (p) => <Icon {...p} d={<path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/>} />,
  lock: (p) => <Icon {...p} d={<><rect x="4" y="11" width="16" height="10" rx="2"/><path d="M8 11V7a4 4 0 0 1 8 0v4"/></>} />,
  eye: (p) => <Icon {...p} d={<><path d="M2 12s4-8 10-8 10 8 10 8-4 8-10 8-10-8-10-8z"/><circle cx="12" cy="12" r="3"/></>} />,
  copy: (p) => <Icon {...p} d={<><rect x="9" y="9" width="12" height="12" rx="2"/><path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"/></>} />,
  grid: (p) => <Icon {...p} d={<><rect x="3" y="3" width="7" height="7" rx="1"/><rect x="14" y="3" width="7" height="7" rx="1"/><rect x="3" y="14" width="7" height="7" rx="1"/><rect x="14" y="14" width="7" height="7" rx="1"/></>} />,
  list: (p) => <Icon {...p} d={<><path d="M8 6h13M8 12h13M8 18h13M3 6h.01M3 12h.01M3 18h.01"/></>} />,
  menu: (p) => <Icon {...p} d={<path d="M3 6h18M3 12h18M3 18h18"/>} />,
  sidebar: (p) => <Icon {...p} d={<><rect x="3" y="4" width="18" height="16" rx="2"/><path d="M9 4v16"/></>} />,
  crown: (p) => <Icon {...p} d={<path d="m3 8 4 4 5-8 5 8 4-4-2 13H5z"/>} />,
  arrowUp: (p) => <Icon {...p} d={<><path d="M12 19V5M5 12l7-7 7 7"/></>} />,
  arrowDown: (p) => <Icon {...p} d={<><path d="M12 5v14M19 12l-7 7-7-7"/></>} />,
  minWin: (p) => <Icon {...p} size={10} d={<path d="M2 6h8" sw={1}/>} />,
  maxWin: (p) => <Icon {...p} size={10} d={<rect x="2" y="2" width="8" height="8" rx=".5" fill="none" sw={1}/>} />,
  closeWin: (p) => <Icon {...p} size={10} d={<path d="m2 2 8 8M10 2l-8 8" sw={1}/>} />,
};

window.Icon = Icon;
window.Icons = Icons;
