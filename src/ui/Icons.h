#pragma once
#include <QPixmap>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QColor>

// ─────────────────────────────────────────────────────────────────────────────
//  Icons — простые векторные line-иконки (QPainter), вместо эмодзи-глифов.
//  Рисуются в виалбоксе 24×24 и масштабируются. Возвращают QPixmap/QIcon.
// ─────────────────────────────────────────────────────────────────────────────
namespace Icons {

enum Kind { Send, Mic, Smile, Paperclip, Clock, Trash, File, Image,
            Search, Pencil, Phone, Location, Checklist, Logout, Plus, More, Lock };

inline QPixmap pixmap(Kind kind, int size, const QColor& color) {
    const qreal dpr = 2.0;
    QPixmap pm(int(size * dpr), int(size * dpr));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.scale(size / 24.0, size / 24.0);

    QPen pen(color);
    pen.setWidthF(2.0);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    switch (kind) {
    case Send: {
        QPainterPath path;
        path.moveTo(3, 12); path.lineTo(21, 3); path.lineTo(14, 21);
        path.lineTo(11.5, 13.5); path.closeSubpath();
        p.setBrush(color); p.setPen(Qt::NoPen);
        p.drawPath(path);
        break;
    }
    case Mic: {
        p.drawRoundedRect(QRectF(9, 3, 6, 11), 3, 3);
        QPainterPath arc;
        arc.moveTo(6, 11); arc.arcTo(QRectF(6, 5, 12, 12), 180, 180);
        p.drawPath(arc);
        p.drawLine(QLineF(12, 18, 12, 21));
        p.drawLine(QLineF(9, 21, 15, 21));
        break;
    }
    case Smile: {
        p.drawEllipse(QPointF(12, 12), 9, 9);
        p.setBrush(color);
        p.drawEllipse(QPointF(9, 10), 1.1, 1.1);
        p.drawEllipse(QPointF(15, 10), 1.1, 1.1);
        p.setBrush(Qt::NoBrush);
        QPainterPath smile;
        smile.moveTo(8, 14); smile.arcTo(QRectF(8, 10.5, 8, 6), 200, 140);
        p.drawPath(smile);
        break;
    }
    case Paperclip: {
        QPainterPath clip;
        clip.moveTo(16, 8);
        clip.lineTo(8.5, 15.5);
        clip.arcTo(QRectF(6, 13, 5, 5), 135, 180);
        clip.lineTo(17, 7);
        clip.arcTo(QRectF(15, 4.5, 5.5, 5.5), 135, 200);
        clip.lineTo(7, 16);
        p.drawPath(clip);
        break;
    }
    case Clock: {
        p.drawEllipse(QPointF(12, 12), 9, 9);
        p.drawLine(QLineF(12, 12, 12, 7.5));
        p.drawLine(QLineF(12, 12, 15.5, 13.5));
        break;
    }
    case Trash: {
        p.drawLine(QLineF(4, 6.5, 20, 6.5));
        p.drawRoundedRect(QRectF(6, 6.5, 12, 14), 2, 2);
        p.drawLine(QLineF(9, 3.5, 15, 3.5));
        p.drawLine(QLineF(10, 10, 10, 17));
        p.drawLine(QLineF(14, 10, 14, 17));
        break;
    }
    case File: {
        QPainterPath f;
        f.moveTo(6, 3); f.lineTo(14, 3); f.lineTo(19, 8); f.lineTo(19, 21);
        f.lineTo(6, 21); f.closeSubpath();
        p.drawPath(f);
        p.drawPolyline(QPolygonF({QPointF(14, 3), QPointF(14, 8), QPointF(19, 8)}));
        break;
    }
    case Image: {
        p.drawRoundedRect(QRectF(3, 5, 18, 14), 2.5, 2.5);
        p.drawEllipse(QPointF(8.5, 10), 1.8, 1.8);
        QPainterPath m;
        m.moveTo(5, 18); m.lineTo(11, 12); m.lineTo(15, 16); m.lineTo(17, 14); m.lineTo(20, 18);
        p.drawPath(m);
        break;
    }
    case Search: {
        p.drawEllipse(QPointF(10.5, 10.5), 6.5, 6.5);
        p.drawLine(QLineF(15.2, 15.2, 20, 20));
        break;
    }
    case Pencil: {
        QPainterPath pp;
        pp.moveTo(4, 20); pp.lineTo(4, 16.5); pp.lineTo(15.5, 5);
        pp.lineTo(19, 8.5); pp.lineTo(7.5, 20); pp.closeSubpath();
        p.drawPath(pp);
        p.drawLine(QLineF(13.5, 7, 17, 10.5));
        break;
    }
    case Phone: {
        QPainterPath ph;
        ph.moveTo(5, 4); ph.lineTo(9, 4); ph.lineTo(11, 9); ph.lineTo(8.5, 11);
        ph.arcTo(QRectF(8.5, 11, 9, 9), 90, -90);
        ph.lineTo(15, 13); ph.lineTo(20, 15); ph.lineTo(20, 19);
        ph.arcTo(QRectF(4, 4, 16, 16), 0, 0);
        p.drawPath(ph);
        break;
    }
    case Location: {
        QPainterPath pin;
        pin.moveTo(12, 21);
        pin.arcTo(QRectF(5, 2, 14, 14), -60, 300);
        pin.closeSubpath();
        p.drawPath(pin);
        p.drawEllipse(QPointF(12, 9), 2.4, 2.4);
        break;
    }
    case Checklist: {
        QPen tp2 = pen; tp2.setWidthF(2.2); p.setPen(tp2);
        p.drawPolyline(QPolygonF({QPointF(3, 6), QPointF(4.5, 7.5), QPointF(7, 4.5)}));
        p.drawPolyline(QPolygonF({QPointF(3, 13), QPointF(4.5, 14.5), QPointF(7, 11.5)}));
        p.drawPolyline(QPolygonF({QPointF(3, 20), QPointF(4.5, 21.5), QPointF(7, 18.5)}));
        p.drawLine(QLineF(10, 6, 21, 6));
        p.drawLine(QLineF(10, 13, 21, 13));
        p.drawLine(QLineF(10, 20, 21, 20));
        break;
    }
    case Logout: {
        QPainterPath d;
        d.moveTo(13, 4); d.lineTo(6, 4); d.lineTo(6, 20); d.lineTo(13, 20);
        p.drawPath(d);
        p.drawLine(QLineF(11, 12, 21, 12));
        p.drawPolyline(QPolygonF({QPointF(17.5, 8.5), QPointF(21, 12), QPointF(17.5, 15.5)}));
        break;
    }
    case Plus: {
        p.drawLine(QLineF(12, 5, 12, 19));
        p.drawLine(QLineF(5, 12, 19, 12));
        break;
    }
    case More: {
        p.setBrush(color); p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(12, 5.5), 1.7, 1.7);
        p.drawEllipse(QPointF(12, 12), 1.7, 1.7);
        p.drawEllipse(QPointF(12, 18.5), 1.7, 1.7);
        break;
    }
    case Lock: {
        p.drawRoundedRect(QRectF(5, 10.5, 14, 10), 2.5, 2.5);
        QPainterPath sh;
        sh.moveTo(8, 10.5); sh.lineTo(8, 7.5);
        sh.arcTo(QRectF(8, 3.5, 8, 8), 180, -180);
        sh.lineTo(16, 10.5);
        p.drawPath(sh);
        break;
    }
    }
    p.end();
    return pm;
}

inline QIcon icon(Kind kind, int size, const QColor& color) {
    return QIcon(pixmap(kind, size, color));
}

} // namespace Icons
