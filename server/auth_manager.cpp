#include "auth_manager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QDebug>
#include <QCoreApplication>

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