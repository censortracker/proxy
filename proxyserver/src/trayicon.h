#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QScopedPointer>
#include <QMenu>
#include <QAction>

class TrayIcon : public QObject
{
    Q_OBJECT

public:
    explicit TrayIcon(QObject *parent = nullptr);
    ~TrayIcon() = default;

    void updateStatus(bool isActive);

signals:
    void settingsRequested();
    void quitRequested();

private:
    void setupMenu();
    void setupIcon();

    QScopedPointer<QSystemTrayIcon> m_trayIcon;
    QScopedPointer<QMenu> m_menu;
    QScopedPointer<QAction> m_statusAction;
}; 