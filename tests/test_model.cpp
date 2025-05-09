// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "test_model.h"

#include "../src/command_delete.h"
#include "../src/command_edit.h"
#include "../src/command_id.h"
#include "../src/model.h"

#include <QFileInfoList>
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

void ModelTest::initTestCase()
{
    const QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    rootDir = tmpDir.filePath(u"bijectif-tests"_s);
    QDir().mkpath(rootDir.path());
    dbDir = rootDir.filePath(u"db"_s);
    QDir().mkpath(dbDir.path());
    filesDir = rootDir.filePath(u"files"_s);
    QDir().mkpath(filesDir.path());
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
}

void ModelTest::test_1()
{
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

void ModelTest::cleanupTestCase()
{
    QSqlDatabase::removeDatabase(u"update"_s);

    if (rootDir.path().startsWith(u"/tmp/"_s))
        rootDir.removeRecursively();
}
