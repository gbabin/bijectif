// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "window.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QTimer>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationDisplayName("Bijectif");

    const QStringList uiLanguages = QLocale::system().uiLanguages();

    QTranslator qtTranslator;
    for (const QString &locale : uiLanguages) {
        if (qtTranslator.load(QLocale(locale), "qt", "_", QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
            qDebug() << "Loaded translation" << qtTranslator.filePath();
            app.installTranslator(&qtTranslator);
            break;
        }
        qWarning() << "No Qt translation found for" << locale;
    }

    QTranslator appTranslator;
    for (const QString &locale : uiLanguages) {
        if (appTranslator.load(QLocale(locale), "bijectif", "_")) {
            qDebug() << "Loaded translation" << appTranslator.filePath();
            app.installTranslator(&appTranslator);
            break;
        }
        if (appTranslator.load(QLocale(locale), "bijectif", "_", "/usr/share/bijectif/translations/")) {
            qDebug() << "Loaded translation" << appTranslator.filePath();
            app.installTranslator(&appTranslator);
            break;
        }
        qWarning() << "No app translation found for" << locale;
    }
    
    Window window;
    window.show();
    
    QTimer::singleShot(0, &window, &Window::modelLoadingStart);

    return app.exec();
}
