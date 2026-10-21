#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QDebug>

int main(int argc, char *argv[]) {
    // 1. Инициализируем GUI-приложение (вместо QCoreApplication)
    QApplication app(argc, argv);

    qDebug() << "[CLIENT] Qt GUI initialized. Version:" << qVersion();

    // 2. Создаём простое тестовое окно
    QMainWindow window;
    window.setWindowTitle("Voice Client (Test)");
    window.resize(400, 300);

    QLabel* label = new QLabel("Hello, Qt Widgets!", &window);
    label->setAlignment(Qt::AlignCenter);
    window.setCentralWidget(label);

    // 3. Показываем окно и запускаем Event Loop
    window.show();
    
    qDebug() << "[CLIENT] Entering event loop. Close window to exit.";
    return app.exec();
}