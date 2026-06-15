#include "ui/BackgroundWidget.h"
#include "ui/Theme.h"

#include <QPainter>
#include <QRadialGradient>
#include <QLinearGradient>

BackgroundWidget::BackgroundWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, false);
}

void BackgroundWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const qreal w = width();
    const qreal h = height();

    // База #05060f
    p.fillRect(rect(), Theme::BgBase);

    // radial-gradient(1200px at 18% 12%, rgba(88,80,220,.18), transparent 60%)
    {
        QRadialGradient g(QPointF(0.18 * w, 0.12 * h), 1200.0);
        QColor c = Theme::GlowPurple; c.setAlphaF(0.18);
        QColor t = Theme::GlowPurple; t.setAlphaF(0.0);
        g.setColorAt(0.0, c);
        g.setColorAt(0.6, t);
        p.fillRect(rect(), g);
    }

    // radial-gradient(900px at 82% -6%, rgba(0,168,255,.18), transparent 55%)
    {
        QRadialGradient g(QPointF(0.82 * w, -0.06 * h), 900.0);
        QColor c = Theme::GlowBlue; c.setAlphaF(0.18);
        QColor t = Theme::GlowBlue; t.setAlphaF(0.0);
        g.setColorAt(0.0, c);
        g.setColorAt(0.55, t);
        p.fillRect(rect(), g);
    }

    // linear-gradient(180deg, rgba(8,10,24,.9), rgba(5,6,16,.95)) — лёгкая виньетка вниз
    {
        QLinearGradient g(0, 0, 0, h);
        g.setColorAt(0.0, QColor(8, 10, 24, 60));
        g.setColorAt(1.0, QColor(5, 6, 16, 110));
        p.fillRect(rect(), g);
    }
}
