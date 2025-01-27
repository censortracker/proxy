#include "xraycontroller.h"
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDebug>

XrayController::XrayController(QObject* parent)
    : QObject(parent)
    , m_process(nullptr)
{
}

XrayController::~XrayController()
{
    stop();
}

bool XrayController::start(const QString& configPath)
{
    if (isXrayRunning()) {
        qDebug() << "Xray process is already running";
        return true;
    }

    QString xrayPath = getXrayExecutablePath();

    if (!QFile::exists(xrayPath)) {
        qDebug() << "Xray binary not found at:" << xrayPath;
        return false;
    }

    if (!QFile::exists(configPath)) {
        qDebug() << "Config file not found at:" << configPath;
        return false;
    }

    m_process.reset(new QProcess(this));
    m_process->setWorkingDirectory(QFileInfo(xrayPath).dir().absolutePath());
    m_process->setProgram(xrayPath);
    m_process->setArguments(getXrayArguments(configPath));

    m_process->start();
    if (!m_process->waitForStarted()) {
        qDebug() << "Failed to start xray process";
        qDebug() << "Error:" << m_process->errorString();
        m_process.reset();
        return false;
    }

    qDebug() << "Xray process started successfully";
    return true;
}

void XrayController::stop()
{
    if (!m_process.isNull()) {
        if (m_process->state() == QProcess::Running) {
            m_process->terminate();
            if (!m_process->waitForFinished(5000)) {
                m_process->kill();
            }
        }
        m_process.reset();
        qDebug() << "Xray process stopped";
    }
}

bool XrayController::isXrayRunning() const
{
    return !m_process.isNull() && m_process->state() == QProcess::Running;
}

qint64 XrayController::getProcessId() const
{
    return m_process ? m_process->processId() : -1;
}

QString XrayController::getError() const
{
    return m_process && m_process->error() != QProcess::UnknownError ? m_process->errorString() : QString();
}

QString XrayController::getXrayExecutablePath() const
{
    QString xrayDir = QCoreApplication::applicationDirPath();
    
#if defined(Q_OS_WIN)
    return QDir(xrayDir).filePath("xray.exe");
#else
    return QDir(xrayDir).filePath("xray");
#endif
}

QStringList XrayController::getXrayArguments(const QString& configPath) const
{
    return QStringList() << "-c" << configPath << "-format=json";
} 