#include "proxyserver.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QApplication>

ProxyServer::ProxyServer(QObject *parent)
    : QObject(parent)
    , m_xrayProcess(nullptr)
    , m_tcpServer(new QTcpServer(this))
    , m_trayIcon(this)
{
    setupRoutes();
    connect(&m_trayIcon, &TrayIcon::settingsRequested, this, &ProxyServer::showSettings);
    connect(&m_trayIcon, &TrayIcon::quitRequested, this, &ProxyServer::quit);
}

ProxyServer::~ProxyServer()
{
    stop();
}

bool ProxyServer::start(quint16 port)
{
    if (!m_tcpServer->listen(QHostAddress::LocalHost, port))
    {
        qDebug() << "Server failed to start!";
        return false;
    }

    m_server.bind(m_tcpServer.data());
    qDebug() << "Server is running on localhost:" << m_tcpServer->serverPort();
    qDebug() << "Available endpoints:";
    qDebug() << "  GET  /api/v1/config";
    qDebug() << "  POST /api/v1/config";
    qDebug() << "  POST /api/v1/up";
    qDebug() << "  POST /api/v1/down";
    qDebug() << "  GET  /api/v1/ping";

    // Auto-start Xray if config exists
    QJsonObject config = m_configManager.readConfig();
    if (!config.isEmpty()) {
        startXrayProcess();
    } else {
        qDebug() << "No config found, Xray will not start automatically";
    }

    return true;
}

void ProxyServer::stop()
{
    stopXrayProcess();
}

bool ProxyServer::startXrayProcess()
{
    bool success = m_xrayController.start(m_configManager.getConfigPath());
    if (success) {
        m_trayIcon.updateStatus(true);
    }
    return success;
}

void ProxyServer::stopXrayProcess()
{
    m_xrayController.stop();
    m_trayIcon.updateStatus(false);
}

void ProxyServer::setupRoutes()
{
    m_server.route("/api/v1/config", QHttpServerRequest::Method::Get, [this] {
        QJsonObject config = m_configManager.readConfig();
        QJsonObject response;
        
        if (config.isEmpty()) {
            response["status"] = "error";
            response["message"] = "Failed to read config";
        } else {
            response["status"] = "success";
            response["config"] = config;
        }
        
        return response;
    });

    m_server.route("/api/v1/config", QHttpServerRequest::Method::Post, [this] (const QHttpServerRequest &request) {
        QJsonObject response;
        
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);
        
        if (parseError.error != QJsonParseError::NoError) {
            response["status"] = "error";
            response["message"] = "Invalid JSON format";
            return response;
        }
        
        if (!doc.isObject() || !doc.object().contains("config") || !doc.object()["config"].isArray()) {
            response["status"] = "error";
            response["message"] = "Request must contain 'config' array field";
            return response;
        }
        
        QJsonArray configArray = doc.object()["config"].toArray();
        if (configArray.isEmpty()) {
            response["status"] = "error";
            response["message"] = "Config array cannot be empty";
            return response;
        }
        
        QString configStr = configArray[0].toString();
        if (configStr.isEmpty()) {
            response["status"] = "error";
            response["message"] = "Config string cannot be empty";
            return response;
        }
        
        QJsonObject config = m_configManager.deserializeConfig(configStr);
        if (config.isEmpty()) {
            response["status"] = "error";
            response["message"] = "Failed to deserialize config string";
            return response;
        }
        
        config = m_configManager.addInbounds(config);
        
        if (m_configManager.writeConfig(config)) {
            response["status"] = "success";
            response["message"] = "Config saved successfully";
        } else {
            response["status"] = "error";
            response["message"] = "Failed to save config";
        }
        
        return response;
    });

    m_server.route("/api/v1/up", QHttpServerRequest::Method::Post, [this] {
        QJsonObject response;
        if (startXrayProcess()) {
            QJsonObject config = m_configManager.readConfig();
            
            response["status"] = "success";
            response["message"] = "Xray process started successfully";
            // Try to get port from inbounds configuration
            if (config.contains("inbounds") && config["inbounds"].isArray()) {
                QJsonArray inbounds = config["inbounds"].toArray();
                if (!inbounds.isEmpty() && inbounds[0].isObject()) {
                    QJsonObject firstInbound = inbounds[0].toObject();
                    if (firstInbound.contains("port")) {
                        response["xray_port"] = firstInbound["port"].toInt();
                    }
                }
            }
            
        } else {
            response["status"] = "error";
            response["message"] = "Failed to start xray process";
        }
        return response;
    });

    m_server.route("/api/v1/down", QHttpServerRequest::Method::Post, [this] {
        stopXrayProcess();
        QJsonObject response;
        response["status"] = "success";
        response["message"] = "Xray process stopped";
        return response;
    });

    m_server.route("/api/v1/ping", QHttpServerRequest::Method::Get, [this] {
        QJsonObject response;
        response["status"] = "success";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        bool isXrayRunning = m_xrayController.isXrayRunning();
        response["xray_running"] = isXrayRunning;
        
        if (isXrayRunning) {
            response["xray_pid"] = m_xrayController.getProcessId();
            response["xray_state"] = "running";
            
            QJsonObject config = m_configManager.readConfig();
            if (config.contains("inbounds") && config["inbounds"].isArray()) {
                QJsonArray inbounds = config["inbounds"].toArray();
                if (!inbounds.isEmpty() && inbounds[0].isObject()) {
                    QJsonObject firstInbound = inbounds[0].toObject();
                    if (firstInbound.contains("port")) {
                        response["xray_port"] = firstInbound["port"].toInt();
                    }
                }
            }
            
            QString error = m_xrayController.getError();
            if (!error.isEmpty()) {
                response["xray_error"] = error;
            }
        } else {
            response["xray_state"] = "stopped";
        }
        
        return response;
    });
}

void ProxyServer::quit()
{
    stop();
    QApplication::quit();
}

void ProxyServer::showSettings()
{
    // TODO: Implement settings window
    qDebug() << "Show settings";
}