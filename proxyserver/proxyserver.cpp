#include "proxyserver.h"
#include "serialization.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
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
    QJsonObject config = readConfig();
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

QString ProxyServer::getPlatformName() const
{
#if defined(Q_OS_WIN)
    return "windows";
#elif defined(Q_OS_MACOS)
    return "macos";
#elif defined(Q_OS_LINUX)
    return "linux";
#else
    #error "Unsupported platform"
#endif
}

QString ProxyServer::getXrayExecutablePath() const
{
    QString xrayDir = QCoreApplication::applicationDirPath();
    
#if defined(Q_OS_WIN)
    return QDir(xrayDir).filePath("xray.exe");
#else
    return QDir(xrayDir).filePath("xray");
#endif
}

QStringList ProxyServer::getXrayArguments(const QString &configPath) const
{
    return QStringList() << "-c" << configPath << "-format=json";
}

bool ProxyServer::startXrayProcess()
{
    if (!m_xrayProcess.isNull() && m_xrayProcess->state() == QProcess::Running) {
        qDebug() << "Xray process is already running";
        return true;
    }

    QString xrayPath = getXrayExecutablePath();
    QString configPath = getConfigPath();

    if (!QFile::exists(xrayPath)) {
        qDebug() << "Xray binary not found at:" << xrayPath;
        return false;
    }

    if (!QFile::exists(configPath)) {
        qDebug() << "Config file not found at:" << configPath;
        return false;
    }

    m_xrayProcess.reset(new QProcess(this));
    m_xrayProcess->setWorkingDirectory(QFileInfo(xrayPath).dir().absolutePath());
    m_xrayProcess->setProgram(xrayPath);
    m_xrayProcess->setArguments(getXrayArguments(configPath));

    m_xrayProcess->start();
    if (!m_xrayProcess->waitForStarted()) {
        qDebug() << "Failed to start xray process";
        qDebug() << "Error:" << m_xrayProcess->errorString();
        m_xrayProcess.reset();
        return false;
    }

    m_trayIcon.updateStatus(true);
    qDebug() << "Xray process started successfully";
    return true;
}

void ProxyServer::stopXrayProcess()
{
    if (!m_xrayProcess.isNull()) {
        if (m_xrayProcess->state() == QProcess::Running) {
            m_xrayProcess->terminate();
            if (!m_xrayProcess->waitForFinished(5000)) {
                m_xrayProcess->kill();
            }
        }
        m_xrayProcess.reset();
        m_trayIcon.updateStatus(false);
        qDebug() << "Xray process stopped";
    }
}

QString ProxyServer::getConfigPath() const
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    return QDir(configDir).filePath("xray-config.json");
}

QJsonObject ProxyServer::readConfig() const
{
    QString configPath = getConfigPath();
    QFile file(configPath);
    
    if (!file.exists()) {
        qDebug() << "Config file not found at:" << configPath;
        return QJsonObject();
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open config file:" << file.errorString();
        return QJsonObject();
    }
    
    QByteArray configData = file.readAll();
    file.close();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(configData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "Failed to parse config file:" << parseError.errorString();
        return QJsonObject();
    }
    
    return doc.object();
}

bool ProxyServer::writeConfig(const QJsonObject &config)
{
    QString configPath = getConfigPath();
    QFile file(configPath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open config file for writing:" << file.errorString();
        return false;
    }
    
    QJsonDocument doc(config);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    return true;
}

void ProxyServer::setupRoutes()
{
    m_server.route("/api/v1/config", QHttpServerRequest::Method::Get, [this] {
        QJsonObject config = readConfig();
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
        
        QJsonArray configArray = doc.object()["configs"].toArray();
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
        
        QJsonObject config = deserializeConfig(configStr);
        if (config.isEmpty()) {
            response["status"] = "error";
            response["message"] = "Failed to deserialize config string";
            return response;
        }
        
        config = addInbounds(config);
        
        if (writeConfig(config)) {
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
            QJsonObject config = readConfig();
            
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
        
        bool isRunning = !m_xrayProcess.isNull() && m_xrayProcess->state() == QProcess::Running;
        response["xray_running"] = isRunning;
        
        if (isRunning) {
            response["xray_pid"] = (qint64)m_xrayProcess->processId();
            response["xray_state"] = "running";
            
            QJsonObject config = readConfig();
            if (config.contains("inbounds") && config["inbounds"].isArray()) {
                QJsonArray inbounds = config["inbounds"].toArray();
                if (!inbounds.isEmpty() && inbounds[0].isObject()) {
                    QJsonObject firstInbound = inbounds[0].toObject();
                    if (firstInbound.contains("port")) {
                        response["xray_port"] = firstInbound["port"].toInt();
                    }
                }
            }
            
            if (m_xrayProcess->error() != QProcess::UnknownError) {
                response["xray_error"] = m_xrayProcess->errorString();
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

QJsonObject ProxyServer::addInbounds(const QJsonObject &config)
{
    QJsonObject resultConfig = config;
    
    QJsonArray inbounds;
    QJsonObject socksInbound;
    socksInbound["listen"] = "127.0.0.1";
    socksInbound["port"] = 10808;
    socksInbound["protocol"] = "socks";
    
    QJsonObject settings;
    settings["udp"] = true;
    socksInbound["settings"] = settings;
    
    inbounds.append(socksInbound);
    resultConfig["inbounds"] = inbounds;
    
    return resultConfig;
}

QJsonObject ProxyServer::deserializeConfig(const QString &configStr)
{
    QJsonObject outConfig;
    
    QString prefix;
    QString errormsg;
    
    if (configStr.startsWith("vless://")) {
        outConfig = amnezia::serialization::vless::Deserialize(configStr, &prefix, &errormsg);
    }

    if (configStr.startsWith("vmess://") && configStr.contains("@")) {
        outConfig = amnezia::serialization::vmess_new::Deserialize(configStr, &prefix, &errormsg);
    }

    if (configStr.startsWith("vmess://")) {
        outConfig = amnezia::serialization::vmess::Deserialize(configStr, &prefix, &errormsg);
    }

    if (configStr.startsWith("trojan://")) {
        outConfig = amnezia::serialization::trojan::Deserialize(configStr, &prefix, &errormsg);
    }

    if (configStr.startsWith("ss://") && !configStr.contains("plugin=")) {
        outConfig = amnezia::serialization::ss::Deserialize(configStr, &prefix, &errormsg);
    }

    return outConfig;
}
