#ifndef THEGUI_WINDOWEVENTSMANAGER_H
#define THEGUI_WINDOWEVENTSMANAGER_H

#include "DataCollector/WindowEvents.h"

class WindowEventsManager : public QObject
{
Q_OBJECT
    Q_DISABLE_COPY(WindowEventsManager)

public:

    static WindowEventsManager &instance();

    ~WindowEventsManager() override = default;
    WindowEvents *getCaptureEventsThread() const;

signals:
    void dataCollectingStopped();
    void updateAfterAwayTime();
    void openAwayTimeManagement();

public slots:
    void startOrStopThread(bool startOrStop);
    void awayPopup(unsigned long);
    void startThread();
    void stopThread();

protected:
    explicit WindowEventsManager(QObject *parent = nullptr);

private:
    WindowEvents *captureEventsThread;
};


#endif //THEGUI_WINDOWEVENTSMANAGER_H
