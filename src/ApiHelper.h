#ifndef TIMECAMPDESKTOP_APIURL_H
#define TIMECAMPDESKTOP_APIURL_H

#include <QUrl>
#include <QUrlQuery>
#include <QString>
#include <QSettings>

class ApiHelper
{
private:
    QString baseUrl;
    QString apiKey;
    QSettings settings;
public:
    explicit ApiHelper(const QString &baseUrl);
    QUrl getApiUrl(QString endpoint, QString format = QLatin1String(""));
    QUrlQuery getDefaultApiParams();
    bool updateApiKeyFromSettings();
    void setApiKey(const QString &apiKey);
    QUrl timerUrl();
    QUrl userInfoUrl();
    QUrl tasksUrl();
    QUrl activitiesUrl();
    QUrl groupSettingsUrl(QString groupId);
};

class ApiEndpoint
{
public:
    static const constexpr char *TIMER = "/timer";
    static const constexpr char *USER = "/user";
    static const constexpr char *ACTIVITY = "/activity";
    static const constexpr char *GROUP = "/group";
    static const constexpr char *TASKS = "/tasks/exclude_archived/0"; // don't sync archived tasks
};


class ApiFormat
{
public:
    static const constexpr char *JSON = "json";
    static const constexpr char *XML = "xml";
};


#endif //TIMECAMPDESKTOP_APIURL_H
