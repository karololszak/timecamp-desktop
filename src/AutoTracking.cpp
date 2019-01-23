#include "AutoTracking.h"
#include <QtCore/QDateTime>
#include <QtCore/QSettings>
#include <QDebug>
#include "DbManager.h"
#include "Settings.h"
#include "Keyword.h"

AutoTracking &AutoTracking::instance()
{
    static AutoTracking _instance;
    return _instance;
}

void AutoTracking::checkAppKeywords(const AppData *app)
{

    QSettings settings;
    bool autoTracking = settings.value(SETT_TRACK_AUTO_SWITCH, false).toBool();
    if (autoTracking) {
        Task *matchedTask = this->matchActivityToTaskKeywords(app);
        if (matchedTask != nullptr) {
            emit foundTask(matchedTask->getTaskId(), false);
        }
    }
}

QString AutoTracking::appToDataString(const AppData *app)
{
    QString dataWithPotentialKeyword(app->getAppName());

    return dataWithPotentialKeyword.append(" ")
    .append(app->getWindowName()).append(" ")
    .append(app->getAdditionalInfo())
    .trimmed();
}

Task *AutoTracking::matchActivityToTaskKeywords(const AppData *app)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
//    // temporary: disable the 30s update threshold lag
//    if (now <= lastUpdate + taskUpdateThreshold) {
//        return nullptr; // if we're not past X minutes since last task update
//    }

    // insert AppData into a one big string
    QString dataWithPotentialKeyword = this->appToDataString(app);

    if (dataWithPotentialKeyword.isEmpty()) {
        return nullptr; // empty app
    }

    // reset best matches
    bestTask = nullptr;
    bestWasWeak = false;
    bestTaskScore = 0;

    // go over tasks (will set bestTask, etc)
    taskLoop(dataWithPotentialKeyword);

    if (bestTask != nullptr && !bestWasWeak) { // weak keyword == switch immediately -> don't push time change
        lastUpdate = now;
    }

    return bestTask;
}

void AutoTracking::taskLoop(const QString &dataWithPotentialKeyword)
{
    for (Task *task: DbManager::instance().getTaskList()) { // in every task
        QStringList taskKeywords = task->getKeywordsList(); // get the KWlist
        if (taskKeywords.isEmpty()) {
            continue; // skip Task if it has no keywords
        }
        // reset some flags and values (for each task)
        taskScore = 0;

        taskNeedsOneOf = false;
        taskOneOfMatchedCount = 0;

        skipTask = false;
        taskWasWeak = false;

        // check each keyword
        keywordLoop(dataWithPotentialKeyword, taskKeywords);

        // after passing by each keyword:
        if (taskNeedsOneOf && taskOneOfMatchedCount == 0) { // didn't find any of [+1] keywords
            skipTask = true;
        }

        if (skipTask) { // when mandatory was not found, negative was found, or none of [+1]s were found
            continue; // go to next task
        }

        if (taskScore > bestTaskScore) { // if it's a better task
            bestTask = task;
            bestWasWeak = taskWasWeak;
        }
    }
}

void AutoTracking::keywordLoop(const QString &dataWithPotentialKeyword, QStringList &taskKeywords)
{
    for (QString &keywordString: taskKeywords) {
        // this handles [*val] (value replacement)
        auto *kw = new Keyword(keywordString, regularExpression); // form an obj, it will scrape off the markers too

        if (kw->isOfType(KeywordUtils::T_ONEOF)) {
            taskNeedsOneOf = true; // any of keywords indicates that task needs some of them
        }

        // this handles T_FULL_WORD inside
        keywordFound = kw->matches(dataWithPotentialKeyword);

        if (!keywordFound && kw->isOfType(KeywordUtils::T_MANDATORY)) { // not found, but required
            skipTask = true;
            break; // stop checking KWs
        }

        if (!keywordFound) {
            continue; // no kw found, skip the rest of the loop
        }


        if (kw->isOfType(KeywordUtils::T_NEGATIVE)) { // found, but not allowed
            skipTask = true;
            break; // stop checking KWs
        }

        if (kw->isOfType(KeywordUtils::T_ONEOF)) {
            taskOneOfMatchedCount++;
        }

        if (kw->isOfType(KeywordUtils::T_WEAK)) {
            taskWasWeak = true;
        }

        taskScore += kw->getValue(); //increase task score by value (length or the overwritten one)
    }
}

AutoTracking::AutoTracking(QObject *parent)
    : QObject(parent)
{
    regularExpression = QRegularExpression(R"(^\[\*([0-9]+)\])");
}

qint64 AutoTracking::getLastUpdate() const
{
    return lastUpdate;
}

void AutoTracking::setLastUpdate(qint64 lastUpdate)
{
    AutoTracking::lastUpdate = lastUpdate;
}
