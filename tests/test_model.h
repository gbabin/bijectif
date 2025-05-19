// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MODELTEST_H
#define MODELTEST_H

#include "../src/settings.h"

#include <QSignalSpy>
#include <QTest>

class ModelTest : public QObject
{
    Q_OBJECT

public:
    static bool createEmptyFile(const QString &path);
    static void keepOnlyDisplayRole(QSignalSpy &spy);
    explicit ModelTest(const Settings &settings, QObject *parent = nullptr);

private Q_SLOTS:
    void init();
    void test_1();
    void test_2();
    void cleanup();

private:
    const Settings& settings;
    QTemporaryDir rootDir;
    QDir dbDir;
    QDir filesDir;
};

#endif // MODELTEST_H
