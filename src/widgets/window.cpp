// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "window.h"

#include "command_delete.h"
#include "command_id.h"
#include "command_insert.h"
#include "command_paste.h"
#include "custommapxyz.h"
#include "delegate.h"
#include "filegroup.h"
#include "tablevieweventfilter.h"

#include <KDSignalThrottler.h>

#include <QGeoView/QGVLayerOSM.h>
#include <QGeoView/QGVMap.h>
#include <QGeoView/QGVWidgetScale.h>
#include <QGeoView/QGVWidgetText.h>
#include <QGeoView/QGVWidgetZoom.h>
#include <QGeoView/Raster/QGVIcon.h>

#include <QApplication>
#include <QClipboard>
#include <QCompleter>
#include <QDesktopServices>
#include <QDockWidget>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QNetworkDiskCache>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QSpacerItem>
#include <QSqlError>
#include <QStandardPaths>
#include <QStatusBar>
#include <QUndoView>
#include <QVBoxLayout>
#include <QtConcurrentRun>

using namespace Qt::Literals::StringLiterals;

const QString Window::versionString = u"v1.8"_s;
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
                      "Map from <a href=\"https://www.openstreetmap.org/copyright\">OpenStreetMap</a><br>"
                      "<br>"
                      "<a href=\"https://www.flaticon.com/free-icon/map_1865269\">Map</a> "
                      "and <a href=\"https://www.flaticon.com/free-icon/placeholder_684908\">placeholder</a> icons "
                      "by <a href=\"https://www.flaticon.com/authors/freepik\">Freepik</a> "
                      "from <a href=\"https://www.flaticon.com/\">Flaticon.com</a><br>"
                      "<br>"
                      "This application uses <a href=\"https://github.com/KDABLabs/KDToolBox/tree/master/qt/KDSignalThrottler\">KDSignalThrottler</a> "
                      "from <a href=\"https://github.com/KDABLabs/KDToolBox\">KDToolBox</a>, "
                      "© 2020 Klarälvdalens Datakonsult AB, a KDAB Group company &lt;info@kdab.com&gt;, used under the MIT License.<br>"
                      "See the full license in the <a href=\"https://github.com/KDABLabs/KDToolBox/blob/master/LICENSES/MIT.txt\">MIT.txt</a> file.");

Window::Window(const Settings &settings, const QDir &directory, bool benchmark, QWidget *parent)
    : QMainWindow(parent)
    , settings(settings)
    , directory(directory)
    , benchmark(benchmark)
    , memoryCache(settings.memoryCacheSize)
    , model(settings, memoryCache, &completerModel)
    , undoStack(new QUndoStack(this))
{
    setMinimumSize(settings.minWidth, settings.minHeight);

    QFont font = this->font();
    font.setPixelSize(settings.fontSize);

    // status bar

    statusBar()->setFont(font);

    QLabel* dirLabel = new QLabel(tr("Folder: %1").arg(QDir::toNativeSeparators(directory.path())));
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
    view.installEventFilter(new TableViewEventFilter(this));

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

    firstAct = new QAction(tr("&First"), this);
    firstAct->setShortcut(Qt::Key_Home);
    firstAct->setEnabled(false);
    connect(firstAct, &QAction::triggered,
            this, &Window::selectFirst);
    menuBar()->addAction(firstAct);

    endAct = new QAction(tr("&End"), this);
    endAct->setShortcut(Qt::Key_End);
    endAct->setEnabled(false);
    connect(endAct, &QAction::triggered,
            this, &Window::selectEnd);
    menuBar()->addAction(endAct);

    QAction* sepAct3 = new QAction(u"|"_s, this);
    sepAct3->setDisabled(true);
    menuBar()->addAction(sepAct3);

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

    // dock

    if (settings.showCommandHistory) {
        QDockWidget *undoDockWidget = new QDockWidget(this);
        undoDockWidget->setWindowTitle(tr("Command history"));
        undoDockWidget->setWidget(new QUndoView(undoStack));
        addDockWidget(Qt::RightDockWidgetArea, undoDockWidget);
    }

    viewerDockWidget = new QDockWidget(this);
    viewer = new Viewer(memoryCache, viewerDockWidget);
    viewerDockWidget->setWidget(viewer);
    viewerDockWidget->setWindowTitle(tr("Viewer"));
    addDockWidget(Qt::LeftDockWidgetArea, viewerDockWidget);

    connect(this, &Window::viewImage,
            viewer, &Viewer::viewImage);

    mapDockWidget = new QDockWidget(this);
    mapStatistics = new QLabel(mapDockWidget);
    map = new QGVMap(mapDockWidget);

    QGVWidgetZoom* zoom = new QGVWidgetZoom();
    zoom->setAnchor(QPoint(10, 10), { Qt::LeftEdge, Qt::TopEdge });
    map->addWidget(zoom);

    QGVWidgetScale* scale = new QGVWidgetScale();
    scale->setAnchor(QPoint(10, 10), { Qt::LeftEdge, Qt::BottomEdge });
    map->addWidget(scale);

    const QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const QString fullCachePath = QDir(cacheLocation).filePath(u"map"_s);

    QNetworkDiskCache *cache = new QNetworkDiskCache(mapDockWidget);
    cache->setCacheDirectory(fullCachePath);
    cache->setMaximumCacheSize(settings.mapCacheSize);
    QNetworkAccessManager *manager = new QNetworkAccessManager(mapDockWidget);
    manager->setCache(cache);
    QGV::setNetworkManager(manager);

    map->setMouseAction(QGV::MouseAction::ZoomRect, false);
    map->setAcceptDrops(false);

    mapProviderChanged(0);

    QWidget *mapWidget = new QWidget(mapDockWidget);
    QVBoxLayout *mapLayout = new QVBoxLayout(mapWidget);

    if (settings.customMaps.size() > 1) {
        mapSelector = new QComboBox(mapDockWidget);
        for (const CustomMap& map : settings.customMaps) {
            mapSelector->addItem(map.name);
        }
        connect(mapSelector, &QComboBox::currentIndexChanged,
                this, &Window::mapProviderChanged);
        mapLayout->addWidget(mapSelector);
    }
    mapLayout->addWidget(map);
    mapLayout->addWidget(mapStatistics);
    mapDockWidget->setWidget(mapWidget);
    mapDockWidget->setWindowTitle(tr("Map"));
    addDockWidget(Qt::LeftDockWidgetArea, mapDockWidget);

    {
        const int height = view.size().height() / 2;
        const int width = settings.initialMapWidth;
        resizeDocks({mapDockWidget, viewerDockWidget}, {height, height}, Qt::Vertical);
        resizeDocks({mapDockWidget, viewerDockWidget}, {width, width}, Qt::Horizontal);
    }

    mapSelectionDebouncer = new KDToolBox::KDSignalDebouncer(this);
    mapSelectionDebouncer->setTimeout(100);

    connect(map, &QGVMap::selectionsChanged,
            mapSelectionDebouncer, &KDToolBox::KDSignalDebouncer::throttle);
    connect(mapSelectionDebouncer, &KDToolBox::KDSignalDebouncer::triggered,
            this, &Window::mapSelectionChanged);
}

Window::~Window()
{
    db.close();
}

QString Window::getThumbnailsDatabasePath()
{
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
    ModelItem::loadIcons();

    QList<const FileGroup *> fileGroupList;

    try {
        fileGroupList = FileGroup::fileGroupList(settings, directory);
    } catch (TooManyPartsError &e) {
        qCritical() << "Too many name parts:" << e.path;
        QMessageBox::critical(this,
                              tr("Too many name parts"),
                              tr("The file \"%1\" has too many parts in its name.\n"
                                 "The maximum number allowed is %2.\n"
                                 "This can be changed with the setting \"max names\".")
                                  .arg(QDir::toNativeSeparators(e.path),
                                       QString::number(settings.maxNames)));
        return;
    }

    if (fileGroupList.isEmpty()) {
        qInfo() << "No file found";
        QMessageBox::information(this,
                                 tr("No file found"),
                                 tr("No appropriate file has been found in %1.")
                                     .arg(QDir::toNativeSeparators(directory.path())));
        return;
    }

    progressDialog = new QProgressDialog(tr("Loading thumbnails..."), tr("Cancel"), 0, 11 * fileGroupList.size(), this);
    progressDialog->setMinimumDuration(2000);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setAttribute(Qt::WA_DeleteOnClose);
    progressDialog->setCancelButton(nullptr);
    progressDialog->setValue(0);

    connect(&model, &Model::loadingProgressed,
            this, &Window::modelLoadingProgressed);

    connect(&model, &Model::loadingFinished,
            this, &Window::modelLoadingDone);

    connect(&model, &Model::fileRenameFailed,
            this, &Window::fileRenameFailed);

    connect(&model, &Model::invalidCharacters,
            this, &Window::invalidCharacters);

    connect(&model, &Model::invalidCharacterSequence,
            this, &Window::invalidCharacterSequence);

    connect(&model, &Model::identifiersMustBeUnique,
            this, &Window::identifiersMustBeUnique);

    future = QtConcurrent::run([this, fileGroupList] {
        model.load(fileGroupList, directory, getThumbnailsDatabasePath());
        qDeleteAll(fileGroupList);
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

    {
        double minLat = +90.0, maxLat = -90.0, minLong = +180.0, maxLong = -180.0;

        for (int row = 0; row < proxyModel->rowCount(); ++row) {
            const QVariant possibleLocation = view.model()->data(proxyModel->index(row, 0), LocationRole);
            if (possibleLocation.isValid()) {
                const Location location = qvariant_cast<Location>(possibleLocation);
                const double latitude = location.getLatitude();
                const double longitude = location.getLongitude();

                minLat = qMin(minLat, latitude);
                maxLat = qMax(maxLat, latitude);
                minLong = qMin(minLong, longitude);
                maxLong = qMax(maxLong, longitude);
            }

            const QVariant possibleMapIcon = view.model()->data(proxyModel->index(row, 0), MapIconRole);
            if (possibleMapIcon.isValid()) {
                QGVIcon* mapIcon = qvariant_cast<QGVIcon*>(possibleMapIcon);

                map->addItem(mapIcon);
            }
        }

        if (minLat != +90.0 && maxLat != -90.0 && minLong != +180.0 && maxLong != -180.0) {
            minLat -= 0.01;
            maxLat += 0.01;
            minLong -= 0.01;
            maxLong += 0.01;
            const QGV::GeoRect rect(minLat, minLong, maxLat, maxLong);
            map->cameraTo(QGVCameraActions(map).scaleTo(rect));
        } else {
            removeDockWidget(mapDockWidget);
        }
    }

    connect(view.selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &Window::selectionChanged);

    connect(view.model(), &QAbstractItemModel::dataChanged,
            this, &Window::dataChanged);

    db = QSqlDatabase::addDatabase(u"QSQLITE"_s, u"update"_s);
    db.setDatabaseName(getThumbnailsDatabasePath());
    if (! db.open()) {
        qWarning() << "Thumbnails database opening failure:" << db.lastError();
    }
}

void Window::fileRenameFailed(const QString &before, const QString &after)
{
    QMessageBox::critical(this,
                          tr("Unable to rename file"),
                          tr("Before : %1\nAfter : %2").arg(before, after));
}

void Window::invalidCharacters()
{
    QMessageBox::information(this,
                             tr("Forbidden characters"),
                             tr("The following characters are not allowed:\n< > : \" / \\ | ? * ,"));
}

void Window::invalidCharacterSequence()
{
    QMessageBox::information(this,
                             tr("Forbidden character sequence"),
                             tr("The following character sequence is not allowed: \" - \" (hyphen with surrounding spaces)."));
}

void Window::identifiersMustBeUnique(const QString &id)
{
    QMessageBox::information(this,
                             tr("Identifiers must be unique"),
                             tr("Identifier \"%1\" is already used.").arg(id));
}

void Window::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected)
    Q_UNUSED(deselected)

    const QItemSelectionModel* selectionModel = view.selectionModel();

    if (selectionModel == nullptr)
        return;

    const QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

    if (selectedIndexes.size() == 1) {
        Q_EMIT viewImage(view.model()->data(selectedIndexes.constFirst().siblingAtColumn(ThumbnailCol), ImagePathRole).toString());
    } else {
        Q_EMIT viewImage(QString());
    }

    {
        const int rowCount = view.model()->rowCount();
        double minLat = +90.0, maxLat = -90.0, minLong = +180.0, maxLong = -180.0;
        QSet<int> selectedRows;

        for (const QModelIndex &index : selectedIndexes)
            selectedRows.insert(index.row());

        disconnect(map, &QGVMap::selectionsChanged,
                   mapSelectionDebouncer, &KDToolBox::KDSignalDebouncer::throttle);

        for (int row = 0 ; row < rowCount ; ++row) {
            const QModelIndex index = view.model()->index(row, 0);
            const bool isSelected = selectedRows.contains(row);

            view.model()->setData(index, isSelected, MapSelectedRole);

            if (isSelected) {
                const QVariant possibleLocation = view.model()->data(index, LocationRole);

                if (possibleLocation.isValid()) {
                    const Location location = qvariant_cast<Location>(possibleLocation);
                    const double latitude = location.getLatitude();
                    const double longitude = location.getLongitude();

                    minLat = qMin(minLat, latitude);
                    maxLat = qMax(maxLat, latitude);
                    minLong = qMin(minLong, longitude);
                    maxLong = qMax(maxLong, longitude);
                }
            }
        }

        connect(map, &QGVMap::selectionsChanged,
                mapSelectionDebouncer, &KDToolBox::KDSignalDebouncer::throttle);

        if (minLat != +90.0 && maxLat != -90.0 && minLong != +180.0 && maxLong != -180.0) {
            if (selectedRows.size() == 1) {
                map->cameraTo(QGVCameraActions(map).moveTo(QGV::GeoPos(minLat, minLong)));
            } else {
                minLat -= 0.01;
                maxLat += 0.01;
                minLong -= 0.01;
                maxLong += 0.01;
                const QGV::GeoRect rect(minLat, minLong, maxLat, maxLong);
                map->cameraTo(QGVCameraActions(map).scaleTo(rect));
            }
        }
    }

    if (selectedIndexes.isEmpty()) {
        pasteAct->setDisabled(true);
        copyAct->setDisabled(true);
        deleteAct->setDisabled(true);
        insertAct->setDisabled(true);
        firstAct->setDisabled(true);
        endAct->setDisabled(true);
    } else {
        const bool allNames = std::all_of(selectedIndexes.cbegin(), selectedIndexes.cend(),
                                          [](const QModelIndex &index){ return index.column() >= FirstNameCol; });

        const bool noneEmpty = std::none_of(selectedIndexes.cbegin(), selectedIndexes.cend(),
                                            [model = view.model()](const QModelIndex &index){ return model->data(index).isNull(); });

        const bool noHoleNames = std::all_of(selectedIndexes.cbegin(), selectedIndexes.cend(),
                                             [model = view.model()](const QModelIndex &index) {
                                                 return index.column() == FirstNameCol
                                                        || (index.column() > FirstNameCol
                                                            && index.column() <= FirstNameCol + model->data(index, NameCountRole).toInt());
                                             });

        const bool noneFull = std::none_of(selectedIndexes.cbegin(), selectedIndexes.cend(),
                                           [model = view.model(), maxNames = settings.maxNames](const QModelIndex &index) {
                                               return model->data(index, NameCountRole).toInt() == maxNames;
                                           });

        pasteAct->setEnabled(selectedIndexes.size() == 1
                             || allNames);

        copyAct->setEnabled((selectedIndexes.size() == 1
                             && ! view.model()->data(selectedIndexes.constFirst()).isNull())
                            || (selectedIndexes.size() > 1
                                && allNames
                                && noneEmpty));

        deleteAct->setEnabled(allNames && noneEmpty);

        insertAct->setEnabled(noHoleNames && noneFull);

        firstAct->setEnabled(true);

        endAct->setEnabled(noneFull);
    }
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
        const QModelIndex index = selection->selectedIndexes().constFirst();
        if (index.column() == IdentifierCol || index.column() >= FirstNameCol) {
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
        for (const QModelIndex& index : std::as_const(indexes)) {
            if (index.column() >= FirstNameCol) {
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
    const QString text = QApplication::clipboard()->text(mode).trimmed();

    if (! text.isEmpty()) {
        const QStringList texts = text.split(u","_s);

        if (texts.size() == 1) {
            QModelIndexList indexes = selection->selectedIndexes();
            if (indexes.size() == 1) {
                const QModelIndex& index = indexes.constFirst();
                if (index.column() == IdentifierCol)
                    undoStack->push(new ChangeIdCommand(text, view.model(), index));
                else if (index.column() >= FirstNameCol)
                    undoStack->push(new PasteCommand(text, view.model(), index));
            } else {
                std::sort(indexes.begin(), indexes.end(),
                          [](const QModelIndex& a, const QModelIndex& b) {
                              return a.row() < b.row()
                                     || (a.row() == b.row() && a.column() < b.column()); });

                for (const QModelIndex& index : std::as_const(indexes)) {
                    if (index.column() >= FirstNameCol)
                        undoStack->push(new PasteCommand(text, view.model(), index));
                }
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
                if (indexes[i].column() >= FirstNameCol)
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

    for (const QModelIndex& index : std::as_const(indexes)) {
        if (index.column() >= FirstNameCol)
            undoStack->push(new DeleteCommand(view.model(), index));
    }
}

void Window::insertAtSelection()
{
    QItemSelectionModel* selection = view.selectionModel();
    if (selection == nullptr) return;

    QInputDialog dialog(this);
    dialog.setWindowTitle(tr("Insertion"));
    dialog.setLabelText(tr("Name:"));
    dialog.setInputMode(QInputDialog::TextInput); // force instanciation of QLineEdit to find it below

    if (QLineEdit* lineEdit = dialog.findChild<QLineEdit *>()) {
        QCompleter *completer = new QCompleter(&completerModel, lineEdit);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
        lineEdit->setCompleter(completer);
    }

    if (dialog.exec() != 1) return;
    const QString text = dialog.textValue();
    if (text.isEmpty()) return;

    QModelIndexList indexes = selection->selectedIndexes();
    std::sort(indexes.begin(), indexes.end(),
              [](const QModelIndex& a, const QModelIndex& b) {
                  return a.row() < b.row()
                         || (a.row() == b.row() && a.column() > b.column()); });

    for (const QModelIndex& index : std::as_const(indexes)) {
        if (index.column() >= FirstNameCol)
            undoStack->push(new InsertCommand(text, view.model(), index));
    }
}

void Window::selectFirst()
{
    QSet<int> selectedRows;
    QItemSelection selection;
    int minRow = INT_MAX;

    const QItemSelectionModel* selectionModel = view.selectionModel();

    if (selectionModel == nullptr)
        return;

    const QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

    for (const QModelIndex &index : selectedIndexes) {
        selectedRows.insert(index.row());
        minRow = qMin(minRow, index.row());
    }

    for (const int row : selectedRows) {
        const QModelIndex index = view.model()->index(row, FirstNameCol);
        selection.select(index, index);

        if (row == minRow)
            view.selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
    }

    view.selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
}

void Window::selectEnd()
{
    QSet<int> selectedRows;
    QItemSelection selection;
    int minRow = INT_MAX;

    const QItemSelectionModel* selectionModel = view.selectionModel();

    if (selectionModel == nullptr)
        return;

    const QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

    for (const QModelIndex &index : selectedIndexes) {
        selectedRows.insert(index.row());
        minRow = qMin(minRow, index.row());
    }

    for (const int row : selectedRows) {
        const QModelIndex indexFirst = view.model()->index(row, FirstNameCol);
        const int nameCount = view.model()->data(indexFirst, NameCountRole).toInt();
        const QModelIndex index = indexFirst.siblingAtColumn(FirstNameCol + nameCount);
        selection.select(index, index);

        if (row == minRow)
            view.selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
    }

    view.selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
}

void Window::mapSelectionChanged()
{
    if (view.selectionModel() == nullptr)
        return;

    QItemSelection selection;
    const int rowCount = view.model()->rowCount();
    bool first = true;

    for (int row = 0 ; row < rowCount ; ++row) {
        const QModelIndex index = view.model()->index(row, FirstNameCol);
        const QVariant possibleMapIcon = view.model()->data(index, MapIconRole);

        if (possibleMapIcon.isValid()) {
            QGVIcon* mapIcon = qvariant_cast<QGVIcon*>(possibleMapIcon);

            if (mapIcon->isSelected()) {
                selection.select(index, index);

                if (first) {
                    view.selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
                    first = false;
                }
            }
        }
    }

    view.selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
    view.setFocus();
}

void Window::mapProviderChanged(int index)
{
    if (mapItemBackground != nullptr) {
        map->removeItem(mapItemBackground);
        delete mapItemBackground;
    }

    if (mapItemCopyrightNotice != nullptr) {
        map->removeWidget(mapItemCopyrightNotice);
        delete mapItemCopyrightNotice;
    }

    CustomMapXYZ *tilesLayer = new CustomMapXYZ(settings.customMaps.at(index).url,
                                                settings.customMaps.at(index).minZoomLevel,
                                                settings.customMaps.at(index).maxZoomLevel);
    map->addItem(tilesLayer);
    mapItemBackground = tilesLayer;

    QGVWidgetText* text = new QGVWidgetText();
    text->setText(settings.customMaps.at(index).copyrightNotice);
    text->setAnchor(QPoint(10, 10), { Qt::RightEdge, Qt::BottomEdge });
    map->addWidget(text);
    mapItemCopyrightNotice = text;

    connect(tilesLayer, &QGVLayerTilesOnline::tileReply,
            this, &Window::mapTileReply);
}

void Window::mapTileReply(bool error, bool canceled, bool fromCache)
{
    mapRepliesTotalCount++;
    if (error) mapRepliesErrorCount++;
    else if (canceled) mapRepliesCanceledCount++;
    else if (fromCache) mapRepliesCacheCount++;
    else mapRepliesNewCount++;

    mapStatistics->setText(tr("Requested tiles: %1 (%2 new, %3 from cache, %4 canceled, %5 errors)")
                               .arg(mapRepliesTotalCount)
                               .arg(mapRepliesNewCount)
                               .arg(mapRepliesCacheCount)
                               .arg(mapRepliesCanceledCount)
                               .arg(mapRepliesErrorCount));
}
