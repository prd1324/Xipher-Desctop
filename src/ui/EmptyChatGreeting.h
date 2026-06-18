#pragma once
#include <QWidget>

class QMovie;
class QLabel;

// ──────────────────────────────────────────────────────────────────��──────────
//  EmptyChatGreeting — карточка пустого чата (как в Telegram): анимированный
//  эмодзи + текст «Здесь пока ничего нет…». Клик по эмодзи шлёт приветствие 👋.
// ─────────────────────────────────────────────────────────────────────────────
class EmptyChatGreeting : public QWidget {
    Q_OBJECT
public:
    explicit EmptyChatGreeting(QWidget* parent = nullptr);

signals:
    void greetingClicked(const QString& emoji);

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    QMovie* movie_ = nullptr;
    QLabel* emojiLabel_ = nullptr;
};
