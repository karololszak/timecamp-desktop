#ifndef COMMS_H
#define COMMS_H

#include <QObject>
#include <QSettings>
#include <QNetworkReply>

#include "AppData.h"

class Comms : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(Comms)

public:

    static Comms &instance();
    explicit Comms(QObject *parent = nullptr);
    virtual ~Comms(){}

    void saveApp(AppData *app);
    void sendAppData(QList<AppData*> *appList);
    void getUserInfo();
    void getSettings();

    qint64 getCurrentTime() const;
    void setCurrentTime(qint64 current_time);
    void timedUpdates();

    bool isApiKeyOK();

private:
    AppData *lastApp;
    QSettings settings;
    qint64 lastSync;
    qint64 currentTime;
    QString apiKey;
    int user_id;
    int root_group_id;
    int primary_group_id;

signals:

public slots:
    void appDataReply(QNetworkReply *reply);
    void userInfoReply(QNetworkReply *reply);
};

#endif // COMMS_H
