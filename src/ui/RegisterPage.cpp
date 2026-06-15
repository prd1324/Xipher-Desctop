#include "ui/RegisterPage.h"
#include "ui/AuthUi.h"
#include "net/ApiClient.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QTimer>
#include <QRegularExpression>

namespace {
const QRegularExpression kUsernameRe(QStringLiteral("^[a-zA-Z0-9_]+$"));
}

RegisterPage::RegisterPage(ApiClient* api, QWidget* parent)
    : BackgroundWidget(parent), api_(api) {
    checkTimer_ = new QTimer(this);
    checkTimer_->setSingleShot(true);
    checkTimer_->setInterval(500);   // дебаунс как в register.js

    buildUi();

    connect(checkTimer_, &QTimer::timeout, this, [this]() {
        const QString u = username_->text().trimmed();
        if (u.length() >= 3) api_->checkUsername(u);
    });

    connect(api_, &ApiClient::usernameChecked, this,
            [this](const QString& checked, bool available, const QString& message) {
        if (checked != username_->text().trimmed()) return;   // устаревший ответ
        if (available) {
            AuthUi::showError(usernameErr_, QString());
            usernameOk_->setText(QStringLiteral("Имя пользователя доступно"));
            usernameOk_->show();
        } else {
            usernameOk_->hide();
            AuthUi::showError(usernameErr_,
                message.isEmpty() ? QStringLiteral("Имя пользователя уже занято") : message);
        }
    });

    connect(api_, &ApiClient::registerFinished, this, [this](const AuthResult& r) {
        setBusy(false);
        if (r.success) {
            usernameOk_->hide();
            AuthUi::showError(passwordErr_, QString());
            emit registeredOk(username_->text().trimmed());
        } else {
            AuthUi::showError(usernameErr_,
                r.message.isEmpty() ? QStringLiteral("Ошибка при регистрации") : r.message);
        }
    });
}

void RegisterPage::buildUi() {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addStretch();

    auto* hcenter = new QHBoxLayout();
    hcenter->addStretch();

    QFrame* card = AuthUi::makeCard(this);
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(52, 48, 52, 48);
    lay->setSpacing(0);

    lay->addWidget(AuthUi::makeBrand(card));
    lay->addSpacing(24);

    lay->addWidget(AuthUi::makeHeading(QStringLiteral("Создайте аккаунт Xipher"), card));
    lay->addSpacing(8);
    lay->addWidget(AuthUi::makeSub(
        QStringLiteral("Придумайте имя пользователя и пароль, чтобы начать общение."), card));
    lay->addSpacing(28);

    // Имя пользователя
    lay->addWidget(AuthUi::makeFieldLabel(QStringLiteral("Имя пользователя"), card));
    lay->addSpacing(8);
    username_ = AuthUi::makeInput(QStringLiteral("Введите имя пользователя"), card);
    lay->addWidget(username_);
    usernameErr_ = AuthUi::makeErrorLabel(card);
    lay->addWidget(usernameErr_);
    usernameOk_ = AuthUi::makeSuccessLabel(card);
    lay->addWidget(usernameOk_);
    lay->addSpacing(20);

    // Пароль
    lay->addWidget(AuthUi::makeFieldLabel(QStringLiteral("Пароль"), card));
    lay->addSpacing(8);
    password_ = AuthUi::makeInput(QStringLiteral("Введите пароль"), card, true);
    lay->addWidget(password_);
    passwordErr_ = AuthUi::makeErrorLabel(card);
    lay->addWidget(passwordErr_);
    lay->addSpacing(20);

    // Подтверждение пароля
    lay->addWidget(AuthUi::makeFieldLabel(QStringLiteral("Подтвердите пароль"), card));
    lay->addSpacing(8);
    confirm_ = AuthUi::makeInput(QStringLiteral("Повторите пароль"), card, true);
    lay->addWidget(confirm_);
    confirmErr_ = AuthUi::makeErrorLabel(card);
    lay->addWidget(confirmErr_);
    lay->addSpacing(24);

    submit_ = new QPushButton(QStringLiteral("Зарегистрироваться"), card);
    submit_->setObjectName(QStringLiteral("submitBtn"));
    submit_->setCursor(Qt::PointingHandCursor);
    lay->addWidget(submit_);
    lay->addSpacing(20);

    auto* linkLogin = new QPushButton(QStringLiteral("Уже есть аккаунт? Войти"), card);
    linkLogin->setProperty("class", "linkPrimary");
    linkLogin->setCursor(Qt::PointingHandCursor);
    linkLogin->setFlat(true);
    lay->addWidget(linkLogin, 0, Qt::AlignHCenter);
    lay->addSpacing(16);

    auto* terms = new QLabel(
        QStringLiteral("Нажимая «Зарегистрироваться», вы принимаете политику "
                       "конфиденциальности и пользовательское соглашение"), card);
    terms->setObjectName(QStringLiteral("authTerms"));
    terms->setWordWrap(true);
    terms->setAlignment(Qt::AlignHCenter);
    lay->addWidget(terms);

    hcenter->addWidget(card);
    hcenter->addStretch();
    outer->addLayout(hcenter);
    outer->addStretch();

    connect(submit_, &QPushButton::clicked, this, &RegisterPage::onSubmit);
    connect(confirm_, &QLineEdit::returnPressed, this, &RegisterPage::onSubmit);
    connect(username_, &QLineEdit::textChanged, this, &RegisterPage::onUsernameEdited);
    connect(linkLogin, &QPushButton::clicked, this, &RegisterPage::goToLogin);
}

void RegisterPage::onUsernameEdited() {
    const QString u = username_->text().trimmed();
    usernameOk_->hide();
    checkTimer_->stop();

    if (u.isEmpty()) {
        AuthUi::showError(usernameErr_, QString());
        return;
    }
    if (!kUsernameRe.match(u).hasMatch()) {
        AuthUi::showError(usernameErr_,
            QStringLiteral("Имя пользователя может содержать только буквы, цифры и подчёркивания"));
        return;
    }
    if (u.length() < 3) {
        AuthUi::showError(usernameErr_,
            QStringLiteral("Имя пользователя должно содержать минимум 3 символа"));
        return;
    }
    if (u.length() > 50) {
        AuthUi::showError(usernameErr_,
            QStringLiteral("Имя пользователя не должно превышать 50 символов"));
        return;
    }
    AuthUi::showError(usernameErr_, QString());
    checkTimer_->start();   // дебаунс-проверка доступности
}

void RegisterPage::clearMessages() {
    AuthUi::showError(usernameErr_, QString());
    AuthUi::showError(passwordErr_, QString());
    AuthUi::showError(confirmErr_, QString());
}

void RegisterPage::onSubmit() {
    clearMessages();
    const QString u = username_->text().trimmed();
    const QString p = password_->text();
    const QString c = confirm_->text();

    bool hasError = false;
    if (u.length() < 3 || u.length() > 50) {
        AuthUi::showError(usernameErr_,
            QStringLiteral("Имя пользователя должно содержать от 3 до 50 символов"));
        hasError = true;
    }
    if (!kUsernameRe.match(u).hasMatch()) {
        AuthUi::showError(usernameErr_,
            QStringLiteral("Имя пользователя может содержать только буквы, цифры и подчёркивания"));
        hasError = true;
    }
    if (p.length() < 6) {
        AuthUi::showError(passwordErr_,
            QStringLiteral("Пароль должен содержать минимум 6 символов"));
        hasError = true;
    }
    if (p != c) {
        AuthUi::showError(confirmErr_, QStringLiteral("Пароли не совпадают"));
        hasError = true;
    }
    if (hasError) return;

    setBusy(true);
    api_->registerUser(u, p);
}

void RegisterPage::setBusy(bool busy) {
    submit_->setDisabled(busy);
    submit_->setText(busy ? QStringLiteral("Регистрация…") : QStringLiteral("Зарегистрироваться"));
}
