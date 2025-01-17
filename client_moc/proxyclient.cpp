#include "proxyclient.h"
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QUrl>

ProxyClient::ProxyClient(const QString &serverUrl, QObject *parent)
    : QObject(parent)
    , m_serverUrl(serverUrl)
{
}

ProxyClient::~ProxyClient()
{
}

void ProxyClient::handleNetworkReply(QNetworkReply *reply, std::function<void(const QJsonObject&)> handler)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit error(reply->errorString());
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        emit error("JSON parsing error: " + parseError.errorString());
        return;
    }

    handler(doc.object());
}

void ProxyClient::getConfig()
{
    QNetworkRequest request(m_serverUrl + "/api/v1/config");
    QNetworkReply *reply = m_networkManager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleNetworkReply(reply, [this](const QJsonObject &response) {
            if (response["status"].toString() == "success") {
                emit configReceived(response["config"].toObject());
            } else {
                emit error(response["message"].toString());
            }
        });
    });
}

void ProxyClient::setConfig(const QJsonObject &config)
{
    QNetworkRequest request(m_serverUrl + "/api/v1/config");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonDocument doc(config);
    QNetworkReply *reply = m_networkManager.post(request, doc.toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleNetworkReply(reply, [this](const QJsonObject &response) {
            bool success = response["status"].toString() == "success";
            emit configSaved(success, response["message"].toString());
        });
    });
}

void ProxyClient::startProxy()
{
    QNetworkRequest request(m_serverUrl + "/api/v1/up");
    QNetworkReply *reply = m_networkManager.post(request, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleNetworkReply(reply, [this](const QJsonObject &response) {
            bool success = response["status"].toString() == "success";
            emit proxyStarted(success, response["message"].toString());
        });
    });
}

void ProxyClient::stopProxy()
{
    QNetworkRequest request(m_serverUrl + "/api/v1/down");
    QNetworkReply *reply = m_networkManager.post(request, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleNetworkReply(reply, [this](const QJsonObject &response) {
            bool success = response["status"].toString() == "success";
            emit proxyStopped(success, response["message"].toString());
        });
    });
}

void ProxyClient::ping()
{
    QNetworkRequest request(m_serverUrl + "/api/v1/ping");
    QNetworkReply *reply = m_networkManager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleNetworkReply(reply, [this](const QJsonObject &response) {
            emit pingReceived(response);
        });
    });
} 