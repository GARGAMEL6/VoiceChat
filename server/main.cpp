#include <QCoreApplication>
#include <QSettings>
#include <QDir>
#include <QDebug>
#include "auth_manager.h"
#include "control_server.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // 1. Чтение конфига
    QString configPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../config.ini");
    QSettings settings(configPath, QSettings::IniFormat);
    quint16 tcpPort = settings.value("network/tcp_port", 9000).toUInt();
    qDebug() << "[SERVER] Config loaded:" << configPath << "| Port:" << tcpPort;

    // 2. Инициализация БД
    AuthManager auth;
    if (!auth.init()) {
        qCritical() << "[SERVER] AuthManager init failed. Exiting.";
        return 1;
    }

    // 3. Запуск сетевого контроллера
    ControlServer server(tcpPort);
    if (!server.isListening()) {
        return 1;
    }

    qDebug() << "[SERVER] Ready. Press Ctrl+C to stop.";
    return app.exec();
}