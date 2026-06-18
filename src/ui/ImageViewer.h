#pragma once
#include <QWidget>
#include <QPixmap>

// ─────────────────────────────────────────────────────────────────────────────
//  ImageViewer — просмотр фото на весь экран (оверлей внутри окна): затемнённый
//  фон, картинка по центру вписана в окно, закрытие по клику/Esc.
// ─────────────────────────────────────────────────────────────────────────────
class ImageViewer : public QWidget {
    Q_OBJECT
public:
    static void show(QWidget* window, const QPixmap& pm);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    explicit ImageViewer(QWidget* parent, const QPixmap& pm);
    QPixmap pm_;
};
