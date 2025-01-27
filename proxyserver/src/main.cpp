#include <QApplication>
#include "proxyserver.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setQuitOnLastWindowClosed(false);
    
    ProxyServer server;
    if (!server.start(8080)) {
        return 1;
    }
    
    return app.exec();
} 