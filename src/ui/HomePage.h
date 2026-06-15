#pragma once
#include <QWidget>

class QLabel;

// Мини-экран после входа (без дизайна) — заглушка под будущий мессенджер.
// Показывает, что вход прошёл: имя, user_id и кнопку выхода.
class HomePage : public QWidget {
    Q_OBJECT
public:
    explicit HomePage(QWidget* parent = nullptr);

    // Обновить данные из текущей Session.
    void refresh();

signals:
    void logoutRequested();

private:
    QLabel* who_ = nullptr;
    QLabel* uid_ = nullptr;
};
