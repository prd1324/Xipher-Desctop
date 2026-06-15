#include "ui/ChatPage.h"
#include "net/ApiClient.h"
#include "net/WsClient.h"
#include "net/Session.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QFrame>
#include <QFontMetrics>
#include <QTime>
#include <QTimer>

namespace {

QString elide(const QString& s, const QFont& f, int px) {
    return QFontMetrics(f).elidedText(s.isEmpty() ? QString() : s, Qt::ElideRight, px);
}

// Круглый аватар: буква на матовом фиолетовом градиенте.
QLabel* makeAvatar(const QString& text, int size) {
    auto* a = new QLabel();
    a->setFixedSize(size, size);
    a->setAlignment(Qt::AlignCenter);
    QString letter = text.trimmed().left(1).toUpper();
    if (letter.isEmpty()) letter = QStringLiteral("?");
    a->setText(letter);
    a->setStyleSheet(QString(
        "border-radius:%1px;color:#fff;font-weight:700;font-size:%2px;"
        "background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #8B5CF6,stop:1 #6D28D9);")
        .arg(size / 2).arg(size * 4 / 10));
    return a;
}

} // namespace

ChatPage::ChatPage(ApiClient* api, WsClient* ws, QWidget* parent)
    : QWidget(parent), api_(api), ws_(ws) {
    buildUi();

    connect(api_, &ApiClient::chatsLoaded,    this, &ChatPage::onChatsLoaded);
    connect(api_, &ApiClient::messagesLoaded,  this, &ChatPage::onMessagesLoaded);
    connect(api_, &ApiClient::messageSent,     this, &ChatPage::onMessageSent);
    connect(ws_,  &WsClient::newMessage,       this, &ChatPage::onWsMessage);
}

void ChatPage::buildUi() {
    setStyleSheet(QStringLiteral(R"QSS(
#sidebar { background:#131218; border-right:1px solid rgba(255,255,255,0.10); }
#sideHeader, #convHeader { background:#131218; border-bottom:1px solid rgba(255,255,255,0.10); }
#brandTitle { font-size:18px; font-weight:800; color:#F3F1F8; }
#searchBox {
    background:#1A1822; border:1px solid rgba(255,255,255,0.10); border-radius:12px;
    min-height:38px; padding:0 14px; color:#F3F1F8;
}
#searchBox:focus { border:1px solid #8B5CF6; }
#chatList { background:#131218; border:none; outline:none; }
#chatList::item { border:none; padding:0; }
#chatList::item:hover { background:#1A1822; }
#chatList::item:selected { background:rgba(139,92,246,0.16); }
#msgArea { background:#0B0A0E; border:none; }
#msgArea > QWidget > QWidget { background:#0B0A0E; }
#composerBar { background:#131218; border-top:1px solid rgba(255,255,255,0.10); }
#composer {
    background:#1A1822; border:1px solid rgba(255,255,255,0.10); border-radius:20px;
    min-height:44px; padding:0 18px; color:#F3F1F8; font-size:15px;
}
#composer:focus { border:1px solid #8B5CF6; }
#sendBtn {
    min-width:44px; max-width:44px; min-height:44px; max-height:44px; border:none;
    border-radius:22px; color:#fff; font-size:18px; font-weight:700;
    background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #8B5CF6,stop:1 #6D28D9);
}
#sendBtn:hover { background:#9B72F8; }
#peerName { font-size:15px; font-weight:700; color:#F3F1F8; }
#peerStatus { font-size:12px; color:#726C82; }
#emptyHint { font-size:15px; color:#726C82; }
#iconBtn { border:none; background:transparent; color:#ACA6BD; font-size:13px; }
#iconBtn:hover { color:#F3F1F8; }
)QSS"));

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Сайдбар ──────────────────────────────────────────────────────────────
    auto* sidebar = new QWidget(this);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setFixedWidth(340);
    auto* side = new QVBoxLayout(sidebar);
    side->setContentsMargins(0, 0, 0, 0);
    side->setSpacing(0);

    auto* sideHeader = new QWidget(sidebar);
    sideHeader->setObjectName(QStringLiteral("sideHeader"));
    sideHeader->setFixedHeight(60);
    auto* shl = new QHBoxLayout(sideHeader);
    shl->setContentsMargins(16, 0, 12, 0);
    auto* brand = new QLabel(QStringLiteral("Xipher"), sideHeader);
    brand->setObjectName(QStringLiteral("brandTitle"));
    auto* logoutBtn = new QPushButton(QStringLiteral("Выйти"), sideHeader);
    logoutBtn->setObjectName(QStringLiteral("iconBtn"));
    logoutBtn->setCursor(Qt::PointingHandCursor);
    shl->addWidget(brand);
    shl->addStretch();
    shl->addWidget(logoutBtn);

    auto* searchWrap = new QWidget(sidebar);
    auto* swl = new QVBoxLayout(searchWrap);
    swl->setContentsMargins(12, 10, 12, 10);
    search_ = new QLineEdit(searchWrap);
    search_->setObjectName(QStringLiteral("searchBox"));
    search_->setPlaceholderText(QStringLiteral("Поиск"));
    swl->addWidget(search_);

    chatList_ = new QListWidget(sidebar);
    chatList_->setObjectName(QStringLiteral("chatList"));
    chatList_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    side->addWidget(sideHeader);
    side->addWidget(searchWrap);
    side->addWidget(chatList_, 1);

    // ── Область переписки ──────────────────────────────────────────────────────
    convStack_ = new QStackedWidget(this);

    // Пустое состояние
    auto* empty = new QWidget(convStack_);
    empty->setStyleSheet(QStringLiteral("background:#0B0A0E;"));
    auto* el = new QVBoxLayout(empty);
    el->addStretch();
    auto* hint = new QLabel(QStringLiteral("Выберите чат, чтобы начать переписку"), empty);
    hint->setObjectName(QStringLiteral("emptyHint"));
    hint->setAlignment(Qt::AlignCenter);
    el->addWidget(hint);
    el->addStretch();

    // Диалог
    auto* conv = new QWidget(convStack_);
    auto* cvl = new QVBoxLayout(conv);
    cvl->setContentsMargins(0, 0, 0, 0);
    cvl->setSpacing(0);

    auto* convHeader = new QWidget(conv);
    convHeader->setObjectName(QStringLiteral("convHeader"));
    convHeader->setFixedHeight(60);
    auto* chl = new QHBoxLayout(convHeader);
    chl->setContentsMargins(16, 0, 16, 0);
    chl->setSpacing(12);
    peerAvatar_ = makeAvatar(QStringLiteral("?"), 40);
    peerAvatar_->setParent(convHeader);
    auto* names = new QVBoxLayout();
    names->setSpacing(0);
    peerName_ = new QLabel(convHeader);
    peerName_->setObjectName(QStringLiteral("peerName"));
    peerStatus_ = new QLabel(convHeader);
    peerStatus_->setObjectName(QStringLiteral("peerStatus"));
    names->addWidget(peerName_);
    names->addWidget(peerStatus_);
    chl->addWidget(peerAvatar_);
    chl->addLayout(names);
    chl->addStretch();

    msgScroll_ = new QScrollArea(conv);
    msgScroll_->setObjectName(QStringLiteral("msgArea"));
    msgScroll_->setWidgetResizable(true);
    msgScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    msgContainer_ = new QWidget(msgScroll_);
    msgContainer_->setStyleSheet(QStringLiteral("background:#0B0A0E;"));
    msgLayout_ = new QVBoxLayout(msgContainer_);
    msgLayout_->setContentsMargins(18, 14, 18, 14);
    msgLayout_->setSpacing(6);
    msgLayout_->addStretch();   // прижимаем сообщения к низу
    msgScroll_->setWidget(msgContainer_);

    auto* composerBar = new QWidget(conv);
    composerBar->setObjectName(QStringLiteral("composerBar"));
    auto* cbl = new QHBoxLayout(composerBar);
    cbl->setContentsMargins(14, 10, 14, 10);
    cbl->setSpacing(10);
    composer_ = new QLineEdit(composerBar);
    composer_->setObjectName(QStringLiteral("composer"));
    composer_->setPlaceholderText(QStringLiteral("Сообщение…"));
    sendBtn_ = new QPushButton(QStringLiteral("➤"), composerBar);
    sendBtn_->setObjectName(QStringLiteral("sendBtn"));
    sendBtn_->setCursor(Qt::PointingHandCursor);
    cbl->addWidget(composer_, 1);
    cbl->addWidget(sendBtn_);

    cvl->addWidget(convHeader);
    cvl->addWidget(msgScroll_, 1);
    cvl->addWidget(composerBar);

    convStack_->addWidget(empty);   // 0
    convStack_->addWidget(conv);    // 1
    convStack_->setCurrentIndex(0);

    root->addWidget(sidebar);
    root->addWidget(convStack_, 1);

    connect(logoutBtn, &QPushButton::clicked, this, &ChatPage::logoutRequested);
    connect(chatList_, &QListWidget::itemClicked, this, &ChatPage::onChatClicked);
    connect(sendBtn_, &QPushButton::clicked, this, &ChatPage::onSendClicked);
    connect(composer_, &QLineEdit::returnPressed, this, &ChatPage::onSendClicked);
    connect(search_, &QLineEdit::textChanged, this, &ChatPage::onSearchChanged);
}

void ChatPage::load() {
    api_->getChats();
    if (!Session::instance().token.isEmpty())
        ws_->start(Session::instance().token);
}

int ChatPage::indexOfChat(const QString& id) const {
    for (int i = 0; i < chats_.size(); ++i)
        if (chats_[i].id == id) return i;
    return -1;
}

void ChatPage::onChatsLoaded(const QList<Chat>& chats) {
    chats_ = chats;
    rebuildChatList();
}

void ChatPage::rebuildChatList() {
    const QString filter = search_->text().trimmed().toLower();
    chatList_->blockSignals(true);
    chatList_->clear();

    for (const Chat& c : chats_) {
        if (!filter.isEmpty() &&
            !c.displayName.toLower().contains(filter) &&
            !c.name.toLower().contains(filter))
            continue;

        auto* row = new QWidget();
        row->setStyleSheet(QStringLiteral("background:transparent;"));
        auto* rl = new QHBoxLayout(row);
        rl->setContentsMargins(12, 8, 12, 8);
        rl->setSpacing(10);

        QString avatarText = c.isSaved ? QStringLiteral("★")
                                       : (c.avatarText.isEmpty() ? c.displayName : c.avatarText);
        rl->addWidget(makeAvatar(avatarText, 48));

        auto* mid = new QVBoxLayout();
        mid->setSpacing(2);
        auto* topRow = new QHBoxLayout();
        topRow->setSpacing(6);
        auto* name = new QLabel(row);
        name->setStyleSheet(QStringLiteral("color:#F3F1F8;font-size:14px;font-weight:600;"));
        name->setText(elide(c.displayName, name->font(), 200));
        auto* time = new QLabel(c.time == QStringLiteral("Нет сообщений") ? QString() : c.time, row);
        time->setStyleSheet(QStringLiteral("color:#726C82;font-size:11px;"));
        topRow->addWidget(name);
        topRow->addStretch();
        topRow->addWidget(time);

        auto* botRow = new QHBoxLayout();
        botRow->setSpacing(6);
        auto* last = new QLabel(row);
        last->setStyleSheet(QStringLiteral("color:#ACA6BD;font-size:13px;"));
        last->setText(elide(c.lastMessage, last->font(), 210));
        botRow->addWidget(last);
        botRow->addStretch();
        if (c.unread > 0) {
            auto* badge = new QLabel(QString::number(c.unread), row);
            badge->setAlignment(Qt::AlignCenter);
            badge->setStyleSheet(QStringLiteral(
                "background:#8B5CF6;color:#fff;font-size:11px;font-weight:700;"
                "border-radius:9px;min-width:18px;min-height:18px;padding:0 5px;"));
            botRow->addWidget(badge);
        }

        mid->addLayout(topRow);
        mid->addLayout(botRow);
        rl->addLayout(mid, 1);

        auto* item = new QListWidgetItem(chatList_);
        item->setSizeHint(QSize(0, 64));
        item->setData(Qt::UserRole, c.id);
        chatList_->addItem(item);
        chatList_->setItemWidget(item, row);

        if (c.id == currentPeerId_)
            item->setSelected(true);
    }
    chatList_->blockSignals(false);
}

void ChatPage::onChatClicked() {
    auto* item = chatList_->currentItem();
    if (!item) return;
    const QString id = item->data(Qt::UserRole).toString();
    const int idx = indexOfChat(id);
    if (idx < 0) return;
    openChat(chats_[idx]);
}

void ChatPage::openChat(const Chat& chat) {
    currentPeerId_   = chat.id;
    currentPeerName_ = chat.displayName;
    peerName_->setText(chat.displayName);
    peerStatus_->setText(chat.isSaved ? QStringLiteral("Заметки для себя")
                                      : (chat.online ? QStringLiteral("в сети")
                                                     : QStringLiteral("не в сети")));
    convStack_->setCurrentIndex(1);
    clearMessages();
    api_->getMessages(chat.id);

    // Сброс непрочитанных в списке
    const int idx = indexOfChat(chat.id);
    if (idx >= 0 && chats_[idx].unread > 0) {
        chats_[idx].unread = 0;
        rebuildChatList();
    }
}

void ChatPage::clearMessages() {
    QLayoutItem* it;
    while ((it = msgLayout_->takeAt(0)) != nullptr) {
        if (it->widget()) it->widget()->deleteLater();
        delete it;
    }
    msgLayout_->addStretch();
    shownIds_.clear();
}

void ChatPage::onMessagesLoaded(const QString& friendId, const QList<ChatMessage>& messages) {
    if (friendId != currentPeerId_) return;
    clearMessages();
    for (const ChatMessage& m : messages) {
        if (!m.id.isEmpty()) shownIds_.insert(m.id);
        addBubble(m);
    }
    scrollToBottom();
}

void ChatPage::addBubble(const ChatMessage& msg) {
    auto* row = new QWidget(msgContainer_);
    row->setStyleSheet(QStringLiteral("background:transparent;"));
    auto* rl = new QHBoxLayout(row);
    rl->setContentsMargins(0, 0, 0, 0);
    rl->setSpacing(0);

    auto* bubble = new QFrame(row);
    bubble->setMaximumWidth(480);
    if (msg.sent) {
        bubble->setStyleSheet(QStringLiteral(
            "background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #4A3A72,stop:1 #3A2D5C);"
            "border-radius:18px;"));
    } else {
        bubble->setStyleSheet(QStringLiteral("background:#1A1822;border-radius:18px;"));
    }
    auto* bl = new QVBoxLayout(bubble);
    bl->setContentsMargins(14, 8, 14, 6);
    bl->setSpacing(2);

    // Reply-превью (если есть)
    if (!msg.replySnippet.isEmpty()) {
        auto* reply = new QLabel(
            QStringLiteral("%1\n%2").arg(msg.replyAuthor, elide(msg.replySnippet, font(), 360)), bubble);
        reply->setStyleSheet(QStringLiteral(
            "color:#BBA4FF;font-size:12px;border-left:2px solid #8B5CF6;padding-left:8px;"));
        bl->addWidget(reply);
    }

    auto* text = new QLabel(msg.content, bubble);
    text->setWordWrap(true);
    text->setTextInteractionFlags(Qt::TextSelectableByMouse);
    text->setStyleSheet(QString("color:%1;font-size:15px;")
                            .arg(msg.sent ? QStringLiteral("#F0ECFA") : QStringLiteral("#F3F1F8")));
    bl->addWidget(text);

    QString metaText = msg.time;
    if (msg.sent) {
        const QString tick = (msg.status == QStringLiteral("read"))
            ? QStringLiteral("✓✓") : (msg.status == QStringLiteral("delivered")
            ? QStringLiteral("✓✓") : QStringLiteral("✓"));
        metaText += QStringLiteral("  ") + tick;
    }
    auto* meta = new QLabel(metaText, bubble);
    meta->setStyleSheet(QString("color:%1;font-size:11px;")
        .arg(msg.sent ? QStringLiteral("rgba(240,236,250,0.65)") : QStringLiteral("#726C82")));
    meta->setAlignment(Qt::AlignRight);
    bl->addWidget(meta);

    if (msg.sent) { rl->addStretch(); rl->addWidget(bubble); }
    else          { rl->addWidget(bubble); rl->addStretch(); }

    msgLayout_->addWidget(row);   // после стартового stretch → прижато к низу
}

void ChatPage::scrollToBottom() {
    QTimer::singleShot(0, this, [this]() {
        msgScroll_->verticalScrollBar()->setValue(msgScroll_->verticalScrollBar()->maximum());
    });
}

void ChatPage::onSendClicked() {
    const QString text = composer_->text().trimmed();
    if (text.isEmpty() || currentPeerId_.isEmpty()) return;

    const QString tempId = QStringLiteral("tmp_%1").arg(++tempCounter_);
    ChatMessage m;
    m.content = text;
    m.sent = true;
    m.status = QStringLiteral("sent");
    m.time = QTime::currentTime().toString(QStringLiteral("HH:mm"));
    m.id = tempId;
    shownIds_.insert(tempId);

    addBubble(m);
    scrollToBottom();
    composer_->clear();

    api_->sendMessage(currentPeerId_, text, tempId);
    bumpChat(currentPeerId_, text, m.time, /*incrementUnread*/ false);
}

void ChatPage::onMessageSent(const ChatMessage& msg, const QString& receiverId, const QString& tempId) {
    Q_UNUSED(receiverId);
    Q_UNUSED(tempId);
    if (!msg.id.isEmpty()) shownIds_.insert(msg.id);   // чтобы WS-эхо не задублировало
}

void ChatPage::onWsMessage(const QString& peerId, const ChatMessage& msgIn, const QString& tempId) {
    ChatMessage msg = msgIn;
    msg.sent = (msg.senderId == Session::instance().userId);

    const bool dupTemp = !tempId.isEmpty() && shownIds_.contains(tempId);
    const bool dupId   = !msg.id.isEmpty() && shownIds_.contains(msg.id);

    if (peerId == currentPeerId_ && !dupTemp && !dupId) {
        if (!msg.id.isEmpty()) shownIds_.insert(msg.id);
        addBubble(msg);
        scrollToBottom();
    }

    bumpChat(peerId, msg.content, msg.time,
             /*incrementUnread*/ (!msg.sent && peerId != currentPeerId_));
}

void ChatPage::bumpChat(const QString& peerId, const QString& lastText,
                        const QString& time, bool incrementUnread) {
    const int idx = indexOfChat(peerId);
    if (idx < 0) {
        api_->getChats();   // новый собеседник — перезагрузим список
        return;
    }
    chats_[idx].lastMessage = lastText;
    if (!time.isEmpty()) chats_[idx].time = time;
    if (incrementUnread) chats_[idx].unread += 1;
    rebuildChatList();
}

void ChatPage::onSearchChanged(const QString&) {
    rebuildChatList();
}
