#include "audio_manager.h"
#include <QDebug>

AudioManager::AudioManager(QObject* parent) : QObject(parent) {}

AudioManager::~AudioManager() {
    stopLoopback();
}

QStringList AudioManager::getMicrophones() const {
    QStringList list;
    for (const QAudioDevice& device : QMediaDevices::audioInputs()) {
        list << device.description();
    }
    return list;
}

QStringList AudioManager::getSpeakers() const {
    QStringList list;
    for (const QAudioDevice& device : QMediaDevices::audioOutputs()) {
        list << device.description();
    }
    return list;
}

void AudioManager::startLoopback(const QString& micDesc, const QString& speakerDesc) {
    stopLoopback();

    // Находим устройства по описанию
    QAudioDevice micDevice, speakerDevice;
    for (const auto& dev : QMediaDevices::audioInputs()) {
        if (dev.description() == micDesc) {
            micDevice = dev;
            break;
        }
    }
    for (const auto& dev : QMediaDevices::audioOutputs()) {
        if (dev.description() == speakerDesc) {
            speakerDevice = dev;
            break;
        }
    }

    // 🔹 В Qt6 проверка через isNull()
    if (micDevice.isNull() || speakerDevice.isNull()) {
        qWarning() << "[AUDIO] Invalid devices selected! mic:" << micDesc << "speaker:" << speakerDesc;
        return;
    }

    // Настраиваем формат аудио (моно, 16 бит, 48000 Гц — стандарт для голоса)
    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    if (!micDevice.isFormatSupported(format)) {
        qWarning() << "[AUDIO] Format not supported, using preferred";
        format = micDevice.preferredFormat();
    }

    // Создаём источники и приёмники
    m_audioSource = new QAudioSource(micDevice, format, this);
    m_audioSink = new QAudioSink(speakerDevice, format, this);

    // 🔹 МАГИЯ: соединяем поток данных напрямую
    m_ioDevice = m_audioSource->start();
    m_audioSink->start(m_ioDevice);

    qDebug() << "[AUDIO] Loopback started:" << micDesc << "->" << speakerDesc
             << "| format:" << format.sampleRate() << "Hz," << format.channelCount() << "ch";
}

void AudioManager::stopLoopback() {
    if (m_audioSource) {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
    }
    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
    }
    m_ioDevice = nullptr;
    qDebug() << "[AUDIO] Loopback stopped";
}