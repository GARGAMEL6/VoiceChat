#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include "tcp_client.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(TcpClient* client, QWidget* parent = nullptr);

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onSendClicked();
    void onConnected();
    void onDisconnected();
    void onError(const QString& errorText);
    void onCommandReceived(const QString& cmd, const QJsonObject& data, int reqId);

private:
    void setupLoginUI();
    void setupChatUI();
    void switchToLogin();
    void switchToChat();

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
};