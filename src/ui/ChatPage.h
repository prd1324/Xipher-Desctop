#pragma once
#include <QWidget>
#include <QList>
#include <QSet>

#include "net/Models.h"

class ApiClient;
class WsClient;
class QListWidget;
class QLineEdit;
class QPushButton;
class QLabel;
class QStackedWidget;
class QScrollArea;
class QVBoxLayout;

// ─────────────────────────────────────────────────────────────────────────────
//  ChatPage — основной экран мессенджера (раскладка как в Telegram/веб-чате):
//  слева список чатов, справа переписка с полем ввода. Реальные данные с
//  прод-API + realtime по WebSocket.
// ─────────────────────────────────────────────────────────────────────────────
class ChatPage : public QWidget {
    Q_OBJECT
public:
    ChatPage(ApiClient* api, WsClient* ws, QWidget* parent = nullptr);

    void load();   // вызвать после входа: грузит чаты и запускает realtime

signals:
    void logoutRequested();

public:
    void openChatWith(const QString& userId, const QString& displayName, const QString& username);

private slots:
    void openNewChatDialog();
    void onChatsLoaded(const QList<Chat>& chats);
    void onMessagesLoaded(const QString& friendId, const QList<ChatMessage>& messages);
    void onMessageSent(const ChatMessage& msg, const QString& receiverId, const QString& tempId);
    void onWsMessage(const QString& peerId, const ChatMessage& msg, const QString& tempId);
    void onChatClicked();
    void onSendClicked();
    void onSearchChanged(const QString& text);

private:
    void buildUi();
    void rebuildChatList();
    int  indexOfChat(const QString& id) const;
    void openChat(const Chat& chat);
    void addBubble(const ChatMessage& msg);
    void clearMessages();
    void scrollToBottom();
    void bumpChat(const QString& peerId, const QString& lastText, const QString& time, bool incrementUnread);

    ApiClient* api_;
    WsClient*  ws_;

    // Сайдбар
    QListWidget* chatList_ = nullptr;
    QLineEdit*   search_   = nullptr;

    // Переписка
    QStackedWidget* convStack_ = nullptr;   // 0 — пусто, 1 — диалог
    QLabel*      peerName_   = nullptr;
    QLabel*      peerStatus_ = nullptr;
    QLabel*      peerAvatar_ = nullptr;
    QScrollArea* msgScroll_  = nullptr;
    QWidget*     msgContainer_ = nullptr;
    QVBoxLayout* msgLayout_  = nullptr;
    QLineEdit*   composer_   = nullptr;
    QPushButton* sendBtn_    = nullptr;

    QList<Chat> chats_;
    QString     currentPeerId_;
    QString     currentPeerName_;
    QSet<QString> shownIds_;   // дедуп сообщений в открытом чате
    int tempCounter_ = 0;
};
