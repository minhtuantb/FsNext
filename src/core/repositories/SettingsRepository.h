#pragma once
#include "core/models/AppSettings.h"
#include <QSettings>
#include <QString>

namespace fsnext {

class SettingsRepository {
public:
    SettingsRepository();
    ~SettingsRepository() = default;

    // High-level load/save of the full settings struct
    AppSettings load();
    void save(const AppSettings &settings);

    // Low-level key/value access
    QString getString(const QString &key, const QString &defaultValue = {}) const;
    void setString(const QString &key, const QString &value);

    int getInt(const QString &key, int defaultValue = 0) const;
    void setInt(const QString &key, int value);

    bool getBool(const QString &key, bool defaultValue = false) const;
    void setBool(const QString &key, bool value);

private:
    QSettings m_settings;
};

} // namespace fsnext
