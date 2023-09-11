// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include <QApplication>
#include <QtTest>

#include "test_model.h"
#include "test_row.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    ModelTest modelTest;
    RowTest rowTest;

    bool test1 = QTest::qExec(&modelTest, argc, argv);
    bool test2 = QTest::qExec(&rowTest, argc, argv);

    return test1 || test2;
}
