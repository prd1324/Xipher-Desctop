#include "ui/AvatarUtil.h"
#include "net/Session.h"

#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

namespace {
QNetworkAccessManager& nam() {
    static QNetworkAccessManager m;
    return m;
}
QHash<QString, QPixmap>& cache() {
    static QHash<QString, QPixmap> c;
    return c;
}
const QString kBase = QStringLiteral("https://messenger.xipher.pro");

QPixmap roundFromImage(const QPixmap& src, int size) {
    const qreal dpr = 2.0;
    QPixmap out(int(size * dpr), int(size * dpr));
    out.setDevicePixelRatio(dpr);
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath clip;
    clip.addEllipse(0, 0, size, size);
    p.setClipPath(clip);
    QPixmap scaled = src.scaled(int(size * dpr), int(size * dpr),
                                Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    p.drawPixmap(0, 0, size, size, scaled);
    return out;
}
}

namespace Avatar {

QPixmap roundLetter(const QString& text, int size) {
    const qreal dpr = 2.0;
    QPixmap pm(int(size * dpr), int(size * dpr));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QLinearGradient g(0, 0, size, size);
    g.setColorAt(0, QColor(0x8B, 0x5C, 0xF6));
    g.setColorAt(1, QColor(0x6D, 0x28, 0xD9));
    p.setPen(Qt::NoPen);
    p.setBrush(g);
    p.drawEllipse(0, 0, size, size);

    QString letter = text.trimmed().left(1).toUpper();
    if (letter.isEmpty()) letter = QStringLiteral("?");
    QFont f = p.font();
    f.setPixelSize(int(size * 0.42));
    f.setBold(true);
    p.setFont(f);
    p.setPen(QColor(0xFF, 0xFF, 0xFF));
    p.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, letter);
    return pm;
}

void setRound(QLabel* label, const QString& url, const QString& fallbackText, int size) {
    if (!label) return;
    label->setFixedSize(size, size);
    label->setPixmap(roundLetter(fallbackText, size));   // плейсхолдер сразу

    if (url.isEmpty()) return;

    if (cache().contains(url)) {
        label->setPixmap(cache().value(url));
        return;
    }

    QString full = url.startsWith(QStringLiteral("http")) ? url : (kBase + url);
    QNetworkRequest req((QUrl(full)));
    req.setRawHeader("Authorization", "Bearer " + Session::instance().token.toUtf8());

    QPointer<QLabel> guard(label);
    const QString key = url;
    QNetworkReply* reply = nam().get(req);
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, guard, key, size]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        QPixmap src;
        if (!src.loadFromData(reply->readAll())) return;
        QPixmap round = roundFromImage(src, size);
        cache().insert(key, round);
        if (guard) guard->setPixmap(round);
    });
}

} // namespace Avatar
