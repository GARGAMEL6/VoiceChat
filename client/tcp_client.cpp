#include "tcp_client.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

TcpClient::TcpClient(QObject* parent) : QObject(parent), m_socket(new QTcpSocket(this)) {
    // Подписываемся на события сокета
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &TcpClient::onError);
}

void TcpClient::connectToServer(const QString& host, quint16 port) {
    m_socket->connectToHost(host, port);
}

void TcpClient::disconnectFromServer() {
    m_socket->disconnectFromHost();
}

bool TcpClient::isConnected() const {
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void TcpClient::sendCommand(const QString& cmd, const QJsonObject& data, int reqId) {
    if (!isConnected()) {
        qWarning() << "[TCP_CLIENT] Cannot send: not connected.";
        return;
    }

    QJsonObject msg;
    msg["cmd"] = cmd;
    msg["data"] = data;
    msg["req_id"] = reqId;

    QByteArray json = QJsonDocument(msg).toJson(QJsonDocument::Compact) + '\n';
    m_socket->write(json);
    qDebug() << "[TCP_CLIENT] Tx ->" << json.trimmed();
}

void TcpClient::onReadyRead() {
    m_buffer.append(m_socket->readAll());

    int pos;
    while ((pos = m_buffer.indexOf('\n')) != -1) {
        QByteArray line = m_buffer.left(pos);
        m_buffer.remove(0, pos + 1); // Удаляем обработанную часть + \n
        if (line.isEmpty()) continue;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError) {
            qWarning() << "[TCP_CLIENT] Invalid JSON:" << err.errorString();
            continue;
        }

        QJsonObject obj = doc.object();
        QString cmd = obj["cmd"].toString();
        QJsonObject data = obj["data"].toObject();
        int reqId = obj["req_id"].toInt(-1);

        qDebug() << "[TCP_CLIENT] Rx <- cmd:" << cmd << "| req_id:" << reqId;
        emit messageReceived(cmd, data, reqId);
    }
}

void TcpClient::onDisconnected() {
    qDebug() << "[TCP_CLIENT] Disconnected from server.";
    emit disconnected();
}

void TcpClient::onError(QAbstractSocket::SocketError socketError) {
    QString errorText = m_socket->errorString();
    qWarning() << "[TCP_CLIENT] Socket error:" << errorText;
    emit errorOccurred(errorText);
}