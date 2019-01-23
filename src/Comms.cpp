#include "Comms.h"
#include "Settings.h"

#include "DbManager.h"
#include "ApiHelper.h"
#include "Formatting.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>

#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

Comms &Comms::instance()
{
    static Comms _instance;
    return _instance;
}

Comms::Comms(QObject *parent) : QObject(parent)
{
    qnam.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    
    apiHelper = new ApiHelper(API_URL);

    lastTimerStatusCheck = QDateTime::currentMSecsSinceEpoch();
    // connect the callback function
    QObject::connect(&qnam, &QNetworkAccessManager::finished, this, &Comms::genericReply);
}

void Comms::timedUpdates()
{
    lastSync = settings.value(SETT_LAST_SYNC, 0).toLongLong(); // set our variable to value from settings (so it works between app restarts)

    QDateTime timestamp;
    timestamp.setTime_t(lastSync/1000);
    qDebug() << "[AppList] last sync: " << timestamp.toString(Qt::ISODate);

    setCurrentTime(QDateTime::currentMSecsSinceEpoch()); // time of DB fetch is passed, so we can update to it if successful

    // we can't be calling API if we don't have the key
    if (!apiHelper->updateApiKeyFromSettings()) {
        return;
    }

    // otherwise (if we have api key), send apps data
    tryToSendAppData();

    // and (if we still have api key), update settings
    getUserInfo();
    getSettings();
    getTasks();
}

void Comms::tryToSendAppData()
{
    QVector<AppData> appList;
    try {
        appList = DbManager::instance().getAppsSinceLastSync(lastSync); // get apps since last sync; SQL queries for LIMIT = MAX_ACTIVITIES_BATCH_SIZE
    } catch (...) {
        qInfo("[AppList] DB fail");
        return;
    }

    qDebug() << "[AppList] length: " << appList.length();
    // send only if there is anything to send (0 is if "computer activities" are disabled, 1 is sometimes only with "IDLE" - don't send that)
    if (appList.length() > 1 || (appList.length() == 1 && appList.first().getAppName() != ACTIVITY_IDLE_NAME)) {
        sendAppData(&appList);
        if (appList.length() >= MAX_ACTIVITIES_BATCH_SIZE) {
            qInfo() << "[AppList] was big";
            lastBatchBig = true;
        } else {
            lastBatchBig = false;
            retryCount = 0; // we send small amount of activities, so our last push must've been success
        }
    }
}

void Comms::clearLastApp()
{
    lastApp = nullptr;
}

void Comms::appChanged(AppData *app)
{
    emit announceAppChange(app);

    if (lastApp == nullptr) {
        qDebug() << "[FIRST APP DETECTED]";
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        lastApp = app;
        app->setStart(now);
        return; // this was a first app, so we have to wait for a second app to have a time diff
    }

    // not the same activity? we need to log
    if (0 != QString::compare(app->getAppName(), lastApp->getAppName())) { // maybe AppName changed
        reportApp(app);
        return;
    }

    if (0 != QString::compare(app->getWindowName(), lastApp->getWindowName())) { // or maybe WindowName changed
        reportApp(app);
        return;
    }
}

void Comms::reportApp(AppData *app)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    lastApp->setEnd(now - 1); // it already must have start, now we only update end time

    if((lastApp->getEnd() - lastApp->getStart()) > 1000) { // if activity is longer than 1sec

        if (!lastApp->getAdditionalInfo().isEmpty()) { // if we have "additional info"
            // this means it's a website, so we set "app name" to internet
            // then backend does it's magic and each website is a different activity
            lastApp->setAppName(Comms::WEBSITES_APPNAME);
        }

        try {
            emit DbSaveApp(lastApp);
        } catch (...) {
            qWarning("[DBSAVE] DB fail");
            return;
        }

        qDebug("[DBSAVE] %llds - %s | %s\nADD_INFO: %s \n",
               (lastApp->getEnd() - lastApp->getStart()) / 1000,
               lastApp->getAppName().toLatin1().constData(),
               lastApp->getWindowName().toLatin1().constData(),
               lastApp->getAdditionalInfo().toLatin1().constData()
        );

        app->setStart(now); // saved OK, so new App starts "NOW"
    } else {
        qWarning("[DBSAVE] Activity too short (%lldms) - %s",
              lastApp->getEnd() - lastApp->getStart(),
              lastApp->getAppName().toLatin1().constData()
        );

        app->setStart(lastApp->getStart()); // not saved, so new App starts when the old one has started
    }

    // some weird case:
    if(lastApp->getStart() > lastApp->getEnd()) { // it started later than it finished?!
        qWarning("[DBSAVE] Activity (%s) broken: from %lld, to %lld",
              lastApp->getAppName().toLatin1().constData(),
              lastApp->getStart(),
              lastApp->getEnd()
        );

        app->setStart(lastApp->getStart()); // not saved, so new App starts when the old one has started
    }

    lastApp = app; // update app reference
}


void Comms::sendAppData(const QVector<AppData> *appList)
{
    bool canSendActivityInfo = !settings.value(QStringLiteral(SETT_WEB_SETTINGS_PREFIX) + QStringLiteral("dontCollectComputerActivity")).toBool();
    bool canSendWindowTitles = settings.value(QStringLiteral(SETT_WEB_SETTINGS_PREFIX) + QStringLiteral("collectWindowTitles")).toBool();

    QUrlQuery params = apiHelper->getDefaultApiParams();

    int count = 0;

    for (const AppData &app: *appList) {
/*
    qDebug() << "[NOTIFY OF APP]";
    qDebug() << "getAppName: " << app.getAppName();
    qDebug() << "getWindowName: " << app.getWindowName();
    qDebug() << "getAdditionalInfo: " << app.getAdditionalInfo();
    qDebug() << "getDomainFromAdditionalInfo: " << app.getDomainFromAdditionalInfo();
    qDebug() << "getStart: " << app.getStart();
    qDebug() << "getEnd: " << app.getEnd();
*/

        // skip sending IDLE activities
        if (app.getAppName() == ACTIVITY_IDLE_NAME || app.getWindowName() == ACTIVITY_IDLE_NAME) {
            continue;
        }

        // build weird GET params array

        QString base_str = QStringLiteral("computer_activities") + QStringLiteral("[") + QString::number(count) + QStringLiteral("]");

        if (canSendActivityInfo) {
            params.addQueryItem(base_str + QStringLiteral("[application_name]"), app.getAppName());

            if (canSendWindowTitles) {
                params.addQueryItem(base_str + QStringLiteral("[window_title]"), app.getWindowName());

                // "Web Browser App" when appName is Internet but no domain
                if (!app.getAdditionalInfo().isEmpty()) {
                    params.addQueryItem(base_str + QStringLiteral("[website_domain]"), app.getDomainFromAdditionalInfo());
                }
            } else {
                params.addQueryItem(base_str + QStringLiteral("[window_title]"), QStringLiteral(""));
            }

        } else { // can't send activity info, collect_computer_activities == 0
            params.addQueryItem(base_str + QStringLiteral("[application_name]"), SETT_HIDDEN_COMPUTER_ACTIVITIES_CONST_NAME);
            params.addQueryItem(base_str + QStringLiteral("[window_title]"), QStringLiteral(""));
        }

        QString start_time = Formatting::DateTimeTC(app.getStart());
        params.addQueryItem(base_str + QStringLiteral("[start_time]"), start_time);
//            qDebug() << "converted start_time: " << start_time;

        QString end_time = Formatting::DateTimeTC(app.getEnd());
        params.addQueryItem(base_str + QStringLiteral("[end_time]"), end_time);
//            qDebug() << "converted end_time: " << end_time;

        count++;
        lastSync = app.getEnd(); // set our internal variable to value from last app
    }

    QUrl api = apiHelper->activitiesUrl();

    commsReplies.insert(api, &Comms::appDataReply);
    this->postRequest(api, params);
}

void Comms::appDataReply(QByteArray buffer)
{
    buffer.truncate(MAX_LOG_TEXT_LENGTH);
    qDebug() << "AppData Response: " << buffer;
    if (buffer == "") {
        qDebug() << "update last sync to whenever we sent the data";
        settings.setValue(SETT_LAST_SYNC, lastSync); // update last sync to our internal variable (to the last app in the last set)
        this->checkBatchSize();
    }
}

void Comms::checkBatchSize()
{
    if (lastBatchBig) {
        retryCount++;
        if(retryCount < 10) {
            auto *timer = new QTimer();
            timer->setSingleShot(true);
            QObject::connect(timer, &QTimer::timeout, this, &Comms::timedUpdates);
        }
    }
}

qint64 Comms::getCurrentTime() const
{
    return currentTime;
}

void Comms::setCurrentTime(qint64 current_time)
{
    Comms::currentTime = current_time;
}

void Comms::getUserInfo()
{
    QNetworkRequest request(apiHelper->userInfoUrl());
    commsReplies.insert(request.url(), &Comms::userInfoReply);
    this->netRequest(request);
}

void Comms::userInfoReply(QByteArray buffer)
{
    QJsonDocument itemDoc = QJsonDocument::fromJson(buffer);

    buffer.truncate(MAX_LOG_TEXT_LENGTH);
    qDebug() << "UserInfo Response: " << buffer;

    QJsonObject rootObject = itemDoc.object();
    Formatting::jsonObjValToInt(rootObject, QStringLiteral("user_id"), &user_id);
    Formatting::jsonObjValToInt(rootObject, QStringLiteral("root_group_id"), &root_group_id);
    Formatting::jsonObjValToInt(rootObject, QStringLiteral("primary_group_id"), &primary_group_id);

    settings.setValue(SETT_USER_ID, user_id);
    settings.setValue(SETT_ROOT_GROUP_ID, root_group_id);
    settings.setValue(SETT_PRIMARY_GROUP_ID, primary_group_id);
    settings.sync();
    qDebug() << "SETT user_id: " << settings.value(SETT_USER_ID).toInt();
    qDebug() << "SETT root_group_id: " << settings.value(SETT_ROOT_GROUP_ID).toInt();
    qDebug() << "SETT primary_group_id: " << settings.value(SETT_PRIMARY_GROUP_ID).toInt();
}

void Comms::getSettings()
{
//    primary_group_id = 134214;
    QString primaryGroupIdAsString = settings.value(SETT_PRIMARY_GROUP_ID).toString();

    QUrl groupSettingsUrl = apiHelper->groupSettingsUrl(primaryGroupIdAsString);

    QUrlQuery params = apiHelper->getDefaultApiParams();

    // dapp settings
    params.addQueryItem(QStringLiteral("name[]"), QStringLiteral("close_agent")); // bool: can close app?

    params.addQueryItem(QStringLiteral("name[]"), QStringLiteral("pause_tracking")); // int: can have "Private Time"? if so, then X limit

    params.addQueryItem(QStringLiteral("name[]"), QStringLiteral("idletime")); // int: how quick app goes into idle

    params.addQueryItem(QStringLiteral("name[]"), QStringLiteral("logoffline")); // bool: show "away popup"?
    params.addQueryItem(QStringLiteral("name[]"), QStringLiteral("logofflinemin")); // int: after how much of idle we start showing "away popup"

    params.addQueryItem(QStringLiteral("name[]"), QStringLiteral("offlineallow")); // // bool: is the offlinecustom Array set
    params.addQueryItem(QStringLiteral("name[]"), QStringLiteral("offlinecustom")); // Array(String): names of activities for "away popup"

    params.addQueryItem(QStringLiteral("name[]"), QStringLiteral("dontCollectComputerActivity")); // bool: BOOL_COLLECT_COMPUTER_ACTIVITIES if true, send only "computer activity"
    params.addQueryItem(QStringLiteral("name[]"), QStringLiteral("collectWindowTitles")); // bool: save windowTitles?
    params.addQueryItem(QStringLiteral("name[]"), QStringLiteral("logOnlyActivitiesWithTasks")); // bool: tracking only when task selected
    params.addQueryItem(QStringLiteral("name[]"), QStringLiteral("make_screenshots")); // bool: take screenshots?

    params.addQueryItem(QStringLiteral("name[]"), QStringLiteral("group_working_time_limit")); // Array(int): stop tracking after "daily hours limit"

    // auto mode:
    params.addQueryItem(QStringLiteral("name[]"), QStringLiteral("tt_window_on_no_task")); // bool: "Display a window to choose a task, when Agent can't match any keyword in Auto Mode"
    params.addQueryItem(QStringLiteral("name[]"), QStringLiteral("turnoff_tt_after")); // int: auto tracking off, after X

    // probably server only:
//    params.addQueryItem("name[]", "form_scheduler"); // bool: "Allow users to log overtime activities"
//    params.addQueryItem("name[]", "unset_concurrent_apps"); // bool: "Dismiss computer activities overlapping other computer activities that are already logged"
//    params.addQueryItem("name[]", "limited_offline"); // bool: "Do not allow adding away time activity before first and after last activity on a computer"
//    params.addQueryItem("name[]", "disableDataSplit"); // bool: "can users split their away time breaks"


    groupSettingsUrl.setQuery(params.query());

//    qDebug() << "Query URL: " << groupSettingsUrl;

    QNetworkRequest request(groupSettingsUrl);

    commsReplies.insert(request.url(), &Comms::settingsReply);
    this->netRequest(request);
}

void Comms::settingsReply(QByteArray buffer)
{
    QJsonDocument itemDoc = QJsonDocument::fromJson(buffer);
    buffer.truncate(MAX_LOG_TEXT_LENGTH);
    qDebug() << "Settings Response: " << buffer;

    QJsonArray rootArray = itemDoc.array();
    for (QJsonValueRef val: rootArray) {
        QJsonObject obj = val.toObject();
//        qDebug() << obj.value("name").toString() << ": " << obj.value("value").toString();
        settings.setValue(QStringLiteral(SETT_WEB_SETTINGS_PREFIX) + obj.value(QStringLiteral("name")).toString(), obj.value(QStringLiteral("value")).toString()); // save web settings to our settingsstore
    }
    settings.sync();

    qDebug() << "SETT idletime: " << settings.value(QStringLiteral(SETT_WEB_SETTINGS_PREFIX) + QStringLiteral("idletime")).toInt();
    qDebug() << "SETT logoffline: " << settings.value(QStringLiteral(SETT_WEB_SETTINGS_PREFIX) + QStringLiteral("logoffline")).toBool();
    qDebug() << "SETT logofflinemin: " << settings.value(QStringLiteral(SETT_WEB_SETTINGS_PREFIX) + QStringLiteral("logofflinemin")).toInt();
    qDebug() << "SETT dontCollectComputerActivity: "
            << settings.value(QStringLiteral(SETT_WEB_SETTINGS_PREFIX) + QStringLiteral("dontCollectComputerActivity")).toBool();
    qDebug() << "SETT collectWindowTitles: "
            << settings.value(QStringLiteral(SETT_WEB_SETTINGS_PREFIX) + QStringLiteral("collectWindowTitles")).toBool();
}

void Comms::getTasks()
{
    QNetworkRequest request(apiHelper->tasksUrl());
    commsReplies.insert(request.url(), &Comms::tasksReply);
    this->netRequest(request);
}

void Comms::tasksReply(QByteArray buffer)
{
    QJsonDocument itemDoc = QJsonDocument::fromJson(buffer);

    buffer.truncate(MAX_LOG_TEXT_LENGTH);
    qDebug() << "Tasks Response: " << buffer;

    QJsonObject rootObject = itemDoc.object();
    for (auto oneTaskJSON: rootObject) {
        QJsonObject oneTask = oneTaskJSON.toObject();
        qint64 task_id = -1;
        Formatting::jsonObjValToInt(oneTask, QStringLiteral("task_id"), &task_id);
        QString name = oneTask.value(QStringLiteral("name")).toString();
        QString tags = oneTask.value(QStringLiteral("tags")).toString();
        auto * impTask = new Task(task_id);
        impTask->setName(name);
        impTask->setKeywords(tags);
        DbManager::instance().addToTaskList(impTask);
    }
}

void Comms::genericReply(QNetworkReply *reply)
{
    QByteArray buffer = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Network error: " << reply->errorString();
        qWarning() << "URL: " << reply->url();
        qWarning() << "TYPE: " << reply->operation();
        qWarning() << "Response: " << buffer;
        return;
    } else {
        qDebug() << "Network success";
    }

    QString stringUrl = reply->url().toString();
    // magic: if the stringUrl is in commsReplies Map(/Hash/Array) struct, then call the associated function
    auto &fn = commsReplies[stringUrl];
    if(fn) {
        fn(this, std::move(buffer));
    } else {
        emit gotGenericReply(reply, std::move(buffer));
    }
}

void Comms::netRequest(QNetworkRequest request, QNetworkAccessManager::Operation netOp, QByteArray data) // default params in Comms.h
{
    // make a copy of the request URL for the logger
    QString requestUrl = request.url().toString();
    requestUrl.truncate(MAX_LOG_TEXT_LENGTH);

    // follow redirects
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    // identify as our app
    request.setRawHeader("User-Agent", CONN_USER_AGENT);
    request.setRawHeader(CONN_CUSTOM_HEADER_NAME, CONN_CUSTOM_HEADER_VALUE);

    // create a reply object
    QNetworkReply *reply = nullptr;

    // make the actual request
    if(netOp == QNetworkAccessManager::GetOperation) {
        qDebug() << "[GET] URL: " << requestUrl;
        reply = qnam.get(request);
    }else if(netOp == QNetworkAccessManager::PostOperation) {
        qDebug() << "[POST] URL: " << requestUrl;
        reply = qnam.post(request, data);
        data.truncate(MAX_LOG_TEXT_LENGTH);
        qDebug() << "[POST] Data: " << data;
    }

    // if we got a reply, make sure it's finished; then queue it for deletion
    // processing is done in the "callback" function (conn1) via async
    if(reply != nullptr) {
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec(); // wait in this function till we get a Network Reply; callback from conn1 gets called in async manner

        reply->deleteLater();
    }
}

void Comms::postRequest(QUrl endpointUrl, QUrlQuery params)
{
    QNetworkRequest request(endpointUrl);

    QUrl URLParams;
    URLParams.setQuery(params);
    QByteArray jsonString = URLParams.toEncoded();
    QByteArray postDataSize = QByteArray::number(jsonString.size());

    // make it "www form" because thats what API expects; and add length
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    request.setRawHeader("Content-Length", postDataSize);

    this->netRequest(request, QNetworkAccessManager::PostOperation, jsonString);
}

void Comms::timerStart(qint64 taskID, qint64 entryID, qint64 startedAtInMS)
{
    QUrlQuery params = apiHelper->getDefaultApiParams();
    params.addQueryItem(QStringLiteral("action"), QStringLiteral("start"));
    if (taskID > 0) {
        params.addQueryItem(QStringLiteral("task_id"), QString::number(taskID));
    }
    if (entryID > 0) {
        params.addQueryItem(QStringLiteral("task_id"), QString::number(entryID));
    }
    if (startedAtInMS > 0) {
        params.addQueryItem(QStringLiteral("started_at"), Formatting::DateTimeTC(startedAtInMS));
    }
    this->postRequest(apiHelper->timerUrl(), params);
}

void Comms::timerStop(qint64 timerID, qint64 stoppedAtInMS)
{
    QUrlQuery params = apiHelper->getDefaultApiParams();
    params.addQueryItem(QStringLiteral("action"), QStringLiteral("stop"));
    if (timerID > 0) {
        params.addQueryItem(QStringLiteral("timer_id"), QString::number(timerID));
    }
    if (stoppedAtInMS > 0) {
        params.addQueryItem(QStringLiteral("stopped_at"), Formatting::DateTimeTC(stoppedAtInMS));
    }
    this->postRequest(apiHelper->timerUrl(), params);
}

void Comms::timerStatus()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (lastTimerStatusCheck - now < 1000) { // if called less than a second ago; prevents a check-spam
        return;
    }
    lastTimerStatusCheck = now;
    QUrlQuery params = apiHelper->getDefaultApiParams();
    params.addQueryItem(QStringLiteral("action"), QStringLiteral("status"));
    this->postRequest(apiHelper->timerUrl(), params);
}
