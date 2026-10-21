#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QJsonObject>

class ClientSession : public QObject {
    Q_OBJECT
public:
    explicit ClientSession(QTcpSocket* socket, QObject* parent = nullptr);
    ~ClientSession();

    void sendReply(const QJsonObject& response);
    void sendRaw(const QByteArray& data);

signals:
    void messageReceived(const QString& cmd, const QJsonObject& data, int reqId);
    void sessionClosed();

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket* m_socket;
    QByteArray m_buffer;
};