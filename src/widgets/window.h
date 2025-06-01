// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef WINDOW_H
#define WINDOW_H

#include "completermodel.h"
#include "model.h"
#include "settings.h"

#include <QDir>
#include <QFuture>
#include <QMainWindow>
#include <QProgressDialog>
#include <QSqlDatabase>
#include <QTableView>
#include <QUndoStack>

class QGVMap;

class Window : public QMainWindow
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Window)

public:
    static const QString versionString;

    explicit Window(const Settings &settings, const QDir &directory, bool benchmark, QWidget *parent = nullptr);
    ~Window() override;

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
    void mapSelectionChanged();

private Q_SLOTS:
    static void doubleClickedHandler(const QModelIndex &index);

private:
    static QString getThumbnailsDatabasePath();
    static const QString websiteString;
    static const char* aboutString;

    const Settings &settings;
    const QDir &directory;
    const bool benchmark;
    QProgressDialog* progressDialog = nullptr;
    QTableView view;
    Model model;
    CompleterModel completerModel;
    QFuture<void> future;
    QUndoStack* undoStack = nullptr;
    QDockWidget* mapDockWidget = nullptr;
    QGVMap* map = nullptr;
    QAction* copyAct = nullptr;
    QAction* pasteAct = nullptr;
    QAction* insertAct = nullptr;
    QAction* deleteAct = nullptr;
    QSqlDatabase db;
};

#endif // WINDOW_H
