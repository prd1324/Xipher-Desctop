#pragma once
#include "ui/ModalOverlay.h"
#include "net/Models.h"

class ApiClient;
class QLineEdit;
class QPlainTextEdit;
class QLabel;
class QVBoxLayout;

// ─────────────────────────────────────────────────────────────────────────────
//  Диалоги групп/каналов в стиле Telegram: создание группы, создание канала и
//  каталог публичных каналов/групп с поиском и вступлением.
// ─────────────────────────────────────────────────────────────────────────────

class CreateGroupDialog : public ModalOverlay {
    Q_OBJECT
public:
    CreateGroupDialog(ApiClient* api, QWidget* parent);
signals:
    void created();
private:
    ApiClient* api_;
};

class CreateChannelDialog : public ModalOverlay {
    Q_OBJECT
public:
    CreateChannelDialog(ApiClient* api, QWidget* parent);
signals:
    void created();
private:
    ApiClient* api_;
};

class CatalogDialog : public ModalOverlay {
    Q_OBJECT
public:
    CatalogDialog(ApiClient* api, QWidget* parent);
signals:
    void joined();
private:
    void reload(const QString& query);
    void renderItems(const QList<DirectoryItem>& items);
    ApiClient*   api_;
    QLineEdit*   search_ = nullptr;
    QVBoxLayout* list_   = nullptr;
};
