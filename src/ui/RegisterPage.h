#pragma once
#include "ui/BackgroundWidget.h"

class ApiClient;
class QLineEdit;
class QLabel;
class QPushButton;
class QTimer;

// Экран регистрации — 1:1 с web/register.html + web/js/register.js.
class RegisterPage : public BackgroundWidget {
    Q_OBJECT
public:
    explicit RegisterPage(ApiClient* api, QWidget* parent = nullptr);

signals:
    void goToLogin();
    // Регистрация успешна → MainWindow уводит на вход (как редирект в вебе).
    void registeredOk(const QString& username);

private slots:
    void onUsernameEdited();
    void onSubmit();

private:
    void buildUi();
    void setBusy(bool busy);
    void clearMessages();

    ApiClient*   api_;
    QTimer*      checkTimer_   = nullptr;
    QLineEdit*   username_     = nullptr;
    QLineEdit*   password_     = nullptr;
    QLineEdit*   confirm_      = nullptr;
    QLabel*      usernameErr_  = nullptr;
    QLabel*      usernameOk_   = nullptr;
    QLabel*      passwordErr_  = nullptr;
    QLabel*      confirmErr_   = nullptr;
    QPushButton* submit_       = nullptr;
};
