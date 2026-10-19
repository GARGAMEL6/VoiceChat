#include <QCoreApplication>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDir>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // 1️⃣ Чтение конфига
    QString configPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../config.ini");
    QSettings settings(configPath, QSettings::IniFormat);
    quint16 tcpPort = settings.value("network/tcp_port", 9000).toUInt();
    qDebug() << "[SERVER] Config loaded:" << configPath << "| Port:" << tcpPort;

    // 2️⃣ Запуск сервера
    QTcpServer tcpServer;
    if (!tcpServer.listen(QHostAddress::Any, tcpPort)) {
        qCritical() << "[SERVER] Bind failed:" << tcpServer.errorString();
        return 1;
    }
    qDebug() << "[SERVER] Listening on port" << tcpPort;

    // 3️⃣ Обработка подключений
    QObject::connect(&tcpServer, &QTcpServer::newConnection, [&]() {
        QTcpSocket* client = tcpServer.nextPendingConnection();
        qDebug() << "[SERVER] Connected:" << client->peerAddress().toString() 
                 << ":" << client->peerPort();
        client->setParent(&tcpServer);

        // 🔹 Буфер в КУЧЕ (живёт, пока жив client)
        QByteArray* buffer = new QByteArray();

        // 🔹 Приход новых данных
        QObject::connect(client, &QTcpSocket::readyRead, [client, buffer]() {
            buffer->append(client->readAll());
            
            int pos;
            while ((pos = buffer->indexOf('\n')) != -1) {
                QByteArray line = buffer->left(pos);
                buffer->remove(0, pos + 1);
                if (line.isEmpty()) continue;

                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(line, &err);
                if (err.error != QJsonParseError::NoError) {
                    qDebug() << "[SERVER] Invalid JSON:" << err.errorString();
                    continue;
                }

                QJsonObject req = doc.object();
                QString cmd = req["cmd"].toString();
                int reqId = req["req_id"].toInt(-1);
                qDebug() << "[SERVER] Rx | cmd:" << cmd << "| req_id:" << reqId;

                QJsonObject resp;
                resp["req_id"] = reqId;
                if (cmd == "PING") {
                    resp["cmd"] = "PONG";
                    resp["status"] = "ok";
                } else {
                    resp["cmd"] = cmd + "_ERR";
                    resp["status"] = "error";
                    resp["reason"] = "Unknown command";
                }

                QByteArray reply = QJsonDocument(resp).toJson(QJsonDocument::Compact) + '\n';
                client->write(reply);
                qDebug() << "[SERVER] Tx:" << reply.trimmed();
            }
        });

        // 🔹 Очистка при разрыве связи
        QObject::connect(client, &QTcpSocket::disconnected, [client, buffer]() {
            qDebug() << "[SERVER] Disconnected:" << client->peerAddress().toString();
            delete buffer; // Освобождаем кучу
            client->deleteLater();
        });
    });

    qDebug() << "[SERVER] Ready. Press Ctrl+C to stop.";
    return app.exec();
}