#ifndef TIMECAMPDESKTOP_AUTORUN_H
#define TIMECAMPDESKTOP_AUTORUN_H

#include "qautostart/qautostart.h"

class Autorun : public QObject
{
Q_OBJECT
Q_DISABLE_COPY(Autorun)

public:
    static Autorun &instance();
    ~Autorun() override = default;
    QAutoStart *getAutostart() const;

#ifdef Q_OS_LINUX
    void addLinuxIcon();
    void addLinuxStartMenuEntry();
#endif

protected:
    explicit Autorun(QObject *parent = nullptr);

private:
    QAutoStart *autostart;

#ifdef Q_OS_LINUX
    QString appImagePath;
    QString iconPath;
    bool isAppImage;
    QAutoStart *startMenuEntry;
public:
    QAutoStart *getStartMenuEntry() const;

#endif
};


#endif //TIMECAMPDESKTOP_AUTORUN_H
