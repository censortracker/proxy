#pragma once

#include "iproxyservice.h"
#include "configmanager.h"
#include "xraycontroller.h"
#include <QObject>
#include <QScopedPointer>

class ProxyService : public QObject, public IProxyService {
    Q_OBJECT

public:
    explicit ProxyService(QObject* parent = nullptr);
    ~ProxyService() = default;

    // IProxyService implementation
    QJsonObject getConfig() const override;
    bool updateConfig(const QString& configStr) override;
    
    bool startXray() override;
    bool stopXray() override;
    bool isXrayRunning() const override;
    qint64 getXrayProcessId() const override;
    QString getXrayError() const override;

private:
    QScopedPointer<ConfigManager> m_configManager;
    QScopedPointer<XrayController> m_xrayController;
}; 