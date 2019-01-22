#include "Settings.h"
#include "ApiHelper.h"
#include <QDebug>

ApiHelper::ApiHelper(const QString &baseUrl)
    : baseUrl(baseUrl)
{}

bool ApiHelper::updateApiKeyFromSettings()
{
    apiKey = settings.value(SETT_APIKEY).toString().trimmed();

    if (apiKey.isEmpty() || apiKey == QLatin1String("false")) {
        qInfo() << "[EMPTY API KEY !!!]";
        return false;
    }
    return true;
}

QUrlQuery ApiHelper::getDefaultApiParams()
{
    QUrlQuery params = QUrlQuery();
    params.addQueryItem(QStringLiteral("api_token"), apiKey);
    params.addQueryItem(QStringLiteral("service"), QStringLiteral(SETT_API_SERVICE_FIELD));
    params.addQueryItem(QStringLiteral("app_version"), QStringLiteral(APPLICATION_VERSION));
    params.addQueryItem(QStringLiteral("operating_system"), QSysInfo::prettyProductName());

    return params;
}

QUrl ApiHelper::getApiUrl(const QString endpoint, const QString format)
{
    QString URL = baseUrl + endpoint + QStringLiteral("/api_token/") + apiKey;
    if (!format.isEmpty()) {
        URL += QStringLiteral("/format/") + format;
    }
    return QUrl(URL);
}

QUrl ApiHelper::timerUrl()
{
    return this->getApiUrl(ApiEndpoint::TIMER, ApiFormat::JSON);
}

QUrl ApiHelper::userInfoUrl()
{
    return this->getApiUrl(ApiEndpoint::USER, ApiFormat::JSON);
}

QUrl ApiHelper::groupSettingsUrl(const QString groupId)
{
    return this->getApiUrl(QString(ApiEndpoint::GROUP) + QStringLiteral("/") + groupId + QStringLiteral("/setting"), ApiFormat::JSON);
}

QUrl ApiHelper::tasksUrl()
{
    return this->getApiUrl(ApiEndpoint::TASKS, ApiFormat::JSON);
}

QUrl ApiHelper::activitiesUrl()
{
    return this->getApiUrl(ApiEndpoint::ACTIVITY, ApiFormat::JSON);
}
