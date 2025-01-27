#include "proxyclient.h"
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    ProxyClient client;

    // Connect signals to debug output
    QObject::connect(&client, &ProxyClient::configReceived, [](const QJsonObject &config) {
        qDebug() << "Configuration received:" << config;
    });

    QObject::connect(&client, &ProxyClient::configSaved, [](bool success, const QString &message) {
        qDebug() << "Config saved:" << (success ? "Success" : "Failed") << "-" << message;
    });

    QObject::connect(&client, &ProxyClient::proxyStarted, [](bool success, const QString &message) {
        qDebug() << "Proxy started:" << (success ? "Success" : "Failed") << "-" << message;
    });

    QObject::connect(&client, &ProxyClient::proxyStopped, [](bool success, const QString &message) {
        qDebug() << "Proxy stopped:" << (success ? "Success" : "Failed") << "-" << message;
    });

    QObject::connect(&client, &ProxyClient::pingReceived, [](const QJsonObject &status) {
        qDebug() << "Ping response:" << status;
    });

    QObject::connect(&client, &ProxyClient::error, [](const QString &error) {
        qDebug() << "Error occurred:" << error;
    });

    // Test all API endpoints
    qDebug() << "Testing API endpoints...";
    
    client.ping();
    client.getConfig();
    
    // Example config for testing setConfig
    QJsonObject testConfig;
    // testConfig["test"] = "value";
    // client.setConfig(testConfig);
    
    client.startProxy();
    
    // Stop proxy after 5 seconds
    QTimer::singleShot(5000, &client, [&client]() {
        qDebug() << "Stopping proxy...";
        client.stopProxy();
    });
    
    // Quit application after 6 seconds
    QTimer::singleShot(6000, &app, &QCoreApplication::quit);

    return app.exec();
} 