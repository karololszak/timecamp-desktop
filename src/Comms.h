#ifndef COMMS_H
#define COMMS_H

#include <QObject>
#include <QSettings>
#include <QNetworkReply>

#include "AppData.h"
#include "Task.h"
#include "ApiHelper.h"

class Comms : public QObject
{
Q_OBJECT
    Q_DISABLE_COPY(Comms)

    const char* WEBSITES_APPNAME = "Internet";
    AppData *lastApp = nullptr;
    QSettings settings;
    qint64 lastSync;
    qint64 currentTime;
    QString apiKey;
    ApiHelper *apiHelper;
    qint64 lastTimerStatusCheck;
    int retryCount = 0;
    bool lastBatchBig = false;

    qint64 user_id;
    qint64 root_group_id;
    qint64 primary_group_id;
    QNetworkAccessManager qnam;
    QHash<QUrl, std::function<void(Comms *, QByteArray buffer)>> commsReplies; // see https://stackoverflow.com/a/7582574/8538394

public:

    static Comms &instance();
    ~Comms() override = default;

    void appChanged(AppData *app);
    void reportApp(AppData *app);
    void sendAppData(const QVector<AppData> *appList);
    void getUserInfo();
    void getSettings();
    void getTasks();

    qint64 getCurrentTime() const;
    void setCurrentTime(qint64 current_time);
    void timedUpdates();
    void tryToSendAppData();

    void netRequest(QNetworkRequest, QNetworkAccessManager::Operation = QNetworkAccessManager::GetOperation, QByteArray = nullptr);
    void postRequest(QUrl endpointUrl, QUrlQuery params);

signals:
    void DbSaveApp(AppData *);
    void announceAppChange(AppData *);
    void gotGenericReply(QNetworkReply *reply, QByteArray buffer);

protected:
    explicit Comms(QObject *parent = nullptr);

public slots:
    void appDataReply(QByteArray buffer);
    void userInfoReply(QByteArray buffer);
    void settingsReply(QByteArray buffer);
    void tasksReply(QByteArray buffer);
    void genericReply(QNetworkReply *reply);
    void checkBatchSize();
    void clearLastApp();
    void timerStart(qint64 taskID = 0, qint64 entryID = 0, qint64 startedAtInMS = 0);
    void timerStop(qint64 timerID = 0, qint64 stoppedAtInMS = 0);
    void timerStatus();
};

#endif // COMMS_H
