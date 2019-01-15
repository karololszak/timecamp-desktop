#ifndef LOGUTILS_H
#define LOGUTILS_H

// courtesy of https://andydunkel.net/2017/11/08/qt_log_file_rotation_with_qdebug/
// slightly modified

#define LOGSIZE (1000 * 1000 * 50) //log size in bytes - 50Mb
#define LOGFILES 5

#include <QObject>
#include <QString>
#include <QDebug>
#include <QDate>
#include <QTime>
#include <QStandardPaths>

namespace LOGUTILS {
    bool initLogging();

    void myMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

}

#endif // LOGUTILS_H
