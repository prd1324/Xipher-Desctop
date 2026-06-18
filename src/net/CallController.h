#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>

class ApiClient;
class WsClient;
class CallEngine;
class CallOverlay;
class QWidget;

// ─────────────────────────────────────────────────────────────────────────────
//  CallController — оркестрация звонка: связывает сигналинг (ApiClient/WsClient),
//  WebRTC-движок (CallEngine) и экран звонка (CallOverlay).
// ─────────────────────────────────────────────────────────────────────────────
class CallController : public QObject {
    Q_OBJECT
public:
    CallController(ApiClient* api, WsClient* ws, QWidget* window, QObject* parent = nullptr);

    void startOutgoing(const QString& peerId, const QString& peerName, const QString& avatarUrl);
    void onIncoming(const QString& callerId, const QString& callerName, const QString& callType);
    bool busy() const { return !peerId_.isEmpty(); }

private:
    CallEngine* createEngine();
    void acceptIncoming();
    void cleanup();

    ApiClient*   api_;
    WsClient*    ws_;
    QWidget*     window_;
    CallEngine*  engine_ = nullptr;
    CallOverlay* overlay_ = nullptr;
    QString      peerId_, peerName_, avatarUrl_;
    bool         caller_ = false;
    std::function<void(const QStringList&)> onIce_;
};
