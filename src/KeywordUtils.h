#ifndef TIMECAMPDESKTOP_KEYWORDUTILS_H
#define TIMECAMPDESKTOP_KEYWORDUTILS_H

#include <QString>
#include "Keyword.h"

class KeywordUtils
{
public:
    enum KeywordTypeFlag
    {
        T_OPTIONAL = 0,
        T_NEGATIVE = 1, // "The task will not be selected if any of its negative keywords is detected"
        T_MANDATORY = 2, // "The task will not be selected if all of its mandatory keywords are not detected"
        T_ONEOF = 4, // "At least one of these keywords must be detected to start a task"
        T_WEAK = 8, // instantly switch to other task when we don't have this keyword
        T_FULL_WORD = 16 // "The task will be selected only if given tag occurs in window title as a full word (and not part of it)" = no spaces
        // [*val] = replace tag value (usually tag length), with the number "val", eg. tag `[*7]qwe` will be more important than `qwerty`
    };

    static constexpr const char* T_EMPTY_STR = "";
    static constexpr const char* T_MANDATORY_STR = "[+]";
    static constexpr const char* T_WEAK_STR = "[0]";
    static constexpr const char* T_ONEOF_STR = "[1+]";
    static constexpr const char* T_NEGATIVE_STR = "[-]";
    static constexpr const char* T_FULL_WORD_STR = "[b]";

    // nicer documentation:
    // https://www.timecamp.com/kb/keywords/
};

#endif //TIMECAMPDESKTOP_KEYWORDUTILS_H
