#include "app/MainWindow.h"
#include "net/ApiClient.h"
#include "net/WsClient.h"
#include "net/Session.h"
#include "ui/LoginPage.h"
#include "ui/RegisterPage.h"
#include "ui/ChatPage.h"
#include "ui/BackgroundWidget.h"
#include "ui/Theme.h"
#include "net/CallController.h"

#include <QStackedWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Xipher"));
    resize(1100, 760);
    setMinimumSize(720, 600);

    api_ = new ApiClient(this);
    ws_  = new WsClient(this);

    stack_    = new QStackedWidget(this);
    splash_   = buildSplash();
    login_    = new LoginPage(api_, this);
    register_ = new RegisterPage(api_, this);
    chat_     = new ChatPage(api_, ws_, this);

    stack_->addWidget(splash_);    // PageSplash
    stack_->addWidget(login_);     // PageLogin
    stack_->addWidget(register_);  // PageRegister
    stack_->addWidget(chat_);      // PageChat
    setCentralWidget(stack_);

    // Навигация между вход ↔ регистрация
    connect(login_, &LoginPage::goToRegister, this, [this]() {
        stack_->setCurrentIndex(PageRegister);
    });
    connect(register_, &RegisterPage::goToLogin, this, [this]() {
        stack_->setCurrentIndex(PageLogin);
    });

    // Успешная регистрация → как в вебе, уводим на экран входа.
    connect(register_, &RegisterPage::registeredOk, this, [this](const QString&) {
        stack_->setCurrentIndex(PageLogin);
    });

    // Успешный вход → мессенджер.
    connect(login_, &LoginPage::loginSucceeded, this, [this]() {
        enterChat();
    });

    // Звонки: оркестратор + поллинг входящих.
    callCtl_ = new CallController(api_, ws_, this, this);
    connect(chat_, &ChatPage::callRequested, this,
            [this](const QString& id, const QString& name, const QString& avatar) {
        callCtl_->startOutgoing(id, name, avatar);
    });
    connect(api_, &ApiClient::incomingCall, this,
            [this](const QString& id, const QString& name, const QString& type) {
        callCtl_->onIncoming(id, name, type);
    });
    callPoll_ = new QTimer(this);
    callPoll_->setInterval(2500);
    connect(callPoll_, &QTimer::timeout, this, [this]() {
        if (Session::instance().isAuthenticated()) api_->checkIncomingCalls();
    });

    // Выход → останавливаем realtime/поллинг, чистим сессию, назад на вход.
    connect(chat_, &ChatPage::logoutRequested, this, [this]() {
        ws_->stop();
        if (callPoll_) callPoll_->stop();
        Session::instance().clear();
        stack_->setCurrentIndex(PageLogin);
    });

    // Восстановление сессии по сохранённому токену.
    connect(api_, &ApiClient::tokenValidated, this, [this](const AuthResult& r) {
        if (r.success) {
            Session& s = Session::instance();
            if (!r.userId.isEmpty())   s.userId = r.userId;
            if (!r.username.isEmpty()) s.username = r.username;
            s.isPremium = r.isPremium;
            s.save();
            enterChat();
        } else {
            Session::instance().clear();
            stack_->setCurrentIndex(PageLogin);
        }
    });

    tryRestoreSession();
}

QWidget* MainWindow::buildSplash() {
    // Нейтральный экран загрузки (как у Telegram при старте), чтобы при
    // автологине не мелькало окно входа.
    auto* page = new BackgroundWidget(this);
    auto* lay = new QVBoxLayout(page);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addStretch();

    auto* icon = new QLabel(QStringLiteral("X"), page);
    icon->setObjectName(QStringLiteral("brandIcon"));
    icon->setFixedSize(64, 64);
    icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet(QStringLiteral("font-size:30px;"));

    auto* name = new QLabel(QStringLiteral("Xipher"), page);
    name->setObjectName(QStringLiteral("brandName"));
    name->setStyleSheet(QStringLiteral("font-size:24px;"));
    name->setAlignment(Qt::AlignHCenter);

    auto* loading = new QLabel(QStringLiteral("Загрузка…"), page);
    loading->setStyleSheet(QStringLiteral("color:#9aa4c6;font-size:14px;"));
    loading->setAlignment(Qt::AlignHCenter);

    lay->addWidget(icon, 0, Qt::AlignHCenter);
    lay->addSpacing(16);
    lay->addWidget(name);
    lay->addSpacing(8);
    lay->addWidget(loading);
    lay->addStretch();
    return page;
}

void MainWindow::enterChat() {
    stack_->setCurrentIndex(PageChat);
    chat_->load();   // грузим чаты + запускаем realtime
    if (callPoll_) callPoll_->start();   // следим за входящими звонками
}

void MainWindow::tryRestoreSession() {
    Session& s = Session::instance();
    s.load();
    if (s.isAuthenticated()) {
        // Есть токен — показываем загрузку и проверяем его в фоне.
        // Окно входа НЕ мелькает: роутинг сделает tokenValidated.
        stack_->setCurrentIndex(PageSplash);
        api_->validateToken(s.token);
    } else {
        stack_->setCurrentIndex(PageLogin);
    }
}
