#pragma once
#include "ui/BackgroundWidget.h"

class ApiClient;
class QLineEdit;
class QLabel;
class QPushButton;

// Экран входа — 1:1 с web/login.html + web/css/login.css.
class LoginPage : public BackgroundWidget {
    Q_OBJECT
public:
    explicit LoginPage(ApiClient* api, QWidget* parent = nullptr);

signals:
    void goToRegister();
    void loginSucceeded();   // Session уже заполнена/сохранена в MainWindow

private slots:
    void onSubmit();

private:
    void buildUi();
    void setBusy(bool busy);
    void clearErrors();

    ApiClient*   api_;
    QLineEdit*   username_   = nullptr;
    QLineEdit*   password_   = nullptr;
    QLabel*      usernameErr_ = nullptr;
    QLabel*      passwordErr_ = nullptr;
    QPushButton* submit_     = nullptr;
};
