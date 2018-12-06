//
// Created by TimeCamp on 12/03/2018.
//

#include "TCWebEngineView.h"
#include "TCNavigationInterceptor.h"
#include <QMenu>
#include <QContextMenuEvent>

// Workaround to get opened URL from: https://bugreports.qt.io/browse/QTBUG-56637


TCWebEngineView::TCWebEngineView(QWidget *parent) : QWebEngineView(parent), result(nullptr)
{
    bannedActionsIds.push_back(QWebEnginePage::PasteAndMatchStyle);
    bannedActionsIds.push_back(QWebEnginePage::OpenLinkInNewBackgroundTab);
    bannedActionsIds.push_back(QWebEnginePage::OpenLinkInThisWindow);
    bannedActionsIds.push_back(QWebEnginePage::OpenLinkInNewWindow);
    bannedActionsIds.push_back(QWebEnginePage::OpenLinkInNewTab);
    bannedActionsIds.push_back(QWebEnginePage::DownloadLinkToDisk);
    bannedActionsIds.push_back(QWebEnginePage::DownloadImageToDisk);
    bannedActionsIds.push_back(QWebEnginePage::DownloadMediaToDisk);
    bannedActionsIds.push_back(QWebEnginePage::InspectElement);
    bannedActionsIds.push_back(QWebEnginePage::RequestClose);
    bannedActionsIds.push_back(QWebEnginePage::SavePage);
    bannedActionsIds.push_back(QWebEnginePage::OpenLinkInNewBackgroundTab);
    bannedActionsIds.push_back(QWebEnginePage::ViewSource);
}

TCWebEngineView::~TCWebEngineView()
{
    delete result;
}

QWebEngineView *TCWebEngineView::createWindow(QWebEnginePage::WebWindowType type)
{
    Q_UNUSED(type);
//    qDebug() << "createWindow called, returning NavigationRequestInterceptor";
    if (!result) {
        result = new TCWebEngineView();
        result->setPage(new TCNavigationInterceptor(this->page()));
    }
    return result;
}

void TCWebEngineView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = page()->createStandardContextMenu();
    const QList<QAction*> contextMenuActions = menu->actions();

    for (QWebEnginePage::WebAction webAction: bannedActionsIds) { // go over all banned actions
        for (QAction *action: contextMenuActions) { // pick an action from context menu
            if (page()->action(webAction) == action) { // if an action in default context menu is in "ban list"
                action->setVisible(false); // then hide it
            }
        }
    }

    menu->popup(event->globalPos());
}
