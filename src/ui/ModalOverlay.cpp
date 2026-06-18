#include "ui/ModalOverlay.h"

#include <QVBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>

ModalOverlay::ModalOverlay(QWidget* parent, int cardWidth)
    : QWidget(parent), cardWidth_(cardWidth) {
    setAttribute(Qt::WA_StyledBackground, false);
    raise();

    card_ = new QWidget(this);
    card_->setObjectName(QStringLiteral("modalCard"));
    card_->setStyleSheet(QStringLiteral(
        "#modalCard{background:#17151E;border:1px solid rgba(255,255,255,0.08);border-radius:18px;}"));
    auto* shadow = new QGraphicsDropShadowEffect(card_);
    shadow->setBlurRadius(60);
    shadow->setOffset(0, 18);
    shadow->setColor(QColor(0, 0, 0, 160));
    card_->setGraphicsEffect(shadow);

    cardLayout_ = new QVBoxLayout(card_);
    cardLayout_->setContentsMargins(22, 20, 22, 20);
    cardLayout_->setSpacing(12);

    if (parent) parent->installEventFilter(this);   // следим за ресайзом окна
    setGeometry(parent ? parent->rect() : QRect());
}

void ModalOverlay::relayout() {
    if (parentWidget()) setGeometry(parentWidget()->rect());
    card_->adjustSize();
    int w = qMin(cardWidth_, width() - 40);
    card_->setFixedWidth(w);
    card_->move((width() - w) / 2, (height() - card_->height()) / 2);
}

bool ModalOverlay::eventFilter(QObject* obj, QEvent* e) {
    if (obj == parentWidget() && e->type() == QEvent::Resize) relayout();
    return QWidget::eventFilter(obj, e);
}

void ModalOverlay::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, int(150 * dim_)));   // затемнение фона
}

void ModalOverlay::mousePressEvent(QMouseEvent* e) {
    // Клик вне карточки → закрыть.
    if (!card_->geometry().contains(e->pos())) closeAnimated();
}

void ModalOverlay::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape) closeAnimated();
    else QWidget::keyPressEvent(e);
}

void ModalOverlay::showAnimated() {
    show();
    raise();
    setFocus();
    relayout();

    auto* fade = new QPropertyAnimation(this, "dim", this);
    fade->setDuration(160);
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);
    fade->start(QAbstractAnimation::DeleteWhenStopped);

    const QPoint to = card_->pos();
    auto* slide = new QPropertyAnimation(card_, "pos", this);
    slide->setDuration(200);
    slide->setStartValue(QPoint(to.x(), to.y() + 18));
    slide->setEndValue(to);
    slide->setEasingCurve(QEasingCurve::OutCubic);
    slide->start(QAbstractAnimation::DeleteWhenStopped);
}

void ModalOverlay::closeAnimated() {
    auto* fade = new QPropertyAnimation(this, "dim", this);
    fade->setDuration(140);
    fade->setStartValue(dim_);
    fade->setEndValue(0.0);
    connect(fade, &QPropertyAnimation::finished, this, [this]() {
        emit closed();
        deleteLater();
    });
    fade->start(QAbstractAnimation::DeleteWhenStopped);
}
