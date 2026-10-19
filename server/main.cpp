#include <QCoreApplication>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDir>
#include <QDebug>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // 1️⃣ Читаем конфиг (ищем на уровень выше папки build/)
    QString configPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../config.ini");
    QSettings settings(configPath, QSettings::IniFormat);

    quint16 tcpPort = settings.value("network/tcp_port", 9000).toUInt();
    qDebug() << "[SERVER] Config loaded from:" << configPath;
    qDebug() << "[SERVER] TCP Port:" << tcpPort;

    // 2️⃣ Инициализируем TCP-слушатель
    QTcpServer tcpServer;
    if (!tcpServer.listen(QHostAddress::Any, tcpPort)) {
        qCritical() << "[SERVER] Failed to bind port" << tcpPort << ":" << tcpServer.errorString();
        return 1;
    }
    qDebug() << "[SERVER] Listening on any interface, port" << tcpPort;

    // 3️⃣ Обработка входящих подключений
    QObject::connect(&tcpServer, &QTcpServer::newConnection, [&]() {
        QTcpSocket* client = tcpServer.nextPendingConnection();
        qDebug() << "[SERVER] New connection:" << client->peerAddress().toString() 
                 << ":" << client->peerPort();
        
        // Оставляем сокет живым (удалится вместе с сервером)
        client->setParent(&tcpServer);
        QObject::connect(client, &QTcpSocket::disconnected, client, &QTcpSocket::deleteLater);
    });

    qDebug() << "[SERVER] Entering event loop. Press Ctrl+C to stop.";
    return app.exec();
}