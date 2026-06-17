#pragma once
#include <QFrame>
#include <QStringList>

class QScrollArea;
class QWidget;
class QGridLayout;

// ─────────────────────────────────────────────────────────────────────────────
//  EmojiPicker — панель эмодзи как в Telegram: вкладки категорий сверху,
//  большая прокручиваемая сетка эмодзи. Открывается всплывающим окном (Qt::Popup),
//  клик по эмодзи вставляет его в поле ввода. Окно закрывается по клику вне.
// ─────────────────────────────────────────────────────────────────────────────
class EmojiPicker : public QFrame {
    Q_OBJECT
public:
    explicit EmojiPicker(QWidget* parent = nullptr);

signals:
    void emojiPicked(const QString& emoji);

private:
    void showCategory(int index);

    QScrollArea* scroll_ = nullptr;
    QWidget*     grid_    = nullptr;
    QList<QStringList> categories_;
    QList<class QPushButton*> tabs_;
    int current_ = 0;
};
