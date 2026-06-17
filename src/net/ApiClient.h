#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <functional>

#include "net/Models.h"

class QNetworkAccessManager;
class QNetworkReply;

// Результат запроса аутентификации (login / register / validate-token).
struct AuthResult {
    bool    success = false;
    QString message;
    QString token;
    QString userId;
    QString username;
    bool    isPremium = false;
};

// ─────────────────────────────────────────────────────────────────────────────
//  ApiClient — обёртка над прод-API https://messenger.xipher.pro.
//  Эндпоинты повторяют web/js/login.js и web/js/register.js:
//    POST /api/login           {username,password} → {success,message,data:{...}}
//    POST /api/register        {username,password} → {success,message}
//    POST /api/check-username  {username}          → {success,message}
//    POST /api/validate-token  {token}             → {success,data:{...}}
// ─────────────────────────────────────────────────────────────────────────────
class ApiClient : public QObject {
    Q_OBJECT
public:
    explicit ApiClient(QObject* parent = nullptr);

    void login(const QString& username, const QString& password);
    void registerUser(const QString& username, const QString& password);
    void checkUsername(const QString& username);
    void validateToken(const QString& token);

    // Чат (токен берётся из Session).
    void getChats();
    void getMessages(const QString& friendId);
    void sendMessage(const QString& receiverId, const QString& content, const QString& tempId);

    // Голосовые / файлы.
    void uploadVoice(const QByteArray& audioBytes, const QString& mimeType, const QString& tempId);
    void sendVoice(const QString& receiverId, const QString& filePath, const QString& fileName,
                   long long fileSize, const QString& caption, const QString& tempId);
    void fetchFile(const QString& filePath);   // GET /files/... с токеном

    // Люди и друзья.
    void searchUsers(const QString& query);
    void sendFriendRequest(const QString& username);
    void getFriendRequests();
    void acceptFriend(const QString& requestId);
    void rejectFriend(const QString& requestId);

    QString baseUrl() const { return base_; }
    void setBaseUrl(const QString& url) { base_ = url; }

signals:
    void loginFinished(const AuthResult& result);
    void registerFinished(const AuthResult& result);
    void usernameChecked(const QString& username, bool available, const QString& message);
    void tokenValidated(const AuthResult& result);

    void chatsLoaded(const QList<Chat>& chats);
    void messagesLoaded(const QString& friendId, const QList<ChatMessage>& messages);
    void messageSent(const ChatMessage& message, const QString& receiverId, const QString& tempId);
    void chatError(const QString& context, const QString& message);

    // voiceUploaded → отдаёт путь на сервере; затем зовём sendVoice.
    void voiceUploaded(const QString& filePath, const QString& fileName, long long fileSize, const QString& tempId);
    void fileFetched(const QString& filePath, const QByteArray& bytes);

    void usersFound(const QString& query, const QList<UserHit>& users);
    void friendRequestSent(const QString& username, bool ok, const QString& message);
    void friendRequestsLoaded(const QList<FriendRequest>& requests);
    void friendActionDone(const QString& requestId, bool accepted, bool ok);

private:
    // Низкоуровневый POST JSON. callback(obj, ok, networkError).
    void postJson(const QString& path,
                  const QJsonObject& body,
                  std::function<void(const QJsonObject&, bool, const QString&)> callback);

    static AuthResult parseAuth(const QJsonObject& obj);

    QNetworkAccessManager* nam_;
    QString base_ = QStringLiteral("https://messenger.xipher.pro");
};
