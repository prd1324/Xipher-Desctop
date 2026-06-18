#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <memory>

namespace rtc { class PeerConnection; class Track; class RtpPacketizationConfig; }
class QAudioSource;
class QAudioSink;
class QIODevice;
struct OpusEncoder;
struct OpusDecoder;

// ─────────────────────────────────────────────────────────────────────────────
//  CallEngine — голосовой звонок поверх WebRTC (libdatachannel) + Opus/Qt audio.
//  Совместим с браузерным пиром (веб-клиент): SDP offer/answer + trickle ICE.
// ─────────────────────────────────────────────────────────────────────────────
class CallEngine : public QObject {
    Q_OBJECT
public:
    explicit CallEngine(QObject* parent = nullptr);
    ~CallEngine() override;

    void setIceServers(const QStringList& servers);   // "stun:...", "turn:user:pass@host"
    void startAsCaller();                              // создаёт offer
    void startAsCallee(const QString& remoteOffer);    // принимает offer, создаёт answer
    void setRemoteAnswer(const QString& sdp);
    void addRemoteCandidate(const QString& cand, const QString& mid);
    void hangup();
    void setMuted(bool muted);

signals:
    void localOffer(const QString& sdp);
    void localAnswer(const QString& sdp);
    void localCandidate(const QString& candidate, const QString& mid);
    void connected();
    void ended();
    void failed(const QString& reason);

private:
    void createPeerConnection();
    void setupAudioTrack();
    void startAudioIo();
    void stopAudioIo();
    void onEncodedFrameReady();
    void onIncomingRtp(const QByteArray& rtp);

    std::shared_ptr<rtc::PeerConnection> pc_;
    std::shared_ptr<rtc::Track> track_;
    std::shared_ptr<rtc::RtpPacketizationConfig> rtpConfig_;

    QStringList iceServers_;
    bool caller_ = false;
    bool muted_ = false;
    bool audioStarted_ = false;

    // Audio (48 kHz mono, 20 ms кадры)
    QAudioSource* mic_ = nullptr;
    QIODevice*    micIo_ = nullptr;
    QAudioSink*   spk_ = nullptr;
    QIODevice*    spkIo_ = nullptr;
    QByteArray    capBuf_;
    OpusEncoder*  enc_ = nullptr;
    OpusDecoder*  dec_ = nullptr;
};
