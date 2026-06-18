#pragma once
#include <QMainWindow>

class QStackedWidget;
class QWidget;
class ApiClient;
class WsClient;
class CallController;
class LoginPage;
class RegisterPage;
class ChatPage;
class QTimer;
class QSystemTrayIcon;

// Главное окно: переключает экраны вход / регистрация / домашний.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    enum Page { PageSplash = 0, PageLogin = 1, PageRegister = 2, PageChat = 3 };

    void tryRestoreSession();
    void enterChat();
    QWidget* buildSplash();

    QStackedWidget* stack_   = nullptr;
    ApiClient*      api_     = nullptr;
    WsClient*       ws_      = nullptr;
    QWidget*        splash_  = nullptr;
    LoginPage*      login_   = nullptr;
    RegisterPage*   register_ = nullptr;
    ChatPage*       chat_    = nullptr;
    CallController* callCtl_ = nullptr;
    QTimer*         callPoll_ = nullptr;
    QSystemTrayIcon* tray_   = nullptr;
};
