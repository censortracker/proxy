#include <QApplication>
#include <QStandardPaths>
#include "proxyserver.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Logger initialization
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    logPath += "/logs/app.log";
    Logger::getInstance().init(logPath);
    Logger::getInstance().setLogLevel(Logger::LogLevel::INFO);
    Logger::getInstance().info("Application started");
    
    app.setQuitOnLastWindowClosed(false);
    app.setWindowIcon(QIcon(":/icons/Winicon.png"));
    
    ProxyServer server;
    if (!server.start(49490)) {
        Logger::getInstance().error("Failed to start server on port 49490");
        return 1;
    }
    
    int result = app.exec();
    Logger::getInstance().info("Application shutdown");
    return result;
} 