# Decision 001: Architecture Pattern

**Date**: 2026-04-14
**Status**: Accepted

## Context

The current Fshare Tool uses a loosely structured MVC pattern with Qt Widgets, global state, and a monolithic ActionThread. The new version (FsNext) needs a cleaner architecture for pure QML UI.

## Options Considered

### Option A: MVC (Model-View-Controller)
- Familiar Qt pattern
- Controllers handle UI events
- Works well with Widgets, less natural with QML
- **Rejected**: QML's declarative binding doesn't align well with imperative controllers

### Option B: MVVM (Model-View-ViewModel)
- Natural fit for QML's declarative data binding
- ViewModel exposes Q_PROPERTY → QML binds directly
- Clean separation: View (QML) ↔ ViewModel (C++) ↔ Model (C++)
- Qt documentation recommends this for QML apps
- **Accepted**

### Option C: Flux/Redux (Unidirectional)
- Single state store, reducers, actions
- Overkill for desktop app with clear page boundaries
- No mature Qt/QML implementation
- **Rejected**: Unnecessary complexity

## Decision

**MVVM with Clean Architecture layers (Service + Repository pattern).**

- ViewModel per page (not per component)
- Services contain business logic
- Repositories abstract data sources
- AppContext provides dependency injection

## Consequences

- More files than current codebase (explicit layers)
- Testability improves (services can be unit-tested without UI)
- QML files stay pure declarative (no JS business logic)
- Clear boundaries prevent the global-state sprawl seen in current codebase
