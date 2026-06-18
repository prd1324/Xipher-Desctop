#include "net/CallController.h"
#include "net/CallEngine.h"
#include "net/ApiClient.h"
#include "net/WsClient.h"
#include "ui/CallOverlay.h"

#include <QWidget>

CallController::CallController(ApiClient* api, WsClient* ws, QWidget* window, QObject* parent)
    : QObject(parent), api_(api), ws_(ws), window_(window) {

    // ICE-серверы пришли → запускаем отложенное действие (offer/answer).
    connect(api_, &ApiClient::turnConfigReady, this, [this](const QStringList& servers) {
        if (onIce_) { auto fn = onIce_; onIce_ = nullptr; fn(servers); }
    });

    // Сигналинг от собеседника.
    connect(ws_, &WsClient::callAnswerReceived, this, [this](const QString& from, const QString& sdp) {
        if (engine_ && from == peerId_) engine_->setRemoteAnswer(sdp);
    });
    connect(ws_, &WsClient::callIceReceived, this, [this](const QString& from, const QString& cand) {
        if (engine_ && from == peerId_) engine_->addRemoteCandidate(cand, QStringLiteral("0"));
    });
    connect(ws_, &WsClient::callEnded, this, [this](const QString& from) {
        if (from == peerId_) cleanup();
    });

    connect(api_, &ApiClient::callOfferReady, this, [this](const QString& callerId, const QString& sdp) {
        if (callerId != peerId_ || caller_) return;
        onIce_ = [this, sdp](const QStringList& servers) {
            if (!engine_) return;
            engine_->setIceServers(servers);
            engine_->startAsCallee(sdp);
        };
        api_->getTurnConfig();
    });
}

CallEngine* CallController::createEngine() {
    auto* e = new CallEngine(this);
    connect(e, &CallEngine::localOffer, this, [this](const QString& sdp) {
        api_->callOffer(peerId_, sdp);
    });
    connect(e, &CallEngine::localAnswer, this, [this](const QString& sdp) {
        api_->callAnswer(peerId_, sdp);
    });
    connect(e, &CallEngine::localCandidate, this, [this](const QString& cand, const QString&) {
        api_->callIce(peerId_, cand);
    });
    connect(e, &CallEngine::connected, this, [this]() {
        if (overlay_) { overlay_->setStatus(QStringLiteral("00:00")); overlay_->startCallTimer(); }
    });
    connect(e, &CallEngine::ended, this, [this]() { cleanup(); });
    connect(e, &CallEngine::failed, this, [this](const QString&) { cleanup(); });
    return e;
}

void CallController::startOutgoing(const QString& peerId, const QString& peerName, const QString& avatarUrl) {
    if (busy() || peerId.isEmpty()) return;
    peerId_ = peerId; peerName_ = peerName; avatarUrl_ = avatarUrl; caller_ = true;

    overlay_ = new CallOverlay(window_);
    overlay_->setPeer(peerName, avatarUrl);
    overlay_->setIncoming(false);
    overlay_->setStatus(QStringLiteral("Вызов…"));
    connect(overlay_, &CallOverlay::hangup, this, [this]() { api_->callEnd(peerId_); cleanup(); });
    connect(overlay_, &CallOverlay::muteToggled, this, [this](bool m) { if (engine_) engine_->setMuted(m); });
    overlay_->show();
    overlay_->raise();

    engine_ = createEngine();
    api_->callNotify(peerId_, QStringLiteral("audio"));   // звоним (рингтон у получателя)
    onIce_ = [this](const QStringList& servers) {
        if (!engine_) return;
        engine_->setIceServers(servers);
        engine_->startAsCaller();   // → localOffer → callOffer
    };
    api_->getTurnConfig();
}

void CallController::onIncoming(const QString& callerId, const QString& callerName, const QString&) {
    if (callerId.isEmpty()) return;
    if (peerId_ == callerId) return;            // уже обрабатываем этот звонок
    if (busy()) { api_->callEnd(callerId); return; }   // заняты — отклоняем

    peerId_ = callerId; peerName_ = callerName.isEmpty() ? callerId : callerName; caller_ = false;

    overlay_ = new CallOverlay(window_);
    overlay_->setPeer(peerName_, QString());
    overlay_->setIncoming(true);
    overlay_->setStatus(QStringLiteral("Входящий звонок…"));
    connect(overlay_, &CallOverlay::accept, this, [this]() { acceptIncoming(); });
    connect(overlay_, &CallOverlay::decline, this, [this]() { api_->callEnd(peerId_); cleanup(); });
    connect(overlay_, &CallOverlay::hangup, this, [this]() { api_->callEnd(peerId_); cleanup(); });
    connect(overlay_, &CallOverlay::muteToggled, this, [this](bool m) { if (engine_) engine_->setMuted(m); });
    overlay_->show();
    overlay_->raise();
}

void CallController::acceptIncoming() {
    if (!overlay_) return;
    overlay_->setIncoming(false);
    overlay_->setStatus(QStringLiteral("Соединение…"));
    engine_ = createEngine();
    api_->getCallOffer(peerId_);   // → callOfferReady → getTurnConfig → startAsCallee
}

void CallController::cleanup() {
    if (engine_) { engine_->hangup(); engine_->deleteLater(); engine_ = nullptr; }
    if (overlay_) { overlay_->hide(); overlay_->deleteLater(); overlay_ = nullptr; }
    peerId_.clear(); peerName_.clear(); avatarUrl_.clear();
    onIce_ = nullptr;
}
