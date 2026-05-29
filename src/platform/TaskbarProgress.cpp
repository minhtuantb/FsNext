#include "TaskbarProgress.h"

#include <QDebug>

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <shobjidl.h>   // ITaskbarList3, CLSID_TaskbarList, TBPFLAG
#endif

namespace fsnext {

TaskbarProgress::TaskbarProgress(QObject *parent)
    : QObject(parent)
{
}

TaskbarProgress::~TaskbarProgress()
{
#ifdef Q_OS_WIN
    if (m_taskbar) {
        m_taskbar->Release();
        m_taskbar = nullptr;
    }
#endif
}

bool TaskbarProgress::ensureInterface()
{
#ifdef Q_OS_WIN
    if (m_taskbar) return true;
    if (m_initFailed) return false;
    // QApplication initialises COM (OleInitialize) on the GUI thread, so we
    // don't CoInitialize here.  Create the taskbar list object.
    ITaskbarList3 *ptr = nullptr;
    const HRESULT hr = CoCreateInstance(CLSID_TaskbarList, nullptr,
                                        CLSCTX_INPROC_SERVER,
                                        IID_ITaskbarList3,
                                        reinterpret_cast<void **>(&ptr));
    if (FAILED(hr) || !ptr) {
        m_initFailed = true;
        qInfo() << "[TaskbarProgress] ITaskbarList3 unavailable (hr=" << hr << ")";
        return false;
    }
    if (FAILED(ptr->HrInit())) {
        ptr->Release();
        m_initFailed = true;
        qInfo() << "[TaskbarProgress] HrInit failed";
        return false;
    }
    m_taskbar = ptr;
    return true;
#else
    return false;
#endif
}

void TaskbarProgress::setWindow(WId winId)
{
#ifdef Q_OS_WIN
    m_hwnd = reinterpret_cast<void *>(winId);
    // Re-apply current state to the new window so the bar isn't stale.
    if (m_hwnd && ensureInterface()) {
        setState(m_state);
    }
#else
    Q_UNUSED(winId);
#endif
}

void TaskbarProgress::setProgress(double ratio)
{
#ifdef Q_OS_WIN
    if (!m_hwnd || !ensureInterface()) return;
    if (m_state == NoProgress) return;   // bar hidden — nothing to fill
    if (ratio < 0.0) ratio = 0.0;
    if (ratio > 1.0) ratio = 1.0;
    // Use a 0..1000 integer scale — finer than 0..100, avoids float jitter.
    const ULONGLONG completed = static_cast<ULONGLONG>(ratio * 1000.0 + 0.5);
    m_taskbar->SetProgressValue(reinterpret_cast<HWND>(m_hwnd), completed, 1000ULL);
#else
    Q_UNUSED(ratio);
#endif
}

void TaskbarProgress::setState(State state)
{
    m_state = state;
#ifdef Q_OS_WIN
    if (!m_hwnd || !ensureInterface()) return;
    TBPFLAG flag = TBPF_NOPROGRESS;
    switch (state) {
    case NoProgress: flag = TBPF_NOPROGRESS; break;
    case Normal:     flag = TBPF_NORMAL;     break;
    case Error:      flag = TBPF_ERROR;      break;
    case Paused:     flag = TBPF_PAUSED;     break;
    }
    m_taskbar->SetProgressState(reinterpret_cast<HWND>(m_hwnd), flag);
#endif
}

} // namespace fsnext
