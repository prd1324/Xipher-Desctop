#pragma once
#include <QWidget>
#include <QList>
#include <QSet>
#include <QHash>

#include "net/Models.h"

class ApiClient;
class WsClient;
class VoiceRecorder;
class VoiceMessageWidget;
class RecordingBar;
class QListWidget;
class QLineEdit;
class QPushButton;
class QLabel;
class QStackedWidget;
class QScrollArea;
class QVBoxLayout;
class QTimer;
class QMediaPlayer;
class QAudioOutput;

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

    // Голосовые
    void onMicClicked();
    void cancelRecording();
    void stopAndSendVoice();
    void onVoiceRecorded(const QString& filePath, const QString& mimeType);
    void onVoiceUploaded(const QString& filePath, const QString& fileName, long long fileSize, const QString& tempId);
    void onFileFetched(const QString& filePath, const QByteArray& bytes);

private:
    void buildUi();
    void playVoice(const QString& serverPath);
    void onVoicePlayPause(VoiceMessageWidget* w, const QString& path);
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
    QStackedWidget* composerStack_ = nullptr;  // 0 — ввод, 1 — запись
    QLineEdit*   composer_   = nullptr;
    QPushButton* sendBtn_    = nullptr;
    QPushButton* micBtn_     = nullptr;
    RecordingBar* recBar_    = nullptr;

    // Голос
    VoiceRecorder* recorder_ = nullptr;
    QMediaPlayer*  player_   = nullptr;
    QAudioOutput*  audioOut_ = nullptr;
    QString        pendingVoiceTempId_;
    QString        pendingVoiceReceiver_;
    int            pendingVoiceSecs_ = 0;
    QString        pendingPlayPath_;
    QHash<QString, QString> voiceCache_;   // серверный путь → локальный temp-файл

    // Текущий проигрываемый голосовой виджет (для прогресса/таймера).
    VoiceMessageWidget* activeVoice_ = nullptr;
    QString             activeVoicePath_;

    QList<Chat> chats_;
    QString     currentPeerId_;
    QString     currentPeerName_;
    QSet<QString> shownIds_;   // дедуп сообщений в открытом чате
    int tempCounter_ = 0;
};
