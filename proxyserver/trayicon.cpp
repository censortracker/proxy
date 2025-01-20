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
    m_trayIcon->setToolTip("CT Desktop Proxy");
    m_trayIcon->setContextMenu(m_menu.data());
    m_trayIcon->show();
}

void TrayIcon::setupMenu()
{
    m_menu.reset(new QMenu());
    
    m_statusAction.reset(m_menu->addAction("Xray Status: Inactive"));
    m_statusAction->setEnabled(false);
    
    m_menu->addSeparator();
    
    auto settingsAction = m_menu->addAction("Settings");
    connect(settingsAction, &QAction::triggered, this, &TrayIcon::settingsRequested);
    
    auto quitAction = m_menu->addAction("Exit");
    connect(quitAction, &QAction::triggered, this, &TrayIcon::quitRequested);
}

void TrayIcon::updateStatus(bool isActive)
{
    m_statusAction->setText(QString("Status: %1").arg(isActive ? "Active" : "Inactive"));
} 