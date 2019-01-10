#ifndef TIMECAMPDESKTOP_KEYWORD_H
#define TIMECAMPDESKTOP_KEYWORD_H

#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include "KeywordUtils.h"

class Keyword
{
public:
//    Keyword(QString text, QRegularExpression re);
//    Keyword(const QString &text, const QRegularExpression &re);
    Keyword(QString &text, const QRegularExpression &re);

    bool isOfType(KeywordUtils::KeywordTypeFlag kwType);
    const QString &getText() const;
    int getFlags() const;
    int getValue() const;

    bool matches(QString data);

private:
    QString text;
    int flags;
    int value;
    QRegularExpression re;
};

#endif //TIMECAMPDESKTOP_KEYWORD_H
