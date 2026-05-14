# Decision 002: Tech Stack

**Date**: 2026-04-14
**Status**: Accepted

## Context

Selecting technologies for FsNext that balance future-proofing (5+ year lifespan: 2026-2031+) with development velocity and compatibility with the existing Fshare API.

## Decisions

### UI Framework: Qt 6 QML (Pure, No Widgets)

**Why**: 
- Qt 6 is actively maintained with LTS releases through 2030+
- QML provides modern declarative UI with hardware-accelerated rendering
- Existing design system (FshareTheme) already implemented in QML
- QML naturally supports animations, transitions, responsive layout
- Qt Widgets is maintenance-only in Qt 6

**Trade-off**: Learning curve for team members familiar only with Widgets. Mitigated by having complete component library (FsButton, FsCard, etc.).

### HTTP: libcurl (static)

**Why**:
- Battle-tested, widely deployed
- Current codebase already uses it extensively
- Multi-segment download / chunked upload patterns proven
- Static linking avoids DLL deployment issues
- vcpkg provides easy integration

**Alternative considered**: Qt QNetworkAccessManager
- Rejected: less control over low-level CURL options needed for segmented downloads, resume, and chunked uploads. Would require rewriting transfer engine.

### JSON: jsoncpp

**Why**:
- Already used in current codebase
- Single-file library, easy to vendor
- Adequate performance for API response sizes
- No additional dependency

**Alternative considered**: nlohmann/json
- Viable but would add new dependency. jsoncpp already works.

### Build: CMake 3.24+ / Ninja / MSVC 2022

**Why**: Same as current, proven toolchain. Ninja provides fast incremental builds.

### Package Manager: vcpkg

**Why**: Same as current, manages CURL + OpenSSL + zlib cleanly with static linking.

### Icons: Phosphor Icons (SVG)

**Why**: Per design system spec. MIT license, 6 weights, cross-platform, 6000+ icons.

## What Changed from Current Stack

| Area | Current | FsNext | Reason |
|------|---------|--------|--------|
| UI | Qt Widgets + early QML | Pure QML | Full migration |
| History | XML persistence | JSON files | Simpler, standard |
| Credentials | base64 in QSettings | OS keychain (stretch goal) | Security |
| Analytics | Google Analytics | TBD | Evaluate modern alternatives |
| Browser ext | Chrome + Firefox | Chrome only | Firefox addon deprecated |
| Single instance | qtsingleapplication lib | Custom minimal impl | Reduce dependency |
