// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "command_delete.h"
#include "command_edit.h"
#include "command_insert.h"
#include "command_paste.h"
#include "model.h"
#include "settings.h"

#include <QAbstractItemModel>
#include <QDir>
#include <QFileInfoList>
#include <QSignalSpy>
#include <QSortFilterProxyModel>
#include <QSqlDatabase>
#include <QStandardPaths>
#include <QTest>
#include <QUndoStack>

using namespace Qt::Literals::StringLiterals;

class RowTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void test_0();
    void test_1();
    void test_2();
    void test_3();
    void test_4();
    void test_5();
    void test_6();
    void test_7();
    void test_8();
    void cleanupTestCase();

private:
    void compare_row(const QAbstractItemModel &model,
                     const QString &id,
                     const QString &name1,
                     const QString &name2,
                     const QString &name3,
                     const QString &name4,
                     const QString &name5,
                     const QString &name6,
                     const QString &name7,
                     const QString &name8);
    void verify_dataChanged(QSignalSpy &spy, int left, int right);
    const Settings settings;
    MemoryCache memoryCache{100*1024*1024};
    QTemporaryDir rootDir;
    QDir dbDir;
    QDir filesDir;
    QAbstractItemModel* model = nullptr;
    QUndoStack undoStack;
    QSqlDatabase db;
};

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
    QCOMPARE(files.constFirst().fileName(), filename);
}

void RowTest::verify_dataChanged(QSignalSpy &spy, int left, int right)
{
    QCOMPARE(spy.signal(), QByteArrayLiteral("dataChanged(QModelIndex,QModelIndex,QList<int>)"));

    spy.removeIf([](const QList<QVariant> &args) {
                     return ! args.at(2).toList().contains(Qt::DisplayRole);
                 });

    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toModelIndex(), model->index(0, left));
    QCOMPARE(arguments.at(1).toModelIndex(), model->index(0, right));

    QCOMPARE_LE(left, right);
    QCOMPARE_GE(left, 0);
    QCOMPARE_LT(right, FirstNameCol + settings.maxNames);
}

void RowTest::initTestCase()
{
    QVERIFY(rootDir.isValid());
    dbDir = rootDir.filePath(u"db"_s);
    QDir().mkpath(dbDir.path());
    filesDir = rootDir.filePath(u"files"_s);
    QDir().mkpath(filesDir.path());
    QImage(8, 8, QImage::Format_RGB888).save(filesDir.filePath(u"image.jpg"_s));

    const QList<const FileGroup *> fileGroupList = FileGroup::fileGroupList(settings, filesDir);
    const QString dbPath = dbDir.filePath(u"thumbnails.sqlite"_s);

    Model* tableModel = new Model(settings, memoryCache, new CompleterModel(this));
    QSignalSpy spyLoadingProgressed(tableModel, &Model::loadingProgressed);
    QVERIFY(spyLoadingProgressed.isValid());
    QSignalSpy spyLoadingFinished(tableModel, &Model::loadingFinished);
    QVERIFY(spyLoadingFinished.isValid());
    tableModel->load(fileGroupList, filesDir, dbPath);
    qDeleteAll(fileGroupList);
    QCOMPARE(spyLoadingProgressed.count(), 3);
    QCOMPARE(spyLoadingProgressed.takeFirst().constFirst().toInt(), 10);
    QCOMPARE(spyLoadingProgressed.takeFirst().constFirst().toInt(), 1);
    QCOMPARE(spyLoadingFinished.count(), 1);

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

    QSignalSpy spyUndoStackIndexChanged(&undoStack, &QUndoStack::indexChanged);
    QSignalSpy spyModelDataChanged(model, &QAbstractItemModel::dataChanged);

    undoStack.push(new DeleteCommand(model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.push(new EditCommand(u"ABC"_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u"ABC"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 1);
    verify_dataChanged(spyModelDataChanged, FirstNameCol, FirstNameCol);

    undoStack.push(new EditCommand(u""_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u"ABC"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.push(new EditCommand(u"ABC"_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u"ABC"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.undo();
    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 0);
    verify_dataChanged(spyModelDataChanged, FirstNameCol, FirstNameCol);

    undoStack.push(new InsertCommand(u"thing"_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u"thing"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 1);
    verify_dataChanged(spyModelDataChanged, FirstNameCol, FirstNameCol);

    undoStack.undo();
    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 0);
    verify_dataChanged(spyModelDataChanged, FirstNameCol, FirstNameCol);

    undoStack.push(new InsertCommand(u"nothing"_s, model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.push(new PasteCommand(u"text1"_s, model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.push(new PasteCommand(u""_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.push(new PasteCommand(u"text1"_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 1);
    verify_dataChanged(spyModelDataChanged, FirstNameCol, FirstNameCol);

    undoStack.push(new PasteCommand(u"text1"_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.undo();
    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 0);
    verify_dataChanged(spyModelDataChanged, FirstNameCol, FirstNameCol);

    undoStack.redo();
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 1);
    verify_dataChanged(spyModelDataChanged, FirstNameCol, FirstNameCol);
}

void RowTest::test_1()
{
    QSignalSpy spyUndoStackIndexChanged(&undoStack, &QUndoStack::indexChanged);
    QSignalSpy spyModelDataChanged(model, &QAbstractItemModel::dataChanged);

    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);

    undoStack.push(new DeleteCommand(model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.push(new DeleteCommand(model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 2);
    verify_dataChanged(spyModelDataChanged, FirstNameCol, FirstNameCol);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 1);
    verify_dataChanged(spyModelDataChanged, FirstNameCol, FirstNameCol);

    undoStack.push(new EditCommand(u"DEF"_s, model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"DEF"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 2);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 1, FirstNameCol + 1);

    undoStack.push(new EditCommand(u""_s, model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"DEF"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.push(new EditCommand(u"DEF"_s, model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"DEF"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 1);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 1, FirstNameCol + 1);

    undoStack.push(new InsertCommand(u"thing"_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u"thing"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 2);
    verify_dataChanged(spyModelDataChanged, FirstNameCol, FirstNameCol + 1);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 1);
    verify_dataChanged(spyModelDataChanged, FirstNameCol, FirstNameCol + 1);

    undoStack.push(new InsertCommand(u"thing"_s, model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"thing"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 2);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 1, FirstNameCol + 1);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 1);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 1, FirstNameCol + 1);

    undoStack.push(new PasteCommand(u"text2"_s, model, model->index(0, FirstNameCol)));
    compare_row(*model, u"image"_s, u"text2"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 2);
    verify_dataChanged(spyModelDataChanged, FirstNameCol, FirstNameCol);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 1);
    verify_dataChanged(spyModelDataChanged, FirstNameCol, FirstNameCol);

    undoStack.push(new PasteCommand(u"text2"_s, model, model->index(0, 1 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 2);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 1, FirstNameCol + 1);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 1);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 1, FirstNameCol + 1);

    undoStack.redo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 2);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 1, FirstNameCol + 1);
}

void RowTest::test_2()
{
    QSignalSpy spyUndoStackIndexChanged(&undoStack, &QUndoStack::indexChanged);
    QSignalSpy spyModelDataChanged(model, &QAbstractItemModel::dataChanged);

    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u""_s, u""_s, u""_s, u""_s, u""_s, u""_s);

    undoStack.push(new PasteCommand(u"text3"_s, model, model->index(0, 2 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u""_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 3);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 2, FirstNameCol + 2);
}

void RowTest::test_3()
{
    QSignalSpy spyUndoStackIndexChanged(&undoStack, &QUndoStack::indexChanged);
    QSignalSpy spyModelDataChanged(model, &QAbstractItemModel::dataChanged);

    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u""_s, u""_s, u""_s, u""_s, u""_s);

    undoStack.push(new PasteCommand(u"text4"_s, model, model->index(0, 3 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u""_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 4);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 3, FirstNameCol + 3);
}

void RowTest::test_4()
{
    QSignalSpy spyUndoStackIndexChanged(&undoStack, &QUndoStack::indexChanged);
    QSignalSpy spyModelDataChanged(model, &QAbstractItemModel::dataChanged);

    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u""_s, u""_s, u""_s, u""_s);

    undoStack.push(new PasteCommand(u"text5"_s, model, model->index(0, 4 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u""_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 5);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 4, FirstNameCol + 4);
}

void RowTest::test_5()
{
    QSignalSpy spyUndoStackIndexChanged(&undoStack, &QUndoStack::indexChanged);
    QSignalSpy spyModelDataChanged(model, &QAbstractItemModel::dataChanged);

    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u""_s, u""_s, u""_s);

    undoStack.push(new PasteCommand(u"text6"_s, model, model->index(0, 5 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 6);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 5, FirstNameCol + 5);
}

void RowTest::test_6()
{
    QSignalSpy spyUndoStackIndexChanged(&undoStack, &QUndoStack::indexChanged);
    QSignalSpy spyModelDataChanged(model, &QAbstractItemModel::dataChanged);

    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u""_s, u""_s);

    undoStack.push(new PasteCommand(u"text7"_s, model, model->index(0, 6 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 7);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 6, FirstNameCol + 6);
}

void RowTest::test_7()
{
    QSignalSpy spyUndoStackIndexChanged(&undoStack, &QUndoStack::indexChanged);
    QSignalSpy spyModelDataChanged(model, &QAbstractItemModel::dataChanged);

    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);

    undoStack.push(new DeleteCommand(model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.push(new DeleteCommand(model, model->index(0, 6 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u""_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 8);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 6, FirstNameCol + 6);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 7);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 6, FirstNameCol + 6);

    undoStack.push(new EditCommand(u"UVW"_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"UVW"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 8);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 7, FirstNameCol + 7);

    undoStack.push(new EditCommand(u""_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"UVW"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.push(new EditCommand(u"UVW"_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"UVW"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 7);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 7, FirstNameCol + 7);

    undoStack.push(new InsertCommand(u"thing"_s, model, model->index(0, 5 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"thing"_s, u"text6"_s, u"text7"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 8);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 5, FirstNameCol + 7);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 7);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 5, FirstNameCol + 7);

    undoStack.push(new InsertCommand(u"thing"_s, model, model->index(0, 6 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"thing"_s, u"text7"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 8);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 6, FirstNameCol + 7);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 7);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 6, FirstNameCol + 7);

    undoStack.push(new InsertCommand(u"thing"_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"thing"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 8);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 7, FirstNameCol + 7);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 7);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 7, FirstNameCol + 7);

    undoStack.push(new PasteCommand(u"textX"_s, model, model->index(0, 6 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"textX"_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 8);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 6, FirstNameCol + 6);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 7);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 6, FirstNameCol + 6);

    undoStack.push(new PasteCommand(u"text8"_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 8);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 7, FirstNameCol + 7);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 7);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 7, FirstNameCol + 7);

    undoStack.redo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 8);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 7, FirstNameCol + 7);
}

void RowTest::test_8()
{
    QSignalSpy spyUndoStackIndexChanged(&undoStack, &QUndoStack::indexChanged);
    QSignalSpy spyModelDataChanged(model, &QAbstractItemModel::dataChanged);

    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);

    undoStack.push(new DeleteCommand(model, model->index(0, 8 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.push(new DeleteCommand(model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u""_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 9);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 7, FirstNameCol + 7);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 8);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 7, FirstNameCol + 7);

    undoStack.push(new EditCommand(u"UVW"_s, model, model->index(0, 8 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.push(new EditCommand(u""_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.push(new EditCommand(u"UVW"_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"UVW"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 9);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 7, FirstNameCol + 7);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 8);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 7, FirstNameCol + 7);

    undoStack.push(new InsertCommand(u"nothing"_s, model, model->index(0, 5 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.push(new InsertCommand(u"nothing"_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.push(new InsertCommand(u"nothing"_s, model, model->index(0, 8 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);

    undoStack.push(new PasteCommand(u"textY"_s, model, model->index(0, 7 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"textY"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 9);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 7, FirstNameCol + 7);

    undoStack.undo();
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 1);
    QCOMPARE(spyUndoStackIndexChanged.takeFirst().constFirst().toInt(), 8);
    verify_dataChanged(spyModelDataChanged, FirstNameCol + 7, FirstNameCol + 7);

    undoStack.push(new PasteCommand(u"text8"_s, model, model->index(0, 8 + FirstNameCol)));
    compare_row(*model, u"image"_s, u"text1"_s, u"text2"_s, u"text3"_s, u"text4"_s, u"text5"_s, u"text6"_s, u"text7"_s, u"text8"_s);
    QCOMPARE(spyUndoStackIndexChanged.count(), 0);
    QCOMPARE(spyModelDataChanged.count(), 0);
}

void RowTest::cleanupTestCase()
{
    db.close();
    db = QSqlDatabase();

    QSqlDatabase::removeDatabase(u"update"_s);

    delete qobject_cast<QSortFilterProxyModel*>(model)->sourceModel();
    delete model;
}

QTEST_MAIN(RowTest)
#include "test_row.moc"
