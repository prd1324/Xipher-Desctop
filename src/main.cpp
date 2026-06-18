#include <QApplication>
#include <QFont>
#include <QPalette>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDateTime>
#include <QMutex>

#include "app/MainWindow.h"
#include "ui/Theme.h"

// Лог в файл рядом с exe (xipher.log) — GUI-приложение не имеет консоли,
// поэтому диагностика (в т.ч. звонки [call] …) пишется сюда.
static void logToFile(QtMsgType type, const QMessageLogContext&, const QString& msg) {
    static QMutex mutex;
    QMutexLocker lock(&mutex);
    QFile f(QCoreApplication::applicationDirPath() + QStringLiteral("/xipher.log"));
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        const char* lvl = type == QtWarningMsg ? "WARN" : type == QtCriticalMsg ? "ERR " : "INFO";
        QTextStream(&f) << QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"))
                        << ' ' << lvl << ' ' << msg << '\n';
    }
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    qInstallMessageHandler(logToFile);

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
