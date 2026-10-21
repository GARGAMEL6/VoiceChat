#include <QApplication>
#include "tcp_client.h"
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    TcpClient client;
    MainWindow window(&client);
    window.show();

    return app.exec();
}