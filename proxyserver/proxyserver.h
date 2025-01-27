#pragma once

#include <QObject>
#include <QHttpServer>
#include <QTcpServer>
#include <QScopedPointer>
#include "trayicon.h"
#include "proxyservice.h"

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
    QScopedPointer<QTcpServer> m_tcpServer;
    TrayIcon m_trayIcon;
    ProxyService m_service;
};