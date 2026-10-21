#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>
#include <QTimer>

MainWindow::MainWindow(TcpClient* client, QWidget* parent)
    : QMainWindow(parent), m_client(client), m_stack(new QStackedWidget(this))
{
    setCentralWidget(m_stack);
    setWindowTitle("Voice Chat");
    resize(400, 350);

    setupLoginUI();
    setupChatUI();

    connect(m_client, &TcpClient::connected, this, &MainWindow::onConnected);
    connect(m_client, &TcpClient::disconnected, this, &MainWindow::onDisconnected);
    connect(m_client, &TcpClient::errorOccurred, this, &MainWindow::onError);
    connect(m_client, &TcpClient::messageReceived, this, &MainWindow::onCommandReceived);

    switchToLogin();
}

void MainWindow::setupLoginUI() {
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->addWidget(new QLabel("Логин:"));
    m_editLogin = new QLineEdit;
    layout->addWidget(m_editLogin);

    layout->addWidget(new QLabel("Пароль:"));
    m_editPass = new QLineEdit;
    m_editPass->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_editPass);

    m_btnLogin = new QPushButton("Войти");
    layout->addWidget(m_btnLogin);

    m_btnRegister = new QPushButton("Зарегистрироваться");
    layout->addWidget(m_btnRegister);

    m_lblStatus = new QLabel;
    m_lblStatus->setStyleSheet("color: red;");
    layout->addWidget(m_lblStatus);

    layout->addStretch();

    connect(m_btnLogin, &QPushButton::clicked, this, &MainWindow::onLoginClicked);
    connect(m_btnRegister, &QPushButton::clicked, this, &MainWindow::onRegisterClicked);

    m_stack->addWidget(page);
}

void MainWindow::setupChatUI() {
    QWidget* page = new QWidget;
    QVBoxLayout* mainLayout = new QVBoxLayout(page);

    // 🔹 Верхняя панель с кнопкой выхода
    QHBoxLayout* topBar = new QHBoxLayout;
    QLabel* lblTitle = new QLabel("Чат");
    lblTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    topBar->addWidget(lblTitle);
    topBar->addStretch();
    
    QPushButton* btnLogout = new QPushButton("Выйти");
    btnLogout->setStyleSheet("padding: 4px 12px;");
    topBar->addWidget(btnLogout);
    
    mainLayout->addLayout(topBar);

    // Список сообщений
    m_listMessages = new QListWidget;
    m_listMessages->setSelectionMode(QAbstractItemView::NoSelection);
    mainLayout->addWidget(m_listMessages);

    // Поле ввода
    QHBoxLayout* inputLayout = new QHBoxLayout;
    m_editMessage = new QLineEdit;
    m_editMessage->setPlaceholderText("Введите сообщение...");
    inputLayout->addWidget(m_editMessage);

    m_btnSend = new QPushButton("Отправить");
    inputLayout->addWidget(m_btnSend);

    mainLayout->addLayout(inputLayout);

    connect(m_btnSend, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(m_editMessage, &QLineEdit::returnPressed, this, &MainWindow::onSendClicked);
    connect(btnLogout, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);

    m_stack->addWidget(page);
}

void MainWindow::switchToLogin() {
    m_stack->setCurrentIndex(0);
}

void MainWindow::switchToChat() {
    m_stack->setCurrentIndex(1);
}

void MainWindow::onLoginClicked() {
    QString login = m_editLogin->text().trimmed();
    QString pass = m_editPass->text();

    if (login.isEmpty() || pass.isEmpty()) {
        m_lblStatus->setText("Заполните все поля");
        return;
    }

    m_lblStatus->setStyleSheet("color: gray;");
    m_lblStatus->setText("Подключение...");
    
    // 🔹 Проверяем, не подключены ли уже
    if (!m_client->isConnected()) {
        m_client->connectToServer("127.0.0.1", 9000);
    }

    // Отправляем LOGIN с задержкой (даём время на подключение)
    QTimer::singleShot(500, [this, login, pass]() {
        QJsonObject data;
        data["login"] = login;
        data["pass"] = pass;
        m_client->sendCommand("LOGIN", data, 1);
    });
}

void MainWindow::onRegisterClicked() {
    QString login = m_editLogin->text().trimmed();
    QString pass = m_editPass->text();

    if (login.isEmpty() || pass.isEmpty()) {
        m_lblStatus->setText("Заполните все поля");
        return;
    }

    m_lblStatus->setStyleSheet("color: gray;");
    m_lblStatus->setText("Подключение...");
    
    if (!m_client->isConnected()) {
        m_client->connectToServer("127.0.0.1", 9000);
    }

    QTimer::singleShot(500, [this, login, pass]() {
        QJsonObject data;
        data["login"] = login;
        data["pass"] = pass;
        m_client->sendCommand("REGISTER", data, 1);
    });
}

void MainWindow::onSendClicked() {
    QString text = m_editMessage->text().trimmed();
    if (text.isEmpty()) return;

    QJsonObject data;
    data["username"] = m_currentUser;
    data["text"] = text;
    m_client->sendCommand("CHAT", data, 0);

    m_editMessage->clear();
}

void MainWindow::onConnected() {
    qDebug() << "[GUI] Connected to server";
    m_lblStatus->setText("Подключено. Авторизация...");
}

void MainWindow::onDisconnected() {
    qDebug() << "[GUI] Disconnected";
    m_lblStatus->setText("Отключено от сервера");
    switchToLogin();
}

void MainWindow::onError(const QString& errorText) {
    qWarning() << "[GUI] Error:" << errorText;
    m_lblStatus->setText("Ошибка: " + errorText);
}

void MainWindow::onCommandReceived(const QString& cmd, const QJsonObject& data, int reqId) {
    qDebug() << "[GUI] Received:" << cmd;

    if (cmd == "KICKED") {
        QString reason = data["reason"].toString();
        QMessageBox::warning(this, "Выход", "Вы вышли из системы: " + reason);
        m_client->disconnectFromServer();
        switchToLogin();
        m_lblStatus->setText("Вы вышли из системы");
        return;
    }
    
    if (cmd == "REGISTER_ACK") {
        QString status = data["status"].toString();
        if (status == "ok") {
            m_lblStatus->setText("Регистрация успешна! Теперь войдите.");
            m_lblStatus->setStyleSheet("color: green;");
        } else {
            QString reason = data["reason"].toString();
            m_lblStatus->setText("Ошибка регистрации: " + reason);
            m_lblStatus->setStyleSheet("color: red;");
        }
    }
    else if (cmd == "LOGIN_ACK") {
        QString status = data["status"].toString();
        if (status == "ok") {
            m_currentUser = m_editLogin->text().trimmed();
            m_lblStatus->clear();
            m_lblStatus->setStyleSheet("color: gray;");
            switchToChat();
            m_listMessages->addItem("[Система] Вы вошли как " + m_currentUser);
        } else {
            QString reason = data["reason"].toString();
            m_lblStatus->setText("Ошибка входа: " + reason);
            m_lblStatus->setStyleSheet("color: red;");
        }
    }
    else if (cmd == "CHAT_MSG") {
        QString from = data.value("from").toString("Unknown");
        QString text = data["text"].toString();
        
        qint64 ts = data["ts"].toVariant().toLongLong();
        QString time = QDateTime::fromMSecsSinceEpoch(ts).toString("HH:mm");

        QString displayText = QString("[%1] %2: %3").arg(time, from, text);
        m_listMessages->addItem(displayText);
        m_listMessages->scrollToBottom();
    }
}


void MainWindow::onLogoutClicked() {
    qDebug() << "[GUI] Logout clicked";
    
    // Отключаемся от сервера
    m_client->disconnectFromServer();
    
    // Очищаем данные пользователя
    m_currentUser.clear();
    m_listMessages->clear();
    m_editMessage->clear();
    
    // Возвращаемся на экран логина
    switchToLogin();
    m_lblStatus->setText("Вы вышли из системы");
    m_lblStatus->setStyleSheet("color: gray;");
}