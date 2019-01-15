//
// Created by TimeCamp on 12/03/2018.
//

#include "TCWebEngineView.h"
#include "TCNavigationInterceptor.h"
#include <QMenu>
#include <QContextMenuEvent>

// Workaround to get opened URL from: https://bugreports.qt.io/browse/QTBUG-56637


TCWebEngineView::TCWebEngineView(QWidget *parent)
    : QWebEngineView(parent), result(nullptr),
      bannedActionsIds{
          QWebEnginePage::PasteAndMatchStyle, QWebEnginePage::PasteAndMatchStyle,
          QWebEnginePage::OpenLinkInNewBackgroundTab, QWebEnginePage::OpenLinkInThisWindow,
          QWebEnginePage::OpenLinkInNewWindow, QWebEnginePage::OpenLinkInNewTab, QWebEnginePage::DownloadLinkToDisk,
          QWebEnginePage::DownloadImageToDisk, QWebEnginePage::DownloadMediaToDisk, QWebEnginePage::InspectElement,
          QWebEnginePage::RequestClose, QWebEnginePage::SavePage, QWebEnginePage::OpenLinkInNewBackgroundTab,
          QWebEnginePage::ViewSource
} {
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
