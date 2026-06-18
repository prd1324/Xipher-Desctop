#pragma once
#include <QWidget>

class QLabel;
class QPushButton;
class QTimer;

// ─────────────────────────────────────────────────────────────────────────────
//  CallOverlay — экран звонка поверх окна (как в Telegram): аватар, имя, статус,
//  кнопки. Для входящего — принять/отклонить; для активного — mute/завершить.
// ─────────────────────────────────────────────────────────────────────────────
class CallOverlay : public QWidget {
    Q_OBJECT
public:
    explicit CallOverlay(QWidget* parent);

    void setPeer(const QString& name, const QString& avatarUrl);
    void setStatus(const QString& text);
    void setIncoming(bool incoming);
    void startCallTimer();

signals:
    void hangup();
    void accept();
    void decline();
    void muteToggled(bool muted);

protected:
    void paintEvent(QPaintEvent*) override;
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    QLabel*  avatar_;
    QLabel*  name_;
    QLabel*  status_;
    QWidget* incomingBtns_;
    QWidget* activeBtns_;
    QPushButton* muteBtn_;
    QTimer*  timer_;
    int      secs_ = 0;
    bool     muted_ = false;
};
