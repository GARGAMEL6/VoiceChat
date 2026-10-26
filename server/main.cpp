#include <QCoreApplication>
#include <QSettings>
#include <QDir>
#include <QDebug>
#include "auth_manager.h"
#include "control_server.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QString configPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../config.ini");
    QSettings settings(configPath, QSettings::IniFormat);
    quint16 tcpPort = settings.value("network/tcp_port", 9000).toUInt();
    quint16 udpPort = settings.value("network/udp_port", 9001).toUInt();
    qDebug() << "[SERVER] Config loaded:" << configPath << "| Port:" << tcpPort;

    AuthManager auth;
    if (!auth.init()) {
        qCritical() << "[SERVER] AuthManager init failed. Exiting.";
        return 1;
    }

    // 🔹 Передаём auth в ControlServer
    ControlServer server(tcpPort, udpPort, &auth);
    if (!server.isListening()) {
        return 1;
    }

    qDebug() << "[SERVER] Ready. Press Ctrl+C to stop.";
    return app.exec();
}