#pragma once
#include <QString>

// ─────────────────────────────────────────────────────────────────────────────
//  Доменные модели чата — повторяют JSON прод-API
//  (/api/chats, /api/messages, /api/send-message).
// ─────────────────────────────────────────────────────────────────────────────

// ICE-сервер для WebRTC. Для STUN username/credential пустые; для TURN —
// эфемерные креды (use-auth-secret/HMAC) из /api/turn-credentials.
// url: "stun:host:port" | "turn:host:port?transport=udp" | "turns:host:port?transport=tcp".
struct IceServerCfg {
    QString url;
    QString username;
    QString credential;
};

// Активная сессия (/api/get-sessions → sessions[]).
struct SessionInfo {
    QString id;          // session_id
    QString userAgent;   // "stealth" / browser UA
    QString createdAt;
    QString lastSeen;
    bool    current = false;
};

// Тип чата (личка / группа / канал).
enum class ChatKind { User = 0, Group = 1, Channel = 2 };

// Публичный элемент каталога (/api/public-directory → items[]).
struct DirectoryItem {
    QString id;
    QString type;          // "channel" | "group"
    QString name;
    QString username;      // @custom_link (опц.)
    QString description;
    QString avatarUrl;
    int     membersCount = 0;
    bool    verified = false;
    bool    isMember = false;
    bool    isPrivate = false;
};

// Элемент списка чатов (/api/chats → chats[]).
struct Chat {
    QString id;            // user_id собеседника (или свой для «Избранных»)
    QString name;          // username
    QString displayName;   // имя для показа (custom_name/имя/username)
    QString avatarUrl;
    QString avatarText;    // эмодзи/буква, если нет аватарки
    QString lastMessage;
    QString time;          // "HH:MM"
    int     unread = 0;
    bool    online = false;
    bool    isSaved = false;   // «Избранные» (saved messages)
    ChatKind kind = ChatKind::User;
    int     membersCount = 0;
    QString customLink;        // @username группы/канала (опц.)
    bool    isBot = false;
};

// Результат поиска пользователей (/api/search-users → users[]).
struct UserHit {
    QString id;            // user_id
    QString username;
    QString displayName;
    QString avatarUrl;
    bool    isPremium = false;
};

// Входящая заявка в друзья (/api/friend-requests → requests[]).
struct FriendRequest {
    QString id;            // request_id
    QString senderId;
    QString senderUsername;
    QString createdAt;
};

// Сообщение (/api/messages → messages[], /api/send-message ack).
struct ChatMessage {
    QString id;
    QString senderId;
    QString senderName;    // имя отправителя (для групп/каналов)
    QString content;
    QString messageType = QStringLiteral("text");
    QString time;          // "HH:MM"
    QString createdAt;     // ISO
    QString status;        // sent | delivered | read (для исходящих)
    bool    sent = false;  // true → исходящее (моё)
    bool    isRead = false;
    bool    isDelivered = false;
    int     ttlSeconds = 0;   // исчезающее сообщение (0 = обычное)

    // Вложение (voice/file/image): путь вида "/files/<имя>"
    QString filePath;
    QString fileName;
    long long fileSize = 0;

    // Reply-превью (если есть)
    QString replyAuthor;
    QString replySnippet;

    bool isVoice() const { return messageType == QStringLiteral("voice"); }
};
