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
    void addSection(const QString& title, int iconKind, const QColor& iconColor, QWidget* page);

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

    ApiClient*      api_;
    QListWidget*    nav_   = nullptr;
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
