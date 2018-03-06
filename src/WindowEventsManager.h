#ifndef THEGUI_WINDOWEVENTSMANAGER_H
#define THEGUI_WINDOWEVENTSMANAGER_H

#include "WindowEvents.h"
#include <QThread>

class WindowEventsManager : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(WindowEventsManager)

public:

    static WindowEventsManager &instance();
    explicit WindowEventsManager(QObject *parent = nullptr);
    virtual ~WindowEventsManager(){}
    WindowEvents *getCaptureEventsThread() const;

public slots:
    void startOrStopThread(bool startOrStop);

private:
    WindowEvents *captureEventsThread;
    void startThread();
    void stopThread();
};


#endif //THEGUI_WINDOWEVENTSMANAGER_H