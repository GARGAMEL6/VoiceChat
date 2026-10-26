#include <QCoreApplication>
#include <QSettings>
#include <QDir>
#include <QDebug>
#include "auth_manager.h"
#include "control_server.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QString configPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/config.ini");
    QSettings settings(configPath, QSettings::IniFormat);
    
    //  Читаем IP, порты и имя БД
    QString bindIp = settings.value("network/bind_ip", "0.0.0.0").toString();
    quint16 tcpPort = settings.value("network/tcp_port", 9000).toUInt();
    quint16 udpPort = settings.value("network/udp_port", 9001).toUInt();
    QString dbName = settings.value("database/db_name", "voice_chat.db").toString();

    qDebug() << "[SERVER] Config loaded:" << configPath;
    qDebug() << "[SERVER] Bind IP:" << bindIp << "| TCP:" << tcpPort << "| UDP:" << udpPort;

    AuthManager auth;
    if (!auth.init()) { // В идеале сюда тоже можно передать путь к БД, но пока оставим как есть
        qCritical() << "[SERVER] AuthManager init failed. Exiting.";
        return 1;
    }

    // 🔹 Передаем bindIp в сервер
    ControlServer server(bindIp, tcpPort, udpPort, &auth);
    if (!server.isListening()) {
        return 1;
    }

    qDebug() << "[SERVER] Ready. Press Ctrl+C to stop.";
    return app.exec();
}