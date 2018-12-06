#include "Settings.h"
#include "ApiHelper.h"
#include <QDebug>

ApiHelper::ApiHelper(const QString &baseUrl)
    : baseUrl(baseUrl)
{}

void ApiHelper::setApiKey(const QString &apiKey)
{
    ApiHelper::apiKey = apiKey;
}

bool ApiHelper::updateApiKeyFromSettings()
{
    apiKey = settings.value(SETT_APIKEY).toString().trimmed();

    if (apiKey.isEmpty() || apiKey == "false") {
        qInfo() << "[EMPTY API KEY !!!]";
        return false;
    }
    return true;
}

QUrlQuery ApiHelper::getDefaultApiParams()
{
    QUrlQuery params = QUrlQuery();
    params.addQueryItem("api_token", apiKey);
    params.addQueryItem("service", SETT_API_SERVICE_FIELD);
    params.addQueryItem("app_version", APPLICATION_VERSION);
    params.addQueryItem("operating_system", QSysInfo::prettyProductName());

    return params;
}

QUrl ApiHelper::getApiUrl(QString endpoint, QString format)
{
    QString URL = baseUrl + endpoint + "/api_token/" + apiKey;
    if (!format.isEmpty()) {
        URL += "/format/" + format;
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

QUrl ApiHelper::groupSettingsUrl(QString groupId)
{
    return this->getApiUrl(QString(ApiEndpoint::GROUP) + QString("/") + groupId + "/setting", ApiFormat::JSON);
}

QUrl ApiHelper::tasksUrl()
{
    return this->getApiUrl(ApiEndpoint::TASKS, ApiFormat::JSON);
}

QUrl ApiHelper::activitiesUrl()
{
    return this->getApiUrl(ApiEndpoint::ACTIVITY, ApiFormat::JSON);
}
