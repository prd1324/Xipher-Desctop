#pragma once
#include <QMainWindow>

class QStackedWidget;
class QWidget;
class ApiClient;
class LoginPage;
class RegisterPage;
class HomePage;

// Главное окно: переключает экраны вход / регистрация / домашний.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    enum Page { PageSplash = 0, PageLogin = 1, PageRegister = 2, PageHome = 3 };

    void tryRestoreSession();
    QWidget* buildSplash();

    QStackedWidget* stack_   = nullptr;
    ApiClient*      api_     = nullptr;
    QWidget*        splash_  = nullptr;
    LoginPage*      login_   = nullptr;
    RegisterPage*   register_ = nullptr;
    HomePage*       home_    = nullptr;
};
