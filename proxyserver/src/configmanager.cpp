#include "configmanager.h"
#include "serialization.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QUuid>

ConfigManager::ConfigManager()
{
    // Create configs directory if it doesn't exist
    QDir().mkpath(getConfigsPath());

    // Read active config UUID
    QJsonObject configsInfo = readConfigsInfo();
    m_activeConfigUuid = configsInfo["activeConfigUuid"].toString();
}

QString ConfigManager::getConfigsPath() const
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return QDir(configDir).filePath("xray_configs");
}

QString ConfigManager::getActiveConfigPath() const
{
    return QDir(getConfigsPath()).filePath("active_config.json");
}

QString ConfigManager::getConfigsInfoPath() const
{
    return QDir(getConfigsPath()).filePath("configs_info.json");
}

QJsonObject ConfigManager::readConfigsInfo() const
{
    QFile file(getConfigsInfoPath());

    if (!file.exists()) {
        // If file doesn't exist, return empty structure
        QJsonObject configsInfo;
        configsInfo["version"] = 1;
        configsInfo["configs"] = QJsonObject();
        return configsInfo;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open configs info file:" << file.errorString();
        return QJsonObject();
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "Failed to parse configs info file:" << parseError.errorString();
        return QJsonObject();
    }

    return doc.object();
}

bool ConfigManager::writeConfigsInfo(const QJsonObject &configsInfo)
{
    QFile file(getConfigsInfoPath());

    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open configs info file for writing:" << file.errorString();
        return false;
    }

    QJsonDocument doc(configsInfo);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

QJsonObject ConfigManager::readActiveConfig() const
{
    QFile file(getActiveConfigPath());

    if (!file.exists()) {
        qDebug() << "Active config file not found at:" << getActiveConfigPath();
        return QJsonObject();
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open active config file:" << file.errorString();
        return QJsonObject();
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "Failed to parse active config file:" << parseError.errorString();
        return QJsonObject();
    }

    return doc.object();
}

bool ConfigManager::writeActiveConfig(const QJsonObject &config)
{
    QFile file(getActiveConfigPath());

    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open active config file for writing:" << file.errorString();
        return false;
    }

    QJsonDocument doc(config);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

QString ConfigManager::generateUuid() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString ConfigManager::getProtocolFromSerializedConfig(const QString &config) const
{
    if (config.startsWith("vless://")) return "vless";
    if (config.startsWith("vmess://")) return "vmess";
    if (config.startsWith("trojan://")) return "trojan";
    if (config.startsWith("ss://")) return "ss";
    return QString();
}

QJsonObject ConfigManager::deserializeConfig(const QString &configStr)
{
    QJsonObject outConfig;
    QString prefix;
    QString errormsg;

    if (configStr.startsWith("vless://")) {
        outConfig = amnezia::serialization::vless::Deserialize(configStr, &prefix, &errormsg);
    }

    if (configStr.startsWith("vmess://") && configStr.contains("@")) {
        outConfig = amnezia::serialization::vmess_new::Deserialize(configStr, &prefix, &errormsg);
    }

    if (configStr.startsWith("vmess://")) {
        outConfig = amnezia::serialization::vmess::Deserialize(configStr, &prefix, &errormsg);
    }

    if (configStr.startsWith("trojan://")) {
        outConfig = amnezia::serialization::trojan::Deserialize(configStr, &prefix, &errormsg);
    }

    if (configStr.startsWith("ss://") && !configStr.contains("plugin=")) {
        outConfig = amnezia::serialization::ss::Deserialize(configStr, &prefix, &errormsg);
    }

    return outConfig;
}

QJsonObject ConfigManager::addInbounds(const QJsonObject &config)
{
    QJsonObject resultConfig = config;

    QJsonArray inbounds;
    QJsonObject socksInbound;
    socksInbound["listen"] = "127.0.0.1";
    socksInbound["port"] = 10808;
    socksInbound["protocol"] = "socks";

    QJsonObject settings;
    settings["udp"] = true;
    socksInbound["settings"] = settings;

    inbounds.append(socksInbound);
    resultConfig["inbounds"] = inbounds;

    return resultConfig;
}

bool ConfigManager::addConfigs(const QStringList &serializedConfigs)
{
    QJsonObject configsInfo = readConfigsInfo();
    QJsonObject configs = configsInfo["configs"].toObject();
    QString activeUuid = getActiveConfigUuid();
    QString firstUuid;

    for (const QString &serializedConfig : serializedConfigs)
    {
        QString uuid = generateUuid();
        if (firstUuid.isEmpty()) {
            firstUuid = uuid;
        }

        QJsonObject currentConfigInfo;
        currentConfigInfo["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        currentConfigInfo["lastUsed"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        currentConfigInfo["protocol"] = getProtocolFromSerializedConfig(serializedConfig);
        currentConfigInfo["serializedConfig"] = serializedConfig;

        configs[uuid] = currentConfigInfo;
    }

    configsInfo["configs"] = configs;
    if (!writeConfigsInfo(configsInfo)) {
        qDebug() << "Failed to write configs info";
        return false;
    }

    // If there's no active config, activate the first added one
    if (activeUuid.isEmpty() && !firstUuid.isEmpty())
    {
        return activateConfig(firstUuid);
    }

    return true;
}

bool ConfigManager::removeConfig(const QString &uuid)
{
    QJsonObject configsInfo = readConfigsInfo();
    QJsonObject configs = configsInfo["configs"].toObject();

    // Check if config exists
    if (!configs.contains(uuid))
    {
        return false;
    }

    configs.remove(uuid);

    // If removing active config, activate another one first   
    if (getActiveConfigUuid() == uuid)
    {
        if (configs.isEmpty())
        {
            if (!activateConfig(QString()))
            {
                return false;
            }
        }
        else
        {
            if (!activateConfig(configs.keys().first()))
            {
                return false;
            }
        }
    }

    configsInfo["configs"] = configs;
    return writeConfigsInfo(configsInfo);
}

bool ConfigManager::activateConfig(const QString &uuid)
{
    QJsonObject configsInfo = readConfigsInfo();
    QJsonObject configs = configsInfo["configs"].toObject();

    // If uuid is empty, just reset active config
    if (uuid.isEmpty())
    {
        m_activeConfigUuid = QString();
        configsInfo["activeConfigUuid"] = QString();
        return writeConfigsInfo(configsInfo);
    }

    // Check if config exists
    if (!configs.contains(uuid))
    {
        qDebug() << "Config with UUID" << uuid << "not found";
        return false;
    }

    QJsonObject currentConfigInfo = configs[uuid].toObject();
    QString serializedConfig = currentConfigInfo["serializedConfig"].toString();

    // Deserialize config and add inbounds
    QJsonObject currentConfig = deserializeConfig(serializedConfig);
    if (currentConfig.isEmpty())
    {
        qDebug() << "Failed to deserialize config for UUID:" << uuid;
        return false;
    }

    currentConfig = addInbounds(currentConfig);

    // Update lastUsed
    currentConfigInfo["lastUsed"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    configs[uuid] = currentConfigInfo;
    configsInfo["configs"] = configs;

    // Update active config
    m_activeConfigUuid = uuid;
    configsInfo["activeConfigUuid"] = uuid;

    // Save changes
    if (!writeConfigsInfo(configsInfo))
    {
        qDebug() << "Failed to write configs info file";
        return false;
    }

    return writeActiveConfig(currentConfig);
}

QJsonObject ConfigManager::getActiveConfig() const
{
    return readActiveConfig();
}

QMap<QString, QJsonObject> ConfigManager::getAllConfigs() const
{
    QJsonObject configsInfo = readConfigsInfo();
    QJsonObject configs = configsInfo["configs"].toObject();

    QMap<QString, QJsonObject> result;
    for (auto it = configs.begin(); it != configs.end(); ++it)
    {
        result[it.key()] = it.value().toObject();
    }

    return result;
}

QMap<QString, QJsonObject> ConfigManager::getConfigsByUuids(const QStringList &uuids) const
{
    QMap<QString, QJsonObject> allConfigs = getAllConfigs();
    if (uuids.isEmpty())
    {
        return allConfigs;
    }

    QMap<QString, QJsonObject> result;
    for (const QString &uuid : uuids)
    {
        if (allConfigs.contains(uuid))
        {
            result[uuid] = allConfigs[uuid];
        }
        else
        {
            QJsonObject undefinedConfig;
            undefinedConfig["status"] = "Undefined";
            result[uuid] = undefinedConfig;
        }
    }

    return result;
}

bool ConfigManager::updateAllConfigs(const QStringList &serializedConfigs)
{
    // Clear all existing configs by writing empty config list
    QJsonObject configsInfo;
    configsInfo["version"] = 1;
    configsInfo["configs"] = QJsonObject();
    
    if (!writeConfigsInfo(configsInfo)) {
        qDebug() << "Failed to clear configs info while updating";
        return false;
    }
    
    // Add new configs
    return addConfigs(serializedConfigs);
}