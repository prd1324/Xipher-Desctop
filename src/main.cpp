#include <QApplication>
#include <QFont>
#include <QPalette>

#include "app/MainWindow.h"
#include "ui/Theme.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    // Имена для QSettings (HKCU\Software\Xipher\Desktop).
    QApplication::setOrganizationName(QStringLiteral("Xipher"));
    QApplication::setApplicationName(QStringLiteral("Desktop"));
    QApplication::setApplicationDisplayName(QStringLiteral("Xipher"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    // Базовый шрифт. Inter если установлен в системе, иначе Segoe UI.
    QFont base(QStringLiteral("Inter"));
    base.setStyleStrategy(QFont::PreferAntialias);
    if (base.exactMatch() == false) base.setFamily(QStringLiteral("Segoe UI"));
    base.setPixelSize(14);
    app.setFont(base);

    // Тёмная палитра ДО создания окна — иначе при старте мелькает белый фон,
    // пока не отрисуется тёмный градиент (классическая «вспышка» Qt на Windows).
    QPalette pal = app.palette();
    pal.setColor(QPalette::Window,     Theme::BgBase);
    pal.setColor(QPalette::Base,       Theme::BgBase);
    pal.setColor(QPalette::WindowText, Theme::TextMain);
    pal.setColor(QPalette::Text,       Theme::TextMain);
    app.setPalette(pal);

    // Глобальный стиль (палитра/QSS из веб-клиента).
    app.setStyleSheet(Theme::styleSheet());

    MainWindow w;
    w.show();
    return app.exec();
}
