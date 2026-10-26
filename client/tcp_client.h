#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QJsonObject>

class TcpClient : public QObject {
    Q_OBJECT
public:
    explicit TcpClient(const QString& host, quint16 tcpPort, quint16 udpPort, QObject* parent = nullptr);
    
    void connectToServer();
    void disconnectFromServer();
    void sendCommand(const QString& cmd, const QJsonObject& data, int reqId);
    bool isConnected() const;
    
    QString getHost() const { return m_host; }
    quint16 getTcpPort() const { return m_tcpPort; }
    quint16 getUdpPort() const { return m_udpPort; }

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& errorText);
    void messageReceived(const QString& cmd, const QJsonObject& data, int reqId);

private slots:
    void onReadyRead();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket* m_socket;
    QByteArray m_buffer;
    QString m_host;
    quint16 m_tcpPort;
    quint16 m_udpPort;
};