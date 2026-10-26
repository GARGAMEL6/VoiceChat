#pragma once
#include <QObject>
#include <QAudioSource>
#include <QAudioSink>
#include <QMediaDevices>
#include <QAudioFormat>
#include <QStringList>

class AudioManager : public QObject {
    Q_OBJECT
public:
    explicit AudioManager(QObject* parent = nullptr);
    ~AudioManager();

    // Получить списки названий устройств для ComboBox
    QStringList getMicrophones() const;
    QStringList getSpeakers() const;

    // Запустить захват с микрофона и вывод в динамик
    void startLoopback(const QString& micDesc, const QString& speakerDesc);
    
    // Остановить и освободить ресурсы
    void stopLoopback();

private:
    QAudioSource* m_audioSource = nullptr;
    QAudioSink* m_audioSink = nullptr;
    QIODevice* m_ioDevice = nullptr;
};