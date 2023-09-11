// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MODELTEST_H
#define MODELTEST_H

#include <QTest>

class ModelTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void test_1();
    void cleanupTestCase();

private:
    QDir rootDir;
    QDir dbDir;
    QDir filesDir;
};

#endif // MODELTEST_H
