#include "auth_manager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QDebug>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

AuthManager::AuthManager(QObject* parent) : QObject(parent) {}

AuthManager::~AuthManager() {
    if (m_db.isOpen()) m_db.close();
}

bool AuthManager::init() {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    QString dbPath = QCoreApplication::applicationDirPath() + "/voice_chat.db";
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qCritical() << "[AUTH] Failed to open DB:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery query;
    if (!query.exec("CREATE TABLE IF NOT EXISTS users ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "login TEXT UNIQUE NOT NULL, "
                    "password_hash TEXT NOT NULL)")) {
        qCritical() << "[AUTH] Failed to create table:" << query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS messages ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "sender TEXT NOT NULL, "
                    "text TEXT NOT NULL, "
                    "ts INTEGER NOT NULL)")) {
        qCritical() << "[AUTH] Failed to create messages table:" << query.lastError().text();
        return false;
    }

    qDebug() << "[AUTH] Database initialized successfully";
    return true;
}

QString AuthManager::hashPassword(const QString& password) {
    QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    return hash.toHex();
}

bool AuthManager::registerUser(const QString& login, const QString& password) {
    if (login.isEmpty() || password.isEmpty()) {
        qDebug() << "[AUTH] Register failed: empty login or password";
        return false;
    }

    QSqlQuery query;
    query.prepare("INSERT INTO users (login, password_hash) VALUES (:login, :hash)");
    query.bindValue(":login", login);
    query.bindValue(":hash", hashPassword(password));

    if (!query.exec()) {
        qDebug() << "[AUTH] Register failed:" << query.lastError().text();
        return false;
    }

    qDebug() << "[AUTH] User registered:" << login;
    return true;
}

bool AuthManager::verifyUser(const QString& login, const QString& password) {
    QSqlQuery query;
    query.prepare("SELECT login FROM users WHERE login = :login AND password_hash = :hash");
    query.bindValue(":login", login);
    query.bindValue(":hash", hashPassword(password));

    if (!query.exec() || !query.next()) {
        qDebug() << "[AUTH] Login failed for:" << login;
        return false;
    }

    qDebug() << "[AUTH] Login successful:" << login;
    return true;
}


bool AuthManager::saveMessage(const QString& sender, const QString& text, qint64 ts) {
    QSqlQuery query;
    query.prepare("INSERT INTO messages (sender, text, ts) VALUES (:sender, :text, :ts)");
    query.bindValue(":sender", sender);
    query.bindValue(":text", text);
    query.bindValue(":ts", ts);

    if (!query.exec()) {
        qWarning() << "[AUTH] Failed to save message:" << query.lastError().text();
        return false;
    }
    return true;
}

QJsonArray AuthManager::getRecentMessages(int limit) {
    QSqlQuery query;
    query.prepare("SELECT sender, text, ts FROM messages ORDER BY id DESC LIMIT :limit");
    query.bindValue(":limit", limit);

    QJsonArray messages;
    if (query.exec()) {
        while (query.next()) {
            QJsonObject msg;
            msg["from"] = query.value("sender").toString();
            msg["text"] = query.value("text").toString();
            msg["ts"] = query.value("ts").toLongLong();
            messages.append(msg);
        }
    }

    // Переворачиваем массив (был от новых к старым, станет от старых к новым)
    QJsonArray reversed;
    for (int i = messages.size() - 1; i >= 0; --i) {
        reversed.append(messages[i]);
    }
    return reversed;
}