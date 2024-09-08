// Copyright (C) 2023, 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "settings.h"
#include "window.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QLoggingCategory>
#include <QTimer>
#include <QTranslator>

using namespace Qt::Literals::StringLiterals;

void setLogVerbosity(const Settings &settings)
{
    QString rules;

    if (settings.logVerbosity > 0)
        rules += u"default.critical=true\n"_s;
    else
        rules += u"default.critical=false\n"_s;

    if (settings.logVerbosity > 1)
        rules += u"default.warning=true\n"_s;
    else
        rules += u"default.warning=false\n"_s;

    if (settings.logVerbosity > 2)
        rules += u"default.info=true\n"_s;
    else
        rules += u"default.info=false\n"_s;

    if (settings.logVerbosity > 3)
        rules += u"default.debug=true"_s;
    else
        rules += u"default.debug=false"_s;

    QLoggingCategory::setFilterRules(rules);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationDisplayName(u"Bijectif"_s);

    const Settings settings;

    setLogVerbosity(settings);

    const QStringList uiLanguages = QLocale::system().uiLanguages();

    QTranslator qtTranslator;
    for (const QString &locale : uiLanguages) {
        if (qtTranslator.load(QLocale(locale), u"qt"_s, u"_"_s, QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
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
        if (appTranslator.load(QLocale(locale), u"bijectif"_s, u"_"_s)) {
            if (QApplication::installTranslator(&appTranslator))
                qDebug() << "Translator install succeeded:" << appTranslator.filePath();
            else
                qWarning() << "Translator install failed:" << appTranslator.filePath();
            break;
        }
        if (appTranslator.load(QLocale(locale), u"bijectif"_s, u"_"_s, u"/usr/share/bijectif/translations/"_s)) {
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
