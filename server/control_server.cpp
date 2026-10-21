#include "control_server.h"
#include "client_session.h"
#include "auth_manager.h" // 🔹 Подключаем
#include <QDateTime>
#include <QJsonDocument>
#include <QDebug>

ControlServer::ControlServer(quint16 port, AuthManager* auth, QObject* parent)
    : QObject(parent), m_auth(auth) // 🔹 Сохраняем указатель
{
    if (!m_tcpServer.listen(QHostAddress::Any, port)) {
        qCritical() << "[CONTROL] Failed to bind port" << port << ":" << m_tcpServer.errorString();
    } else {
        qDebug() << "[CONTROL] Listening on port" << port;
    }

    connect(&m_tcpServer, &QTcpServer::newConnection, this, &ControlServer::onNewConnection);
}

bool ControlServer::isListening() const {
    return m_tcpServer.isListening();
}

void ControlServer::onNewConnection() {
    QTcpSocket* socket = m_tcpServer.nextPendingConnection();
    ClientSession* session = new ClientSession(socket, this);
    m_sessions.append(session);
    qDebug() << "[CONTROL] Client connected. Active sessions:" << m_sessions.size();

    connect(session, &ClientSession::sessionClosed, this, [this, session]() {
        onSessionClosed(session);
    });

    connect(session, &ClientSession::messageReceived, this, [this, session](const QString& cmd, const QJsonObject& data, int reqId) {
        QJsonObject resp;
        resp["req_id"] = reqId;

        if (cmd == "PING") {
            resp["cmd"] = "PONG";
            resp["status"] = "ok";
            session->sendReply(resp);
        } 
        else if (cmd == "REGISTER") {
            QString login = data["login"].toString();
            QString pass = data["pass"].toString();
            bool ok = m_auth->registerUser(login, pass); // 🔹 Вызываем AuthManager
            resp["cmd"] = "REGISTER_ACK";
            resp["status"] = ok ? "ok" : "error";
            resp["reason"] = ok ? "" : "User already exists or invalid data";
            session->sendReply(resp);
        } 
        else if (cmd == "LOGIN") {
            QString login = data["login"].toString();
            QString pass = data["pass"].toString();
            bool ok = m_auth->verifyUser(login, pass); // 🔹 Вызываем AuthManager
            resp["cmd"] = "LOGIN_ACK";
            resp["status"] = ok ? "ok" : "error";
            resp["reason"] = ok ? "" : "Invalid credentials";
            session->sendReply(resp);
        } 
        else if (cmd == "CHAT") {
            QString username = data.value("username").toString("Anonymous");
            QString text = data.value("text").toString();
            
            if (text.isEmpty()) return;

            QJsonObject payload;
            payload["from"] = username;
            payload["text"] = text;
            payload["ts"] = QDateTime::currentMSecsSinceEpoch();

            QJsonObject broadcastMsg;
            broadcastMsg["cmd"] = "CHAT_MSG";
            broadcastMsg["data"] = payload;

            qDebug() << "[CONTROL] Broadcasting CHAT_MSG from" << username;
            broadcast(broadcastMsg);
        } 
        else {
            resp["cmd"] = cmd + "_ERR";
            resp["status"] = "error";
            resp["reason"] = "Unknown command";
            session->sendReply(resp);
        }
    });
}

void ControlServer::onSessionClosed(ClientSession* session) {
    m_sessions.removeAll(session);
    qDebug() << "[CONTROL] Client disconnected. Active sessions:" << m_sessions.size();
}

void ControlServer::broadcast(const QJsonObject& msg) {
    QByteArray raw = QJsonDocument(msg).toJson(QJsonDocument::Compact) + '\n';
    int sentCount = 0;
    for (ClientSession* session : m_sessions) {
        if (session) {
            session->sendRaw(raw);
            sentCount++;
        }
    }
    qDebug() << "[CONTROL] Broadcast complete. Sent to" << sentCount << "clients.";
}