#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QScopedPointer>
#include <QMenu>
#include <QAction>
#include <QMap>
#include <QJsonObject>

class TrayIcon : public QObject
{
    Q_OBJECT

public:
    explicit TrayIcon(QObject *parent = nullptr);
    ~TrayIcon() = default;

    void updateStatus(bool isActive);
    void updateConfigsMenu(const QMap<QString, QJsonObject>& configs, const QString& activeConfigUuid);
    void updatePorts(quint16 proxyPort, quint16 httpPort);
    void updateError(const QString &errorMessage);

signals:
    void quitRequested();
    void configSelected(const QString& uuid);

private:
    void setupMenu();
    void setupIcon();

    QScopedPointer<QSystemTrayIcon> m_trayIcon;
    QScopedPointer<QMenu> m_menu;
    QScopedPointer<QAction> m_statusAction;
    QScopedPointer<QAction> m_portsAction;
    QScopedPointer<QMenu> m_configsMenu;
}; 