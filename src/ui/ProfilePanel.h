#pragma once
#include <QWidget>
#include <QJsonObject>

class ApiClient;
class QLabel;
class QVBoxLayout;

// ─────────────────────────────────────────────────────────────────────────────
//  ProfilePanel — профиль пользователя как в Telegram: выезжает справа поверх
//  окна, затемняет фон, показывает аватар, имя, статус, био и инфо-строки.
// ─────────────────────────────────────────────────────────────────────────────
class ProfilePanel : public QWidget {
    Q_OBJECT
public:
    ProfilePanel(ApiClient* api, QWidget* parent);

    void openFor(const QString& userId);

signals:
    void messageRequested(const QString& userId);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    void closePanel();
    void layoutPanel(bool animate);
    void onProfileLoaded(const QJsonObject& obj);
    void addInfoRow(const QString& value, const QString& caption);

    ApiClient*   api_;
    QWidget*     panel_;
    QLabel*      avatar_;
    QLabel*      name_;
    QLabel*      status_;
    QVBoxLayout* infoBox_;
    QString      userId_;
    int          panelW_ = 360;

    qreal dim_ = 0.0;
    Q_PROPERTY(qreal dim READ dim WRITE setDim)
public:
    qreal dim() const { return dim_; }
    void  setDim(qreal d) { dim_ = d; update(); }
};
