#pragma once
#include <QObject>
#include <QString>
#include <memory>

namespace rtc { class PeerConnection; }

// ─────────────────────────────────────────────────────────────────────────────
//  CallEngine — нативный WebRTC-пир (libdatachannel) для звонков.
//  Этап 1: создание PeerConnection, генерация SDP offer/answer, обмен ICE.
//  Аудио (Opus capture/playback) и привязка к UI — следующими шагами.
// ─────────────────────────────────────────────────────────────────────────────
class CallEngine : public QObject {
    Q_OBJECT
public:
    explicit CallEngine(QObject* parent = nullptr);
    ~CallEngine() override;

    // Проверка интеграции: создать/уничтожить PeerConnection. true — WebRTC жив.
    bool selfTest();

signals:
    void localDescription(const QString& sdp, const QString& type);
    void localCandidate(const QString& candidate, const QString& mid);
    void stateChanged(const QString& state);

private:
    std::shared_ptr<rtc::PeerConnection> pc_;
};
