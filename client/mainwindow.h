#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QJsonObject>
#include "tcp_client.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(TcpClient* client, QWidget* parent = nullptr);

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onLogoutClicked();
    void onSendClicked();          // <-- ДОБАВЛЕНО (было пропущено)
    void onConnected();
    void onDisconnected();
    void onError(const QString& errorText);
    void onCommandReceived(const QString& cmd, const QJsonObject& data, int reqId);
    void onHistoryScroll(int value);

private:
    void setupLoginUI();
    void setupChatUI();
    void switchToLogin();
    void switchToChat();
    
    // <-- ОБНОВЛЕНО: 5 аргументов
    void addMessageToChat(const QString& from, const QString& text, qint64 ts, qint64 id, bool prepend = false);

    TcpClient* m_client;
    QStackedWidget* m_stack;
    
    // Экран логина
    QLineEdit* m_editLogin;
    QLineEdit* m_editPass;
    QPushButton* m_btnLogin;
    QPushButton* m_btnRegister;
    QLabel* m_lblStatus;
    
    // Экран чата
    QListWidget* m_listMessages;
    QLineEdit* m_editMessage;
    QPushButton* m_btnSend;
    
    QString m_currentUser;
    
    // Флаги для управления историей
    bool m_isLoadingHistory;
    bool m_isHistoryExhausted;
};