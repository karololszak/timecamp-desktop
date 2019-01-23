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
    // we used to parse all API responses here
//    QString stringUrl = reply->url().toString();
//    if (stringUrl.contains(QLatin1String("/timer"))) {
//        // TODO: this needs some better checks
//        timerStatusReply(std::move(buffer));
//    }

    // now: we got a response for START or STOP to API
    // let's ask web browser to refresh it's status
    // and a callback will call timerStatusReply
    qDebug() << "We got some response for API - START or STOP, use WEB to check STATUS";
    emit askForWebTimerUpdate();
}

void TCTimer::timerStatusReply(QByteArray buffer)
{
    QJsonDocument itemDoc = QJsonDocument::fromJson(buffer);

    int bufLength = buffer.length();
    buffer.truncate(MAX_LOG_TEXT_LENGTH);
    qDebug() << "Timer Status Response length: " << bufLength;
//    qDebug() << "Timer Status Response [" << bufLength << "]: " << buffer;

    QJsonObject rootObject = itemDoc.object();

    isRunning = false;
    bool wasIsTimerRunningFound = true;
    QJsonValue isTimerRuninngJsonVal = rootObject.value(QStringLiteral("isTimerRunning"));
    if (isTimerRuninngJsonVal.isString()) {
        isRunning = isTimerRuninngJsonVal.toString().toLower() == QLatin1String("true");
    } else if (isTimerRuninngJsonVal.isDouble()) {
        isRunning = (bool) isTimerRuninngJsonVal.toInt();
    } else if (isTimerRuninngJsonVal.isBool()) {
        isRunning = isTimerRuninngJsonVal.toBool();
    } else {
        wasIsTimerRunningFound = false;
    }
    if (!wasIsTimerRunningFound) {
        isRunning = !rootObject.value(QStringLiteral("entry_time")).isDouble(); // entry_time is given only on stopTimer
    }

    // clear elapsed, task, etc - when timer wasn't running
    if (!isRunning) {
        clearData();
    }

    // then set / update everything with data from response

    Formatting::jsonObjValToInt(rootObject, QStringLiteral("elapsed"), &elapsed);
    Formatting::jsonObjValToInt(rootObject, QStringLiteral("task_id"), &task_id);
    Formatting::jsonObjValToInt(rootObject, QStringLiteral("entry_id"), &entry_id);
    Formatting::jsonObjValToInt(rootObject, QStringLiteral("timer_id"), &timer_id);
    Formatting::jsonObjValToInt(rootObject, QStringLiteral("external_task_id"), &external_task_id);

    QJsonValue nameJsonVal = rootObject.value(QStringLiteral("name"));
    if (nameJsonVal.isString()) {
        name = nameJsonVal.toString();
    }

    start_time = rootObject.value(QStringLiteral("start_time")).toString();

    QJsonValue newTimerIdVal = rootObject.value(QStringLiteral("new_timer_id"));
    if(newTimerIdVal.isDouble()) {
        timer_id = newTimerIdVal.toInt(); // set internal timer_id
        elapsed = 0; // we started new timer, but got info about old timers elapsed time... weird API
    }

    if (isRunning) {
        onTimerStartRoutine(nullptr, elapsed+1); // this will restart counting from 0, get names, etc
    }
    emit timerStatusChanged(isRunning, name);
}

void TCTimer::clearData()
{
    isRunning = false;
    elapsed = 0;
    task_id = 0;
    entry_id = 0;
    timer_id = 0;
    external_task_id = 0;
    name = QString();
    start_time = QString();
}

void TCTimer::mergedStartTimerSlots(qint64 taskID)
{
    task_id = taskID;
    comms->timerStart(task_id);
    onTimerStartRoutine(nullptr, 1); // "start timer" with 1 sec to show any time
}

void TCTimer::startTaskByTaskObj(Task *task, bool force)
{
    if (force || task_id != task->getTaskId()) {
        this->mergedStartTimerSlots(task->getTaskId());
    }
}

void TCTimer::startTaskByID(qint64 taskID, bool force)
{
    if (force || task_id != taskID) {
        this->mergedStartTimerSlots(taskID);
    }
}

void TCTimer::startTimerSlot()
{
    this->mergedStartTimerSlots(0);
}

void TCTimer::stopTimerSlot()
{
    comms->timerStop();
}

void TCTimer::startIfNotRunningYet()
{
    if (!isRunning) {
        this->mergedStartTimerSlots(0);
    }
}


void TCTimer::onTimerStartRoutine(Task* taskObj, qint64 fromStartElapsed)
{
    isRunning = true;
    QString previousName = name;

    if(taskObj == nullptr) { // if nothing was passed
        taskObj = DbManager::instance().getTaskById(task_id); // get obj from DB
    }

    if(taskObj != nullptr) { // if task was found in DB (or was passed and was not null)
        task_id = taskObj->getTaskId();
        name = taskObj->getName(); // set TimerName to TaskName
    }

    emit timerStatusChanged(isRunning, name);
    emit timerElapsedSeconds(fromStartElapsed);
}

