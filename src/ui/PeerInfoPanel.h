#pragma once
#include "ui/ModalOverlay.h"
#include "net/Models.h"

#include <QList>

class ApiClient;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QVBoxLayout;
class QJsonObject;

// ─────────────────────────────────────────────────────────────────────────────
//  PeerInfoPanel — экран канала/группы как в Telegram: инфо (аватар, имя, @,
//  участники, описание), действия участника (подписаться/выйти) и блок
//  управления для создателя/админа (правка имени/описания/@, участники,
//  ссылка-приглашение, удаление).
// ─────────────────────────────────────────────────────────────────────────────
class PeerInfoPanel : public ModalOverlay {
    Q_OBJECT
public:
    PeerInfoPanel(ApiClient* api, const QString& peerId, bool isChannel,
                  const QString& name, const QString& avatarUrl, QWidget* parent);

signals:
    void changed();   // что-то изменилось → ChatPage перезагрузит списки
    void leftPeer(const QString& peerId);

private:
    void rebuild();
    void onChannelInfo(const QJsonObject& obj);
    void onMembers(const QString& peerId, const QList<Member>& members);

    ApiClient*      api_;
    QString         peerId_;
    bool            isChannel_;
    QString         name_;
    QString         avatarUrl_;
    QString         description_;
    QString         customLink_;
    int             count_ = 0;
    bool            canManage_ = false;
    bool            isCreator_ = false;
    bool            subscribed_ = true;
    QList<Member>   members_;

    QVBoxLayout*    body_ = nullptr;
};
