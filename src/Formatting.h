#ifndef TIMECAMPDESKTOP_FORMATTING_H
#define TIMECAMPDESKTOP_FORMATTING_H

#include <QtCore/QString>
#include <QtCore/QJsonObject>

class Formatting
{
public:
    static QString DateTimeTC(qint64 msecs);
    static void jsonObjValToInt(QJsonObject jsonObj, QString valName, qint64 *value);
};


#endif //TIMECAMPDESKTOP_FORMATTING_H
