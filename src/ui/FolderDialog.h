#pragma once
#include "ui/ModalOverlay.h"
#include "net/Models.h"

#include <QList>
#include <QSet>

class QLineEdit;
class QVBoxLayout;

// ─────────────────────────────────────────────────────────────────────────────
//  FolderEditorDialog — создание/редактирование папки в стиле Telegram:
//  имя + список чатов с галочками (поиск). Сохранение отдаёт Folder наружу
//  (ChatPage синхронизирует с сервером).
// ─────────────────────────────────────────────────────────────────────────────
class FolderEditorDialog : public ModalOverlay {
    Q_OBJECT
public:
    FolderEditorDialog(const QList<Chat>& chats, const Folder& existing, QWidget* parent);

signals:
    void saved(const Folder& folder);
    void removed(const QString& folderId);

private:
    void rebuildList(const QString& filter);

    QList<Chat>   chats_;
    Folder        folder_;
    bool          isNew_;
    QSet<QString> selected_;   // выбранные chatKey
    QLineEdit*    nameEdit_  = nullptr;
    QVBoxLayout*  listBox_   = nullptr;
};
