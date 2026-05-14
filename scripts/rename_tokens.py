#!/usr/bin/env python3
"""One-shot token rename: FsNext/fsnext/FSNEXT -> FsNext/fsnext/FSNEXT.

Walks the FsNext project tree and rewrites text files in-place. Case-sensitive.
Skips build output and git dirs. Safe to re-run: after tokens are replaced,
subsequent runs are a no-op.

Run from inside FsNext/:  python scripts/rename_tokens.py
"""
from pathlib import Path
import sys

TEXT_EXTS = {
    ".cpp", ".h", ".hpp", ".cc", ".cxx",
    ".qml", ".qmldir", ".js",
    ".md", ".txt", ".json", ".yml", ".yaml",
    ".bat", ".cmd", ".ps1", ".sh",
    ".py",
    ".cmake",
    ".svg",
}
# Also match these specific filenames (no extension)
TEXT_NAMES = {"CMakeLists.txt", "qmldir", "CMakePresets.json"}

SKIP_DIRS = {"build", "build-debug", "build-production", "output", ".git", "__pycache__"}

# Order matters: longest/most-specific first so we never rewrite the
# already-replaced text. All three are case-sensitive & distinct.
REPLACEMENTS = [
    ("FsNext",  "FsNext"),   # brand / path display
    ("FSNEXT",  "FSNEXT"),   # CMake vars, macros
    ("fsnext",  "fsnext"),   # C++ namespace, log filename
]


def is_text_file(p: Path) -> bool:
    if p.name in TEXT_NAMES:
        return True
    return p.suffix.lower() in TEXT_EXTS


def process(root: Path) -> tuple[int, int]:
    files_changed = 0
    total_replacements = 0
    for p in root.rglob("*"):
        if not p.is_file():
            continue
        if any(part in SKIP_DIRS for part in p.relative_to(root).parts):
            continue
        if not is_text_file(p):
            continue
        try:
            original = p.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue  # binary / mixed-encoding file

        new = original
        file_count = 0
        for old, rep in REPLACEMENTS:
            cnt = new.count(old)
            if cnt:
                new = new.replace(old, rep)
                file_count += cnt

        if file_count:
            p.write_text(new, encoding="utf-8", newline="\n" if "\n" in original and "\r\n" not in original else None)
            # Preserve original line endings by re-reading and comparing
            # The above newline handling is approximate; rely on Git to normalize if needed.
            files_changed += 1
            total_replacements += file_count
            print(f"  {p.relative_to(root)}: {file_count} replacements")

    return files_changed, total_replacements


def main():
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent
    root = root.resolve()
    print(f"Root: {root}")
    files, total = process(root)
    print(f"\nDone: {files} files changed, {total} replacements total.")


if __name__ == "__main__":
    main()
