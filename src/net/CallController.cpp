#include "net/CallController.h"
#include "net/CallEngine.h"
#include "net/ApiClient.h"
#include "net/WsClient.h"
#include "net/Session.h"
#include "ui/CallOverlay.h"

#include <QWidget>
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
// Веб шлёт offer/answer как JSON {"type","sdp"}, ICE как {"candidate","sdpMid",...}.
QString wrapSdp(const QString& type, const QString& sdp) {
    return QString::fromUtf8(QJsonDocument(QJsonObject{
        {QStringLiteral("type"), type}, {QStringLiteral("sdp"), sdp}}).toJson(QJsonDocument::Compact));
}
QString wrapCandidate(const QString& cand) {
    return QString::fromUtf8(QJsonDocument(QJsonObject{
        {QStringLiteral("candidate"), cand}, {QStringLiteral("sdpMid"), QStringLiteral("0")},
        {QStringLiteral("sdpMLineIndex"), 0}}).toJson(QJsonDocument::Compact));
}
QString unwrapSdp(const QString& payload) {
    QString s = payload.trimmed();
    QJsonParseError e{};
    QJsonDocument d = QJsonDocument::fromJson(s.toUtf8(), &e);
    if (e.error == QJsonParseError::NoError && d.isObject()) {
        const QString sdp = d.object().value(QStringLiteral("sdp")).toString();
        if (!sdp.isEmpty()) return sdp;
    }
    // возможно base64
    if (!s.contains('\n') && !s.contains(' ')) {
        const QByteArray dec = QByteArray::fromBase64(s.toUtf8());
        QJsonDocument d2 = QJsonDocument::fromJson(dec);
        if (d2.isObject()) {
            const QString sdp = d2.object().value(QStringLiteral("sdp")).toString();
            if (!sdp.isEmpty()) return sdp;
        }
        const QString ds = QString::fromUtf8(dec);
        if (ds.contains(QStringLiteral("v=0"))) return ds;
    }
    return s;   // уже сырой SDP
}
// → {candidate, sdpMid}
QPair<QString, QString> unwrapCandidate(const QString& payload) {
    QString s = payload.trimmed();
    QJsonDocument d = QJsonDocument::fromJson(s.toUtf8());
    if (d.isObject()) {
        const QJsonObject o = d.object();
        return { o.value(QStringLiteral("candidate")).toString(),
                 o.value(QStringLiteral("sdpMid")).toString(QStringLiteral("0")) };
    }
    if (!s.contains(' ') && !s.startsWith(QStringLiteral("candidate"))) {
        const QByteArray dec = QByteArray::fromBase64(s.toUtf8());
        QJsonDocument d2 = QJsonDocument::fromJson(dec);
        if (d2.isObject()) {
            const QJsonObject o = d2.object();
            return { o.value(QStringLiteral("candidate")).toString(),
                     o.value(QStringLiteral("sdpMid")).toString(QStringLiteral("0")) };
        }
    }
    return { s, QStringLiteral("0") };
}
}

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
        qInfo() << "[call] turn-config servers:" << servers;
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
    connect(e, &CallEngine::localOffer, this, [this](const QString& sdp) { api_->callOffer(peerId_, wrapSdp(QStringLiteral("offer"), sdp)); });
    connect(e, &CallEngine::localAnswer, this, [this](const QString& sdp) { api_->callAnswer(peerId_, wrapSdp(QStringLiteral("answer"), sdp)); });
    connect(e, &CallEngine::localCandidate, this, [this](const QString& cand, const QString&) { api_->callIce(peerId_, wrapCandidate(cand)); });
    connect(e, &CallEngine::connected, this, [this]() {
        connected_ = true;
        if (overlay_) { overlay_->setStatus(QStringLiteral("00:00")); overlay_->startCallTimer(); }
    });
    connect(e, &CallEngine::ended, this, [this]() { qInfo() << "[call] engine ended"; cleanup(); });
    connect(e, &CallEngine::failed, this, [this](const QString& r) { qWarning() << "[call] engine FAILED:" << r; cleanup(); });
    return e;
}

void CallController::startOutgoing(const QString& peerId, const QString& peerName, const QString& avatarUrl) {
    if (busy() || peerId.isEmpty()) return;
    if (peerId == Session::instance().userId) return;
    peerId_ = peerId; peerName_ = peerName; avatarUrl_ = avatarUrl; caller_ = true;
    answerApplied_ = false; offerFetched_ = false; addedCandidates_.clear(); connected_ = false;

    overlay_ = new CallOverlay(window_);
    overlay_->setPeer(peerName, avatarUrl);
    overlay_->setIncoming(false);
    overlay_->setStatus(QStringLiteral("Вызов…"));
    connect(overlay_, &CallOverlay::hangup, this, [this]() {
        if (!connected_) sendCallEvent(QStringLiteral("cancelled"));   // лог «Звонок отменён»
        api_->callEnd(peerId_); cleanup();
    });
    connect(overlay_, &CallOverlay::muteToggled, this, [this](bool m) { if (engine_) engine_->setMuted(m); });
    overlay_->show();
    overlay_->raise();

    qInfo() << "[call] outgoing to" << peerId_;
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
    qInfo() << "[call] incoming from" << callerId << callerName;

    peerId_ = callerId; peerName_ = callerName.isEmpty() ? callerId : callerName; caller_ = false;
    answerApplied_ = false; offerFetched_ = false; addedCandidates_.clear();

    overlay_ = new CallOverlay(window_);
    overlay_->setPeer(peerName_, QString());
    overlay_->setIncoming(true);
    overlay_->setStatus(QStringLiteral("Входящий звонок…"));
    connect(overlay_, &CallOverlay::accept, this, [this]() { acceptIncoming(); });
    connect(overlay_, &CallOverlay::decline, this, [this]() {
        sendCallEvent(QStringLiteral("rejected"));
        api_->callEnd(peerId_); cleanup();
    });
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

void CallController::applyOffer(const QString& callerId, const QString& payload) {
    if (caller_ || engine_ || callerId != peerId_ || payload.isEmpty()) return;
    const QString sdp = unwrapSdp(payload);
    qInfo() << "[call] got remote offer, sdp len" << sdp.size();
    offerFetched_ = true;
    engine_ = createEngine();
    onIce_ = [this, sdp](const QStringList& servers) {
        if (engine_) { engine_->setIceServers(servers); engine_->startAsCallee(sdp); }
    };
    api_->getTurnConfig();
}

void CallController::applyAnswer(const QString& calleeId, const QString& payload) {
    if (!engine_ || answerApplied_ || calleeId != peerId_ || payload.isEmpty()) return;
    const QString sdp = unwrapSdp(payload);
    qInfo() << "[call] got remote answer, sdp len" << sdp.size();
    answerApplied_ = true;
    engine_->setRemoteAnswer(sdp);
}

void CallController::addCandidates(const QString& otherId, const QStringList& cands) {
    if (!engine_ || otherId != peerId_) return;
    for (const QString& c : cands) {
        if (c.isEmpty() || addedCandidates_.contains(c)) continue;
        addedCandidates_.insert(c);
        const QPair<QString, QString> cm = unwrapCandidate(c);
        if (!cm.first.isEmpty()) engine_->addRemoteCandidate(cm.first, cm.second);
    }
}

void CallController::sendCallEvent(const QString& status) {
    if (peerId_.isEmpty()) return;
    const qint64 ts = QDateTime::currentMSecsSinceEpoch();
    const QString content = QStringLiteral("[[XIPHER_CALL_EVENT]]{\"status\":\"%1\",\"ts\":%2,\"v\":1}")
                                .arg(status).arg(ts);
    api_->sendRaw(peerId_, content, QStringLiteral("text"),
                  QStringLiteral("ce_%1").arg(ts));
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
