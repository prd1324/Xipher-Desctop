#pragma once
#include <QWidget>
#include <QList>
#include <QSet>
#include <QHash>
#include <QPointer>
#include <QJsonObject>

#include "net/Models.h"

class ApiClient;
class WsClient;
class VoiceRecorder;
class VoiceMessageWidget;
class RecordingBar;
class EmojiPicker;
class ChecklistWidget;
class EmptyChatGreeting;
class ProfilePanel;
class QNetworkAccessManager;
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

    // Эмодзи / вложения / таймер
    void onEmojiClicked();
    void onAttachClicked();
    void onTimerClicked();
    void pickAndSendFile();
    void openChecklistDialog();
    void sendLocation();
    void toggleSearch();
    void startCall();
    void showChatMenu();
    void openRenameDialog();
    void onFileUploaded(const QString& filePath, const QString& fileName, long long fileSize, const QString& tempId);

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    void buildUi();
    void playVoice(const QString& serverPath);
    void onVoicePlayPause(VoiceMessageWidget* w, const QString& path);
    void rebuildChatList();
    int  indexOfChat(const QString& id) const;
    void openChat(const Chat& chat);
    void addBubble(const ChatMessage& msg);
    void clearMessages();
    void renderMessages(const QString& filter);   // отрисовать (с фильтром поиска)
    void updateGreeting();   // показать/скрыть пустой-чат приветствие
    void scrollToBottom();
    void bumpChat(const QString& peerId, const QString& lastText, const QString& time, bool incrementUnread);

    ApiClient* api_;
    WsClient*  ws_;

    // Сайдбар
    QListWidget* chatList_ = nullptr;
    QLineEdit*   search_   = nullptr;

    // Переписка
    QStackedWidget* convStack_ = nullptr;   // 0 — пусто, 1 — диалог
    QWidget*     peerHeader_ = nullptr;
    QPushButton* moreBtn_    = nullptr;
    QWidget*     searchBar_  = nullptr;
    QLineEdit*   msgSearch_  = nullptr;
    QLabel*      searchCount_ = nullptr;
    QList<ChatMessage> currentMessages_;   // загруженные сообщения текущего чата
    QLabel*      peerName_   = nullptr;
    QLabel*      peerStatus_ = nullptr;
    QLabel*      peerAvatar_ = nullptr;
    ProfilePanel* profilePanel_ = nullptr;
    QScrollArea* msgScroll_  = nullptr;
    QWidget*     msgContainer_ = nullptr;
    QVBoxLayout* msgLayout_  = nullptr;
    EmptyChatGreeting* greeting_ = nullptr;
    int          bubbleCount_ = 0;   // сколько сообщений сейчас в переписке
    QStackedWidget* composerStack_ = nullptr;  // 0 — ввод, 1 — запись
    QLineEdit*   composer_   = nullptr;
    QPushButton* sendBtn_    = nullptr;
    QPushButton* micBtn_     = nullptr;
    QPushButton* emojiBtn_   = nullptr;
    QPushButton* attachBtn_  = nullptr;
    QPushButton* timerBtn_   = nullptr;
    RecordingBar* recBar_    = nullptr;
    EmojiPicker*  emojiPicker_ = nullptr;
    int           disappearTtl_ = 0;   // сек, 0 = выкл (пока UI-состояние)

    // Файлы: отложенная отправка/открытие
    QString pendingFileReceiver_;
    QHash<QString, QString> pendingFileOpen_;   // серверный путь → имя для сохранения

    // Чек-листы (live-обновления) + гео
    QHash<QString, ChecklistWidget*> checklistWidgets_;   // id → виджет
    QHash<QString, QJsonObject>      mergedChecklists_;   // id → склеенное состояние
    QString                          pendingLocReceiver_;
    QNetworkAccessManager*           geoNam_ = nullptr;

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
    QList<UserHit> searchHits_;   // глобальный поиск людей в сайдбаре
    QString     searchQuery_;
    QTimer*     searchTimer_ = nullptr;
    QHash<QString, QPointer<QLabel>> pendingImage_;   // путь → QLabel для картинки
    QString     currentPeerId_;
    QString     currentPeerName_;
    QSet<QString> shownIds_;   // дедуп сообщений в открытом чате
    int tempCounter_ = 0;
};
