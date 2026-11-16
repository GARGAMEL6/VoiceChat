#include "control_server.h"
#include "client_session.h"
#include "auth_manager.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

ControlServer::ControlServer(const QString& bindIp, quint16 tcpPort, quint16 udpPort, AuthManager* auth, QObject* parent)
    : QObject(parent), m_auth(auth), m_udpSocket(new QUdpSocket(this))
{
    // 🔹 Преобразуем строку "0.0.0.0" в объект QHostAddress
    QHostAddress address(bindIp);

    // TCP
    if (!m_tcpServer.listen(address, tcpPort)) {
        qCritical() << "[CONTROL] Failed to bind TCP to" << bindIp << ":" << tcpPort << "-" << m_tcpServer.errorString();
    } else {
        qDebug() << "[CONTROL] TCP listening on" << bindIp << ":" << tcpPort;
    }

    // UDP
    if (!m_udpSocket->bind(address, udpPort)) {
        qCritical() << "[CONTROL] Failed to bind UDP to" << bindIp << ":" << udpPort << "-" << m_udpSocket->errorString();
    } else {
        qDebug() << "[CONTROL] UDP listening on" << bindIp << ":" << udpPort;
    }

    connect(&m_tcpServer, &QTcpServer::newConnection, this, &ControlServer::onNewConnection);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &ControlServer::onUdpReadyRead);
}

// ... остальное без изменений ...

bool ControlServer::isListening() const {
    return m_tcpServer.isListening();
}

void ControlServer::onNewConnection() {
    QTcpSocket* socket = m_tcpServer.nextPendingConnection();
    ClientSession* session = new ClientSession(socket, this);
    m_sessions.append(session);
    qDebug() << "[CONTROL] Client connected. Active sessions:" << m_sessions.size();

    connect(session, &ClientSession::sessionClosed, this, [this, session]() {
        // 🔹 Получаем логин пользователя, привязанного к этой сессии
        QString login = loginForSession(session);
        
        if (!login.isEmpty()) {
            // Удаляем из обоих списков: авторизованных и голосовых
            m_onlineUsers.remove(login);
            m_voiceClients.remove(login);
            qDebug() << "[CONTROL] User" << login << "disconnected, removed from online and voice lists";
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
        else if (cmd == "JOIN_VOICE") {
            QString login = loginForSession(session);
            if (login.isEmpty()) {
                sendStatus(session, "JOIN_VOICE_ERR", reqId, false, "Login required");
                return;
            }
        
            // Генерируем уникальный токен для этой сессии
            QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
            m_voiceTokens[login] = token;
            // m_voiceIps пока пуст, он зафиксируется при первом UDP REGISTER
            
            QJsonObject res;
            res["cmd"] = "JOIN_VOICE_ACK";
            res["req_id"] = reqId;
            QJsonObject data;
            data["status"] = "ok";
            data["voice_token"] = token; // Отправляем токен клиенту
            res["data"] = data;
            session->sendReply(res);
            
            qDebug() << "[CONTROL] Voice token issued for" << login;
        }
        else if (cmd == "LEAVE_VOICE") {
            QString login = loginForSession(session);
            if (login.isEmpty()) {
                sendStatus(session, "LEAVE_VOICE_ERR", reqId, false, "Login required");
                return;
            }
        
            m_voiceClients.remove(login);
            m_voiceTokens.remove(login);   // <-- ДОБАВЬ
            m_voiceIps.remove(login);      // <-- ДОБАВЬ
            qDebug() << "[CONTROL] User" << login << "left voice room";
            sendStatus(session, "LEAVE_VOICE_ACK", reqId, true);
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


void ControlServer::broadcastUdp(const QByteArray& data, const QString& senderLogin)
{
    // Рассылаем всем, КРОМЕ отправителя
    for (auto it = m_voiceClients.begin(); it != m_voiceClients.end(); ++it) {
        if (it.key() != senderLogin) {
            m_udpSocket->writeDatagram(data, it.value().first, it.value().second);
        }
    }
    qDebug() << "[CONTROL] UDP broadcast from" << senderLogin << "to" << (m_voiceClients.size() - 1) << "clients";
}


void ControlServer::onUdpReadyRead() {
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress senderAddr;
        quint16 senderPort;

        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &senderAddr, &senderPort);

        if (datagram.startsWith("REGISTER:")) {
            QString payload = QString::fromUtf8(datagram).trimmed();
            QStringList parts = payload.split(':');
            if (parts.size() < 3) {
                qWarning() << "[CONTROL] Malformed REGISTER packet from" << senderAddr;
                continue;
            }
            QString login = parts[1];
            QString token = parts[2];

            if (!m_onlineUsers.contains(login)) {
                qWarning() << "[CONTROL] REGISTER for non-online user:" << login;
                continue;
            }
            if (!m_voiceTokens.contains(login) || m_voiceTokens[login] != token) {
                qWarning() << "[CONTROL] INVALID TOKEN for" << login << "from" << senderAddr << "— spoofing attempt";
                continue;
            }

            // ПРОВЕРКА IP: если уже зафиксирован, должен совпадать
            if (m_voiceIps.contains(login)) {
                if (m_voiceIps[login] != senderAddr) {
                    qWarning() << "[CONTROL] TOKEN REPLAY ATTACK:" << login 
                               << "from" << senderAddr << "(expected" << m_voiceIps[login] << ")";
                    continue; // Блокируем атаку!
                }
            } else {
                // Первая регистрация: фиксируем реальный IP за NAT
                m_voiceIps[login] = senderAddr;
                qDebug() << "[CONTROL] IP locked for" << login << "to" << senderAddr;
            }

            m_voiceClients[login] = qMakePair(senderAddr, senderPort);
            qDebug() << "[CONTROL] UDP REGISTER OK:" << login;
        }
        else if (datagram.startsWith("UNREGISTER:")) {
            QString login = QString::fromUtf8(datagram.mid(11)).trimmed();
            if (m_voiceClients.contains(login)) {
                m_voiceClients.remove(login);
                m_voiceIps.remove(login);
                qDebug() << "[CONTROL] UDP UNREGISTER:" << login;
            }
        }
        else {
            // Это аудио-пакет. Ищем отправителя по зафиксированному IP+PORT
            QString senderLogin;
            for (auto it = m_voiceClients.begin(); it != m_voiceClients.end(); ++it) {
                if (it.value().first == senderAddr && it.value().second == senderPort) {
                    senderLogin = it.key();
                    break;
                }
            }
            if (!senderLogin.isEmpty()) {
                broadcastUdp(datagram, senderLogin);
            }
        }
    }
}