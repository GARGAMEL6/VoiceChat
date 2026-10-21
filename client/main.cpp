#include <QApplication>
#include <QTimer>
#include <QDebug>
#include "tcp_client.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    qDebug() << "[CLIENT] App started.";

    TcpClient client;

    // Подписываемся на сигналы для отладки
    QObject::connect(&client, &TcpClient::connected, []() {
        qDebug() << "[CLIENT] ✅ Connected to server!";
    });
    QObject::connect(&client, &TcpClient::disconnected, []() {
        qDebug() << "[CLIENT] ❌ Disconnected.";
    });
    QObject::connect(&client, &TcpClient::messageReceived, [](const QString& cmd, const QJsonObject& data, int reqId) {
        qDebug() << "[CLIENT] 📨 Received:" << cmd << "| Status:" << data["status"].toString();
    });

    // Подключаемся к нашему серверу
    client.connectToServer("127.0.0.1", 9000);

    // Через 1 секунду после запуска отправляем PING
    QTimer::singleShot(1000, [&client]() {
        qDebug() << "[CLIENT] Sending PING...";
        client.sendCommand("PING", {}, 1);
    });

    // Через 2 секунды отключаемся и завершаем приложение
    QTimer::singleShot(2000, [&client, &app]() {
        qDebug() << "[CLIENT] Disconnecting and exiting...";
        client.disconnectFromServer();
        app.quit(); // Завершаем Event Loop
    });

    return app.exec();
}