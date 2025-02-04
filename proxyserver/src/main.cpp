#include <QApplication>
#include "proxyserver.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setQuitOnLastWindowClosed(false);
    app.setWindowIcon(QIcon(":/icons/Winicon.png"));
    
    ProxyServer server;
    if (!server.start(49490)) {
        return 1;
    }
    
    return app.exec();
} 