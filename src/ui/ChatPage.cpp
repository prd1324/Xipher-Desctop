#include "ui/ChatPage.h"
#include "ui/NewChatDialog.h"
#include "ui/VoiceMessageWidget.h"
#include "ui/RecordingBar.h"
#include "ui/EmojiPicker.h"
#include "ui/Icons.h"
#include "ui/AvatarUtil.h"
#include "ui/Checklist.h"
#include "ui/ModalOverlay.h"
#include "ui/EmptyChatGreeting.h"
#include "net/ApiClient.h"
#include "net/WsClient.h"
#include "net/Session.h"
#include "net/VoiceRecorder.h"

#include <QMenu>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QToolTip>
#include <QDate>
#include <QLocale>
#include <QPixmap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

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
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QRegularExpression>
#include <algorithm>

namespace {

QString elide(const QString& s, const QFont& f, int px) {
    return QFontMetrics(f).elidedText(s.isEmpty() ? QString() : s, Qt::ElideRight, px);
}

// Подпись голосового с длительностью (хранится в content, т.к. сервер не отдаёт duration).
QString voiceLabel(int secs) {
    return QStringLiteral("🎤 %1:%2").arg(secs / 60).arg(secs % 60, 2, 10, QLatin1Char('0'));
}

QJsonObject parseChecklist(const QString& content) {
    if (!content.startsWith(ChecklistProto::kPrefix)) return {};
    const QString raw = content.mid(ChecklistProto::kPrefix.length()).trimmed();
    return QJsonDocument::fromJson(raw.toUtf8()).object();
}
QJsonObject parseChecklistUpdate(const QString& content) {
    if (!content.startsWith(ChecklistProto::kUpdatePrefix)) return {};
    const QString raw = content.mid(ChecklistProto::kUpdatePrefix.length()).trimmed();
    return QJsonDocument::fromJson(raw.toUtf8()).object();
}
bool isGeoContent(const QString& c) {
    return c.startsWith(QStringLiteral("geo:")) || c.contains(QStringLiteral("yandex.")) ||
           c.contains(QStringLiteral("maps?pt=")) || c.contains(QStringLiteral("maps?ll="));
}

// Человекочитаемый размер файла.
QString humanSize(long long bytes) {
    if (bytes <= 0) return QString();
    const char* u[] = {"Б", "КБ", "МБ", "ГБ"};
    double v = double(bytes);
    int i = 0;
    while (v >= 1024.0 && i < 3) { v /= 1024.0; ++i; }
    return QStringLiteral("%1 %2").arg(v, 0, 'f', (i == 0 ? 0 : 1)).arg(QString::fromUtf8(u[i]));
}

// Метка дня для разделителя (Сегодня / Вчера / 13 июня).
QString dateLabel(const QString& createdAt) {
    const QDate d = QDate::fromString(createdAt.left(10), QStringLiteral("yyyy-MM-dd"));
    if (!d.isValid()) return QString();
    const QDate today = QDate::currentDate();
    if (d == today) return QStringLiteral("Сегодня");
    if (d == today.addDays(-1)) return QStringLiteral("Вчера");
    const QLocale ru(QLocale::Russian);
    return ru.toString(d, d.year() == today.year() ? QStringLiteral("d MMMM")
                                                    : QStringLiteral("d MMMM yyyy"));
}

// Центрированная «пилюля»-разделитель дат.
QWidget* makeDateSeparator(const QString& label) {
    auto* row = new QWidget();
    row->setStyleSheet(QStringLiteral("background:transparent;"));
    auto* l = new QHBoxLayout(row);
    l->setContentsMargins(0, 10, 0, 6);
    l->addStretch();
    auto* pill = new QLabel(label);
    pill->setStyleSheet(QStringLiteral(
        "background:rgba(255,255,255,0.07);color:#ACA6BD;font-size:12px;font-weight:600;"
        "padding:4px 12px;border-radius:11px;"));
    l->addWidget(pill);
    l->addStretch();
    return row;
}

// Достаёт секунды из подписи вида "🎤 m:ss" (0 — если нет).
int parseVoiceSeconds(const QString& content) {
    QRegularExpression re(QStringLiteral("(\\d+):(\\d{2})"));
    auto m = re.match(content);
    if (!m.hasMatch()) return 0;
    return m.captured(1).toInt() * 60 + m.captured(2).toInt();
}

// Круглый аватар: картинка по url (если есть) либо буква на градиенте.
QLabel* makeAvatar(const QString& url, const QString& text, int size) {
    auto* a = new QLabel();
    Avatar::setRound(a, url, text, size);
    return a;
}

} // namespace

ChatPage::ChatPage(ApiClient* api, WsClient* ws, QWidget* parent)
    : QWidget(parent), api_(api), ws_(ws) {
    recorder_ = new VoiceRecorder(this);
    player_   = new QMediaPlayer(this);
    audioOut_ = new QAudioOutput(this);
    player_->setAudioOutput(audioOut_);
    searchTimer_ = new QTimer(this);
    searchTimer_->setSingleShot(true);
    searchTimer_->setInterval(350);

    buildUi();

    connect(searchTimer_, &QTimer::timeout, this, [this]() {
        if (!searchQuery_.isEmpty()) api_->searchUsers(searchQuery_);
    });
    connect(api_, &ApiClient::usersFound, this,
            [this](const QString& query, const QList<UserHit>& users) {
        if (query != searchQuery_) return;   // устаревший/чужой ответ
        searchHits_ = users;
        rebuildChatList();
    });

    connect(api_, &ApiClient::chatsLoaded,    this, &ChatPage::onChatsLoaded);
    connect(api_, &ApiClient::messagesLoaded,  this, &ChatPage::onMessagesLoaded);
    connect(api_, &ApiClient::messageSent,     this, &ChatPage::onMessageSent);
    connect(ws_,  &WsClient::newMessage,       this, &ChatPage::onWsMessage);
    connect(api_, &ApiClient::voiceUploaded,   this, &ChatPage::onVoiceUploaded);
    connect(api_, &ApiClient::fileFetched,     this, &ChatPage::onFileFetched);
    connect(recorder_, &VoiceRecorder::recordingFinished, this, &ChatPage::onVoiceRecorded);
    connect(recorder_, &VoiceRecorder::error, this, [this](const QString&) {
        cancelRecording();
    });

    // Прогресс воспроизведения → активный голосовой виджет.
    connect(player_, &QMediaPlayer::positionChanged, this, [this](qint64 pos) {
        if (!activeVoice_) return;
        const qint64 dur = player_->duration();
        activeVoice_->setProgress(dur > 0 ? qreal(pos) / dur : 0.0);
        activeVoice_->setElapsedMs(pos);
    });
    connect(player_, &QMediaPlayer::durationChanged, this, [this](qint64 dur) {
        if (activeVoice_) activeVoice_->setTotalMs(dur);
    });
    connect(player_, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus s) {
        if (s == QMediaPlayer::EndOfMedia && activeVoice_) {
            activeVoice_->setPlaying(false);
            activeVoice_->setProgress(0.0);
            activeVoice_->setElapsedMs(0);
        }
    });
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
/* Тонкий ненавязчивый скроллбар (как в Telegram): без стрелок и фона */
#msgArea QScrollBar:vertical { background:transparent; width:8px; margin:2px; }
#msgArea QScrollBar::handle:vertical {
    background:rgba(255,255,255,0.12); border-radius:4px; min-height:36px;
}
#msgArea QScrollBar::handle:vertical:hover { background:rgba(255,255,255,0.22); }
#msgArea QScrollBar::add-line:vertical, #msgArea QScrollBar::sub-line:vertical { height:0; }
#msgArea QScrollBar::add-page:vertical, #msgArea QScrollBar::sub-page:vertical { background:transparent; }
/* То же для списка чатов */
#chatList QScrollBar:vertical { background:transparent; width:8px; margin:2px; }
#chatList QScrollBar::handle:vertical {
    background:rgba(255,255,255,0.10); border-radius:4px; min-height:36px;
}
#chatList QScrollBar::handle:vertical:hover { background:rgba(255,255,255,0.20); }
#chatList QScrollBar::add-line:vertical, #chatList QScrollBar::sub-line:vertical { height:0; }
#chatList QScrollBar::add-page:vertical, #chatList QScrollBar::sub-page:vertical { background:transparent; }
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
#micBtn {
    min-width:44px; max-width:44px; min-height:44px; max-height:44px; border:none;
    border-radius:22px; background:#1A1822; color:#ACA6BD; font-size:16px;
}
#micBtn:hover { color:#F3F1F8; background:#221F2C; }
#composerIcon {
    min-width:38px; max-width:38px; min-height:38px; max-height:38px; border:none;
    border-radius:19px; background:transparent; color:#ACA6BD; font-size:18px;
}
#composerIcon:hover { color:#F3F1F8; background:#221F2C; }
QMenu {
    background:#1A1822; border:1px solid rgba(255,255,255,0.12); border-radius:10px; padding:6px;
    color:#F3F1F8;
}
QMenu::item { padding:8px 18px; border-radius:6px; }
QMenu::item:selected { background:rgba(139,92,246,0.22); }
QMenu::item:disabled { color:#55556a; }
QMenu::separator { height:1px; background:rgba(255,255,255,0.08); margin:4px 8px; }
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
    auto* newChatBtn = new QPushButton(QStringLiteral(" Новый"), sideHeader);
    newChatBtn->setObjectName(QStringLiteral("iconBtn"));
    newChatBtn->setCursor(Qt::PointingHandCursor);
    newChatBtn->setIcon(Icons::icon(Icons::Pencil, 15, QColor(0xAC, 0xA6, 0xBD)));
    newChatBtn->setIconSize(QSize(15, 15));
    auto* logoutBtn = new QPushButton(QStringLiteral("Выйти"), sideHeader);
    logoutBtn->setObjectName(QStringLiteral("iconBtn"));
    logoutBtn->setCursor(Qt::PointingHandCursor);
    shl->addWidget(brand);
    shl->addStretch();
    shl->addWidget(newChatBtn);
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
    peerAvatar_ = makeAvatar(QString(), QStringLiteral("?"), 40);
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

    // Приветствие пустого чата — оверлей поверх области сообщений.
    greeting_ = new EmptyChatGreeting(msgScroll_);
    greeting_->hide();
    msgScroll_->viewport()->installEventFilter(this);
    connect(greeting_, &EmptyChatGreeting::greetingClicked, this, [this](const QString& e) {
        if (currentPeerId_.isEmpty()) return;
        composer_->setText(e);
        onSendClicked();
    });

    auto* composerBar = new QWidget(conv);
    composerBar->setObjectName(QStringLiteral("composerBar"));
    auto* cblOuter = new QVBoxLayout(composerBar);
    cblOuter->setContentsMargins(0, 0, 0, 0);
    composerStack_ = new QStackedWidget(composerBar);
    cblOuter->addWidget(composerStack_);

    // Страница 0 — обычный ввод [⏱][📎][текст][😀][🎤][➤]
    auto* normal = new QWidget();
    auto* cbl = new QHBoxLayout(normal);
    cbl->setContentsMargins(12, 10, 12, 10);
    cbl->setSpacing(6);

    const QColor iconClr(0xAC, 0xA6, 0xBD);
    timerBtn_ = new QPushButton(normal);
    timerBtn_->setObjectName(QStringLiteral("composerIcon"));
    timerBtn_->setCursor(Qt::PointingHandCursor);
    timerBtn_->setToolTip(QStringLiteral("Исчезающие сообщения"));
    timerBtn_->setIcon(Icons::icon(Icons::Clock, 20, iconClr));
    timerBtn_->setIconSize(QSize(20, 20));

    attachBtn_ = new QPushButton(normal);
    attachBtn_->setObjectName(QStringLiteral("composerIcon"));
    attachBtn_->setCursor(Qt::PointingHandCursor);
    attachBtn_->setToolTip(QStringLiteral("Прикрепить"));
    attachBtn_->setIcon(Icons::icon(Icons::Paperclip, 20, iconClr));
    attachBtn_->setIconSize(QSize(20, 20));

    composer_ = new QLineEdit(normal);
    composer_->setObjectName(QStringLiteral("composer"));
    composer_->setPlaceholderText(QStringLiteral("Сообщение…"));

    emojiBtn_ = new QPushButton(normal);
    emojiBtn_->setObjectName(QStringLiteral("composerIcon"));
    emojiBtn_->setCursor(Qt::PointingHandCursor);
    emojiBtn_->setToolTip(QStringLiteral("Эмодзи"));
    emojiBtn_->setIcon(Icons::icon(Icons::Smile, 20, iconClr));
    emojiBtn_->setIconSize(QSize(20, 20));

    micBtn_ = new QPushButton(normal);
    micBtn_->setObjectName(QStringLiteral("micBtn"));
    micBtn_->setCursor(Qt::PointingHandCursor);
    micBtn_->setIcon(Icons::icon(Icons::Mic, 20, iconClr));
    micBtn_->setIconSize(QSize(20, 20));
    sendBtn_ = new QPushButton(normal);
    sendBtn_->setObjectName(QStringLiteral("sendBtn"));
    sendBtn_->setCursor(Qt::PointingHandCursor);
    sendBtn_->setIcon(Icons::icon(Icons::Send, 20, QColor(0xFF, 0xFF, 0xFF)));
    sendBtn_->setIconSize(QSize(20, 20));

    cbl->addWidget(timerBtn_);
    cbl->addWidget(attachBtn_);
    cbl->addWidget(composer_, 1);
    cbl->addWidget(emojiBtn_);
    cbl->addWidget(micBtn_);
    cbl->addWidget(sendBtn_);

    // Страница 1 — запись (стиль Discord): пульс + waveform + таймер.
    recBar_ = new RecordingBar();

    composerStack_->addWidget(normal);    // 0
    composerStack_->addWidget(recBar_);   // 1
    composerStack_->setCurrentIndex(0);

    cvl->addWidget(convHeader);
    cvl->addWidget(msgScroll_, 1);
    cvl->addWidget(composerBar);

    convStack_->addWidget(empty);   // 0
    convStack_->addWidget(conv);    // 1
    convStack_->setCurrentIndex(0);

    root->addWidget(sidebar);
    root->addWidget(convStack_, 1);

    connect(newChatBtn, &QPushButton::clicked, this, &ChatPage::openNewChatDialog);
    connect(logoutBtn, &QPushButton::clicked, this, &ChatPage::logoutRequested);
    connect(chatList_, &QListWidget::itemClicked, this, &ChatPage::onChatClicked);
    connect(sendBtn_, &QPushButton::clicked, this, &ChatPage::onSendClicked);
    connect(composer_, &QLineEdit::returnPressed, this, &ChatPage::onSendClicked);
    connect(search_, &QLineEdit::textChanged, this, &ChatPage::onSearchChanged);
    connect(micBtn_, &QPushButton::clicked, this, &ChatPage::onMicClicked);
    connect(recBar_, &RecordingBar::cancelClicked, this, &ChatPage::cancelRecording);
    connect(recBar_, &RecordingBar::sendClicked, this, &ChatPage::stopAndSendVoice);
    connect(emojiBtn_, &QPushButton::clicked, this, &ChatPage::onEmojiClicked);
    connect(attachBtn_, &QPushButton::clicked, this, &ChatPage::onAttachClicked);
    connect(timerBtn_, &QPushButton::clicked, this, &ChatPage::onTimerClicked);
    connect(api_, &ApiClient::fileUploaded, this, &ChatPage::onFileUploaded);
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

// Строка контакта (аватар + имя + подзаголовок + опц. время/бейдж).
static QWidget* buildContactRow(const QString& url, const QString& avatarText,
                                const QString& title, const QString& subtitle,
                                const QString& time, int unread) {
    auto* row = new QWidget();
    row->setStyleSheet(QStringLiteral("background:transparent;"));
    auto* rl = new QHBoxLayout(row);
    rl->setContentsMargins(12, 8, 12, 8);
    rl->setSpacing(10);

    auto* av = new QLabel();
    Avatar::setRound(av, url, avatarText, 48);
    rl->addWidget(av);

    auto* mid = new QVBoxLayout();
    mid->setSpacing(2);
    auto* topRow = new QHBoxLayout();
    topRow->setSpacing(6);
    auto* name = new QLabel(row);
    name->setStyleSheet(QStringLiteral("color:#F3F1F8;font-size:14px;font-weight:600;"));
    name->setText(elide(title, name->font(), 200));
    topRow->addWidget(name);
    topRow->addStretch();
    if (!time.isEmpty()) {
        auto* t = new QLabel(time, row);
        t->setStyleSheet(QStringLiteral("color:#726C82;font-size:11px;"));
        topRow->addWidget(t);
    }

    auto* botRow = new QHBoxLayout();
    botRow->setSpacing(6);
    auto* last = new QLabel(row);
    last->setStyleSheet(QStringLiteral("color:#ACA6BD;font-size:13px;"));
    last->setText(elide(subtitle, last->font(), 210));
    botRow->addWidget(last);
    botRow->addStretch();
    if (unread > 0) {
        auto* badge = new QLabel(QString::number(unread), row);
        badge->setAlignment(Qt::AlignCenter);
        badge->setStyleSheet(QStringLiteral(
            "background:#8B5CF6;color:#fff;font-size:11px;font-weight:700;"
            "border-radius:9px;min-width:18px;min-height:18px;padding:0 5px;"));
        botRow->addWidget(badge);
    }
    mid->addLayout(topRow);
    mid->addLayout(botRow);
    rl->addLayout(mid, 1);
    return row;
}

void ChatPage::rebuildChatList() {
    const QString filter = search_->text().trimmed().toLower();
    chatList_->blockSignals(true);
    chatList_->clear();

    QSet<QString> shownChatIds;
    for (const Chat& c : chats_) {
        if (!filter.isEmpty() &&
            !c.displayName.toLower().contains(filter) &&
            !c.name.toLower().contains(filter))
            continue;
        shownChatIds.insert(c.id);

        const QString avatarText = c.isSaved ? QStringLiteral("★")
                                  : (c.avatarText.isEmpty() ? c.displayName : c.avatarText);
        const QString time = c.time == QStringLiteral("Нет сообщений") ? QString() : c.time;
        auto* row = buildContactRow(c.isSaved ? QString() : c.avatarUrl,
                                    avatarText, c.displayName, c.lastMessage, time, c.unread);

        auto* item = new QListWidgetItem(chatList_);
        item->setSizeHint(QSize(0, 64));
        item->setData(Qt::UserRole, c.id);
        item->setData(Qt::UserRole + 1, false);   // не результат поиска
        chatList_->addItem(item);
        chatList_->setItemWidget(item, row);
        if (c.id == currentPeerId_) item->setSelected(true);
    }

    // Глобальный поиск людей (как в Telegram): показываем найденных, кого ещё нет в чатах.
    if (!filter.isEmpty() && !searchHits_.isEmpty()) {
        bool headerAdded = false;
        for (const UserHit& u : searchHits_) {
            if (shownChatIds.contains(u.id)) continue;
            if (!headerAdded) {
                auto* h = new QListWidgetItem(chatList_);
                h->setFlags(Qt::NoItemFlags);
                h->setSizeHint(QSize(0, 28));
                auto* hl = new QLabel(QStringLiteral("  Люди"));
                hl->setStyleSheet(QStringLiteral("color:#726C82;font-size:11px;font-weight:700;"
                                                 "text-transform:uppercase;padding:6px 4px;"));
                chatList_->addItem(h);
                chatList_->setItemWidget(h, hl);
                headerAdded = true;
            }
            auto* row = buildContactRow(u.avatarUrl, u.displayName.isEmpty() ? u.username : u.displayName,
                                        u.displayName.isEmpty() ? u.username : u.displayName,
                                        QStringLiteral("@") + u.username, QString(), 0);
            auto* item = new QListWidgetItem(chatList_);
            item->setSizeHint(QSize(0, 64));
            item->setData(Qt::UserRole, u.id);
            item->setData(Qt::UserRole + 1, true);   // результат поиска
            item->setData(Qt::UserRole + 2, u.displayName.isEmpty() ? u.username : u.displayName);
            item->setData(Qt::UserRole + 3, u.username);
            chatList_->addItem(item);
            chatList_->setItemWidget(item, row);
        }
    }
    chatList_->blockSignals(false);
}

void ChatPage::onChatClicked() {
    auto* item = chatList_->currentItem();
    if (!item) return;
    const QString id = item->data(Qt::UserRole).toString();
    if (item->data(Qt::UserRole + 1).toBool()) {   // найденный человек → открыть новый чат
        openChatWith(id, item->data(Qt::UserRole + 2).toString(),
                     item->data(Qt::UserRole + 3).toString());
        return;
    }
    const int idx = indexOfChat(id);
    if (idx < 0) return;
    openChat(chats_[idx]);
}

void ChatPage::openChat(const Chat& chat) {
    currentPeerId_   = chat.id;
    currentPeerName_ = chat.displayName;
    peerName_->setText(chat.displayName);
    Avatar::setRound(peerAvatar_, chat.isSaved ? QString() : chat.avatarUrl,
                     chat.isSaved ? QStringLiteral("★") : chat.displayName, 40);
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
    // Останавливаем воспроизведение — виджеты ниже будут удалены.
    player_->stop();
    activeVoice_ = nullptr;
    pendingPlayPath_.clear();

    QLayoutItem* it;
    while ((it = msgLayout_->takeAt(0)) != nullptr) {
        if (it->widget()) it->widget()->deleteLater();
        delete it;
    }
    msgLayout_->addStretch();
    shownIds_.clear();
    checklistWidgets_.clear();
    mergedChecklists_.clear();
    bubbleCount_ = 0;
    updateGreeting();
}

void ChatPage::updateGreeting() {
    if (!greeting_) return;
    const bool show = !currentPeerId_.isEmpty() && bubbleCount_ == 0;
    if (show) {
        greeting_->setGeometry(msgScroll_->viewport()->rect());
        greeting_->raise();
        greeting_->show();
    } else {
        greeting_->hide();
    }
}

bool ChatPage::eventFilter(QObject* obj, QEvent* e) {
    if (greeting_ && obj == msgScroll_->viewport() && e->type() == QEvent::Resize) {
        if (greeting_->isVisible()) greeting_->setGeometry(msgScroll_->viewport()->rect());
    }
    return QWidget::eventFilter(obj, e);
}

void ChatPage::onMessagesLoaded(const QString& friendId, const QList<ChatMessage>& messages) {
    if (friendId != currentPeerId_) return;
    clearMessages();
    // Сервер отдаёт историю в порядке DESC (новые первыми) — разворачиваем в
    // хронологию: старые сверху, новые снизу.
    QList<ChatMessage> ordered = messages;
    std::sort(ordered.begin(), ordered.end(),
              [](const ChatMessage& a, const ChatMessage& b) { return a.createdAt < b.createdAt; });
    QString lastDay;
    for (const ChatMessage& m : ordered) {
        const QString day = m.createdAt.left(10);
        if (!day.isEmpty() && day != lastDay) {
            const QString lbl = dateLabel(m.createdAt);
            if (!lbl.isEmpty()) msgLayout_->addWidget(makeDateSeparator(lbl));
            lastDay = day;
        }
        if (!m.id.isEmpty()) shownIds_.insert(m.id);
        addBubble(m);
    }
    updateGreeting();
    scrollToBottom();
}

void ChatPage::addBubble(const ChatMessage& msg) {
    // Апдейт чек-листа — не рисуем бабблом, применяем к существующему виджету.
    if (msg.content.startsWith(ChecklistProto::kUpdatePrefix)) {
        const QJsonObject upd = parseChecklistUpdate(msg.content);
        const QString clId = upd.value(QStringLiteral("checklistId")).toString();
        // сохраняем накопленное состояние (на случай, если виджет ещё не создан)
        QJsonObject merged = mergedChecklists_.value(clId);
        if (!merged.isEmpty() || checklistWidgets_.contains(clId)) {
            if (auto* w = checklistWidgets_.value(clId)) w->applyUpdate(upd);
        }
        return;
    }

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

    if (msg.isVoice()) {
        // Голосовое (Telegram-style): ▶/⏸ + waveform + время.
        const QString seed = msg.id.isEmpty() ? msg.filePath : msg.id;
        auto* voice = new VoiceMessageWidget(seed, msg.sent, bubble);
        voice->setMinimumWidth(240);
        bubble->setMinimumWidth(280);
        const int vsecs = parseVoiceSeconds(msg.content);   // длительность из подписи
        if (vsecs > 0) voice->setTotalMs(qint64(vsecs) * 1000);
        bl->addWidget(voice);
        const QString path = msg.filePath;
        connect(voice, &VoiceMessageWidget::playPauseClicked, this,
                [this, voice, path]() { onVoicePlayPause(voice, path); });
        connect(voice, &VoiceMessageWidget::seekRequested, this, [this, voice](qreal frac) {
            if (activeVoice_ == voice && player_->duration() > 0)
                player_->setPosition(qint64(frac * player_->duration()));
        });
    } else if (msg.content.startsWith(ChecklistProto::kPrefix)) {
        // Чек-лист (как в вебе): [[XIPHER_CHECKLIST]] + JSON.
        QJsonObject pl = parseChecklist(msg.content);
        const QString clId = pl.value(QStringLiteral("id")).toString();
        if (mergedChecklists_.contains(clId))   // применяем накопленные апдейты
            pl = mergedChecklists_.value(clId);
        const bool canMark = msg.sent || pl.value(QStringLiteral("othersCanMark")).toBool(true);
        auto* cl = new ChecklistWidget(pl, msg.sent, canMark, bubble);
        bubble->setMinimumWidth(300);
        bl->addWidget(cl);
        checklistWidgets_.insert(clId, cl);
        const QString peer = currentPeerId_;
        connect(cl, &ChecklistWidget::itemToggled, this, [this, clId, peer](const QString& itemId, bool done) {
            QJsonObject upd{{QStringLiteral("checklistId"), clId},
                            {QStringLiteral("updates"), QJsonArray{QJsonObject{
                                {QStringLiteral("id"), itemId}, {QStringLiteral("done"), done}}}}};
            const QString content = ChecklistProto::kUpdatePrefix +
                QString::fromUtf8(QJsonDocument(upd).toJson(QJsonDocument::Compact));
            api_->sendRaw(peer, content, QStringLiteral("text"),
                          QStringLiteral("clu_%1").arg(++tempCounter_));
        });
    } else if (msg.messageType == QStringLiteral("location") ||
               msg.messageType == QStringLiteral("live_location") || isGeoContent(msg.content)) {
        // Геопозиция: кнопка-карточка, открывает карту в браузере.
        auto* geo = new QPushButton(bubble);
        geo->setCursor(Qt::PointingHandCursor);
        geo->setIcon(Icons::icon(Icons::Location, 20,
            msg.sent ? QColor(0xF0,0xEC,0xFA) : QColor(0x8B,0x5C,0xF6)));
        geo->setIconSize(QSize(20, 20));
        geo->setText(QStringLiteral("  Геопозиция\n  Открыть на карте"));
        geo->setStyleSheet(QString(
            "QPushButton{border:none;background:transparent;text-align:left;font-size:14px;color:%1;}"
            "QPushButton:hover{text-decoration:underline;}")
            .arg(msg.sent ? QStringLiteral("#F0ECFA") : QStringLiteral("#F3F1F8")));
        bubble->setMinimumWidth(220);
        bl->addWidget(geo);
        const QString url = msg.content;
        connect(geo, &QPushButton::clicked, this, [url]() {
            QString u = url;
            if (u.startsWith(QStringLiteral("geo:"))) {
                const QString c = u.mid(4).split('?').first();
                u = QStringLiteral("https://yandex.ru/maps/?text=") + c;
            }
            QDesktopServices::openUrl(QUrl(u));
        });
    } else if (msg.messageType == QStringLiteral("image") || msg.messageType == QStringLiteral("photo")) {
        // Фото: показываем картинку (локально или тянем с /files).
        auto* img = new QLabel(bubble);
        img->setAlignment(Qt::AlignCenter);
        img->setMinimumSize(180, 120);
        img->setStyleSheet(QStringLiteral("background:rgba(255,255,255,0.05);border-radius:10px;color:#ACA6BD;"));
        img->setText(QStringLiteral("Фото…"));
        bubble->setMinimumWidth(220);
        bl->addWidget(img);
        const QString path = msg.filePath;
        if (!path.isEmpty()) {
            if (!path.startsWith(QStringLiteral("/files"))) {
                QPixmap pm(path);
                if (!pm.isNull()) {
                    img->setPixmap(pm.scaled(280, 360, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    img->setText(QString());
                }
            } else {
                pendingImage_.insert(path, img);
                api_->fetchFile(path);
            }
        }
    } else if (msg.messageType == QStringLiteral("file")) {
        // Файл: 📎 имя + размер, клик — скачать и открыть.
        auto* fbtn = new QPushButton(bubble);
        fbtn->setCursor(Qt::PointingHandCursor);
        fbtn->setFlat(true);
        const QString nm = msg.fileName.isEmpty() ? QStringLiteral("файл") : msg.fileName;
        const QColor fclr = msg.sent ? QColor(0xF0, 0xEC, 0xFA) : QColor(0xF3, 0xF1, 0xF8);
        fbtn->setIcon(Icons::icon(Icons::File, 18, fclr));
        fbtn->setIconSize(QSize(18, 18));
        fbtn->setText(QStringLiteral("  %1   %2").arg(nm, humanSize(msg.fileSize)));
        fbtn->setStyleSheet(QString(
            "QPushButton{border:none;background:transparent;text-align:left;font-size:14px;color:%1;}"
            "QPushButton:hover{text-decoration:underline;}")
            .arg(msg.sent ? QStringLiteral("#F0ECFA") : QStringLiteral("#F3F1F8")));
        bl->addWidget(fbtn);
        const QString path = msg.filePath, name = nm;
        connect(fbtn, &QPushButton::clicked, this, [this, path, name]() {
            if (path.isEmpty()) return;
            if (!path.startsWith(QStringLiteral("/files"))) {   // локальный (только что отправленный)
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
                return;
            }
            pendingFileOpen_.insert(path, name);
            api_->fetchFile(path);
        });
    } else {
        auto* text = new QLabel(msg.content, bubble);
        text->setWordWrap(true);
        text->setTextInteractionFlags(Qt::TextSelectableByMouse);
        text->setStyleSheet(QString("color:%1;font-size:15px;")
                                .arg(msg.sent ? QStringLiteral("#F0ECFA") : QStringLiteral("#F3F1F8")));
        bl->addWidget(text);
    }

    QString metaText = msg.time;
    if (msg.sent) {
        const QString tick = (msg.status == QStringLiteral("read"))
            ? QStringLiteral("✓✓") : (msg.status == QStringLiteral("delivered")
            ? QStringLiteral("✓✓") : QStringLiteral("✓"));
        metaText += QStringLiteral("  ") + tick;
    }
    auto* metaRow = new QHBoxLayout();
    metaRow->setContentsMargins(0, 0, 0, 0);
    metaRow->setSpacing(6);
    metaRow->addStretch();

    // Исчезающее сообщение: иконка-часы + обратный отсчёт.
    if (msg.ttlSeconds > 0) {
        auto* flame = new QLabel(bubble);
        flame->setPixmap(Icons::pixmap(Icons::Clock, 12,
            msg.sent ? QColor(0xF0,0xEC,0xFA) : QColor(0xAC,0xA6,0xBD)));
        auto* ttlLbl = new QLabel(bubble);
        ttlLbl->setStyleSheet(QString("color:%1;font-size:11px;font-weight:600;")
            .arg(msg.sent ? QStringLiteral("rgba(240,236,250,0.8)") : QStringLiteral("#ACA6BD")));

        const qint64 deadline = QDateTime::currentMSecsSinceEpoch() + qint64(msg.ttlSeconds) * 1000;
        auto fmt = [](qint64 sec) -> QString {
            if (sec >= 3600) return QStringLiteral("%1ч").arg(sec / 3600);
            if (sec >= 60)   return QStringLiteral("%1м").arg(sec / 60);
            return QStringLiteral("%1с").arg(sec);
        };
        ttlLbl->setText(fmt(msg.ttlSeconds));

        auto* tick = new QTimer(ttlLbl);
        tick->setInterval(1000);
        QPointer<QWidget> rowGuard(row);
        connect(tick, &QTimer::timeout, this, [this, ttlLbl, deadline, fmt, rowGuard]() {
            const qint64 left = (deadline - QDateTime::currentMSecsSinceEpoch()) / 1000;
            if (left <= 0) {
                if (rowGuard) { rowGuard->deleteLater(); }   // сообщение «сгорело» (локально)
                return;
            }
            ttlLbl->setText(fmt(left));
        });
        tick->start();

        metaRow->addWidget(flame);
        metaRow->addWidget(ttlLbl);
    }

    auto* meta = new QLabel(metaText, bubble);
    meta->setStyleSheet(QString("color:%1;font-size:11px;")
        .arg(msg.sent ? QStringLiteral("rgba(240,236,250,0.65)") : QStringLiteral("#726C82")));
    metaRow->addWidget(meta);
    bl->addLayout(metaRow);

    if (msg.sent) { rl->addStretch(); rl->addWidget(bubble); }
    else          { rl->addWidget(bubble); rl->addStretch(); }

    msgLayout_->addWidget(row);   // после стартового stretch → прижато к низу
    ++bubbleCount_;
    if (greeting_ && greeting_->isVisible()) greeting_->hide();
}

void ChatPage::scrollToBottom() {
    auto* sb = msgScroll_->verticalScrollBar();
    // Диапазон скролла пересчитывается ПОСЛЕ раскладки бабблов (wordwrap и т.п.),
    // поэтому прыгаем в самый низ по сигналу rangeChanged — один раз.
    connect(sb, &QAbstractSlider::rangeChanged, this,
            [sb]() { sb->setValue(sb->maximum()); }, Qt::SingleShotConnection);
    sb->setValue(sb->maximum());
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
    m.ttlSeconds = disappearTtl_;
    shownIds_.insert(tempId);

    addBubble(m);
    scrollToBottom();
    composer_->clear();

    api_->sendMessage(currentPeerId_, text, tempId, disappearTtl_);
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
        auto* sb = msgScroll_->verticalScrollBar();
        const bool nearBottom = (sb->maximum() - sb->value()) < 140;
        if (!msg.id.isEmpty()) shownIds_.insert(msg.id);
        addBubble(msg);
        if (nearBottom || msg.sent) scrollToBottom();   // не дёргаем, если читаешь историю
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

void ChatPage::onSearchChanged(const QString& text) {
    rebuildChatList();   // мгновенно фильтруем существующие чаты
    const QString q = text.trimmed();
    if (q.length() >= 2) {
        searchQuery_ = q;
        searchTimer_->start();   // дебаунс глобального поиска людей
    } else {
        searchQuery_.clear();
        if (!searchHits_.isEmpty()) { searchHits_.clear(); rebuildChatList(); }
    }
}

void ChatPage::openNewChatDialog() {
    auto* dlg = new NewChatDialog(api_, this);
    connect(dlg, &NewChatDialog::openChatRequested, this,
            [this](const QString& id, const QString& displayName, const QString& username) {
        openChatWith(id, displayName, username);
    });
    connect(dlg, &NewChatDialog::friendsChanged, this, [this]() { api_->getChats(); });
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}

void ChatPage::openChatWith(const QString& userId, const QString& displayName, const QString& username) {
    const int idx = indexOfChat(userId);
    if (idx >= 0) {
        openChat(chats_[idx]);
        return;
    }
    // Чата ещё нет в списке — открываем «на лету».
    Chat c;
    c.id = userId;
    c.name = username;
    c.displayName = displayName.isEmpty() ? username : displayName;
    openChat(c);
}

// ── Эмодзи ───────────────────────────────────────────────────────────────────

void ChatPage::onEmojiClicked() {
    if (!emojiPicker_) {
        emojiPicker_ = new EmojiPicker(this);
        connect(emojiPicker_, &EmojiPicker::emojiPicked, this, [this](const QString& e) {
            composer_->insert(e);   // вставляем в позицию курсора, пикер не закрываем
        });
    }
    // Открываем панель над кнопкой, выравнивая по правому краю (как в Telegram).
    const QPoint topRight = emojiBtn_->mapToGlobal(QPoint(emojiBtn_->width(), 0));
    int x = topRight.x() - emojiPicker_->width();
    int y = topRight.y() - emojiPicker_->height() - 8;
    if (y < 0) y = emojiBtn_->mapToGlobal(QPoint(0, emojiBtn_->height() + 8)).y();
    emojiPicker_->move(qMax(8, x), y);
    emojiPicker_->show();
}

// ── Вложения (скрепка) ───────────────────────────────────────────────────────

void ChatPage::onAttachClicked() {
    const QColor mclr(0xAC, 0xA6, 0xBD);
    QMenu menu(this);
    QAction* fileAct = menu.addAction(Icons::icon(Icons::File, 18, mclr), QStringLiteral("Файл"));
    QAction* checklist = menu.addAction(Icons::icon(Icons::Checklist, 18, mclr),
                                        QStringLiteral("Чек-лист"));
    menu.addSeparator();
    QMenu* geo = menu.addMenu(QStringLiteral("Геопозиция"));
    geo->setIcon(Icons::icon(Icons::Location, 18, mclr));
    QAction* geoSend = geo->addAction(QStringLiteral("Отправить"));
    QAction* geoLive = geo->addAction(QStringLiteral("Транслировать (Live, скоро)"));
    geoLive->setEnabled(false);

    connect(fileAct, &QAction::triggered, this, &ChatPage::pickAndSendFile);
    connect(checklist, &QAction::triggered, this, &ChatPage::openChecklistDialog);
    connect(geoSend, &QAction::triggered, this, &ChatPage::sendLocation);
    QPoint pos = attachBtn_->mapToGlobal(QPoint(0, 0));
    pos.setY(pos.y() - menu.sizeHint().height() - 6);   // открываем ВВЕРХ
    menu.exec(pos);
}

// ── Таймер исчезающих (пока UI-состояние) ────────────────────────────────────

void ChatPage::onTimerClicked() {
    QMenu menu(this);
    const QList<QPair<QString, int>> opts = {
        {QStringLiteral("Выключено"), 0}, {QStringLiteral("5 секунд"), 5},
        {QStringLiteral("10 секунд"), 10}, {QStringLiteral("30 секунд"), 30},
        {QStringLiteral("1 минута"), 60}, {QStringLiteral("1 час"), 3600},
        {QStringLiteral("24 часа"), 86400}};
    for (const auto& o : opts) {
        QAction* a = menu.addAction(o.first);
        a->setCheckable(true);
        a->setChecked(disappearTtl_ == o.second);
        const int v = o.second;
        connect(a, &QAction::triggered, this, [this, v]() {
            disappearTtl_ = v;
            timerBtn_->setStyleSheet(v > 0
                ? QStringLiteral("#composerIcon{color:#8B5CF6;}") : QString());
        });
    }
    QPoint pos = timerBtn_->mapToGlobal(QPoint(0, 0));
    pos.setY(pos.y() - menu.sizeHint().height() - 6);   // открываем ВВЕРХ
    menu.exec(pos);
}

// ── Файлы ────────────────────────────────────────────────────────────────────

void ChatPage::pickAndSendFile() {
    if (currentPeerId_.isEmpty()) return;
    const QString fn = QFileDialog::getOpenFileName(this, QStringLiteral("Выберите файл"));
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QByteArray bytes = f.readAll();
    f.close();

    const QString base = QFileInfo(fn).fileName();
    const QString tempId = QStringLiteral("tmpf_%1").arg(++tempCounter_);
    shownIds_.insert(tempId);
    pendingFileReceiver_ = currentPeerId_;

    // Оптимистичный баббл (клик открывает локальный файл).
    ChatMessage m;
    m.id = tempId;
    m.sent = true;
    m.status = QStringLiteral("sent");
    m.messageType = QStringLiteral("file");
    m.fileName = base;
    m.fileSize = bytes.size();
    m.filePath = fn;   // локальный путь
    m.time = QTime::currentTime().toString(QStringLiteral("HH:mm"));
    addBubble(m);
    scrollToBottom();

    api_->uploadFile(bytes, base, tempId);
}

void ChatPage::openChecklistDialog() {
    if (currentPeerId_.isEmpty()) return;

    // Оверлей поверх приложения (как модалки в Telegram), не отдельное окно.
    auto* overlay = new ModalOverlay(window(), 440);
    auto* editor = new ChecklistEditor(overlay->card());
    overlay->cardLayout()->addWidget(editor);

    connect(editor, &ChecklistEditor::cancelled, overlay, &ModalOverlay::closeAnimated);
    connect(editor, &ChecklistEditor::submitted, this,
            [this, overlay](const QJsonObject& payload) {
        overlay->closeAnimated();
        const QString content = ChecklistProto::kPrefix +
            QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
        const QString tempId = QStringLiteral("clk_%1").arg(++tempCounter_);
        shownIds_.insert(tempId);

        ChatMessage m;
        m.id = tempId; m.sent = true; m.status = QStringLiteral("sent");
        m.messageType = QStringLiteral("text");
        m.content = content;
        m.time = QTime::currentTime().toString(QStringLiteral("HH:mm"));
        addBubble(m);
        scrollToBottom();

        api_->sendRaw(currentPeerId_, content, QStringLiteral("text"), tempId);
        bumpChat(currentPeerId_, QStringLiteral("Чек-лист"), m.time, false);
    });

    overlay->showAnimated();
}

void ChatPage::sendLocation() {
    if (currentPeerId_.isEmpty()) return;
    pendingLocReceiver_ = currentPeerId_;
    if (!geoNam_) geoNam_ = new QNetworkAccessManager(this);
    // На десктопе нет GPS — берём приблизительные координаты по IP (ip-api.com).
    QNetworkReply* reply = geoNam_->get(QNetworkRequest(QUrl(
        QStringLiteral("http://ip-api.com/json/?fields=status,lat,lon"))));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        double lat = 0, lon = 0;
        bool ok = false;
        if (reply->error() == QNetworkReply::NoError) {
            const QJsonObject o = QJsonDocument::fromJson(reply->readAll()).object();
            if (o.value(QStringLiteral("status")).toString() == QStringLiteral("success")) {
                lat = o.value(QStringLiteral("lat")).toDouble();
                lon = o.value(QStringLiteral("lon")).toDouble();
                ok = true;
            }
        }
        if (!ok || pendingLocReceiver_.isEmpty()) return;

        const QString content = QStringLiteral("https://yandex.ru/maps/?pt=%1,%2&z=16&l=map")
            .arg(lon, 0, 'f', 6).arg(lat, 0, 'f', 6);
        const QString tempId = QStringLiteral("loc_%1").arg(++tempCounter_);
        shownIds_.insert(tempId);

        ChatMessage m;
        m.id = tempId; m.sent = true; m.status = QStringLiteral("sent");
        m.messageType = QStringLiteral("location");
        m.content = content;
        m.time = QTime::currentTime().toString(QStringLiteral("HH:mm"));
        if (pendingLocReceiver_ == currentPeerId_) { addBubble(m); scrollToBottom(); }

        api_->sendRaw(pendingLocReceiver_, content, QStringLiteral("location"), tempId);
        bumpChat(pendingLocReceiver_, QStringLiteral("Геопозиция"), m.time, false);
        pendingLocReceiver_.clear();
    });
}

void ChatPage::onFileUploaded(const QString& filePath, const QString& fileName,
                              long long fileSize, const QString& tempId) {
    if (pendingFileReceiver_.isEmpty()) return;
    const QString caption = QStringLiteral("📎 ") + fileName;
    api_->sendFile(pendingFileReceiver_, filePath, fileName, fileSize, caption, tempId);
    bumpChat(pendingFileReceiver_, caption,
             QTime::currentTime().toString(QStringLiteral("HH:mm")), false);
}

// ── Голосовые сообщения ──────────────────────────────────────────────────────

void ChatPage::onMicClicked() {
    if (currentPeerId_.isEmpty() || recorder_->isRecording()) return;
    if (recorder_->start()) {
        composerStack_->setCurrentIndex(1);
        recBar_->start();
    }
}

void ChatPage::cancelRecording() {
    recBar_->stop();
    recorder_->cancel();
    composerStack_->setCurrentIndex(0);
}

void ChatPage::stopAndSendVoice() {
    pendingVoiceSecs_ = recBar_->seconds();   // длительность записи
    recBar_->stop();
    composerStack_->setCurrentIndex(0);
    pendingVoiceReceiver_ = currentPeerId_;
    recorder_->stop();   // → onVoiceRecorded
}

void ChatPage::onVoicePlayPause(VoiceMessageWidget* w, const QString& path) {
    // Тот же виджет — пауза/продолжение.
    if (activeVoice_ == w) {
        if (player_->playbackState() == QMediaPlayer::PlayingState) {
            player_->pause();
            w->setPlaying(false);
        } else {
            player_->play();
            w->setPlaying(true);
        }
        return;
    }
    // Переключение на другой голосовой — сбрасываем предыдущий.
    if (activeVoice_) {
        activeVoice_->setPlaying(false);
        activeVoice_->setProgress(0.0);
    }
    activeVoice_ = w;
    activeVoicePath_ = path;
    w->setPlaying(true);
    playVoice(path);
}

void ChatPage::onVoiceRecorded(const QString& filePath, const QString& mimeType) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QByteArray bytes = f.readAll();
    f.close();
    if (bytes.isEmpty()) return;

    const QString tempId = QStringLiteral("tmpv_%1").arg(++tempCounter_);
    pendingVoiceTempId_ = tempId;
    shownIds_.insert(tempId);

    // Оптимистичный баббл (играем сразу из локального файла).
    ChatMessage m;
    m.id = tempId;
    m.sent = true;
    m.status = QStringLiteral("sent");
    m.messageType = QStringLiteral("voice");
    m.filePath = filePath;   // локальный путь → playVoice сыграет напрямую
    m.content = voiceLabel(pendingVoiceSecs_);   // длительность в подписи
    m.time = QTime::currentTime().toString(QStringLiteral("HH:mm"));
    if (pendingVoiceReceiver_ == currentPeerId_) {
        addBubble(m);
        scrollToBottom();
    }

    api_->uploadVoice(bytes, mimeType, tempId);
}

void ChatPage::onVoiceUploaded(const QString& filePath, const QString& fileName,
                               long long fileSize, const QString& tempId) {
    Q_UNUSED(tempId);
    if (pendingVoiceReceiver_.isEmpty()) return;
    const QString caption = voiceLabel(pendingVoiceSecs_);   // "🎤 m:ss"
    api_->sendVoice(pendingVoiceReceiver_, filePath, fileName, fileSize, caption, pendingVoiceTempId_);
    bumpChat(pendingVoiceReceiver_, caption,
             QTime::currentTime().toString(QStringLiteral("HH:mm")), false);
}

void ChatPage::playVoice(const QString& path) {
    if (path.isEmpty()) return;
    // Локальный путь (оптимистичный или уже скачанный) — играем сразу.
    if (!path.startsWith(QStringLiteral("/files"))) {
        player_->setSource(QUrl::fromLocalFile(path));
        player_->play();
        return;
    }
    if (voiceCache_.contains(path)) {
        player_->setSource(QUrl::fromLocalFile(voiceCache_.value(path)));
        player_->play();
        return;
    }
    pendingPlayPath_ = path;
    api_->fetchFile(path);   // скачаем с токеном, потом сыграем
}

void ChatPage::onFileFetched(const QString& filePath, const QByteArray& bytes) {
    // Картинка для сообщения-фото.
    if (pendingImage_.contains(filePath)) {
        QPointer<QLabel> lbl = pendingImage_.take(filePath);
        QPixmap pm;
        if (lbl && pm.loadFromData(bytes)) {
            lbl->setPixmap(pm.scaled(280, 360, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            lbl->setText(QString());
            lbl->setMinimumSize(0, 0);
        }
        return;
    }

    // Скачивание обычного файла → сохраняем в «Загрузки» и открываем.
    if (pendingFileOpen_.contains(filePath)) {
        const QString name = pendingFileOpen_.take(filePath);
        QString dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (dir.isEmpty()) dir = QDir::tempPath();
        const QString dest = QDir(dir).filePath(name);
        QFile out(dest);
        if (out.open(QIODevice::WriteOnly)) {
            out.write(bytes);
            out.close();
            QDesktopServices::openUrl(QUrl::fromLocalFile(dest));
        }
        return;
    }

    // Иначе — голосовое: кэшируем и проигрываем.
    const QString local = QDir::temp().filePath(
        QStringLiteral("xipher_dl_%1.m4a").arg(qHash(filePath)));
    QFile f(local);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(bytes);
        f.close();
        voiceCache_.insert(filePath, local);
        if (filePath == pendingPlayPath_) {
            pendingPlayPath_.clear();
            player_->setSource(QUrl::fromLocalFile(local));
            player_->play();
        }
    }
}
