#include "proxyservice.h"

ProxyService::ProxyService(QObject* parent)
    : QObject(parent)
    , m_configManager(new ConfigManager())
    , m_xrayController(new XrayController())
{
}

QJsonObject ProxyService::getConfig() const
{
    return m_configManager->getActiveConfig();
}

bool ProxyService::updateConfig(const QString& configStr)
{
    bool success = m_configManager->updateAllConfigs({configStr});
    if (success) {
        emit configsChanged();
    }
    return success;
}

bool ProxyService::startXray()
{
    return m_xrayController->start(m_configManager->getActiveConfigPath());
}

bool ProxyService::stopXray()
{
    m_xrayController->stop();
    return true;
}

bool ProxyService::isXrayRunning() const
{
    return m_xrayController->isXrayRunning();
}

qint64 ProxyService::getXrayProcessId() const
{
    return m_xrayController->getProcessId();
}

QString ProxyService::getXrayError() const
{
    return m_xrayController->getError();
}

QMap<QString, QJsonObject> ProxyService::getAllConfigs() const
{
    return m_configManager->getAllConfigs();
}

QMap<QString, QJsonObject> ProxyService::getConfigsByUuids(const QStringList &uuids) const
{
    return m_configManager->getConfigsByUuids(uuids);
}

bool ProxyService::addConfigs(const QStringList &serializedConfigs)
{
    bool success = m_configManager->addConfigs(serializedConfigs);
    if (success) {
        emit configsChanged();
    }
    return success;
}

bool ProxyService::removeConfig(const QString &uuid)
{
    // Store current active config UUID before removal
    QString activeUuid = m_configManager->getActiveConfigUuid();

    // Try to remove the config
    bool removed = m_configManager->removeConfig(uuid);
    if (removed) {
        emit configsChanged();
        if (uuid == activeUuid && isXrayRunning()) {
            // If we removed the active config and Xray is running
            stopXray();
            
            // Check if there are any configs left
            QString newActiveUuid = m_configManager->getActiveConfigUuid();
            if (!newActiveUuid.isEmpty()) {
                // If we have a new active config, restart Xray with it
                return startXray();
            }
        }
    }
    return removed;
}

bool ProxyService::activateConfig(const QString &uuid)
{
    if (m_configManager->activateConfig(uuid)) {
        emit configsChanged();
        // If config is successfully activated, restart Xray
        if (isXrayRunning()) {
            stopXray();
            return startXray();
        }
        return true;
    }
    return false;
}

QJsonObject ProxyService::getActiveConfig() const
{
    return m_configManager->getActiveConfig();
}

bool ProxyService::updateAllConfigs(const QStringList &serializedConfigs)
{
    bool success = m_configManager->updateAllConfigs(serializedConfigs);
    if (success) {
        emit configsChanged();
        if (isXrayRunning()) {
            // If configs are successfully updated and Xray is running, restart it
            stopXray();
            return startXray();
        }
    }
    return success;
}

QString ProxyService::getActiveConfigUuid() const
{
    return m_configManager->getActiveConfigUuid();
} 