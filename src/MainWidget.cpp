#include <QEventLoop>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QDir>

#include "MainWidget.h"
#include "ui_MainWidget.h"

#include "Settings.h"
#include "WindowEventsManager.h"


MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::MainWidget)
{
    ui->setupUi(this);

//    this->setAttribute(Qt::WA_TranslucentBackground);
//    this->setAutoFillBackground(true);

    QPixmap bkgnd(MAIN_BG);
//    bkgnd = bkgnd.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPalette palette;
    palette.setBrush(QPalette::Window, bkgnd);
    this->setPalette(palette);

    // set some defaults
    loggedIn = false;

    this->setMinimumSize(QSize(350, 500));
//
//#ifdef Q_OS_MACOS
//    this->setWindowFlags(Qt::Sheet | Qt::WindowCloseButtonHint | Qt::WindowTitleHint | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint);
//#else
//    this->setWindowFlags( Qt::WindowStaysOnTopHint );
//#endif
//
#ifdef Q_OS_MACOS
    this->setWindowFlags(Qt::WindowCloseButtonHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
#else
//    this->setWindowFlags( Qt::Window );
#endif

    this->setAcceptDrops(false);

    restoreGeometry(settings.value("mainWindowGeometry").toByteArray()); // from QWidget; restore saved window position

    taskbar = new QTaskbarControl(this);
    taskbar->setAttribute(QTaskbarControl::LinuxDesktopFile, QDir::homePath() + "/.config/autostart/TimeCamp Desktop.desktop");
}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::init()
{
    this->setWindowTitle(WINDOW_NAME);
    this->setupWebview(); // starts the embedded webpage
    MainWidgetWasInitialised = true;
    this->wasTheWindowLeftOpened();
    this->refreshTimerStatus();
}

void MainWidget::handleSpacingEvents()
{
//    qInfo("Size: %d x %d", size().width(), size().height());
    if (MainWidgetWasInitialised) {
        this->setUpdatesEnabled(false);
        QTWEView->resize(size()); // resize webview
        settings.setValue("mainWindowGeometry", saveGeometry()); // save window position
        settings.sync();
        this->setUpdatesEnabled(true);
    }
}

void MainWidget::moveEvent(QMoveEvent *event)
{
    this->handleSpacingEvents();
    QWidget::moveEvent(event); // do the default "whatever happens on move"
}

void MainWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        this->handleSpacingEvents();
    }
    QWidget::changeEvent(event);
}

void MainWidget::resizeEvent(QResizeEvent *event)
{
    this->handleSpacingEvents();
    QWidget::resizeEvent(event); // do the default "whatever happens on resize"
}

void MainWidget::closeEvent(QCloseEvent *event)
{
    this->handleSpacingEvents();
    settings.setValue(SETT_WAS_WINDOW_LEFT_OPENED, false); // save if window was opened
    settings.sync();
    hide(); // hide our window when X was pressed
    emit windowStatusChanged(false);
    event->ignore(); // don't do the default action (which usually is app exit)
}

void MainWidget::webpageDataUpdateOnInterval()
{
    if (loggedIn) {
        emit checkIsIdle(); // this shouldn't really be here; check idle or not only when user is logged in
        if(this->isActiveWindow()) { // only when we're in the window, clicking
//            checkIsTimerRunning();
            refreshTimerStatus();
            fetchRecentTasks();
        }
        QString apiKeyStr = settings.value(SETT_APIKEY).toString().trimmed();
        if (apiKeyStr.isEmpty() || apiKeyStr == "false") {
            fetchAPIkey();
        }
    } else {
        setApiKey("");
    }
}

void MainWidget::setupWebview()
{
    QTWEView = new TCWebEngineView(this);
    QTWEView->setAcceptDrops(false);
    QTWEView->setAttribute(Qt::WA_TranslucentBackground);
    connect(QTWEView, &QWebEngineView::loadStarted, this, &MainWidget::handleLoadStarted);
    connect(QTWEView, &QWebEngineView::loadProgress, this, &MainWidget::handleLoadProgress);
    connect(QTWEView, &QWebEngineView::loadFinished, this, &MainWidget::handleLoadFinished);


    QTWEProfile = new QWebEngineProfile(APPLICATION_NAME, QTWEView); // set "profile" as appName
    QTWEProfile->setHttpUserAgent(CONN_USER_AGENT); // add useragent to this profile

    QTWESettings = QTWEProfile->settings();
    QTWESettings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true); // modify settings: enable Fullscreen
    //QTWESettings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);

    TCri = new TCRequestInterceptor();
    QTWEProfile->setRequestInterceptor(TCri);

    QTWEPage = new QWebEnginePage(QTWEProfile, QTWEView);
    QTWEPage->setBackgroundColor(Qt::transparent);
    QTWEView->setPage(QTWEPage);
    connect(QTWEProfile, &QWebEngineProfile::downloadRequested, this, &MainWidget::downloadRequested);

    refreshBind = new QShortcut(QKeySequence::Refresh, this);
    refreshBind->setContext(Qt::ApplicationShortcut);
    connect(refreshBind, &QShortcut::activated, this, &MainWidget::webviewRefresh);

    fullscreenBind = new QShortcut(QKeySequence::FullScreen, this);
    fullscreenBind->setContext(Qt::ApplicationShortcut);
    connect(fullscreenBind, &QShortcut::activated, this, &MainWidget::webviewFullscreen);

//    pagePointer = m_pWebEngineView->page();

    this->goToTimerPage(); // loads main app url
//    QTWEPage->load(QUrl(APPLICATION_URL));
//    QTWEPage->load(QUrl("http://request.urih.com/"));


    connect(QTWEPage, &QWebEnginePage::titleChanged, this, &MainWidget::webpageTitleChanged);
}

void MainWidget::handleLoadStarted()
{
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
//    qDebug() << "cursor: load started";
    taskbar->setProgressVisible(true);
}

void MainWidget::handleLoadProgress(int progress)
{
//    qDebug() << "cursor: load progress " << progress;
    if (progress > 0) {
        taskbar->setProgress((float) progress / 100);
    }
    if (progress == 100) {
        QGuiApplication::restoreOverrideCursor(); // pop from the cursor stack
        taskbar->setProgressVisible(false);
    }
}

void MainWidget::handleLoadFinished(bool ok)
{
    Q_UNUSED(ok);
    QGuiApplication::restoreOverrideCursor(); // pop from the cursor stack
//    qDebug() << "cursor: load finished " << ok;
    taskbar->setProgressVisible(false);
}

void MainWidget::wasTheWindowLeftOpened()
{
    if (settings.value(SETT_WAS_WINDOW_LEFT_OPENED, true).toBool()) {
        this->open();
    }
}

void MainWidget::webpageTitleChanged(QString title)
{
//    qInfo("[NEW_TC]: Webpage title changed: ");
//    qInfo(title.toLatin1().constData());
    checkIfLoggedIn(title);
    if (!loggedIn) {
        // javascript magic: hide the "news" on login page
        this->runJSinPage("jQuery('#about .news').parent().parent().attr('class', 'hidden').siblings().first().attr('class', 'col-xs-12 col-sm-10 col-sm-push-1 col-md-8 col-md-push-2 col-lg-6 col-lg-push-3')");
        LastTasks.clear(); // clear last tasks
        LastTasksCache = QJsonDocument(); // clear the cache
        emit lastTasksChanged();
    }
    emit pageStatusChanged(loggedIn, title);
    this->setWindowTitle(title); // https://trello.com/c/J8dCKeV2/43-niech-tytul-apki-desktopowej-sie-zmienia-
//    this->checkIsTimerRunning();
}

void MainWidget::clearCache()
{
    this->setUpdatesEnabled(false);
    this->runJSinPage("localStorage.clear()");
    QTWEProfile->clearAllVisitedLinks();
    QTWEProfile->clearHttpCache();
    this->setUpdatesEnabled(true);
}

void MainWidget::webviewRefresh()
{
    qDebug("[NEW_TC]: page refresh");
    this->clearCache();
    this->goToTimerPage();
    this->forceLoadUrl(APPLICATION_URL);
}

void MainWidget::webviewFullscreen()
{
    qDebug("[NEW_TC]: go full screen");
    this->setUpdatesEnabled(false);
    this->setWindowState(this->windowState() ^ Qt::WindowFullScreen);
    this->setUpdatesEnabled(true);
}

void MainWidget::checkIfLoggedIn(QString title)
{
    if (!title.toLower().contains(QRegExp("log in|login|register|create free account|create account|time tracking software|blog"))) {
        loggedIn = true;
    } else {
        loggedIn = false; // when we log out, we need to set this variable again
    }
}

void MainWidget::open()
{
    settings.setValue(SETT_WAS_WINDOW_LEFT_OPENED, true); // save if window was opened
    settings.sync();
    restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
    QTWEView->resize(size()); // resize webview
    show();
    setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    raise();  // for MacOS
    activateWindow(); // for Windows
    raise();  // for MacOS
    emit windowStatusChanged(true);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 256); // force waiting before we quit this func; and process some events
}

void MainWidget::runJSinPage(QString js)
{
    qDebug() << "Running JS: " << js;
    QTWEPage->runJavaScript(js);
}

void MainWidget::forceLoadUrl(QString url)
{
    this->runJSinPage("window.location='" + url + "'");
}

bool MainWidget::checkIfOnTimerPage()
{
    if (QTWEPage->url().toString().indexOf("app#/timesheets/timer") != -1) {
        return true;
    }
    return false;
}

void MainWidget::goToTimerPage()
{
    if (!this->checkIfOnTimerPage()) {
        QEventLoop loop;
        QMetaObject::Connection conn1 = QObject::connect(QTWEPage, &QWebEnginePage::loadFinished, &loop, &QEventLoop::quit);
        QMetaObject::Connection conn2 = QObject::connect(QTWEPage, &QWebEnginePage::loadProgress, [&loop](const int &newValue)
        {
            qDebug() << "Load progress: " << newValue;
            if (newValue == 100) {
                QThread::msleep(128);
                loop.quit();
            }
        });
        QMetaObject::Connection conn3 = QObject::connect(QTWEPage, &QWebEnginePage::iconUrlChanged, [this]()
        {
            this->webpageTitleChanged(QTWEPage->title());
        });
        QTWEPage->load(QUrl(APPLICATION_URL));
        loop.exec();
        QObject::disconnect(conn1);
        QObject::disconnect(conn2);
        QObject::disconnect(conn3);
        QThread::msleep(128);

        this->webpageTitleChanged(QTWEPage->title());
    } else {
        this->refreshTimerPageData();
    }
}

void MainWidget::goToAwayPage()
{
    this->clearCache();
    this->open();

    QTWEPage->load(QUrl(OFFLINE_URL));
}

void MainWidget::chooseTask()
{
    this->goToTimerPage();
    this->showTaskPicker();
    this->open();
}

void MainWidget::showTaskPicker()
{
    this->runJSinPage("$('html').click();$('#timer-task-picker, #new-entry-task-picker').tcTTTaskPicker('hide').tcTTTaskPicker('show');");
}

void MainWidget::refreshTimerPageData()
{
    this->runJSinPage("if(typeof(angular) !== 'undefined'){"
                      "angular.element(document.body).injector().get('TTEntriesService').reload();"
                      "angular.element(document.body).injector().get('UserLogService').reload();"
                      "}");
    this->refreshTimerStatus();
}

void MainWidget::refreshTimerStatus()
{
    this->runJSinPage("typeof(angular) !== 'undefined' && angular.element(document.body).injector().get('TimerService').status();");
    this->checkIsTimerRunning();
}

void MainWidget::shouldRefreshTimerStatus(bool isRunning, QString name)
{
    if (!this->isActiveWindow()) { // only when we're in the window, clicking
        return;
    }

    QString timerTempStr = "angular.element(document.body).injector().get('TimerService').timer";

    QString jsToRun = "typeof(angular) !== 'undefined' &&  typeof("+timerTempStr+".isTimerRunning) !== 'undefined' "
                      " && "+timerTempStr+".isTimerRunning == " + QString::number(isRunning);

    if(!name.isEmpty()) {
        // if we have a timer name, make sure it's the same in the web AND in cpp
        jsToRun += " && ( typeof("+timerTempStr+".name) !== 'undefined' && "+timerTempStr+".name == '" + name + "')";
    } else {
        // if we don't have timer name, make sure it's either undefined OR "same" (empty = "", spaces, etc)
        jsToRun += " && ( typeof("+timerTempStr+".name) === 'undefined' || "+timerTempStr+".name == '" + name + "')";
    }

    qDebug() << "Running JS2: " << jsToRun;

    QTWEPage->runJavaScript(jsToRun,
                            [this, isRunning, name](const QVariant &v)
                            {
                                if(v.isValid() && !v.toBool()) { // when the above statement IS NOT TRUE (i.e. we have no angular or isTimerRunning is a different flag
                                    this->refreshTimerStatus();
                                }
                            });
}

void MainWidget::checkIsTimerRunning()
{
    QTWEPage->runJavaScript("typeof(angular) !== 'undefined' && JSON.stringify(angular.element(document.body).injector().get('TimerService').timer)",
        [this](const QVariant &v)
    {
        if(!v.isValid()) {
            return;
        }
        emit updateTimerStatus(v.toByteArray());
    });
}

void MainWidget::fetchRecentTasks()
{
    QTWEPage->runJavaScript("typeof(TC) !== 'undefined' && JSON.stringify(TC.TimeTracking.Lasts)", [this](const QVariant &v)
    {
//        LastTasks.clear(); // don't need to clear a QHash

//        qDebug() << v.toString();
        if (!v.isValid()) {
            return;
        }

        QJsonDocument itemDoc = QJsonDocument::fromJson(v.toByteArray());
        if (itemDoc != LastTasksCache) {
            QJsonArray rootArray = itemDoc.array();
            for (QJsonValueRef val: rootArray) {
                QJsonObject obj = val.toObject();
//            qDebug() << obj.value("task_id").toString().toInt() << ": " << obj.value("name").toString();
                LastTasks.insert(obj.value("name").toString(), obj.value("task_id").toString().toInt());
            }
            LastTasksCache = itemDoc;
            emit lastTasksChanged();
        }
    });
}

void MainWidget::fetchAPIkey()
{
//    QTWEPage->runJavaScript("await window.apiService.getToken()",
    QTWEPage->runJavaScript("typeof(window.apiService) !== 'undefined' && window.apiService.getToken().$$state.value", [this](const QVariant &v)
    {
//        qDebug() << "API Key: " << v.toString();

        if(v.isValid() && v.toBool()) { // http://doc.qt.io/qt-5/qvariant.html#toBool
            // is true for many types when non-zero; and (quote):

            // "[returns true] if the variant has type QString or QByteArray
            // and its lower-case content is not one of the following:
            // empty, "0" or "false"; otherwise returns false"

            setApiKey(v.toString());
        }
    });
    // no problem if it fails, since it runs in a loop "webpageDataUpdateOnInterval"
    // and that handles failed states (empty or "false" api key)
}


void MainWidget::quit()
{
    QCoreApplication::quit();
}

void MainWidget::setApiKey(const QString &apiKey)
{
    settings.setValue(SETT_APIKEY, apiKey); // save apikey to settings
    settings.sync();
}

void MainWidget::downloadRequested(QWebEngineDownloadItem* download) {
//    qDebug() << "Format: " <<  download->savePageFormat();
    qDebug() << "url: " << download->url();
    qDebug() << "mimeType: " << download->mimeType();
    qDebug() << "Save path: " << download->path();
    qDebug() << "Bytes: " << download->receivedBytes() << " / " << download->totalBytes();

    QMessageBox msgBox;
    msgBox.setTextInteractionFlags(Qt::NoTextInteraction);
    msgBox.setIconPixmap(QPixmap(MAIN_ICON).scaledToWidth(96));
    msgBox.setText(tr("You started a file download"));
    msgBox.setInformativeText(tr("Do you want to save this file to:") + QStringLiteral("\n") + download->path() + QStringLiteral("?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);

    // bring the Message to front
//    msgBox.show();
//    msgBox.setWindowState(Qt::WindowActive);
//    msgBox.raise();  // for MacOS
//    msgBox.activateWindow(); // for Windows

    QObject::connect(&msgBox, &QDialog::finished, [this, &download](int ret)
    {
        switch (ret) {
            case QMessageBox::Yes:
            default:
                qDebug() << "[SaveFileDialog] Yes";
                download->accept();
                QObject::connect(download, &QWebEngineDownloadItem::downloadProgress, this, &MainWidget::setDownloadProgress);
                QObject::connect(download, &QWebEngineDownloadItem::finished, this, &MainWidget::downloadFinished);
                // no need to call disconnect() - "The connection will automatically disconnect if the sender or the context is destroyed." - DOCS
                // sender (here) - the 'download'
                break;
            case QMessageBox::No:
                qDebug() << "[SaveFileDialog] No";
                download->cancel();
                break;
        }
    });

    msgBox.exec();
}

void MainWidget::setDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    taskbar->setProgressVisible(true);
    if (bytesTotal > 0) {
        taskbar->setProgress((float) bytesReceived / bytesTotal);
    }
}

void MainWidget::downloadFinished() {
    // TODO: add some form of notification - KDE/snorenotify?
    qDebug() << "[Download] Finished";

    taskbar->setProgress(1);
    taskbar->setProgressVisible(false);
}
