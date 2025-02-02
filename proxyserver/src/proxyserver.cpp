#include "proxyserver.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QApplication>

ProxyServer::ProxyServer(QObject *parent)
    : QObject(parent)
    , m_trayIcon(this)
    , m_service(new ProxyService(this))
{
    connect(&m_trayIcon, &TrayIcon::settingsRequested, this, &ProxyServer::showSettings);
    connect(&m_trayIcon, &TrayIcon::quitRequested, this, &ProxyServer::quit);
    connect(&m_trayIcon, &TrayIcon::configSelected, this, &ProxyServer::onConfigSelected);
    connect(m_service.data(), &ProxyService::configsChanged, this, &ProxyServer::updateTrayConfigsMenu);
}

ProxyServer::~ProxyServer()
{
    stop();
}

bool ProxyServer::start(quint16 port)
{
    m_api.reset(new HttpApi(m_service.toWeakRef()));
    if (!m_api->start(port)) {
        return false;
    }

    // Auto-start Xray if config exists
    QJsonObject config = m_service->getConfig();
    if (!config.isEmpty()) {
        startXrayProcess();
    } else {
        qDebug() << "No config found, Xray will not start automatically";
    }

    updateTrayConfigsMenu();
    return true;
}

void ProxyServer::stop()
{
    stopXrayProcess();
    if (m_api) {
        m_api->stop();
    }
}

bool ProxyServer::startXrayProcess()
{
    bool success = m_service->startXray();
    if (success) {
        m_trayIcon.updateStatus(true);
    }
    return success;
}

void ProxyServer::stopXrayProcess()
{
    m_service->stopXray();
    m_trayIcon.updateStatus(false);
}

void ProxyServer::quit()
{
    stop();
    QApplication::quit();
}

void ProxyServer::showSettings()
{
    // TODO: Implement settings window
    qDebug() << "Show settings";
}

void ProxyServer::onConfigSelected(const QString& uuid)
{
    m_service->activateConfig(uuid);
}

void ProxyServer::updateTrayConfigsMenu()
{
    auto configs = m_service->getAllConfigs();
    QString activeConfigUuid = m_service->getActiveConfigUuid();
    m_trayIcon.updateConfigsMenu(configs, activeConfigUuid);
}