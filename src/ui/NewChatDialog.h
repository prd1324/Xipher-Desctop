#pragma once
#include <QDialog>
#include "net/Models.h"

class ApiClient;
class QLineEdit;
class QListWidget;
class QLabel;
class QTimer;

// ─────────────────────────────────────────────────────────────────────────────
//  NewChatDialog — поиск пользователей (/api/search-users), отправка заявок в
//  друзья и приём входящих заявок. Выбор «Написать» открывает чат в ChatPage.
// ─────────────────────────────────────────────────────────────────────────────
class NewChatDialog : public QDialog {
    Q_OBJECT
public:
    explicit NewChatDialog(ApiClient* api, QWidget* parent = nullptr);

signals:
    void openChatRequested(const QString& userId, const QString& displayName, const QString& username);
    void friendsChanged();   // приняли заявку → ChatPage перезагрузит чаты

private slots:
    void onSearchChanged();
    void runSearch();
    void onUsersFound(const QString& query, const QList<UserHit>& users);
    void onFriendRequestSent(const QString& username, bool ok, const QString& message);
    void onRequestsLoaded(const QList<FriendRequest>& requests);
    void onFriendActionDone(const QString& requestId, bool accepted, bool ok);

private:
    void buildUi();

    ApiClient*   api_;
    QLineEdit*   search_   = nullptr;
    QListWidget* results_  = nullptr;
    QListWidget* requests_ = nullptr;
    QLabel*      status_   = nullptr;
    QLabel*      reqTitle_ = nullptr;
    QTimer*      debounce_ = nullptr;
    QString      lastQuery_;
};
