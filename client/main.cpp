#include <QApplication>
#include <QSettings>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include "tcp_client.h"
#include "mainwindow.h"

// Глобальный файл для логирования
QFile* g_logFile = nullptr;

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QString logMessage = QString("[%1] %2").arg(
        QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"),
        msg
    );
    
    // Вывод в консоль (если есть)
    fprintf(stderr, "%s\n", logMessage.toLocal8Bit().constData());
    
    // Запись в файл
    if (g_logFile && g_logFile->isOpen()) {
        QTextStream stream(g_logFile);
        stream << logMessage << "\n";
        stream.flush();
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Читаем настройки
    QString configPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/config.ini");
    QSettings settings(configPath, QSettings::IniFormat);
    
    QString serverHost = settings.value("server/host", "127.0.0.1").toString();
    quint16 tcpPort = settings.value("server/tcp_port", 9000).toUInt();
    quint16 udpPort = settings.value("server/udp_port", 9001).toUInt();
    
    qDebug() << "[CLIENT] Config loaded:" << configPath;
    qDebug() << "[CLIENT] Server:" << serverHost << "TCP:" << tcpPort << "UDP:" << udpPort;
    
    // Настраиваем логирование в файл
    QString logFile = settings.value("logging/file", "voice_client.log").toString();
    g_logFile = new QFile(logFile);
    if (g_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        qInstallMessageHandler(messageHandler);
        qDebug() << "[CLIENT] Logging to:" << logFile;
    } else {
        qWarning() << "[CLIENT] Failed to open log file:" << logFile;
    }
    
    // Создаем клиент с настройками из конфига
    TcpClient client(serverHost, tcpPort, udpPort);
    MainWindow window(&client);
    window.show();
    
    return app.exec();
}