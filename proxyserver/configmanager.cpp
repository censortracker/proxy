#include "configmanager.h"
#include "serialization.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

ConfigManager::ConfigManager()
{
}

QString ConfigManager::getConfigPath() const
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    return QDir(configDir).filePath("xray-config.json");
}

QJsonObject ConfigManager::readConfig() const
{
    QString configPath = getConfigPath();
    QFile file(configPath);
    
    if (!file.exists()) {
        qDebug() << "Config file not found at:" << configPath;
        return QJsonObject();
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open config file:" << file.errorString();
        return QJsonObject();
    }
    
    QByteArray configData = file.readAll();
    file.close();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(configData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "Failed to parse config file:" << parseError.errorString();
        return QJsonObject();
    }
    
    return doc.object();
}

bool ConfigManager::writeConfig(const QJsonObject &config)
{
    QString configPath = getConfigPath();
    QFile file(configPath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open config file for writing:" << file.errorString();
        return false;
    }
    
    QJsonDocument doc(config);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    return true;
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