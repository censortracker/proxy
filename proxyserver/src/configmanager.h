#pragma once

#include <QJsonObject>
#include <QString>
#include <QFile>

class ConfigManager {
public:
    ConfigManager();
    
    // Config file operations
    QJsonObject readConfig() const;
    bool writeConfig(const QJsonObject& config);
    QString getConfigPath() const;
    
    // Config transformation
    QJsonObject deserializeConfig(const QString& configStr);
    QJsonObject addInbounds(const QJsonObject& config);
    bool updateConfigFromString(const QString& configStr);
}; 