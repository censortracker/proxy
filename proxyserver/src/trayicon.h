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

signals:
    void settingsRequested();
    void quitRequested();
    void configSelected(const QString& uuid);

private:
    void setupMenu();
    void setupIcon();

    QScopedPointer<QSystemTrayIcon> m_trayIcon;
    QScopedPointer<QMenu> m_menu;
    QScopedPointer<QAction> m_statusAction;
    QScopedPointer<QMenu> m_configsMenu;
}; 