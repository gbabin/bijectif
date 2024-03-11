// Copyright (C) 2023, 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "settings.h"
#include "window.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QLoggingCategory>
#include <QTimer>
#include <QTranslator>

void setLogVerbosity(const Settings &settings)
{
    QString rules;

    if (settings.logVerbosity > 0)
        rules += QStringLiteral("default.critical=true\n");
    else
        rules += QStringLiteral("default.critical=false\n");

    if (settings.logVerbosity > 1)
        rules += QStringLiteral("default.warning=true\n");
    else
        rules += QStringLiteral("default.warning=false\n");

    if (settings.logVerbosity > 2)
        rules += QStringLiteral("default.info=true\n");
    else
        rules += QStringLiteral("default.info=false\n");

    if (settings.logVerbosity > 3)
        rules += QStringLiteral("default.debug=true");
    else
        rules += QStringLiteral("default.debug=false");

    QLoggingCategory::setFilterRules(rules);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationDisplayName(QStringLiteral("Bijectif"));

    const Settings settings;

    setLogVerbosity(settings);

    const QStringList uiLanguages = QLocale::system().uiLanguages();

    QTranslator qtTranslator;
    for (const QString &locale : uiLanguages) {
        if (qtTranslator.load(QLocale(locale), QStringLiteral("qt"), QStringLiteral("_"), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
            if (QApplication::installTranslator(&qtTranslator))
                qDebug() << "Translator install succeeded:" << qtTranslator.filePath();
            else
                qWarning() << "Translator install failed:" << qtTranslator.filePath();
            break;
        }
        qWarning() << "No Qt translation found for" << locale;
    }

    QTranslator appTranslator;
    for (const QString &locale : uiLanguages) {
        if (appTranslator.load(QLocale(locale), QStringLiteral("bijectif"), QStringLiteral("_"))) {
            if (QApplication::installTranslator(&appTranslator))
                qDebug() << "Translator install succeeded:" << appTranslator.filePath();
            else
                qWarning() << "Translator install failed:" << appTranslator.filePath();
            break;
        }
        if (appTranslator.load(QLocale(locale), QStringLiteral("bijectif"), QStringLiteral("_"), QStringLiteral("/usr/share/bijectif/translations/"))) {
            if (QApplication::installTranslator(&appTranslator))
                qDebug() << "Translator install succeeded:" << appTranslator.filePath();
            else
                qWarning() << "Translator install failed:" << appTranslator.filePath();
            break;
        }
        qWarning() << "No app translation found for" << locale;
    }
    
    Window window(settings);
    window.show();
    
    QTimer::singleShot(0, &window, &Window::modelLoadingStart);

    return QApplication::exec();
}
