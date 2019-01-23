#include "TCTimer.h"
#include "Formatting.h"

TCTimer::TCTimer(Comms *comms)
    : comms(comms)
{
    clearData();

    // connect the generic reply from Comms
    QObject::connect(comms, &Comms::gotGenericReply, this, &TCTimer::decideTimerReply);
}

void TCTimer::decideTimerReply(QNetworkReply *reply, QByteArray buffer)
{
    // this reply can be caught and forwarded from the built-in WebBrowser and we can still parse it here
    QString stringUrl = reply->url().toString();
    if (stringUrl.contains("/timer")) {
        // TODO: this needs some better checks
        timerStatusReply(std::move(buffer));
    }
}

void TCTimer::timerStatusReply(QByteArray buffer)
{
    bool previousIsRunning = isRunning;
    QString previousName = name;
    QJsonDocument itemDoc = QJsonDocument::fromJson(buffer);

    buffer.truncate(MAX_LOG_TEXT_LENGTH);
    qDebug() << "Timer Status Response: " << buffer;

    QJsonObject rootObject = itemDoc.object();
    QJsonValue isTimerRuninngJsonValue = rootObject.value("isTimerRunning");
    isRunning = (isTimerRuninngJsonValue.isString() && (isTimerRuninngJsonValue.toString() == "true"));
    isRunning = (isTimerRuninngJsonValue.isBool() && isTimerRuninngJsonValue.toBool());
    if (isRunning) {
        QJsonValue elapsedJsonVal = rootObject.value("elapsed");
        if (elapsedJsonVal.isString()) {
            elapsed = elapsedJsonVal.toString().toInt();
        } else {
            elapsed = elapsedJsonVal.toInt();
        }
        task_id = rootObject.value("task_id").toString().toInt();
        entry_id = rootObject.value("entry_id").toString().toInt();
        timer_id = rootObject.value("timer_id").toString().toInt();
        external_task_id = rootObject.value("external_task_id").toString().toInt();
        name = rootObject.value("name").toString();
        if(task_id != 0) {
            Task *taskObj = DbManager::instance().getTaskById(task_id);
            if(taskObj != nullptr) {
                name = taskObj->getName();
            }
        }
        start_time = rootObject.value("external_task_id").toString();
    } else {
        clearData();
    }
    QJsonValue newTimerId = rootObject.value("new_timer_id");
    if(!newTimerId.isUndefined() && !newTimerId.isNull()) {
        timer_id = newTimerId.toInt();
        elapsed = 1; // let's say it's been running for 1 sec, just to show some time
        isRunning = true;
    }

    if (previousIsRunning != isRunning || previousName != name) {
        emit timerStatusChanged(isRunning, name);
    }
    emit timerElapsedSeconds(elapsed);
}

void TCTimer::clearData()
{
    isRunning = false;
    elapsed = 0;
    task_id = 0;
    entry_id = 0;
    timer_id = 0;
    external_task_id = 0;
    name = QString("");
    start_time = QString("");
}

void TCTimer::startTaskByTaskObj(Task *task, bool force)
{
    if (force || timer_id == 0 || timer_id != task->getTaskId()) {
        comms->timerStart(task->getTaskId());
    }
}

void TCTimer::startTaskByID(qint64 taskID)
{
    comms->timerStart(taskID);
}

void TCTimer::startTimerSlot()
{
    comms->timerStart();
}

void TCTimer::stopTimerSlot()
{
    comms->timerStop();
}

void TCTimer::startIfNotRunningYet()
{
    if (!isRunning) {
        this->start();
    }
}
