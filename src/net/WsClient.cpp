#include "net/WsClient.h"

#include <QWebSocket>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

WsClient::WsClient(QObject* parent)
    : QObject(parent), sock_(new QWebSocket()), reconnect_(new QTimer(this)) {
    sock_->setParent(this);
    reconnect_->setSingleShot(true);
    reconnect_->setInterval(3000);

    connect(sock_, &QWebSocket::connected,    this, &WsClient::onConnected);
    connect(sock_, &QWebSocket::disconnected, this, &WsClient::onDisconnected);
    connect(sock_, &QWebSocket::textMessageReceived, this, &WsClient::onTextMessage);
    connect(reconnect_, &QTimer::timeout, this, &WsClient::tryReconnect);
}

void WsClient::start(const QString& token) {
    token_ = token;
    wantConnected_ = true;
    sock_->open(QUrl(baseWs_));
}

void WsClient::stop() {
    wantConnected_ = false;
    reconnect_->stop();
    sock_->close();
}

void WsClient::onConnected() {
    emit connectedChanged(true);
    sendAuth();
}

void WsClient::onDisconnected() {
    emit connectedChanged(false);
    if (wantConnected_) reconnect_->start();   // авто-reconnect
}

void WsClient::tryReconnect() {
    if (wantConnected_) sock_->open(QUrl(baseWs_));
}

void WsClient::sendAuth() {
    QJsonObject auth{
        {QStringLiteral("type"), QStringLiteral("auth")},
        {QStringLiteral("token"), token_},
        {QStringLiteral("device_id"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {QStringLiteral("platform"), QStringLiteral("desktop")}
    };
    sock_->sendTextMessage(QString::fromUtf8(QJsonDocument(auth).toJson(QJsonDocument::Compact)));
}

void WsClient::onTextMessage(const QString& text) {
    const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
    if (!doc.isObject()) return;
    const QJsonObject o = doc.object();
    const QString type = o.value(QStringLiteral("type")).toString();

    if (type == QStringLiteral("new_message")) {
        ChatMessage m;
        m.id          = o.value(QStringLiteral("id")).toString();
        if (m.id.isEmpty()) m.id = o.value(QStringLiteral("message_id")).toString();
        m.senderId    = o.value(QStringLiteral("sender_id")).toString();
        m.content     = o.value(QStringLiteral("content")).toString();
        m.messageType = o.value(QStringLiteral("message_type")).toString();
        if (m.messageType.isEmpty()) m.messageType = QStringLiteral("text");
        m.time        = o.value(QStringLiteral("time")).toString();
        m.createdAt   = o.value(QStringLiteral("created_at")).toString();
        m.status      = o.value(QStringLiteral("status")).toString();
        m.isRead      = o.value(QStringLiteral("is_read")).toBool(false);
        m.isDelivered = o.value(QStringLiteral("is_delivered")).toBool(false);
        m.filePath    = o.value(QStringLiteral("file_path")).toString();
        m.fileName    = o.value(QStringLiteral("file_name")).toString();
        m.fileSize    = static_cast<long long>(o.value(QStringLiteral("file_size")).toDouble(0));

        // chat_id с серверной стороны — это «другой участник» с точки зрения
        // получателя пакета (для исходящего эха — receiver, для входящего — sender).
        const QString peerId = o.value(QStringLiteral("chat_id")).toString();
        const QString tempId = o.value(QStringLiteral("temp_id")).toString();
        emit newMessage(peerId, m, tempId);
    }
    // auth_success / auth_error / прочие типы пока не требуют обработки.
}
