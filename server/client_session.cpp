#include "client_session.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

ClientSession::ClientSession(QTcpSocket* socket, QObject* parent)
    : QObject(parent), m_socket(socket)
{
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientSession::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientSession::onDisconnected);
    qDebug() << "[SESSION] Created for" << m_socket->peerAddress().toString();
}

ClientSession::~ClientSession() {
    qDebug() << "[SESSION] Destroyed";
}

void ClientSession::onReadyRead() {
    m_buffer.append(m_socket->readAll());
    
    int pos;
    while ((pos = m_buffer.indexOf('\n')) != -1) {
        QByteArray line = m_buffer.left(pos);
        m_buffer.remove(0, pos + 1);
        if (line.isEmpty()) continue;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError) {
            qDebug() << "[SESSION] Invalid JSON:" << err.errorString();
            continue;
        }

        QJsonObject req = doc.object();
        QString cmd = req["cmd"].toString();
        QJsonObject data = req["data"].toObject();
        int reqId = req["req_id"].toInt(-1);

        qDebug() << "[SESSION] Rx | cmd:" << cmd << "| req_id:" << reqId;
        emit messageReceived(cmd, data, reqId);
    }
}

void ClientSession::onDisconnected() {
    qDebug() << "[SESSION] Disconnected";
    emit sessionClosed();
    deleteLater();
}

void ClientSession::sendReply(const QJsonObject& response) {
    QByteArray reply = QJsonDocument(response).toJson(QJsonDocument::Compact) + '\n';
    m_socket->write(reply);
    qDebug() << "[SESSION] Tx:" << reply.trimmed();
}