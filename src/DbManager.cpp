#include "DbManager.h"
#include "Settings.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>

DbManager &DbManager::instance()
{
    static DbManager _instance;
    return _instance;
}

DbManager::DbManager(QObject *parent)
    : QObject(parent)
{
    qInfo() << "[DB] Starting DB manager!";
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(DB_FILENAME);

    if (!m_db.open())
    {
        qInfo() << "[DB] Error: connection with database fail";
    }
    else
    {
        qDebug() << "[DB] Database: connection ok";
        createTable();
    }
}

DbManager::~DbManager()
{
    if (m_db.isOpen())
    {
        m_db.close();
    }
}

bool DbManager::isOpen() const
{
    return m_db.isOpen();
}

bool DbManager::createTable()
{
    bool success = false;

    QSqlQuery query;
    query.prepare("CREATE TABLE \"apps\" ( `ID` INTEGER PRIMARY KEY AUTOINCREMENT, `app_name` TEXT, `window_name` TEXT, `additional_info` TEXT, `start_time` INTEGER NOT NULL, `end_time` INTEGER NOT NULL )");

    if (!query.exec())
    {
        qDebug() << "Couldn't create the table: one might already exist.";
        success = false;
    }

    return success;
}

bool DbManager::saveAppToDb(AppData *app)
{
    bool success = false;
    QString appName = app->getAppName();
    QString windowName = app->getWindowName();
    QString additionalInfo = app->getAdditionalInfo();
    qint64 start = app->getStart();
    qint64 end = app->getEnd();

    if (start > 0 && end > 0)
    {
        QSqlQuery queryAdd;
        queryAdd.prepare("INSERT INTO apps (ID, app_name, window_name, additional_info, start_time, end_time) VALUES (NULL, ?, ?, ?, ?, ?)");
        queryAdd.addBindValue(appName);
        queryAdd.addBindValue(windowName);
        queryAdd.addBindValue(additionalInfo);
        queryAdd.addBindValue(start);
        queryAdd.addBindValue(end);

        if(queryAdd.exec())
        {
//            qDebug() << "[DB] app added successfully";
            success = true;
        }
        else
        {
            qInfo() << "[DB] adding failed: " << queryAdd.lastError();
        }
    }
    else
    {
        qInfo() << "[DB] adding failed: missing values!";
    }

    return success;
}

QList<AppData*> DbManager::getAppsSinceLastSync(qint64 last_sync)
{
    QSqlQuery querySelect;
    querySelect.prepare("SELECT app_name, window_name, additional_info, start_time, end_time FROM apps WHERE start_time > :lastSync");
    querySelect.bindValue(":lastSync", last_sync);

    QList<AppData*> appList;

    if(querySelect.exec())
    {
        while(querySelect.next())
        {
            AppData *tempApp = new AppData();
            tempApp->setAppName(querySelect.value("app_name").toString());
            tempApp->setWindowName(querySelect.value("window_name").toString());
            tempApp->setAdditionalInfo(querySelect.value("additional_info").toString());
            tempApp->setStart(querySelect.value("start_time").toLongLong());
            tempApp->setEnd(querySelect.value("end_time").toLongLong());
            appList.push_back(tempApp);
        }
    }
    return appList;
}