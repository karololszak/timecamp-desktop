#include "AppData.h"
#include "Settings.h"

#include <QUrl>
#include <utility>

AppData::AppData()
= default;

AppData::AppData(QString appName, QString windowName, QString additionalInfo)
{
    this->appName = std::move(appName);
    this->windowName = std::move(windowName);
    this->additionalInfo = std::move(additionalInfo);
}

const QString AppData::getAppName() const
{
    if (appName.isEmpty()) {
        return ACTIVITY_DEFAULT_APP_NAME; // explorer2
    }
    return appName;
}

void AppData::setAppName(const QString &appName)
{
    AppData::appName = appName;
}

const QString &AppData::getWindowName() const
{
    return windowName;
}

void AppData::setWindowName(const QString &windowName)
{
    AppData::windowName = windowName;
}
const QString &AppData::getAdditionalInfo() const
{
    return additionalInfo;
}
void AppData::setAdditionalInfo(const QString &additionalInfo)
{
    AppData::additionalInfo = additionalInfo;
}

const qint64 AppData::getStart() const
{
    return start;
}
void AppData::setStart(const qint64 start)
{
    AppData::start = start;
}

const qint64 AppData::getEnd() const
{
    return end;
}
void AppData::setEnd(const qint64 end)
{
    AppData::end = end;
}

const QString AppData::getDomainFromAdditionalInfo() const
{
    QUrl url(this->additionalInfo);
    return url.host();
}
