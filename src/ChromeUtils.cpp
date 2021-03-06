//
//  ChromeUtils.cpp
//  TimeCamp
//
//  Created by Aleksey Dvoryanskiy on 20.06.16
//  Ported to Qt by Karol Olszacki on 19.04.18
//

#include "ChromeUtils.h"

#include <QStandardPaths>
#include <QFileInfo>
#include <QDebug>
#include <QTextCodec>


QString getChromeSessionFilePath() {
#ifdef Q_OS_LINUX
    QString homeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).constFirst();
#elif defined(Q_OS_WIN)
    QString homeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).constFirst();
#else
    QString homeDir = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).constFirst();
#endif

    QString currentSessionPath = homeDir + "/.config/google-chrome/Default/Current Session";

    return currentSessionPath;
}

QString getChromeTabsFilePath() {
#ifdef Q_OS_LINUX
    QString homeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).constFirst();
#elif defined(Q_OS_WIN)
    QString homeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).constFirst();
#else
    QString homeDir = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).constFirst();
#endif

    QString currentTabsPath = homeDir + "/.config/google-chrome/Default/Current Tabs";

    return currentTabsPath;
}

QString getCurrentURLFromChromeConfig(const QByteArray &data, const QString &windowTitle) {
    // try to read data the best way possible
    QString snss = QTextCodec::codecForName("UTF-16")->toUnicode(data);

    // remove some confusing strings
    snss = snss.remove(QStringLiteral("\r\n"));
    snss = snss.remove('\r');
    snss = snss.remove('\n');

    QString urlTitle = windowTitle;
    urlTitle = urlTitle.replace(QLatin1String("- Google Chrome"), QLatin1String("")); // cut browser name
    urlTitle = urlTitle.trimmed();

    // super extensive searching, removing some troublesome characters
    // might still not be enough :(
    int pos;
    pos = snss.indexOf(urlTitle);
    if (pos == -1) {
        snss = snss.remove('|');
        urlTitle = urlTitle.remove('|');
        urlTitle = urlTitle.trimmed();
        pos = snss.indexOf(urlTitle);
        if (pos == -1) {
            snss = snss.remove(QStringLiteral("-"));
            snss = snss.remove(QStringLiteral("−"));
            snss = snss.remove(QStringLiteral("–"));
            snss = snss.remove(QStringLiteral("—"));
            snss = snss.remove(QStringLiteral("―"));

            urlTitle = urlTitle.remove(QStringLiteral("-"));
            urlTitle = urlTitle.remove(QStringLiteral("−"));
            urlTitle = urlTitle.remove(QStringLiteral("–"));
            urlTitle = urlTitle.remove(QStringLiteral("—"));
            urlTitle = urlTitle.remove(QStringLiteral("―"));
            urlTitle = urlTitle.trimmed();
            pos = snss.indexOf(urlTitle);
            if (pos == -1) {
                qInfo("[ChromeURL] Failed to find url for given title");
                return QString();
            }
        }
    }

    QStringRef truncatedRef = snss.midRef(pos);
    int urlPos = truncatedRef.indexOf(QStringLiteral("http"));

    if (urlPos == -1) {
        qInfo("[ChromeURL] Failed to find 'http' after website name");
        return QString();
    }

    // convert to latin1 - it strips most of weird unicode chars, leaves proper URI stuff
    QString truncatedStr = truncatedRef.mid(urlPos).toLatin1();

    return truncatedStr.trimmed();
}

QByteArray readChromeFile(const QString &filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return "";
    }

    QByteArray blob = file.readAll();
    return blob;
}

QString getCurrentURLFromChrome(const QString windowTitle) {
    QString chromeSessionFilePath = getChromeSessionFilePath();
    QByteArray sessionContent = readChromeFile(chromeSessionFilePath);
    QString activeURL = QString();
    if (!sessionContent.isEmpty()) {
        activeURL = getCurrentURLFromChromeConfig(sessionContent, windowTitle).trimmed();
    }
    if (activeURL.isEmpty()) {
        QString chromeTabsFilePath = getChromeTabsFilePath();
        QByteArray tabsContent = readChromeFile(chromeTabsFilePath);
        activeURL = getCurrentURLFromChromeConfig(tabsContent, windowTitle).trimmed();
    }

//    qDebug() << "[ChromeUtils::getCurrentURLFromChrome] Chrome active URL: " << activeURL;

    return activeURL;
}
