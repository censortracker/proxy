#include "trayicon.h"

TrayIcon::TrayIcon(QObject *parent)
    : QObject(parent)
{
    setupMenu();
    setupIcon();
}

void TrayIcon::setupIcon()
{
    m_trayIcon.reset(new QSystemTrayIcon(this));
    m_trayIcon->setIcon(QIcon(":/icons/tray.png"));
    m_trayIcon->setToolTip("Censor Tracker Proxy");
    m_trayIcon->setContextMenu(m_menu.data());
    m_trayIcon->show();
}

void TrayIcon::setupMenu()
{
    m_menu.reset(new QMenu());
    
    m_statusAction.reset(m_menu->addAction("Xray Status: Inactive"));
    m_statusAction->setEnabled(false);
    
    m_portsAction.reset(m_menu->addAction("Ports: -"));
    m_portsAction->setEnabled(false);
    
    m_menu->addSeparator();
    
    m_configsMenu.reset(m_menu->addMenu("Xray Configs"));
    
    m_menu->addSeparator();
    
    auto quitAction = m_menu->addAction("Exit");
    connect(quitAction, &QAction::triggered, this, &TrayIcon::quitRequested);
}

void TrayIcon::updateStatus(bool isActive)
{
    m_statusAction->setText(QString("Status: %1").arg(isActive ? "Active" : "Inactive"));
}

void TrayIcon::updateConfigsMenu(const QMap<QString, QJsonObject>& configs, const QString& activeConfigUuid)
{
    if (!m_configsMenu) {
        return;
    }

    m_configsMenu->clear();
    
    for (auto it = configs.constBegin(); it != configs.constEnd(); ++it) {
        const QString& uuid = it.key();
        const QJsonObject& config = it.value();
        
        QString configName = config["name"].toString();
        if (configName.isEmpty()) {
            configName = QString("Config %1").arg(uuid.left(8));
        }
        
        QAction* action = m_configsMenu->addAction(configName);
        action->setCheckable(true);
        action->setChecked(uuid == activeConfigUuid);
        
        connect(action, &QAction::triggered, this, [this, uuid]() {
            emit configSelected(uuid);
        });
    }
}

void TrayIcon::updatePorts(quint16 proxyPort, quint16 httpPort)
{
    if(m_portsAction)
        m_portsAction->setText(QString("Ports: Xray: %1, HttpApi: %2").arg(proxyPort).arg(httpPort));
    if(m_trayIcon)
        m_trayIcon->setToolTip(QString("Censor Tracker Proxy (Xray: %1, HttpApi: %2)").arg(proxyPort).arg(httpPort));
}

void TrayIcon::updateError(const QString &errorMessage)
{
    if(m_statusAction)
        m_statusAction->setText(QString("Error: %1").arg(errorMessage));
    if(m_trayIcon)
        m_trayIcon->setToolTip(QString("Error: %1").arg(errorMessage));
} 