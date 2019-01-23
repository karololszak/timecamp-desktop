#ifndef TIMECAMPDESKTOP_TCTIMER_H
#define TIMECAMPDESKTOP_TCTIMER_H

#include <QtCore>
#include "Comms.h"
#include "Task.h"
#include "DbManager.h"
#include "Settings.h"

class TCTimer : public QObject
{
Q_OBJECT
    Q_DISABLE_COPY(TCTimer)

private:
    Comms *comms;
    bool isRunning = false;
    qint64 elapsed = 0;
    qint64 task_id;
    qint64 entry_id;
    qint64 timer_id;
    qint64 external_task_id;
    QString name;
    QString start_time;
    void mergedStartTimerSlots(qint64 taskID);

public:
    explicit TCTimer(Comms *comms);
    void decideTimerReply(QNetworkReply *reply, QByteArray buffer);
    void timerStatusReply(QByteArray buffer);
    void clearData();
    void onTimerStartRoutine(Task *taskObj = nullptr, qint64 fromStartElapsed = 1);

signals:
    void timerStatusChanged(bool, QString);
    void timerElapsedSeconds(qint64);
    void askForWebTimerUpdate();

public slots:
    void startTaskByTaskObj(Task *task, bool force);
    void startTaskByID(qint64 taskID, bool force);
    void startTimerSlot();
    void stopTimerSlot();
    void startIfNotRunningYet();
};

#endif //TIMECAMPDESKTOP_TCTIMER_H
