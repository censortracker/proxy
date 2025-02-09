#include "proxyservice.h"
#include "logger.h"

ProxyService::ProxyService(QObject* parent)
    : QObject(parent)
    , m_configManager(new ConfigManager())
    , m_xrayController(new XrayController())
{
    Logger::getInstance().debug("ProxyService initialized");
}

QJsonObject ProxyService::getConfig() const
{
    Logger::getInstance().debug("Getting active config");
    return m_configManager->getActiveConfig();
}

bool ProxyService::updateConfig(const QString& configStr)
{
    Logger::getInstance().info("Updating config");
    bool success = m_configManager->updateAllConfigs({configStr});
    if (success) {
        Logger::getInstance().info("Config updated successfully");
        emit configsChanged();
    } else {
        Logger::getInstance().error("Failed to update config");
    }
    return success;
}

bool ProxyService::startXray()
{
    Logger::getInstance().info("Starting Xray");
    bool success = m_xrayController->start(m_configManager->getActiveConfigPath());
    if (success) {
        Logger::getInstance().info("Xray started successfully");
    } else {
        Logger::getInstance().error("Failed to start Xray");
    }
    return success;
}

bool ProxyService::stopXray()
{
    Logger::getInstance().info("Stopping Xray");
    m_xrayController->stop();
    Logger::getInstance().info("Xray stopped");
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
    Logger::getInstance().debug("Getting all configs");
    return m_configManager->getAllConfigs();
}

QMap<QString, QJsonObject> ProxyService::getConfigsByUuids(const QStringList &uuids) const
{
    Logger::getInstance().debug(QString("Getting configs for UUIDs: %1").arg(uuids.join(", ")));
    return m_configManager->getConfigsByUuids(uuids);
}

bool ProxyService::addConfigs(const QStringList &serializedConfigs)
{
    Logger::getInstance().info(QString("Adding %1 new config(s)").arg(serializedConfigs.size()));
    bool success = m_configManager->addConfigs(serializedConfigs);
    if (success) {
        Logger::getInstance().info("Configs added successfully");
        emit configsChanged();
    } else {
        Logger::getInstance().error("Failed to add configs");
    }
    return success;
}

bool ProxyService::removeConfig(const QString &uuid)
{
    Logger::getInstance().info(QString("Removing config with UUID: %1").arg(uuid));
    
    // Store current active config UUID before removal
    QString activeUuid = m_configManager->getActiveConfigUuid();

    // Try to remove the config
    bool removed = m_configManager->removeConfig(uuid);
    if (removed) {
        Logger::getInstance().info("Config removed successfully");
        emit configsChanged();
        if (uuid == activeUuid && isXrayRunning()) {
            Logger::getInstance().info("Removed active config, restarting Xray");
            stopXray();
            
            // Check if there are any configs left
            QString newActiveUuid = m_configManager->getActiveConfigUuid();
            if (!newActiveUuid.isEmpty()) {
                Logger::getInstance().info(QString("Starting Xray with new active config: %1").arg(newActiveUuid));
                return startXray();
            } else {
                Logger::getInstance().info("No configs left after removal");
            }
        }
    } else {
        Logger::getInstance().error(QString("Failed to remove config with UUID: %1").arg(uuid));
    }
    return removed;
}

bool ProxyService::activateConfig(const QString &uuid)
{
    Logger::getInstance().info(QString("Activating config with UUID: %1").arg(uuid));
    if (m_configManager->activateConfig(uuid)) {
        Logger::getInstance().info("Config activated successfully");
        emit configsChanged();
        // If config is successfully activated, restart Xray
        if (isXrayRunning()) {
            Logger::getInstance().info("Restarting Xray with new config");
            stopXray();
            return startXray();
        }
        return true;
    }
    Logger::getInstance().error(QString("Failed to activate config with UUID: %1").arg(uuid));
    return false;
}

QJsonObject ProxyService::getActiveConfig() const
{
    Logger::getInstance().debug("Getting active config");
    return m_configManager->getActiveConfig();
}

bool ProxyService::updateAllConfigs(const QStringList &serializedConfigs)
{
    Logger::getInstance().info(QString("Updating all configs with %1 new config(s)").arg(serializedConfigs.size()));
    bool success = m_configManager->updateAllConfigs(serializedConfigs);
    if (success) {
        Logger::getInstance().info("All configs updated successfully");
        emit configsChanged();
        if (isXrayRunning()) {
            Logger::getInstance().info("Restarting Xray with updated configs");
            stopXray();
            return startXray();
        }
    } else {
        Logger::getInstance().error("Failed to update all configs");
    }
    return success;
}

QString ProxyService::getActiveConfigUuid() const
{
    return m_configManager->getActiveConfigUuid();
} 