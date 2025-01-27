#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QNetworkReply>

class ProxyClient : public QObject
{
    Q_OBJECT

public:
    explicit ProxyClient(const QString &serverUrl = "http://localhost:8080", QObject *parent = nullptr);
    ~ProxyClient();

    void getConfig();
    void setConfig(const QJsonObject &config);
    void startProxy();
    void stopProxy();
    void ping();

signals:
    void configReceived(const QJsonObject &config);
    void configSaved(bool success, const QString &message);
    void proxyStarted(bool success, const QString &message);
    void proxyStopped(bool success, const QString &message);
    void pingReceived(const QJsonObject &status);
    void error(const QString &errorMessage);

private:
    void handleNetworkReply(QNetworkReply *reply, std::function<void(const QJsonObject&)> handler);

    QNetworkAccessManager m_networkManager;
    QString m_serverUrl;
}; 