#ifndef THEGUI_TRAYMANAGER_H
#define THEGUI_TRAYMANAGER_H

#include <QMenu>
#include <QAction>
#include <QShortcut>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QSettings>
#include <QSignalMapper>

#include "Widget/Widget.h"

#ifdef Q_OS_MACOS

#include "Widget/Widget_M.h"
#define _WIDGET_EXISTS_

#else

#include "Widget/FloatingWidget.h"
#define _WIDGET_EXISTS_

#endif

class MainWidget;

class TrayManager : public QObject
{
Q_OBJECT
Q_DISABLE_COPY(TrayManager)

public:
    static const constexpr char *STOP_TIMER = "Stop timer";
    static const constexpr char *STOP_PREFIX = "Stop ";

    static TrayManager &instance();
    ~TrayManager() override = default;

    void setupTray(MainWidget *);
    void setupSettings();

    void updateWidget(bool);
    void updateStopMenu(bool, QString);
    void loginLogout(bool);
    void updateTooltip(QString tooltipText);

    bool wasLoggedIn = false;
    QMenu *getTrayMenu() const;

signals:
    void pcActivitiesValueChanged(bool);
    void taskSelected(int, bool);
    void startTaskClicked();
    void stopTaskClicked();

public slots:
    void iconActivated(QSystemTrayIcon::ActivationReason);
    void menuActionHandler(QAction *action);
    void autoStart(bool checked);
    void tracker(bool checked);
    void autoTracking(bool checked);
    void autoStop(bool checked);
    void updateRecentTasks();
    void openCloseWindowAction();
    void contactSupport();
    void onAppClose();
#ifdef _WIDGET_EXISTS_
    void widgetToggl(bool checked);
#endif

protected:
    explicit TrayManager(QObject *parent = nullptr);

private:
#ifndef Q_OS_MACOS
    QSystemTrayIcon *trayIcon = nullptr;
#endif

    QMenu *trayMenu;
    QSettings settings;
    int menuWidth = 100; // px

    void createActions(QMenu *);
    void assignActions(QMenu *);
    QAction *openAct;
    QAction *recentTasksTitleAct;
    QAction *startTaskAct;
    QAction *stopTaskAct;
    QAction *trackerAct;
    QAction *autoTrackingAct;
    QAction *autoStartAct;
    QAction *widgetAct;
    QAction *autoStopAct;
    QAction *helpAct;
    QAction *quitAct;

    MainWidget *mainWidget;

    bool areMenusEqual(QMenu *menu1, QMenu *menu2);

#ifdef _WIDGET_EXISTS_
    Widget *widget;
public:
    void setWidget(Widget *widget);
#endif
};


#endif //THEGUI_TRAYMANAGER_H
