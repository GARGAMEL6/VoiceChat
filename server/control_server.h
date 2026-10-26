#pragma once
#include <QObject>
#include <QTcpServer>
#include <QList>
#include <QJsonObject>
#include <QMap>
#include <QUdpSocket>


class ClientSession;
class AuthManager; // Forward declaration

class ControlServer : public QObject {
    Q_OBJECT
public:
    explicit ControlServer(const QString& bindIp, quint16 tcpPort, quint16 udpPort, AuthManager* auth, QObject* parent = nullptr);
    bool isListening() const;

private slots:
    void onNewConnection();
    void onSessionClosed(ClientSession* session);
    void onUdpReadyRead();  

private:
    void broadcast(const QJsonObject& msg);
    void sendStatus(ClientSession* session, const QString& ack, 
                    int reqId, bool ok, const QString& reason = QString());
    void broadcastUdp(const QByteArray& data, const QString& senderLogin);

    QString loginForSession(ClientSession* session) const;
    
    QMap<QString, ClientSession*> m_onlineUsers;
    QTcpServer m_tcpServer;
    QUdpSocket* m_udpSocket;
    QList<ClientSession*> m_sessions;
    AuthManager* m_auth; // 🔹 Указатель на AuthManager
    QMap<QString, QPair<QHostAddress, quint16>> m_voiceClients;
};