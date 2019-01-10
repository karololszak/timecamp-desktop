#include <QtCore/QRegularExpression>
#include "Keyword.h"

Keyword::Keyword(QString &text, const QRegularExpression &re)
    : re(re)
{
    this->flags = 0;
    this->value = 0;

    if (text.contains(KeywordUtils::T_MANDATORY_STR)) {
        text.replace(KeywordUtils::T_MANDATORY_STR, KeywordUtils::T_EMPTY_STR, Qt::CaseSensitivity::CaseInsensitive);
        this->flags |= KeywordUtils::T_MANDATORY;
    }

    if (text.contains(KeywordUtils::T_WEAK_STR)) {
        text.replace(KeywordUtils::T_WEAK_STR, KeywordUtils::T_EMPTY_STR, Qt::CaseSensitivity::CaseInsensitive);
        this->flags |= KeywordUtils::T_WEAK;
    }

    if (text.contains(KeywordUtils::T_ONEOF_STR)) {
        text.replace(KeywordUtils::T_ONEOF_STR, KeywordUtils::T_EMPTY_STR, Qt::CaseSensitivity::CaseInsensitive);
        this->flags |= KeywordUtils::T_ONEOF;
    }

    if (text.contains(KeywordUtils::T_NEGATIVE_STR)) {
        text.replace(KeywordUtils::T_NEGATIVE_STR, KeywordUtils::T_EMPTY_STR, Qt::CaseSensitivity::CaseInsensitive);
        this->flags |= KeywordUtils::T_NEGATIVE;
    }

    if (text.contains(KeywordUtils::T_FULL_WORD_STR)) {
        text.replace(KeywordUtils::T_FULL_WORD_STR, KeywordUtils::T_EMPTY_STR, Qt::CaseSensitivity::CaseInsensitive);
        this->flags |= KeywordUtils::T_FULL_WORD;
    }

    QRegularExpressionMatch match = re.match(text);
    if (match.hasMatch()) {
        QString matched = match.captured(0);
        bool wasOk = false;
        int newValue = matched.toInt(&wasOk);
        if (wasOk) {
            this->value = newValue;
        }
        text.replace(matched, KeywordUtils::T_EMPTY_STR, Qt::CaseSensitivity::CaseInsensitive);
    }
    if (this->value <= 0) {
        this->value = text.length();
    }

    this->text = text;
}

bool Keyword::isOfType(KeywordUtils::KeywordTypeFlag kwType)
{
    if (kwType != 0 && (this->flags & kwType) == kwType) {
        return true;
    } else {
        return kwType == this->flags;
    }
}

const QString &Keyword::getText() const
{
    return text;
}

int Keyword::getFlags() const
{
    return flags;
}

int Keyword::getValue() const
{
    return value;
}

bool Keyword::matches(QString data)
{
    if (this->isOfType(KeywordUtils::T_FULL_WORD)) {
        QString dataSorroundedWithSpaces = data;
        dataSorroundedWithSpaces.prepend(" ").append(" ");

        QString kwWithSpaces = this->getText();
        kwWithSpaces.prepend(" ").append(" ");

        return dataSorroundedWithSpaces.contains(kwWithSpaces, Qt::CaseInsensitive);
    }

    return data.contains(this->getText(), Qt::CaseInsensitive);
}
