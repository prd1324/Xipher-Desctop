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
    void sendMessage(const QString& receiverId, const QString& content, const QString& tempId,
                     int ttlSeconds = 0);
    // Универсальная отправка с произвольным message_type (location/checklist/…).
    void sendRaw(const QString& receiverId, const QString& content,
                 const QString& messageType, const QString& tempId);

    // Голосовые / файлы.
    void uploadVoice(const QByteArray& audioBytes, const QString& mimeType, const QString& tempId);
    void sendVoice(const QString& receiverId, const QString& filePath, const QString& fileName,
                   long long fileSize, const QString& caption, const QString& tempId);
    void uploadFile(const QByteArray& bytes, const QString& fileName, const QString& tempId);
    void sendFile(const QString& receiverId, const QString& filePath, const QString& fileName,
                  long long fileSize, const QString& caption, const QString& tempId);
    void fetchFile(const QString& filePath);   // GET /files/... с токеном

    // Звонки (сигналинг)
    void getTurnConfig();
    void callNotify(const QString& receiverId, const QString& callType);
    void callOffer(const QString& receiverId, const QString& sdp);
    void callAnswer(const QString& receiverId, const QString& sdp);
    void callIce(const QString& receiverId, const QString& candidate);
    void callEnd(const QString& receiverId);
    void getCallOffer(const QString& callerId);
    void getCallAnswer(const QString& calleeId);
    void getCallIce(const QString& otherId);
    void checkIncomingCalls();

    // Группы и каналы.
    void getGroups();
    void getChannels();
    void createGroup(const QString& name, const QString& description);
    void createChannel(const QString& name, const QString& description, const QString& customLink);
    void uploadChannelAvatar(const QString& channelId, const QByteArray& bytes, const QString& fileName);
    void getGroupMessages(const QString& groupId);
    void getChannelMessages(const QString& channelId);
    void sendGroupMessage(const QString& groupId, const QString& content, const QString& tempId);
    void sendChannelMessage(const QString& channelId, const QString& content, const QString& tempId);
    void publicDirectory(const QString& category, const QString& search, int offset);
    void joinPublic(const QString& id, const QString& type);   // type: "channel"|"group"

    // Управление каналом/группой.
    void getChannelInfo(const QString& channelId);
    void getMembers(const QString& peerId, bool isChannel);
    void updatePeerName(const QString& peerId, bool isChannel, const QString& name);
    void updatePeerDescription(const QString& peerId, bool isChannel, const QString& desc);
    void setPeerCustomLink(const QString& peerId, bool isChannel, const QString& link);
    void unsubscribeChannel(const QString& channelId);
    void leaveGroup(const QString& groupId);
    void deleteChannel(const QString& channelId);
    void deleteGroup(const QString& groupId);
    void createInvite(const QString& peerId, bool isChannel);
    void kickGroupMember(const QString& groupId, const QString& userId);
    void setGroupMemberRole(const QString& groupId, const QString& userId, const QString& role);
    void banChannelMember(const QString& channelId, const QString& userId, bool banned);

    // Папки чатов (синхронизация с сервером).
    void getChatFolders();
    void setChatFolders(const QList<Folder>& folders);

    // Настройки: профиль, приватность, сеансы, email восстановления.
    void getMyProfile();
    void updateMyProfile(const QString& firstName, const QString& lastName, const QString& bio,
                         int birthDay, int birthMonth, int birthYear);
    void uploadAvatar(const QByteArray& bytes, const QString& fileName);
    void updateMyPrivacy(const QJsonObject& fields);
    void getSessions();
    void revokeSession(const QString& sessionId);
    void revokeSelectedSessions(const QStringList& ids);
    void revokeOtherSessions();
    void getRecoveryEmail();
    void setRecoveryEmail(const QString& email);

    // Люди и друзья.
    void getUserProfile(const QString& userId);
    void setContactName(const QString& contactId, const QString& customName);
    void searchUsers(const QString& query);
    void getFriends();
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
    void fileUploaded(const QString& filePath, const QString& fileName, long long fileSize, const QString& tempId);
    void fileFetched(const QString& filePath, const QByteArray& bytes);

    void usersFound(const QString& query, const QList<UserHit>& users);
    void friendsLoaded(const QList<UserHit>& friends);
    void profileLoaded(const QJsonObject& profile);

    // Группы/каналы/каталог.
    void groupsLoaded(const QList<Chat>& groups);
    void channelsLoaded(const QList<Chat>& channels);
    void groupCreated(bool ok, const QString& message);
    void channelCreated(bool ok, const QString& message);
    void channelAvatarUploaded(bool ok);
    void groupMessagesLoaded(const QString& groupId, const QList<ChatMessage>& messages);
    void channelMessagesLoaded(const QString& channelId, const QList<ChatMessage>& messages);
    void directoryLoaded(const QList<DirectoryItem>& items, bool hasMore);
    void publicJoined(bool ok, const QString& id);
    void foldersLoaded(const QList<Folder>& folders);
    void foldersSaved(bool ok);
    void channelInfoLoaded(const QJsonObject& info);
    void membersLoaded(const QString& peerId, const QList<Member>& members);
    void peerActionDone(bool ok, const QString& message);
    void inviteLinkReady(const QString& url);

    // Настройки.
    void profileUpdated(bool ok, const QString& message);
    void avatarUploaded(bool ok, const QString& avatarUrl);
    void privacyUpdated(bool ok);
    void sessionsLoaded(const QList<SessionInfo>& sessions);
    void sessionsChanged();
    void recoveryEmailLoaded(const QString& email);
    void recoveryEmailSaved(bool ok, const QString& message);
    void contactRenamed(const QString& contactId, const QString& newName, bool ok);

    void turnConfigReady(const QList<IceServerCfg>& iceServers);
    void callOfferReady(const QString& callerId, const QString& sdp);
    void callAnswerReady(const QString& calleeId, const QString& sdp);
    void callIceBatch(const QString& otherId, const QStringList& candidates);
    void incomingCall(const QString& callerId, const QString& callerName, const QString& callType);
    void friendRequestSent(const QString& username, bool ok, const QString& message);
    void friendRequestsLoaded(const QList<FriendRequest>& requests);
    void friendActionDone(const QString& requestId, bool accepted, bool ok);

private:
    // Низкоуровневый POST JSON. callback(obj, ok, networkError).
    void postJson(const QString& path,
                  const QJsonObject& body,
                  std::function<void(const QJsonObject&, bool, const QString&)> callback);

    static AuthResult parseAuth(const QJsonObject& obj);

    // Fallback на /api/turn-config (как у веба), если turn-credentials недоступен.
    void getTurnConfigFallback();

    QNetworkAccessManager* nam_;
    QString base_ = QStringLiteral("https://messenger.xipher.pro");
};
