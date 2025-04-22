// Copyright (C) 2023 gbabin
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

using namespace Qt::Literals::StringLiterals;

const QString Window::versionString = u"v1.7"_s;
const QString Window::websiteString = u"https://github.com/gbabin/bijectif"_s;
const char* Window::aboutString =
    QT_TRANSLATE_NOOP("Window",
                      "<b>Bijectif %1</b><br>"
                      "Edit titles in filenames<br>"
                      "<a href=\"%2\">%2</a><br>"
                      "Licensed under the terms of the GNU General Public License v3.0<br>"
                      "<br>"
                      "Thumbnails database:<br>"
                      "%3<br>"
                      "Settings file:<br>"
                      "%4<br>"
                      "<br>"
                      "<a href=\"https://www.flaticon.com/free-icon/map_1865269\">Map icon</a> "
                      "by <a href=\"https://www.flaticon.com/authors/freepik\">Freepik</a> "
                      "from <a href=\"https://www.flaticon.com/\">Flaticon.com</a>");

Window::Window(const Settings &settings, const QDir &directory, bool benchmark, QWidget *parent)
    : QMainWindow(parent)
    , settings(settings)
    , dir(directory)
    , benchmark(benchmark)
    , model(settings, &completerModel)
    , undoStack(new QUndoStack(this))
{
    setMinimumSize(settings.minWidth, settings.minHeight);

    QFont font = this->font();
    font.setPixelSize(settings.fontSize);

    // status bar

    statusBar()->setFont(font);

    QLabel* dirLabel = new QLabel(tr("Folder: %1").arg(QDir::toNativeSeparators(dir.path())));
    dirLabel->setFont(font);

    QLabel* versionLabel = new QLabel(versionString);
    versionLabel->setFont(font);

    statusBar()->addWidget(dirLabel);
    statusBar()->addPermanentWidget(versionLabel);

    // central widget

    view.setItemDelegate(new Delegate(&completerModel, undoStack, this));
    view.setFont(font);
    view.horizontalHeader()->setFont(font);
    view.verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    view.verticalHeader()->setDefaultSectionSize(settings.thumbnailSize + 4);
    view.setAlternatingRowColors(true);
    view.setWordWrap(true);

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

    QAction* sepAct1 = new QAction(u"|"_s, this);
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

    QAction* sepAct2 = new QAction(u"|"_s, this);
    sepAct2->setDisabled(true);
    menuBar()->addAction(sepAct2);

    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction* websiteAct = new QAction(tr("&Website"), this);
    connect(websiteAct, &QAction::triggered,
            this, []{ QDesktopServices::openUrl(QUrl(websiteString)); });
    helpMenu->addAction(websiteAct);

    QAction* aboutAct = new QAction(tr("&About"), this);
    connect(aboutAct, &QAction::triggered,
            this, [this]{ QMessageBox::about(this, tr("About"),
                                             tr(aboutString)
                                             .arg(versionString,
                                                  websiteString,
                                                  QDir::toNativeSeparators(getThumbnailsDatabasePath()).toHtmlEscaped(),
                                                  QDir::toNativeSeparators(this->settings.fileName).toHtmlEscaped())); });
    helpMenu->addAction(aboutAct);

    QAction* aboutQtAct = new QAction(tr("About &Qt"), this);
    connect(aboutQtAct, &QAction::triggered,
            this, &QApplication::aboutQt);
    helpMenu->addAction(aboutQtAct);

    menuBar()->setFont(font);
    helpMenu->setFont(font);

    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &Window::clipboardChanged);

    // dock

    if (settings.showCommandHistory) {
        QDockWidget *undoDockWidget = new QDockWidget;
        undoDockWidget->setWindowTitle(tr("Command history"));
        undoDockWidget->setWidget(new QUndoView(undoStack));
        addDockWidget(Qt::RightDockWidgetArea, undoDockWidget);
    }
}

Window::~Window()
{
    db.close();
}

QFileInfoList Window::listFiles(const QDir &dir)
{
    return dir.entryInfoList({// images
                              u"*.avif"_s,
                              u"*.bmp"_s,
                              u"*.exr"_s,
                              u"*.gif"_s,
                              u"*.heic"_s,
                              u"*.heif"_s,
                              u"*.jpg"_s,
                              u"*.jpeg"_s,
                              u"*.jxl"_s,
                              u"*.png"_s,
                              u"*.tif"_s,
                              u"*.tiff"_s,
                              u"*.webp"_s,
                              // raw
                              u"*.3fr"_s,
                              u"*.ari"_s,
                              u"*.arw"_s,
                              u"*.bay"_s,
                              u"*.braw"_s,
                              u"*.cap"_s,
                              u"*.cr2"_s,
                              u"*.cr3"_s,
                              u"*.cri"_s,
                              u"*.crw"_s,
                              u"*.dcr"_s,
                              u"*.dcs"_s,
                              u"*.dng"_s,
                              u"*.drf"_s,
                              u"*.eip"_s,
                              u"*.erf"_s,
                              u"*.fff"_s,
                              u"*.gpr"_s,
                              u"*.iiq"_s,
                              u"*.k25"_s,
                              u"*.kdc"_s,
                              u"*.mdc"_s,
                              u"*.mef"_s,
                              u"*.mos"_s,
                              u"*.mrw"_s,
                              u"*.nef"_s,
                              u"*.nrw"_s,
                              u"*.orf"_s,
                              u"*.pef"_s,
                              u"*.ptx"_s,
                              u"*.pxn"_s,
                              u"*.r3d"_s,
                              u"*.raf"_s,
                              u"*.raw"_s,
                              u"*.rw2"_s,
                              u"*.rwl"_s,
                              u"*.rwz"_s,
                              u"*.sr2"_s,
                              u"*.srf"_s,
                              u"*.srw"_s,
                              u"*.tco"_s,
                              u"*.x3f"_s,
                              // others
                              u"*.psd"_s,
                              u"*.svg"_s,
                              u"*.xcf"_s,
                              // videos
                              u"*.avi"_s,
                              u"*.mkv"_s,
                              u"*.mov"_s,
                              u"*.mp4"_s,
                              u"*.mpg"_s,
                              u"*.mts"_s,
                              u"*.vob"_s,
                              u"*.wmv"_s},
                             QDir::Files | QDir::Readable,
                             QDir::Name | QDir::LocaleAware);
}

QString Window::getThumbnailsDatabasePath() {
    const QDir dbDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    if (! dbDir.exists()) {
        if (! QDir().mkpath(dbDir.path())) {
            qWarning() << "Thumbnails database folder creation failed: " << dbDir;
        }
    }
    QString dbPath = dbDir.filePath(u"thumbnails.sqlite"_s);
    return dbPath;
}

void Window::modelLoadingStart()
{
    const QFileInfoList files = listFiles(dir);

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

    connect(&model, &Model::tooManyParts,
            this, &Window::modelTooManyParts);

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

    future.waitForFinished();

    if (benchmark)
        QApplication::exit();

    QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel(&view);
    model.setParent(proxyModel);
    proxyModel->setSourceModel(&model);
    proxyModel->sort(IdentifierCol, Qt::AscendingOrder);
    view.setModel(proxyModel);
    if (settings.resizableColumns) {
        for (int i = 0 ; i < FirstNameCol ; i++) {
            view.horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
        }
    } else {
        view.horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    }

    statusBar()->showMessage(tr("Load completed (Thumbnails database: %1)")
                                 .arg(locale().formattedDataSize(dbFileSize)),
                             3000);

    connect(view.selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &Window::selectionChanged);

    connect(view.model(), &QAbstractItemModel::dataChanged,
            this, &Window::dataChanged);

    db = QSqlDatabase::addDatabase(u"QSQLITE"_s);
    db.setDatabaseName(getThumbnailsDatabasePath());
    db.open();
}

void Window::modelTooManyParts(const QString &path)
{
    QMessageBox::critical(this,
                          tr("Too many name parts"),
                          tr("The file \"%1\" has too many parts in its name.\n"
                             "The maximum number allowed is %2.\n"
                             "It will been ignored.")
                              .arg(QDir::toNativeSeparators(path),
                                   QString::number(settings.maxNames)));
}

void Window::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected)
    Q_UNUSED(deselected)

    if (view.selectionModel() == nullptr)
        return;

    const QModelIndexList selectedIndexes = view.selectionModel()->selectedIndexes();

    if (selectedIndexes.isEmpty()) {
        pasteAct->setDisabled(true);
        copyAct->setDisabled(true);
        deleteAct->setDisabled(true);
        insertAct->setDisabled(true);
    } else {
        const bool anyId = std::any_of(selectedIndexes.cbegin(), selectedIndexes.cend(),
                                       [](const QModelIndex &index){ return index.column() == IdentifierCol; });

        const bool anyEmpty = std::any_of(selectedIndexes.cbegin(), selectedIndexes.cend(),
                                          [this](const QModelIndex &index){ return view.model()->data(index).isNull(); });

        QString mode = u"plain"_s;
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
                                                  return ! view.model()->data(index.siblingAtColumn(settings.maxNames + 1)).isNull(); }));
    }
}

void Window::clipboardChanged()
{
    selectionChanged(QItemSelection(), QItemSelection());
}

void Window::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)
    Q_UNUSED(roles)

    selectionChanged(QItemSelection(), QItemSelection());
}

void Window::doubleClickedHandler(const QModelIndex &index)
{
    if (index.column() == ThumbnailCol) {
        const QVariant data = index.data(ImagePathRole);
        if (data.userType() == QMetaType::QString) {
            const QString path = data.toString();
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    } else if (index.column() == MapCol) {
        const QVariant data = index.data(MapUrlRole);
        if (data.userType() == QMetaType::QUrl) {
            const QUrl url = data.toUrl();
            QDesktopServices::openUrl(url);
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
        QApplication::clipboard()->setText(names.join(u", "_s));
    }
}

void Window::pasteIntoSelection()
{
    QItemSelectionModel* selection = view.selectionModel();
    if (selection == nullptr) return;

    QString mode = u"plain"_s;
    QString text = QApplication::clipboard()->text(mode).trimmed();

    if (! text.isEmpty()) {
        static const QRegularExpression reCommaSpaces(u" *, *"_s);
        text.replace(reCommaSpaces, u","_s);
        QStringList texts = text.split(u","_s);

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
