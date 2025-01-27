#pragma once

#include <QJsonObject>

class IProxyService {
public:
    virtual ~IProxyService() = default;

    // Config operations
    virtual QJsonObject getConfig() const = 0;
    virtual bool updateConfig(const QString& configStr) = 0;

    // Xray process operations
    virtual bool startXray() = 0;
    virtual bool stopXray() = 0;
    virtual bool isXrayRunning() const = 0;
    virtual qint64 getXrayProcessId() const = 0;
    virtual QString getXrayError() const = 0;
}; 