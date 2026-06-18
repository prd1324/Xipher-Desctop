#pragma once
#include "ui/ModalOverlay.h"
#include "net/Models.h"

#include <QList>

class ApiClient;
class QLineEdit;
class QVBoxLayout;
class QTimer;
class QLabel;

// ─────────────────────────────────────────────────────────────────────────────
//  ContactsPanel — встроенная панель контактов в стиле Telegram (оверлей внутри
//  окна, не отдельное окно). Показывает входящие заявки, список друзей и поиск
//  людей с добавлением. Клик по контакту открывает чат.
// ─────────────────────────────────────────────────────────────────────────────
class ContactsPanel : public ModalOverlay {
    Q_OBJECT
public:
    ContactsPanel(ApiClient* api, QWidget* parent);

signals:
    void openChatRequested(const QString& userId, const QString& displayName, const QString& username);
    void friendsChanged();

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    void render();
    void addSectionLabel(const QString& text);
    void addPersonRow(const UserHit& u, bool searchMode);
    void addRequestRow(const FriendRequest& r);

    ApiClient*           api_;
    QLineEdit*           search_  = nullptr;
    QVBoxLayout*         listBox_ = nullptr;
    QLabel*              status_  = nullptr;
    QTimer*              debounce_ = nullptr;
    QList<UserHit>       friends_;
    QList<FriendRequest> requests_;
    QList<UserHit>       results_;
    QString              query_;
};
