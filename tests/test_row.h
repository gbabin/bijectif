// Copyright (C) 2023, 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ROWTEST_H
#define ROWTEST_H

#include "../src/settings.h"

#include <QAbstractItemModel>
#include <QDir>
#include <QSqlDatabase>
#include <QUndoStack>

class RowTest : public QObject
{
    Q_OBJECT

public:
    RowTest(const Settings &settings, QObject *parent = nullptr);

private slots:
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
    const Settings settings;
    QDir rootDir;
    QDir dbDir;
    QDir filesDir;
    QAbstractItemModel* model = nullptr;
    QUndoStack undoStack;
    QSqlDatabase db;
};

#endif // ROWTEST_H
