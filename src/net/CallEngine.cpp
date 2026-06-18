#include "net/CallEngine.h"

#include <rtc/rtc.hpp>
#include <QDebug>

CallEngine::CallEngine(QObject* parent) : QObject(parent) {}
CallEngine::~CallEngine() = default;

bool CallEngine::selfTest() {
    try {
        rtc::Configuration config;
        config.iceServers.emplace_back("stun:stun.l.google.com:19302");
        auto pc = std::make_shared<rtc::PeerConnection>(config);
        pc->onStateChange([](rtc::PeerConnection::State) {});
        pc.reset();
        return true;
    } catch (const std::exception& e) {
        qWarning() << "CallEngine selfTest failed:" << e.what();
        return false;
    }
}
