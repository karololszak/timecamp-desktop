#include "Formatting.h"
#include <QtCore/QDateTime>

QString Formatting::DateTimeTC(qint64 msecs)
{
    return QDateTime::fromMSecsSinceEpoch(msecs).toString(Qt::ISODate).replace("T", " ");
}
