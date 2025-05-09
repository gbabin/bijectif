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

using namespace Qt::Literals::StringLiterals;

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
    const int indexFirstEmpty = names.indexOf(u""_s);

    QCOMPARE(model.data(model.index(0, IdentifierCol), Qt::DisplayRole), QVariant(id));

    for (int i = 0; i < 8; i++) {
        if (indexFirstEmpty != -1 && i > indexFirstEmpty)
            QCOMPARE(names.at(i), u""_s);

        if (names.at(i).isEmpty())
            QCOMPARE(model.data(model.index(0, i + FirstNameCol), Qt::DisplayRole), QVariant());
        else
            QCOMPARE(model.data(model.index(0, i + FirstNameCol), Qt::DisplayRole), QVariant(names.at(i)));
    }

    names.removeAll(u""_s);

    const QString allNames = names.join(u", "_s);
    QStringList all;
    all.append(id);
    if (! allNames.isEmpty()) all.append(allNames);
    const QString filename = all.join(u" - "_s) + u".jpg"_s;
    const QFileInfoList files = filesDir.entryInfoList({u"*.jpg"_s}, QDir::Files, QDir::Name);
    QCOMPARE(files.size(), 1);
    QCOMPARE(files.first().fileName(), filename);
}

void RowTest::initTestCase()
{
    const QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    rootDir = tmpDir.filePath(u"bijectif-tests"_s);
    QDir().mkpath(rootDir.path());
    dbDir = rootDir.filePath(u"db"_s);
    QDir().mkpath(dbDir.path());
    filesDir = rootDir.filePath(u"files"_s);
    QDir().mkpath(filesDir.path());
    QImage(8, 8, QImage::Format_RGB888).save(filesDir.filePath(u"image.jpg"_s));

    const QList<const FileGroup *> fileGroupList = FileGroup::fileGroupList(settings, filesDir);
    const QString dbPath = dbDir.filePath(u"thumbnails.sqlite"_s);

    Model* tableModel = new Model(settings, new CompleterModel(this));
    tableModel->load(fileGroupList, filesDir, dbPath);

    QVERIFY(QFile::exists(dbPath));
    db = QSqlDatabase::addDatabase(u"QSQLITE"_s, u"update"_s);
    db.setDatabaseName(dbPath);
    QVERIFY(db.open());

    QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel;
    proxyModel->setSourceModel(tableModel);
    proxyModel->sort(1, Qt::AscendingOrder);
    model = proxyModel;

    QCOMPARE(model->columnCount(), FirstNameCol + settings.maxNames);
    QCOMPARE(model->rowCount(), 1);

    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
}

void RowTest::test_0()
{
    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);

    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    undoStack.push(new DeleteCommand(model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 0);

    undoStack.push(new EditCommand(u"ABC"_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u"ABC"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.push(new EditCommand(u""_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u"ABC"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 0);

    undoStack.push(new EditCommand(u"ABC"_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u"ABC"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 0);

    undoStack.undo();
    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 0);

    undoStack.push(new InsertCommand(u"thing"_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u"thing"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.undo();
    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 0);

    undoStack.push(new InsertCommand(u"nothing"_s, model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 0);

    undoStack.push(new PasteCommand(u"text1"_s, model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 0);

    undoStack.push(new PasteCommand(u""_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 0);

    undoStack.push(new PasteCommand(u"text1"_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.push(new PasteCommand(u"text1"_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 0);

    undoStack.undo();
    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 0);

    undoStack.redo();
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

}

void RowTest::test_1()
{
    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);

    undoStack.push(new DeleteCommand(model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 0);

    undoStack.push(new DeleteCommand(model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 2);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.push(new EditCommand(u"DEF"_s, model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"DEF"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 2);

    undoStack.push(new EditCommand(u""_s, model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"DEF"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 0);

    undoStack.push(new EditCommand(u"DEF"_s, model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"DEF"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 0);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.push(new InsertCommand(u"thing"_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u"thing"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 2);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.push(new InsertCommand(u"thing"_s, model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"thing"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 2);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.push(new PasteCommand(u"text2"_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u"text2"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 2);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.push(new PasteCommand(u"text2"_s, model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 2);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 1);

    undoStack.redo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 2);
}

void RowTest::test_2()
{
    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);

    undoStack.push(new PasteCommand(u"text3"_s, model, model->index(0, 2 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 3);
}

void RowTest::test_3()
{
    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u""_s, u""_s, u""_s, u""_s, u""_s);

    undoStack.push(new PasteCommand(u"text4"_s, model, model->index(0, 3 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 4);
}

void RowTest::test_4()
{
    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u""_s, u""_s, u""_s, u""_s);

    undoStack.push(new PasteCommand(u"text5"_s, model, model->index(0, 4 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u""_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 5);
}

void RowTest::test_5()
{
    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u""_s, u""_s, u""_s);

    undoStack.push(new PasteCommand(u"text6"_s, model, model->index(0, 5 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 6);
}

void RowTest::test_6()
{
    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u""_s, u""_s);

    undoStack.push(new PasteCommand(u"text7"_s, model, model->index(0, 6 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 7);
}

void RowTest::test_7()
{
    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);

    undoStack.push(new DeleteCommand(model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spy.count(), 0);

    undoStack.push(new DeleteCommand(model, model->index(0, 6 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u""_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 7);

    undoStack.push(new EditCommand(u"UVW"_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"UVW"_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.push(new EditCommand(u""_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"UVW"_s);
    QCOMPARE(spy.count(), 0);

    undoStack.push(new EditCommand(u"UVW"_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"UVW"_s);
    QCOMPARE(spy.count(), 0);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 7);

    undoStack.push(new InsertCommand(u"thing"_s, model, model->index(0, 5 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"thing"_s, u"text6"_s, u"text7"_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 7);

    undoStack.push(new InsertCommand(u"thing"_s, model, model->index(0, 6 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"thing"_s, u"text7"_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 7);

    undoStack.push(new InsertCommand(u"thing"_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"thing"_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 7);

    undoStack.push(new PasteCommand(u"textX"_s, model, model->index(0, 6 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"textX"_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 7);

    undoStack.push(new PasteCommand(u"text8"_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 7);

    undoStack.redo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);
}

void RowTest::test_8()
{
    QSignalSpy spy(&undoStack, &QUndoStack::indexChanged);

    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);

    undoStack.push(new DeleteCommand(model, model->index(0, 8 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spy.count(), 0);

    undoStack.push(new DeleteCommand(model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 9);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.push(new EditCommand(u"UVW"_s, model, model->index(0, 8 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spy.count(), 0);

    undoStack.push(new EditCommand(u""_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spy.count(), 0);

    undoStack.push(new EditCommand(u"UVW"_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"UVW"_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 9);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.push(new InsertCommand(u"nothing"_s, model, model->index(0, 5 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spy.count(), 0);

    undoStack.push(new InsertCommand(u"nothing"_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spy.count(), 0);

    undoStack.push(new InsertCommand(u"nothing"_s, model, model->index(0, 8 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spy.count(), 0);

    undoStack.push(new PasteCommand(u"textY"_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"textY"_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 9);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().constFirst().toInt(), 8);

    undoStack.push(new PasteCommand(u"text8"_s, model, model->index(0, 8 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spy.count(), 0);
}

void RowTest::cleanupTestCase()
{
    db.close();
    db = QSqlDatabase();

    QSqlDatabase::removeDatabase(u"update"_s);

    if (rootDir.path().startsWith(u"/tmp/"_s))
        rootDir.removeRecursively();
}
