#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QMenu>
#include <QAction>
#include <QShortcut>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QUrl>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QSettings>
#include <QJsonDocument>

#include "third-party/vendor/de/skycoder42/qtaskbarcontrol/qtaskbarcontrol.h"

#include "Overrides/TCRequestInterceptor.h"
#include "Overrides/TCWebEngineView.h"
#include "Task.h"

namespace Ui
{
    class MainWidget;
}

class MainWidget : public QWidget
{
Q_OBJECT
    Q_DISABLE_COPY(MainWidget)

public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget() override;
    void init();

    void webpageDataUpdateOnInterval();
    QHash<QString, int> LastTasks;
    QJsonDocument LastTasksCache;

signals:
    void pageStatusChanged(bool);
    void pageTitleChanged(QString);
    void checkIsIdle();
    void windowStatusChanged(bool);
    void lastTasksChanged();
    void updateTimerStatus(QByteArray);

protected:
    void handleSpacingEvents();
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

public slots:
    void clearCache();
    void webviewRefresh();
    void webviewFullscreen();

    void webpageTitleChanged(const QString &title);

    void wasTheWindowLeftOpened();
    void open();
    void chooseTask();
    void quit();

    void handleLoadStarted();
    void handleLoadProgress(int);
    void handleLoadFinished(bool);
    void goToAwayPage();
    void triggerTimerStatusFetchAsync();
    void getCurrentWebTimerStatusEmitted();
    void downloadRequested(QWebEngineDownloadItem* download);
    void setDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished();

private:
    Ui::MainWidget *ui;
    QSettings settings;

    TCRequestInterceptor *TCri;

    TCWebEngineView *QTWEView;
    QWebEngineProfile *QTWEProfile;
    QWebEnginePage *QTWEPage;
    QWebEngineSettings *QTWESettings;

    void runJSinPage(QString js);
    void forceLoadUrl(QString url);
    void showTaskPicker();
    void fetchRecentTasks();
    void checkAPIkey();
    void fetchAPIkey();
    bool checkIfOnTimerPage();
    void goToTimerPage();
    void refreshTimerPageData();

    bool checkIfOnLoginPage();
    void setupWebview();

    QShortcut *refreshBind;
    QShortcut *fullscreenBind;

    bool MainWidgetWasInitialised = false;
    bool loggedIn;
private:
    void setLoggedIn(bool loggedInUpdate);
    void setApiKey(const QString &apiKey);

    QTaskbarControl *taskbar;

};

#endif // MAINWIDGET_H
