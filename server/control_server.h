#pragma once
#include <QObject>
#include <QTcpServer>
#include <QList>
#include <QJsonObject>
#include <QMap>

class ClientSession;
class AuthManager; // Forward declaration

class ControlServer : public QObject {
    Q_OBJECT
public:
    explicit ControlServer(quint16 port, AuthManager* auth, QObject* parent = nullptr);
    bool isListening() const;

private slots:
    void onNewConnection();
    void onSessionClosed(ClientSession* session);

private:
    void broadcast(const QJsonObject& msg);
    void sendStatus(ClientSession* session, const QString& ack, 
                    int reqId, bool ok, const QString& reason = QString());

    QString loginForSession(ClientSession* session) const;
    
    QMap<QString, ClientSession*> m_onlineUsers;
    QTcpServer m_tcpServer;
    QList<ClientSession*> m_sessions;
    AuthManager* m_auth; // 🔹 Указатель на AuthManager
};