#ifndef TIMECAMPDESKTOP_AUTOTRACKING_H
#define TIMECAMPDESKTOP_AUTOTRACKING_H


#include <QObject>
#include <QString>
#include <QtCore/QVector>
#include "Task.h"
#include "Keyword.h"
#include "AppData.h"

class AutoTracking: public QObject
{
Q_OBJECT
    Q_DISABLE_COPY(AutoTracking)

    int taskUpdateThreshold = 30 * 1000; // in ms; prompt every X sec
    qint64 lastUpdate = 0;

protected:
    explicit AutoTracking(QObject *parent = nullptr);
//    checkOneKeyword(Keyword);

public:
    static AutoTracking &instance();
    qint64 getLastUpdate() const;

    Task *matchActivityToTaskKeywords(const AppData *app);

public slots:
    void checkAppKeywords(const AppData *app);
    void setLastUpdate(qint64 lastUpdate);

signals:
    void foundTask(qint64 taskId, bool force);

private:
    QRegularExpression regularExpression;
    QString appToDataString(const AppData *app);

    int taskScore;

    bool taskNeedsOneOf;
    int taskOneOfMatchedCount;

    bool skipTask;
    bool taskWasWeak;

    bool keywordFound;

    Task *bestTask;
    bool bestWasWeak;
    int bestTaskScore;

    void taskLoop(const QString &dataWithPotentialKeyword);

    void keywordLoop(const QString &dataWithPotentialKeyword, QStringList &taskKeywords);
};


#endif //TIMECAMPDESKTOP_AUTOTRACKING_H
