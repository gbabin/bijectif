// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef WINDOW_H
#define WINDOW_H

#include "completermodel.h"
#include "memorycache.h"
#include "model.h"
#include "settings.h"
#include "viewer.h"

#include <QComboBox>
#include <QDir>
#include <QFuture>
#include <QMainWindow>
#include <QProgressDialog>
#include <QSqlDatabase>
#include <QTableView>
#include <QUndoStack>

namespace KDToolBox {
    class KDSignalDebouncer;
}
class QGVItem;
class QGVMap;
class QGVWidgetText;

class Window : public QMainWindow
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Window)

public:
    static const QString versionString;

    explicit Window(const Settings &settings, const QDir &directory, bool benchmark, QWidget *parent = nullptr);
    ~Window() override;

Q_SIGNALS:
    void viewImage(const QString& path);

public Q_SLOTS:
    void modelLoadingStart();
    void modelLoadingProgressed(int step);
    void modelLoadingDone(qint64 dbFileSize);
    void fileRenameFailed(const QString &before, const QString &after);
    void invalidCharacters();
    void invalidCharacterSequence();
    void identifiersMustBeUnique(const QString &id);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>());
    void copySelection();
    void pasteIntoSelection();
    void deleteSelection();
    void insertAtSelection();
    void selectFirst();
    void selectEnd();
    void mapSelectionChanged();
    void mapProviderChanged(int index);
    void mapTileReply(bool error, bool canceled, bool fromCache);

private Q_SLOTS:
    static void doubleClickedHandler(const QModelIndex &index);

private:
    static QString getThumbnailsDatabasePath();
    static const QString websiteString;
    static const char* aboutString;

    const Settings &settings;
    const QDir &directory;
    const bool benchmark;
    MemoryCache memoryCache;
    QProgressDialog* progressDialog = nullptr;
    QTableView view;
    Model model;
    CompleterModel completerModel;
    QFuture<void> future;
    QUndoStack* undoStack = nullptr;
    QDockWidget* mapDockWidget = nullptr;
    QGVMap* map = nullptr;
    QGVItem* mapItemBackground = nullptr;
    QGVWidgetText* mapItemCopyrightNotice = nullptr;
    QComboBox* mapSelector = nullptr;
    QLabel* mapStatistics = nullptr;
    int mapRepliesTotalCount = 0;
    int mapRepliesNewCount = 0;
    int mapRepliesCanceledCount = 0;
    int mapRepliesErrorCount = 0;
    int mapRepliesCacheCount = 0;
    KDToolBox::KDSignalDebouncer* mapSelectionDebouncer = nullptr;
    QDockWidget* viewerDockWidget = nullptr;
    Viewer* viewer = nullptr;
    QAction* copyAct = nullptr;
    QAction* pasteAct = nullptr;
    QAction* insertAct = nullptr;
    QAction* deleteAct = nullptr;
    QAction* firstAct = nullptr;
    QAction* endAct = nullptr;
    QSqlDatabase db;
};

#endif // WINDOW_H
