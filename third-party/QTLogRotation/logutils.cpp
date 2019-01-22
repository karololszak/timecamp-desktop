#include "logutils.h"

#include <ctime>
#include <iomanip>
#include <iostream>

#include <QTime>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QDir>
#include <QFileInfoList>
#include <QStringBuilder>

namespace LOGUTILS {
    static QString logFileName;
    static QString logFolderName;
    QString lastMessage;
    qint64 sameMessageCount = 0;

    void initLogFileName() {
        logFileName = QString(logFolderName + "/Log_%1__%2.txt")
            .arg(QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd")))
            .arg(QTime::currentTime().toString(QStringLiteral("hh_mm_ss_zzz")));
    }

    /**
     * @brief deletes old log files, only the last ones are kept
     */
    void deleteOldLogs() {
        QDir dir;
        dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
        dir.setSorting(QDir::Time | QDir::Reversed);
        dir.setPath(logFolderName);

        QFileInfoList list = dir.entryInfoList();
        if (list.size() <= LOGFILES) {
            return; //no files to delete
        } else {
            for (int i = 0; i < (list.size() - LOGFILES); i++) {
                QString path = list.at(i).absoluteFilePath();
                QFile file(path);
                file.remove();
            }
        }
    }

    bool initLogging() {
        // Create folder for logfiles if not exists
        logFolderName = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation).constFirst() + QStringLiteral("/logs");

        if (!QDir(logFolderName).exists()) {
            QDir().mkpath(logFolderName);
        }

        std::cout << "[LOG PATH] " << logFolderName.toStdString() << std::endl;

        deleteOldLogs(); //delete old log files
        initLogFileName(); //create the logfile name

        QFile outFile(logFileName);
        if (outFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            qInstallMessageHandler(LOGUTILS::myMessageHandler);
            return true;
        } else {
            return false;
        }
    }

    void myMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
        //check file size and if needed create new log!
        {
            QFile outFileCheck(logFileName);
            qint64 size = outFileCheck.size();

            if (size > LOGSIZE) //check current log size
            {
                deleteOldLogs();
                initLogFileName();
            }
        }

        QDateTime now = QDateTime::currentDateTime();
        QString txt = QLatin1String("[") + now.toString(QStringLiteral("HH:mm:ss.zzz"))+ QLatin1String("] ");
        switch (type) {
            case QtDebugMsg:
                txt += QStringLiteral("\x1b[0mDebug:\t%1").arg(msg);
                break;
            case QtInfoMsg:
                txt += QStringLiteral("\x1b[106;97mInfo:\x1b[1;96m\t%1\x1b[0m").arg(msg);
                break;
            case QtWarningMsg:
                txt += QStringLiteral("\x1b[103;97mWarn:\x1b[1;93m\t%1\x1b[0m").arg(msg);
                break;
            case QtCriticalMsg:
                txt += QStringLiteral("\x1b[101;97mCrit:\x1b[1;91m\t%1\x1b[0m").arg(msg);
                break;
            case QtFatalMsg:
                txt += QStringLiteral("\x1b[105;30mFatal:\x1b[1;95m\t%1\x1b[0m").arg(msg);
                break;
        }

        QFile outFile(logFileName);
        outFile.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream ts(&outFile);
        if(msg != lastMessage || sameMessageCount > 20) {
            if(sameMessageCount > 0){
                QString repeated;
                repeated.append("^ repeated x").append(QString::number(sameMessageCount));
                ts << repeated << endl;
                std::cout << repeated.toStdString() << std::endl;
            }
            ts << txt << endl;
            std::cout << txt.toStdString() << std::endl;
            sameMessageCount = 0;
        } else {
            sameMessageCount++;
        }
        lastMessage = msg;
    }
}
