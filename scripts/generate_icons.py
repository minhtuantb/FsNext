#!/usr/bin/env python3
"""Generate Phosphor-style SVG icon files from path data.

Writes one .svg per icon into qml/Fshare/Icons/. The SVG uses stroke="#000"
(fill="#000" for filled icons) so MultiEffect.colorization can tint them
at runtime via FsIcon.qml.
"""
from pathlib import Path

# (path_data, filled)
ICONS = {
    # Navigation / arrows
    "arrow-down":    ('M12 4 L12 20 M5 13 L12 20 L19 13', False),
    "arrow-up":      ('M12 20 L12 4 M5 11 L12 4 L19 11', False),
    "arrow-right":   ('M4 12 L20 12 M13 5 L20 12 L13 19', False),
    "arrow-left":    ('M20 12 L4 12 M11 5 L4 12 L11 19', False),
    "chevron-right": ('M9 6 L15 12 L9 18', False),
    "chevron-down":  ('M6 9 L12 15 L18 9', False),
    "chevron-up":    ('M6 15 L12 9 L18 15', False),

    # Actions
    "plus":    ('M12 5 L12 19 M5 12 L19 12', False),
    "minus":   ('M5 12 L19 12', False),
    "x":       ('M6 6 L18 18 M18 6 L6 18', False),
    "check":   ('M5 12 L10 17 L19 7', False),
    "play":    ('M7 5 L19 12 L7 19 Z', True),
    "pause":   ('M9 5 L9 19 M15 5 L15 19', False),
    "refresh": ('M21 12 a9 9 0 1 1 -3.5 -7.1 M21 4 L21 10 L15 10', False),
    "trash":   ('M4 7 L20 7 M9 7 L9 4 L15 4 L15 7 M6 7 L7 20 L17 20 L18 7', False),

    # Transfer (new — requested by design-system docs)
    "download": ('M12 3 L12 15 M6 9 L12 15 L18 9 M3 17 L3 21 L21 21 L21 17', False),
    "upload":   ('M12 15 L12 3 M6 9 L12 3 L18 9 M3 17 L3 21 L21 21 L21 17', False),

    # Domain
    "folder":      ('M3 7 L3 19 a1 1 0 0 0 1 1 L20 20 a1 1 0 0 0 1 -1 L21 8 a1 1 0 0 0 -1 -1 L11 7 L9 4 L4 4 a1 1 0 0 0 -1 1 Z', False),
    "folder-open": ('M3 7 L3 19 a1 1 0 0 0 1 1 L19 20 L22 10 L8 10 L6 13 M3 7 L3 5 a1 1 0 0 0 1 -1 L9 4 L11 7 L20 7', False),
    "file":        ('M6 3 L14 3 L19 8 L19 21 L6 21 Z M14 3 L14 8 L19 8', False),
    "house":       ('M3 12 L12 4 L21 12 M5 10 L5 20 L19 20 L19 10', True),

    # Status / info
    "info":   ('M12 8 L12 8.01 M11 12 L13 12 L13 17 L11 17 M12 3 a9 9 0 1 0 0 18 a9 9 0 1 0 0 -18', False),
    "lock":   ('M6 11 L18 11 L18 20 L6 20 Z M8 11 L8 7 a4 4 0 0 1 8 0 L16 11', False),
    "key":    ('M14 7 a4 4 0 1 1 -3.46 6 L4 19 L4 21 L7 21 L7 19 L9 19 L9 17 L11 17 L11 15 L11 13 L17 7', False),
    "link":   ('M10 13 a5 5 0 0 0 7 0 L20 10 a5 5 0 1 0 -7 -7 L11 5 M14 11 a5 5 0 0 0 -7 0 L4 14 a5 5 0 1 0 7 7 L13 19', False),
    "shield": ('M12 3 L4 6 L4 12 a8 8 0 0 0 8 8 a8 8 0 0 0 8 -8 L20 6 Z', True),

    # UI
    "search":      ('M11 4 a7 7 0 1 1 0 14 a7 7 0 1 1 0 -14 M21 21 L16 16', False),
    "gear":        ('M12 9 a3 3 0 1 0 0 6 a3 3 0 1 0 0 -6 M19.4 15 a1.65 1.65 0 0 0 .33 1.82 L19.79 16.9 a2 2 0 0 1 -2.83 2.83 L16.9 19.7 a1.65 1.65 0 0 0 -1.82 -.33 a1.65 1.65 0 0 0 -1 1.51 L14.08 21 a2 2 0 0 1 -4 0 L10 20.91 a1.65 1.65 0 0 0 -1 -1.51 a1.65 1.65 0 0 0 -1.82 .33 L7.1 19.79 a2 2 0 0 1 -2.83 -2.83 L4.3 16.9 a1.65 1.65 0 0 0 .33 -1.82 a1.65 1.65 0 0 0 -1.51 -1 L3 14.08 a2 2 0 0 1 0 -4 L3.09 10 a1.65 1.65 0 0 0 1.51 -1 a1.65 1.65 0 0 0 -.33 -1.82 L4.21 7.1 a2 2 0 0 1 2.83 -2.83 L7.1 4.3 a1.65 1.65 0 0 0 1.82 .33 a1.65 1.65 0 0 0 1 -1.51 L10 3 a2 2 0 0 1 4 0 L14 3.09 a1.65 1.65 0 0 0 1 1.51 a1.65 1.65 0 0 0 1.82 -.33 L16.9 4.21 a2 2 0 0 1 2.83 2.83 L19.7 7.1 a1.65 1.65 0 0 0 -.33 1.82 a1.65 1.65 0 0 0 1.51 1 L21 9.92 a2 2 0 0 1 0 4 Z', False),
    "user":        ('M16 11 a4 4 0 1 1 -8 0 a4 4 0 1 1 8 0 M4 21 a8 8 0 0 1 16 0', False),
    "user-circle": ('M12 3 a9 9 0 1 0 0 18 a9 9 0 1 0 0 -18 M9 11 a3 3 0 1 0 6 0 a3 3 0 1 0 -6 0 M6 18 a6 6 0 0 1 12 0', False),
    "list":        ('M3 6 L21 6 M3 12 L21 12 M3 18 L21 18', False),
    "power":       ('M18 6 a8 8 0 1 1 -12 0 M12 4 L12 12', False),
    "sparkle":     ('M12 3 L13.5 9 L21 12 L13.5 15 L12 21 L10.5 15 L3 12 L10.5 9 Z', True),
    "menu":        ('M3 6 L21 6 M3 12 L21 12 M3 18 L21 18', False),
}

SVG_TEMPLATE = (
    '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" '
    'fill="{fill}" stroke="#000000" stroke-width="1.6" '
    'stroke-linecap="round" stroke-linejoin="round">\n'
    '  <path d="{path}"/>\n'
    '</svg>\n'
)

def main():
    out_dir = Path(__file__).resolve().parent.parent / "qml" / "Fshare" / "Icons"
    out_dir.mkdir(parents=True, exist_ok=True)

    for name, (path, filled) in ICONS.items():
        fill = "#000000" if filled else "none"
        svg = SVG_TEMPLATE.format(fill=fill, path=path)
        (out_dir / f"{name}.svg").write_text(svg, encoding="utf-8")

    print(f"Wrote {len(ICONS)} SVG files to {out_dir}")

if __name__ == "__main__":
    main()
