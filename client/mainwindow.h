#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QComboBox>
#include <QUdpSocket>       // <-- ДОБАВЛЕНО
#include <QJsonObject>
#include <QSlider> 
#include "tcp_client.h"
#include "audio_manager.h"

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
    
    // 🔹 Слоты для голосовой комнаты
    void onJoinVoiceRoom();
    void onLeaveVoiceRoom();
    void onAudioCaptured();   // Микрофон -> UDP
    void onUdpAudioReceived();// UDP -> Динамик
    
    void onMicChanged(int index);
    void onSpeakerChanged(int index);
    void onMuteToggled(bool checked);
    void onVolumeChanged(int value);
    void onAudioDevicesChanged();
    

private:
    void setupLoginUI();
    void setupChatUI();
    void switchToLogin();
    void switchToChat();
    void addMessageToChat(const QString& from, const QString& text, qint64 ts, qint64 id, bool prepend = false);

    TcpClient* m_client;
    QStackedWidget* m_stack;
    
    QLineEdit* m_editLogin;
    QLineEdit* m_editPass;
    QPushButton* m_btnLogin;
    QPushButton* m_btnRegister;
    QLabel* m_lblStatus;
    
    QListWidget* m_listMessages;
    QLineEdit* m_editMessage;
    QPushButton* m_btnSend;
    
    QComboBox* m_comboMic;
    QComboBox* m_comboSpeaker;
    QPushButton* m_btnVoiceRoom; // <-- ЗАМЕНИЛ ДВЕ КНОПКИ НА ОДНУ
    
    QString m_currentUser;
    bool m_isLoadingHistory;
    bool m_isHistoryExhausted;
    
    AudioManager* m_audioManager;
    
    // 🔹 Сетевые переменные для голоса
    QUdpSocket* m_udpSocket;
    bool m_inVoiceRoom;
    quint16 m_localUdpPort;
    
    QPushButton* m_btnMute;
    QSlider* m_volumeSlider;
    bool m_isMuted;
    
    QMediaDevices m_mediaDevices; 
};