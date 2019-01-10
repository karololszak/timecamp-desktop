#include "Formatting.h"
#include <QtCore/QDateTime>

QString Formatting::DateTimeTC(qint64 msecs)
{
    return QDateTime::fromMSecsSinceEpoch(msecs).toString(Qt::ISODate).replace("T", " ");
}

void Formatting::jsonObjValToInt(const QJsonObject jsonObj, QString valName, qint64 *value)
{
    QJsonValue jsonVal = jsonObj.value(valName);
    if (jsonVal.isDouble()) { // JSON numbers are always "double" in QJson
        *value = jsonVal.toInt();
    } else if (jsonVal.isString()) {
        *value = jsonVal.toString().toInt();
    } else if (jsonVal.isBool()) {
        *value = (int) jsonVal.toBool();
    }
}
