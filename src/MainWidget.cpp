#include <QEventLoop>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QDir>
#include <QtCore/QStringBuilder>

#include "MainWidget.h"
#include "ui_MainWidget.h"

#include "Settings.h"
#include "WindowEventsManager.h"
#include "Autorun.h"


MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::MainWidget)
{
    ui->setupUi(this);

//    this->setAttribute(Qt::WA_TranslucentBackground);
//    this->setAutoFillBackground(true);

//    QPixmap bkgnd(MAIN_BG);
////    bkgnd = bkgnd.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
//    QPalette palette;
//    palette.setBrush(QPalette::Window, bkgnd);
//    this->setPalette(palette);

    // set some defaults
    setLoggedIn(false);

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

    restoreGeometry(settings.value(QStringLiteral("mainWindowGeometry")).toByteArray()); // from QWidget; restore saved window position

    taskbar = new QTaskbarControl(this);

#ifdef Q_OS_LINUX
    taskbar->setAttribute(QTaskbarControl::LinuxDesktopFile, settings.value(DESKTOP_FILE_PATH));
#endif
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
    this->triggerTimerStatusFetchAsync();
}

void MainWidget::handleSpacingEvents()
{
//    qInfo("Size: %d x %d", size().width(), size().height());
    if (MainWidgetWasInitialised) {
        this->setUpdatesEnabled(false);
        QTWEView->resize(size()); // resize webview
        settings.setValue(QStringLiteral("mainWindowGeometry"), saveGeometry()); // save window position
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
            fetchRecentTasks();
        }
        getCurrentWebTimerStatusEmitted();
    } else {
        checkAPIkey();
    }
}

void MainWidget::setupWebview()
{
    QTWEView = new TCWebEngineView(this);
    QTWEView->setAcceptDrops(false);
//    QTWEView->setAttribute(Qt::WA_TranslucentBackground);
    QObject::connect(QTWEView, &QWebEngineView::loadStarted, this, &MainWidget::handleLoadStarted);
    QObject::connect(QTWEView, &QWebEngineView::loadProgress, this, &MainWidget::handleLoadProgress);
    QObject::connect(QTWEView, &QWebEngineView::loadFinished, this, &MainWidget::handleLoadFinished);


    QTWEProfile = new QWebEngineProfile(APPLICATION_NAME, QTWEView); // set "profile" as appName
    QTWEProfile->setHttpUserAgent(CONN_USER_AGENT); // add useragent to this profile

    QTWESettings = QTWEProfile->settings();
    QTWESettings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true); // modify settings: enable Fullscreen
    //QTWESettings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);

    TCri = new TCRequestInterceptor();
    QTWEProfile->setRequestInterceptor(TCri);

    QTWEPage = new QWebEnginePage(QTWEProfile, QTWEView);
//    QTWEPage->setBackgroundColor(Qt::transparent);
    QTWEView->setPage(QTWEPage);
    QObject::connect(QTWEProfile, &QWebEngineProfile::downloadRequested, this, &MainWidget::downloadRequested);

    refreshBind = new QShortcut(QKeySequence::Refresh, this);
    refreshBind->setContext(Qt::ApplicationShortcut);
    QObject::connect(refreshBind, &QShortcut::activated, this, &MainWidget::webviewRefresh);

    fullscreenBind = new QShortcut(QKeySequence::FullScreen, this);
    fullscreenBind->setContext(Qt::ApplicationShortcut);
    QObject::connect(fullscreenBind, &QShortcut::activated, this, &MainWidget::webviewFullscreen);

    QObject::connect(QTWEPage, &QWebEnginePage::titleChanged, this, &MainWidget::webpageTitleChanged);

    checkAPIkey(); // try to fetch api key once on the start
    this->goToTimerPage(); // loads main app url
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
    if (!loggedIn) {
        // javascript magic: hide the "news" on login page
        this->runJSinPage(QStringLiteral(
                              "jQuery('#about .news').parent().parent().attr('class', 'hidden').siblings().first().attr('class', 'col-xs-12 col-sm-10 col-sm-push-1 col-md-8 col-md-push-2 col-lg-6 col-lg-push-3')"));
    }
}

void MainWidget::wasTheWindowLeftOpened()
{
    if (settings.value(SETT_WAS_WINDOW_LEFT_OPENED, true).toBool()) {
        this->open();
    }
}

void MainWidget::webpageTitleChanged(const QString title)
{
//    qInfo("[NEW_TC]: Webpage title changed: ");
//    qInfo(title.toLatin1().constData());
    checkIfLoggedIn(title);
    if (!loggedIn) {
        setApiKey(QStringLiteral(""));
        LastTasks.clear(); // clear last tasks
        LastTasksCache = QJsonDocument(); // clear the cache
        emit lastTasksChanged();
    }
    this->setWindowTitle(title); // https://trello.com/c/J8dCKeV2/43-niech-tytul-apki-desktopowej-sie-zmienia-
    emit pageTitleChanged(title);
}

void MainWidget::clearCache()
{
    this->setUpdatesEnabled(false);
    this->runJSinPage(QStringLiteral("localStorage.clear()"));
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

void MainWidget::checkIfLoggedIn(const QString title)
{
    if (title.toLower().contains(QRegExp("log in|login|register|create free account|create account|time tracking software|blog"))) {
        setLoggedIn(false); // when we log out, we need to set this variable again
    }
}

void MainWidget::open()
{
    settings.setValue(SETT_WAS_WINDOW_LEFT_OPENED, true); // save if window was opened
    settings.sync();
    restoreGeometry(settings.value(QStringLiteral("mainWindowGeometry")).toByteArray());
    QTWEView->resize(size()); // resize webview
    show();
    setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    raise();  // for MacOS
    activateWindow(); // for Windows
    raise();  // for MacOS
    emit windowStatusChanged(true);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 256); // force waiting before we quit this func; and process some events
}

void MainWidget::runJSinPage(const QString js)
{
//    qDebug() << "Running JS: " << js; // too verbose
    QTWEPage->runJavaScript(js);
}

void MainWidget::forceLoadUrl(const QString url)
{
    this->runJSinPage("window.location='" + url + "'");
}

bool MainWidget::checkIfOnTimerPage()
{
    if (QTWEPage->url().toString().indexOf(QLatin1String("app#/timesheets/timer")) != -1) {
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
//                QThread::msleep(128);
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
//        QThread::msleep(128);

    } else {
        this->refreshTimerPageData();
    }
    this->webpageTitleChanged(QTWEPage->title());
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
    this->runJSinPage(QStringLiteral("$('html').click();$('#timer-task-picker, #new-entry-task-picker').tcTTTaskPicker('hide').tcTTTaskPicker('show');"));
}

void MainWidget::refreshTimerPageData()
{
    this->runJSinPage(QStringLiteral("if(typeof(angular) !== 'undefined'){"
                      "angular.element(document.body).injector().get('TTEntriesService').reload();"
                      "angular.element(document.body).injector().get('UserLogService').reload();"
                      "}"));
    this->triggerTimerStatusFetchAsync();
}

void MainWidget::triggerTimerStatusFetchAsync()
{
    qDebug() << "[JS] Timer STATUS fetch";
    this->runJSinPage(QStringLiteral("typeof(angular) !== 'undefined' && typeof(angular.element(document.body).injector()) !== 'undefined' && angular.element(document.body).injector().get('TimerService').status();"));
}

void MainWidget::getCurrentWebTimerStatusEmitted()
{
    QTWEPage->runJavaScript(QStringLiteral("typeof(angular) !== 'undefined' && typeof(angular.element(document.body).injector()) !== 'undefined' && JSON.stringify(angular.element(document.body).injector().get('TimerService').timer)"),
        [this](const QVariant &v)
    {
        if(!v.isValid() || !v.toBool()) { // when the above statement is invalid, OR returns false
            qWarning() << "Timer Status Refresh - failed";
            return;
        }
//        qDebug() << "[JS] Timer STATUS Update"; // too verbose
        emit updateTimerStatus(v.toByteArray());
    });
}

void MainWidget::fetchRecentTasks()
{
    QTWEPage->runJavaScript(QStringLiteral("typeof(TC) !== 'undefined' && JSON.stringify(TC.TimeTracking.Lasts)"), [this](const QVariant &v)
    {
//        LastTasks.clear(); // don't need to clear a QHash

        if (!v.isValid()) {
            return;
        }

        QJsonDocument itemDoc = QJsonDocument::fromJson(v.toByteArray());
        if (itemDoc != LastTasksCache) {
            QJsonArray rootArray = itemDoc.array();
            for (QJsonValueRef val: rootArray) {
                QJsonObject obj = val.toObject();
//            qDebug() << obj.value("task_id").toString().toInt() << ": " << obj.value("name").toString();
                LastTasks.insert(obj.value(QStringLiteral("name")).toString(), obj.value(QStringLiteral("task_id")).toString().toInt());
            }
            LastTasksCache = itemDoc;
            emit lastTasksChanged();
        }
    });
}

void MainWidget::checkAPIkey()
{
    QString apiKeyStr = settings.value(SETT_APIKEY).toString().trimmed();
    if (apiKeyStr.isEmpty() || apiKeyStr == QLatin1String("false")) {
        fetchAPIkey();
    }
}

void MainWidget::fetchAPIkey()
{
    auto *apiPageView = new TCWebEngineView(this);
    auto *apiPage = new QWebEnginePage(QTWEProfile, apiPageView);
    apiPageView->setPage(apiPage);
    apiPage->load(QUrl(APIKEY_URL));

    QObject::connect(apiPageView, &QWebEngineView::loadFinished, [=](bool loadOk) {
        if (!loadOk) {
            qWarning() << "Couldn't fetch API key";
            return;
        }

        qInfo() << "Fetched API key";

        apiPageView->page()->toPlainText([=](const QString &pageText) {
            if (pageText.contains(QLatin1String("no_session"))) {
                setLoggedIn(false);
            } else {
                setLoggedIn(true);
                setApiKey(pageText); // not no_session - then it's an API KEY
                webpageTitleChanged(QStringLiteral(""));
            }
            apiPageView->deleteLater(); // after we finished using the temporary view, we can put it into Qt's "GC" delete queue
        });
    });
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
                // no need to call QObject::disconnect() - "The connection will automatically disconnect if the sender or the context is destroyed." - DOCS
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

void MainWidget::setLoggedIn(bool loggedIn) {
    MainWidget::loggedIn = loggedIn;
    qInfo() << "Logged in: " << loggedIn;
    emit pageStatusChanged(loggedIn);
}
