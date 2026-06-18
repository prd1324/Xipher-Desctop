#include "net/CallController.h"
#include "net/CallEngine.h"
#include "net/ApiClient.h"
#include "net/WsClient.h"
#include "net/Session.h"
#include "ui/CallOverlay.h"

#include <QWidget>
#include <QTimer>

CallController::CallController(ApiClient* api, WsClient* ws, QWidget* window, QObject* parent)
    : QObject(parent), api_(api), ws_(ws), window_(window) {

    poll_ = new QTimer(this);
    poll_->setInterval(900);   // опрос сигналинга (надёжнее, чем только WS)
    connect(poll_, &QTimer::timeout, this, [this]() {
        if (peerId_.isEmpty()) return;
        if (caller_) {
            if (engine_ && !answerApplied_) api_->getCallAnswer(peerId_);
            if (engine_) api_->getCallIce(peerId_);
        } else {
            if (!engine_ && !offerFetched_) api_->getCallOffer(peerId_);
            if (engine_) api_->getCallIce(peerId_);
        }
    });

    // ICE-серверы пришли → запускаем отложенное действие.
    connect(api_, &ApiClient::turnConfigReady, this, [this](const QStringList& servers) {
        if (onIce_) { auto fn = onIce_; onIce_ = nullptr; fn(servers); }
    });

    // Поллинг-результаты.
    connect(api_, &ApiClient::callOfferReady, this, [this](const QString& id, const QString& sdp) { applyOffer(id, sdp); });
    connect(api_, &ApiClient::callAnswerReady, this, [this](const QString& id, const QString& sdp) { applyAnswer(id, sdp); });
    connect(api_, &ApiClient::callIceBatch, this, [this](const QString& id, const QStringList& c) { addCandidates(id, c); });

    // WS (real-time, дублирует поллинг — дедуп ниже).
    connect(ws_, &WsClient::callAnswerReceived, this, [this](const QString& from, const QString& sdp) { applyAnswer(from, sdp); });
    connect(ws_, &WsClient::callIceReceived, this, [this](const QString& from, const QString& cand) { addCandidates(from, {cand}); });
    connect(ws_, &WsClient::callEnded, this, [this](const QString& from) { if (from == peerId_) cleanup(); });
}

CallEngine* CallController::createEngine() {
    auto* e = new CallEngine(this);
    connect(e, &CallEngine::localOffer, this, [this](const QString& sdp) { api_->callOffer(peerId_, sdp); });
    connect(e, &CallEngine::localAnswer, this, [this](const QString& sdp) { api_->callAnswer(peerId_, sdp); });
    connect(e, &CallEngine::localCandidate, this, [this](const QString& cand, const QString&) { api_->callIce(peerId_, cand); });
    connect(e, &CallEngine::connected, this, [this]() {
        if (overlay_) { overlay_->setStatus(QStringLiteral("00:00")); overlay_->startCallTimer(); }
    });
    connect(e, &CallEngine::ended, this, [this]() { cleanup(); });
    connect(e, &CallEngine::failed, this, [this](const QString&) { cleanup(); });
    return e;
}

void CallController::startOutgoing(const QString& peerId, const QString& peerName, const QString& avatarUrl) {
    if (busy() || peerId.isEmpty()) return;
    if (peerId == Session::instance().userId) return;
    peerId_ = peerId; peerName_ = peerName; avatarUrl_ = avatarUrl; caller_ = true;
    answerApplied_ = false; offerFetched_ = false; addedCandidates_.clear();

    overlay_ = new CallOverlay(window_);
    overlay_->setPeer(peerName, avatarUrl);
    overlay_->setIncoming(false);
    overlay_->setStatus(QStringLiteral("Вызов…"));
    connect(overlay_, &CallOverlay::hangup, this, [this]() { api_->callEnd(peerId_); cleanup(); });
    connect(overlay_, &CallOverlay::muteToggled, this, [this](bool m) { if (engine_) engine_->setMuted(m); });
    overlay_->show();
    overlay_->raise();

    engine_ = createEngine();
    api_->callNotify(peerId_, QStringLiteral("audio"));
    onIce_ = [this](const QStringList& servers) {
        if (engine_) { engine_->setIceServers(servers); engine_->startAsCaller(); }
    };
    api_->getTurnConfig();
    poll_->start();
}

void CallController::onIncoming(const QString& callerId, const QString& callerName, const QString&) {
    if (callerId.isEmpty() || peerId_ == callerId) return;
    if (busy()) { api_->callEnd(callerId); return; }

    peerId_ = callerId; peerName_ = callerName.isEmpty() ? callerId : callerName; caller_ = false;
    answerApplied_ = false; offerFetched_ = false; addedCandidates_.clear();

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
    poll_->start();              // начнём опрашивать offer
    api_->getCallOffer(peerId_);
}

void CallController::applyOffer(const QString& callerId, const QString& sdp) {
    if (caller_ || engine_ || callerId != peerId_ || sdp.isEmpty()) return;
    offerFetched_ = true;
    engine_ = createEngine();
    onIce_ = [this, sdp](const QStringList& servers) {
        if (engine_) { engine_->setIceServers(servers); engine_->startAsCallee(sdp); }
    };
    api_->getTurnConfig();
}

void CallController::applyAnswer(const QString& calleeId, const QString& sdp) {
    if (!engine_ || answerApplied_ || calleeId != peerId_ || sdp.isEmpty()) return;
    answerApplied_ = true;
    engine_->setRemoteAnswer(sdp);
}

void CallController::addCandidates(const QString& otherId, const QStringList& cands) {
    if (!engine_ || otherId != peerId_) return;
    for (const QString& c : cands) {
        if (c.isEmpty() || addedCandidates_.contains(c)) continue;
        addedCandidates_.insert(c);
        engine_->addRemoteCandidate(c, QStringLiteral("0"));
    }
}

void CallController::cleanup() {
    static bool inCleanup = false;
    if (inCleanup) return;
    inCleanup = true;
    if (poll_) poll_->stop();
    onIce_ = nullptr;
    answerApplied_ = false; offerFetched_ = false; addedCandidates_.clear();
    if (engine_) { CallEngine* e = engine_; engine_ = nullptr; e->hangup(); e->deleteLater(); }
    if (overlay_) { CallOverlay* o = overlay_; overlay_ = nullptr; o->hide(); o->deleteLater(); }
    peerId_.clear(); peerName_.clear(); avatarUrl_.clear();
    inCleanup = false;
}
