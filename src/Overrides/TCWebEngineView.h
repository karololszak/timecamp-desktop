#ifndef TIMECAMPDESKTOP_TCWEBENGINEVIEW_H
#define TIMECAMPDESKTOP_TCWEBENGINEVIEW_H

#include <QObject>
#include <QWebEngineView>

class TCWebEngineView : public QWebEngineView
{
Q_OBJECT

public:
    explicit TCWebEngineView(QWidget *parent = nullptr);
    ~TCWebEngineView() override;

protected:
    QWebEngineView *createWindow(QWebEnginePage::WebWindowType type) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    TCWebEngineView *result;
    QList<QWebEnginePage::WebAction> bannedActionsIds;
};


#endif //TIMECAMPDESKTOP_TCWEBENGINEVIEW_H
