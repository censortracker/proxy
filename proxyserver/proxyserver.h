#pragma once

#include <QObject>
#include <QHttpServer>
#include <QProcess>
#include <QScopedPointer>
#include <QJsonObject>
#include <QTcpServer>
#include "trayicon.h"
#include "configmanager.h"
#include "xraycontroller.h"

class ProxyServer : public QObject
{
    Q_OBJECT

public:
    explicit ProxyServer(QObject *parent = nullptr);
    ~ProxyServer();

    bool start(quint16 port = 8080);
    void stop();

private slots:
    void quit();
    void showSettings();

private:
    void setupRoutes();
    bool startXrayProcess();
    void stopXrayProcess();

    QHttpServer m_server;
    QScopedPointer<QProcess> m_xrayProcess;
    QScopedPointer<QTcpServer> m_tcpServer;
    TrayIcon m_trayIcon;
    ConfigManager m_configManager;
    XrayController m_xrayController;
};