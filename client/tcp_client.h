#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QJsonObject>

class TcpClient : public QObject {
    Q_OBJECT
public:
    explicit TcpClient(QObject* parent = nullptr);
    
    void connectToServer(const QString& host, quint16 port);
    void disconnectFromServer();
    void sendCommand(const QString& cmd, const QJsonObject& data, int reqId);
    bool isConnected() const;

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
};