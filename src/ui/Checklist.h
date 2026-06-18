#pragma once
#include <QWidget>
#include <QJsonObject>
#include <QHash>

class QLabel;
class QCheckBox;
class QVBoxLayout;
class QLineEdit;

// Префиксы протокола чек-листа (как в web/js/chat.js).
namespace ChecklistProto {
inline const QString kPrefix       = QStringLiteral("[[XIPHER_CHECKLIST]]");
inline const QString kUpdatePrefix = QStringLiteral("[[XIPHER_CHECKLIST_UPDATE]]");
}

// ─────────────────────────────────────────────────────────────────────────────
//  ChecklistWidget — отрисовка чек-листа в баббле: заголовок, прогресс,
//  пункты с галочками. Отметка пункта → сигнал (ChatPage отправит update).
// ─────────────────────────────────────────────────────────────────────────────
class ChecklistWidget : public QWidget {
    Q_OBJECT
public:
    ChecklistWidget(const QJsonObject& payload, bool outgoing, bool canMark, QWidget* parent = nullptr);

    QString checklistId() const { return payload_.value(QStringLiteral("id")).toString(); }
    void applyUpdate(const QJsonObject& update);   // {updates:[{id,done}], added:[{...}]}

signals:
    void itemToggled(const QString& itemId, bool done);

private:
    void rebuild();
    void updateProgress();

    QJsonObject payload_;
    bool outgoing_;
    bool canMark_;
    QVBoxLayout* itemsBox_ = nullptr;
    QLabel* progress_ = nullptr;
    QHash<QString, QCheckBox*> boxes_;
    bool applying_ = false;
};

// ─────────────────────────────────────────────────────────────────────────────
//  ChecklistEditor — встраиваемый редактор создания чек-листа (для оверлея,
//  как в Telegram). Сигналы submitted(payload) / cancelled.
// ─────────────────────────────────────────────────────────────────────────────
class ChecklistEditor : public QWidget {
    Q_OBJECT
public:
    explicit ChecklistEditor(QWidget* parent = nullptr);

signals:
    void submitted(const QJsonObject& payload);
    void cancelled();

private:
    void addItemRow();

    QLineEdit* title_ = nullptr;
    QVBoxLayout* items_ = nullptr;
    QCheckBox* othersMark_ = nullptr;
    QCheckBox* othersAdd_ = nullptr;
};
