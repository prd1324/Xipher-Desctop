#include "ui/ImageViewer.h"

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QEvent>

ImageViewer::ImageViewer(QWidget* parent, const QPixmap& pm)
    : QWidget(parent), pm_(pm) {
    setAttribute(Qt::WA_DeleteOnClose);
    setFocusPolicy(Qt::StrongFocus);
    if (parent) {
        setGeometry(parent->rect());
        parent->installEventFilter(this);
    }
}

void ImageViewer::show(QWidget* window, const QPixmap& pm) {
    if (!window || pm.isNull()) return;
    auto* v = new ImageViewer(window, pm);
    v->QWidget::show();
    v->raise();
    v->setFocus();
}

bool ImageViewer::eventFilter(QObject* obj, QEvent* e) {
    if (obj == parentWidget() && e->type() == QEvent::Resize)
        setGeometry(parentWidget()->rect());
    return QWidget::eventFilter(obj, e);
}

void ImageViewer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, 225));
    if (pm_.isNull()) return;
    const QSize area = size() * 0.92;
    QPixmap scaled = pm_.scaled(area, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const int x = (width() - scaled.width()) / 2;
    const int y = (height() - scaled.height()) / 2;
    p.drawPixmap(x, y, scaled);
}

void ImageViewer::mousePressEvent(QMouseEvent*) { close(); }
void ImageViewer::keyPressEvent(QKeyEvent* e) { if (e->key() == Qt::Key_Escape) close(); else QWidget::keyPressEvent(e); }
