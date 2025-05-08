// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "test_row.h"

#include "../src/command_delete.h"
#include "../src/command_edit.h"
#include "../src/command_insert.h"
#include "../src/command_paste.h"
#include "../src/model.h"

#include <QFileInfoList>
#include <QSignalSpy>
#include <QSortFilterProxyModel>
#include <QStandardPaths>

RowTest::RowTest(const Settings &settings, QObject *parent)
    : QObject(parent)
    , settings(settings)
{
}

void RowTest::compare_row(const QAbstractItemModel &model,
                          const QString &id,
                          const QString &name1,
                          const QString &name2,
                          const QString &name3,
                          const QString &name4,
                          const QString &name5,
                          const QString &name6,
                          const QString &name7,
                          const QString &name8)
{
    QCOMPARE(settings.maxNames, 8);

    QStringList names = QStringList{name1, name2, name3, name4, name5, name6, name7, name8};
    int indexFirstEmpty = names.indexOf("");

    QCOMPARE(model.data(model.index(0, IdentifierCol), Qt::DisplayRole), QVariant(id));

    for (int i = 0; i < 8; i++) {
        if (indexFirstEmpty != -1 && i > indexFirstEmpty)
            QCOMPARE(names.at(i), "");

        if (names.at(i).isEmpty())
            QCOMPARE(model.data(model.index(0, i + FirstNameCol), Qt::DisplayRole), QVariant());
        else
            QCOMPARE(model.data(model.index(0, i + FirstNameCol), Qt::DisplayRole), QVariant(names.at(i)));
    }

    names.removeAll("");

    QString allNames = names.join(", ");
    QStringList all;
    all.append(id);
    if (! allNames.isEmpty()) all.append(allNames);
    const QString filename = all.join(" - ") + ".jpg";
    const QFileInfoList files = filesDir.entryInfoList({"*.jpg"}, QDir::Files, QDir::Name);
    QCOMPARE(files.size(), 1);
    QCOMPARE(files.first().fileName(), filename);
}

void RowTest::initTestCase()
{
    QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    rootDir = tmpDir.filePath("bijectif-tests");
    QDir().mkpath(rootDir.path());
    dbDir = rootDir.filePath("db");
    QDir().mkpath(dbDir.path());
    filesDir = rootDir.filePath("files");
    QDir().mkpath(filesDir.path());
    QImage(8, 8, QImage::Format_RGB888).save(filesDir.filePath("image.jpg"));

    const QList<const FileGroup *> fileGroupList = FileGroup::fileGroupList(settings, filesDir);
    const QString dbPath = dbDir.filePath("thumbnails.sqlite");

    Model* tableModel = new Model(settings, new CompleterModel(this));
    tableModel->load(fileGroupList, filesDir, dbPath);

    QVERIFY(QFile::exists(dbPath));
    db = QSqlDatabase::addDatabase("QSQLITE", "update");
    db.setDatabaseName(dbPath);
    QVERIFY(db.open());

    QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel;
    proxyModel->setSourceModel(tableModel);
    proxyModel->sort(1, Qt::AscendingOrder);
    model = proxyModel;

    QCOMPARE(model->columnCount(), FirstNameCol + settings.maxNames);
    QCOMPARE(model->rowCount(), 1);

    compare_row(*model, "image", "", "", "", "", "", "", "", "");
}

void RowTest::test_0()
{
    compare_row(*model, "image", "", "", "", "", "", "", "", "");

    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    undoStack.push(new DeleteCommand(model, model->index(0, FirstNameCol)));
    compare_row(*model, "image", "", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 0);

    undoStack.push(new EditCommand("ABC", model, model->index(0, FirstNameCol)));
    compare_row(*model, "image", "ABC", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.push(new EditCommand("", model, model->index(0, FirstNameCol)));
    compare_row(*model, "image", "ABC", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 0);

    undoStack.push(new EditCommand("ABC", model, model->index(0, FirstNameCol)));
    compare_row(*model, "image", "ABC", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 0);

    undoStack.undo();
    compare_row(*model, "image", "", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 0);

    undoStack.push(new InsertCommand("thing", model, model->index(0, FirstNameCol)));
    compare_row(*model, "image", "thing", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.undo();
    compare_row(*model, "image", "", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 0);

    undoStack.push(new InsertCommand("nothing", model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, "image", "", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 0);

    undoStack.push(new PasteCommand("text1", model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, "image", "", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 0);

    undoStack.push(new PasteCommand("", model, model->index(0, FirstNameCol)));
    compare_row(*model, "image", "", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 0);

    undoStack.push(new PasteCommand("text1", model, model->index(0, FirstNameCol)));
    compare_row(*model, "image", "text1", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.push(new PasteCommand("text1", model, model->index(0, FirstNameCol)));
    compare_row(*model, "image", "text1", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 0);

    undoStack.undo();
    compare_row(*model, "image", "", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 0);

    undoStack.redo();
    compare_row(*model, "image", "text1", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

}

void RowTest::test_1()
{
    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    compare_row(*model, "image", "text1", "", "", "", "", "", "", "");

    undoStack.push(new DeleteCommand(model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, "image", "text1", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 0);

    undoStack.push(new DeleteCommand(model, model->index(0, FirstNameCol)));
    compare_row(*model, "image", "", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 2);

    undoStack.undo();
    compare_row(*model, "image", "text1", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.push(new EditCommand("DEF", model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, "image", "text1", "DEF", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 2);

    undoStack.push(new EditCommand("", model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, "image", "text1", "DEF", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 0);

    undoStack.push(new EditCommand("DEF", model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, "image", "text1", "DEF", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 0);

    undoStack.undo();
    compare_row(*model, "image", "text1", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.push(new InsertCommand("thing", model, model->index(0, FirstNameCol)));
    compare_row(*model, "image", "thing", "text1", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 2);

    undoStack.undo();
    compare_row(*model, "image", "text1", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.push(new InsertCommand("thing", model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, "image", "text1", "thing", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 2);

    undoStack.undo();
    compare_row(*model, "image", "text1", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.push(new PasteCommand("text2", model, model->index(0, FirstNameCol)));
    compare_row(*model, "image", "text2", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 2);

    undoStack.undo();
    compare_row(*model, "image", "text1", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.push(new PasteCommand("text2", model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 2);

    undoStack.undo();
    compare_row(*model, "image", "text1", "", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.redo();
    compare_row(*model, "image", "text1", "text2", "", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 2);
}

void RowTest::test_2()
{
    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    compare_row(*model, "image", "text1", "text2", "", "", "", "", "", "");

    undoStack.push(new PasteCommand("text3", model, model->index(0, 2 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 3);
}

void RowTest::test_3()
{
    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    compare_row(*model, "image", "text1", "text2", "text3", "", "", "", "", "");

    undoStack.push(new PasteCommand("text4", model, model->index(0, 3 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 4);
}

void RowTest::test_4()
{
    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    compare_row(*model, "image", "text1", "text2", "text3", "text4", "", "", "", "");

    undoStack.push(new PasteCommand("text5", model, model->index(0, 4 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 5);
}

void RowTest::test_5()
{
    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "", "", "");

    undoStack.push(new PasteCommand("text6", model, model->index(0, 5 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 6);
}

void RowTest::test_6()
{
    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "", "");

    undoStack.push(new PasteCommand("text7", model, model->index(0, 6 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 7);
}

void RowTest::test_7()
{
    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "");

    undoStack.push(new DeleteCommand(model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "");
    QCOMPARE(spy.count(), 0);

    undoStack.push(new DeleteCommand(model, model->index(0, 6 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.undo();
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 7);

    undoStack.push(new EditCommand("UVW", model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "UVW");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.push(new EditCommand("", model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "UVW");
    QCOMPARE(spy.count(), 0);

    undoStack.push(new EditCommand("UVW", model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "UVW");
    QCOMPARE(spy.count(), 0);

    undoStack.undo();
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 7);

    undoStack.push(new InsertCommand("thing", model, model->index(0, 5 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "thing", "text6", "text7");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.undo();
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 7);

    undoStack.push(new InsertCommand("thing", model, model->index(0, 6 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "thing", "text7");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.undo();
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 7);

    undoStack.push(new InsertCommand("thing", model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "thing");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.undo();
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 7);

    undoStack.push(new PasteCommand("textX", model, model->index(0, 6 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "textX", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.undo();
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 7);

    undoStack.push(new PasteCommand("text8", model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "text8");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.undo();
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 7);

    undoStack.redo();
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "text8");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);
}

void RowTest::test_8()
{
    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "text8");

    undoStack.push(new DeleteCommand(model, model->index(0, 8 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "text8");
    QCOMPARE(spy.count(), 0);

    undoStack.push(new DeleteCommand(model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 9);

    undoStack.undo();
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "text8");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.push(new EditCommand("UVW", model, model->index(0, 8 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "text8");
    QCOMPARE(spy.count(), 0);

    undoStack.push(new EditCommand("", model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "text8");
    QCOMPARE(spy.count(), 0);

    undoStack.push(new EditCommand("UVW", model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "UVW");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 9);

    undoStack.undo();
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "text8");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.push(new InsertCommand("nothing", model, model->index(0, 5 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "text8");
    QCOMPARE(spy.count(), 0);

    undoStack.push(new InsertCommand("nothing", model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "text8");
    QCOMPARE(spy.count(), 0);

    undoStack.push(new InsertCommand("nothing", model, model->index(0, 8 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "text8");
    QCOMPARE(spy.count(), 0);

    undoStack.push(new PasteCommand("textY", model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "textY");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 9);

    undoStack.undo();
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "text8");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.push(new PasteCommand("text8", model, model->index(0, 8 + FirstNameCol)));
    compare_row(*model, "image", "text1", "text2", "text3", "text4", "text5", "text6", "text7", "text8");
    QCOMPARE(spy.count(), 0);
}

void RowTest::cleanupTestCase()
{
    db.close();
    db = QSqlDatabase();

    QSqlDatabase::removeDatabase("update");

    if (rootDir.path().startsWith("/tmp/"))
        rootDir.removeRecursively();
}
