#include "ui/LoginPage.h"
#include "ui/AuthUi.h"
#include "net/ApiClient.h"
#include "net/Session.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>

LoginPage::LoginPage(ApiClient* api, QWidget* parent)
    : BackgroundWidget(parent), api_(api) {
    buildUi();

    connect(api_, &ApiClient::loginFinished, this, [this](const AuthResult& r) {
        setBusy(false);
        if (r.success) {
            Session& s = Session::instance();
            s.token    = r.token;
            s.userId   = r.userId;
            s.username = r.username.isEmpty() ? username_->text().trimmed() : r.username;
            s.isPremium = r.isPremium;
            s.save();
            emit loginSucceeded();
        } else {
            AuthUi::showError(passwordErr_,
                r.message.isEmpty() ? QStringLiteral("Неверное имя пользователя или пароль")
                                    : r.message);
        }
    });
}

void LoginPage::buildUi() {
    // Внешний слой: центрируем карточку по горизонтали и вертикали.
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addStretch();

    auto* hcenter = new QHBoxLayout();
    hcenter->addStretch();

    QFrame* card = AuthUi::makeCard(this);
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(52, 48, 52, 48);   // ~3rem
    lay->setSpacing(0);

    lay->addWidget(AuthUi::makeBrand(card));
    lay->addSpacing(24);

    lay->addWidget(AuthUi::makeHeading(QStringLiteral("С каким аккаунтом хотите войти?"), card));
    lay->addSpacing(8);
    lay->addWidget(AuthUi::makeSub(
        QStringLiteral("Используйте имя пользователя и пароль, чтобы продолжить."), card));
    lay->addSpacing(28);

    // Поле: имя пользователя
    lay->addWidget(AuthUi::makeFieldLabel(QStringLiteral("Имя пользователя"), card));
    lay->addSpacing(8);
    username_ = AuthUi::makeInput(QStringLiteral("Введите имя пользователя"), card);
    lay->addWidget(username_);
    usernameErr_ = AuthUi::makeErrorLabel(card);
    lay->addWidget(usernameErr_);
    lay->addSpacing(20);

    // Поле: пароль
    lay->addWidget(AuthUi::makeFieldLabel(QStringLiteral("Пароль"), card));
    lay->addSpacing(8);
    password_ = AuthUi::makeInput(QStringLiteral("Введите пароль"), card, /*password*/ true);
    lay->addWidget(password_);
    passwordErr_ = AuthUi::makeErrorLabel(card);
    lay->addWidget(passwordErr_);
    lay->addSpacing(24);

    // Кнопка «Войти»
    submit_ = new QPushButton(QStringLiteral("Войти"), card);
    submit_->setObjectName(QStringLiteral("submitBtn"));
    submit_->setCursor(Qt::PointingHandCursor);
    lay->addWidget(submit_);
    lay->addSpacing(20);

    // Ссылки
    auto* linkRegister = new QPushButton(QStringLiteral("Зарегистрироваться"), card);
    linkRegister->setProperty("class", "linkPrimary");
    linkRegister->setCursor(Qt::PointingHandCursor);
    linkRegister->setFlat(true);

    auto* linkForgot = new QPushButton(QStringLiteral("Забыли пароль?"), card);
    linkForgot->setProperty("class", "linkSecondary");
    linkForgot->setCursor(Qt::PointingHandCursor);
    linkForgot->setFlat(true);

    auto* links = new QVBoxLayout();
    links->setSpacing(8);
    links->addWidget(linkRegister, 0, Qt::AlignHCenter);
    links->addWidget(linkForgot, 0, Qt::AlignHCenter);
    lay->addLayout(links);
    lay->addSpacing(16);

    auto* terms = new QLabel(
        QStringLiteral("Нажимая «Войти», вы принимаете политику конфиденциальности "
                       "и пользовательское соглашение"), card);
    terms->setObjectName(QStringLiteral("authTerms"));
    terms->setWordWrap(true);
    terms->setAlignment(Qt::AlignHCenter);
    lay->addWidget(terms);

    hcenter->addWidget(card);
    hcenter->addStretch();
    outer->addLayout(hcenter);
    outer->addStretch();

    // Поведение
    connect(submit_, &QPushButton::clicked, this, &LoginPage::onSubmit);
    connect(username_, &QLineEdit::returnPressed, this, &LoginPage::onSubmit);
    connect(password_, &QLineEdit::returnPressed, this, &LoginPage::onSubmit);
    connect(linkRegister, &QPushButton::clicked, this, &LoginPage::goToRegister);
    // «Забыли пароль?» — в v0.1 без перехода (заглушка).
}

void LoginPage::clearErrors() {
    AuthUi::showError(usernameErr_, QString());
    AuthUi::showError(passwordErr_, QString());
}

void LoginPage::onSubmit() {
    clearErrors();
    const QString u = username_->text().trimmed();
    const QString p = password_->text();

    if (u.isEmpty()) {
        AuthUi::showError(usernameErr_, QStringLiteral("Введите имя пользователя"));
        return;
    }
    if (p.isEmpty()) {
        AuthUi::showError(passwordErr_, QStringLiteral("Введите пароль"));
        return;
    }

    setBusy(true);
    api_->login(u, p);
}

void LoginPage::setBusy(bool busy) {
    submit_->setDisabled(busy);
    submit_->setText(busy ? QStringLiteral("Вход…") : QStringLiteral("Войти"));
}
