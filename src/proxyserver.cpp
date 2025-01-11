#include "proxyserver.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

ProxyServer::ProxyServer(QObject *parent)
    : QObject(parent)
    , m_xrayProcess(nullptr)
{
    setupRoutes();
}

ProxyServer::~ProxyServer()
{
    stop();
}

bool ProxyServer::start(quint16 port)
{
    const auto serverPort = m_server.listen(QHostAddress::LocalHost, port);
    if (!serverPort) {
        qDebug() << "Server failed to start!";
        return false;
    }

    qDebug() << "Server is running on localhost:" << serverPort;
    qDebug() << "Available endpoints:";
    qDebug() << "  GET  /api/v1/config";
    qDebug() << "  POST /api/v1/config";
    qDebug() << "  POST /api/v1/up";
    qDebug() << "  POST /api/v1/down";
    qDebug() << "  GET  /api/v1/ping";

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
    QString appDir = QCoreApplication::applicationDirPath();
    QString xrayDir = QDir(appDir).filePath("xray-prebuilt/" + getPlatformName());
    
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
            response["message"] = "Invalid JSON: " + parseError.errorString();
            return response;
        }
        
        QJsonObject config = doc.object();
        if (config.isEmpty()) {
            response["status"] = "error";
            response["message"] = "Config cannot be empty";
            return response;
        }
        
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
            response["status"] = "success";
            response["message"] = "Xray process started successfully";
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
            
            if (m_xrayProcess->error() != QProcess::UnknownError) {
                response["xray_error"] = m_xrayProcess->errorString();
            }
        } else {
            response["xray_state"] = "stopped";
        }
        
        return response;
    });
} 