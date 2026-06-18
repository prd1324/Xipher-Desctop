#pragma once
#include "ui/ModalOverlay.h"
#include "net/Models.h"

#include <QList>

class QLineEdit;
class QVBoxLayout;

// ─────────────────────────────────────────────────────────────────────────────
//  ChatPickerDialog — выбор чата (для пересылки сообщения). Список chats_ с
//  поиском; по клику эмитит picked(Chat) и закрывается.
// ─────────────────────────────────────────────────────────────────────────────
class ChatPickerDialog : public ModalOverlay {
    Q_OBJECT
public:
    ChatPickerDialog(const QList<Chat>& chats, const QString& title, QWidget* parent);

signals:
    void picked(const Chat& chat);

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    void rebuild(const QString& filter);
    QList<Chat>   chats_;
    QVBoxLayout*  listBox_ = nullptr;
};
