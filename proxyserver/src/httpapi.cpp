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
    qDebug() << "  GET    /api/v1/configs";
    qDebug() << "  POST   /api/v1/configs";
    qDebug() << "  PUT    /api/v1/configs";
    qDebug() << "  DELETE /api/v1/configs";
    qDebug() << "  PUT    /api/v1/configs/activate";
    qDebug() << "  GET    /api/v1/configs/active";
    qDebug() << "  POST   /api/v1/up";
    qDebug() << "  POST   /api/v1/down";
    qDebug() << "  GET    /api/v1/ping";
    
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
    // Config management routes
    m_server.route("/api/v1/configs", QHttpServerRequest::Method::Get, 
        [this](const QHttpServerRequest &request) { return handleGetConfigs(request); });
    
    m_server.route("/api/v1/configs", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) { return handleAddConfigs(request); });
    
    m_server.route("/api/v1/configs", QHttpServerRequest::Method::Put,
        [this](const QHttpServerRequest &request) { return handleUpdateConfigs(request); });
    
    m_server.route("/api/v1/configs", QHttpServerRequest::Method::Delete,
        [this](const QHttpServerRequest &request) { return handleDeleteConfig(request); });
    
    m_server.route("/api/v1/configs/activate", QHttpServerRequest::Method::Put,
        [this](const QHttpServerRequest &request) { return handleActivateConfig(request); });
    
    m_server.route("/api/v1/configs/active", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) { return handleGetActiveConfig(request); });

    // Xray control routes
    m_server.route("/api/v1/up", QHttpServerRequest::Method::Post,
        [this] { return handlePostUp(); });

    m_server.route("/api/v1/down", QHttpServerRequest::Method::Post,
        [this] { return handlePostDown(); });

    m_server.route("/api/v1/ping", QHttpServerRequest::Method::Get,
        [this] { return handleGetPing(); });
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

QHttpServerResponse HttpApi::handleGetConfigs(const QHttpServerRequest &request)
{
    if (auto service = m_service.lock()) {
        // Get UUIDs from query parameters if present
        QString uuidList = request.query().queryItemValue("uuid");
        QMap<QString, QJsonObject> configs;

        if (uuidList.isEmpty()) {
            // Return all configs if no UUIDs specified
            configs = service->getAllConfigs();
        } else {
            // Return only requested configs
            QStringList uuids = uuidList.split(',', Qt::SkipEmptyParts);
            configs = service->getConfigsByUuids(uuids);
        }

        QJsonObject response;
        response["status"] = "success";
        
        // Convert QMap to QJsonObject manually
        QJsonObject configsJson;
        for (auto it = configs.constBegin(); it != configs.constEnd(); ++it) {
            configsJson[it.key()] = it.value();
        }
        response["configs"] = configsJson;
        
        return QHttpServerResponse(response);
    }

    return QHttpServerResponse(
        QJsonObject{{"status", "error"}, {"message", "Service unavailable"}},
        QHttpServerResponse::StatusCode::ServiceUnavailable
    );
}

QHttpServerResponse HttpApi::handleAddConfigs(const QHttpServerRequest &request)
{
    if (auto service = m_service.lock()) {
        // Parse request body
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);
        
        if (parseError.error != QJsonParseError::NoError) {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "error"},
                    {"message", "Invalid JSON format"}
                },
                QHttpServerResponse::StatusCode::BadRequest
            );
        }

        if (!doc.isObject()) {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "error"},
                    {"message", "Request body must be a JSON object"}
                },
                QHttpServerResponse::StatusCode::BadRequest
            );
        }

        QJsonObject root = doc.object();
        if (!root.contains("configs") || !root["configs"].isArray()) {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "error"},
                    {"message", "Request body must contain 'configs' array"}
                },
                QHttpServerResponse::StatusCode::BadRequest
            );
        }

        // Convert JSON array to string list
        QStringList configs;
        QJsonArray configsArray = root["configs"].toArray();
        for (const auto &value : configsArray) {
            if (!value.isString()) {
                return QHttpServerResponse(
                    QJsonObject{
                        {"status", "error"},
                        {"message", "All configs must be strings"}
                    },
                    QHttpServerResponse::StatusCode::BadRequest
                );
            }
            configs.append(value.toString());
        }

        if (configs.isEmpty()) {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "error"},
                    {"message", "Configs array cannot be empty"}
                },
                QHttpServerResponse::StatusCode::BadRequest
            );
        }

        // Add configs
        if (service->addConfigs(configs)) {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "success"},
                    {"message", "Configs added successfully"}
                }
            );
        } else {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "error"},
                    {"message", "Failed to add configs"}
                },
                QHttpServerResponse::StatusCode::InternalServerError
            );
        }
    }

    return QHttpServerResponse(
        QJsonObject{{"status", "error"}, {"message", "Service unavailable"}},
        QHttpServerResponse::StatusCode::ServiceUnavailable
    );
}

QHttpServerResponse HttpApi::handleUpdateConfigs(const QHttpServerRequest &request)
{
    if (auto service = m_service.lock()) {
        // Parse request body
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);
        
        if (parseError.error != QJsonParseError::NoError) {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "error"},
                    {"message", "Invalid JSON format"}
                },
                QHttpServerResponse::StatusCode::BadRequest
            );
        }

        if (!doc.isObject()) {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "error"},
                    {"message", "Request body must be a JSON object"}
                },
                QHttpServerResponse::StatusCode::BadRequest
            );
        }

        QJsonObject root = doc.object();
        if (!root.contains("configs") || !root["configs"].isArray()) {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "error"},
                    {"message", "Request body must contain 'configs' array"}
                },
                QHttpServerResponse::StatusCode::BadRequest
            );
        }

        // Convert JSON array to string list
        QStringList configs;
        QJsonArray configsArray = root["configs"].toArray();
        for (const auto &value : configsArray) {
            if (!value.isString()) {
                return QHttpServerResponse(
                    QJsonObject{
                        {"status", "error"},
                        {"message", "All configs must be strings"}
                    },
                    QHttpServerResponse::StatusCode::BadRequest
                );
            }
            configs.append(value.toString());
        }

        if (configs.isEmpty()) {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "error"},
                    {"message", "Configs array cannot be empty"}
                },
                QHttpServerResponse::StatusCode::BadRequest
            );
        }

        // Update configs
        if (service->updateAllConfigs(configs)) {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "success"},
                    {"message", "Configs updated successfully"}
                }
            );
        } else {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "error"},
                    {"message", "Failed to update configs"}
                },
                QHttpServerResponse::StatusCode::InternalServerError
            );
        }
    }

    return QHttpServerResponse(
        QJsonObject{{"status", "error"}, {"message", "Service unavailable"}},
        QHttpServerResponse::StatusCode::ServiceUnavailable
    );
}

QHttpServerResponse HttpApi::handleDeleteConfig(const QHttpServerRequest &request)
{
    if (auto service = m_service.lock()) {
        // Get UUID from query parameters
        QString uuid = request.query().queryItemValue("uuid");
        
        if (uuid.isEmpty()) {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "error"},
                    {"message", "UUID parameter is required"}
                },
                QHttpServerResponse::StatusCode::BadRequest
            );
        }

        // Delete config
        if (service->removeConfig(uuid)) {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "success"},
                    {"message", "Config deleted successfully"}
                }
            );
        } else {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "error"},
                    {"message", "Failed to delete config"}
                },
                QHttpServerResponse::StatusCode::InternalServerError
            );
        }
    }

    return QHttpServerResponse(
        QJsonObject{{"status", "error"}, {"message", "Service unavailable"}},
        QHttpServerResponse::StatusCode::ServiceUnavailable
    );
}

QHttpServerResponse HttpApi::handleActivateConfig(const QHttpServerRequest &request)
{
    if (auto service = m_service.lock()) {
        // Get UUID from query parameters
        QString uuid = request.query().queryItemValue("uuid");
        
        if (uuid.isEmpty()) {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "error"},
                    {"message", "UUID parameter is required"}
                },
                QHttpServerResponse::StatusCode::BadRequest
            );
        }

        // Activate config
        if (service->activateConfig(uuid)) {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "success"},
                    {"message", "Config activated successfully"}
                }
            );
        } else {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "error"},
                    {"message", "Failed to activate config"}
                },
                QHttpServerResponse::StatusCode::InternalServerError
            );
        }
    }

    return QHttpServerResponse(
        QJsonObject{{"status", "error"}, {"message", "Service unavailable"}},
        QHttpServerResponse::StatusCode::ServiceUnavailable
    );
}

QHttpServerResponse HttpApi::handleGetActiveConfig(const QHttpServerRequest &request)
{
    if (auto service = m_service.lock()) {
        QJsonObject activeConfig = service->getActiveConfig();
        
        if (!activeConfig.isEmpty()) {
            QJsonObject response;
            response["status"] = "success";
            response["config"] = activeConfig;
            return QHttpServerResponse(response);
        } else {
            return QHttpServerResponse(
                QJsonObject{
                    {"status", "error"},
                    {"message", "No active config found"}
                },
                QHttpServerResponse::StatusCode::NotFound
            );
        }
    }

    return QHttpServerResponse(
        QJsonObject{{"status", "error"}, {"message", "Service unavailable"}},
        QHttpServerResponse::StatusCode::ServiceUnavailable
    );
} 