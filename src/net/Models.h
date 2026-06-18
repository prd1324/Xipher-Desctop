#pragma once
#include <QString>

// ─────────────────────────────────────────────────────────────────────────────
//  Доменные модели чата — повторяют JSON прод-API
//  (/api/chats, /api/messages, /api/send-message).
// ─────────────────────────────────────────────────────────────────────────────

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
