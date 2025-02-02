#pragma once

#include <QObject>
#include <QScopedPointer>
#include <QSharedPointer>
#include "trayicon.h"
#include "proxyservice.h"
#include "httpapi.h"

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
    void onConfigSelected(const QString& uuid);

private:
    bool startXrayProcess();
    void stopXrayProcess();
    void updateTrayConfigsMenu();

    QScopedPointer<HttpApi> m_api;
    TrayIcon m_trayIcon;
    QSharedPointer<ProxyService> m_service;
};