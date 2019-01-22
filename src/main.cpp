#include <QApplication>
#include <QTimer>
#include <QStandardPaths>
#include <QLibraryInfo>

#ifdef Q_OS_MACOS

#include "Utils_M.h"

#endif

#include "Settings.h"
#include "MainWidget.h"
#include "Comms.h"
#include "DbManager.h"
#include "AutoTracking.h"
#include "TrayManager.h"
#include "TCTimer.h"
#include "DataCollector/WindowEvents.h"
#include "WindowEventsManager.h"
#include "Widget/FloatingWidget.h"
#include "Autorun.h"

#include "qhotkey/QHotkey/qhotkey.h"
#include "qsingleinstance/QSingleInstance/qsingleinstance.h"
#include "third-party/QTLogRotation/logutils.h"

void firstRun()
{
    QSettings settings;

    if (settings.value(SETT_IS_FIRST_RUN, true).toBool()) {
        Autorun::instance().getAutostart()->setAutoStartEnabled(true);

#if defined(Q_OS_LINUX)
        Autorun::instance().addLinuxStartMenuEntry();
#elif defined(Q_OS_MACOS)
        enableAssistiveDevices();
#endif
    }
    settings.setValue(SETT_IS_FIRST_RUN, false);
}

int main(int argc, char *argv[])
{
    // Caches are saved in %localappdata%/org_name/APPLICATION_NAME
    // Eg. C:\Users\timecamp\AppData\Local\TimeCamp SA\TimeCamp Desktop
    // Settings are saved in registry: HKEY_CURRENT_USER\Software\TimeCamp SA\TimeCamp Desktop

    QCoreApplication::setOrganizationName(ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationName(APPLICATION_NAME);
    QCoreApplication::setApplicationVersion(APPLICATION_VERSION);

    // install log handler
    LOGUTILS::initLogging();

    // prevent our app from closing
    QGuiApplication::setQuitOnLastWindowClosed(false);

    // Enable high dpi support
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    // Set up an OpenGL Context that can be shared between threads
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    // standard Qt init
    QApplication app(argc, argv);
    QSingleInstance instance;

    if (instance.process()) {
        if (!instance.isMaster()) {
            return 0;
        }
    } else {
        return -1;
    }

    qInfo() << QString(APPLICATION_NAME + QStringLiteral(" ") + APPLICATION_VERSION + ", " + QSysInfo::prettyProductName() + ", " + QSysInfo::currentCpuArchitecture());
    qDebug() << "build ABI: " << QSysInfo::buildAbi();
    qInfo() << "OpenSSL @ compile:\t" << QSslSocket::sslLibraryBuildVersionNumber() << "\t| "<< QSslSocket::sslLibraryBuildVersionString();
    qInfo() << "OpenSSL @ runtime:\t"<< QSslSocket::sslLibraryVersionNumber() << "\t| " << QSslSocket::sslLibraryVersionString();

    qDebug() << "Libraries: ";
    // debugging library locations (most useful for Linux debugging)
    for(int i = 0; i < QLibraryInfo::TestsPath; i++) { // TestsPath is the last location in the enum
        qDebug() << "\tLocation " << i << QLibraryInfo::location(QLibraryInfo::LibraryLocation(i));
    }
    qDebug() << "App Location: " << QCoreApplication::applicationDirPath();
    qDebug() << "qt.conf: " << QDir(QCoreApplication::applicationDirPath()).exists(QStringLiteral("qt.conf"));
    qDebug() << "$PATH: " << qgetenv("PATH");
#ifdef Q_OS_LINUX
    qInfo() << "$APPIMAGE: " << qgetenv("APPIMAGE");
    qInfo() << "$APPDIR: " << qgetenv("APPDIR");
#endif

    // check if it's a first run, and i.e. on Mac ask for permissions
    firstRun();

    // set the app icon
    QIcon appIcon = QIcon(MAIN_ICON);
    appIcon.addFile(QStringLiteral(":/Icons/AppIcon_256.png"), QSize(256, 256));
    appIcon.addFile(QStringLiteral(":/Icons/AppIcon_128.png"), QSize(128, 128));
    appIcon.addFile(QStringLiteral(":/Icons/AppIcon_64.png"), QSize(64, 64));
    appIcon.addFile(QStringLiteral(":/Icons/AppIcon_48.png"), QSize(48, 48));
    appIcon.addFile(QStringLiteral(":/Icons/AppIcon_32.png"), QSize(32, 32));
    appIcon.addFile(QStringLiteral(":/Icons/AppIcon_16.png"), QSize(16, 16));
    // all of these: see "TimeCampDesktop.qrc"
    // order matters! :( paths are of the qrc standard,
    // would be pointless to store them all in another file
    // but we can't just include ":/Icons/*.png"

    QApplication::setWindowIcon(appIcon);

    // create DB Manager instance early, as it needs some time to prepare queries etc
    DbManager *dbManager = &DbManager::instance();
    AutoTracking *autoTracking = &AutoTracking::instance();

    // create events manager
    WindowEventsManager *windowEventsManager = &WindowEventsManager::instance();

    // create main widget
    MainWidget mainWidget;
    mainWidget.setWindowIcon(appIcon);

    // create tray manager
    TrayManager *trayManager = &TrayManager::instance();
    trayManager->setupTray(&mainWidget); // connect the mainWidget to tray

    QObject::connect(&mainWidget, &MainWidget::pageStatusChanged, trayManager, &TrayManager::loginLogout);
    QObject::connect(&mainWidget, &MainWidget::pageStatusChanged, dbManager, &DbManager::loginLogout);
    QObject::connect(&mainWidget, &MainWidget::lastTasksChanged, trayManager, &TrayManager::updateRecentTasks);
    QObject::connect(trayManager, &TrayManager::pcActivitiesValueChanged, windowEventsManager, &WindowEventsManager::startOrStopThread);

    // send updates from DB to server
    Comms *comms = &Comms::instance();
    auto *syncDBtimer = new QTimer();
    QObject::connect(syncDBtimer, &QTimer::timeout, comms, &Comms::timedUpdates);

    // Away time bindings
    QObject::connect(windowEventsManager, &WindowEventsManager::updateAfterAwayTime, comms, &Comms::timedUpdates);
    QObject::connect(windowEventsManager, &WindowEventsManager::openAwayTimeManagement, &mainWidget, &MainWidget::goToAwayPage);

    // Stopped logging bind
    QObject::connect(windowEventsManager, &WindowEventsManager::dataCollectingStopped, comms, &Comms::clearLastApp);

    // change apps in autotracking ASAP
    QObject::connect(comms, &Comms::announceAppChange, autoTracking, &AutoTracking::checkAppKeywords);

    // Save apps to sqlite on signal-slot basis
    QObject::connect(comms, &Comms::DbSaveApp, dbManager, &DbManager::saveAppToDb);

    // 2 sec timer for updating submenu and other features
    auto *webpageDataUpdateTimer = new QTimer();
    QObject::connect(webpageDataUpdateTimer, &QTimer::timeout, &mainWidget, &MainWidget::webpageDataUpdateOnInterval);
    // above timeout triggers func that emits checkIsIdle when logged in
    QObject::connect(&mainWidget, &MainWidget::checkIsIdle, windowEventsManager->getCaptureEventsThread(), &WindowEvents::checkIdleStatus);

    // sync DB on page change
    QObject::connect(&mainWidget, &MainWidget::pageStatusChanged, [&syncDBtimer](bool loggedIn)
    {
        if (!loggedIn) {
            if(syncDBtimer->isActive()) {
                qInfo("Stopping DB Sync timer");
                syncDBtimer->stop();
            }
        } else {
            if(!syncDBtimer->isActive()) {
                qInfo("Restarting DB Sync timer");
                syncDBtimer->start();
            }
        }
    });

    // the smart widget that floats around
    auto *theWidget = new FloatingWidget(); // FloatingWidget can't be bound to mainwidget (it won't set state=visible when main is hidden)

    // the timer that syncs via API
    auto *TimeCampTimer = new TCTimer(comms);
//    QObject::connect(syncDBtimer, &QTimer::timeout, comms, &Comms::timerStatus); // checking Timer Status on the same interval as DB Sync

    // Hotkeys
    auto hotkeyNewTimer = new QHotkey(QKeySequence(KB_SHORTCUTS_START_TIMER), true, &app);
    QObject::connect(hotkeyNewTimer, &QHotkey::activated, TimeCampTimer, &TCTimer::startTimerSlot);
    QObject::connect(hotkeyNewTimer, &QHotkey::activated, &mainWidget, &MainWidget::chooseTask);

    auto hotkeyStopTimer = new QHotkey(QKeySequence(KB_SHORTCUTS_STOP_TIMER), true, &app);
    QObject::connect(hotkeyStopTimer, &QHotkey::activated, TimeCampTimer, &TCTimer::stopTimerSlot);
//    QObject::connect(hotkeyStopTimer, &QHotkey::activated, &mainWidget, &MainWidget::triggerTimerStatusFetchAsync);

    auto hotkeyOpenWindow = new QHotkey(QKeySequence(KB_SHORTCUTS_OPEN_WINDOW), true, &app);
    QObject::connect(hotkeyOpenWindow, &QHotkey::activated, trayManager, &TrayManager::openCloseWindowAction);

    // Starting Timer
    QObject::connect(autoTracking, &AutoTracking::foundTask, TimeCampTimer, &TCTimer::startTaskByID);
    QObject::connect(trayManager, &TrayManager::taskSelected, TimeCampTimer, &TCTimer::startTaskByID);

    QObject::connect(theWidget, &FloatingWidget::taskNameClicked, TimeCampTimer, &TCTimer::startIfNotRunningYet);
    QObject::connect(theWidget, &FloatingWidget::taskNameClicked, &mainWidget, &MainWidget::chooseTask);

    QObject::connect(theWidget, &FloatingWidget::playButtonClicked, TimeCampTimer, &TCTimer::startTimerSlot);
    QObject::connect(theWidget, &FloatingWidget::playButtonClicked, &mainWidget, &MainWidget::chooseTask);

    QObject::connect(trayManager, &TrayManager::startTaskClicked, TimeCampTimer, &TCTimer::startTimerSlot);
    QObject::connect(trayManager, &TrayManager::startTaskClicked, &mainWidget, &MainWidget::chooseTask);

    // Timer stop
    QObject::connect(theWidget, &FloatingWidget::pauseButtonClicked, TimeCampTimer, &TCTimer::stopTimerSlot);
//    QObject::connect(theWidget, &FloatingWidget::pauseButtonClicked, &mainWidget, &MainWidget::triggerTimerStatusFetchAsync);

    QObject::connect(trayManager, &TrayManager::stopTaskClicked, TimeCampTimer, &TCTimer::stopTimerSlot);
//    QObject::connect(trayManager, &TrayManager::stopTaskClicked, &mainWidget, &MainWidget::triggerTimerStatusFetchAsync);

    // Timer listeners
    QObject::connect(TimeCampTimer, &TCTimer::timerStatusChanged, trayManager, &TrayManager::updateStopMenu);
    QObject::connect(TimeCampTimer, &TCTimer::timerStatusChanged, theWidget, &FloatingWidget::updateWidgetStatus);

    QObject::connect(TimeCampTimer, &TCTimer::timerElapsedSeconds, theWidget, &FloatingWidget::setTimerElapsed);

    QObject::connect(TimeCampTimer, &TCTimer::askForWebTimerUpdate, &mainWidget, &MainWidget::triggerTimerStatusFetchAsync);
    QObject::connect(syncDBtimer, &QTimer::timeout, &mainWidget, &MainWidget::triggerTimerStatusFetchAsync);

    QObject::connect(&mainWidget, &MainWidget::updateTimerStatus, TimeCampTimer, &TCTimer::timerStatusReply);

    trayManager->setWidget(theWidget);
    trayManager->setupSettings();
    comms->timedUpdates(); // fetch userInfo, userSettings, send apps since last update
    mainWidget.init(); // init the WebView

    // now timers
    syncDBtimer->start(ACTIVITIES_SYNC_INTERVAL * 1000); // sync DB every 30s
    webpageDataUpdateTimer->start(WEBPAGE_DATA_SYNC_INTERVAL * 1000);

    QObject::connect(&app, &QApplication::aboutToQuit, windowEventsManager, &WindowEventsManager::stopThread);
    QObject::connect(&app, &QApplication::aboutToQuit, trayManager, &TrayManager::onAppClose);

    return QApplication::exec();
}
