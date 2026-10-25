#include "control_server.h"
#include "client_session.h"
#include "auth_manager.h" // 🔹 Подключаем
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
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
        // 🔹 Удаляем пользователя из маппинга, если он там есть
        for (auto it = m_onlineUsers.begin(); it != m_onlineUsers.end(); ++it) {
            if (it.value() == session) {
                m_onlineUsers.erase(it);
                qDebug() << "[CONTROL] Removed user from online list";
                break;
            }
        }
        onSessionClosed(session);
    });

    connect(session, &ClientSession::messageReceived, this, [this, session](const QString& cmd, const QJsonObject& data, int reqId) {
        QJsonObject resp;
        resp["req_id"] = reqId;

        if (cmd == "PING") {
            sendStatus(session, "PONG", reqId, true);
        } 
        else if (cmd == "REGISTER") {
            QString login = data["login"].toString();
            QString pass = data["pass"].toString();
            bool ok = m_auth->registerUser(login, pass);
            sendStatus(session, "REGISTER_ACK", reqId, ok, "User already exists or invalid data");
        } 
        else if (cmd == "LOGIN") {
            QString login = data["login"].toString();
            QString pass = data["pass"].toString();
            bool ok = m_auth->verifyUser(login, pass);
            
            if (ok) {
                // 🔹 Проверяем, не онлайн ли уже этот пользователь
                if (m_onlineUsers.contains(login)) {
                    ClientSession* oldSession = m_onlineUsers[login];
                    
                    // Шлём старой сессии команду KICKED
                    QJsonObject kickMsg;
                    kickMsg["cmd"] = "KICKED";
                    QJsonObject kickData;
                    kickData["reason"] = "Logged in elsewhere";
                    kickMsg["data"] = kickData;
                    oldSession->sendReply(kickMsg);
                    
                    // Закрываем старую сессию
                    oldSession->disconnectFromServer();
                    m_onlineUsers.remove(login);
                    
                    qDebug() << "[CONTROL] Kicked old session for user:" << login;
                }
                
                // Регистрируем новую сессию
                m_onlineUsers[login] = session;

                QJsonArray history = m_auth->getRecentMessages(10);
                if (!history.isEmpty()) {
                    QJsonObject historyMsg;
                    historyMsg["cmd"] = "HISTORY";
                    QJsonObject historyData;
                    historyData["messages"] = history;
                    historyData["has_more"] = (history.size() == 10);
                    historyMsg["data"] = historyData;
                    session->sendReply(historyMsg);
                    qDebug() << "[CONTROL] Sent" << history.size() << "messages to" << login;
                }

                qDebug() << "[CONTROL] User logged in:" << login;
            }
            
            sendStatus(session, "LOGIN_ACK", reqId, ok, "Invalid credentials");
        }
        else if (cmd == "CHAT") {
            // 1. Отправлять может только авторизованная сессия
            QString login = loginForSession(session);
            if (login.isEmpty()) {
                sendStatus(session, "CHAT_ERR", reqId, false, "Login required");
                return;
            }

            // 2. Валидация текста
            QString text = data.value("text").toString().trimmed();
            if (text.isEmpty()) return;
            if (text.size() > 2000) {
                sendStatus(session, "CHAT_ERR", reqId, false, "Message too long");
                return;
            }

            // 3. ОДИН timestamp и для БД, и для рассылки
            qint64 ts = QDateTime::currentMSecsSinceEpoch();
            qint64 id = m_auth->saveMessage(login, text, ts);

            if (id < 0) {
                sendStatus(session, "CHAT_ERR", reqId, false, "Failed to save message");
                return;
            }

            // 4. from — из сессии, НЕ из данных клиента
            QJsonObject payload;
            payload["id"]   = id;
            payload["from"] = login;
            payload["text"] = text;
            payload["ts"]   = ts;

            QJsonObject out;
            out["cmd"]  = "CHAT_MSG";
            out["data"] = payload;

            broadcast(out);
            qDebug() << "[CONTROL] Saved + broadcast id" << id << "from" << login;
        }
        else if (cmd == "HISTORY_REQ") {
            // Историю тоже имеет право запрашивать только вошедший пользователь
            if (loginForSession(session).isEmpty()) {
                sendStatus(session, "HISTORY_ERR", reqId, false, "Login required");
                return;
            }

            qint64 beforeId = data.value("before_id").toVariant().toLongLong();
            int limit = data.value("limit").toInt(10);
            if (limit <= 0 || limit > 100) limit = 10;

            QJsonArray messages = m_auth->getRecentMessages(limit, beforeId);

            QJsonObject payload;
            payload["messages"] = messages;
            payload["has_more"] = (messages.size() == limit);

            QJsonObject out;
            out["cmd"]    = "HISTORY_RESP";
            out["req_id"] = reqId;
            out["data"]   = payload;

            session->sendReply(out);
            qDebug() << "[CONTROL] History page:" << messages.size()
                    << "before_id:" << beforeId;
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


void ControlServer::sendStatus(ClientSession* session, const QString& ack, 
                               int reqId, bool ok, const QString& reason)
{
    // 🔹 Полезные данные ВСЕГДА в data
    QJsonObject payload;
    payload["status"] = ok ? "ok" : "error";
    if (!ok) payload["reason"] = reason;

    // 🔹 Конверт: cmd и req_id в корне
    QJsonObject resp;
    resp["cmd"]    = ack;
    resp["req_id"] = reqId;
    resp["data"]   = payload;

    session->sendReply(resp);
}


QString ControlServer::loginForSession(ClientSession* session) const
{
    for (auto it = m_onlineUsers.constBegin(); it != m_onlineUsers.constEnd(); ++it) {
        if (it.value() == session) return it.key();
    }
    return QString();   // пустая строка = сессия не авторизована
}