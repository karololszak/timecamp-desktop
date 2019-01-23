#include <QtCore/QStandardPaths>
#include <QtCore/QFile>
#include <QtCore/QSettings>

#include "Autorun.h"
#include "Settings.h"

Autorun &Autorun::instance() {
    static Autorun _instance;
    return _instance;
}

Autorun::Autorun(QObject *parent)
    : QObject(parent), autostart{new QAutoStart{this}}
{
    // app that is autostarted will have this argument
    autostart->setArguments({QStringLiteral("--autostart")});

#ifdef Q_OS_LINUX
    // AppImage can autostart too
    appImagePath = qgetenv("APPIMAGE");
    isAppImage = !appImagePath.isEmpty();

    if (isAppImage) {
        autostart->setProgram(appImagePath);
    }

    addLinuxIcon();
#endif
}

QAutoStart *Autorun::getAutostart() const {
    return autostart;
}

#ifdef Q_OS_LINUX
void Autorun::addLinuxIcon()
{
    iconPath = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation).constFirst() + QStringLiteral("/") + APP_ICON; // determine where we want Linux icon
    QFile::copy(QStringLiteral(":/Icons/AppIcon_128.png"), iconPath); // copy from our "res"
    autostart->setExtraProperty(QAutoStart::IconName, iconPath); // set it to both autostart and startMenuEntry
}

void Autorun::addLinuxStartMenuEntry()
{
    // hack: start menu entries on LINUX are the .desktop files too, so we can re-use the QAutoStart
    startMenuEntry = new QAutoStart(this);

    // update path if it's AppImage
    if (isAppImage) {
        startMenuEntry->setProgram(appImagePath);
    }

    // write to Apps location
    startMenuEntry->setExtraProperty(QAutoStart::CustomLocation, QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation));
    startMenuEntry->setExtraProperty(QAutoStart::Comment, QStringLiteral("Version ") + APPLICATION_VERSION + (isAppImage ? QStringLiteral(" (AppImage)") : QStringLiteral("")));

    // set icon
    startMenuEntry->setExtraProperty(QAutoStart::IconName, iconPath);

    // set some other arg to let app know where it was launched from
    startMenuEntry->setArguments({QStringLiteral("--startmenu")});

    // push the file
    startMenuEntry->setAutoStartEnabled(true);

    QSettings settings;
    settings.setValue(DESKTOP_FILE_PATH, startMenuEntry->desktopFilePath());
    settings.sync();
}

#endif
