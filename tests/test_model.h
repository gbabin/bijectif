// Copyright (C) 2023, 2024, 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MODELTEST_H
#define MODELTEST_H

#include "../src/settings.h"

#include <QTest>

class ModelTest : public QObject
{
    Q_OBJECT

public:
    ModelTest(const Settings &settings, QObject *parent = nullptr);

private Q_SLOTS:
    void initTestCase();
    void test_1();
    void cleanupTestCase();

private:
    const Settings settings;
    QDir rootDir;
    QDir dbDir;
    QDir filesDir;
};

#endif // MODELTEST_H
