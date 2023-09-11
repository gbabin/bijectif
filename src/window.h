// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef WINDOW_H
#define WINDOW_H

#include "model.h"

#include <QDir>
#include <QFuture>
#include <QMainWindow>
#include <QProgressDialog>
#include <QSqlDatabase>
#include <QTableView>
#include <QUndoStack>

class Window : public QMainWindow
{
    Q_OBJECT

public:
    static const int thumbnailSize = 128;
    static const int tooltipSize = 512;
    static QFileInfoList listFiles(const QDir &dir);

    Window();
    ~Window();

public slots:
    void modelLoadingStart();
    void modelLoadingProgressed(int step);
    void modelLoadingDone();
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>());
    void copySelection();
    void pasteIntoSelection();
    void deleteSelection();
    void insertAtSelection();

private slots:
    void doubleClickedHandler(const QModelIndex &index);

private:
    const QDir dir;
    QProgressDialog* progressDialog = nullptr;
    QTableView view;
    Model model;
    QFuture<void> future;
    QUndoStack* undoStack = nullptr;
    QAction* copyAct = nullptr;
    QAction* pasteAct = nullptr;
    QAction* insertAct = nullptr;
    QAction* deleteAct = nullptr;
    QSqlDatabase db;
};

#endif // WINDOW_H
