#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>
#include <QTimer>
#include <QJsonArray>
#include <QScrollBar>

MainWindow::MainWindow(TcpClient* client, QWidget* parent)
    : QMainWindow(parent), m_client(client), m_stack(new QStackedWidget(this)),
      m_isLoadingHistory(false), m_isHistoryExhausted(false),
      m_audioManager(new AudioManager(this)),
      m_udpSocket(new QUdpSocket(this)), // <-- ДОБАВЛЕНО
      m_inVoiceRoom(false),              // <-- ДОБАВЛЕНО
      m_localUdpPort(0),
      m_isMuted(false) // <-- ДОБАВЛЕНО
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
    
    connect(&m_mediaDevices, &QMediaDevices::audioInputsChanged, this, &MainWindow::onAudioDevicesChanged);
    connect(&m_mediaDevices, &QMediaDevices::audioOutputsChanged, this, &MainWindow::onAudioDevicesChanged);
    // ...
    
    

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
    
        // 🔹 Панель управления голосом
        // 🔹 Панель управления голосом
    QHBoxLayout* audioLayout = new QHBoxLayout;
    
    m_comboMic = new QComboBox;
    m_comboMic->addItems(m_audioManager->getMicrophones());
    audioLayout->addWidget(new QLabel("Микрофон:"));
    audioLayout->addWidget(m_comboMic, 1);
    connect(m_comboMic, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onMicChanged);
    
    m_comboSpeaker = new QComboBox;
    m_comboSpeaker->addItems(m_audioManager->getSpeakers());
    audioLayout->addWidget(new QLabel("Динамик:"));
    audioLayout->addWidget(m_comboSpeaker, 1);
    connect(m_comboSpeaker, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onSpeakerChanged);
    
    // Кнопка Mute
    m_btnMute = new QPushButton("Mute");
    m_btnMute->setCheckable(true);
    m_btnMute->setEnabled(false); // Неактивна, пока не в комнате
    audioLayout->addWidget(m_btnMute);
    connect(m_btnMute, &QPushButton::toggled, this, &MainWindow::onMuteToggled);
    
    // Ползунок громкости
    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(80);
    m_volumeSlider->setEnabled(false);
    audioLayout->addWidget(new QLabel("Громкость:"));
    audioLayout->addWidget(m_volumeSlider, 1);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    
    // Кнопка входа
    m_btnVoiceRoom = new QPushButton("🎙️ Войти в комнату");
    m_btnVoiceRoom->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
    audioLayout->addWidget(m_btnVoiceRoom);
    
    mainLayout->addLayout(audioLayout);
    connect(m_btnVoiceRoom, &QPushButton::clicked, this, &MainWindow::onJoinVoiceRoom);

    mainLayout->addLayout(inputLayout);

    connect(m_btnSend, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(m_editMessage, &QLineEdit::returnPressed, this, &MainWindow::onSendClicked);
    connect(btnLogout, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
    connect(m_listMessages->verticalScrollBar(), &QScrollBar::valueChanged, 
            this, &MainWindow::onHistoryScroll);

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

    m_currentUser = login;

    m_lblStatus->setStyleSheet("color: gray;");
    m_lblStatus->setText("Подключение...");
    
    // 🔹 Проверяем, не подключены ли уже
    if (!m_client->isConnected()) {
        m_client->connectToServer();
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
        m_client->connectToServer();
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
    else if (cmd == "HISTORY") {
        // Это push-сообщение при входе. Очищаем список и грузим начальные 10.
        m_listMessages->clear();
        QJsonArray messages = data["messages"].toArray();
        for (const QJsonValue& val : messages) {
            QJsonObject msg = val.toObject();
            addMessageToChat(msg["from"].toString(), msg["text"].toString(),
                             msg["ts"].toVariant().toLongLong(),
                             msg["id"].toVariant().toLongLong(),
                             false); // false = добавляем в конец (они уже отсортированы сервером)
        }
        m_isHistoryExhausted = !data["has_more"].toBool();
    }
    else if (cmd == "HISTORY_RESP") {
        // Это ответ на запрос при скролле вверх
        QJsonArray messages = data["messages"].toArray();
        bool hasMore = data["has_more"].toBool();

        if (messages.isEmpty()) {
            m_isHistoryExhausted = true;
            m_isLoadingHistory = false;
            return;
        }

        // 🔹 МАГИЯ БЕЗ ПРЫЖКОВ: запоминаем текущий верхний элемент
        QListWidgetItem* oldTopItem = m_listMessages->item(0);

        // Вставляем новые (более старые) сообщения НАВЕРХ
        for (const QJsonValue& val : messages) {
            QJsonObject msg = val.toObject();
            addMessageToChat(msg["from"].toString(), msg["text"].toString(),
                             msg["ts"].toVariant().toLongLong(),
                             msg["id"].toVariant().toLongLong(),
                             true); // true = вставляем в начало (prepend)
        }

        m_isHistoryExhausted = !hasMore;
        m_isLoadingHistory = false;

        // 🔹 ВОССТАНАВЛИВАЕМ позицию скролла, чтобы экран не дернулся
        if (oldTopItem) {
            m_listMessages->scrollToItem(oldTopItem, QAbstractItemView::PositionAtTop);
        }
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
            // 🔹 БЕРИ ЛОГИН ИЗ ПОЛЯ ВВОДА, НО ТОЛЬКО ЕСЛИ ОНО НЕ ПУСТОЕ
            // ИЛИ лучше: сервер должен возвращать логин в ответе
            QString login = m_editLogin->text().trimmed();
            if (!login.isEmpty()) {
                m_currentUser = login;
            }
            
            m_lblStatus->clear();
            m_lblStatus->setStyleSheet("color: gray;");
            switchToChat();
            
            // Системное сообщение
            QListWidgetItem* sysItem = new QListWidgetItem(
                "[Система] Вы вошли как " + m_currentUser, m_listMessages);
            sysItem->setBackground(QColor(220, 220, 255));
            sysItem->setForeground(QColor(0, 0, 100));
            QFont sysFont = sysItem->font();
            sysFont.setItalic(true);
            sysItem->setFont(sysFont);
            m_listMessages->addItem(sysItem);
            m_listMessages->scrollToBottom();
        } else {
            m_currentUser.clear();
            
            QString reason = data["reason"].toString();
            m_lblStatus->setText("Ошибка входа: " + reason);
            m_lblStatus->setStyleSheet("color: red;");
        }
    }
    else if (cmd == "CHAT_MSG") {
        QString from = data.value("from").toString("Unknown");
        QString text = data["text"].toString();
        qint64 ts = data["ts"].toVariant().toLongLong();
        qint64 id = data["id"].toVariant().toLongLong(); // <-- ДОБАВЛЕНО: извлекаем id
        
        // <-- ОБНОВЛЕНО: передаем 5 аргументов, false = добавляем в конец
        addMessageToChat(from, text, ts, id, false);
    }
    else if (cmd == "JOIN_VOICE_ACK") {
        if (data["status"].toString() == "ok") {
            m_voiceToken = data["voice_token"].toString();
            
            // Сразу отправляем UDP REGISTER с токеном
            QByteArray registerPacket = "REGISTER:" + m_currentUser.toUtf8() + ":" + m_voiceToken.toUtf8() + "\n";
            m_udpSocket->writeDatagram(registerPacket, QHostAddress(m_client->getHost()), m_client->getUdpPort());
            qDebug() << "[VOICE] Sent UDP REGISTER with token";
        } else {
            qWarning() << "[VOICE] JOIN failed:" << data["reason"].toString();
        }
    }
}


void MainWindow::onLogoutClicked() {
    qDebug() << "[GUI] Logout clicked";
    
    // Отключаемся от сервера
    m_client->disconnectFromServer();
    
    // Очищаем данные пользователя
    m_currentUser.clear();
    m_editLogin->clear();
    m_editPass->clear();

    m_listMessages->clear();
    m_editMessage->clear();
    
    // Возвращаемся на экран логина
    switchToLogin();
    m_lblStatus->setText("Вы вышли из системы");
    m_lblStatus->setStyleSheet("color: gray;");
}


void MainWindow::addMessageToChat(const QString& from, const QString& text, qint64 ts, qint64 id, bool prepend) {
    QString time = QDateTime::fromMSecsSinceEpoch(ts).toString("HH:mm");

    // 🔹 ОТЛАДКА: выведи в консоль
    qDebug() << "[GUI] Adding msg | from:" << from << "| currentUser:" << m_currentUser 
             << "| isMine:" << (from == m_currentUser);
    
    
    bool isMine = (from == m_currentUser);
    QString displayFrom = isMine ? "Вы" : from;
    QString displayText = QString("[%1] %2: %3").arg(time, displayFrom, text);
    
    QListWidgetItem* item = new QListWidgetItem(displayText);
    
    // 🔹 КЛЮЧЕВОЙ МОМЕНТ: сохраняем ID сообщения в данные элемента
    item->setData(Qt::UserRole, id);
    
    if (isMine) {
        item->setBackground(QColor(200, 230, 200));
        item->setForeground(QColor(0, 80, 0));
    } else {
        item->setBackground(QColor(240, 240, 240));
        item->setForeground(QColor(0, 0, 0));
    }
    
    if (prepend) {
        // Вставляем в начало (для подгружаемой истории)
        m_listMessages->insertItem(0, item);
    } else {
        // Добавляем в конец (для новых сообщений и начальной загрузки)
        m_listMessages->addItem(item);
        m_listMessages->scrollToBottom();
    }
}


void MainWindow::onHistoryScroll(int value) {
    Q_UNUSED(value);
    
    // Если уже грузим или история кончилась — ничего не делаем
    if (m_isLoadingHistory || m_isHistoryExhausted) return;

    QScrollBar* scrollBar = m_listMessages->verticalScrollBar();
    
    // Триггер: пользователь прокрутил почти до самого верха (с запасом 100 пикселей)
    if (scrollBar->value() <= scrollBar->minimum() + 100) {
        m_isLoadingHistory = true;
        qDebug() << "[GUI] Triggered history load...";

        // Берем ID самого верхнего сообщения как курсор
        qint64 beforeId = -1;
        QListWidgetItem* topItem = m_listMessages->item(0);
        if (topItem) {
            beforeId = topItem->data(Qt::UserRole).toLongLong();
        }

        QJsonObject data;
        data["before_id"] = beforeId;
        data["limit"] = 10;
        m_client->sendCommand("HISTORY_REQ", data, 0);
    }
}


void MainWindow::onJoinVoiceRoom() {
    if (m_inVoiceRoom) {
        onLeaveVoiceRoom();
        return;
    }

    QString mic = m_comboMic->currentText();
    QString speaker = m_comboSpeaker->currentText();
    if (mic.isEmpty() || speaker.isEmpty()) return;

    // 1. Биндим UDP сокет на случайный порт (ОС сама выберет свободный)
    if (!m_udpSocket->bind(0, QUdpSocket::ShareAddress)) {
        qWarning() << "[VOICE] Failed to bind UDP socket";
        return;
    }
    m_localUdpPort = m_udpSocket->localPort();
    qDebug() << "[VOICE] Local UDP port:" << m_localUdpPort;

    // 2. Запускаем аудио
    m_audioManager->startCapture(mic);
    m_audioManager->startPlayback(speaker);

    // 3. Соединяем Микрофон -> UDP
    QIODevice* captureDev = m_audioManager->getCaptureDevice();
    if (captureDev) {
        connect(captureDev, &QIODevice::readyRead, this, &MainWindow::onAudioCaptured);
    }

    // 4. Соединяем UDP -> Динамик
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &MainWindow::onUdpAudioReceived);

    // 5. Говорим серверу, что мы в комнате
    QJsonObject data;
    data["udp_port"] = m_localUdpPort;
    m_client->sendCommand("JOIN_VOICE", data, 0);

    // 🔹 ИЗМЕНЕНО: Разблокируем элементы
    m_inVoiceRoom = true;
    m_btnVoiceRoom->setText("🔇 Выйти из комнаты");
    m_btnVoiceRoom->setStyleSheet("background-color: #f44336; color: white; font-weight: bold;");
    
    m_btnMute->setEnabled(true);
    m_volumeSlider->setEnabled(true);
    // ComboBox'ы остаются активными для Hot-swap!
}

void MainWindow::onLeaveVoiceRoom() {
    if (!m_inVoiceRoom) return;
    
    if (!m_voiceToken.isEmpty()) {
        QByteArray unregisterPacket = "UNREGISTER:" + m_currentUser.toUtf8() + "\n";
        m_udpSocket->writeDatagram(unregisterPacket, QHostAddress(m_client->getHost()), m_client->getUdpPort());
    }

    // 1. Останавливаем аудио
    m_audioManager->stopCapture();
    m_audioManager->stopPlayback();

    // 2. Отключаем сигналы
    disconnect(m_udpSocket, &QUdpSocket::readyRead, this, &MainWindow::onUdpAudioReceived);

    // 3. Говорим серверу
    m_client->sendCommand("LEAVE_VOICE", {}, 0);

    // 4. Освобождаем порт
    m_udpSocket->close();
    m_localUdpPort = 0;

    // 5. Меняем UI
    m_voiceToken.clear();
    m_inVoiceRoom = false;
    m_btnVoiceRoom->setText("🎙️ Войти в комнату");
    m_btnVoiceRoom->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
    
    m_btnMute->setEnabled(false);
    m_btnMute->setChecked(false);
    m_volumeSlider->setEnabled(false);
    m_isMuted = false;
}

void MainWindow::onAudioCaptured() {
    if (m_isMuted) {
        // Если замьючены, просто сбрасываем буфер, не отправляя
        QIODevice* captureDev = m_audioManager->getCaptureDevice();
        if (captureDev) captureDev->readAll();
        return;
    }

    QIODevice* captureDev = m_audioManager->getCaptureDevice();
    if (!captureDev) return;

    QByteArray data = captureDev->readAll();
    if (!data.isEmpty()) {
        m_udpSocket->writeDatagram(data, QHostAddress(m_client->getHost()), m_client->getUdpPort());
    }
}

void MainWindow::onUdpAudioReceived() {
    // Читаем байты из сети и скармливаем динамику
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        m_udpSocket->readDatagram(datagram.data(), datagram.size());
        
        m_audioManager->playAudioData(datagram);
    }
}

void MainWindow::onMuteToggled(bool checked) {
    m_isMuted = checked;
    m_btnMute->setText(checked ? "Unmute" : "Mute");
    m_btnMute->setStyleSheet(checked ? "background-color: #ff9800; color: white;" : "");
    qDebug() << "[VOICE] Mute:" << checked;
}

void MainWindow::onVolumeChanged(int value) {
    qreal volume = value / 100.0;
    m_audioManager->setVolume(volume);
}

void MainWindow::onMicChanged(int index) {
    if (!m_inVoiceRoom || index < 0) return;
    qDebug() << "[VOICE] Hot-swap Mic to:" << m_comboMic->currentText();
    
    // Перезапуск захвата
    m_audioManager->stopCapture();
    m_audioManager->startCapture(m_comboMic->currentText());
    
    QIODevice* dev = m_audioManager->getCaptureDevice();
    if (dev) {
        connect(dev, &QIODevice::readyRead, this, &MainWindow::onAudioCaptured);
    }
}

void MainWindow::onSpeakerChanged(int index) {
    if (!m_inVoiceRoom || index < 0) return;
    qDebug() << "[VOICE] Hot-swap Speaker to:" << m_comboSpeaker->currentText();
    
    // Перезапуск воспроизведения
    m_audioManager->stopPlayback();
    m_audioManager->startPlayback(m_comboSpeaker->currentText());
}

void MainWindow::onAudioDevicesChanged() {
    qDebug() << "[VOICE] Audio devices changed in system";
    
    QString currentMic = m_comboMic->currentText();
    QString currentSpeaker = m_comboSpeaker->currentText();
    
    m_comboMic->blockSignals(true);
    m_comboSpeaker->blockSignals(true);
    
    m_comboMic->clear();
    m_comboMic->addItems(m_audioManager->getMicrophones());
    m_comboSpeaker->clear();
    m_comboSpeaker->addItems(m_audioManager->getSpeakers());
    
    // Пытаемся восстановить выбор
    int micIdx = m_comboMic->findText(currentMic);
    if (micIdx >= 0) {
        m_comboMic->setCurrentIndex(micIdx);
    } else if (m_comboMic->count() > 0) {
        // 🔹 Старое устройство исчезло — переключаемся на первое доступное
        m_comboMic->setCurrentIndex(0);
        if (m_inVoiceRoom) {
            qDebug() << "[VOICE] Mic device lost, switching to:" << m_comboMic->currentText();
            onMicChanged(0);
        }
    }
    
    int spkIdx = m_comboSpeaker->findText(currentSpeaker);
    if (spkIdx >= 0) {
        m_comboSpeaker->setCurrentIndex(spkIdx);
    } else if (m_comboSpeaker->count() > 0) {
        //  Старое устройство исчезло — переключаемся на первое доступное
        m_comboSpeaker->setCurrentIndex(0);
        if (m_inVoiceRoom) {
            qDebug() << "[VOICE] Speaker device lost, switching to:" << m_comboSpeaker->currentText();
            onSpeakerChanged(0);
        }
    }
    
    m_comboMic->blockSignals(false);
    m_comboSpeaker->blockSignals(false);
}