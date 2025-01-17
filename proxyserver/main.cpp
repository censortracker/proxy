#include <QCoreApplication>
#include "proxyserver.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    ProxyServer server;
    if (!server.start(8080)) {
        return 1;
    }
    
    return app.exec();
} 