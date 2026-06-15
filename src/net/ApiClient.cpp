#include "net/ApiClient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

ApiClient::ApiClient(QObject* parent)
    : QObject(parent), nam_(new QNetworkAccessManager(this)) {}

void ApiClient::postJson(const QString& path,
                         const QJsonObject& body,
                         std::function<void(const QJsonObject&, bool, const QString&)> callback) {
    QNetworkRequest req(QUrl(base_ + path));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Accept", "application/json");

    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
    QNetworkReply* reply = nam_->post(req, payload);

    QObject::connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        reply->deleteLater();
        const QByteArray data = reply->readAll();
        const QString netErr = reply->error() == QNetworkReply::NoError
                                   ? QString()
                                   : reply->errorString();

        QJsonParseError pe{};
        const QJsonDocument doc = QJsonDocument::fromJson(data, &pe);
        if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
            // Нет валидного JSON — отдаём то, что есть, как сетевую/серверную ошибку.
            callback(QJsonObject{}, false,
                     netErr.isEmpty() ? QStringLiteral("Некорректный ответ сервера") : netErr);
            return;
        }
        callback(doc.object(), netErr.isEmpty(), netErr);
    });
}

AuthResult ApiClient::parseAuth(const QJsonObject& obj) {
    AuthResult r;
    r.success = obj.value(QStringLiteral("success")).toBool(false);
    r.message = obj.value(QStringLiteral("message")).toString();
    const QJsonObject data = obj.value(QStringLiteral("data")).toObject();
    r.token     = data.value(QStringLiteral("token")).toString();
    r.userId    = data.value(QStringLiteral("user_id")).toString();
    r.username  = data.value(QStringLiteral("username")).toString();
    r.isPremium = data.value(QStringLiteral("is_premium")).toString() == QStringLiteral("true");
    return r;
}

void ApiClient::login(const QString& username, const QString& password) {
    QJsonObject body{{QStringLiteral("username"), username},
                     {QStringLiteral("password"), password}};
    postJson(QStringLiteral("/api/login"), body,
             [this](const QJsonObject& obj, bool ok, const QString& netErr) {
        if (!ok && obj.isEmpty()) {
            AuthResult r;
            r.success = false;
            r.message = QStringLiteral("Ошибка соединения с сервером: ") + netErr;
            emit loginFinished(r);
            return;
        }
        emit loginFinished(parseAuth(obj));
    });
}

void ApiClient::registerUser(const QString& username, const QString& password) {
    QJsonObject body{{QStringLiteral("username"), username},
                     {QStringLiteral("password"), password}};
    postJson(QStringLiteral("/api/register"), body,
             [this](const QJsonObject& obj, bool ok, const QString& netErr) {
        if (!ok && obj.isEmpty()) {
            AuthResult r;
            r.success = false;
            r.message = QStringLiteral("Ошибка соединения с сервером: ") + netErr;
            emit registerFinished(r);
            return;
        }
        emit registerFinished(parseAuth(obj));
    });
}

void ApiClient::checkUsername(const QString& username) {
    QJsonObject body{{QStringLiteral("username"), username}};
    postJson(QStringLiteral("/api/check-username"), body,
             [this, username](const QJsonObject& obj, bool ok, const QString&) {
        if (!ok && obj.isEmpty()) {
            // Сетевую ошибку при проверке ника не показываем как «занят».
            return;
        }
        const bool available = obj.value(QStringLiteral("success")).toBool(false);
        const QString message = obj.value(QStringLiteral("message")).toString();
        emit usernameChecked(username, available, message);
    });
}

void ApiClient::validateToken(const QString& token) {
    QJsonObject body{{QStringLiteral("token"), token}};
    postJson(QStringLiteral("/api/validate-token"), body,
             [this](const QJsonObject& obj, bool ok, const QString& netErr) {
        if (!ok && obj.isEmpty()) {
            AuthResult r;
            r.success = false;
            r.message = netErr;
            emit tokenValidated(r);
            return;
        }
        emit tokenValidated(parseAuth(obj));
    });
}
