#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QComboBox>       // <-- ДОБАВЛЕНО
#include <QJsonObject>
#include "tcp_client.h"
#include "audio_manager.h" // <-- ДОБАВЛЕНО

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(TcpClient* client, QWidget* parent = nullptr);

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onLogoutClicked();
    void onSendClicked();
    void onConnected();
    void onDisconnected();
    void onError(const QString& errorText);
    void onCommandReceived(const QString& cmd, const QJsonObject& data, int reqId);
    void onHistoryScroll(int value);
    
    // 🔹 Новые слоты для аудио
    void onStartAudioTest();
    void onStopAudioTest();

private:
    void setupLoginUI();
    void setupChatUI();
    void switchToLogin();
    void switchToChat();
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
    
    // 🔹 Элементы управления звуком
    QComboBox* m_comboMic;
    QComboBox* m_comboSpeaker;
    QPushButton* m_btnStartAudio;
    QPushButton* m_btnStopAudio;
    
    QString m_currentUser;
    bool m_isLoadingHistory;
    bool m_isHistoryExhausted;
    
    AudioManager* m_audioManager; // <-- ДОБАВЛЕНО
};