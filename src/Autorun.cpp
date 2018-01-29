#include "Autorun.h"

#define APPLICATION_NAME "TimeCamp"

void Autorun::enableAutorun() {
#ifdef _WIN32
    QSettings settings("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    settings.setValue(APPLICATION_NAME, QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
    settings.sync();
#endif
}

void Autorun::disableAutorun() {
#ifdef _WIN32
    QSettings settings("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    settings.remove(APPLICATION_NAME);
    settings.sync();
#endif
}