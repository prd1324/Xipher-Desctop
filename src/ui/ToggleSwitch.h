#pragma once
#include <QAbstractButton>

// ─────────────────────────────────────────────────────────────────────────────
//  ToggleSwitch — переключатель в стиле iOS/Telegram (трек + бегунок, анимация).
//  checkable QAbstractButton: сигнал toggled(bool) как у QCheckBox.
// ─────────────────────────────────────────────────────────────────────────────
class ToggleSwitch : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(qreal pos READ pos WRITE setPos)
public:
    explicit ToggleSwitch(QWidget* parent = nullptr);

    QSize sizeHint() const override;
    qreal pos() const { return pos_; }
    void  setPos(qreal p) { pos_ = p; update(); }

protected:
    void paintEvent(QPaintEvent*) override;
    void enterEvent(QEnterEvent*) override;

private:
    void animateTo(bool on);
    qreal pos_ = 0.0;   // 0 = выкл, 1 = вкл
};
