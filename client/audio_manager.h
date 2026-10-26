#pragma once
#include <QObject>
#include <QAudioSource>
#include <QAudioSink>
#include <QMediaDevices>
#include <QAudioFormat>
#include <QStringList>
#include <QIODevice>

class AudioManager : public QObject {
    Q_OBJECT
public:
    explicit AudioManager(QObject* parent = nullptr);
    ~AudioManager();

    QStringList getMicrophones() const;
    QStringList getSpeakers() const;

    void startCapture(const QString& micDesc);
    void startPlayback(const QString& speakerDesc);
    
    void stopCapture();
    void stopPlayback();

    void playAudioData(const QByteArray& data);
    QIODevice* getCaptureDevice() const { return m_inputDevice; }
    QIODevice* getPlaybackDevice() const { return m_outputDevice; }

    void setVolume(qreal volume);
    bool isMuted() const { return m_isMuted; }
    void setMuted(bool muted) { m_isMuted = muted; }

private:
    QAudioSource* m_source = nullptr;
    QAudioSink* m_sink = nullptr;
    QIODevice* m_inputDevice = nullptr;
    QIODevice* m_outputDevice = nullptr;
    bool m_isMuted = false;
};