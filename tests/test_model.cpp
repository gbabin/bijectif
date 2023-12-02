// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "test_model.h"
#include "../src/command_delete.h"
#include "../src/command_edit.h"
#include "../src/command_id.h"
#include "../src/window.h"
#include "../src/model.h"

#include <QFileInfoList>
#include <QSignalSpy>
#include <QSortFilterProxyModel>
#include <QStandardPaths>

void ModelTest::initTestCase()
{
    QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    rootDir = tmpDir.filePath("bijectif-tests");
    QDir().mkpath(rootDir.path());
    dbDir = rootDir.filePath("db");
    QDir().mkpath(dbDir.path());
    filesDir = rootDir.filePath("files");
    QDir().mkpath(filesDir.path());
    for (int i = 99; i >= 0; i--) {
        QList<int> numbers(i%(1+Model::maxNames));
        std::iota(numbers.begin(), numbers.end(), 1);
        QStringList namesPart;
        std::transform(numbers.cbegin(), numbers.cend(),
                       std::back_inserter(namesPart),
                       [](int num){ return QString::number(num); });
        QString names = namesPart.join(", ");
        QStringList all;
        all.append(QString::asprintf("IMG_%04d", i));
        if (! names.isEmpty()) all.append(names);
        QString filename = all.join(" - ") + ".jpg";
        QImage(8, 8, QImage::Format_RGB888).save(filesDir.filePath(filename));
    }
}

void ModelTest::test_1()
{
    const QFileInfoList files = Window::listFiles(filesDir);
    const QString dbPath = dbDir.filePath("thumbnails.sqlite");

    QSortFilterProxyModel proxyModel;
    {
        Model* model = new Model;
        model->load(files, dbPath);
        proxyModel.setSourceModel(model);
    }
    proxyModel.sort(1, Qt::AscendingOrder);

    QVERIFY(QFile::exists(dbPath));
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);
    QVERIFY(db.open());

    QCOMPARE(proxyModel.columnCount(), 2 + Model::maxNames);
    QCOMPARE(proxyModel.rowCount(), 100);

    for (int row = 0; row < 100; row++) {
        QCOMPARE(proxyModel.data(proxyModel.index(row, 1), Qt::DisplayRole),
                 QVariant(QString::asprintf("IMG_%04d", row)));

        for (int name = 1; name <= Model::maxNames; name++) {
            if (name <= row%(1+Model::maxNames)) {
                QCOMPARE(proxyModel.data(proxyModel.index(row, name+1), Qt::DisplayRole),
                         QVariant(QString::number(name)));
            } else {
                QCOMPARE(proxyModel.data(proxyModel.index(row, name+1), Qt::DisplayRole),
                         QVariant());
            }
        }
    }

    QUndoStack undoStack;

    for (int row = 0; row <= Model::maxNames; row++) {
        undoStack.push(new DeleteCommand(&proxyModel, proxyModel.index(row, 2)));
    }

    for (int row = 0; row <= Model::maxNames; row++) {
        QCOMPARE(proxyModel.data(proxyModel.index(row, 1), Qt::DisplayRole),
                 QVariant(QString::asprintf("IMG_%04d", row)));

        for (int name = 1; name <= Model::maxNames; name++) {
            if (name < row%(1+Model::maxNames)) {
                QCOMPARE(proxyModel.data(proxyModel.index(row, name+1), Qt::DisplayRole),
                         QVariant(QString::number(name+1)));
            } else {
                QCOMPARE(proxyModel.data(proxyModel.index(row, name+1), Qt::DisplayRole),
                         QVariant());
            }
        }
    }

    QCOMPARE(undoStack.index(), Model::maxNames);
    for (int row = 0; row < Model::maxNames; row++) {
        QVERIFY(undoStack.canUndo());
        undoStack.undo();
    }
    QVERIFY(! undoStack.canUndo());

    for (int row = 0; row < 100; row++) {
        QCOMPARE(proxyModel.data(proxyModel.index(row, 1), Qt::DisplayRole),
                 QVariant(QString::asprintf("IMG_%04d", row)));

        for (int name = 1; name <= Model::maxNames; name++) {
            if (name <= row%(1+Model::maxNames)) {
                QCOMPARE(proxyModel.data(proxyModel.index(row, name+1), Qt::DisplayRole),
                         QVariant(QString::number(name)));
            } else {
                QCOMPARE(proxyModel.data(proxyModel.index(row, name+1), Qt::DisplayRole),
                         QVariant());
            }
        }
    }

    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    QCOMPARE(proxyModel.data(proxyModel.index(5, 1), Qt::DisplayRole),
             QVariant("IMG_0005"));
    undoStack.push(new ChangeIdCommand("IMG_0010", &proxyModel, proxyModel.index(5, 1)));
    QCOMPARE(spy.count(), 0);
    QVERIFY(! undoStack.canUndo());
    QCOMPARE(proxyModel.data(proxyModel.index(5, 1), Qt::DisplayRole),
             QVariant("IMG_0005"));

    QCOMPARE(proxyModel.data(proxyModel.index(50, 6), Qt::DisplayRole),
             QVariant(QString::number(5)));
    QCOMPARE(proxyModel.data(proxyModel.index(50, 7), Qt::DisplayRole),
             QVariant());

    undoStack.push(new ChangeIdCommand("IMG_0100", &proxyModel, proxyModel.index(10, 1)));
    QVERIFY(undoStack.canUndo());
    QCOMPARE(proxyModel.data(proxyModel.index(10, 1), Qt::DisplayRole),
             QVariant("IMG_0011"));
    QCOMPARE(proxyModel.data(proxyModel.index(11, 1), Qt::DisplayRole),
             QVariant("IMG_0012"));
    QCOMPARE(proxyModel.data(proxyModel.index(99, 1), Qt::DisplayRole),
             QVariant("IMG_0100"));

    QCOMPARE(proxyModel.data(proxyModel.index(49, 6), Qt::DisplayRole),
             QVariant(QString::number(5)));
    QCOMPARE(proxyModel.data(proxyModel.index(49, 7), Qt::DisplayRole),
             QVariant());

    undoStack.push(new EditCommand("XYZ", &proxyModel, proxyModel.index(49, 6)));

    QCOMPARE(proxyModel.data(proxyModel.index(49, 6), Qt::DisplayRole),
             QVariant("XYZ"));
    QCOMPARE(proxyModel.data(proxyModel.index(49, 7), Qt::DisplayRole),
             QVariant());

    undoStack.undo();

    QCOMPARE(proxyModel.data(proxyModel.index(49, 6), Qt::DisplayRole),
             QVariant(QString::number(5)));
    QCOMPARE(proxyModel.data(proxyModel.index(49, 7), Qt::DisplayRole),
             QVariant());

    QVERIFY(undoStack.canUndo());
    undoStack.undo();
    for (int row = 0; row < 100; row++) {
        QCOMPARE(proxyModel.data(proxyModel.index(row, 1), Qt::DisplayRole),
                 QVariant(QString::asprintf("IMG_%04d", row)));

        for (int name = 1; name <= Model::maxNames; name++) {
            if (name <= row%(1+Model::maxNames)) {
                QCOMPARE(proxyModel.data(proxyModel.index(row, name+1), Qt::DisplayRole),
                         QVariant(QString::number(name)));
            } else {
                QCOMPARE(proxyModel.data(proxyModel.index(row, name+1), Qt::DisplayRole),
                         QVariant());
            }
        }
    }

    QVERIFY(! undoStack.canUndo());

    db.close();
}

void ModelTest::cleanupTestCase()
{
    for (const QString &name : QSqlDatabase::connectionNames())
        QSqlDatabase::removeDatabase(name);
    if (rootDir.path().startsWith("/tmp/"))
        rootDir.removeRecursively();
}
