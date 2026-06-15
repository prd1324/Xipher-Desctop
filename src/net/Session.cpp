#include "net/Session.h"
#include <QSettings>

namespace {
QSettings settings() {
    // Реестр Windows: HKCU\Software\Xipher\Desktop
    return QSettings(QStringLiteral("Xipher"), QStringLiteral("Desktop"));
}
}

Session& Session::instance() {
    static Session s;
    return s;
}

void Session::clear() {
    token.clear();
    userId.clear();
    username.clear();
    isPremium = false;
    QSettings st = settings();
    st.remove(QStringLiteral("token"));
    st.remove(QStringLiteral("user_id"));
    st.remove(QStringLiteral("username"));
    st.remove(QStringLiteral("is_premium"));
}

void Session::save() const {
    QSettings st = settings();
    st.setValue(QStringLiteral("token"), token);
    st.setValue(QStringLiteral("user_id"), userId);
    st.setValue(QStringLiteral("username"), username);
    st.setValue(QStringLiteral("is_premium"), isPremium);
}

void Session::load() {
    QSettings st = settings();
    token    = st.value(QStringLiteral("token")).toString();
    userId   = st.value(QStringLiteral("user_id")).toString();
    username = st.value(QStringLiteral("username")).toString();
    isPremium = st.value(QStringLiteral("is_premium"), false).toBool();
}
