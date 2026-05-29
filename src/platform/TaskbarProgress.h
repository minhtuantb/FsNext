#pragma once

// TaskbarProgress — native Windows taskbar progress bar on the app's taskbar
// button, via the COM ITaskbarList3 API (shobjidl).
//
// Why raw COM instead of Qt?  Qt 6 dropped QtWinExtras (which had
// QWinTaskbarButton/QWinTaskbarProgress in Qt 5).  There is no Qt 6
// replacement, so we talk to ITaskbarList3 directly.  This is the same API
// Explorer file-copy, browsers, and qBittorrent use.
//
// Lifetime / threading: lives on the GUI thread (main.cpp owns it).  COM is
// already initialised by QApplication on Windows (apartment-threaded), so we
// only CoCreateInstance the taskbar object.  All methods are safe no-ops when:
//   • not on Windows,
//   • COM object failed to create (older Windows / locked-down env),
//   • the HWND hasn't been set yet.
//
// Usage (main.cpp):
//   TaskbarProgress tp;
//   tp.setWindow(quickWindow->winId());          // after window shown
//   tp.setState(TaskbarProgress::Normal);
//   tp.setProgress(0.42);                          // 0.0 .. 1.0

#include <QObject>
#include <QtGlobal>
#include <qwindowdefs.h>   // WId typedef (QtGui)

#ifdef Q_OS_WIN
struct ITaskbarList3;   // forward-declare; full def only in the .cpp
#endif

namespace fsnext {

class TaskbarProgress : public QObject
{
    Q_OBJECT
public:
    enum State {
        NoProgress,   // hide the bar (TBPF_NOPROGRESS)
        Normal,       // green (TBPF_NORMAL)
        Error,        // red   (TBPF_ERROR)
        Paused        // yellow (TBPF_PAUSED)
    };

    explicit TaskbarProgress(QObject *parent = nullptr);
    ~TaskbarProgress() override;

    // Bind to the main window's native handle.  Call once the QQuickWindow
    // is shown (winId() is stable then).  Passing a new handle re-targets.
    void setWindow(WId winId);

    // ratio: 0.0 .. 1.0.  Values outside are clamped.  Negative (idle) is
    // treated as NoProgress by setState; setProgress alone won't switch
    // state — call setState(Normal) first for the bar to appear.
    void setProgress(double ratio);

    // Switch the bar's visual state. NoProgress hides it entirely.
    void setState(State state);

private:
    bool ensureInterface();   // lazy CoCreateInstance; returns m_taskbar != null

#ifdef Q_OS_WIN
    ITaskbarList3 *m_taskbar = nullptr;
    void          *m_hwnd    = nullptr;   // HWND, stored as void* to keep header clean
    bool           m_initFailed = false;  // don't retry CoCreateInstance forever
#endif
    State m_state = NoProgress;
};

} // namespace fsnext
