#include "audio_manager.h"
#include <QDebug>

AudioManager::AudioManager(QObject* parent) : QObject(parent) {}

AudioManager::~AudioManager() {
    stopCapture();
    stopPlayback();
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

void AudioManager::startCapture(const QString& micDesc) {
    stopCapture();

    QAudioDevice micDevice;
    for (const auto& dev : QMediaDevices::audioInputs()) {
        if (dev.description() == micDesc) { micDevice = dev; break; }
    }

    if (micDevice.isNull()) {
        qWarning() << "[AUDIO] Mic not found:" << micDesc;
        return;
    }

    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    if (!micDevice.isFormatSupported(format)) {
        format = micDevice.preferredFormat();
    }

    m_source = new QAudioSource(micDevice, format, this);
    m_inputDevice = m_source->start();
    qDebug() << "[AUDIO] Capture started:" << micDesc;
}

void AudioManager::startPlayback(const QString& speakerDesc) {
    stopPlayback();

    QAudioDevice speakerDevice;
    for (const auto& dev : QMediaDevices::audioOutputs()) {
        if (dev.description() == speakerDesc) { speakerDevice = dev; break; }
    }

    if (speakerDevice.isNull()) {
        qWarning() << "[AUDIO] Speaker not found:" << speakerDesc;
        return;
    }

    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    if (!speakerDevice.isFormatSupported(format)) {
        format = speakerDevice.preferredFormat();
    }

    m_sink = new QAudioSink(speakerDevice, format, this);
    m_outputDevice = m_sink->start();
    qDebug() << "[AUDIO] Playback started:" << speakerDesc;
}

void AudioManager::stopCapture() {
    if (m_source) { m_source->stop(); delete m_source; m_source = nullptr; }
    m_inputDevice = nullptr;
}

void AudioManager::stopPlayback() {
    if (m_sink) { m_sink->stop(); delete m_sink; m_sink = nullptr; }
    m_outputDevice = nullptr;
}

void AudioManager::playAudioData(const QByteArray& data) {
    if (m_outputDevice) {
        m_outputDevice->write(data);
    }
}

void AudioManager::setVolume(qreal volume) {
    if (m_sink) {
        m_sink->setVolume(volume);
    }
}