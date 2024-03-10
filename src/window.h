// Copyright (C) 2023, 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef WINDOW_H
#define WINDOW_H

#include "model.h"
#include "settings.h"

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
    Q_DISABLE_COPY_MOVE(Window)

public:
    static QFileInfoList listFiles(const QDir &dir);

    Window(const Settings &settings, QWidget *parent = nullptr);
    ~Window() override;

public slots:
    void modelLoadingStart();
    void modelLoadingProgressed(int step);
    void modelLoadingDone(qint64 dbFileSize);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void clipboardChanged();
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>());
    void copySelection();
    void pasteIntoSelection();
    void deleteSelection();
    void insertAtSelection();

private slots:
    static void doubleClickedHandler(const QModelIndex &index);

private:
    static QString getThumbnailsDatabasePath();
    static const QString versionString;
    static const QString websiteString;
    static const QString aboutString;

    const Settings &settings;
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
