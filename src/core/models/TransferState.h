#pragma once
#include <QObject>

namespace fsnext {

Q_NAMESPACE

enum class TransferState {
    Queued,
    Active,
    Paused,
    Complete,
    Error,
    Cancelled
};
Q_ENUM_NS(TransferState)

enum class TransferType {
    Download,
    Upload
};
Q_ENUM_NS(TransferType)

} // namespace fsnext
