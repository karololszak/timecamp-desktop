#include "TCRequestInterceptor.h"
#include "src/Settings.h"

#include <QDebug>

TCRequestInterceptor::TCRequestInterceptor(QObject *p) : QWebEngineUrlRequestInterceptor(p)
{

}

void TCRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    info.setHttpHeader(CONN_CUSTOM_HEADER_NAME, CONN_CUSTOM_HEADER_VALUE);
//    qDebug() << "[TCRequestInterceptor] URL: " << info.requestUrl();
    // if exactly the login page (so after logout)
    QStringList marketingPages{
        QStringLiteral("https://www.timecamp.com/"),
        QStringLiteral("https://www.timecamp.com/de/"),
        QStringLiteral("https://www.timecamp.com/fr/"),
        QStringLiteral("https://www.timecamp.pl/"),
        QStringLiteral("https://www.timecamp.com/es/"),
        QStringLiteral("https://www.timecamp.com/br/"),
        QStringLiteral("https://www.timecamp.com/ru/"),
        QStringLiteral("https://www.timecamp.com/it/"),
        QStringLiteral("https://www.timecamp.com/zh/"),
        QStringLiteral("https://www.timecamp.com/ar/"),
        };

    QString pageURL = info.requestUrl().toString();
    bool isTCMarketingPage = marketingPages.contains(pageURL, Qt::CaseInsensitive);

    if (isTCMarketingPage) {
        qDebug() << "[TCRequestInterceptor] URL: " << pageURL;
        qDebug() << "[TCRequestInterceptor] Redirect logout -> to login page";
        info.redirect(QUrl(LOGIN_URL));
    }
}
