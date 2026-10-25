#pragma once
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QJsonArray> 

class AuthManager : public QObject {
    Q_OBJECT
public:
    explicit AuthManager(QObject* parent = nullptr);
    ~AuthManager();

    bool init(); // Инициализация БД
    bool registerUser(const QString& login, const QString& password);
    bool verifyUser(const QString& login, const QString& password);

    qint64 saveMessage(const QString& sender, const QString& text, qint64 ts);
    QJsonArray getRecentMessages(int limit = 10, qint64 beforeId = -1);

private:
    QSqlDatabase m_db;
    QString hashPassword(const QString& password);
};