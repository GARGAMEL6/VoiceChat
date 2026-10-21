#include <QCoreApplication>
#include <QSettings>
#include <QTcpServer>
#include <QDir>
#include <QDebug>
#include <QJsonObject>
#include "client_session.h"
#include "auth_manager.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QString configPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../config.ini");
    QSettings settings(configPath, QSettings::IniFormat);
    quint16 tcpPort = settings.value("network/tcp_port", 9000).toUInt();
    qDebug() << "[SERVER] Config loaded:" << configPath << "| Port:" << tcpPort;

    // 🔹 Инициализация БД
    AuthManager auth;
    if (!auth.init()) {
        qCritical() << "[SERVER] AuthManager init failed. Exiting.";
        return 1;
    }

    QTcpServer tcpServer;
    if (!tcpServer.listen(QHostAddress::Any, tcpPort)) {
        qCritical() << "[SERVER] Bind failed:" << tcpServer.errorString();
        return 1;
    }
    qDebug() << "[SERVER] Listening on port" << tcpPort;

    QObject::connect(&tcpServer, &QTcpServer::newConnection, [&]() {
        QTcpSocket* socket = tcpServer.nextPendingConnection();
        ClientSession* session = new ClientSession(socket, &tcpServer);

        QObject::connect(session, &ClientSession::messageReceived, 
            [session, &auth](const QString& cmd, const QJsonObject& data, int reqId) {
                QJsonObject resp;
                resp["req_id"] = reqId;

                if (cmd == "PING") {
                    resp["cmd"] = "PONG";
                    resp["status"] = "ok";
                } 
                else if (cmd == "REGISTER") {
                    QString login = data["login"].toString();
                    QString pass = data["pass"].toString();
                    bool ok = auth.registerUser(login, pass);
                    resp["cmd"] = "REGISTER_ACK";
                    resp["status"] = ok ? "ok" : "error";
                    resp["reason"] = ok ? "" : "User already exists or invalid data";
                } 
                else if (cmd == "LOGIN") {
                    QString login = data["login"].toString();
                    QString pass = data["pass"].toString();
                    bool ok = auth.verifyUser(login, pass);
                    resp["cmd"] = "LOGIN_ACK";
                    resp["status"] = ok ? "ok" : "error";
                    resp["reason"] = ok ? "" : "Invalid credentials";
                } 
                else {
                    resp["cmd"] = cmd + "_ERR";
                    resp["status"] = "error";
                    resp["reason"] = "Unknown command";
                }

                session->sendReply(resp);
            });

        QObject::connect(session, &ClientSession::sessionClosed, []() {
            qDebug() << "[SERVER] Session closed";
        });
    });

    qDebug() << "[SERVER] Ready. Press Ctrl+C to stop.";
    return app.exec();
}