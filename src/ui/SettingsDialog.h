#pragma once
#include "ui/ModalOverlay.h"
#include "net/Models.h"

#include <QList>
#include <QColor>

class ApiClient;
class QListWidget;
class QStackedWidget;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class QVBoxLayout;
class QJsonObject;

// ─────────────────────────────────────────────────────────────────────────────
//  SettingsDialog — центральная модалка настроек со split-навигацией (как веб):
//  слева список разделов, справа их содержимое. Аккаунт/приватность/сеансы/email
//  ходят на сервер; уведомления/звонки/язык/блокировки — локально (Prefs).
// ─────────────────────────────────────────────────────────────────────────────
class SettingsDialog : public ModalOverlay {
    Q_OBJECT
public:
    SettingsDialog(ApiClient* api, QWidget* parent);

private:
    void buildChrome();
    // Добавляет sub-страницу в стек и строку-раздел на главный экран (Telegram-style).
    void addSection(QVBoxLayout* list, const QString& title, int iconKind,
                    const QColor& iconColor, QWidget* page);
    QWidget* wrapSubPage(const QString& title, QWidget* content);   // header «‹ назад» + scroll
    void pickAvatar();

    QWidget* buildAccountPage();
    QWidget* buildNotificationsPage();
    QWidget* buildPrivacyPage();
    QWidget* buildCallsPage();
    QWidget* buildSessionsPage();
    QWidget* buildBlockedPage();
    QWidget* buildLanguagePage();
    QWidget* buildEmailPage();
    QWidget* buildPremiumPage();
    QWidget* buildAboutPage();

    void onProfileLoaded(const QJsonObject& obj);
    void refreshSessions();

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

private:

    ApiClient*      api_;
    QStackedWidget* stack_ = nullptr;

    // Аккаунт.
    QLabel*         avatar_    = nullptr;
    QLabel*         heroName_  = nullptr;
    QLabel*         heroUser_  = nullptr;
    QLineEdit*      firstName_ = nullptr;
    QLineEdit*      lastName_  = nullptr;
    QPlainTextEdit* bio_       = nullptr;
    QSpinBox*       bDay_      = nullptr;
    QSpinBox*       bMonth_    = nullptr;
    QSpinBox*       bYear_     = nullptr;
    QLineEdit*      usernameRO_= nullptr;
    QString         avatarUrl_;

    // Email.
    QLineEdit*      email_     = nullptr;

    // Сеансы.
    QVBoxLayout*    sessionsBox_ = nullptr;

    // Privacy controls собираются в лямбдах; сохранение через updateMyPrivacy.
};
