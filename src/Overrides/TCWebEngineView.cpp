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
    auto it = std::find(contextMenuActions.cbegin(), contextMenuActions.cend(), page()->action(QWebEnginePage::ViewSource));
    if (it != contextMenuActions.cend()) {
        (*it)->setVisible(false);
    }
    QList<QAction*> bannedActions;
    bannedActions.push_back(page()->action(QWebEnginePage::PasteAndMatchStyle));
    bannedActions.push_back(page()->action(QWebEnginePage::OpenLinkInNewBackgroundTab));
    bannedActions.push_back(page()->action(QWebEnginePage::OpenLinkInThisWindow));
    bannedActions.push_back(page()->action(QWebEnginePage::OpenLinkInNewWindow));
    bannedActions.push_back(page()->action(QWebEnginePage::OpenLinkInNewTab));
    bannedActions.push_back(page()->action(QWebEnginePage::DownloadLinkToDisk));
    bannedActions.push_back(page()->action(QWebEnginePage::DownloadImageToDisk));
    bannedActions.push_back(page()->action(QWebEnginePage::DownloadMediaToDisk));
    bannedActions.push_back(page()->action(QWebEnginePage::InspectElement));
    bannedActions.push_back(page()->action(QWebEnginePage::RequestClose));
    bannedActions.push_back(page()->action(QWebEnginePage::SavePage));
    bannedActions.push_back(page()->action(QWebEnginePage::OpenLinkInNewBackgroundTab));
    bannedActions.push_back(page()->action(QWebEnginePage::ViewSource));
    bannedActions.push_back(page()->action(QWebEnginePage::ViewSource));

    for (QAction *action: contextMenuActions) {
        if (bannedActions.contains(action)) {
            action->setVisible(false);
        }
    }

    menu->popup(event->globalPos());
}
