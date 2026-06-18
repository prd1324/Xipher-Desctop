#pragma once
#include <QObject>
#include <QString>

#include "net/Models.h"

class QWebSocket;
class QTimer;

// ─────────────────────────────────────────────────────────────────────────────
//  WsClient — realtime-канал через WebSocket /ws.
//  Протокол прод-сервера:
//    1) после коннекта шлём {"type":"auth","token":...,"device_id":...,"platform":"desktop"}
//    2) сервер отвечает {"type":"auth_success"} / {"type":"auth_error"}
//    3) входящие сообщения: {"type":"new_message","chat_id":...,"sender_id":...,...}
//  Авто-reconnect при обрыве.
// ─────────────────────────────────────────────────────────────────────────────
class WsClient : public QObject {
    Q_OBJECT
public:
    explicit WsClient(QObject* parent = nullptr);

    void start(const QString& token);   // подключиться и авторизоваться
    void stop();

signals:
    // Пришло новое сообщение. peerId — id чата с моей точки зрения (собеседник).
    void newMessage(const QString& peerId, const ChatMessage& msg, const QString& tempId);
    void connectedChanged(bool connected);

    // Сигналинг звонков (real-time)
    void callAnswerReceived(const QString& fromUserId, const QString& sdp);
    void callIceReceived(const QString& fromUserId, const QString& candidate);
    void callEnded(const QString& fromUserId);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessage(const QString& text);
    void tryReconnect();

private:
    void sendAuth();

    QWebSocket* sock_;
    QTimer*     reconnect_;
    QString     token_;
    QString     baseWs_ = QStringLiteral("wss://messenger.xipher.pro/ws");
    bool        wantConnected_ = false;
};
