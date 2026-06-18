#include "ui/CallOverlay.h"
#include "ui/AvatarUtil.h"
#include "ui/Icons.h"

#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QEvent>

static QPushButton* roundCallBtn(const QColor& bg, Icons::Kind icon, QWidget* p) {
    auto* b = new QPushButton(p);
    b->setCursor(Qt::PointingHandCursor);
    b->setFixedSize(64, 64);
    b->setIcon(Icons::icon(icon, 26, QColor(0xFF, 0xFF, 0xFF)));
    b->setIconSize(QSize(26, 26));
    b->setStyleSheet(QString("QPushButton{border:none;border-radius:32px;background:%1;}"
                             "QPushButton:hover{background:%2;}")
                         .arg(bg.name(), bg.lighter(115).name()));
    return b;
}

CallOverlay::CallOverlay(QWidget* parent) : QWidget(parent) {
    setGeometry(parent->rect());
    parent->installEventFilter(this);

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addStretch(2);

    avatar_ = new QLabel(this);
    avatar_->setAlignment(Qt::AlignCenter);
    name_ = new QLabel(this);
    name_->setAlignment(Qt::AlignCenter);
    name_->setStyleSheet(QStringLiteral("color:#FFFFFF;font-size:26px;font-weight:800;"));
    status_ = new QLabel(QStringLiteral("Соединение…"), this);
    status_->setAlignment(Qt::AlignCenter);
    status_->setStyleSheet(QStringLiteral("color:#BBB6C8;font-size:15px;"));

    lay->addWidget(avatar_, 0, Qt::AlignHCenter);
    lay->addSpacing(20);
    lay->addWidget(name_);
    lay->addSpacing(6);
    lay->addWidget(status_);
    lay->addStretch(3);

    // Кнопки активного звонка: mute + завершить.
    activeBtns_ = new QWidget(this);
    auto* abl = new QHBoxLayout(activeBtns_);
    abl->setSpacing(40);
    abl->addStretch();
    muteBtn_ = roundCallBtn(QColor(0x2B, 0x27, 0x37), Icons::Mic, activeBtns_);
    auto* end = roundCallBtn(QColor(0xE2, 0x53, 0x4B), Icons::Phone, activeBtns_);
    abl->addWidget(muteBtn_);
    abl->addWidget(end);
    abl->addStretch();
    connect(end, &QPushButton::clicked, this, &CallOverlay::hangup);
    connect(muteBtn_, &QPushButton::clicked, this, [this]() {
        muted_ = !muted_;
        muteBtn_->setIcon(Icons::icon(Icons::Mic, 26,
            muted_ ? QColor(0xE2, 0x6A, 0x63) : QColor(0xFF, 0xFF, 0xFF)));
        emit muteToggled(muted_);
    });

    // Кнопки входящего: отклонить + принять.
    incomingBtns_ = new QWidget(this);
    auto* ibl = new QHBoxLayout(incomingBtns_);
    ibl->setSpacing(40);
    ibl->addStretch();
    auto* decline = roundCallBtn(QColor(0xE2, 0x53, 0x4B), Icons::Phone, incomingBtns_);
    auto* accept = roundCallBtn(QColor(0x46, 0xB9, 0x8A), Icons::Phone, incomingBtns_);
    ibl->addWidget(decline);
    ibl->addWidget(accept);
    ibl->addStretch();
    connect(decline, &QPushButton::clicked, this, &CallOverlay::decline);
    connect(accept, &QPushButton::clicked, this, &CallOverlay::accept);

    lay->addWidget(activeBtns_);
    lay->addWidget(incomingBtns_);
    lay->addStretch(1);

    timer_ = new QTimer(this);
    timer_->setInterval(1000);
    connect(timer_, &QTimer::timeout, this, [this]() {
        ++secs_;
        status_->setText(QStringLiteral("%1:%2").arg(secs_ / 60, 2, 10, QLatin1Char('0'))
                                                .arg(secs_ % 60, 2, 10, QLatin1Char('0')));
    });
}

void CallOverlay::setPeer(const QString& name, const QString& avatarUrl) {
    name_->setText(name);
    Avatar::setRound(avatar_, avatarUrl, name, 130);
}

void CallOverlay::setStatus(const QString& text) { status_->setText(text); }

void CallOverlay::setIncoming(bool incoming) {
    incomingBtns_->setVisible(incoming);
    activeBtns_->setVisible(!incoming);
}

void CallOverlay::startCallTimer() {
    secs_ = 0;
    timer_->start();
}

void CallOverlay::paintEvent(QPaintEvent*) {
    QPainter p(this);
    QLinearGradient g(0, 0, 0, height());
    g.setColorAt(0, QColor(0x1A, 0x16, 0x24));
    g.setColorAt(1, QColor(0x0B, 0x0A, 0x0E));
    p.fillRect(rect(), g);
}

bool CallOverlay::eventFilter(QObject* obj, QEvent* e) {
    if (obj == parentWidget() && e->type() == QEvent::Resize && isVisible())
        setGeometry(parentWidget()->rect());
    return QWidget::eventFilter(obj, e);
}
