// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "test_model.h"
#include "test_row.h"
#include "../src/settings.h"

#include <QApplication>
#include <QTest>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    const Settings settings;

    ModelTest modelTest(settings);
    RowTest rowTest(settings);

    const bool okModel = QTest::qExec(&modelTest, argc, argv) == 0;
    const bool okRow = QTest::qExec(&rowTest, argc, argv) == 0;

    if (okModel && okRow)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}
