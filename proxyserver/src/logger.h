#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QDateTime>
#include <QMutex>
#include <QString>

class Logger : public QObject
{
    Q_OBJECT

public:
    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    static Logger& getInstance();

    void init(const QString& logPath, qint64 maxFileSize = 1024 * 1024 * 10); // 10MB by default
    void setLogLevel(LogLevel level);

    // Main logging method
    void log(LogLevel level, const QString& message);

    // Helper methods for convenience
    void debug(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);

private:
    Logger(QObject* parent = nullptr);
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void logInternal(LogLevel level, const QString& message);
    void checkRotation();
    QString levelToString(LogLevel level);
    bool openLogFile(QFile& file);
    qint64 getCurrentFileSize() const;

    QString m_logPath;
    qint64 m_maxFileSize;
    LogLevel m_currentLevel;
    QMutex m_mutex;
    static const int MAX_BACKUP_FILES = 3;
};

#endif // LOGGER_H 