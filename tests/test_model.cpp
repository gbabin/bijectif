// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "test_model.h"

#include "../src/command_delete.h"
#include "../src/command_edit.h"
#include "../src/command_id.h"
#include "../src/model.h"

#include <QSignalSpy>
#include <QSortFilterProxyModel>
#include <QSqlDatabase>
#include <QStandardPaths>

using namespace Qt::Literals::StringLiterals;

ModelTest::ModelTest(const Settings &settings, QObject *parent)
    : QObject(parent)
    , settings(settings)
{
}

bool ModelTest::createEmptyFile(const QString &path)
{
    QFile file(path);
    const bool ret = file.open(QIODevice::WriteOnly);
    file.close();
    return ret;
}

void ModelTest::init()
{
    const QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    rootDir = tmpDir.filePath(u"bijectif-tests"_s);
    QDir().mkpath(rootDir.path());
    dbDir = rootDir.filePath(u"db"_s);
    QDir().mkpath(dbDir.path());
    filesDir = rootDir.filePath(u"files"_s);
    QDir().mkpath(filesDir.path());
}

void ModelTest::test_1()
{
    for (int i = 99; i >= 0; i--) {
        QList<int> numbers(i%(1+settings.maxNames));
        std::iota(numbers.begin(), numbers.end(), 1);
        QStringList namesPart;
        std::transform(numbers.cbegin(), numbers.cend(),
                       std::back_inserter(namesPart),
                       [](int num){ return QString::number(num); });
        const QString names = namesPart.join(u", "_s);
        QString filename = QString::asprintf("IMG_%04d", i);
        if (! names.isEmpty()) filename += u" - "_s + names;
        filename += u".jpg"_s;
        QImage(8, 8, QImage::Format_RGB888).save(filesDir.filePath(filename));
    }

    const QList<const FileGroup *> fileGroupList = FileGroup::fileGroupList(settings, filesDir);
    const QString dbPath = dbDir.filePath(u"thumbnails.sqlite"_s);

    QSortFilterProxyModel proxyModel;
    {
        Model* model = new Model(settings, new CompleterModel(this));
        model->load(fileGroupList, filesDir, dbPath);
        proxyModel.setSourceModel(model);
    }
    proxyModel.sort(IdentifierCol, Qt::AscendingOrder);

    QVERIFY(QFile::exists(dbPath));
    QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, u"update"_s);
    db.setDatabaseName(dbPath);
    QVERIFY(db.open());

    QCOMPARE(proxyModel.columnCount(), FirstNameCol + settings.maxNames);
    QCOMPARE(proxyModel.rowCount(), 100);

    for (int row = 0; row < 100; row++) {
        QCOMPARE(proxyModel.data(proxyModel.index(row, IdentifierCol), Qt::DisplayRole),
                 QVariant(QString::asprintf("IMG_%04d", row)));

        for (int name = 1; name <= settings.maxNames; name++) {
            if (name <= row%(1+settings.maxNames)) {
                QCOMPARE(proxyModel.data(proxyModel.index(row, name + FirstNameCol - 1), Qt::DisplayRole),
                         QVariant(QString::number(name)));
            } else {
                QCOMPARE(proxyModel.data(proxyModel.index(row, name + FirstNameCol - 1), Qt::DisplayRole),
                         QVariant());
            }
        }
    }

    QUndoStack undoStack;

    for (int row = 0; row <= settings.maxNames; row++) {
        undoStack.push(new DeleteCommand(&proxyModel, proxyModel.index(row, FirstNameCol)));
    }

    for (int row = 0; row <= settings.maxNames; row++) {
        QCOMPARE(proxyModel.data(proxyModel.index(row, IdentifierCol), Qt::DisplayRole),
                 QVariant(QString::asprintf("IMG_%04d", row)));

        for (int name = 1; name <= settings.maxNames; name++) {
            if (name < row%(1+settings.maxNames)) {
                QCOMPARE(proxyModel.data(proxyModel.index(row, name + FirstNameCol - 1), Qt::DisplayRole),
                         QVariant(QString::number(name+1)));
            } else {
                QCOMPARE(proxyModel.data(proxyModel.index(row, name + FirstNameCol - 1), Qt::DisplayRole),
                         QVariant());
            }
        }
    }

    QCOMPARE(undoStack.index(), settings.maxNames);
    for (int row = 0; row < settings.maxNames; row++) {
        QVERIFY(undoStack.canUndo());
        undoStack.undo();
    }
    QVERIFY(! undoStack.canUndo());

    for (int row = 0; row < 100; row++) {
        QCOMPARE(proxyModel.data(proxyModel.index(row, IdentifierCol), Qt::DisplayRole),
                 QVariant(QString::asprintf("IMG_%04d", row)));

        for (int name = 1; name <= settings.maxNames; name++) {
            if (name <= row%(1+settings.maxNames)) {
                QCOMPARE(proxyModel.data(proxyModel.index(row, name + FirstNameCol - 1), Qt::DisplayRole),
                         QVariant(QString::number(name)));
            } else {
                QCOMPARE(proxyModel.data(proxyModel.index(row, name + FirstNameCol - 1), Qt::DisplayRole),
                         QVariant());
            }
        }
    }

    const QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    QCOMPARE(proxyModel.data(proxyModel.index(5, IdentifierCol), Qt::DisplayRole),
             QVariant(u"IMG_0005"_s));
    undoStack.push(new ChangeIdCommand(u"IMG_0010"_s, &proxyModel, proxyModel.index(5, IdentifierCol)));
    QCOMPARE(spy.count(), 0);
    QVERIFY(! undoStack.canUndo());
    QCOMPARE(proxyModel.data(proxyModel.index(5, IdentifierCol), Qt::DisplayRole),
             QVariant(u"IMG_0005"_s));

    QCOMPARE(proxyModel.data(proxyModel.index(50, FirstNameCol + 4), Qt::DisplayRole),
             QVariant(QString::number(5)));
    QCOMPARE(proxyModel.data(proxyModel.index(50, FirstNameCol + 5), Qt::DisplayRole),
             QVariant());

    undoStack.push(new ChangeIdCommand(u"IMG_0100"_s, &proxyModel, proxyModel.index(10, IdentifierCol)));
    QVERIFY(undoStack.canUndo());
    QCOMPARE(proxyModel.data(proxyModel.index(10, IdentifierCol), Qt::DisplayRole),
             QVariant(u"IMG_0011"_s));
    QCOMPARE(proxyModel.data(proxyModel.index(11, IdentifierCol), Qt::DisplayRole),
             QVariant(u"IMG_0012"_s));
    QCOMPARE(proxyModel.data(proxyModel.index(99, IdentifierCol), Qt::DisplayRole),
             QVariant(u"IMG_0100"_s));

    QCOMPARE(proxyModel.data(proxyModel.index(49, FirstNameCol + 4), Qt::DisplayRole),
             QVariant(QString::number(5)));
    QCOMPARE(proxyModel.data(proxyModel.index(49, FirstNameCol + 5), Qt::DisplayRole),
             QVariant());

    undoStack.push(new EditCommand(u"XYZ"_s, &proxyModel, proxyModel.index(49, FirstNameCol + 4)));

    QCOMPARE(proxyModel.data(proxyModel.index(49, FirstNameCol + 4), Qt::DisplayRole),
             QVariant(u"XYZ"_s));
    QCOMPARE(proxyModel.data(proxyModel.index(49, FirstNameCol + 5), Qt::DisplayRole),
             QVariant());

    undoStack.undo();

    QCOMPARE(proxyModel.data(proxyModel.index(49, FirstNameCol + 4), Qt::DisplayRole),
             QVariant(QString::number(5)));
    QCOMPARE(proxyModel.data(proxyModel.index(49, FirstNameCol + 5), Qt::DisplayRole),
             QVariant());

    QVERIFY(undoStack.canUndo());
    undoStack.undo();
    for (int row = 0; row < 100; row++) {
        QCOMPARE(proxyModel.data(proxyModel.index(row, IdentifierCol), Qt::DisplayRole),
                 QVariant(QString::asprintf("IMG_%04d", row)));

        for (int name = 1; name <= settings.maxNames; name++) {
            if (name <= row%(1+settings.maxNames)) {
                QCOMPARE(proxyModel.data(proxyModel.index(row, name + FirstNameCol - 1), Qt::DisplayRole),
                         QVariant(QString::number(name)));
            } else {
                QCOMPARE(proxyModel.data(proxyModel.index(row, name + FirstNameCol - 1), Qt::DisplayRole),
                         QVariant());
            }
        }
    }

    QVERIFY(! undoStack.canUndo());

    db.close();
}

void ModelTest::test_2()
{
    QVERIFY(QImage(8, 8, QImage::Format_RGB888).save(filesDir.filePath(u"IMG_0001.jpg"_s)));  // main image file
    QVERIFY(createEmptyFile(filesDir.filePath(u"IMG_0001.jpg.xmp"_s))); // sidecar (additional suffix)
    QVERIFY(createEmptyFile(filesDir.filePath(u"IMG_0001.dnf"_s))); // sidecar (other image suffix)
    QVERIFY(createEmptyFile(filesDir.filePath(u"IMG_0001.dnf.xmp"_s))); // sidecar of sidecar
    QVERIFY(createEmptyFile(filesDir.filePath(u"IMG_0001.dnf.xmp.bak"_s))); // sidecar of sidecar of sidecar
    QVERIFY(createEmptyFile(filesDir.filePath(u"IMG_0001.dnf.xmp.old"_s))); // other sidecar of sidecar of sidecar
    QVERIFY(createEmptyFile(filesDir.filePath(u"IMG_0001.txt"_s))); // sidecar (other suffix)
    QVERIFY(createEmptyFile(filesDir.filePath(u"IMG_0001.webp"_s))); // sidecar (other image suffix)
    QVERIFY(createEmptyFile(filesDir.filePath(u"IMG_0001.tar.gz"_s))); // other suffix (gz) not sidecar
    QVERIFY(createEmptyFile(filesDir.filePath(u"IMG_0001.jpg.foo.bar"_s))); // other suffix of not sidecar
    QVERIFY(QImage(8, 8, QImage::Format_RGB888).save(filesDir.filePath(u"IMG_0001.1.jpg"_s))); // other image file
    QVERIFY(QImage(8, 8, QImage::Format_RGB888).save(filesDir.filePath(u"IMG_0002.JPG"_s))); // other image file

    const QList<const FileGroup *> fileGroupList = FileGroup::fileGroupList(settings, filesDir);
    const QString dbPath = dbDir.filePath(u"thumbnails.sqlite"_s);

    QSortFilterProxyModel proxyModel;
    {
        Model* model = new Model(settings, new CompleterModel(this));
        model->load(fileGroupList, filesDir, dbPath);
        proxyModel.setSourceModel(model);
    }
    proxyModel.sort(IdentifierCol, Qt::AscendingOrder);

    QSignalSpy fileRenameFailedSpy(qobject_cast<Model*>(proxyModel.sourceModel()), &Model::fileRenameFailed);
    QVERIFY(fileRenameFailedSpy.isValid());

    QVERIFY(QFile::exists(dbPath));
    QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, u"update"_s);
    db.setDatabaseName(dbPath);
    QVERIFY(db.open());

    QCOMPARE(proxyModel.columnCount(), FirstNameCol + settings.maxNames);
    QCOMPARE(proxyModel.rowCount(), 3);

    QCOMPARE(proxyModel.data(proxyModel.index(0, ThumbnailCol), ImagePathRole),
             filesDir.filePath(u"IMG_0001.jpg"_s));
    QCOMPARE(proxyModel.data(proxyModel.index(0, IdentifierCol), Qt::DisplayRole),
             u"IMG_0001"_s);
    for (int name = 1; name <= settings.maxNames; name++) {
        QCOMPARE(proxyModel.data(proxyModel.index(0, name + FirstNameCol - 1), Qt::DisplayRole),
                 QVariant());
    }

    QCOMPARE(proxyModel.data(proxyModel.index(1, ThumbnailCol), ImagePathRole),
             filesDir.filePath(u"IMG_0001.1.jpg"_s));
    QCOMPARE(proxyModel.data(proxyModel.index(1, IdentifierCol), Qt::DisplayRole),
             u"IMG_0001.1"_s);

    QCOMPARE(proxyModel.data(proxyModel.index(2, ThumbnailCol), ImagePathRole),
             filesDir.filePath(u"IMG_0002.JPG"_s));
    QCOMPARE(proxyModel.data(proxyModel.index(2, IdentifierCol), Qt::DisplayRole),
             u"IMG_0002"_s);

    EditCommand edit1(u"ABC"_s, &proxyModel, proxyModel.index(0, FirstNameCol));
    edit1.redo();

    EditCommand edit2(u"DEF"_s, &proxyModel, proxyModel.index(0, FirstNameCol + 1));
    edit2.redo();

    EditCommand edit3(u"GHI"_s, &proxyModel, proxyModel.index(0, FirstNameCol + 2));
    edit3.redo();

    QCOMPARE(filesDir.entryList(QDir::Files, QDir::Name),
             QStringList()
                 << u"IMG_0001 - ABC, DEF, GHI.dnf"_s
                 << u"IMG_0001 - ABC, DEF, GHI.dnf.xmp"_s
                 << u"IMG_0001 - ABC, DEF, GHI.dnf.xmp.bak"_s
                 << u"IMG_0001 - ABC, DEF, GHI.dnf.xmp.old"_s
                 << u"IMG_0001 - ABC, DEF, GHI.jpg"_s
                 << u"IMG_0001 - ABC, DEF, GHI.jpg.xmp"_s
                 << u"IMG_0001 - ABC, DEF, GHI.txt"_s
                 << u"IMG_0001 - ABC, DEF, GHI.webp"_s
                 << u"IMG_0001.1.jpg"_s
                 << u"IMG_0001.jpg.foo.bar"_s
                 << u"IMG_0001.tar.gz"_s
                 << u"IMG_0002.JPG"_s);

    edit3.undo();
    edit2.undo();

    QCOMPARE(filesDir.entryList(QDir::Files, QDir::Name),
             QStringList()
                 << u"IMG_0001 - ABC.dnf"_s
                 << u"IMG_0001 - ABC.dnf.xmp"_s
                 << u"IMG_0001 - ABC.dnf.xmp.bak"_s
                 << u"IMG_0001 - ABC.dnf.xmp.old"_s
                 << u"IMG_0001 - ABC.jpg"_s
                 << u"IMG_0001 - ABC.jpg.xmp"_s
                 << u"IMG_0001 - ABC.txt"_s
                 << u"IMG_0001 - ABC.webp"_s
                 << u"IMG_0001.1.jpg"_s
                 << u"IMG_0001.jpg.foo.bar"_s
                 << u"IMG_0001.tar.gz"_s
                 << u"IMG_0002.JPG"_s);

    edit2.redo();

    QCOMPARE(filesDir.entryList(QDir::Files, QDir::Name),
             QStringList()
                 << u"IMG_0001 - ABC, DEF.dnf"_s
                 << u"IMG_0001 - ABC, DEF.dnf.xmp"_s
                 << u"IMG_0001 - ABC, DEF.dnf.xmp.bak"_s
                 << u"IMG_0001 - ABC, DEF.dnf.xmp.old"_s
                 << u"IMG_0001 - ABC, DEF.jpg"_s
                 << u"IMG_0001 - ABC, DEF.jpg.xmp"_s
                 << u"IMG_0001 - ABC, DEF.txt"_s
                 << u"IMG_0001 - ABC, DEF.webp"_s
                 << u"IMG_0001.1.jpg"_s
                 << u"IMG_0001.jpg.foo.bar"_s
                 << u"IMG_0001.tar.gz"_s
                 << u"IMG_0002.JPG"_s);

    QVERIFY(createEmptyFile(filesDir.filePath(u"IMG_0001 - ABC, DEF, GHI.dnf"_s))); // prepares conflict

    QCOMPARE(fileRenameFailedSpy.count(), 0);
    edit3.redo(); // fails because of conflict
    QCOMPARE(fileRenameFailedSpy.count(), 1);
    {
        const QList<QVariant> arguments = fileRenameFailedSpy.takeFirst();
        QCOMPARE(arguments.at(0).toString(), filesDir.filePath(u"IMG_0001 - ABC, DEF.dnf"_s));
        QCOMPARE(arguments.at(1).toString(), filesDir.filePath(u"IMG_0001 - ABC, DEF, GHI.dnf"_s));
    }

    QCOMPARE(filesDir.entryList(QDir::Files, QDir::Name),
             QStringList()
                 << u"IMG_0001 - ABC, DEF, GHI.dnf"_s
                 << u"IMG_0001 - ABC, DEF.dnf"_s
                 << u"IMG_0001 - ABC, DEF.dnf.xmp"_s
                 << u"IMG_0001 - ABC, DEF.dnf.xmp.bak"_s
                 << u"IMG_0001 - ABC, DEF.dnf.xmp.old"_s
                 << u"IMG_0001 - ABC, DEF.jpg"_s
                 << u"IMG_0001 - ABC, DEF.jpg.xmp"_s
                 << u"IMG_0001 - ABC, DEF.txt"_s
                 << u"IMG_0001 - ABC, DEF.webp"_s
                 << u"IMG_0001.1.jpg"_s
                 << u"IMG_0001.jpg.foo.bar"_s
                 << u"IMG_0001.tar.gz"_s
                 << u"IMG_0002.JPG"_s);

    db.close();
}

void ModelTest::cleanup()
{
    QSqlDatabase::removeDatabase(u"update"_s);

    if (rootDir.path().startsWith(u"/tmp/"_s))
        rootDir.removeRecursively();
}
