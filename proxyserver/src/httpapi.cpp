#include "httpapi.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QHostAddress>
#include <QDebug>

HttpApi::HttpApi(QWeakPointer<IProxyService> service, QObject* parent)
    : QObject(parent)
    , m_tcpServer(new QTcpServer(this))
    , m_service(service)
{
}

HttpApi::~HttpApi()
{
    stop();
}

bool HttpApi::start(quint16 port)
{
    if (!m_tcpServer->listen(QHostAddress::LocalHost, port)) {
        qDebug() << "Server failed to start!";
        return false;
    }

    setupRoutes();
    m_server.bind(m_tcpServer.data());
    
    qDebug() << "Server is running on localhost:" << m_tcpServer->serverPort();
    qDebug() << "Available endpoints:";
    qDebug() << "  GET  /api/v1/config";
    qDebug() << "  POST /api/v1/config";
    qDebug() << "  POST /api/v1/up";
    qDebug() << "  POST /api/v1/down";
    qDebug() << "  GET  /api/v1/ping";
    
    return true;
}

void HttpApi::stop()
{
    if (m_tcpServer) {
        m_tcpServer->close();
    }
}

void HttpApi::setupRoutes()
{
    m_server.route("/api/v1/config", QHttpServerRequest::Method::Get,
        [this] { return handleGetConfig(); });

    m_server.route("/api/v1/config", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest& request) { return handlePostConfig(request); });

    m_server.route("/api/v1/up", QHttpServerRequest::Method::Post,
        [this] { return handlePostUp(); });

    m_server.route("/api/v1/down", QHttpServerRequest::Method::Post,
        [this] { return handlePostDown(); });

    m_server.route("/api/v1/ping", QHttpServerRequest::Method::Get,
        [this] { return handleGetPing(); });
}

QJsonObject HttpApi::handleGetConfig() const
{
    if (auto service = m_service.lock()) {
        QJsonObject config = service->getConfig();
        QJsonObject response;
        
        if (config.isEmpty()) {
            response["status"] = "error";
            response["message"] = "Failed to read config";
        } else {
            response["status"] = "success";
            response["config"] = config;
        }
        
        return response;
    }
    return { {"status", "error"}, {"message", "Service unavailable"} };
}

QJsonObject HttpApi::handlePostConfig(const QHttpServerRequest& request)
{
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
    
    if (auto service = m_service.lock()) {
        if (service->updateConfig(configStr)) {
            response["status"] = "success";
            response["message"] = "Config saved successfully";
        } else {
            response["status"] = "error";
            response["message"] = "Failed to save config";
        }
    } else {
        response["status"] = "error";
        response["message"] = "Service unavailable";
    }
    
    return response;
}

QJsonObject HttpApi::handlePostUp()
{
    QJsonObject response;
    if (auto service = m_service.lock()) {
        if (service->startXray()) {
            response["status"] = "success";
            response["message"] = "Xray process started successfully";
            
            // Try to get port from inbounds configuration
            QJsonObject config = service->getConfig();
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
    } else {
        response["status"] = "error";
        response["message"] = "Service unavailable";
    }
    return response;
}

QJsonObject HttpApi::handlePostDown()
{
    if (auto service = m_service.lock()) {
        service->stopXray();
        QJsonObject response;
        response["status"] = "success";
        response["message"] = "Xray process stopped";
        return response;
    }
    return { {"status", "error"}, {"message", "Service unavailable"} };
}

QJsonObject HttpApi::handleGetPing() const
{
    QJsonObject response;
    response["status"] = "success";
    response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    if (auto service = m_service.lock()) {
        bool isRunning = service->isXrayRunning();
        response["xray_running"] = isRunning;
        
        if (isRunning) {
            response["xray_pid"] = service->getXrayProcessId();
            response["xray_state"] = "running";
            
            QJsonObject config = service->getConfig();
            if (config.contains("inbounds") && config["inbounds"].isArray()) {
                QJsonArray inbounds = config["inbounds"].toArray();
                if (!inbounds.isEmpty() && inbounds[0].isObject()) {
                    QJsonObject firstInbound = inbounds[0].toObject();
                    if (firstInbound.contains("port")) {
                        response["xray_port"] = firstInbound["port"].toInt();
                    }
                }
            }
            
            QString error = service->getXrayError();
            if (!error.isEmpty()) {
                response["xray_error"] = error;
            }
        } else {
            response["xray_state"] = "stopped";
        }
    } else {
        response["status"] = "error";
        response["message"] = "Service unavailable";
    }
    
    return response;
} 