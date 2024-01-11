// Copyright (C) 2023, 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "window.h"
#include "command_delete.h"
#include "command_insert.h"
#include "command_paste.h"
#include "delegate.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDockWidget>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSpacerItem>
#include <QStatusBar>
#include <QUndoView>
#include <QVBoxLayout>
#include <QtConcurrent>

static const char* versionString = "v1.3";
static const char* websiteString = "https://github.com/gbabin/bijectif";
static const char* aboutString =
    QT_TRANSLATE_NOOP("Window",
                      "Bijectif %1\n"
                      "Edit titles in filenames\n"
                      "%2\n"
                      "Licensed under the terms of the GNU General Public License v3.0\n"
                      "\n"
                      "Thumbnails database:\n"
                      "%3");

Window::Window()
    : QMainWindow()
    , dir(QFileDialog::getExistingDirectory(nullptr,
                                            tr("Choose image folder")))
    , undoStack(new QUndoStack(this))
{
    if (! dir.exists()) exit(1);

    setMinimumSize(800, 600);

    QFont font = this->font();
    font.setPixelSize(16);

    // status bar

    statusBar()->setFont(font);

    QLabel* dirLabel = new QLabel(tr("Folder: %1").arg(QDir::toNativeSeparators(dir.path())));
    dirLabel->setFont(font);

    QLabel* versionLabel = new QLabel(versionString);
    versionLabel->setFont(font);

    statusBar()->addWidget(dirLabel);
    statusBar()->addPermanentWidget(versionLabel);

    // central widget

    view.setItemDelegate(new Delegate(undoStack, this));
    view.setFont(font);
    view.horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    view.horizontalHeader()->setFont(font);
    view.verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    view.verticalHeader()->setDefaultSectionSize(thumbnailSize+4);
    view.setAlternatingRowColors(true);

    connect(&view, &QTableView::doubleClicked,
            this, &Window::doubleClickedHandler);

    setCentralWidget(&view);

    // menu & actions

    QAction* undoAct = undoStack->createUndoAction(this, tr("&Undo"));
    undoAct->setShortcut(QKeySequence::Undo);
    menuBar()->addAction(undoAct);

    QAction* redoAct = undoStack->createRedoAction(this, tr("&Redo"));
    redoAct->setShortcut(QKeySequence::Redo);
    menuBar()->addAction(redoAct);

    QAction* sepAct1 = new QAction("|", this);
    sepAct1->setDisabled(true);
    menuBar()->addAction(sepAct1);

    copyAct = new QAction(tr("&Copy"), this);
    copyAct->setShortcut(QKeySequence::Copy);
    copyAct->setEnabled(false);
    connect(copyAct, &QAction::triggered,
            this, &Window::copySelection);
    menuBar()->addAction(copyAct);

    pasteAct = new QAction(tr("&Paste"), this);
    pasteAct->setShortcut(QKeySequence::Paste);
    pasteAct->setEnabled(false);
    connect(pasteAct, &QAction::triggered,
            this, &Window::pasteIntoSelection);
    menuBar()->addAction(pasteAct);

    insertAct = new QAction(tr("&Insert"), this);
    insertAct->setShortcut(Qt::Key_Insert);
    insertAct->setEnabled(false);
    connect(insertAct, &QAction::triggered,
            this, &Window::insertAtSelection);
    menuBar()->addAction(insertAct);

    deleteAct = new QAction(tr("&Delete"), this);
    deleteAct->setShortcut(QKeySequence::Delete);
    deleteAct->setEnabled(false);
    connect(deleteAct, &QAction::triggered,
            this, &Window::deleteSelection);
    menuBar()->addAction(deleteAct);

    QAction* sepAct2 = new QAction("|", this);
    sepAct2->setDisabled(true);
    menuBar()->addAction(sepAct2);

    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction* websiteAct = new QAction(tr("&Website"), this);
    connect(websiteAct, &QAction::triggered,
            this, []{ QDesktopServices::openUrl(QUrl(websiteString)); });
    helpMenu->addAction(websiteAct);

    QAction* aboutAct = new QAction(tr("&About"), this);
    connect(aboutAct, &QAction::triggered,
            this, [this]{ QMessageBox::about(this, "About",
                                             tr(aboutString)
                                             .arg(versionString,
                                                  websiteString,
                                                  QDir::toNativeSeparators(this->getThumbnailsDatabasePath()))); });
    helpMenu->addAction(aboutAct);

    QAction* aboutQtAct = new QAction(tr("About &Qt"), this);
    connect(aboutQtAct, &QAction::triggered,
            this, &QApplication::aboutQt);
    helpMenu->addAction(aboutQtAct);

    menuBar()->setFont(font);

    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &Window::clipboardChanged);

    // dock

    QDockWidget *undoDockWidget = new QDockWidget;
    undoDockWidget->setWindowTitle(tr("Commands"));
    undoDockWidget->setWidget(new QUndoView(undoStack));
    addDockWidget(Qt::RightDockWidgetArea, undoDockWidget);
}

Window::~Window()
{
    db.close();
}

QFileInfoList Window::listFiles(const QDir &dir)
{
    return dir.entryInfoList({"*.bmp", "*.gif", "*.jpg", "*.jpeg", "*.png", "*.tif", "*.tiff",
                              "*.dng", "*.nef", "*.rw2",
                              "*.psd", "*.svg",
                              "*.avi", "*.mkv", "*.mov", "*.mp4", "*.mpg", "*.mts", "*.vob", "*.wmv"},
                             QDir::Files | QDir::Readable,
                             QDir::Name | QDir::LocaleAware);
}

QString Window::getThumbnailsDatabasePath() {
    QDir dbDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    if (! dbDir.exists()) {
        if (! QDir().mkpath(dbDir.path())) {
            qWarning() << "Thumbnails database folder creation failed: " << dbDir;
        }
    }
    QString dbPath = dbDir.filePath("thumbnails.sqlite");
    return dbPath;
}

void Window::modelLoadingStart()
{
    QFileInfoList files = listFiles(dir);

    if (files.isEmpty()) return;

    progressDialog = new QProgressDialog(tr("Loading thumbnails..."), tr("Cancel"), 0, 11 * files.count(), this);
    progressDialog->setMinimumDuration(2000);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setAttribute(Qt::WA_DeleteOnClose);
    progressDialog->setCancelButton(nullptr);
    progressDialog->setValue(0);

    connect(&model, &Model::loadingProgressed,
            this, &Window::modelLoadingProgressed);

    connect(&model, &Model::loadingFinished,
            this, &Window::modelLoadingDone);

    future = QtConcurrent::run([this, files] {
        model.load(files, getThumbnailsDatabasePath());
    });
}

void Window::modelLoadingProgressed(int step)
{
    progressDialog->setValue(progressDialog->value() + step);
}

void Window::modelLoadingDone(qint64 dbFileSize)
{
    progressDialog->cancel();

    try {
        future.waitForFinished();
    } catch (TooManyPartsError &e) {
        QMessageBox::critical(nullptr,
                              tr("Too many name parts"),
                              tr("The file \"%1\" has too many parts in its name.\n"
                                 "The maximum number allowed is %2.")
                                  .arg(e.path, QString::number(Model::maxNames)));
        exit(2);
    }

    QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel(&view);
    model.setParent(proxyModel);
    proxyModel->setSourceModel(&model);
    proxyModel->sort(1, Qt::AscendingOrder);
    view.setModel(proxyModel);

    if (dbFileSize < 0) {
        statusBar()->showMessage(tr("Load completed"),
                                 3000);
    } else {
        statusBar()->showMessage(tr("Load completed (Thumbnails database: %1)")
                                     .arg(locale().formattedDataSize(dbFileSize)),
                                 3000);
    }

    connect(view.selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &Window::selectionChanged);

    connect(view.model(), &QAbstractItemModel::dataChanged,
            this, &Window::dataChanged);

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(getThumbnailsDatabasePath());
    db.open();
}

void Window::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected);
    Q_UNUSED(deselected);

    if (! view.selectionModel()) return;

    const QModelIndexList selectedIndexes = view.selectionModel()->selectedIndexes();

    if (selectedIndexes.isEmpty()) {
        pasteAct->setDisabled(true);
        copyAct->setDisabled(true);
        deleteAct->setDisabled(true);
        insertAct->setDisabled(true);
    } else {
        bool anyId = std::any_of(selectedIndexes.cbegin(), selectedIndexes.cend(),
                                 [](const QModelIndex &index){ return index.column() == 1; });

        bool anyEmpty = std::any_of(selectedIndexes.cbegin(), selectedIndexes.cend(),
                                    [this](const QModelIndex &index){ return view.model()->data(index).isNull(); });

        QString mode = "plain";
        pasteAct->setEnabled((! QApplication::clipboard()->text(mode).trimmed().isEmpty())
                             && (selectedIndexes.size() == 1
                                 || !anyId));

        copyAct->setEnabled((selectedIndexes.size() == 1
                             && ! view.model()->data(selectedIndexes.first()).isNull())
                            || (selectedIndexes.size() > 1
                                && !anyId
                                && !anyEmpty));

        deleteAct->setDisabled(anyEmpty || anyId);

        insertAct->setDisabled(anyEmpty
                               || anyId
                               || std::any_of(selectedIndexes.cbegin(), selectedIndexes.cend(),
                                              [this](const QModelIndex &index){
                                                  return ! view.model()->data(index.siblingAtColumn(Model::maxNames + 1)).isNull(); }));
    }
}

void Window::clipboardChanged()
{
    selectionChanged(QItemSelection(), QItemSelection());
}

void Window::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    Q_UNUSED(topLeft);
    Q_UNUSED(bottomRight);
    Q_UNUSED(roles);

    selectionChanged(QItemSelection(), QItemSelection());
}

void Window::doubleClickedHandler(const QModelIndex &index)
{
    if (index.column() == 0) {
        const QVariant data = index.data(ImagePathRole);
        if (data.userType() == QMetaType::QString) {
            const QString path = data.toString();
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    }
}

void Window::copySelection()
{
    QApplication::clipboard()->clear();

    QItemSelectionModel* selection = view.selectionModel();
    if (selection == nullptr) return;

    if (selection->selectedIndexes().size() == 1) {
        const QModelIndex index = selection->selectedIndexes().first();
        if (index.column() > 0) {
            const QVariant data = index.data(Qt::DisplayRole);
            if (data.userType() == QMetaType::QString) {
                QApplication::clipboard()->setText(data.toString());
            }
        }
    }
    else if (selection->selectedIndexes().size() > 1) {
        QModelIndexList indexes = selection->selectedIndexes();
        std::sort(indexes.begin(), indexes.end(),
                  [](const QModelIndex& a, const QModelIndex& b) {
                      return a.column() < b.column()
                             || (a.column() == b.column() && a.row() < b.row()); });
        QStringList names;
        for (const QModelIndex index : std::as_const(indexes)) {
            if (index.column() > 1) {
                const QVariant data = index.data(Qt::DisplayRole);
                if (data.userType() == QMetaType::QString) {
                    names.append(data.toString());
                }
            }
        }
        QApplication::clipboard()->setText(names.join(", "));
    }
}

void Window::pasteIntoSelection()
{
    QItemSelectionModel* selection = view.selectionModel();
    if (selection == nullptr) return;

    QString mode = "plain";
    QString text = QApplication::clipboard()->text(mode).trimmed();

    if (! text.isEmpty()) {
        static const QRegularExpression reCommaSpaces(" *, *");
        text.replace(reCommaSpaces, ",");
        QStringList texts = text.split(",");

        if (texts.size() == 1) {
            QModelIndexList indexes = selection->selectedIndexes();
            std::sort(indexes.begin(), indexes.end(),
                      [](const QModelIndex& a, const QModelIndex& b) {
                          return a.row() < b.row()
                                 || (a.row() == b.row() && a.column() < b.column()); });

            for (const QModelIndex index : std::as_const(indexes)) {
                undoStack->push(new PasteCommand(text, view.model(), index));
            }
        }
        else if (texts.size() > 1) {
            if (selection->selectedIndexes().size() % texts.size() != 0) {
                QMessageBox::information(this,
                                         tr("Pasting multiple cells"),
                                         tr("The pasted content has %1 parts. Therefore, each modifed row must have %1 selected cells.")
                                             .arg(texts.size()));
                return;
            }

            QModelIndexList indexes = selection->selectedIndexes();
            std::sort(indexes.begin(), indexes.end(),
                      [](const QModelIndex& a, const QModelIndex& b) {
                          return a.row() < b.row()
                                 || (a.row() == b.row() && a.column() < b.column()); });
            for (int i = 0 ; i < indexes.size() ; i++) {
                if (indexes[i].row() != indexes[i - i % texts.size()].row()) {
                    QMessageBox::information(this,
                                             tr("Pasting multiple cells"),
                                             tr("The pasted content has %1 parts. Therefore, each modifed row must have %1 selected cells.")
                                                 .arg(texts.size()));
                    return;
                }
            }
            for (int i = 0 ; i < indexes.size() ; i++) {
                undoStack->push(new PasteCommand(texts[i % texts.size()], view.model(), indexes[i]));
            }
        }
    }
}

void Window::deleteSelection()
{
    QItemSelectionModel* selection = view.selectionModel();
    if (selection == nullptr) return;

    QModelIndexList indexes = selection->selectedIndexes();
    std::sort(indexes.begin(), indexes.end(),
              [](const QModelIndex& a, const QModelIndex& b) {
                  return a.row() < b.row()
                         || (a.row() == b.row() && a.column() > b.column()); });

    for (const QModelIndex index : std::as_const(indexes)) {
        undoStack->push(new DeleteCommand(view.model(), index));
    }
}

void Window::insertAtSelection()
{
    QItemSelectionModel* selection = view.selectionModel();
    if (selection == nullptr) return;

    const QString text = QInputDialog::getText(this, tr("Insertion"), tr("Name:"));
    if (text.isEmpty()) return;

    QModelIndexList indexes = selection->selectedIndexes();
    std::sort(indexes.begin(), indexes.end(),
              [](const QModelIndex& a, const QModelIndex& b) {
                  return a.row() < b.row()
                         || (a.row() == b.row() && a.column() > b.column()); });

    for (const QModelIndex index : std::as_const(indexes)) {
        undoStack->push(new InsertCommand(text, view.model(), index));
    }
}
