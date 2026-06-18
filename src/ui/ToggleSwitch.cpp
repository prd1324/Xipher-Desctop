#include "ui/ToggleSwitch.h"

#include <QPainter>
#include <QPropertyAnimation>

ToggleSwitch::ToggleSwitch(QWidget* parent) : QAbstractButton(parent) {
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFixedSize(44, 26);
    connect(this, &QAbstractButton::toggled, this, [this](bool on) { animateTo(on); });
}

QSize ToggleSwitch::sizeHint() const { return QSize(44, 26); }

void ToggleSwitch::enterEvent(QEnterEvent*) { update(); }

void ToggleSwitch::animateTo(bool on) {
    auto* a = new QPropertyAnimation(this, "pos", this);
    a->setDuration(150);
    a->setStartValue(pos_);
    a->setEndValue(on ? 1.0 : 0.0);
    a->start(QAbstractAnimation::DeleteWhenStopped);
}

void ToggleSwitch::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const qreal h = height();
    const qreal r = h / 2.0;

    // Трек: от серого (выкл) к фиолетовому (вкл).
    const QColor off(0x3A, 0x37, 0x46);
    const QColor on(0x8B, 0x5C, 0xF6);
    QColor track(
        int(off.red()   + (on.red()   - off.red())   * pos_),
        int(off.green() + (on.green() - off.green()) * pos_),
        int(off.blue()  + (on.blue()  - off.blue())  * pos_));
    p.setPen(Qt::NoPen);
    p.setBrush(track);
    p.drawRoundedRect(rect(), r, r);

    // Бегунок.
    const qreal d = h - 6;
    const qreal x = 3 + pos_ * (width() - d - 6);
    p.setBrush(QColor(0xF3, 0xF1, 0xF8));
    p.drawEllipse(QRectF(x, 3, d, d));
}
