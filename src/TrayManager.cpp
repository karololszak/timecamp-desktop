#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QDesktopServices>
#include <unordered_map>

#include "Settings.h"
#include "TrayManager.h"
#include "MainWidget.h"
#include "Autorun.h"

TrayManager &TrayManager::instance() {
    static TrayManager _instance;
    return _instance;
}

TrayManager::TrayManager(QObject *parent)
    : QObject(parent) {}

void TrayManager::setupTray(MainWidget *parent) {
    mainWidget = parent;
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(mainWidget, QStringLiteral("No tray"),

                              "We couldn't detect system tray. Please contact us at "
                              CONTACT_EMAIL " "
                              "with information about your Operating System, Desktop Environment you use (KDE, Gnome, MATE, etc) "
                              "and their respective versions. The more information you can give the better."
                              "\n\n"
                              "If you're using GNOME, try their Shell Extensions: TopIcons or AppIndicator Support.");
    }

// trayIcon is unused on MacOS
#ifndef Q_OS_MACOS
    trayIcon = new QSystemTrayIcon(parent);

    /*
    // Unbind "tray icon activates window"
     https://trello.com/c/qyCrTMfy/39-tray-kliknięcie-niech-zawsze-pokazuje-menu-otwieramy-przez-open-z-tego-menu

    QObject::connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
    */
    trayIcon->setIcon(QIcon(MAIN_ICON));
    trayIcon->show();
#endif

    // setup Tray Menu
    trayMenu = new QMenu(parent);
    createActions(trayMenu);
    assignActions(trayMenu);
    updateStopMenu(false, QString());

// trayIcon is unused on MacOS
#ifndef Q_OS_MACOS
    trayIcon->setContextMenu(trayMenu);
#endif

#ifdef Q_OS_MACOS
    widget = new Widget_M();
    widget->setMenu(trayMenu);
    widget->setIcon(":/Icons/AppIcon_Dark.png");
    widget->setTimerText(""); // at the start there should be no timer text
#endif

// lastly, sync settings
    settings.sync();
}

void TrayManager::setupSettings() {
    qDebug() << "[Tray] Update checkboxes (Settings)";
    // set checkboxes
    autoStartAct->setDisabled(false);
    autoStartAct->setChecked(Autorun::instance().getAutostart()->isAutoStartEnabled());
    trackerAct->setChecked(settings.value(SETT_TRACK_PC_ACTIVITIES, true).toBool());
    autoTrackingAct->setChecked(settings.value(SETT_TRACK_AUTO_SWITCH, false).toBool());
#ifdef _WIDGET_EXISTS_
    widgetAct->setChecked(settings.value(SETT_SHOW_WIDGET, true).toBool());
#endif
    autoStopAct->setChecked(settings.value(SETT_STOP_TIMER_ON_SHUTDOWN, true).toBool());

    // act on the saved settings
    this->autoStart(autoStartAct->isChecked());
    this->tracker(trackerAct->isChecked());
    this->autoTracking(autoTrackingAct->isChecked());
    this->autoStop(autoStopAct->isChecked());
}

void TrayManager::updateRecentTasks() {
    this->assignActions(trayMenu);
}

void TrayManager::updateStopMenu(bool canBeStopped, const QString timerName) {
//    qInfo() << "STOP MENU: " + QString::number(canBeStopped) + " (" + timerName + ")";
    QString stoppableTask = timerName;
    if (!canBeStopped) {
        stoppableTask = TrayManager::STOP_TIMER;
    } else {
        QFont x = QFont();
        QFontMetrics metrics(x);
        stoppableTask = QString(TrayManager::STOP_PREFIX) + metrics.elidedText(timerName, Qt::ElideRight, menuWidth);
    }
    stopTaskAct->setText(stoppableTask);
    stopTaskAct->setEnabled(canBeStopped);
}

void TrayManager::autoStart(bool checked) {
    Autorun::instance().getAutostart()->setAutoStartEnabled(checked);
}

void TrayManager::tracker(bool checked) {
    settings.setValue(SETT_TRACK_PC_ACTIVITIES, checked);
    emit pcActivitiesValueChanged(checked);
}

void TrayManager::autoTracking(bool checked) {
    settings.setValue(SETT_TRACK_AUTO_SWITCH, checked);
    settings.sync();
}

void TrayManager::autoStop(bool checked) {
    settings.setValue(SETT_STOP_TIMER_ON_SHUTDOWN, checked);
    settings.sync();
}

#ifdef _WIDGET_EXISTS_

void TrayManager::widgetToggl(bool checked) {
    settings.setValue(SETT_SHOW_WIDGET, checked);
    bool widgetIsHidden = widget->isHidden();
    if (checked && widgetIsHidden) {
        widget->showMe();
    }
    if (!checked && !widgetIsHidden) {
        widget->hideMe();
    }
}

#endif

void TrayManager::iconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason != QSystemTrayIcon::Context) {
        mainWidget->open();
    }
}

void TrayManager::openCloseWindowAction() {
    mainWidget->open();
}

void TrayManager::contactSupport() {
    QUrl mail(CONTACT_SUPPORT_URL);
    QDesktopServices::openUrl(mail);
};

void TrayManager::createActions(QMenu *menu) {
    openAct = new QAction(tr("Show"), this);
    openAct->setStatusTip(tr("Opens TimeCamp interface"));
    openAct->setShortcut(QKeySequence(KB_SHORTCUTS_OPEN_WINDOW));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    openAct->setShortcutVisibleInContextMenu(true);
#endif
//    QObject::connect(openAct, &QAction::triggered, mainWidget, &MainWidget::open);
    QObject::connect(openAct, &QAction::triggered, this, &TrayManager::openCloseWindowAction);

    recentTasksTitleAct = new QAction(tr("Start recent task: "), this);
    recentTasksTitleAct->setEnabled(false);

    startTaskAct = new QAction(tr("Start timer"), this);
    startTaskAct->setStatusTip(tr("Go to task selection screen"));
    startTaskAct->setShortcut(QKeySequence(KB_SHORTCUTS_START_TIMER));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    startTaskAct->setShortcutVisibleInContextMenu(true);
#endif
    startTaskAct->setShortcutContext(Qt::ApplicationShortcut);
    QObject::connect(startTaskAct, &QAction::triggered, this, [this]()
    {
        qDebug() << "Start Timer clicked";
        emit startTaskClicked();
    });

    stopTaskAct = new QAction(tr(TrayManager::STOP_TIMER), this);
    stopTaskAct->setStatusTip(tr("Stop currently running timer"));
    stopTaskAct->setShortcut(QKeySequence(KB_SHORTCUTS_STOP_TIMER));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    stopTaskAct->setShortcutVisibleInContextMenu(true);
#endif
    stopTaskAct->setShortcutContext(Qt::ApplicationShortcut);
    QObject::connect(stopTaskAct, &QAction::triggered, this, [this]()
    {
        qDebug() << "Stop Timer clicked";
        emit stopTaskClicked();
    });

    trackerAct = new QAction(tr("Track computer activities"), this);
    trackerAct->setCheckable(true);
    QObject::connect(trackerAct, &QAction::triggered, this, &TrayManager::tracker);

    autoTrackingAct = new QAction(tr("Automatic task switching"), this);
    autoTrackingAct->setCheckable(true);
    QObject::connect(autoTrackingAct, &QAction::triggered, this, &TrayManager::autoTracking);

#ifdef _WIDGET_EXISTS_
    widgetAct = new QAction(tr("Time widget"), this);
    widgetAct->setCheckable(true);
    QObject::connect(widgetAct, &QAction::triggered, this, &TrayManager::widgetToggl);
#endif

    autoStartAct = new QAction(tr("Start with computer"), this);
    autoStartAct->setDisabled(true); // disable by default, till we login
    autoStartAct->setCheckable(true);
    QObject::connect(autoStartAct, &QAction::triggered, this, &TrayManager::autoStart);

    QObject::connect(Autorun::instance().getAutostart(), &QAutoStart::autoStartEnabledChanged,
                     autoStartAct, &QAction::setChecked);

    autoStopAct = new QAction(tr("Stop timer when quitting the app"), this);
    autoStopAct->setCheckable(true);

    helpAct = new QAction(tr("Help && support"), this);
    helpAct->setStatusTip(tr("Need help? Talk to one of our support gurus"));
    QObject::connect(helpAct, &QAction::triggered, this, &TrayManager::contactSupport);

    quitAct = new QAction(tr("Quit"), this);
    quitAct->setStatusTip(tr("Close the app"));
    QObject::connect(quitAct, &QAction::triggered, mainWidget, &MainWidget::quit);

    QObject::connect(menu, &QMenu::triggered, this, &TrayManager::menuActionHandler);
}

void TrayManager::menuActionHandler(QAction *action) {
    bool wasOK;
    int taskID = action->data().toInt(&wasOK);
    if (wasOK) {
        emit taskSelected(taskID, true);
    }
    qInfo() << action->text() + " - was clicked";
}

void TrayManager::assignActions(QMenu *menu) {
    QMenu *tempMenu = new QMenu(mainWidget);

    tempMenu->clear();

    tempMenu->addAction(openAct);
    tempMenu->addSeparator();

    QFont x = QFont();
    QFontMetrics metrix(x);
    int width = 150; // pixels

    if (!mainWidget->LastTasks.empty()) {
        tempMenu->addAction(recentTasksTitleAct);
    }

    // add LastTasks
    QHash<QString, int>::iterator i;
    for (i = mainWidget->LastTasks.begin(); i != mainWidget->LastTasks.end(); ++i) {
        QAction *temp;
        temp = new QAction(metrix.elidedText(i.key(), Qt::TextElideMode::ElideRight, width), this);
        temp->setData(i.value());
        tempMenu->addAction(temp);
//        qDebug() << "ADDED key: " << i.key() << ", value: " << i.value();
    }

    if (!mainWidget->LastTasks.empty()) {
        tempMenu->addSeparator();
    }

    tempMenu->addAction(startTaskAct);
    tempMenu->addAction(stopTaskAct);
    tempMenu->addSeparator();
    tempMenu->addAction(trackerAct);
    tempMenu->addAction(autoTrackingAct);
    tempMenu->addAction(autoStartAct);
#ifdef _WIDGET_EXISTS_
    tempMenu->addAction(widgetAct);
#endif
    tempMenu->addAction(autoStopAct);
    tempMenu->addSeparator();
    tempMenu->addAction(helpAct);
    tempMenu->addSeparator();
    tempMenu->addAction(quitAct);

    // replace only if menu should change
    if (!areMenusEqual(tempMenu, menu)) {
        qDebug() << "Tray Menu Update";
        trayMenu = tempMenu;
    }
}

bool TrayManager::areMenusEqual(QMenu *menu1, QMenu *menu2) {
    if (menu1->actions().length() != menu2->actions().length()) {
        return false; // different lengths
    }
    for (int i = 0; i < menu1->actions().length(); i++) {
        if (QString::compare(menu1->actions().at(i)->text(), menu2->actions().at(i)->text()) != 0) {
            return false; // strings are not equal
        }
        if (menu1->actions().at(i)->isCheckable() == menu2->actions().at(i)->isCheckable()
            && menu1->actions().at(i)->isChecked() != menu2->actions().at(i)->isChecked()) {
            return false; // it's a checkbox, but has different state
        }
    }
    return true; // we checked everything and never failed, so they're equal
}

void TrayManager::updateWidget(bool loggedIn) {
#ifdef _WIDGET_EXISTS_
    if (!loggedIn && widget != nullptr) {
        widget->hideMe();
    }
    widgetAct->setChecked(loggedIn);
    widgetAct->setEnabled(loggedIn);
    this->widgetToggl(widgetAct->isChecked());
#endif
}

void TrayManager::updateTooltip(QString tooltipText) {
#ifndef Q_OS_MACOS
    trayIcon->setToolTip(tooltipText); // we don't use trayIcon on macOS
#endif
}

void TrayManager::loginLogout(bool isLoggedIn) {
//    qDebug() << "[Browser] Page changed; update whether logged in or not";

    this->updateWidget(isLoggedIn);
    startTaskAct->setEnabled(isLoggedIn);
    trackerAct->setEnabled(isLoggedIn);
    autoTrackingAct->setEnabled(isLoggedIn);

    if (isLoggedIn) {
        if (!wasLoggedIn) {
            // if wasn't logged in, but is now:
            this->setupSettings(); // make context menu settings, again (after we logged in)
        }
    } else {
        emit pcActivitiesValueChanged(false); // don't track PC activities when not logged in, despite the setting
    }
    wasLoggedIn = isLoggedIn;
    this->assignActions(trayMenu);
}

void TrayManager::setWidget(Widget *widget) {
    TrayManager::widget = widget;
    widget->setMenu(trayMenu);
}


void TrayManager::onAppClose()
{
    if (autoStopAct->isChecked()) {
        emit stopTaskClicked();
    }
}
