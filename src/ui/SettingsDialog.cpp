#include "ui/SettingsDialog.h"
#include "ui/Icons.h"
#include "ui/ToggleSwitch.h"
#include "ui/AvatarUtil.h"
#include "net/ApiClient.h"
#include "net/Session.h"
#include "net/Prefs.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QScrollArea>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QTimeEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QFileDialog>
#include <QDesktopServices>
#include <QFile>
#include <QUrl>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QTime>
#include <QFileInfo>

namespace {

// Карточка-группа с заголовком; body — куда класть строки.
QFrame* sectionCard(const QString& title, QVBoxLayout*& body) {
    auto* card = new QFrame();
    card->setObjectName(QStringLiteral("card"));
    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(16, 14, 16, 14);
    v->setSpacing(10);
    if (!title.isEmpty()) {
        auto* t = new QLabel(title);
        t->setObjectName(QStringLiteral("cardTitle"));
        v->addWidget(t);
    }
    body = v;
    return card;
}

// Строка «подпись слева — контрол справа».
QWidget* rowLabeled(const QString& label, QWidget* control) {
    auto* w = new QWidget();
    auto* h = new QHBoxLayout(w);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(12);
    auto* l = new QLabel(label);
    l->setObjectName(QStringLiteral("rowLabel"));
    h->addWidget(l, 1);
    h->addWidget(control, 0, Qt::AlignRight);
    return w;
}

QComboBox* visibilityCombo(const QString& current) {
    auto* c = new QComboBox();
    c->addItem(QStringLiteral("Все"), QStringLiteral("everyone"));
    c->addItem(QStringLiteral("Контакты"), QStringLiteral("contacts"));
    c->addItem(QStringLiteral("Никто"), QStringLiteral("nobody"));
    const int idx = c->findData(current.isEmpty() ? QStringLiteral("everyone") : current);
    c->setCurrentIndex(idx < 0 ? 0 : idx);
    return c;
}

QScrollArea* scrollPage(QWidget* content) {
    auto* sa = new QScrollArea();
    sa->setObjectName(QStringLiteral("settingsScroll"));
    sa->setWidgetResizable(true);
    sa->setFrameShape(QFrame::NoFrame);
    sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sa->setWidget(content);
    return sa;
}

// Ширина карточки под размер окна: на больших окнах — шире (правая часть просторнее),
// на маленьких — ужимается, чтобы не вылезать за края.
int settingsCardWidth(QWidget* parent) {
    const int pw = parent ? parent->width() : 1000;
    return qBound(660, pw - 90, 920);
}

} // namespace

SettingsDialog::SettingsDialog(ApiClient* api, QWidget* parent)
    : ModalOverlay(parent, settingsCardWidth(parent)), api_(api) {
    card()->setFixedHeight(560);
    card()->setStyleSheet(QStringLiteral(R"QSS(
#modalCard{background:#17151E;border:1px solid rgba(255,255,255,0.08);border-radius:18px;}
QLabel{color:#F3F1F8;}
#settingsHeader{
  background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #6D28D9,stop:0.55 #8B5CF6,stop:1 #B06CF0);
  border-top-left-radius:18px;border-top-right-radius:18px;}
#settingsTitle{font-size:18px;font-weight:800;color:#FFFFFF;}
#closeBtn{background:rgba(255,255,255,0.16);border:none;border-radius:16px;color:#FFFFFF;font-size:16px;font-weight:700;}
#closeBtn:hover{background:rgba(255,255,255,0.30);}
#navWrap{background:#141019;border-radius:14px;}
#settingsNav{background:transparent;border:none;outline:none;}
#settingsNav::item{color:#ACA6BD;padding:9px 10px;border-radius:10px;margin:2px 6px;}
#settingsNav::item:hover{background:#221F2C;color:#F3F1F8;}
#settingsNav::item:selected{background:rgba(139,92,246,0.20);color:#FFFFFF;}
#card{background:#1A1822;border:1px solid rgba(255,255,255,0.06);border-radius:14px;}
#cardTitle{font-size:12px;font-weight:800;color:#9B82C9;letter-spacing:1px;}
#rowLabel{font-size:14px;color:#CFC9DC;}
#hint{font-size:12px;color:#726C82;}
#heroBanner{border-radius:16px;
  background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #2A2140,stop:0.5 #3A2A5E,stop:1 #221A33);
  border:1px solid rgba(139,92,246,0.25);}
#heroName{font-size:19px;font-weight:800;color:#FFFFFF;}
#heroUser{font-size:13px;color:#C9B6FF;}
QLineEdit,QPlainTextEdit,QComboBox,QSpinBox,QTimeEdit{
  background:#131218;border:1px solid rgba(255,255,255,0.10);border-radius:10px;
  min-height:34px;padding:0 10px;color:#F3F1F8;selection-background-color:#8B5CF6;}
QPlainTextEdit{padding:8px 10px;}
QLineEdit:focus,QPlainTextEdit:focus,QComboBox:focus,QSpinBox:focus,QTimeEdit:focus{border:1px solid #8B5CF6;}
QLineEdit:disabled{color:#726C82;}
QComboBox::drop-down{border:none;width:22px;}
QComboBox QAbstractItemView{background:#1A1822;border:1px solid rgba(255,255,255,0.12);
  color:#F3F1F8;selection-background-color:rgba(139,92,246,0.30);outline:none;}
QSpinBox::up-button,QSpinBox::down-button{width:0;border:none;}
#primaryBtn{background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #8B5CF6,stop:1 #6D28D9);
  color:#fff;border:none;border-radius:10px;min-height:38px;padding:0 22px;font-weight:700;}
#primaryBtn:hover{background:#9B72F8;}
#ghostBtn{background:#221F2C;color:#F3F1F8;border:none;border-radius:10px;min-height:34px;padding:0 16px;}
#ghostBtn:hover{background:#2C2838;}
#dangerBtn{background:transparent;color:#E5687A;border:1px solid rgba(229,104,122,0.4);
  border-radius:10px;min-height:34px;padding:0 16px;}
#dangerBtn:hover{background:rgba(229,104,122,0.12);}
#closeBtn{background:transparent;border:none;color:#ACA6BD;font-size:20px;}
#closeBtn:hover{color:#F3F1F8;}
#statusOk{color:#46B98A;font-size:12px;}
#pill{background:rgba(139,92,246,0.18);color:#C9B6FF;border-radius:10px;padding:3px 10px;font-size:12px;font-weight:700;}
QScrollArea{background:transparent;}
QScrollBar:vertical{background:transparent;width:8px;margin:2px;}
QScrollBar::handle:vertical{background:rgba(255,255,255,0.12);border-radius:4px;min-height:36px;}
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}
)QSS"));

    buildChrome();

    connect(api_, &ApiClient::profileLoaded, this, &SettingsDialog::onProfileLoaded);
    api_->getMyProfile();
}

void SettingsDialog::buildChrome() {
    auto* lay = cardLayout();
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // Шапка — градиентная плашка под акцент, скруглённая сверху как карточка.
    auto* head = new QWidget();
    head->setObjectName(QStringLiteral("settingsHeader"));
    head->setAttribute(Qt::WA_StyledBackground, true);
    head->setFixedHeight(56);
    auto* hh = new QHBoxLayout(head);
    hh->setContentsMargins(22, 0, 14, 0);
    auto* title = new QLabel(QStringLiteral("Настройки"));
    title->setObjectName(QStringLiteral("settingsTitle"));
    auto* close = new QPushButton(QStringLiteral("✕"));
    close->setObjectName(QStringLiteral("closeBtn"));
    close->setCursor(Qt::PointingHandCursor);
    close->setFixedSize(32, 32);
    connect(close, &QPushButton::clicked, this, &ModalOverlay::closeAnimated);
    hh->addWidget(title);
    hh->addStretch();
    hh->addWidget(close);
    lay->addWidget(head);

    // Тело: навигация (в своей подложке) + стек.
    auto* bodyW = new QWidget();
    auto* bh = new QHBoxLayout(bodyW);
    bh->setContentsMargins(10, 8, 10, 12);
    bh->setSpacing(8);

    auto* navWrap = new QWidget();
    navWrap->setObjectName(QStringLiteral("navWrap"));
    navWrap->setAttribute(Qt::WA_StyledBackground, true);
    navWrap->setFixedWidth(244);
    auto* nwl = new QVBoxLayout(navWrap);
    nwl->setContentsMargins(2, 8, 2, 8);
    nav_ = new QListWidget();
    nav_->setObjectName(QStringLiteral("settingsNav"));
    nav_->setIconSize(QSize(18, 18));
    nav_->setFocusPolicy(Qt::NoFocus);
    nav_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    nwl->addWidget(nav_);

    stack_ = new QStackedWidget();

    bh->addWidget(navWrap);
    bh->addWidget(stack_, 1);
    lay->addWidget(bodyW, 1);

    connect(nav_, &QListWidget::currentRowChanged, stack_, &QStackedWidget::setCurrentIndex);

    addSection(QStringLiteral("Мой аккаунт"),    Icons::User,   QColor(0x8B,0x5C,0xF6), buildAccountPage());
    addSection(QStringLiteral("Уведомления"),    Icons::Bell,   QColor(0xF0,0x9A,0x3E), buildNotificationsPage());
    addSection(QStringLiteral("Приватность"),    Icons::Shield, QColor(0x46,0xB9,0x8A), buildPrivacyPage());
    addSection(QStringLiteral("Звонки"),         Icons::Phone,  QColor(0x4C,0x9A,0xF5), buildCallsPage());
    addSection(QStringLiteral("Активные сеансы"),Icons::Device, QColor(0x5E,0xC8,0xD8), buildSessionsPage());
    addSection(QStringLiteral("Заблокированные"),Icons::Block,  QColor(0xE5,0x68,0x7A), buildBlockedPage());
    addSection(QStringLiteral("Язык"),           Icons::Globe,  QColor(0x7E,0x8B,0xF0), buildLanguagePage());
    addSection(QStringLiteral("Email для входа"), Icons::Lock,  QColor(0xC0,0x86,0x4E), buildEmailPage());
    addSection(QStringLiteral("Premium"),        Icons::Star,   QColor(0xF0,0xC8,0x4C), buildPremiumPage());
    addSection(QStringLiteral("О приложении"),   Icons::Gear,   QColor(0x9A,0x93,0xAD), buildAboutPage());

    nav_->setCurrentRow(0);
}

void SettingsDialog::addSection(const QString& title, int iconKind,
                                const QColor& iconColor, QWidget* page) {
    auto* item = new QListWidgetItem(Icons::icon(Icons::Kind(iconKind), 18, iconColor), title);
    nav_->addItem(item);
    stack_->addWidget(scrollPage(page));
}

// ── Мой аккаунт ───────────────────────────────────────────────────────────────
QWidget* SettingsDialog::buildAccountPage() {
    auto* page = new QWidget();
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(12, 4, 12, 12);
    v->setSpacing(14);

    // Герой — градиентный баннер с аватаром (как обложка профиля в Telegram).
    auto* hero = new QFrame();
    hero->setObjectName(QStringLiteral("heroBanner"));
    auto* hl = new QHBoxLayout(hero);
    hl->setContentsMargins(18, 16, 18, 16);
    hl->setSpacing(16);
    avatar_ = new QLabel();
    avatar_->setFixedSize(76, 76);
    Avatar::setRound(avatar_, QString(), Session::instance().username, 76);
    auto* camBtn = new QPushButton(QStringLiteral("Сменить фото"));
    camBtn->setCursor(Qt::PointingHandCursor);
    camBtn->setStyleSheet(QStringLiteral(
        "QPushButton{background:rgba(255,255,255,0.14);color:#fff;border:none;"
        "border-radius:9px;min-height:30px;padding:0 14px;font-size:12px;font-weight:600;}"
        "QPushButton:hover{background:rgba(255,255,255,0.24);}"));
    connect(camBtn, &QPushButton::clicked, this, [this]() {
        const QString fn = QFileDialog::getOpenFileName(this, QStringLiteral("Выберите фото"),
            QString(), QStringLiteral("Изображения (*.jpg *.jpeg *.png *.gif)"));
        if (fn.isEmpty()) return;
        QFile f(fn);
        if (!f.open(QIODevice::ReadOnly)) return;
        const QByteArray bytes = f.readAll();
        api_->uploadAvatar(bytes, QFileInfo(fn).fileName());
    });
    // Сам аватар тоже кликабельный (как в Telegram) — открывает тот же выбор фото.
    avatar_->setCursor(Qt::PointingHandCursor);
    avatar_->setToolTip(QStringLiteral("Сменить фото"));
    auto* avHit = new QPushButton(avatar_);
    avHit->setCursor(Qt::PointingHandCursor);
    avHit->setFixedSize(76, 76);
    avHit->setStyleSheet(QStringLiteral(
        "QPushButton{background:transparent;border:none;border-radius:38px;}"
        "QPushButton:hover{background:rgba(0,0,0,0.30);}"));
    connect(avHit, &QPushButton::clicked, camBtn, &QPushButton::click);
    auto* names = new QVBoxLayout();
    names->setSpacing(2);
    heroName_ = new QLabel(Session::instance().username);
    heroName_->setObjectName(QStringLiteral("heroName"));
    heroUser_ = new QLabel(QStringLiteral("@") + Session::instance().username);
    heroUser_->setObjectName(QStringLiteral("heroUser"));
    names->addStretch();
    names->addWidget(heroName_);
    names->addWidget(heroUser_);
    names->addWidget(camBtn, 0, Qt::AlignLeft);
    names->addStretch();
    hl->addWidget(avatar_);
    hl->addLayout(names, 1);
    v->addWidget(hero);

    connect(api_, &ApiClient::avatarUploaded, this, [this](bool ok, const QString& url) {
        if (!ok) return;
        if (!url.isEmpty()) avatarUrl_ = url;
        Avatar::setRound(avatar_, avatarUrl_,
                         heroName_->text().isEmpty() ? Session::instance().username : heroName_->text(), 76);
    });

    // Форма.
    QVBoxLayout* body = nullptr;
    auto* card = sectionCard(QStringLiteral("Основные данные"), body);
    firstName_ = new QLineEdit(); firstName_->setMaxLength(255);
    lastName_  = new QLineEdit(); lastName_->setMaxLength(255);
    bio_       = new QPlainTextEdit(); bio_->setFixedHeight(70);
    usernameRO_= new QLineEdit(); usernameRO_->setDisabled(true);
    body->addWidget(rowLabeled(QStringLiteral("Имя"), firstName_));
    body->addWidget(rowLabeled(QStringLiteral("Фамилия"), lastName_));
    auto* bioL = new QLabel(QStringLiteral("О себе")); bioL->setObjectName(QStringLiteral("rowLabel"));
    body->addWidget(bioL);
    body->addWidget(bio_);
    body->addWidget(rowLabeled(QStringLiteral("Имя пользователя"), usernameRO_));
    v->addWidget(card);

    // День рождения.
    QVBoxLayout* bd = nullptr;
    auto* bdCard = sectionCard(QStringLiteral("День рождения"), bd);
    auto* bdRow = new QWidget();
    auto* bdh = new QHBoxLayout(bdRow); bdh->setContentsMargins(0,0,0,0); bdh->setSpacing(8);
    bDay_   = new QSpinBox(); bDay_->setRange(0, 31);   bDay_->setSpecialValueText(QStringLiteral("—"));
    bMonth_ = new QSpinBox(); bMonth_->setRange(0, 12); bMonth_->setSpecialValueText(QStringLiteral("—"));
    bYear_  = new QSpinBox(); bYear_->setRange(0, 2100);bYear_->setSpecialValueText(QStringLiteral("—"));
    bdh->addWidget(new QLabel(QStringLiteral("День"))); bdh->addWidget(bDay_);
    bdh->addWidget(new QLabel(QStringLiteral("Месяц")));bdh->addWidget(bMonth_);
    bdh->addWidget(new QLabel(QStringLiteral("Год")));  bdh->addWidget(bYear_);
    bdh->addStretch();
    bd->addWidget(bdRow);
    auto* bdHint = new QLabel(QStringLiteral("0 — не указывать")); bdHint->setObjectName(QStringLiteral("hint"));
    bd->addWidget(bdHint);
    v->addWidget(bdCard);

    // Сохранить.
    auto* save = new QPushButton(QStringLiteral("Сохранить"));
    save->setObjectName(QStringLiteral("primaryBtn"));
    save->setCursor(Qt::PointingHandCursor);
    auto* status = new QLabel(); status->setObjectName(QStringLiteral("statusOk"));
    connect(save, &QPushButton::clicked, this, [this, status]() {
        api_->updateMyProfile(firstName_->text(), lastName_->text(), bio_->toPlainText(),
                              bDay_->value(), bMonth_->value(), bYear_->value());
        status->setText(QStringLiteral("Сохраняем…"));
    });
    connect(api_, &ApiClient::profileUpdated, this, [status](bool ok, const QString& m) {
        status->setStyleSheet(ok ? QString() : QStringLiteral("color:#E5687A;font-size:12px;"));
        status->setText(ok ? QStringLiteral("Сохранено ✓")
                           : (QStringLiteral("Ошибка: ") + m));
    });
    auto* saveRow = new QWidget();
    auto* srh = new QHBoxLayout(saveRow); srh->setContentsMargins(0,0,0,0);
    srh->addWidget(status); srh->addStretch(); srh->addWidget(save);
    v->addWidget(saveRow);

    v->addStretch();
    return page;
}

void SettingsDialog::onProfileLoaded(const QJsonObject& obj) {
    const QJsonObject u = obj.value(QStringLiteral("user")).toObject();
    if (u.value(QStringLiteral("id")).toString() != Session::instance().userId) return;

    const QString display = u.value(QStringLiteral("display_name")).toString();
    const QString uname   = u.value(QStringLiteral("username")).toString();
    avatarUrl_ = u.value(QStringLiteral("avatar_url")).toString();
    if (avatar_) Avatar::setRound(avatar_, avatarUrl_, display.isEmpty() ? uname : display, 76);
    if (heroName_) heroName_->setText(display.isEmpty() ? uname : display);
    if (heroUser_) heroUser_->setText(QStringLiteral("@") + uname);
    if (firstName_) firstName_->setText(u.value(QStringLiteral("first_name")).toString());
    if (lastName_)  lastName_->setText(u.value(QStringLiteral("last_name")).toString());
    if (bio_)       bio_->setPlainText(u.value(QStringLiteral("bio")).toString());
    if (usernameRO_) usernameRO_->setText(QStringLiteral("@") + uname);
    if (bDay_)   bDay_->setValue(u.value(QStringLiteral("birth_day")).toInt());
    if (bMonth_) bMonth_->setValue(u.value(QStringLiteral("birth_month")).toInt());
    if (bYear_)  bYear_->setValue(u.value(QStringLiteral("birth_year")).toInt());
}

// ── Уведомления ────────────────────────────────────────────────────────────────
QWidget* SettingsDialog::buildNotificationsPage() {
    auto* page = new QWidget();
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(12, 4, 12, 12);
    v->setSpacing(14);

    auto addToggle = [](QVBoxLayout* body, const QString& label, const QString& key, bool def) {
        auto* sw = new ToggleSwitch();
        sw->setChecked(Prefs::getBool(key, def));
        QObject::connect(sw, &QAbstractButton::toggled, [key](bool on){ Prefs::setBool(key, on); });
        body->addWidget(rowLabeled(label, sw));
    };

    QVBoxLayout* b1 = nullptr;
    auto* c1 = sectionCard(QStringLiteral("Уведомления"), b1);
    addToggle(b1, QStringLiteral("Уведомления на рабочем столе"), QStringLiteral("xipher_notif_desktop"), true);
    addToggle(b1, QStringLiteral("Звук сообщений"), QStringLiteral("xipher_notif_sound"), true);
    addToggle(b1, QStringLiteral("Текст в уведомлении"), QStringLiteral("xipher_notif_preview"), true);
    addToggle(b1, QStringLiteral("Звук звонка"), QStringLiteral("xipher_notif_call_sound"), true);
    v->addWidget(c1);

    QVBoxLayout* b2 = nullptr;
    auto* c2 = sectionCard(QStringLiteral("Тихие часы"), b2);
    auto* from = new QTimeEdit(QTime::fromString(Prefs::getStr(QStringLiteral("xipher_quiet_hours_from"),
        QStringLiteral("23:00")), QStringLiteral("HH:mm")));
    auto* to = new QTimeEdit(QTime::fromString(Prefs::getStr(QStringLiteral("xipher_quiet_hours_to"),
        QStringLiteral("08:00")), QStringLiteral("HH:mm")));
    from->setDisplayFormat(QStringLiteral("HH:mm"));
    to->setDisplayFormat(QStringLiteral("HH:mm"));
    QObject::connect(from, &QTimeEdit::timeChanged, [](const QTime& t){
        Prefs::setStr(QStringLiteral("xipher_quiet_hours_from"), t.toString(QStringLiteral("HH:mm"))); });
    QObject::connect(to, &QTimeEdit::timeChanged, [](const QTime& t){
        Prefs::setStr(QStringLiteral("xipher_quiet_hours_to"), t.toString(QStringLiteral("HH:mm"))); });
    b2->addWidget(rowLabeled(QStringLiteral("С"), from));
    b2->addWidget(rowLabeled(QStringLiteral("До"), to));
    auto* qh = new QLabel(QStringLiteral("В этом диапазоне уведомления беззвучные"));
    qh->setObjectName(QStringLiteral("hint"));
    b2->addWidget(qh);
    v->addWidget(c2);

    QVBoxLayout* b3 = nullptr;
    auto* c3 = sectionCard(QStringLiteral("События"), b3);
    addToggle(b3, QStringLiteral("Контакт присоединился"), QStringLiteral("xipher_notif_contact_join"), true);
    addToggle(b3, QStringLiteral("Закреплённые сообщения"), QStringLiteral("xipher_notif_pinned"), true);
    v->addWidget(c3);

    v->addStretch();
    return page;
}

// ── Приватность ────────────────────────────────────────────────────────────────
QWidget* SettingsDialog::buildPrivacyPage() {
    auto* page = new QWidget();
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(12, 4, 12, 12);
    v->setSpacing(14);

    // Серверные «кто видит».
    QVBoxLayout* b = nullptr;
    auto* card = sectionCard(QStringLiteral("Кто видит"), b);
    auto* lastSeen = visibilityCombo(QString());
    auto* birthday = visibilityCombo(QString());
    auto* bioVis   = visibilityCombo(QString());
    b->addWidget(rowLabeled(QStringLiteral("Последний онлайн"), lastSeen));
    b->addWidget(rowLabeled(QStringLiteral("День рождения"), birthday));
    b->addWidget(rowLabeled(QStringLiteral("О себе"), bioVis));
    auto* readRcpt = new ToggleSwitch();
    readRcpt->setChecked(Prefs::getBool(QStringLiteral("xipher_read_receipts"), true));
    b->addWidget(rowLabeled(QStringLiteral("Отчёты о прочтении"), readRcpt));
    v->addWidget(card);

    // Локальные доп-настройки.
    QVBoxLayout* b2 = nullptr;
    auto* card2 = sectionCard(QStringLiteral("Дополнительно"), b2);
    auto* blur = new ToggleSwitch();
    blur->setChecked(Prefs::getBool(QStringLiteral("xipher_chat_privacy"), false));
    QObject::connect(blur, &QAbstractButton::toggled, [](bool on){
        Prefs::setBool(QStringLiteral("xipher_chat_privacy"), on); });
    b2->addWidget(rowLabeled(QStringLiteral("Размывать превью в списке чатов"), blur));

    auto* autoDel = new QComboBox();
    autoDel->addItem(QStringLiteral("Выключено"), QStringLiteral("off"));
    autoDel->addItem(QStringLiteral("1 день"), QStringLiteral("1d"));
    autoDel->addItem(QStringLiteral("1 неделя"), QStringLiteral("1w"));
    autoDel->addItem(QStringLiteral("1 месяц"), QStringLiteral("1m"));
    {
        const int i = autoDel->findData(Prefs::getStr(QStringLiteral("xipher_auto_delete"), QStringLiteral("off")));
        autoDel->setCurrentIndex(i < 0 ? 0 : i);
    }
    QObject::connect(autoDel, &QComboBox::currentIndexChanged, [autoDel](int){
        Prefs::setStr(QStringLiteral("xipher_auto_delete"), autoDel->currentData().toString()); });
    b2->addWidget(rowLabeled(QStringLiteral("Авто-удаление сообщений"), autoDel));
    v->addWidget(card2);

    // Сохранить серверные.
    auto* save = new QPushButton(QStringLiteral("Сохранить"));
    save->setObjectName(QStringLiteral("primaryBtn"));
    save->setCursor(Qt::PointingHandCursor);
    auto* status = new QLabel(); status->setObjectName(QStringLiteral("statusOk"));
    connect(save, &QPushButton::clicked, this, [this, lastSeen, birthday, bioVis, readRcpt, status]() {
        QJsonObject f{
            {QStringLiteral("last_seen_visibility"), lastSeen->currentData().toString()},
            {QStringLiteral("birth_visibility"), birthday->currentData().toString()},
            {QStringLiteral("bio_visibility"), bioVis->currentData().toString()},
            {QStringLiteral("send_read_receipts"), readRcpt->isChecked()}
        };
        Prefs::setBool(QStringLiteral("xipher_read_receipts"), readRcpt->isChecked());
        api_->updateMyPrivacy(f);
        status->setText(QStringLiteral("Сохраняем…"));
    });
    connect(api_, &ApiClient::privacyUpdated, this, [status](bool ok){
        status->setStyleSheet(ok ? QString() : QStringLiteral("color:#E5687A;font-size:12px;"));
        status->setText(ok ? QStringLiteral("Сохранено ✓") : QStringLiteral("Ошибка сохранения"));
    });
    // подтянуть текущие значения из профиля
    connect(api_, &ApiClient::profileLoaded, this, [lastSeen, birthday, bioVis, readRcpt](const QJsonObject& obj){
        const QJsonObject pr = obj.value(QStringLiteral("user")).toObject().value(QStringLiteral("privacy")).toObject();
        auto set = [](QComboBox* c, const QString& val){ int i = c->findData(val); if (i>=0) c->setCurrentIndex(i); };
        set(lastSeen, pr.value(QStringLiteral("last_seen_visibility")).toString());
        set(birthday, pr.value(QStringLiteral("birth_visibility")).toString());
        set(bioVis, pr.value(QStringLiteral("bio_visibility")).toString());
        if (pr.contains(QStringLiteral("send_read_receipts")))
            readRcpt->setChecked(pr.value(QStringLiteral("send_read_receipts")).toBool());
    });
    auto* saveRow = new QWidget();
    auto* srh = new QHBoxLayout(saveRow); srh->setContentsMargins(0,0,0,0);
    srh->addWidget(status); srh->addStretch(); srh->addWidget(save);
    v->addWidget(saveRow);

    v->addStretch();
    return page;
}

// ── Звонки ─────────────────────────────────────────────────────────────────────
QWidget* SettingsDialog::buildCallsPage() {
    auto* page = new QWidget();
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(12, 4, 12, 12);
    v->setSpacing(14);

    QVBoxLayout* b = nullptr;
    auto* card = sectionCard(QStringLiteral("Звонки"), b);
    auto* accept = new ToggleSwitch();
    accept->setChecked(Prefs::getBool(QStringLiteral("xipher_calls_allow"), true));
    QObject::connect(accept, &QAbstractButton::toggled, [](bool on){ Prefs::setBool(QStringLiteral("xipher_calls_allow"), on); });
    b->addWidget(rowLabeled(QStringLiteral("Принимать звонки"), accept));

    auto* who = new QComboBox();
    who->addItem(QStringLiteral("Все"), QStringLiteral("everyone"));
    who->addItem(QStringLiteral("Контакты"), QStringLiteral("contacts"));
    who->addItem(QStringLiteral("Никто"), QStringLiteral("nobody"));
    { int i = who->findData(Prefs::getStr(QStringLiteral("xipher_calls_who"), QStringLiteral("everyone"))); who->setCurrentIndex(i<0?0:i); }
    QObject::connect(who, &QComboBox::currentIndexChanged, [who](int){ Prefs::setStr(QStringLiteral("xipher_calls_who"), who->currentData().toString()); });
    b->addWidget(rowLabeled(QStringLiteral("Кто может звонить"), who));
    v->addWidget(card);

    QVBoxLayout* b2 = nullptr;
    auto* card2 = sectionCard(QStringLiteral("Качество (видео/экран)"), b2);
    auto addQuality = [&](const QString& label, const QString& key, const QString& def) {
        auto* c = new QComboBox();
        c->addItem(QStringLiteral("360p"), QStringLiteral("360"));
        c->addItem(QStringLiteral("540p"), QStringLiteral("540"));
        c->addItem(QStringLiteral("720p"), QStringLiteral("720"));
        c->addItem(QStringLiteral("1080p (Premium)"), QStringLiteral("1080"));
        int i = c->findData(Prefs::getStr(key, def)); c->setCurrentIndex(i<0?1:i);
        QObject::connect(c, &QComboBox::currentIndexChanged, [c, key](int){ Prefs::setStr(key, c->currentData().toString()); });
        b2->addWidget(rowLabeled(label, c));
    };
    addQuality(QStringLiteral("Разрешение камеры"), QStringLiteral("xipher_call_camera_quality"), QStringLiteral("540"));
    addQuality(QStringLiteral("Разрешение экрана"), QStringLiteral("xipher_call_screen_quality"), QStringLiteral("720"));
    auto* qhint = new QLabel(QStringLiteral("HD и 60 FPS доступны в Premium. Видеозвонки на десктопе появятся позже."));
    qhint->setObjectName(QStringLiteral("hint")); qhint->setWordWrap(true);
    b2->addWidget(qhint);
    v->addWidget(card2);

    v->addStretch();
    return page;
}

// ── Активные сеансы ──────────────────────────────────────────────────────────────
QWidget* SettingsDialog::buildSessionsPage() {
    auto* page = new QWidget();
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(12, 4, 12, 12);
    v->setSpacing(12);

    QVBoxLayout* b = nullptr;
    auto* card = sectionCard(QStringLiteral("Активные сеансы"), b);
    sessionsBox_ = new QVBoxLayout();
    sessionsBox_->setSpacing(8);
    b->addLayout(sessionsBox_);
    v->addWidget(card);

    auto* endAll = new QPushButton(QStringLiteral("Завершить все другие"));
    endAll->setObjectName(QStringLiteral("dangerBtn"));
    endAll->setCursor(Qt::PointingHandCursor);
    connect(endAll, &QPushButton::clicked, this, [this]() { api_->revokeOtherSessions(); });
    v->addWidget(endAll, 0, Qt::AlignLeft);
    v->addStretch();

    connect(api_, &ApiClient::sessionsLoaded, this, [this](const QList<SessionInfo>& list) {
        if (!sessionsBox_) return;
        while (QLayoutItem* it = sessionsBox_->takeAt(0)) { if (it->widget()) it->widget()->deleteLater(); delete it; }
        if (list.isEmpty()) {
            auto* e = new QLabel(QStringLiteral("Нет активных сеансов"));
            e->setObjectName(QStringLiteral("hint"));
            sessionsBox_->addWidget(e);
            return;
        }
        for (const SessionInfo& s : list) {
            auto* row = new QWidget();
            auto* h = new QHBoxLayout(row); h->setContentsMargins(0,0,0,0); h->setSpacing(8);
            const QString dev = s.userAgent.isEmpty() ? QStringLiteral("Устройство") : s.userAgent;
            auto* info = new QLabel(QStringLiteral("%1\n%2").arg(dev, s.lastSeen.left(16)));
            info->setStyleSheet(QStringLiteral("color:#CFC9DC;font-size:13px;"));
            auto* end = new QPushButton(QStringLiteral("Завершить"));
            end->setObjectName(QStringLiteral("ghostBtn"));
            end->setCursor(Qt::PointingHandCursor);
            const QString sid = s.id;
            connect(end, &QPushButton::clicked, this, [this, sid]() { api_->revokeSession(sid); });
            h->addWidget(info, 1);
            h->addWidget(end, 0, Qt::AlignRight);
            sessionsBox_->addWidget(row);
        }
    });
    connect(api_, &ApiClient::sessionsChanged, this, [this]() { refreshSessions(); });
    refreshSessions();
    return page;
}

void SettingsDialog::refreshSessions() { api_->getSessions(); }

// ── Заблокированные ──────────────────────────────────────────────────────────────
QWidget* SettingsDialog::buildBlockedPage() {
    auto* page = new QWidget();
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(12, 4, 12, 12);
    v->setSpacing(12);
    QVBoxLayout* b = nullptr;
    auto* card = sectionCard(QStringLiteral("Заблокированные пользователи"), b);

    const QJsonArray arr = QJsonDocument::fromJson(
        Prefs::getStr(QStringLiteral("xipher_blocked_users"), QStringLiteral("[]")).toUtf8()).array();
    if (arr.isEmpty()) {
        auto* e = new QLabel(QStringLiteral("Список пуст"));
        e->setObjectName(QStringLiteral("hint"));
        b->addWidget(e);
    } else {
        for (const QJsonValue& it : arr) {
            const QJsonObject o = it.toObject();
            auto* row = new QWidget();
            auto* h = new QHBoxLayout(row); h->setContentsMargins(0,0,0,0);
            auto* nm = new QLabel(o.value(QStringLiteral("name")).toString(
                o.value(QStringLiteral("username")).toString(QStringLiteral("Пользователь"))));
            nm->setStyleSheet(QStringLiteral("color:#CFC9DC;font-size:13px;"));
            h->addWidget(nm, 1);
            b->addWidget(row);
        }
    }
    v->addWidget(card);
    v->addStretch();
    return page;
}

// ── Язык ──────────────────────────────────────────────────────────────────────
QWidget* SettingsDialog::buildLanguagePage() {
    auto* page = new QWidget();
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(12, 4, 12, 12);
    v->setSpacing(12);
    QVBoxLayout* b = nullptr;
    auto* card = sectionCard(QStringLiteral("Язык"), b);
    auto* lang = new QComboBox();
    lang->addItem(QStringLiteral("Русский"), QStringLiteral("ru"));
    lang->addItem(QStringLiteral("English (скоро)"), QStringLiteral("en"));
    { int i = lang->findData(Prefs::getStr(QStringLiteral("xipher_language"), QStringLiteral("ru"))); lang->setCurrentIndex(i<0?0:i); }
    QObject::connect(lang, &QComboBox::currentIndexChanged, [lang](int){ Prefs::setStr(QStringLiteral("xipher_language"), lang->currentData().toString()); });
    b->addWidget(rowLabeled(QStringLiteral("Язык интерфейса"), lang));
    auto* h = new QLabel(QStringLiteral("Полный перевод интерфейса появится в обновлении."));
    h->setObjectName(QStringLiteral("hint")); h->setWordWrap(true);
    b->addWidget(h);
    v->addWidget(card);
    v->addStretch();
    return page;
}

// ── Email для входа ──────────────────────────────────────────────────────────────
QWidget* SettingsDialog::buildEmailPage() {
    auto* page = new QWidget();
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(12, 4, 12, 12);
    v->setSpacing(12);
    QVBoxLayout* b = nullptr;
    auto* card = sectionCard(QStringLiteral("Email для восстановления"), b);
    email_ = new QLineEdit();
    email_->setPlaceholderText(QStringLiteral("you@example.com"));
    b->addWidget(email_);
    auto* h = new QLabel(QStringLiteral("Используется для восстановления пароля."));
    h->setObjectName(QStringLiteral("hint"));
    b->addWidget(h);
    auto* save = new QPushButton(QStringLiteral("Сохранить"));
    save->setObjectName(QStringLiteral("primaryBtn"));
    save->setCursor(Qt::PointingHandCursor);
    auto* status = new QLabel(); status->setObjectName(QStringLiteral("statusOk"));
    connect(save, &QPushButton::clicked, this, [this, status]() {
        api_->setRecoveryEmail(email_->text().trimmed());
        status->setText(QStringLiteral("Сохраняем…"));
    });
    connect(api_, &ApiClient::recoveryEmailSaved, this, [status](bool ok, const QString& m){
        status->setStyleSheet(ok ? QString() : QStringLiteral("color:#E5687A;font-size:12px;"));
        status->setText(ok ? QStringLiteral("Сохранено ✓") : (QStringLiteral("Ошибка: ") + m));
    });
    connect(api_, &ApiClient::recoveryEmailLoaded, this, [this](const QString& e){ if (email_) email_->setText(e); });
    auto* row = new QWidget(); auto* rh = new QHBoxLayout(row); rh->setContentsMargins(0,0,0,0);
    rh->addWidget(status); rh->addStretch(); rh->addWidget(save);
    b->addWidget(row);
    v->addWidget(card);
    v->addStretch();
    api_->getRecoveryEmail();
    return page;
}

// ── Premium ──────────────────────────────────────────────────────────────────────
QWidget* SettingsDialog::buildPremiumPage() {
    auto* page = new QWidget();
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(12, 4, 12, 12);
    v->setSpacing(14);

    QVBoxLayout* b = nullptr;
    auto* card = sectionCard(QString(), b);
    auto* head = new QWidget(); auto* hh = new QHBoxLayout(head); hh->setContentsMargins(0,0,0,0);
    auto* t = new QLabel(QStringLiteral("Xipher Premium")); t->setObjectName(QStringLiteral("heroName"));
    auto* pill = new QLabel(Session::instance().isPremium ? QStringLiteral("Активен") : QStringLiteral("Неактивен"));
    pill->setObjectName(QStringLiteral("pill"));
    hh->addWidget(t); hh->addStretch(); hh->addWidget(pill);
    b->addWidget(head);
    auto* sub = new QLabel(QStringLiteral("Новый уровень для чатов, профиля и звонков."));
    sub->setObjectName(QStringLiteral("hint"));
    b->addWidget(sub);
    v->addWidget(card);

    QVBoxLayout* fb = nullptr;
    auto* feat = sectionCard(QStringLiteral("Возможности"), fb);
    const QStringList feats = {
        QStringLiteral("×2 лимиты каналов и закреплений"),
        QStringLiteral("Анимированные видео-аватары"),
        QStringLiteral("Перевод сообщений ru/en"),
        QStringLiteral("HD-звонки и демонстрация экрана"),
        QStringLiteral("Боты и Bot IDE"),
        QStringLiteral("Значок профиля ★"),
        QStringLiteral("Xipher для бизнеса"),
    };
    for (const QString& f : feats) {
        auto* l = new QLabel(QStringLiteral("•  ") + f);
        l->setStyleSheet(QStringLiteral("color:#CFC9DC;font-size:13px;"));
        fb->addWidget(l);
    }
    v->addWidget(feat);

    if (!Session::instance().isPremium) {
        auto* buy = new QPushButton(QStringLiteral("Оформить Premium"));
        buy->setObjectName(QStringLiteral("primaryBtn"));
        buy->setCursor(Qt::PointingHandCursor);
        connect(buy, &QPushButton::clicked, this, []() {
            QDesktopServices::openUrl(QUrl(QStringLiteral("https://messenger.xipher.pro")));
        });
        v->addWidget(buy, 0, Qt::AlignLeft);
        auto* h = new QLabel(QStringLiteral("Оплата доступна в веб-версии."));
        h->setObjectName(QStringLiteral("hint"));
        v->addWidget(h);
    }
    v->addStretch();
    return page;
}

// ── О приложении / FAQ ───────────────────────────────────────────────────────────
QWidget* SettingsDialog::buildAboutPage() {
    auto* page = new QWidget();
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(12, 4, 12, 12);
    v->setSpacing(14);

    QVBoxLayout* b = nullptr;
    auto* card = sectionCard(QStringLiteral("О приложении"), b);
    auto* name = new QLabel(QStringLiteral("Xipher Desktop")); name->setObjectName(QStringLiteral("heroName"));
    auto* ver = new QLabel(QStringLiteral("Версия 0.1 · нативный клиент (Qt)")); ver->setObjectName(QStringLiteral("hint"));
    b->addWidget(name); b->addWidget(ver);
    v->addWidget(card);

    QVBoxLayout* fb = nullptr;
    auto* faq = sectionCard(QStringLiteral("FAQ"), fb);
    auto addQA = [&](const QString& q, const QString& a) {
        auto* qq = new QLabel(q); qq->setStyleSheet(QStringLiteral("color:#F3F1F8;font-size:13px;font-weight:700;"));
        auto* aa = new QLabel(a); aa->setObjectName(QStringLiteral("hint")); aa->setWordWrap(true);
        fb->addWidget(qq); fb->addWidget(aa);
    };
    addQA(QStringLiteral("Как сменить фото профиля?"),
          QStringLiteral("Настройки → Мой аккаунт → «Сменить фото»."));
    addQA(QStringLiteral("Почему звонок соединяется не сразу?"),
          QStringLiteral("При VPN соединение идёт через TURN-сервер и занимает несколько секунд."));
    addQA(QStringLiteral("Где включить приватность?"),
          QStringLiteral("Настройки → Приватность: кто видит онлайн, ДР, отчёты о прочтении."));
    v->addWidget(faq);
    v->addStretch();
    return page;
}
