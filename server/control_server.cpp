#include "control_server.h"
#include "client_session.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QDebug>

ControlServer::ControlServer(quint16 port, QObject* parent)
    : QObject(parent)
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

    // Когда сессия закрывается, удаляем её из списка
    connect(session, &ClientSession::sessionClosed, this, [this, session]() {
        onSessionClosed(session);
    });

    // Когда сессия получает команду, маршрутизируем её
    connect(session, &ClientSession::messageReceived, this, [this, session](const QString& cmd, const QJsonObject& data, int reqId) {
        QJsonObject resp;
        resp["req_id"] = reqId;

        if (cmd == "PING") {
            resp["cmd"] = "PONG";
            resp["status"] = "ok";
            session->sendReply(resp);
        } 
        else if (cmd == "REGISTER" || cmd == "LOGIN") {
            // Эти команды обрабатываются в main.cpp через AuthManager, 
            // но для простоты оставим заглушку или передадим сигнал выше. 
            // Пока оставим базовый ответ, чтобы не усложнять.
            resp["cmd"] = cmd + "_ACK";
            resp["status"] = "ok";
            session->sendReply(resp);
        } 
        else if (cmd == "CHAT") {
            QString username = data.value("username").toString("Anonymous");
            QString text = data.value("text").toString();
            
            if (text.isEmpty()) return; // Игнорируем пустые сообщения

            // Формируем полезную нагрузку
            QJsonObject payload;
            payload["from"] = username;
            payload["text"] = text;
            payload["ts"] = QDateTime::currentMSecsSinceEpoch();

            // Оборачиваем в команду для клиентов
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