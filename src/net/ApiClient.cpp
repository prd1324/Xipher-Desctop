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
    // HTTP/2 у прода режет параллельные стримы ("Server refused a stream") —
    // форсируем HTTP/1.1, иначе поллинг/сигналинг периодически падает.
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

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
    m.filePath    = o.value(QStringLiteral("file_path")).toString();
    m.fileName    = o.value(QStringLiteral("file_name")).toString();
    m.fileSize    = static_cast<long long>(o.value(QStringLiteral("file_size")).toDouble(0));
    m.ttlSeconds  = o.value(QStringLiteral("ttl_seconds")).toInt(0);
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

void ApiClient::sendMessage(const QString& receiverId, const QString& content, const QString& tempId,
                            int ttlSeconds) {
    QJsonObject body{{QStringLiteral("token"), Session::instance().token},
                     {QStringLiteral("receiver_id"), receiverId},
                     {QStringLiteral("content"), content},
                     {QStringLiteral("message_type"), QStringLiteral("text")},
                     {QStringLiteral("temp_id"), tempId}};
    if (ttlSeconds > 0) body.insert(QStringLiteral("ttl_seconds"), ttlSeconds);
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

void ApiClient::sendRaw(const QString& receiverId, const QString& content,
                        const QString& messageType, const QString& tempId) {
    QJsonObject body{{QStringLiteral("token"), Session::instance().token},
                     {QStringLiteral("receiver_id"), receiverId},
                     {QStringLiteral("content"), content},
                     {QStringLiteral("message_type"), messageType},
                     {QStringLiteral("temp_id"), tempId}};
    postJson(QStringLiteral("/api/send-message"), body,
             [this, receiverId, content, messageType, tempId](
                 const QJsonObject& obj, bool ok, const QString& netErr) {
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
        m.messageType = messageType;
        m.time        = obj.value(QStringLiteral("time")).toString();
        m.createdAt   = obj.value(QStringLiteral("created_at")).toString();
        m.status      = QStringLiteral("sent");
        m.sent        = true;
        emit messageSent(m, receiverId, tempId);
    });
}

// ── Люди и друзья ─────────────────────────────────────────────────────────────

void ApiClient::setContactName(const QString& contactId, const QString& customName) {
    QJsonObject body{{QStringLiteral("token"), Session::instance().token},
                     {QStringLiteral("contact_id"), contactId},
                     {QStringLiteral("custom_name"), customName}};
    postJson(QStringLiteral("/api/set-contact-name"), body,
             [this, contactId, customName](const QJsonObject& obj, bool ok, const QString&) {
        emit contactRenamed(contactId, customName,
                            ok && obj.value(QStringLiteral("success")).toBool(false));
    });
}

void ApiClient::getUserProfile(const QString& userId) {
    QJsonObject body{{QStringLiteral("token"), Session::instance().token},
                     {QStringLiteral("user_id"), userId}};
    postJson(QStringLiteral("/api/get-user-profile"), body,
             [this](const QJsonObject& obj, bool ok, const QString& netErr) {
        if (!ok && obj.isEmpty()) { emit chatError(QStringLiteral("profile"), netErr); return; }
        emit profileLoaded(obj);
    });
}

void ApiClient::searchUsers(const QString& query) {
    QJsonObject body{{QStringLiteral("token"), Session::instance().token},
                     {QStringLiteral("query"), query}};
    postJson(QStringLiteral("/api/search-users"), body,
             [this, query](const QJsonObject& obj, bool ok, const QString&) {
        if (!ok && obj.isEmpty()) return;
        QList<UserHit> users;
        for (const QJsonValue& v : obj.value(QStringLiteral("users")).toArray()) {
            const QJsonObject o = v.toObject();
            UserHit u;
            u.id          = o.value(QStringLiteral("user_id")).toString();
            u.username    = o.value(QStringLiteral("username")).toString();
            u.displayName = o.value(QStringLiteral("display_name")).toString();
            if (u.displayName.isEmpty()) u.displayName = u.username;
            u.avatarUrl   = o.value(QStringLiteral("avatar_url")).toString();
            u.isPremium   = o.value(QStringLiteral("is_premium")).toBool(false);
            users.append(u);
        }
        emit usersFound(query, users);
    });
}

void ApiClient::sendFriendRequest(const QString& username) {
    QJsonObject body{{QStringLiteral("token"), Session::instance().token},
                     {QStringLiteral("username"), username}};
    postJson(QStringLiteral("/api/friend-request"), body,
             [this, username](const QJsonObject& obj, bool ok, const QString& netErr) {
        if (!ok && obj.isEmpty()) { emit friendRequestSent(username, false, netErr); return; }
        const bool success = obj.value(QStringLiteral("success")).toBool(false);
        emit friendRequestSent(username, success, obj.value(QStringLiteral("message")).toString());
    });
}

void ApiClient::getFriendRequests() {
    QJsonObject body{{QStringLiteral("token"), Session::instance().token}};
    postJson(QStringLiteral("/api/friend-requests"), body,
             [this](const QJsonObject& obj, bool ok, const QString&) {
        if (!ok && obj.isEmpty()) return;
        QList<FriendRequest> reqs;
        for (const QJsonValue& v : obj.value(QStringLiteral("requests")).toArray()) {
            const QJsonObject o = v.toObject();
            FriendRequest r;
            r.id             = o.value(QStringLiteral("id")).toString();
            r.senderId       = o.value(QStringLiteral("sender_id")).toString();
            r.senderUsername = o.value(QStringLiteral("sender_username")).toString();
            r.createdAt      = o.value(QStringLiteral("created_at")).toString();
            reqs.append(r);
        }
        emit friendRequestsLoaded(reqs);
    });
}

// ── Голосовые / файлы ─────────────────────────────────────────────────────────

void ApiClient::uploadVoice(const QByteArray& audioBytes, const QString& mimeType, const QString& tempId) {
    QJsonObject body{
        {QStringLiteral("token"), Session::instance().token},
        {QStringLiteral("voice_data"), QString::fromLatin1(audioBytes.toBase64())},
        {QStringLiteral("mime_type"), mimeType}
    };
    postJson(QStringLiteral("/api/upload-voice"), body,
             [this, tempId](const QJsonObject& obj, bool ok, const QString& netErr) {
        if (!ok && obj.isEmpty()) { emit chatError(QStringLiteral("voice"), netErr); return; }
        if (!obj.value(QStringLiteral("success")).toBool(false)) {
            emit chatError(QStringLiteral("voice"),
                obj.value(QStringLiteral("message")).toString(QStringLiteral("Не удалось загрузить голос")));
            return;
        }
        emit voiceUploaded(obj.value(QStringLiteral("file_path")).toString(),
                           obj.value(QStringLiteral("file_name")).toString(),
                           static_cast<long long>(obj.value(QStringLiteral("file_size")).toDouble(0)),
                           tempId);
    });
}

void ApiClient::sendVoice(const QString& receiverId, const QString& filePath, const QString& fileName,
                          long long fileSize, const QString& caption, const QString& tempId) {
    QJsonObject body{
        {QStringLiteral("token"), Session::instance().token},
        {QStringLiteral("receiver_id"), receiverId},
        {QStringLiteral("content"), caption},
        {QStringLiteral("message_type"), QStringLiteral("voice")},
        {QStringLiteral("file_path"), filePath},
        {QStringLiteral("file_name"), fileName},
        {QStringLiteral("file_size"), static_cast<double>(fileSize)},
        {QStringLiteral("temp_id"), tempId}
    };
    postJson(QStringLiteral("/api/send-message"), body,
             [this, receiverId, filePath, fileName, fileSize, caption, tempId](
                 const QJsonObject& obj, bool ok, const QString& netErr) {
        if (!ok && obj.isEmpty()) { emit chatError(QStringLiteral("send"), netErr); return; }
        if (!obj.value(QStringLiteral("success")).toBool(false)) {
            emit chatError(QStringLiteral("send"),
                obj.value(QStringLiteral("message")).toString(QStringLiteral("Не удалось отправить")));
            return;
        }
        ChatMessage m;
        m.id          = obj.value(QStringLiteral("message_id")).toString();
        m.senderId    = Session::instance().userId;
        m.content     = caption;
        m.messageType = QStringLiteral("voice");
        m.filePath    = obj.value(QStringLiteral("file_path")).toString(filePath);
        m.fileName    = fileName;
        m.fileSize    = fileSize;
        m.time        = obj.value(QStringLiteral("time")).toString();
        m.createdAt   = obj.value(QStringLiteral("created_at")).toString();
        m.status      = QStringLiteral("sent");
        m.sent        = true;
        emit messageSent(m, receiverId, tempId);
    });
}

void ApiClient::uploadFile(const QByteArray& bytes, const QString& fileName, const QString& tempId) {
    QJsonObject body{
        {QStringLiteral("token"), Session::instance().token},
        {QStringLiteral("file_data"), QString::fromLatin1(bytes.toBase64())},
        {QStringLiteral("file_name"), fileName}
    };
    postJson(QStringLiteral("/api/upload-file"), body,
             [this, fileName, tempId](const QJsonObject& obj, bool ok, const QString& netErr) {
        if (!ok && obj.isEmpty()) { emit chatError(QStringLiteral("file"), netErr); return; }
        if (!obj.value(QStringLiteral("success")).toBool(false)) {
            emit chatError(QStringLiteral("file"),
                obj.value(QStringLiteral("message")).toString(QStringLiteral("Не удалось загрузить файл")));
            return;
        }
        emit fileUploaded(obj.value(QStringLiteral("file_path")).toString(),
                          obj.value(QStringLiteral("file_name")).toString(fileName),
                          static_cast<long long>(obj.value(QStringLiteral("file_size")).toDouble(0)),
                          tempId);
    });
}

void ApiClient::sendFile(const QString& receiverId, const QString& filePath, const QString& fileName,
                         long long fileSize, const QString& caption, const QString& tempId) {
    QJsonObject body{
        {QStringLiteral("token"), Session::instance().token},
        {QStringLiteral("receiver_id"), receiverId},
        {QStringLiteral("content"), caption},
        {QStringLiteral("message_type"), QStringLiteral("file")},
        {QStringLiteral("file_path"), filePath},
        {QStringLiteral("file_name"), fileName},
        {QStringLiteral("file_size"), static_cast<double>(fileSize)},
        {QStringLiteral("temp_id"), tempId}
    };
    postJson(QStringLiteral("/api/send-message"), body,
             [this, receiverId, filePath, fileName, fileSize, caption, tempId](
                 const QJsonObject& obj, bool ok, const QString& netErr) {
        if (!ok && obj.isEmpty()) { emit chatError(QStringLiteral("send"), netErr); return; }
        if (!obj.value(QStringLiteral("success")).toBool(false)) {
            emit chatError(QStringLiteral("send"),
                obj.value(QStringLiteral("message")).toString(QStringLiteral("Не удалось отправить")));
            return;
        }
        ChatMessage m;
        m.id          = obj.value(QStringLiteral("message_id")).toString();
        m.senderId    = Session::instance().userId;
        m.content     = caption;
        m.messageType = QStringLiteral("file");
        m.filePath    = obj.value(QStringLiteral("file_path")).toString(filePath);
        m.fileName    = fileName;
        m.fileSize    = fileSize;
        m.time        = obj.value(QStringLiteral("time")).toString();
        m.createdAt   = obj.value(QStringLiteral("created_at")).toString();
        m.status      = QStringLiteral("sent");
        m.sent        = true;
        emit messageSent(m, receiverId, tempId);
    });
}

void ApiClient::fetchFile(const QString& filePath) {
    QNetworkRequest req(QUrl(base_ + filePath));
    req.setRawHeader("Authorization", "Bearer " + Session::instance().token.toUtf8());
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    QNetworkReply* reply = nam_->get(req);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, filePath]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit chatError(QStringLiteral("file"), reply->errorString());
            return;
        }
        emit fileFetched(filePath, reply->readAll());
    });
}

// ── Звонки (сигналинг) ────────────────────────────────────────────────────────

void ApiClient::getTurnConfig() {
    QJsonObject body{{QStringLiteral("token"), Session::instance().token}};
    postJson(QStringLiteral("/api/turn-config"), body,
             [this](const QJsonObject& obj, bool ok, const QString&) {
        QStringList servers;
        for (const QJsonValue& v : obj.value(QStringLiteral("ice_servers")).toArray()) {
            const QJsonValue urls = v.toObject().value(QStringLiteral("urls"));
            if (urls.isString()) servers << urls.toString();
            else for (const QJsonValue& u : urls.toArray()) servers << u.toString();
        }
        emit turnConfigReady(servers);
    });
}

void ApiClient::callNotify(const QString& receiverId, const QString& callType) {
    postJson(QStringLiteral("/api/call-notification"),
             {{QStringLiteral("token"), Session::instance().token},
              {QStringLiteral("receiver_id"), receiverId},
              {QStringLiteral("call_type"), callType}},
             [](const QJsonObject&, bool, const QString&) {});
}

void ApiClient::callOffer(const QString& receiverId, const QString& sdp) {
    postJson(QStringLiteral("/api/call-offer"),
             {{QStringLiteral("token"), Session::instance().token},
              {QStringLiteral("receiver_id"), receiverId},
              {QStringLiteral("offer"), sdp}},
             [](const QJsonObject&, bool, const QString&) {});
}

void ApiClient::callAnswer(const QString& receiverId, const QString& sdp) {
    postJson(QStringLiteral("/api/call-answer"),
             {{QStringLiteral("token"), Session::instance().token},
              {QStringLiteral("receiver_id"), receiverId},
              {QStringLiteral("answer"), sdp}},
             [](const QJsonObject&, bool, const QString&) {});
}

void ApiClient::callIce(const QString& receiverId, const QString& candidate) {
    postJson(QStringLiteral("/api/call-ice"),
             {{QStringLiteral("token"), Session::instance().token},
              {QStringLiteral("receiver_id"), receiverId},
              {QStringLiteral("candidate"), candidate}},
             [](const QJsonObject&, bool, const QString&) {});
}

void ApiClient::callEnd(const QString& receiverId) {
    postJson(QStringLiteral("/api/call-end"),
             {{QStringLiteral("token"), Session::instance().token},
              {QStringLiteral("receiver_id"), receiverId}},
             [](const QJsonObject&, bool, const QString&) {});
}

void ApiClient::getCallOffer(const QString& callerId) {
    postJson(QStringLiteral("/api/get-call-offer"),
             {{QStringLiteral("token"), Session::instance().token},
              {QStringLiteral("receiver_id"), callerId}},   // receiver_id здесь = инициатор
             [this, callerId](const QJsonObject& obj, bool ok, const QString&) {
        const QJsonObject data = obj.value(QStringLiteral("data")).toObject();
        QString sdp = data.value(QStringLiteral("offer")).toString();
        if (sdp.isEmpty()) sdp = obj.value(QStringLiteral("offer")).toString();
        if (!sdp.isEmpty()) emit callOfferReady(callerId, sdp);
    });
}

void ApiClient::getCallAnswer(const QString& calleeId) {
    postJson(QStringLiteral("/api/get-call-answer"),
             {{QStringLiteral("token"), Session::instance().token},
              {QStringLiteral("receiver_id"), calleeId}},
             [this, calleeId](const QJsonObject& obj, bool ok, const QString&) {
        const QJsonObject data = obj.value(QStringLiteral("data")).toObject();
        QString sdp = data.value(QStringLiteral("answer")).toString();
        if (sdp.isEmpty()) sdp = obj.value(QStringLiteral("answer")).toString();
        if (!sdp.isEmpty()) emit callAnswerReady(calleeId, sdp);
    });
}

void ApiClient::getCallIce(const QString& otherId) {
    postJson(QStringLiteral("/api/get-call-ice"),
             {{QStringLiteral("token"), Session::instance().token},
              {QStringLiteral("receiver_id"), otherId}},
             [this, otherId](const QJsonObject& obj, bool ok, const QString&) {
        QStringList cands;
        const QJsonValue cv = obj.value(QStringLiteral("data")).toObject().value(QStringLiteral("candidates"));
        // Сервер отдаёт candidates как СТРОКУ с JSON-массивом внутри: "[\"...\"]".
        QJsonArray arr;
        if (cv.isString())      arr = QJsonDocument::fromJson(cv.toString().toUtf8()).array();
        else if (cv.isArray())  arr = cv.toArray();
        else {
            const QJsonValue top = obj.value(QStringLiteral("candidates"));
            if (top.isString())     arr = QJsonDocument::fromJson(top.toString().toUtf8()).array();
            else                    arr = top.toArray();
        }
        for (const QJsonValue& v : arr) {
            if (v.isString()) cands << v.toString();
            else cands << v.toObject().value(QStringLiteral("candidate")).toString();
        }
        if (!cands.isEmpty()) emit callIceBatch(otherId, cands);
    });
}

void ApiClient::checkIncomingCalls() {
    postJson(QStringLiteral("/api/check-incoming-calls"),
             {{QStringLiteral("token"), Session::instance().token}},
             [this](const QJsonObject& obj, bool ok, const QString&) {
        if (!ok) return;
        const QJsonObject call = obj.value(QStringLiteral("call")).toObject();
        const QString callerId = call.value(QStringLiteral("caller_id")).toString();
        if (callerId.isEmpty()) return;
        emit incomingCall(callerId, call.value(QStringLiteral("caller_username")).toString(),
                          call.value(QStringLiteral("call_type")).toString());
    });
}

void ApiClient::acceptFriend(const QString& requestId) {
    QJsonObject body{{QStringLiteral("token"), Session::instance().token},
                     {QStringLiteral("request_id"), requestId}};
    postJson(QStringLiteral("/api/accept-friend"), body,
             [this, requestId](const QJsonObject& obj, bool ok, const QString&) {
        emit friendActionDone(requestId, true, ok && obj.value(QStringLiteral("success")).toBool(false));
    });
}

void ApiClient::rejectFriend(const QString& requestId) {
    QJsonObject body{{QStringLiteral("token"), Session::instance().token},
                     {QStringLiteral("request_id"), requestId}};
    postJson(QStringLiteral("/api/reject-friend"), body,
             [this, requestId](const QJsonObject& obj, bool ok, const QString&) {
        emit friendActionDone(requestId, false, ok && obj.value(QStringLiteral("success")).toBool(false));
    });
}
