#!/usr/bin/env python3
"""Generate the FsNext app icon set.

Sources the design from a single PNG (resources/icons/app.png — 256x256) and
writes out:
  - resources/icons/app-16.png … app-256.png   (per-size PNGs, used by qrc)
  - resources/icons/app.ico                    (multi-resolution Windows ICO)

The .ico is what Explorer / taskbar / Alt-Tab use for the executable itself
(via .rc resource).  The PNGs are bundled into qrc:/icons/ so QIcon at runtime
can pick the closest size for any DPI.

Re-run whenever app.png changes:  python scripts/generate_app_icon.py
"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "resources" / "icons" / "app.png"
SIZES = [16, 24, 32, 48, 64, 128, 256]

if not SRC.exists():
    raise SystemExit(f"Missing source: {SRC}")

base = Image.open(SRC).convert("RGBA")
if base.size != (256, 256):
    base = base.resize((256, 256), Image.LANCZOS)

# Write per-size PNGs (for qrc — Qt picks the right one per DPI/usage).
out_dir = SRC.parent
for s in SIZES:
    img = base.resize((s, s), Image.LANCZOS)
    img.save(out_dir / f"app-{s}.png", optimize=True)
    print(f"  wrote {out_dir / f'app-{s}.png'}  ({s}x{s})")

# Write multi-resolution .ico.  Pillow handles the ICONDIR/ICONDIRENTRY layout
# for us; passing `sizes=` makes it embed each size as a separate image.
ico_path = out_dir / "app.ico"
base.save(ico_path, format="ICO", sizes=[(s, s) for s in SIZES])
print(f"  wrote {ico_path}  (multi-res: {SIZES})")
