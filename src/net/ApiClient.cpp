#include "net/ApiClient.h"
#include "net/Session.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

ApiClient::ApiClient(QObject* parent)
    : QObject(parent), nam_(new QNetworkAccessManager(this)) {}

void ApiClient::postJson(const QString& path,
                         const QJsonObject& body,
                         std::function<void(const QJsonObject&, bool, const QString&)> callback) {
    QNetworkRequest req(QUrl(base_ + path));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Accept", "application/json");

    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
    QNetworkReply* reply = nam_->post(req, payload);

    QObject::connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        reply->deleteLater();
        const QByteArray data = reply->readAll();
        const QString netErr = reply->error() == QNetworkReply::NoError
                                   ? QString()
                                   : reply->errorString();

        QJsonParseError pe{};
        const QJsonDocument doc = QJsonDocument::fromJson(data, &pe);
        if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
            // Нет валидного JSON — отдаём то, что есть, как сетевую/серверную ошибку.
            callback(QJsonObject{}, false,
                     netErr.isEmpty() ? QStringLiteral("Некорректный ответ сервера") : netErr);
            return;
        }
        callback(doc.object(), netErr.isEmpty(), netErr);
    });
}

AuthResult ApiClient::parseAuth(const QJsonObject& obj) {
    AuthResult r;
    r.success = obj.value(QStringLiteral("success")).toBool(false);
    r.message = obj.value(QStringLiteral("message")).toString();
    const QJsonObject data = obj.value(QStringLiteral("data")).toObject();
    r.token     = data.value(QStringLiteral("token")).toString();
    r.userId    = data.value(QStringLiteral("user_id")).toString();
    r.username  = data.value(QStringLiteral("username")).toString();
    r.isPremium = data.value(QStringLiteral("is_premium")).toString() == QStringLiteral("true");
    return r;
}

void ApiClient::login(const QString& username, const QString& password) {
    QJsonObject body{{QStringLiteral("username"), username},
                     {QStringLiteral("password"), password}};
    postJson(QStringLiteral("/api/login"), body,
             [this](const QJsonObject& obj, bool ok, const QString& netErr) {
        if (!ok && obj.isEmpty()) {
            AuthResult r;
            r.success = false;
            r.message = QStringLiteral("Ошибка соединения с сервером: ") + netErr;
            emit loginFinished(r);
            return;
        }
        emit loginFinished(parseAuth(obj));
    });
}

void ApiClient::registerUser(const QString& username, const QString& password) {
    QJsonObject body{{QStringLiteral("username"), username},
                     {QStringLiteral("password"), password}};
    postJson(QStringLiteral("/api/register"), body,
             [this](const QJsonObject& obj, bool ok, const QString& netErr) {
        if (!ok && obj.isEmpty()) {
            AuthResult r;
            r.success = false;
            r.message = QStringLiteral("Ошибка соединения с сервером: ") + netErr;
            emit registerFinished(r);
            return;
        }
        emit registerFinished(parseAuth(obj));
    });
}

void ApiClient::checkUsername(const QString& username) {
    QJsonObject body{{QStringLiteral("username"), username}};
    postJson(QStringLiteral("/api/check-username"), body,
             [this, username](const QJsonObject& obj, bool ok, const QString&) {
        if (!ok && obj.isEmpty()) {
            // Сетевую ошибку при проверке ника не показываем как «занят».
            return;
        }
        const bool available = obj.value(QStringLiteral("success")).toBool(false);
        const QString message = obj.value(QStringLiteral("message")).toString();
        emit usernameChecked(username, available, message);
    });
}

void ApiClient::validateToken(const QString& token) {
    QJsonObject body{{QStringLiteral("token"), token}};
    postJson(QStringLiteral("/api/validate-token"), body,
             [this](const QJsonObject& obj, bool ok, const QString& netErr) {
        if (!ok && obj.isEmpty()) {
            AuthResult r;
            r.success = false;
            r.message = netErr;
            emit tokenValidated(r);
            return;
        }
        emit tokenValidated(parseAuth(obj));
    });
}

// ── Чат ──────────────────────────────────────────────────────────────────────

static Chat parseChat(const QJsonObject& o) {
    Chat c;
    c.id          = o.value(QStringLiteral("id")).toString();
    c.name        = o.value(QStringLiteral("name")).toString();
    c.displayName = o.value(QStringLiteral("display_name")).toString();
    if (c.displayName.isEmpty()) c.displayName = c.name;
    c.avatarUrl   = o.value(QStringLiteral("avatar_url")).toString();
    c.avatarText  = o.value(QStringLiteral("avatar")).toString();
    c.lastMessage = o.value(QStringLiteral("lastMessage")).toString();
    c.time        = o.value(QStringLiteral("time")).toString();
    c.unread      = o.value(QStringLiteral("unread")).toInt(0);
    c.online      = o.value(QStringLiteral("online")).toBool(false);
    c.isSaved     = o.value(QStringLiteral("is_saved_messages")).toBool(false);
    c.isBot       = o.value(QStringLiteral("is_bot")).toBool(false);
    return c;
}

static ChatMessage parseMessage(const QJsonObject& o) {
    ChatMessage m;
    m.id          = o.value(QStringLiteral("id")).toString();
    m.senderId    = o.value(QStringLiteral("sender_id")).toString();
    m.content     = o.value(QStringLiteral("content")).toString();
    m.messageType = o.value(QStringLiteral("message_type")).toString();
    if (m.messageType.isEmpty()) m.messageType = QStringLiteral("text");
    m.time        = o.value(QStringLiteral("time")).toString();
    m.createdAt   = o.value(QStringLiteral("created_at")).toString();
    m.status      = o.value(QStringLiteral("status")).toString();
    m.sent        = o.value(QStringLiteral("sent")).toBool(false);
    m.isRead      = o.value(QStringLiteral("is_read")).toBool(false);
    m.isDelivered = o.value(QStringLiteral("is_delivered")).toBool(false);
    const QJsonObject reply = o.value(QStringLiteral("reply")).toObject();
    m.replyAuthor  = reply.value(QStringLiteral("author")).toString();
    m.replySnippet = reply.value(QStringLiteral("snippet")).toString();
    return m;
}

void ApiClient::getChats() {
    QJsonObject body{{QStringLiteral("token"), Session::instance().token}};
    postJson(QStringLiteral("/api/chats"), body,
             [this](const QJsonObject& obj, bool ok, const QString& netErr) {
        if (!ok && obj.isEmpty()) { emit chatError(QStringLiteral("chats"), netErr); return; }
        if (!obj.value(QStringLiteral("success")).toBool(true) && obj.contains(QStringLiteral("message"))) {
            emit chatError(QStringLiteral("chats"), obj.value(QStringLiteral("message")).toString());
            return;
        }
        QList<Chat> chats;
        for (const QJsonValue& v : obj.value(QStringLiteral("chats")).toArray())
            chats.append(parseChat(v.toObject()));
        emit chatsLoaded(chats);
    });
}

void ApiClient::getMessages(const QString& friendId) {
    QJsonObject body{{QStringLiteral("token"), Session::instance().token},
                     {QStringLiteral("friend_id"), friendId}};
    postJson(QStringLiteral("/api/messages"), body,
             [this, friendId](const QJsonObject& obj, bool ok, const QString& netErr) {
        if (!ok && obj.isEmpty()) { emit chatError(QStringLiteral("messages"), netErr); return; }
        QList<ChatMessage> msgs;
        for (const QJsonValue& v : obj.value(QStringLiteral("messages")).toArray())
            msgs.append(parseMessage(v.toObject()));
        emit messagesLoaded(friendId, msgs);
    });
}

void ApiClient::sendMessage(const QString& receiverId, const QString& content, const QString& tempId) {
    QJsonObject body{{QStringLiteral("token"), Session::instance().token},
                     {QStringLiteral("receiver_id"), receiverId},
                     {QStringLiteral("content"), content},
                     {QStringLiteral("message_type"), QStringLiteral("text")},
                     {QStringLiteral("temp_id"), tempId}};
    postJson(QStringLiteral("/api/send-message"), body,
             [this, receiverId, content, tempId](const QJsonObject& obj, bool ok, const QString& netErr) {
        if (!ok && obj.isEmpty()) { emit chatError(QStringLiteral("send"), netErr); return; }
        if (!obj.value(QStringLiteral("success")).toBool(false)) {
            emit chatError(QStringLiteral("send"),
                obj.value(QStringLiteral("message")).toString(QStringLiteral("Не удалось отправить")));
            return;
        }
        ChatMessage m;
        m.id          = obj.value(QStringLiteral("message_id")).toString();
        m.senderId    = Session::instance().userId;
        m.content     = obj.value(QStringLiteral("content")).toString(content);
        m.messageType = QStringLiteral("text");
        m.time        = obj.value(QStringLiteral("time")).toString();
        m.createdAt   = obj.value(QStringLiteral("created_at")).toString();
        m.status      = QStringLiteral("sent");
        m.sent        = true;
        emit messageSent(m, receiverId, tempId);
    });
}
