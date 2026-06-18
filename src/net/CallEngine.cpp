#include "net/CallEngine.h"

#include <rtc/rtc.hpp>
#include <opus.h>

#include <QAudioSource>
#include <QAudioSink>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QIODevice>
#include <QDebug>

namespace {
constexpr int kSampleRate = 48000;
constexpr int kChannels   = 1;
constexpr int kFrame      = 960;             // 20 мс при 48 кГц
constexpr int kFrameBytes = kFrame * 2;      // int16 mono
constexpr int kPayloadType = 111;            // Opus

QAudioFormat pcmFormat() {
    QAudioFormat f;
    f.setSampleRate(kSampleRate);
    f.setChannelCount(kChannels);
    f.setSampleFormat(QAudioFormat::Int16);
    return f;
}
}

CallEngine::CallEngine(QObject* parent) : QObject(parent) {
    int err = 0;
    enc_ = opus_encoder_create(kSampleRate, kChannels, OPUS_APPLICATION_VOIP, &err);
    if (enc_) opus_encoder_ctl(enc_, OPUS_SET_BITRATE(24000));
    dec_ = opus_decoder_create(kSampleRate, kChannels, &err);
}

CallEngine::~CallEngine() {
    hangup();
    if (enc_) opus_encoder_destroy(enc_);
    if (dec_) opus_decoder_destroy(dec_);
}

void CallEngine::setIceServers(const QStringList& servers) { iceServers_ = servers; }

void CallEngine::createPeerConnection() {
    rtc::Configuration config;
    for (const QString& s : iceServers_) {
        try { config.iceServers.emplace_back(s.toStdString()); } catch (...) {}
    }
    if (config.iceServers.empty())
        config.iceServers.emplace_back("stun:stun.l.google.com:19302");

    pc_ = std::make_shared<rtc::PeerConnection>(config);

    pc_->onLocalDescription([this](rtc::Description desc) {
        const QString sdp = QString::fromStdString(std::string(desc));
        if (desc.type() == rtc::Description::Type::Offer) emit localOffer(sdp);
        else if (desc.type() == rtc::Description::Type::Answer) emit localAnswer(sdp);
    });
    pc_->onLocalCandidate([this](rtc::Candidate cand) {
        emit localCandidate(QString::fromStdString(std::string(cand)),
                            QString::fromStdString(cand.mid()));
    });
    pc_->onStateChange([this](rtc::PeerConnection::State state) {
        if (state == rtc::PeerConnection::State::Connected) {
            QMetaObject::invokeMethod(this, [this]() { startAudioIo(); emit connected(); });
        } else if (state == rtc::PeerConnection::State::Disconnected ||
                   state == rtc::PeerConnection::State::Closed) {
            QMetaObject::invokeMethod(this, [this]() { emit ended(); });
        } else if (state == rtc::PeerConnection::State::Failed) {
            QMetaObject::invokeMethod(this, [this]() { emit failed(QStringLiteral("ICE failed")); });
        }
    });
}

void CallEngine::setupAudioTrack() {
    rtc::Description::Audio media("audio", rtc::Description::Direction::SendRecv);
    media.addOpusCodec(kPayloadType);
    media.setBitrate(24);

    track_ = pc_->addTrack(media);

    rtpConfig_ = std::make_shared<rtc::RtpPacketizationConfig>(
        12345678u, "xipher", kPayloadType, 48000u);
    auto packetizer = std::make_shared<rtc::OpusRtpPacketizer>(rtpConfig_);
    auto srReporter = std::make_shared<rtc::RtcpSrReporter>(rtpConfig_);
    packetizer->addToChain(srReporter);
    auto nack = std::make_shared<rtc::RtcpNackResponder>();
    packetizer->addToChain(nack);
    track_->setMediaHandler(packetizer);

    // Приём RTP: парсим заголовок и декодируем Opus.
    track_->onMessage([this](rtc::message_variant data) {
        if (!std::holds_alternative<rtc::binary>(data)) return;
        const auto& bin = std::get<rtc::binary>(data);
        QByteArray pkt(reinterpret_cast<const char*>(bin.data()), int(bin.size()));
        QMetaObject::invokeMethod(this, [this, pkt]() { onIncomingRtp(pkt); });
    }, nullptr);
}

void CallEngine::startAsCaller() {
    caller_ = true;
    createPeerConnection();
    setupAudioTrack();
    pc_->setLocalDescription();   // → onLocalDescription(offer)
}

void CallEngine::startAsCallee(const QString& remoteOffer) {
    caller_ = false;
    createPeerConnection();
    setupAudioTrack();
    try {
        pc_->setRemoteDescription(rtc::Description(remoteOffer.toStdString(), "offer"));
        pc_->setLocalDescription();   // → onLocalDescription(answer)
    } catch (const std::exception& e) {
        emit failed(QString::fromUtf8(e.what()));
    }
}

void CallEngine::setRemoteAnswer(const QString& sdp) {
    if (!pc_) return;
    try { pc_->setRemoteDescription(rtc::Description(sdp.toStdString(), "answer")); }
    catch (const std::exception& e) { emit failed(QString::fromUtf8(e.what())); }
}

void CallEngine::addRemoteCandidate(const QString& cand, const QString& mid) {
    if (!pc_) return;
    try { pc_->addRemoteCandidate(rtc::Candidate(cand.toStdString(), mid.toStdString())); }
    catch (...) {}
}

void CallEngine::setMuted(bool muted) { muted_ = muted; }

// ── Аудио ввод/вывод ──────────────────────────────────────────────────────────

void CallEngine::startAudioIo() {
    if (audioStarted_) return;
    audioStarted_ = true;

    const QAudioFormat fmt = pcmFormat();
    if (!QMediaDevices::defaultAudioInput().isNull()) {
        mic_ = new QAudioSource(QMediaDevices::defaultAudioInput(), fmt, this);
        micIo_ = mic_->start();
        if (micIo_) connect(micIo_, &QIODevice::readyRead, this, &CallEngine::onEncodedFrameReady);
    }
    if (!QMediaDevices::defaultAudioOutput().isNull()) {
        spk_ = new QAudioSink(QMediaDevices::defaultAudioOutput(), fmt, this);
        spkIo_ = spk_->start();
    }
}

void CallEngine::stopAudioIo() {
    audioStarted_ = false;
    if (mic_) { mic_->stop(); mic_->deleteLater(); mic_ = nullptr; micIo_ = nullptr; }
    if (spk_) { spk_->stop(); spk_->deleteLater(); spk_ = nullptr; spkIo_ = nullptr; }
    capBuf_.clear();
}

void CallEngine::onEncodedFrameReady() {
    if (!micIo_ || !enc_ || !track_) return;
    capBuf_.append(micIo_->readAll());
    while (capBuf_.size() >= kFrameBytes) {
        QByteArray frame = capBuf_.left(kFrameBytes);
        capBuf_.remove(0, kFrameBytes);
        if (muted_) continue;   // тишину не шлём

        unsigned char out[4000];
        const int n = opus_encode(enc_, reinterpret_cast<const opus_int16*>(frame.constData()),
                                  kFrame, out, sizeof(out));
        if (n <= 0) continue;
        rtpConfig_->timestamp += kFrame;
        try {
            track_->send(reinterpret_cast<const std::byte*>(out), size_t(n));
        } catch (...) {}
    }
}

void CallEngine::onIncomingRtp(const QByteArray& rtp) {
    if (!dec_ || !spkIo_ || rtp.size() < 13) return;
    // Минимальный разбор RTP: версия 2, payload после 12 + 4*CSRC байт.
    const auto* d = reinterpret_cast<const unsigned char*>(rtp.constData());
    const int csrc = d[0] & 0x0F;
    int offset = 12 + 4 * csrc;
    if ((d[0] & 0x10) && rtp.size() > offset + 4) {   // extension
        const int extLen = (d[offset + 2] << 8 | d[offset + 3]);
        offset += 4 + extLen * 4;
    }
    if (rtp.size() <= offset) return;

    opus_int16 pcm[kFrame * 2];
    const int samples = opus_decode(dec_,
        reinterpret_cast<const unsigned char*>(d + offset), rtp.size() - offset,
        pcm, kFrame * 2, 0);
    if (samples > 0)
        spkIo_->write(reinterpret_cast<const char*>(pcm), samples * 2);
}

void CallEngine::hangup() {
    stopAudioIo();
    track_.reset();
    rtpConfig_.reset();
    if (pc_) { try { pc_->close(); } catch (...) {} pc_.reset(); }
}
