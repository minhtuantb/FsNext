# Aurora fonts

Download fonts here by running, from the project root:

```powershell
pwsh scripts/fetch-aurora-fonts.ps1
```

or

```bash
bash scripts/fetch-aurora-fonts.sh
```

Files expected after running the script:

- `Geist-Variable.ttf`
- `GeistMono-Variable.ttf`
- `InstrumentSerif-Regular.ttf`
- `InstrumentSerif-Italic.ttf`
- `BeVietnamPro-Regular.ttf` / `Medium` / `SemiBold` / `Bold`

CMake globs `resources/fonts/*.ttf` into the Qt resource bundle, so a rebuild
is enough after the download — no need to edit `resources.qrc` by hand.

If the Aurora variant is not selected (user keeps the legacy design), the
fonts bloat the binary by ~2.5 MB but do no harm. Delete files + re-configure
to drop them.

All fonts are SIL OFL 1.1 licensed. Source: <https://github.com/google/fonts>.
