#pragma once
#include <QObject>
#include <QTcpServer>
#include <QList>
#include <QJsonObject>

class ClientSession;

class ControlServer : public QObject {
    Q_OBJECT
public:
    explicit ControlServer(quint16 port, QObject* parent = nullptr);
    bool isListening() const;

private slots:
    void onNewConnection();
    void onSessionClosed(ClientSession* session);

private:
    void broadcast(const QJsonObject& msg);
    QTcpServer m_tcpServer;
    QList<ClientSession*> m_sessions;
};