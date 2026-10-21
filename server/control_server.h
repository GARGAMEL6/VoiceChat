#pragma once
#include <QObject>
#include <QTcpServer>
#include <QList>
#include <QJsonObject>

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
    QTcpServer m_tcpServer;
    QList<ClientSession*> m_sessions;
    AuthManager* m_auth; // 🔹 Указатель на AuthManager
};